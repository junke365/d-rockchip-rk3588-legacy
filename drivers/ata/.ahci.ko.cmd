cmd_drivers/ata/ahci.ko := aarch64-none-linux-gnu-ld -r -EL  -maarch64elf -z noexecstack --build-id=sha1  -T scripts/module.lds -o drivers/ata/ahci.ko drivers/ata/ahci.o drivers/ata/ahci.mod.o;  true
