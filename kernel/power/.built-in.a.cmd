cmd_kernel/power/built-in.a := rm -f kernel/power/built-in.a; aarch64-none-linux-gnu-ar cDPrST kernel/power/built-in.a kernel/power/qos.o kernel/power/main.o kernel/power/console.o kernel/power/process.o kernel/power/suspend.o kernel/power/poweroff.o kernel/power/energy_model.o
