cmd_drivers/power/supply/built-in.a := rm -f drivers/power/supply/built-in.a; aarch64-none-linux-gnu-ar cDPrST drivers/power/supply/built-in.a drivers/power/supply/power_supply_core.o drivers/power/supply/power_supply_sysfs.o drivers/power/supply/power_supply_hwmon.o drivers/power/supply/sbs-battery.o drivers/power/supply/gpio-charger.o drivers/power/supply/bq24735-charger.o