cmd_drivers/net/ethernet/modules.order := {   cat drivers/net/ethernet/alacritech/modules.order;   cat drivers/net/ethernet/amazon/modules.order;   cat drivers/net/ethernet/aquantia/modules.order;   cat drivers/net/ethernet/aurora/modules.order;   cat drivers/net/ethernet/cadence/modules.order;   cat drivers/net/ethernet/cortina/modules.order;   cat drivers/net/ethernet/google/modules.order;   cat drivers/net/ethernet/huawei/modules.order;   cat drivers/net/ethernet/mscc/modules.order;   cat drivers/net/ethernet/neterion/modules.order;   cat drivers/net/ethernet/netronome/modules.order;   cat drivers/net/ethernet/ni/modules.order;   cat drivers/net/ethernet/packetengines/modules.order;   cat drivers/net/ethernet/realtek/modules.order;   cat drivers/net/ethernet/sfc/modules.order;   cat drivers/net/ethernet/socionext/modules.order;   cat drivers/net/ethernet/stmicro/modules.order;   cat drivers/net/ethernet/xilinx/modules.order;   cat drivers/net/ethernet/pensando/modules.order; :; } | awk '!x[$$0]++' - > drivers/net/ethernet/modules.order
