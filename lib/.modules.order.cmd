cmd_lib/modules.order := {   cat lib/math/modules.order;   cat lib/crypto/modules.order;   cat lib/zlib_inflate/modules.order;   cat lib/zlib_deflate/modules.order;   cat lib/reed_solomon/modules.order;   cat lib/lzo/modules.order;   cat lib/lz4/modules.order;   cat lib/zstd/modules.order;   cat lib/xz/modules.order;   cat lib/raid6/modules.order;   cat lib/mpi/modules.order;   cat lib/fonts/modules.order; :; } | awk '!x[$$0]++' - > lib/modules.order
