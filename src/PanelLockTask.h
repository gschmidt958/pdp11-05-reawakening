#ifndef PANELLOCKTASK_H
#define PANELLOCKTASK_H

#include <Arduino.h>
#include <TaskScheduler.h>
#include "BT201ParentTask.h"
#include "BT201TransactionTask.h"
#include "FlashSettings.h"
#include "AlgorithmicPatternTasks.h"

/*
 * PanelLockTask
 *
 * Activated when a hardware panel lock signal is detected.
 * Plays a self-destruct track at high volume while running
 * the RadiateDisplay effect. Restores the previous volume
 * level on Cleanup().
 *
 * TIMELINE
 *
 *   Phase 1 — RadiateDisplay starts, audio begins playing.
 *
 *   Phase 2 — RadiateDisplay stops. OverrideCountdown starts,
 *              displaying a binary countdown then a collapsing
 *              bar (length configurable via SetOverrideBarCount,
 *              default 10) shrinking left-to-right to zero,
 *              synchronized to the audio track.
 *
 *   Phase 3 — OverrideCountdown stops. RadiateDisplay restarts.
 *              Permanent lock latches: IsPanelLockActive()
 *              returns true regardless of hardware pin state.
 *
 *   Phase 4 — RadiateDisplay stops. FinalCountdown starts,
 *              displaying a binary countdown then a collapsing
 *              bar (length configurable via SetFinalBarCount,
 *              default 10) shrinking left-to-right to zero,
 *              synchronized to the audio track.
 *
 *   Phase 5 — FinalCountdown stops. DamagedRadiate starts.
 *              Radiate animation runs while pixels are
 *              progressively killed across three explosion
 *              phases. After the last pixel dies the display
 *              holds blank.
 *
 *   Phase 6 — Audio ends. DamagedRadiate stops. A 30-second
 *              reboot timer begins. If reboot is enabled via
 *              flash settings, the ESP32 reboots; otherwise
 *              the display holds dark indefinitely.
 *
 * PANEL LOCK PIN
 *   IsPanelLockActive() reads the hardware lock signal
 *   directly and is intended to be polled by the parent
 *   task to determine when to start and stop this task.
 *   Once permanently locked, always returns true regardless
 *   of the hardware pin state. The only way to recover is
 *   to power cycle the panel.
 */

class PanelLockTask : public BT201ParentTask
{
  static const int panelLockPin = 39;

  static const unsigned long overrideCountdownStartMs = (4UL * 60UL + 22UL) * 1000UL;       //  4:22
  static const unsigned long lockoutMs = 5UL * 60UL * 1000UL;                               //  5:00
  static const unsigned long finalCountdownStartMs = (9UL * 60UL + 25UL) * 1000UL + 000UL;  //  9:25:000
  static const unsigned long finalCountdownEndMs = 10UL * 60UL * 1000UL;                    // 10:00
  static const unsigned long rebootDelayMs = 30UL * 1000UL;                                 // 30 s after audio ends

  enum class Phase
  {
    Radiate1,           // Phase 1: initial radiate + audio
    OverrideCountdown,  // Phase 2: override countdown
    Radiate2,           // Phase 3: radiate resumes, permanent lock latches
    FinalCountdown,     // Phase 4: final countdown
    DamagedRadiate,     // Phase 5: damaged radiate, pixels dying
    RebootCountdown,    // Phase 6: audio done, reboot timer running
  };

public:
  static inline PanelLockTask* instance = nullptr;

  PanelLockTask(const char* name, unsigned int period = 0)
    : BT201ParentTask(name, period)
    , phase(Phase::Radiate1)
    , permanentlyLocked(false)
    , blePanelLock(false)
  {
    instance = this;
  }

  void Setup() override
  {
    pinMode(panelLockPin, INPUT);
  }

  void Init() override
  {
    taskScheduler->StartTask("RadiateDisplay");

    previousVolumeLevel = BT201TransactionTask::instance->GetVolume();
    BT201TransactionTask::instance->SetVolume(FlashSettings::instance->Get("pnlLckVol"));
    BT201TransactionTask::instance->PlayTrack("PANLLCK", 1);

    phase            = Phase::Radiate1;
    permanentlyLocked = false;
    lockStartTime    = millis();

    ((OverrideCountdownDisplayTask*)taskScheduler->GetTask("OverrideCountdownDisplay"))->SetOverrideBarCount(FlashSettings::instance->Get("ovrBarCnt"));
    ((FinalCountdownDisplayTask*)taskScheduler->GetTask("FinalCountdownDisplay"))->SetFinalBarCount(FlashSettings::instance->Get("fnlBarCnt"));
    rebootEnabled = FlashSettings::instance->Get("reboot");
  }

