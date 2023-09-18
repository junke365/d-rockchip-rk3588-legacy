// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Copyright (C) 1994 Linus Torvalds
 *
 *  Pentium III FXSR, SSE support
 *  General FPU state handling cleanups
 *	Gareth Hughes <gareth@valinux.com>, May 2000
 */
#include <asm/fpu/internal.h>
#include <asm/fpu/regset.h>
#include <asm/fpu/signal.h>
#include <asm/fpu/types.h>
#include <asm/traps.h>
#include <asm/irq_regs.h>

#include <linux/hardirq.h>
#include <linux/pkeys.h>
#include <linux/cpuhotplug.h>

#define CREATE_TRACE_POINTS
#include <asm/trace/fpu.h>

/*
 * Represents the initial FPU state. It's mostly (but not completely) zeroes,
 * depending on the FPU hardware format:
 */
union fpregs_state init_fpstate __read_mostly;

/* Track in-kernel FPU usage */
static DEFINE_PER_CPU(bool, in_kernel_fpu);

/*
 * Track which context is using the FPU on the CPU:
 */
DEFINE_PER_CPU(struct fpu *, fpu_fpregs_owner_ctx);

/*
 * Can we use the FPU in kernel mode with the
 * whole "kernel_fpu_begin/end()" sequence?
 */
bool irq_fpu_usable(void)
{
	if (running_oob())
		return false;

	if (WARN_ON_ONCE(in_nmi()))
		return false;

	/* In kernel FPU usage already active? */
	if (this_cpu_read(in_kernel_fpu))
		return false;

	/*
	 * When not in NMI or hard interrupt context, FPU can be used in:
	 *
	 * - Task context except from within fpregs_lock()'ed critical
	 *   regions.
	 *
	 * - Soft interrupt processing context which cannot happen
	 *   while in a fpregs_lock()'ed critical region.
	 */
	if (!in_irq())
		return true;

	/*
	 * In hard interrupt context it's safe when soft interrupts
	 * are enabled, which means the interrupt did not hit in
	 * a fpregs_lock()'ed critical region.
	 */
	return !softirq_count();
}
EXPORT_SYMBOL(irq_fpu_usable);

/*
 * These must be called with preempt disabled. Returns
 * 'true' if the FPU state is still intact and we can
 * keep registers active.
 *
 * The legacy FNSAVE instruction cleared all FPU state
 * unconditionally, so registers are essentially destroyed.
 * Modern FPU state can be kept in registers, if there are
 * no pending FP exceptions.
 */
int copy_fpregs_to_fpstate(struct fpu *fpu)
{
	if (likely(use_xsave())) {
		copy_xregs_to_kernel(&fpu->state.xsave);

		/*
		 * AVX512 state is tracked here because its use is
		 * known to slow the max clock speed of the core.
		 */
		if (fpu->state.xsave.header.xfeatures & XFEATURE_MASK_AVX512)
			fpu->avx512_timestamp = jiffies;
		return 1;
	}

	if (likely(use_fxsr())) {
		copy_fxregs_to_kernel(fpu);
		return 1;
	}

	/*
	 * Legacy FPU register saving, FNSAVE always clears FPU registers,
	 * so we have to mark them inactive:
	 */
	asm volatile("fnsave %[fp]; fwait" : [fp] "=m" (fpu->state.fsave));

	return 0;
}
EXPORT_SYMBOL(copy_fpregs_to_fpstate);

void kernel_fpu_begin_mask(unsigned int kfpu_mask)
{
	unsigned long flags;

	preempt_disable();

	WARN_ON_FPU(!irq_fpu_usable());
	WARN_ON_FPU(this_cpu_read(in_kernel_fpu));

	flags = hard_cond_local_irq_save();

	this_cpu_write(in_kernel_fpu, true);

	if (!(current->flags & PF_KTHREAD) &&
	    !test_thread_flag(TIF_NEED_FPU_LOAD)) {
		set_thread_flag(TIF_NEED_FPU_LOAD);
		/*
		 * Ignore return value -- we don't care if reg state
		 * is clobbered.
		 */
		copy_fpregs_to_fpstate(&current->thread.fpu);
	}

	__cpu_invalidate_fpregs_state();

	/* Put sane initial values into the control registers. */
	if (likely(kfpu_mask & KFPU_MXCSR) && boot_cpu_has(X86_FEATURE_XMM))
		ldmxcsr(MXCSR_DEFAULT);

	if (unlikely(kfpu_mask & KFPU_387) && boot_cpu_has(X86_FEATURE_FPU))
		asm volatile ("fninit");

	hard_cond_local_irq_restore(flags);
}
EXPORT_SYMBOL_GPL(kernel_fpu_begin_mask);

