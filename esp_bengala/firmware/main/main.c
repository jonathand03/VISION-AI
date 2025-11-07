#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"

// Definições do MPU6050
#define MPU6050_SENSOR_ADDR     0x68   // Endereço I2C do MPU6050
#define MPU6050_PWR_MGMT_1      0x6B   // Registro de gerenciamento de energia
#define MPU6050_ACCEL_XOUT_H    0x3B   // Registro de início dos dados do acelerômetro

// Definições do Sonar I2CXL-MaxSonar-EZ
#define SONAR_SENSOR_ADDR       0x70   // Endereço I2C padrão do MaxSonar
#define SONAR_RANGE_COMMAND     0x51   // Comando para iniciar medição em CM

// Configuração do I2C Master (COMPARTILHADO)
#define I2C_MASTER_SCL_IO       GPIO_NUM_22 // Pino SCL (vai para SCL do MPU e CR do Sonar)
#define I2C_MASTER_SDA_IO       GPIO_NUM_21 // Pino SDA (vai para SDA do MPU e DT do Sonar)
#define I2C_MASTER_NUM          I2C_NUM_0   // Port I2C
#define I2C_MASTER_FREQ_HZ      100000      // Frequência I2C
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0

// Fatores de Sensibilidade (MPU6050)
#define ACCEL_SENSITIVITY_DEFAULT   16384.0f
#define GYRO_SENSITIVITY_DEFAULT    131.0f

static const char *TAG = "multi_sensor_i2c";

// ***********************************************
// FUNÇÕES I2C (INICIALIZAÇÃO)
// ***********************************************

/**
 * @brief Inicializa o driver I2C como mestre (usado por ambos os sensores)
 */
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

// ***********************************************
// FUNÇÕES DO SENSOR MPU6050 (Acelerômetro/Giro)
// ***********************************************

/**
 * @brief "Acorda" o MPU6050
 */
static esp_err_t mpu6050_init(void)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, MPU6050_PWR_MGMT_1, true);
    i2c_master_write_byte(cmd, 0x00, true); // Acorda o sensor
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * @brief Lê 14 bytes de dados (Acel + Temp + Giro) do MPU6050
 */
static esp_err_t mpu6050_read_data(int16_t *ax, int16_t *ay, int16_t *az, int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t data[14];
    i2c_cmd_handle_t cmd;
    esp_err_t ret;

    // 1. Define o registro inicial (ACCEL_XOUT_H)
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, MPU6050_ACCEL_XOUT_H, true);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) return ret;

    // 2. Lê os 14 bytes (burst read)
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_SENSOR_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 13, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &data[13], I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) return ret;

    // 3. Combina os bytes
    *ax = (int16_t)((data[0] << 8) | data[1]);
    *ay = (int16_t)((data[2] << 8) | data[3]);
    *az = (int16_t)((data[4] << 8) | data[5]);
    *gx = (int16_t)((data[8] << 8) | data[9]);
    *gy = (int16_t)((data[10] << 8) | data[11]);
    *gz = (int16_t)((data[12] << 8) | data[13]);

    return ESP_OK;
}

// ***********************************************
// FUNÇÕES DO SENSOR SONAR (Distância)
// ***********************************************

/**
 * @brief Envia o comando para o Sonar iniciar uma medição (0x51)
 */
static esp_err_t sonar_trigger_reading(void)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    // Endereço do sensor + bit de escrita
    i2c_master_write_byte(cmd, (SONAR_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, true);
    // Comando para "Range em CM"
    i2c_master_write_byte(cmd, SONAR_RANGE_COMMAND, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao DISPARAR leitura do Sonar. Erro: %s", esp_err_to_name(ret));
    }
    return ret;
}

/**
 * @brief Lê os 2 bytes de distância do Sonar
 *
 * @param[out] distance_cm Ponteiro para armazenar a distância em CM
 */
static esp_err_t sonar_read_distance(uint16_t *distance_cm)
{
    uint8_t data[2] = {0};
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SONAR_SENSOR_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &data[0], I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &data[1], I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao LER dados do Sonar. Erro: %s", esp_err_to_name(ret));
        return ret;
    }

    *distance_cm = (uint16_t)((data[0] << 8) | data[1]);

    return ESP_OK;
}


// ***********************************************
// FUNÇÃO PRINCIPAL (APP_MAIN)
// ***********************************************

void app_main(void)
{
    // Variáveis do MPU6050
    int16_t ax_raw, ay_raw, az_raw;
    int16_t gx_raw, gy_raw, gz_raw;
    float ax_g, ay_g, az_g;
    float gx_dps, gy_dps, gz_dps;

    // Variável do Sonar
    uint16_t distance_cm = 0;

    // Inicializa o barramento I2C (UMA VEZ SÓ)
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "Driver I2C inicializado (para MPU6050 e Sonar)");

    // Inicializa o MPU6050 (acorda)
    if (mpu6050_init() == ESP_OK) {
        ESP_LOGI(TAG, "MPU6050 inicializado (acordado)");
    } else {
        ESP_LOGE(TAG, "Falha ao inicializar MPU6050");
    }

    while (1) {
        
        esp_err_t mpu_ret = mpu6050_read_data(&ax_raw, &ay_raw, &az_raw, &gx_raw, &gy_raw, &gz_raw);
        
        if (mpu_ret == ESP_OK) {
            ax_g = (float)ax_raw / ACCEL_SENSITIVITY_DEFAULT;
            ay_g = (float)ay_raw / ACCEL_SENSITIVITY_DEFAULT;
            az_g = (float)az_raw / ACCEL_SENSITIVITY_DEFAULT;
            gx_dps = (float)gx_raw / GYRO_SENSITIVITY_DEFAULT;
            gy_dps = (float)gy_raw / GYRO_SENSITIVITY_DEFAULT;
            gz_dps = (float)gz_raw / GYRO_SENSITIVITY_DEFAULT;

            printf("--------------------------------------\n");
            printf("Aceleração (g):   X=%.2f, Y=%.2f, Z=%.2f\n", ax_g, ay_g, az_g);
            printf("Giroscópio (°/s): X=%.2f, Y=%.2f, Z=%.2f\n", gx_dps, gy_dps, gz_dps);
        } else {
            ESP_LOGE(TAG, "Falha ao ler dados do MPU6050");
        }

        // --- 2. Leitura do Sonar ---
        esp_err_t sonar_trig_ret = sonar_trigger_reading();
        
        if (sonar_trig_ret == ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(100));
            esp_err_t sonar_read_ret = sonar_read_distance(&distance_cm);
            
            if (sonar_read_ret == ESP_OK) {
                if (distance_cm == 0) {
                     printf("Distância Sonar: Fora de alcance (mín 20cm)\n");
                } else {
                    printf("Distância Sonar: %u cm\n", distance_cm);
                }
            } else {
                ESP_LOGE(TAG, "Falha ao ler distância do Sonar");
            }
        } else {
            ESP_LOGE(TAG, "Falha ao disparar o Sonar");
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        vTaskDelay(pdMS_TO_TICKS(400));
    }
}