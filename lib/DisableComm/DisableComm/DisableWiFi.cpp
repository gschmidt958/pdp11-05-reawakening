#include "esp_wifi.h"


// This code will disconnect from the current WiFi network, stop the WiFi driver, and deinitialize it, effectively terminating the WiFi task.
void DisableWiFi() {
    esp_wifi_disconnect(); // Disconnect from the current WiFi network
    esp_wifi_stop();       // Stop the WiFi driver
    esp_wifi_deinit();     // Deinitialize the WiFi driver and free resources
}