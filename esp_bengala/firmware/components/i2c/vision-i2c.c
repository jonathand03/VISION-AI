
// #include <stdio.h>
// #include "sdkconfig.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_log.h"
// #include "driver/i2c_master.h"


// static const char *TAG = "example";

// #define I2C_MASTER_SCL_IO           CONFIG_I2C_MASTER_SCL       /*!< GPIO number used for I2C master clock */
// #define I2C_MASTER_SDA_IO           CONFIG_I2C_MASTER_SDA       /*!< GPIO number used for I2C master data  */
// #define I2C_MASTER_NUM              I2C_NUM_0                   /*!< I2C port number for master dev */
// #define I2C_MASTER_FREQ_HZ          CONFIG_I2C_MASTER_FREQUENCY /*!< I2C master clock frequency */
// #define I2C_MASTER_TX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
// #define I2C_MASTER_RX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
// #define I2C_MASTER_TIMEOUT_MS       1000

// #define MPU9250_SENSOR_ADDR         0x68        /*!< Address of the MPU9250 sensor */
// #define MPU9250_WHO_AM_I_REG_ADDR   0x75        /*!< Register addresses of the "who am I" register */
// #define MPU9250_PWR_MGMT_1_REG_ADDR 0x6B        /*!< Register addresses of the power management register */
// #define MPU9250_RESET_BIT           7


// static void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle)
// {
//     i2c_master_bus_config_t bus_config = {
//         .i2c_port = I2C_MASTER_NUM,
//         .sda_io_num = I2C_MASTER_SDA_IO,
//         .scl_io_num = I2C_MASTER_SCL_IO,
//         .clk_source = I2C_CLK_SRC_DEFAULT,
//         .glitch_ignore_cnt = 7,
//         .flags.enable_internal_pullup = true,
//     };
//     ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));

//     i2c_device_config_t dev_config = {
//         .dev_addr_length = I2C_ADDR_BIT_LEN_7,
//         .device_address = MPU9250_SENSOR_ADDR,
//         .scl_speed_hz = I2C_MASTER_FREQ_HZ,
//     };
//     ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_config, dev_handle));
// }
