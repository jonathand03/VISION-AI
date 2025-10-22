/** @brief 
 * 
 * 
 * ANTES DE COMPILAR E FAZER O UPLOAD DO CODIGO PARA O ESP32-S3 VOCE DEVE USAR UMA DAS SEGUINTES FLAGS:
 * 
 * @p SHOW_CAPTURE_TIME_MS  -> PARA MOSTRAR O TEMPO DE CAPTURA E TEMPO TOTAL DE CADA CICLO DE CAPTURA
 * @p SAVE_PIC_TO_SD        -> PARA SALVAR AS IMAGENS CAPTURADAS NO CARTAO SD 
 */

// Descomente uma das linhas abaixo para ativar a funcionalidade desejada

#define SHOW_CAPTURE_TIME_MS
//#define SAVE_PIC_TO_SD


#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"



// Inclusões ESP-IDF para SD Card (VFS e SDMMC Host)
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_types.h"
#include <stdio.h>
#include <sys/stat.h>

#define SD_MOUNT_POINT "/sdcard"
#define PIN_NUM_DAT0    40  // SD_DATA
#define PIN_NUM_CLK     39  // SD_CLK
#define PIN_NUM_CMD     38  // SD_CMD

// support IDF 5.x
#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

#include "esp_camera.h"

//#define BOARD_WROVER_KIT 1
#define BOARD_ESP32S3_WROOM 1
//#define ESP_CAMERA_SUPPORTED 1
// WROVER-KIT PIN Map
#ifdef BOARD_WROVER_KIT

#define CAM_PIN_PWDN -1  //power down is not used
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 21
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 19
#define CAM_PIN_D2 18
#define CAM_PIN_D1 5
#define CAM_PIN_D0 4
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#endif

// ESP32Cam (AiThinker) PIN Map
#ifdef BOARD_ESP32CAM_AITHINKER

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#endif
// ESP32S3 (WROOM) PIN Map
#ifdef BOARD_ESP32S3_WROOM
#define CAM_PIN_PWDN 38
#define CAM_PIN_RESET -1   //software reset will be performed
#define CAM_PIN_VSYNC 6
#define CAM_PIN_HREF 7
#define CAM_PIN_PCLK 13
#define CAM_PIN_XCLK 15
#define CAM_PIN_SIOD 4
#define CAM_PIN_SIOC 5
#define CAM_PIN_D0 11
#define CAM_PIN_D1 9
#define CAM_PIN_D2 8
#define CAM_PIN_D3 10
#define CAM_PIN_D4 12
#define CAM_PIN_D5 18
#define CAM_PIN_D6 17
#define CAM_PIN_D7 16
#endif
// ESP32S3 (GOOUU TECH)
#ifdef BOARD_ESP32S3_GOOUUU
#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET -1   //software reset will be performed
#define CAM_PIN_VSYNC 6
#define CAM_PIN_HREF 7
#define CAM_PIN_PCLK 13
#define CAM_PIN_XCLK 15
#define CAM_PIN_SIOD 4
#define CAM_PIN_SIOC 5
#define CAM_PIN_D0 11
#define CAM_PIN_D1 9
#define CAM_PIN_D2 8
#define CAM_PIN_D3 10
#define CAM_PIN_D4 12
#define CAM_PIN_D5 18
#define CAM_PIN_D6 17
#define CAM_PIN_D7 16
#endif
static const char *TAG = "example:take_picture";

#if ESP_CAMERA_SUPPORTED
static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA,    //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 12, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_DRAM, //CAMERA_FB_IN_PSRAM or CAMERA_FB_IN_DRAM
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

static esp_err_t init_camera(void)
{
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}
#endif

static esp_err_t init_sd_card(void) {
    ESP_LOGI(TAG, "Iniciando SD Card (SDMMC 1-bit)...");

    // Configurações do VFS FAT
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false, // Não formata automaticamente
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    // Configuração do Host (SDMMC padrão do S3)
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED; // Usando 40MHz para melhor desempenho
    
    // Configuração do Slot (Mapeamento dos seus 3 pinos)
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // Mapeamento dos pinos (SDMMC 1-bit)
    slot_config.clk = PIN_NUM_CLK;
    slot_config.cmd = PIN_NUM_CMD;
    slot_config.d0 = PIN_NUM_DAT0;
    
    // As linhas não utilizadas (D1, D2, D3) devem ser desativadas ou configuradas como -1
    slot_config.d1 = -1;
    slot_config.d2 = -1;
    slot_config.d3 = -1; 
    
    // Define a largura do barramento como 1-bit
    slot_config.width = 1;

    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao montar SD Card (0x%x). Verifique pinos/pull-ups.", ret);
    } else {
        ESP_LOGI(TAG, "SD Card montado com sucesso em: %s", SD_MOUNT_POINT);
    }
    return ret;
}


