cmd_drivers/net/ipvlan/modules.order := {  :; } | awk '!x[$$0]++' - > drivers/net/ipvlan/modules.order
