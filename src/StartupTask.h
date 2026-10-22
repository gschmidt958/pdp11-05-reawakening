#ifndef STARTUP_H
#define STARTUP_H

#include <Arduino.h>
#include "esp_random.h"

#include "UITask.h"
#include "VUMeterTask.h"
#include "FlashSettings.h"
#include "BT201Provisioning.h"

/*
 * StartupTask
 *
 * Runs once at power-up to initialize the BT201 module and
 * play an intro track before handing off to UITask.
 *
 * SEQUENCE
 *   1. Waits for the BT201 to finish its startup announcement.
 *   2. Sets sound mode and restores the saved volume level.
 *   3. Plays an intro track from the INTROS folder with the
 *      voice VU meter style active.
 *   4. Once the intro finishes, resets the VU meter style
 *      and ends itself.
 *   5. Cleanup() creates and starts UITask to take over.
 *
 * INTRO SELECTION
 *   The introTrack and introStyle flash settings select a
 *   specific track and VU meter style respectively. A value
 *   of 0 for either picks randomly within the valid range.
 *   For introTrack, the valid range is 1..maxValue since 0
 *   is the sentinel for random selection, not a track number.
 */

class StartupTask : public Task
{
public:
  StartupTask(const char* name, unsigned int period = 0)
    : Task(name, period)
  {
  }

  void Init() override
  {
    delay(2000);  // BT201 needs some time to announce itself at powerup.
    BT201TransactionTask::FlushSerial();  // Eat the BT201 initial startup response.
    
    // Force a BT201 reflash if firmware is not properly configured.    
    BT201Provisioning bt201;
    bt201.FlushSerial();
    if (!bt201.IsProvisioned("PDP-11/05"))
    {
      bt201.SetChipModes("PDP-11/05");
      //bt.FactoryResetAndProvision("PDP-11/05");
      Serial.println("Power cycle required.");
      digitalWrite(2, HIGH);  // Turn on built-in LED
      while (true) delay(1000);
    }

    BT201TransactionTask::instance->SoundMode();
    uint16_t volume = FlashSettings::instance->Get("volume");
    BT201TransactionTask::instance->SetVolume(volume);

    BT201TransactionTask::instance->PlayTrack("INTROS", GetIntroTrack());
    VUMeterTask::instance->SetStyle(GetIntroStyle());
    taskScheduler->StartTask("VUMeter");
  }

  void Execute() override
  {
    // Exit when the intro stops playing.
    if (!taskScheduler->IsTaskActive("BT201Transaction"))
    {
      VUMeterTask::instance->SetStyle(VUMeterTask::STYLE_L2RVU);
      taskScheduler->EndTask("VUMeter");
      taskScheduler->EndTask(Name());
    }
  }

  void Cleanup() override
  {
    BT201TransactionTask::instance->AutoSwitchBluetooth(false); // CK00
    BT201TransactionTask::instance->SoundMode();                // CM04
    //BT201TransactionTask::instance->DisableA2DP();             // B300 - no BT audio
    //BT201TransactionTask::instance->SoftReset();

    static UITask uiTask("UI", 200);
    taskScheduler->AddTask(&uiTask);
    taskScheduler->StartTask("UI");
  }

  VUMeterTask::Style GetIntroStyle()
  {
    const FlashSettings::Entry* entry = FlashSettings::instance->LookupByName("introStyle");
    uint16_t introStyle = FlashSettings::instance->Get(entry->name);

    if (introStyle == 0)
      introStyle = RandInRange(1, entry->maxValue);

    static const VUMeterTask::Style introMap[] = {
      VUMeterTask::STYLE_VOXC,
      VUMeterTask::STYLE_VOXE,
      VUMeterTask::STYLE_B9
    };

    return introMap[introStyle - 1];
  }

  uint16_t GetIntroTrack()
  {
    const FlashSettings::Entry* entry = FlashSettings::instance->LookupByName("introTrack");
    uint16_t introTrack = FlashSettings::instance->Get(entry->name);

    if (introTrack == 0)
      introTrack = RandInRange(1, entry->maxValue); // minValue is 0 (sentinel), valid tracks start at 1

    return introTrack;
  }

private:
  uint16_t RandInRange(int first, int last)
  {
    return esp_random() % (last - first + 1) + first;
  }
};

#endif // STARTUP_H