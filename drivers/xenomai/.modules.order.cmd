cmd_drivers/xenomai/modules.order := {   cat drivers/xenomai/autotune/modules.order;   cat drivers/xenomai/serial/modules.order;   cat drivers/xenomai/testing/modules.order;   cat drivers/xenomai/can/modules.order;   cat drivers/xenomai/net/modules.order;   cat drivers/xenomai/analogy/modules.order;   cat drivers/xenomai/ipc/modules.order;   cat drivers/xenomai/udd/modules.order;   cat drivers/xenomai/gpio/modules.order;   cat drivers/xenomai/gpiopwm/modules.order;   cat drivers/xenomai/spi/modules.order; :; } | awk '!x[$$0]++' - > drivers/xenomai/modules.order
