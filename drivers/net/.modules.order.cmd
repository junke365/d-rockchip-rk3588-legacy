cmd_drivers/net/modules.order := {   cat drivers/net/ipvlan/modules.order;   cat drivers/net/phy/modules.order;   cat drivers/net/mdio/modules.order;   cat drivers/net/pcs/modules.order;   cat drivers/net/can/modules.order;   cat drivers/net/dsa/modules.order;   cat drivers/net/ethernet/modules.order;   cat drivers/net/wireless/modules.order;   cat drivers/net/usb/modules.order;   cat drivers/net/vxlan/modules.order; :; } | awk '!x[$$0]++' - > drivers/net/modules.order