void kernel_fpu_end(void)
{
	WARN_ON_FPU(!this_cpu_read(in_kernel_fpu));

	this_cpu_write(in_kernel_fpu, false);
	preempt_enable();
}
EXPORT_SYMBOL_GPL(kernel_fpu_end);

/*
 * Save the FPU state (mark it for reload if necessary):
 *
 * This only ever gets called for the current task.
 */
void fpu__save(struct fpu *fpu)
{
	unsigned long flags;

	WARN_ON_FPU(fpu != &current->thread.fpu);

	flags = fpregs_lock();
	trace_x86_fpu_before_save(fpu);

	if (!test_thread_flag(TIF_NEED_FPU_LOAD)) {
		if (!copy_fpregs_to_fpstate(fpu)) {
			copy_kernel_to_fpregs(&fpu->state);
		}
	}

	trace_x86_fpu_after_save(fpu);
	fpregs_unlock(flags);
}

/*
 * Legacy x87 fpstate state init:
 */
static inline void fpstate_init_fstate(struct fregs_state *fp)
{
	fp->cwd = 0xffff037fu;
	fp->swd = 0xffff0000u;
	fp->twd = 0xffffffffu;
	fp->fos = 0xffff0000u;
}

void fpstate_init(union fpregs_state *state)
{
	if (!static_cpu_has(X86_FEATURE_FPU)) {
		fpstate_init_soft(&state->soft);
		return;
	}

	memset(state, 0, fpu_kernel_xstate_size);

	if (static_cpu_has(X86_FEATURE_XSAVES))
		fpstate_init_xstate(&state->xsave);
	if (static_cpu_has(X86_FEATURE_FXSR))
		fpstate_init_fxstate(&state->fxsave);
	else
		fpstate_init_fstate(&state->fsave);
}
EXPORT_SYMBOL_GPL(fpstate_init);

int fpu__copy(struct task_struct *dst, struct task_struct *src)
{
	struct fpu *dst_fpu = &dst->thread.fpu;
	struct fpu *src_fpu = &src->thread.fpu;
	unsigned long flags;

	dst_fpu->last_cpu = -1;

	if (!static_cpu_has(X86_FEATURE_FPU))
		return 0;

	WARN_ON_FPU(src_fpu != &current->thread.fpu);

	/*
	 * Don't let 'init optimized' areas of the XSAVE area
	 * leak into the child task:
	 */
	memset(&dst_fpu->state.xsave, 0, fpu_kernel_xstate_size);

	/*
	 * If the FPU registers are not current just memcpy() the state.
	 * Otherwise save current FPU registers directly into the child's FPU
	 * context, without any memory-to-memory copying.
	 *
	 * ( The function 'fails' in the FNSAVE case, which destroys
	 *   register contents so we have to load them back. )
	 */
	flags = fpregs_lock();
	if (test_thread_flag(TIF_NEED_FPU_LOAD))
		memcpy(&dst_fpu->state, &src_fpu->state, fpu_kernel_xstate_size);

	else if (!copy_fpregs_to_fpstate(dst_fpu))
		copy_kernel_to_fpregs(&dst_fpu->state);

	fpregs_unlock(flags);

	set_tsk_thread_flag(dst, TIF_NEED_FPU_LOAD);

	trace_x86_fpu_copy_src(src_fpu);
	trace_x86_fpu_copy_dst(dst_fpu);

	return 0;
}

/*
 * Activate the current task's in-memory FPU context,
 * if it has not been used before:
 */
static void fpu__initialize(struct fpu *fpu)
{
	WARN_ON_FPU(fpu != &current->thread.fpu);

	set_thread_flag(TIF_NEED_FPU_LOAD);
	fpstate_init(&fpu->state);
	trace_x86_fpu_init_state(fpu);
}

/*
 * This function must be called before we read a task's fpstate.
 *
 * There's two cases where this gets called:
 *
 * - for the current task (when coredumping), in which case we have
 *   to save the latest FPU registers into the fpstate,
 *
 * - or it's called for stopped tasks (ptrace), in which case the
 *   registers were already saved by the context-switch code when
 *   the task scheduled out.
 *
 * If the task has used the FPU before then save it.
 */
void fpu__prepare_read(struct fpu *fpu)
{
	if (fpu == &current->thread.fpu)
		fpu__save(fpu);
}

/*
 * This function must be called before we write a task's fpstate.
 *
 * Invalidate any cached FPU registers.
 *
 * After this function call, after registers in the fpstate are
 * modified and the child task has woken up, the child task will
 * restore the modified FPU state from the modified context. If we
 * didn't clear its cached status here then the cached in-registers
 * state pending on its former CPU could be restored, corrupting
 * the modifications.
 */
