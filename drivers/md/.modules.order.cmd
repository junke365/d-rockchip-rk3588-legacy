cmd_drivers/md/modules.order := {   cat drivers/md/persistent-data/modules.order; :; } | awk '!x[$$0]++' - > drivers/md/modules.order
