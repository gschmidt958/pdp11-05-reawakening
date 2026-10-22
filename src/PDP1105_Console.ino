// Note: The ESP32 boards seem to be buggy.
// 2.0.17 is a known stable version, newer boards
// cause crashes and break the USB serial (2) port.
// I have tried both 3.2.0 and 3.3.0 and neither work.
#define BT201_ENABLE_DEBUG

#include "BT201Provisioning.h"
#include <SPI.h>
#include <DisableComm.h>
#include <esp_timer.h>
#include "RunLight.h"

#include "TaskSchedule.h"
#include "BlinkTask.h"
#include "DisplayRefreshTask.h"
#include "SwitchScanTask.h"
#include "BT201ReceiverTask.h"
#include "BT201TransactionTask.h"
#include "StartupTask.h"

#include "FlashTask.h"
#include "VUMeterTask.h"
#include "ClockTask.h"
#include "KnightRiderTask.h"
#include "ComputerTask.h"
#include "BluetoothTask.h"
#include "PanelLockTask.h"
#include "JukeboxTask.h"
#include "SoundboardTask.h"
#include "PatternTasks.h"

// System Tasks
BlinkTask blinkTask("Blink", 1000);
DisplayRefreshTask displayRefreshTask("DisplayRefresh", 1);
SwitchScanTask switchScanTask("SwitchScan", 1);
DisplaySwitchesTask displaySwitchesTask("DisplaySwitches", 1);
BT201ReceiverTask BT201ReceiverTask("BT201Receiver", 10);
BT201TransactionTask BT201TransactionTask("BT201Transaction", 10);
StartupTask startupTask("Startup", 1);

// Additional user level tasks
FlashTask flashTask("Flash", 100);
VUMeterTask vuMeterTask("VUMeter", 2);

PanelLockTask panelLockTask("PanelLock", 100);
ClockTask clockTask("Clock", 1);
KnightRiderTask knightRiderTask("KnightRider", 100);
ComputerTask computerTask("Computer", 100);
BluetoothTask bluetoothTask("Bluetooth", 100);
JukeboxTask jukeboxTask("Jukebox", 10);
SoundboardTask soundboardTask("Soundboard", 100);

// Hardware timer and sampling instrumentation — must be global
esp_timer_handle_t sampleTimer;  

void IRAM_ATTR onSampleTimer(void* arg)
{
  vuMeterTask.Sample();
}

SET_LOOP_TASK_STACK_SIZE(12 * 1024);

void setup()
{
  Serial.begin(9600);
  disableCore0WDT();  // esp_timer on Core 0 starves IDLE0

  DisableWiFi();
  DisableBluetooth();

  Scheduler.AddTask(&flashTask);

  RunLight::Setup();

  Scheduler.AddTask(&blinkTask);
  Scheduler.StartTask("Blink");

  Scheduler.AddTask(&displayRefreshTask);
  Scheduler.StartTask("DisplayRefresh");

  Scheduler.AddTask(&switchScanTask);
  Scheduler.StartTask("SwitchScan");

  Scheduler.AddTask(&displaySwitchesTask);

  Scheduler.AddTask(&vuMeterTask);

  Scheduler.AddTask(&clockTask);
  Scheduler.AddTask(&knightRiderTask);
  Scheduler.AddTask(&computerTask);
  Scheduler.AddTask(&bluetoothTask);
  Scheduler.AddTask(&panelLockTask);
  Scheduler.AddTask(&jukeboxTask);
  Scheduler.AddTask(&soundboardTask);

  AddPatternTasks();

  Scheduler.AddTask(&BT201ReceiverTask);
  Scheduler.StartTask("BT201Receiver");
  Scheduler.AddTask(&BT201TransactionTask);
  BT201ReceiverTask.SetSyncClient(&BT201TransactionTask);
    
  Scheduler.AddTask(&startupTask);
  Scheduler.StartTask("Startup");

  // Start hardware timer for audio sampling — fires every 32µs
  esp_timer_create_args_t timerArgs = {
    .callback        = onSampleTimer,
    .arg             = nullptr,
    .dispatch_method = ESP_TIMER_TASK,
    .name            = "vuSample"
  };
  esp_timer_create(&timerArgs, &sampleTimer);
  esp_timer_start_periodic(sampleTimer, 70); // 14kHz sample rate, ~7kHz frequency response.
}

// Runs all active tasks
void loop()
{
  Scheduler.Run();
}
