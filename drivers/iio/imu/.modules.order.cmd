cmd_drivers/iio/imu/modules.order := {   cat drivers/iio/imu/bmi160/modules.order;   cat drivers/iio/imu/inv_icm42600/modules.order;   cat drivers/iio/imu/inv_mpu6050/modules.order;   cat drivers/iio/imu/st_lsm6dsx/modules.order; :; } | awk '!x[$$0]++' - > drivers/iio/imu/modules.order
