#include "esp_bt.h"
#include "esp_bt_main.h"

void DisableBluetooth()
{
  esp_bluedroid_disable();
  esp_bluedroid_deinit();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
}