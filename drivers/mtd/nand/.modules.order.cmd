cmd_drivers/mtd/nand/modules.order := {   cat drivers/mtd/nand/onenand/modules.order;   cat drivers/mtd/nand/raw/modules.order;   cat drivers/mtd/nand/spi/modules.order; :; } | awk '!x[$$0]++' - > drivers/mtd/nand/modules.order