cmd_drivers/net/vxlan/modules.order := {   echo drivers/net/vxlan/vxlan.ko; :; } | awk '!x[$$0]++' - > drivers/net/vxlan/modules.order
