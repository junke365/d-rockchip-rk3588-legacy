cmd_net/netfilter/modules.order := {   echo net/netfilter/xt_ipvs.ko;   cat net/netfilter/ipvs/modules.order; :; } | awk '!x[$$0]++' - > net/netfilter/modules.order