void fpu__prepare_write(struct fpu *fpu)
{
	/*
	 * Only stopped child tasks can be used to modify the FPU
	 * state in the fpstate buffer:
	 */
	WARN_ON_FPU(fpu == &current->thread.fpu);

	/* Invalidate any cached state: */
	__fpu_invalidate_fpregs_state(fpu);
}

/*
 * Drops current FPU state: deactivates the fpregs and
 * the fpstate. NOTE: it still leaves previous contents
 * in the fpregs in the eager-FPU case.
 *
 * This function can be used in cases where we know that
 * a state-restore is coming: either an explicit one,
 * or a reschedule.
 */
void fpu__drop(struct fpu *fpu)
{
	unsigned long flags;

	flags = hard_preempt_disable();

	if (fpu == &current->thread.fpu) {
		/* Ignore delayed exceptions from user space */
		asm volatile("1: fwait\n"
			     "2:\n"
			     _ASM_EXTABLE(1b, 2b));
		fpregs_deactivate(fpu);
	}

	trace_x86_fpu_dropped(fpu);

	hard_preempt_enable(flags);
}

/*
 * Clear FPU registers by setting them up from the init fpstate.
 * Caller must do fpregs_[un]lock() around it.
 */
static inline void copy_init_fpstate_to_fpregs(u64 features_mask)
{
	if (use_xsave())
		copy_kernel_to_xregs(&init_fpstate.xsave, features_mask);
	else if (static_cpu_has(X86_FEATURE_FXSR))
		copy_kernel_to_fxregs(&init_fpstate.fxsave);
	else
		copy_kernel_to_fregs(&init_fpstate.fsave);

	if (boot_cpu_has(X86_FEATURE_OSPKE))
		copy_init_pkru_to_fpregs();
}

/*
 * Clear the FPU state back to init state.
 *
 * Called by sys_execve(), by the signal handler code and by various
 * error paths.
 */
static void fpu__clear(struct fpu *fpu, bool user_only)
{
	unsigned long flags;

	WARN_ON_FPU(fpu != &current->thread.fpu);

	if (!static_cpu_has(X86_FEATURE_FPU)) {
		flags = hard_cond_local_irq_save();
		fpu__drop(fpu);
		fpu__initialize(fpu);
		hard_cond_local_irq_restore(flags);
		return;
	}

	flags = fpregs_lock();

	if (user_only) {
		if (!fpregs_state_valid(fpu, smp_processor_id()) &&
		    xfeatures_mask_supervisor())
			copy_kernel_to_xregs(&fpu->state.xsave,
					     xfeatures_mask_supervisor());
		copy_init_fpstate_to_fpregs(xfeatures_mask_user());
	} else {
		copy_init_fpstate_to_fpregs(xfeatures_mask_all);
	}

	fpregs_mark_activate();
	fpregs_unlock(flags);
}

void fpu__clear_user_states(struct fpu *fpu)
{
	fpu__clear(fpu, true);
}

void fpu__clear_all(struct fpu *fpu)
{
	fpu__clear(fpu, false);
}

/*
 * Load FPU context before returning to userspace.
 */
void switch_fpu_return(void)
{
	unsigned long flags;

	if (!static_cpu_has(X86_FEATURE_FPU))
		return;

	flags = hard_cond_local_irq_save();
	__fpregs_load_activate();
	hard_cond_local_irq_restore(flags);
}
EXPORT_SYMBOL_GPL(switch_fpu_return);

#ifdef CONFIG_X86_DEBUG_FPU
/*
 * If current FPU state according to its tracking (loaded FPU context on this
 * CPU) is not valid then we must have TIF_NEED_FPU_LOAD set so the context is
 * loaded on return to userland.
 */
void fpregs_assert_state_consistent(void)
{
	struct fpu *fpu = &current->thread.fpu;

	if (test_thread_flag(TIF_NEED_FPU_LOAD))
		return;

	WARN_ON_FPU(!fpregs_state_valid(fpu, smp_processor_id()));
}
EXPORT_SYMBOL_GPL(fpregs_assert_state_consistent);
#endif

void fpregs_mark_activate(void)
{
	struct fpu *fpu = &current->thread.fpu;

	fpregs_activate(fpu);
	fpu->last_cpu = smp_processor_id();
	clear_thread_flag(TIF_NEED_FPU_LOAD);
}
EXPORT_SYMBOL_GPL(fpregs_mark_activate);

/*
 * x87 math exception handling:
 */

