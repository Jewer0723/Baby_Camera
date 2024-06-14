#include "esp_camera.h"
#include "SD_MMC.h"
#include "FS.h"
#include <time.h>
#include <Arduino.h>

void recordVideo_With_Net();
void recordVideo_Without_Net();
void checkSDspace();
void deleteOldestfile();
void findOldestFile(const char* directory, String& oldestFileName, time_t& oldestTime);