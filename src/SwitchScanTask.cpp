#include "SwitchScanTask.h"
#include "RunLight.h"
#include "FlashTask.h"
#include "PanelLockTask.h"

SwitchScanTask::SwitchScanTask(const char* name, unsigned int period)
  : Task(name, period)
  , fsi(QhDataPin, ClockPin, MSBFIRST)
{
}

void SwitchScanTask::Setup()
{
  pinMode(ClockPin, OUTPUT);
  pinMode(QhDataPin, INPUT);
  pinMode(LoadPin, OUTPUT);

  int debounceLimit = FlashSettings::instance->Get("debounce");
  debounceSwitchRegister.SetLimit(debounceLimit);
  debounceSenseSwitches.SetLimit(debounceLimit);

  BT201ReceiverTask::instance->Subscribe(OnBLEMessage);

  instance = this;
}

void SwitchScanTask::Execute()
{
  static int counter;

  // Release any BLE-simulated sense switches held from the previous scan.
  if (btSensePending)
  {
    SenseSwitches  |= btSensePending;  // active-low: set bits back = release
    btSensePending  = 0;
    FireToggle();
  }

  uint16_t switchRegister;
  uint8_t senseSwitches;

  digitalWrite(LoadPin, LOW);
  delayMicroseconds(2);
  digitalWrite(LoadPin, HIGH);
  delayMicroseconds(2);

#ifdef USE_FASTSHIFTIN
  switchRegister = fsi.read();
  switchRegister |= fsi.read() << 8;
  senseSwitches = fsi.read();
#else
  digitalWrite(ClockPin, LOW);
  switchRegister = shiftIn(QhDataPin, ClockPin, MSBFIRST);
  switchRegister |= shiftIn(QhDataPin, ClockPin, MSBFIRST) << 8;
  senseSwitches = shiftIn(QhDataPin, ClockPin, MSBFIRST);
#endif

  senseSwitches &= ANY;

  if (debounceSwitchRegister.Update(switchRegister))
  {
    SwitchRegister = switchRegister;

    Serial.print(++counter), Serial.print('\t');
    Serial.print("SwitchReg: "), Serial.println(SwitchRegister, BIN);
  }

  if (debounceSenseSwitches.Update(senseSwitches))
  {
    SenseSwitches = senseSwitches;

    Serial.print(++counter), Serial.print('\t');
    Serial.print("Sense: "), Serial.println(SenseSwitches, BIN);

    FireToggle();
  }
}

void SwitchScanTask::Subscribe(void (*callback)(void*), void* instance)
{
  if (subscriberCount < MAX_SUBSCRIBERS)
  {
    subscribers[subscriberCount].callback = callback;
    subscribers[subscriberCount].instance = instance;
    subscriberCount++;
  }
}

void SwitchScanTask::Unsubscribe(void* instance)
{
  for (uint8_t i = 0; i < subscriberCount; i++)
  {
    if (subscribers[i].instance == instance)
    {
      for (uint8_t j = i; j < subscriberCount - 1; j++)
        subscribers[j] = subscribers[j + 1];
      subscriberCount--;
      return;
    }
  }
}

void SwitchScanTask::FireToggle()
{
  for (uint8_t i = 0; i < subscriberCount; i++)
    subscribers[i].callback(subscribers[i].instance);
}

// ---------------------------------------------------------------------------
//  BLE switch control
//
//  $Bnn / $bnn  — set/clear bit nn of SwitchRegister (active-high).
//  $H  / $h     — Halt/Enable on/off (active-low in SenseSwitches).
//  $L $E $C $S $D — momentary sense switch pulse (active-low).
//  $KO $KN $KP  — keyswitch (placeholder).
//  TL+02        — BLE disconnect; restore physical state from debounce.
// ---------------------------------------------------------------------------

byte SwitchScanTask::SenseByte(char c)
{
  static const char keys[] = "LECSD";
  static const byte bits[] = { LOAD, EXAMINE, CONTINUE, START, DEPOSIT };
  const char* p = strchr(keys, toupper((unsigned char)c));
  return p ? bits[p - keys] : 0;
}

void SwitchScanTask::OnBLEMessage(const CharacterBuffer& line)
{
  const char* s = line.ToString();

  // BLE disconnect — restore SwitchRegister and HALT_ENABLE to physical state.
  if (line.StartsWithToken("TL+02"))
  {
    SwitchRegister = instance->debounceSwitchRegister.GetStableValue();
    if (instance->debounceSenseSwitches.GetStableValue() & HALT_ENABLE)
      SenseSwitches |=  HALT_ENABLE;
    else
      SenseSwitches &= ~HALT_ENABLE;
    Serial.println("[SST] BLE disconnect — registers restored to physical state");
    return;
  }

  if (s[0] != '$') return;

  bool on    = isupper((unsigned char)s[1]);
  char upper = toupper((unsigned char)s[1]);

  // Spring-loaded sense switches — momentary pulse (active-low).
  if (strchr("LECSD", upper))
  {
    byte bit = SenseByte(s[1]);
    SenseSwitches &= ~bit;
    instance->btSensePending |=  bit;
    instance->FireToggle();
    Serial.print("[SST] BLE sense switch: ");
    Serial.println(s[1]);
    return;
  }

  switch (upper)
  {
    // $Bnn / $bnn — binary switch register set/clear.
    case 'B':
    {
      char bitStr[3] = { s[2], s[3], '\0' };
      uint8_t bit = (uint8_t)atoi(bitStr);
      if (bit >= 16) return;
      if (on)
        SwitchRegister |= (uint16_t)(1 << bit);
      else
        SwitchRegister &= (uint16_t)~(1 << bit);
      Serial.print("[SST] BLE binary switch bit ");
      Serial.print(bit);
      Serial.println(on ? " ON" : " OFF");
      break;
    }

    // $H / $h — Halt/Enable toggle (active-low).
    case 'H':
      if (on)
        SenseSwitches &= ~HALT_ENABLE;
      else
        SenseSwitches |=  HALT_ENABLE;
      Serial.println(on ? "[SST] BLE HALT_ENABLE ON" : "[SST] BLE HALT_ENABLE OFF");
      break;

    // $KO / $KN / $KP — keyswitch: panel lock on ($KP) or off ($KN/$KO).
    // Has no effect once PanelLockTask permanently latches.
    case 'K':
      PanelLockTask::instance->SetBLEPanelLock(strcmp(s + 2, "P") == 0);
      Serial.print("[SST] BLE keyswitch: ");
      Serial.println(s + 2);
      break;

    default:
      break;
  }
}

// ---------------------------------------------------------------------------
//  DisplaySwitchesTask
// ---------------------------------------------------------------------------

DisplaySwitchesTask::DisplaySwitchesTask(const char* name, unsigned int period)
  : Task(name, period)
{
}

void DisplaySwitchesTask::Execute()
{
  DisplayRegister = SwitchRegister;
  DisplaySense    = SenseSwitches;

  // Any active sense switch turns on run light.
  if (IsAnySenseSwitchActive())
    RunLight::On();
  else
    RunLight::Off();
}