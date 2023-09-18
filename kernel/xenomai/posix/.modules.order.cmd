cmd_kernel/xenomai/posix/modules.order := {  :; } | awk '!x[$$0]++' - > kernel/xenomai/posix/modules.order
