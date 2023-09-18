/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2019 Philippe Gerum.
 */
#ifndef _ASM_X86_DOVETAIL_H
#define _ASM_X86_DOVETAIL_H

#if !defined(__ASSEMBLY__) && defined(CONFIG_DOVETAIL)

#include <asm/fpu/api.h>
#include <asm/io_bitmap.h>

static inline void arch_dovetail_exec_prepare(void)
{
	clear_thread_flag(TIF_NEED_FPU_LOAD);
}

static inline
void arch_dovetail_switch_prepare(bool leave_inband)
{
	if (leave_inband)
		fpu__suspend_inband();
}

static inline
void arch_dovetail_switch_finish(bool enter_inband)
{
	unsigned int ti_work = READ_ONCE(current_thread_info()->flags);

	if (unlikely(ti_work & _TIF_IO_BITMAP))
		tss_update_io_bitmap();

	if (enter_inband) {
		fpu__resume_inband();
	} else {
		  if (unlikely(ti_work & _TIF_NEED_FPU_LOAD &&
				  !(current->flags & PF_KTHREAD)))
			  switch_fpu_return();
	}
}

#endif

#endif /* _ASM_X86_DOVETAIL_H */
