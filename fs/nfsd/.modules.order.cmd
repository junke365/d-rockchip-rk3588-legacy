cmd_fs/nfsd/modules.order := {  :; } | awk '!x[$$0]++' - > fs/nfsd/modules.order
