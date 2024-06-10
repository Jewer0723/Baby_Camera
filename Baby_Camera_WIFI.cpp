#include <WiFi.h>
#include <Arduino.h>

//區網連接最大測試次數
#define Max_LocalNetwork_Trytime 20

void BUZZER(int times);

void Connect_to_STA_WIFI(const char* ssid_STA, const char* password_STA) {
  //試著連接 WIFI 區網
  int ConnectTime = 1;
  WiFi.begin(ssid_STA, password_STA);
  while (WiFi.status() != WL_CONNECTED && ConnectTime <= Max_LocalNetwork_Trytime) {
    Serial.println("連接至本地WiFi中...");
    delay(1000);
    ConnectTime++;
  }
  //檢查區網是否連接成功
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("本地WIFI已連接");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("本地WIFI無法連接");
    BUZZER(3);
  }
}

void Connect_to_AP_WIFI(const char* ssid_AP, const char* password_AP) {
  WiFi.softAP(ssid_AP, password_AP);
}
