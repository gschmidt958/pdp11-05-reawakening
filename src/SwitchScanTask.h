#ifndef SWITCHSCANTASK_H
#define SWITCHSCANTASK_H

#include <Task.h>
#include <FastShiftIn.h>
#include "Registers.h"
#include "DebounceSwitches.h"
#include "BT201ReceiverTask.h"
#include "CharacterBuffer.h"

#define USE_FASTSHIFTIN


/*
 * SwitchScanTask
 *
 * Scans the front panel switch register and sense switches
 * each tick via a chain of shift registers, debounces both
 * inputs, and notifies subscribers when a stable change is
 * detected.
 *
 * HARDWARE
 *   Three bytes are clocked in on each scan: two bytes for
 *   the 16-bit switch register and one byte for the sense
 *   switches. A load pulse latches the current switch state
 *   into the shift registers before reading. USE_FASTSHIFTIN
 *   selects the FastShiftIn library over software shiftIn().
 *
 * DEBOUNCING
 *   Switch register and sense switches are debounced
 *   independently. The stable-sample threshold is loaded
 *   from flash at Setup(). SwitchRegister and SenseSwitches
 *   globals are only updated when a debounced change is
 *   confirmed.
 *
 * SUBSCRIBERS
 *   Tasks register a callback via Subscribe() to be notified
 *   when the sense switches change. Up to MAX_SUBSCRIBERS
 *   callbacks are supported. Unsubscribe() removes a
 *   registration by instance pointer.
 *
 * BLE SWITCH CONTROL
 *   SwitchScanTask subscribes to BT201ReceiverTask for async
 *   $-prefixed messages. The full protocol:
 *
 *   $Bnn / $bnn  — set/clear bit nn (0–15) of SwitchRegister.
 *   $H  / $h     — Halt/Enable toggle on/off (SenseSwitches, active-low).
 *   $L           — Load sense switch (momentary pulse).
 *   $E           — Examine sense switch (momentary pulse).
 *   $C           — Continue sense switch (momentary pulse).
 *   $S           — Start sense switch (momentary pulse).
 *   $D           — Deposit sense switch (momentary pulse).
 *   $KO/$KN/$KP  — Keyswitch off/on/panel-lock (placeholder).
 *
 *   Sense switches are active-low. Momentary BLE pulses clear the
 *   bit (press) on receipt, fire subscribers, then set it back
 *   (release) at the top of the next Execute() tick and fire again.
 *   btSensePending tracks which bits are awaiting release.
 *
 *   On TL+02 (BLE disconnect) both SwitchRegister and SenseSwitches
 *   are restored to the last debounced physical state.
 *
 * DisplaySwitchesTask
 *   A companion task that mirrors SwitchRegister and SenseSwitches
 *   to the display registers each tick. Used as the default idle
 *   task when no other task is active.
 */

class SwitchScanTask : public Task
{
  static const uint8_t MAX_SUBSCRIBERS = 8;
  static const int LoadPin   = 27;
  static const int QhDataPin = 19;
  static const int ClockPin  = 26;

public:
  SwitchScanTask(const char* name, unsigned int period = 0);

  void Setup() override;
  void Execute() override;

  void Subscribe(void (*callback)(void*), void* instance);
  void Unsubscribe(void* instance);

  static void OnBLEMessage(const CharacterBuffer& line);

  static inline SwitchScanTask* instance = nullptr;

private:
  FastShiftIn fsi;

  DebounceSwitches debounceSwitchRegister;
  DebounceSwitches debounceSenseSwitches;

  struct Subscriber
  {
    void (*callback)(void*) = nullptr;
    void* instance = nullptr;
  };

  Subscriber subscribers[MAX_SUBSCRIBERS];
  uint8_t subscriberCount = 0;
  uint8_t btSensePending  = 0;

  void FireToggle();
  static byte SenseByte(char c);
};

class DisplaySwitchesTask : public Task
{
public:
  DisplaySwitchesTask(const char* name, unsigned int period = 0);
  void Execute() override;
};

#endif // SWITCHSCANTASK_H