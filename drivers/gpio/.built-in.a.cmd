cmd_drivers/gpio/built-in.a := rm -f drivers/gpio/built-in.a; aarch64-none-linux-gnu-ar cDPrST drivers/gpio/built-in.a drivers/gpio/gpiolib.o drivers/gpio/gpiolib-devres.o drivers/gpio/gpiolib-legacy.o drivers/gpio/gpiolib-of.o drivers/gpio/gpiolib-cdev.o drivers/gpio/gpiolib-sysfs.o drivers/gpio/gpio-mmio.o drivers/gpio/gpio-pca953x.o