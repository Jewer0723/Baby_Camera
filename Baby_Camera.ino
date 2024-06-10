//buzzer_pin = 33, servo_vertical_pin 1, servo_horizontal_pin 3
//BUZZER(1)代表SD卡開始刪除最舊檔案, BUZZER(3)次代表錯誤出現
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "SD_MMC.h"
#include <WiFi.h>
#include <Arduino.h>

//區網帳密
const char* ssid_STA = "oldman";        // 區域網路的名稱
const char* password_STA = "opp549vx";  // 區域網路的密碼

/*//熱點帳密
const char* ssid_AP = "BabyCam1";      // 熱點網路的名稱
const char* password_AP = "opp549vx";  // 熱點網路的密碼*/

// This project was tested with the AI Thinker Model, M5STACK PSRAM Model and M5STACK WITHOUT PSRAM
#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

void Connect_to_STA_WIFI(const char* ssid_STA, const char* password_STA);
void Connect_to_AP_WIFI(const char* ssid_AP, const char* password_AP);
void startCameraServer();
void recordVideo_With_Net();
void recordVideo_Without_Net();
void checkSDspace();
void BUZZER(int times);

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  //disable brownout detector
  Serial.begin(115200);
  Serial.setDebugOutput(false);

  // 初始化 SD 卡
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("SD 卡初始化失敗");
    BUZZER(3);
    ESP.restart();
  }
  Serial.println("SD 卡初始化成功");

  // 初始化相機設置
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_VGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 2;

  // 相機初始化
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("相機初始化失敗，錯誤碼：0x%x", err);
    BUZZER(3);
    ESP.restart();
  } else {
    Serial.printf("相機初始化成功");
    delay(2000);
  }

  // 設定為 WIFI 模式
  //WiFi.mode(WIFI_AP);//熱點模式
  WiFi.mode(WIFI_STA);  //區網模式
  //WiFi.mode(WIFI_AP_STA);  //熱點+區網模式

  //嘗試連接區網
  Connect_to_STA_WIFI(ssid_STA, password_STA);

  //嘗試連接熱點
  //Connect_to_AP_WIFI(ssid_AP, password_AP);

  // 開始串流伺服器
  startCameraServer();
}

void loop() {
  //檢查空間
  checkSDspace();
  if (WiFi.status() == WL_CONNECTED) {
    //開始寫入SD卡(網路模式)
    recordVideo_With_Net();
  } else {
    Serial.printf("網路未連接，開始進入無網路程序");
    //開始寫入SD卡(無網路模式)
    recordVideo_Without_Net();
    //嘗試連接區網
    Connect_to_STA_WIFI(ssid_STA, password_STA);
  }
}