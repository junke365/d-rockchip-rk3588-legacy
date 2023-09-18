cmd_net/netfilter/ipvs/modules.order := {   echo net/netfilter/ipvs/ip_vs.ko;   echo net/netfilter/ipvs/ip_vs_rr.ko; :; } | awk '!x[$$0]++' - > net/netfilter/ipvs/modules.order
