#ifndef CLOCKTASK_H
#define CLOCKTASK_H

#include "RTClib.h" // AdaFruit Realtime Clock library
#include "BT201ParentTask.h"
#include "Registers.h"
#include "RunLight.h"
#include "FlashTask.h"
#include "SwitchScanTask.h"

/*
 * ClockTask
 *
 * Displays and manages a real-time clock using a DS3231 RTC
 * module. Derives from BT201ParentTask to support audio
 * effects (ticking, chimes, alarms) with clean shutdown.
 *
 * DISPLAY STATES
 *   The task cycles through several display states, advanced
 *   by the CONTINUE sense switch:
 *     DisplayTime        - shows current time in octal digit
 *                          format (hours | minutes).
 *     DisplayTemperature - shows temperature from the RTC's
 *                          built-in sensor (Fahrenheit).
 *     SetTimer           - loads a countdown value from the
 *                          switch register.
 *     CountdownTimer     - counts down seconds; plays an
 *                          alarm track on expiry.
 *     PlayingAlarm       - waits for the alarm track to
 *                          finish, then returns to SetTimer.
 *
 * TIME SETTING
 *   Pressing LOAD while in DisplayTime enters SetTime mode.
 *   The switch register is interpreted as hours (bits 15-12)
 *   and minutes in two octal digits (bits 11-6 and 5-0).
 *   The run light indicates a valid time entry. Pressing
 *   DEPOSIT commits the time to the RTC and plays the
 *   confirmation tone.
 *
 * AUDIO
 *   Chime selection and ticking are loaded from flash at
 *   Init(). Chimes play on the quarter-hour; the tick sound
 *   plays each second when no chime is active. Audio is
 *   inhibited while a BT201 transaction is already in
 *   progress, and suppressed when HALT_ENABLE is active.
 */

class ClockTask : public BT201ParentTask
{
public:
  ClockTask(const char* name, unsigned int period = 0);

  void Setup() override;
  void Init() override;
  void Cleanup() override;
  void Execute() override;
  void HandleSenseSwitch();

private:
  enum ClockState { DisplayTime, SetTime, DisplayTemperature, SetTimer, CountdownTimer, PlayingAlarm };

  static void OnToggle(void* instance);

  uint16_t ToOctalDigitFormat(uint8_t val);
  void UpdateTempDisplay();
  bool SecondsRolledOver(DateTime now);
  const char* ChimeFolder(uint8_t selection);
  uint8_t ChimeTrack(uint8_t twelveHour, uint8_t minutes, uint8_t seconds);

  ClockState clockState;
  RTC_DS3231 rtc;
  uint8_t lastSecond;

  bool tickingOn;
  uint8_t chimeSelection;
  uint8_t tempCal;
  
  char chimeFolder[16];
};

#endif // CLOCKTASK_H