static bool save_jpeg_to_sd(camera_fb_t *pic) {
    static int pictureNumber = 0;
    char fileName[64];
    
    // Cria o caminho completo: /sdcard/pic_001.jpg
    snprintf(fileName, sizeof(fileName), "%s/pic_%03d.jpg", SD_MOUNT_POINT, pictureNumber++);

    ESP_LOGI(TAG, "Tentando salvar: %s", fileName);

    // Usa a função C padrão fopen para abrir o arquivo para escrita (modo binário "wb")
    FILE *file = fopen(fileName, "wb");

    if (file == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir o arquivo para escrita.");
        return false;
    }

    // Escreve os dados da imagem (pic->buf, pic->len)
    size_t bytesWritten = fwrite(pic->buf, 1, pic->len, file);

    // Fecha o arquivo
    if (fclose(file) != 0) {
        ESP_LOGE(TAG, "Erro ao fechar o arquivo.");
        return false;
    }

    if (bytesWritten == pic->len) {
        ESP_LOGI(TAG, "Sucesso! %zu bytes escritos em %s", bytesWritten, fileName);
    } else {
        ESP_LOGE(TAG, "Falha na escrita! (Escritos: %zu de %zu)", bytesWritten, pic->len);
    }

    return bytesWritten == pic->len;
}


#ifdef SHOW_CAPTURE_TIME_MS

void app_main(void)
{
#if ESP_CAMERA_SUPPORTED
    if(ESP_OK != init_camera()) {
        return;
    }

    while (1)
    {
        // 1. INÍCIO DA MEDIÇÃO TOTAL DO CICLO (T_total)
        int64_t total_cycle_start = esp_timer_get_time();
        
        ESP_LOGI(TAG, "Taking picture...");

        // 2. REGISTRA O INÍCIO DA CAPTURA (T_capture)
        int64_t capture_start = esp_timer_get_time();
        
        // CHAMADA DE BLOQUEIO QUE DEVE LEVAR ~12ms
        camera_fb_t *pic = esp_camera_fb_get(); 

        // 3. REGISTRA O FIM DA CAPTURA
        int64_t capture_end = esp_timer_get_time();
        
        if (!pic) {
            ESP_LOGE(TAG, "Falha na captura da câmera!");
        } else {
             ESP_LOGI(TAG, "Picture taken! Size: %zu bytes", pic->len);
             
             // CÁLCULO DO TEMPO DE CAPTURA PURO (T_capture)
             int64_t capture_time_us = capture_end - capture_start;
             
             // Log T_capture (Deve ser ~12ms, ou o valor real de bloqueio)
             ESP_LOGI(TAG, "Tempo de Captura (T_capture): %.2f ms", (float)capture_time_us / 1000.0);

             // 4. LIBERAÇÃO DO BUFFER
             esp_camera_fb_return(pic);

             // 5. FIM DA MEDIÇÃO TOTAL DO CICLO
             int64_t total_cycle_end = esp_timer_get_time();

             // CÁLCULO DO TEMPO TOTAL (T_total)
             int64_t total_time_us = total_cycle_end - total_cycle_start;
             
             // Log T_total (Deve ser ~12ms + tempo de liberação e log)
             ESP_LOGI(TAG, "Tempo Total do Ciclo (T_total): %.2f ms", (float)total_time_us / 1000.0);
        }

        vTaskDelay(5000 / portTICK_RATE_MS);
    }
#else
    ESP_LOGE(TAG, "Camera support is not available for this chip");
    return;
#endif
}

#endif // SHOW_CAPTURE_TIME_MS

#ifdef SAVE_PIC_TO_SD
void app_main(void)
{
#if ESP_CAMERA_SUPPORTED

    if(ESP_OK != init_camera()) {
        ESP_LOGE(TAG, "Fail on initializing camera");
        return;
    }
    
    // 2. Inicializar o SD Card
    if (ESP_OK != init_sd_card()) {
        ESP_LOGE(TAG, "SD Card initialization failed. We only can take pictures without saving.");
    }

    while (1)
    {
        ESP_LOGI(TAG, "Taking picture...");
        camera_fb_t *pic = esp_camera_fb_get();

        if (!pic) {
             ESP_LOGE(TAG, "Capture failed");
        } else {
             // use pic->buf to access the image
             ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);
             
             // 3. Salvar a imagem no SD Card
             save_jpeg_to_sd(pic);
        
             esp_camera_fb_return(pic);
        }

        vTaskDelay(5000 / portTICK_RATE_MS);
    }
#else
    ESP_LOGE(TAG, "Camera support is not available for this chip");
    return;
#endif
}
#endif //SAVE_PIC_TO_SD