  void Cleanup() override
  {
    BT201TransactionTask::instance->SetVolume(previousVolumeLevel);

    // Stop all display tasks (stop is idempotent, so just stop all possibilites)
    taskScheduler->EndTask("RadiateDisplay");
    taskScheduler->EndTask("OverrideCountdownDisplay");
    taskScheduler->EndTask("FinalCountdownDisplay");
    taskScheduler->EndTask("DamagedRadiateDisplay");
  }

  void Execute() override
  {
    unsigned long elapsed = millis() - lockStartTime;

    switch (phase)
    {
      // --- Phase 1 -> 2: stop RadiateDisplay, start OverrideCountdown ---
      case Phase::Radiate1:
        if (elapsed >= overrideCountdownStartMs)
        {
          taskScheduler->EndTask("RadiateDisplay");
          taskScheduler->StartTask("OverrideCountdownDisplay");
          phase = Phase::OverrideCountdown;
        }
        break;

      // --- Phase 2 -> 3: end OverrideCountdown, restart RadiateDisplay, latch lock ---
      case Phase::OverrideCountdown:
        if (elapsed >= lockoutMs)
        {
          taskScheduler->EndTask("OverrideCountdownDisplay");
          taskScheduler->StartTask("RadiateDisplay");

          Serial.println("PanelLock: permanent lockout engaged");
          permanentlyLocked = true;
          phase = Phase::Radiate2;
        }
        break;

      // --- Phase 3 -> 4: stop RadiateDisplay, start FinalCountdown ---
      case Phase::Radiate2:
        if (elapsed >= finalCountdownStartMs)
        {
          taskScheduler->EndTask("RadiateDisplay");
          taskScheduler->StartTask("FinalCountdownDisplay");
          phase = Phase::FinalCountdown;
        }
        break;

      // --- Phase 4 -> 5: stop FinalCountdown, start DamagedRadiate ---
      case Phase::FinalCountdown:
        if (elapsed >= finalCountdownEndMs && taskScheduler->IsTaskActive("FinalCountdownDisplay"))
        {
          taskScheduler->EndTask("FinalCountdownDisplay");
          taskScheduler->StartTask("DamagedRadiateDisplay");
          phase = Phase::DamagedRadiate;
        }
        break;

      // --- Phase 5 -> 6: audio ends, stop DamagedRadiate, begin reboot countdown ---
      case Phase::DamagedRadiate:
        if (!taskScheduler->IsTaskActive("BT201Transaction"))
        {
          taskScheduler->EndTask("DamagedRadiateDisplay");
          rebootTime = millis();
          phase = Phase::RebootCountdown;
        }
        break;

      // --- Phase 6 -> 7: reboot (if enabled), otherwise hold ---
      case Phase::RebootCountdown:
        if (millis() - rebootTime >= rebootDelayMs)
        {
          if (rebootEnabled)
          {
            Serial.println("PanelLock: rebooting");
            ESP.restart();
          }
        }
        break;
    }
  }

  // Returns true if the hardware lock signal is active, the BLE keyswitch
  // is in panel-lock position, or the permanent lockout latch has tripped.
  // Once permanently locked, always returns true regardless of any input.
  bool IsPanelLockActive()
  {
    if (permanentlyLocked) return true;
    return digitalRead(panelLockPin) || blePanelLock;
  }

  // Called by BLE keyswitch handler. $KP sets true; $KN/$KO set false.
  // Has no effect once permanentlyLocked latches.
  void SetBLEPanelLock(bool active)
  {
    if (!permanentlyLocked)
      blePanelLock = active;
  }

private:
  Phase         phase;
  bool          permanentlyLocked;
  bool          blePanelLock;
  bool          rebootEnabled;
  uint8_t       previousVolumeLevel;
  unsigned long lockStartTime;
  unsigned long rebootTime;
};

#endif // PANELLOCKTASK_H