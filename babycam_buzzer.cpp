#include <Arduino.h>

#define buzzer_pin 33

//蜂鳴器警告(代表錯誤出現, 或處理SD卡程序)
void BUZZER(int times) {
  pinMode(buzzer_pin, OUTPUT);
  for (int i = 1; i <= times; i++) {
    digitalWrite(buzzer_pin, LOW);
    delay(100);
    digitalWrite(buzzer_pin, HIGH);
    delay(100);
  }
  digitalWrite(buzzer_pin, HIGH);
}