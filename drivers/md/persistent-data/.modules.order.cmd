cmd_drivers/md/persistent-data/modules.order := {  :; } | awk '!x[$$0]++' - > drivers/md/persistent-data/modules.order
