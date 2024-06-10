#include "esp_camera.h"
#include "SD_MMC.h"
#include "FS.h"
#include <time.h>
#include <Arduino.h>

#define StartRecordingHour 6
#define StopRecordingHour 20
#define StartRecordingWeekDay 1
#define StopRecordingWeekDay 5

void deleteOldestfile();
void findOldestFile(const char* directory, String& oldestFileName, time_t& oldestTime);
void BUZZER(int times);

//有網路情況下錄製
void recordVideo_With_Net() {
  const char* ntpServer = "pool.ntp.org";             // NTP 伺服器的名稱
  const long gmtOffset_sec = 28800;                   // 你所在的時區的 GMT 偏移量（以秒為單位）
  const int daylightOffset_sec = 0;                   // 是否適用日光節約時間（以秒為單位）
  static bool recording = true;                       // Change this to true
  static const unsigned long recordInterval = 60000;  // 錄影區間(秒為單位)
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  // 確認 /VIDEO 路徑是否存在
  if (!SD_MMC.exists("/VIDEO")) {
    // 創建 /VIDEO 路徑
    SD_MMC.mkdir("/VIDEO");
  }
  fs::FS& fs = SD_MMC;
  fs::File file;
  if (recording) {
    camera_fb_t* fb = NULL;  // pointer
    // 獲取現在時間
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("獲取時間失敗");
      BUZZER(3);
      return;  // Non-critical error, use return
    } else {
      Serial.println("獲取時間成功");
    }
    int currentHour = timeinfo.tm_hour;
    int currentWeekDay = timeinfo.tm_wday;
    if (currentHour >= StartRecordingHour && currentHour < StopRecordingHour && currentWeekDay >= StartRecordingWeekDay && currentWeekDay <= StopRecordingWeekDay) {
      char datestamp[20];
      strftime(datestamp, sizeof(datestamp), "%Y%m%d", &timeinfo);
      // Check if /VIDEO/date directory exists
      String datePath = "/VIDEO/" + String(datestamp);
      if (!SD_MMC.exists(datePath)) {
        // Create /VIDEO/date directory
        SD_MMC.mkdir(datePath.c_str());
      }
      char timestamp[20];
      strftime(timestamp, sizeof(timestamp), "%H%M%S", &timeinfo);
      // Generate a unique filename for each frame
      String path = datePath + "/BabyCam" + String(datestamp) + "_" + String(timestamp) + ".mp4";
      // Open file
      file = fs.open(path.c_str(), FILE_WRITE);
      if (!file) {
        Serial.println("寫入模式開啟檔案失敗");
        BUZZER(3);
        ESP.restart();  // 關鍵錯誤，使用 ESP.restart()
      } else {
        Serial.println("寫入模式檔案開啟成功");
      }
      unsigned long startMillis = millis();
      while (millis() - startMillis < recordInterval) {
        fb = esp_camera_fb_get();
        if (!fb) {
          Serial.println("影像捕捉失敗");
          BUZZER(3);
          ESP.restart();  // 關鍵錯誤，使用 ESP.restart()
        } else {
          Serial.println("影像捕捉成功");
        }
        // Write frame to file
        size_t bytesWritten = file.write(fb->buf, fb->len);
        if (bytesWritten != fb->len) {
          Serial.println("寫入失敗");
          BUZZER(3);
          ESP.restart();  // 關鍵錯誤，使用 ESP.restart()
        }
        // Release frame buffer
        esp_camera_fb_return(fb);
      }
      file.close();  // Close the file after recording
    } else {
      Serial.println("未達錄影時間(6AM ~ 8PM, 禮拜一 ~ 禮拜五)");
    }
  }
}

