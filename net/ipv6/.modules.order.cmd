cmd_net/ipv6/modules.order := {   cat net/ipv6/netfilter/modules.order;   echo net/ipv6/ip6_udp_tunnel.ko; :; } | awk '!x[$$0]++' - > net/ipv6/modules.order