int fpu__exception_code(struct fpu *fpu, int trap_nr)
{
	int err;

	if (trap_nr == X86_TRAP_MF) {
		unsigned short cwd, swd;
		/*
		 * (~cwd & swd) will mask out exceptions that are not set to unmasked
		 * status.  0x3f is the exception bits in these regs, 0x200 is the
		 * C1 reg you need in case of a stack fault, 0x040 is the stack
		 * fault bit.  We should only be taking one exception at a time,
		 * so if this combination doesn't produce any single exception,
		 * then we have a bad program that isn't synchronizing its FPU usage
		 * and it will suffer the consequences since we won't be able to
		 * fully reproduce the context of the exception.
		 */
		if (boot_cpu_has(X86_FEATURE_FXSR)) {
			cwd = fpu->state.fxsave.cwd;
			swd = fpu->state.fxsave.swd;
		} else {
			cwd = (unsigned short)fpu->state.fsave.cwd;
			swd = (unsigned short)fpu->state.fsave.swd;
		}

		err = swd & ~cwd;
	} else {
		/*
		 * The SIMD FPU exceptions are handled a little differently, as there
		 * is only a single status/control register.  Thus, to determine which
		 * unmasked exception was caught we must mask the exception mask bits
		 * at 0x1f80, and then use these to mask the exception bits at 0x3f.
		 */
		unsigned short mxcsr = MXCSR_DEFAULT;

		if (boot_cpu_has(X86_FEATURE_XMM))
			mxcsr = fpu->state.fxsave.mxcsr;

		err = ~(mxcsr >> 7) & mxcsr;
	}

	if (err & 0x001) {	/* Invalid op */
		/*
		 * swd & 0x240 == 0x040: Stack Underflow
		 * swd & 0x240 == 0x240: Stack Overflow
		 * User must clear the SF bit (0x40) if set
		 */
		return FPE_FLTINV;
	} else if (err & 0x004) { /* Divide by Zero */
		return FPE_FLTDIV;
	} else if (err & 0x008) { /* Overflow */
		return FPE_FLTOVF;
	} else if (err & 0x012) { /* Denormal, Underflow */
		return FPE_FLTUND;
	} else if (err & 0x020) { /* Precision */
		return FPE_FLTRES;
	}

	/*
	 * If we're using IRQ 13, or supposedly even some trap
	 * X86_TRAP_MF implementations, it's possible
	 * we get a spurious trap, which is not an error.
	 */
	return 0;
}

#ifdef CONFIG_DOVETAIL

/*
 * Holds the in-kernel fpu state when preempted by a task running on
 * the out-of-band stage.
 */
static DEFINE_PER_CPU(struct fpu *, in_kernel_fpstate);

static int fpu__init_kernel_fpstate(unsigned int cpu)
{
	struct fpu *fpu;

	fpu = kzalloc(sizeof(*fpu) + fpu_kernel_xstate_size, GFP_KERNEL);
	if (fpu == NULL)
		return -ENOMEM;

	this_cpu_write(in_kernel_fpstate, fpu);
	fpstate_init(&fpu->state);

	return 0;
}

static int fpu__drop_kernel_fpstate(unsigned int cpu)
{
	struct fpu *fpu = this_cpu_read(in_kernel_fpstate);

	kfree(fpu);

	return 0;
}

void fpu__suspend_inband(void)
{
	struct fpu *kfpu = this_cpu_read(in_kernel_fpstate);
	struct task_struct *tsk = current;

	if (this_cpu_read(in_kernel_fpu)) {
		copy_fpregs_to_fpstate(kfpu);
		__cpu_invalidate_fpregs_state();
		oob_fpu_set_preempt(&tsk->thread.fpu);
	}
}

void fpu__resume_inband(void)
{
	struct fpu *kfpu = this_cpu_read(in_kernel_fpstate);
	struct task_struct *tsk = current;

	if (oob_fpu_preempted(&tsk->thread.fpu)) {
		copy_kernel_to_fpregs(&kfpu->state);
		__cpu_invalidate_fpregs_state();
		oob_fpu_clear_preempt(&tsk->thread.fpu);
	} else if (!(tsk->flags & PF_KTHREAD) &&
		test_thread_flag(TIF_NEED_FPU_LOAD))
		switch_fpu_return();
}

static void __init fpu__init_dovetail(void)
{
	cpuhp_setup_state(CPUHP_AP_ONLINE_DYN,
			"platform/x86/dovetail:online",
			fpu__init_kernel_fpstate, fpu__drop_kernel_fpstate);
}
core_initcall(fpu__init_dovetail);

#endif