//無網路情況下錄製
void recordVideo_Without_Net() {
  static bool recording = true;                       // Change this to true
  static const unsigned long recordInterval = 60000;  // Record every 60 seconds
  // 確認 /VIDEO 路徑是否存在
  if (!SD_MMC.exists("/VIDEO")) {
    // 創建 /VIDEO 路徑
    SD_MMC.mkdir("/VIDEO");
  }
  fs::FS& fs = SD_MMC;
  fs::File file;
  if (recording) {
    camera_fb_t* fb = NULL;  // pointer
    String Path = String("/VIDEO/") + String("Missing");
    if (!SD_MMC.exists(Path)) {
      // Create /VIDEO/date directory
      SD_MMC.mkdir(Path.c_str());
    }
    unsigned long FileName = millis();
    // Generate a unique filename for each frame
    String path = Path + "/BabyCam_" + String(FileName) + ".mp4";
    // Open file
    file = fs.open(path.c_str(), FILE_WRITE);
    if (!file) {
      Serial.println("寫入模式開啟檔案失敗");
      BUZZER(3);
      ESP.restart();  // 關鍵錯誤，使用 ESP.restart()
    } else {
      Serial.println("寫入模式檔案開啟成功");
    }
    unsigned long startMillis = millis();
    while (millis() - startMillis < recordInterval) {
      fb = esp_camera_fb_get();
      if (!fb) {
        Serial.println("影像捕捉失敗");
        BUZZER(3);
        ESP.restart();  // 關鍵錯誤，使用 ESP.restart()
      } else {
        Serial.println("影像捕捉成功");
      }
      // Write frame to file
      size_t bytesWritten = file.write(fb->buf, fb->len);
      if (bytesWritten != fb->len) {
        Serial.println("寫入失敗");
        BUZZER(3);
        ESP.restart();  // 關鍵錯誤，使用 ESP.restart()
      }
      // Release frame buffer
      esp_camera_fb_return(fb);
    }
    file.close();  // Close the file after recording
  }
}

//檢查SD卡空間
void checkSDspace() {
  float full = (float)SD_MMC.usedBytes() / (float)SD_MMC.totalBytes();
  if (full >= 0.99) {
    Serial.println("SD卡快滿,開始刪除舊檔案...");
    BUZZER(1);
    deleteOldestfile();
  } else {
    Serial.println("SD卡未滿");
  }
}

//刪除最舊檔案
void deleteOldestfile() {
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("SD 卡初始化失敗");
    BUZZER(3);
    ESP.restart();  // 關鍵錯誤，使用 ESP.restart()
  }
  Serial.println("SD 卡初始化成功");
  // 遞迴遍歷 /VIDEO 目錄下的所有文件和子目錄
  String oldestFileName = "";
  time_t oldestTime = 0;
  findOldestFile("/VIDEO", oldestFileName, oldestTime);
  // 刪除最舊的檔案
  if (oldestFileName != "") {
    Serial.print("找到最舊的檔案：");
    Serial.println(oldestFileName);
    if (SD_MMC.exists(oldestFileName)) {
      Serial.print("刪除最舊的檔案：");
      Serial.println(oldestFileName);
      if (SD_MMC.remove(oldestFileName)) {
        Serial.println("檔案刪除成功");
      } else {
        Serial.println("檔案刪除失敗");
        BUZZER(3);
        ESP.restart();  // 關鍵錯誤，使用 ESP.restart()
      }
    } else {
      Serial.println("無法找到最舊的檔案");
      BUZZER(3);
      ESP.restart();  // 關鍵錯誤，使用 ESP.restart()
    }
  } else {
    Serial.println("沒有找到檔案");
    BUZZER(3);
    ESP.restart();  // 關鍵錯誤，使用 ESP.restart()
  }
}

//找出最舊檔案
void findOldestFile(const char* directory, String& oldestFileName, time_t& oldestTime) {
  File root = SD_MMC.open(directory);
  if (!root || !root.isDirectory()) {
    Serial.print("無法打開目錄：");
    Serial.println(directory);
    BUZZER(3);
    return;  // Non-critical error, only return
  }
  while (File entry = root.openNextFile()) {
    if (entry.isDirectory()) {
      // 構建子目錄完整路徑
      String subDir = String(directory) + "/" + entry.name();
      findOldestFile(subDir.c_str(), oldestFileName, oldestTime);
    } else {
      time_t fileTime = entry.getLastWrite();
      Serial.print("檢查檔案：");
      Serial.print(entry.name());
      Serial.print(" 修改時間：");
      Serial.println(fileTime);
      if (oldestTime == 0 || fileTime < oldestTime) {
        oldestTime = fileTime;
        oldestFileName = String(directory) + "/" + entry.name();  // 保持完整路徑
      }
    }
    entry.close();
  }
  root.close();
}