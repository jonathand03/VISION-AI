// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/i2c.h"
// #include "esp_log.h"
// // Definições do MPU6050
// #define MPU6050_SENSOR_ADDR     0x68   // Endereço I2C do MPU6050
// #define MPU6050_PWR_MGMT_1      0x6B   // Registro de gerenciamento de energia
// #define MPU6050_ACCEL_XOUT_H    0x3B   // Registro de início dos dados do acelerômetro
// #define MPU6050_GYRO_XOUT_H     0x43   // Registro de início dos dados do giroscópio
// #define MPU6050_WHO_AM_I        0x75   // Registro "Quem sou eu"

// // Configuração do I2C Master
// #define I2C_MASTER_SCL_IO       GPIO_NUM_22 // Pino SCL
// #define I2C_MASTER_SDA_IO       GPIO_NUM_21 // Pino SDA
// #define I2C_MASTER_NUM          I2C_NUM_0   // Port I2C
// #define I2C_MASTER_FREQ_HZ      100000      // Frequência I2C (100KHz)
// #define I2C_MASTER_TX_BUF_DISABLE 0         // Sem buffer de TX
// #define I2C_MASTER_RX_BUF_DISABLE 0         // Sem buffer de RX

// static const char *TAG = "mpu6050";

// /**
//  * @brief Inicializa o driver I2C como mestre
//  */
// static esp_err_t i2c_master_init(void)
// {
//     i2c_config_t conf = {
//         .mode = I2C_MODE_MASTER,
//         .sda_io_num = I2C_MASTER_SDA_IO,
//         .scl_io_num = I2C_MASTER_SCL_IO,
//         .sda_pullup_en = GPIO_PULLUP_ENABLE,
//         .scl_pullup_en = GPIO_PULLUP_ENABLE,
//         .master.clk_speed = I2C_MASTER_FREQ_HZ,
//     };
    
//     esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
//     if (err != ESP_OK) {
//         return err;
//     }

//     return i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
// }

// /**
//  * @brief "Acorda" o MPU6050, tirando-o do modo de suspensão
//  */
// static esp_err_t mpu6050_init(void)
// {
//     i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
//     // Inicia a transmissão
//     i2c_master_start(cmd);
//     // Escreve o endereço do sensor + bit de escrita
//     i2c_master_write_byte(cmd, (MPU6050_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, true);
//     // Escreve o endereço do registro PWR_MGMT_1
//     i2c_master_write_byte(cmd, MPU6050_PWR_MGMT_1, true);
//     // Escreve 0x00 para acordar o sensor (limpa o bit 'sleep')
//     i2c_master_write_byte(cmd, 0x00, true);
//     // Para a transmissão
//     i2c_master_stop(cmd);
    
//     // Executa o comando
//     esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
//     i2c_cmd_link_delete(cmd);
    
//     return ret;
// }

// /**
//  * @brief Lê 14 bytes de dados (Acel + Temp + Giro) do MPU6050
//  *
//  * @param[out] ax Ponteiro para armazenar Aceleração X
//  * @param[out] ay Ponteiro para armazenar Aceleração Y
//  * @param[out] az Ponteiro para armazenar Aceleração Z
//  * @param[out] gx Ponteiro para armazenar Giroscópio X
//  * @param[out] gy Ponteiro para armazenar Giroscópio Y
//  * @param[out] gz Ponteiro para armazenar Giroscópio Z
//  */
// static esp_err_t mpu6050_read_data(int16_t *ax, int16_t *ay, int16_t *az, int16_t *gx, int16_t *gy, int16_t *gz)
// {
//     uint8_t data[14]; // Buffer para 6 bytes de acel, 2 de temp, 6 de giro
//     i2c_cmd_handle_t cmd;
//     esp_err_t ret;

//     // 1. Define o registro inicial que queremos ler (ACCEL_XOUT_H)
//     cmd = i2c_cmd_link_create();
//     i2c_master_start(cmd);
//     i2c_master_write_byte(cmd, (MPU6050_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, true);
//     i2c_master_write_byte(cmd, MPU6050_ACCEL_XOUT_H, true); // Começa do 0x3B
//     i2c_master_stop(cmd);
    
//     ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
//     i2c_cmd_link_delete(cmd);

//     if (ret != ESP_OK) {
//         ESP_LOGE(TAG, "Falha ao definir o ponteiro do registro. Erro: %s", esp_err_to_name(ret));
//         return ret;
//     }

//     // 2. Lê os 14 bytes de dados de uma vez (burst read)
//     cmd = i2c_cmd_link_create();
//     i2c_master_start(cmd);
//     i2c_master_write_byte(cmd, (MPU6050_SENSOR_ADDR << 1) | I2C_MASTER_READ, true);
    
//     // Lê 13 bytes com ACK
//     i2c_master_read(cmd, data, 13, I2C_MASTER_ACK);
//     // Lê o último (14º) byte com NACK
//     i2c_master_read_byte(cmd, &data[13], I2C_MASTER_NACK);
    
//     i2c_master_stop(cmd);
    
//     ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
//     i2c_cmd_link_delete(cmd);
    
//     if (ret != ESP_OK) {
//         ESP_LOGE(TAG, "Falha ao ler os dados do sensor. Erro: %s", esp_err_to_name(ret));
//         return ret;
//     }

//     // 3. Combina os bytes (High e Low) em valores de 16 bits (int16_t)
//     // Formato: Big-endian (High byte primeiro)
//     *ax = (int16_t)((data[0] << 8) | data[1]);
//     *ay = (int16_t)((data[2] << 8) | data[3]);
//     *az = (int16_t)((data[4] << 8) | data[5]);
    
//     // data[6] e data[7] são da Temperatura (pulamos)
    
//     *gx = (int16_t)((data[8] << 8) | data[9]);
//     *gy = (int16_t)((data[10] << 8) | data[11]);
//     *gz = (int16_t)((data[12] << 8) | data[13]);

//     return ESP_OK;
// }


// void app_main(void)
// {
//     int16_t ax, ay, az, gx, gy, gz;

//     // Inicializa o I2C
//     ESP_ERROR_CHECK(i2c_master_init());
//     ESP_LOGI(TAG, "Driver I2C inicializado");

//     // Inicializa o MPU6050 (acorda)
//     esp_err_t init_ret = mpu6050_init();
//     if (init_ret != ESP_OK) {
//         ESP_LOGE(TAG, "Falha ao inicializar MPU6050. Erro: %s", esp_err_to_name(init_ret));
//         // Se falhar aqui, provavelmente o sensor não está conectado ou o endereço está errado.
//         // O código continuará, mas as leituras falharão.
//     } else {
//         ESP_LOGI(TAG, "MPU6050 inicializado (acordado)");
//     }
    

//     while (1) {
//         esp_err_t ret = mpu6050_read_data(&ax, &ay, &az, &gx, &gy, &gz);

//         if (ret == ESP_OK) {
//             // Imprime os dados brutos
//             printf("--------------------------------------\n");
//             printf("Aceleração (Raw): X=%d, Y=%d, Z=%d\n", ax, ay, az);
//             printf("Giroscópio (Raw): X=%d, Y=%d, Z=%d\n", gx, gy, gz);
//         } else {
//             ESP_LOGE(TAG, "Falha ao ler dados do MPU6050");
//         }

//         vTaskDelay(pdMS_TO_TICKS(500)); // Espera 500ms
//     }
// }