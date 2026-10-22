#include "ClockTask.h"
#include "BT201TransactionTask.h"

ClockTask::ClockTask(const char* name, unsigned int period)
  : BT201ParentTask(name, period)
{
}

void ClockTask::Setup()
{
  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    taskScheduler->EndTask(Name());
  }
}

void ClockTask::Init()
{
  clockState = ClockState::DisplayTime;
  DisplayRegister = 0;
  lastSecond = rtc.now().second();

  tickingOn = (FlashSettings::instance->Get("tick") == 1);
  chimeSelection = FlashSettings::instance->Get("chime");
  tempCal = FlashSettings::instance->Get("tempCal");

  if (chimeSelection != 0)
    strcpy(chimeFolder, ChimeFolder(chimeSelection));

  SwitchScanTask::instance->Subscribe(ClockTask::OnToggle, this);
}

void ClockTask::Cleanup()
{
  SwitchScanTask::instance->Unsubscribe(this);
  DisplayRegister = 0;
}

/*
  Press LOAD to enter time setting mode.
  The display mirrors the switch register while in this mode.
  If the switch settings represent a valid time, the run light turns on.
  Press DEPOSIT to commit the time; the run light turns off.
*/
void ClockTask::Execute()
{
  DateTime now = rtc.now();

  switch (clockState)
  {
    case ClockState::DisplayTime:
      if (SecondsRolledOver(now))
      {
        DisplayRegister = (now.twelveHour() << 12) | ToOctalDigitFormat(now.minute());

        if (!taskScheduler->IsTaskActive("BT201Transaction"))
        {
          bool soundEnabled = !IsSenseSwitchActive(HALT_ENABLE);
          uint8_t track = ChimeTrack(now.twelveHour(), now.minute(), now.second());
          if (track > 0 && chimeSelection != 0 && soundEnabled)
            BT201TransactionTask::instance->PlayTrack(chimeFolder, track);
          else if (tickingOn && soundEnabled)
            BT201TransactionTask::instance->PlayTrack("TICK", 1);
        }
      }
      break;

    case ClockState::SetTime:
    {
      DisplayRegister = SwitchRegister;
      uint8_t hours = (SwitchRegister >> 12) & 017;
      uint8_t tensOfMinutes = (SwitchRegister >> 6) & 077;
      uint8_t onesOfMinutes = SwitchRegister & 077;

      bool validTime = hours > 0 && hours < 13 && tensOfMinutes < 6 && onesOfMinutes < 10;
      if (validTime)
        RunLight::On();
      else
        RunLight::Off();

      if (IsSenseSwitchActive(DEPOSIT) && validTime)
      {
        rtc.adjust(DateTime(26, 1, 1, hours, tensOfMinutes * 10 + onesOfMinutes, 0));
        RunLight::Off();
        BT201TransactionTask::instance->PlayTrack("SYSTEM", 2);
        clockState = ClockState::DisplayTime;
      }
    }
    break;

    case ClockState::DisplayTemperature:
      UpdateTempDisplay();
      break;

    case ClockState::SetTimer:
      DisplayRegister = SwitchRegister;
      break;

    case ClockState::CountdownTimer:
      if (SecondsRolledOver(now))
      {
        if (--DisplayRegister == 0)
        {
          BT201TransactionTask::instance->PlayTrack("SYSTEM", 3);
          clockState = ClockState::PlayingAlarm;
        }
        else if (tickingOn && !IsSenseSwitchActive(HALT_ENABLE))
          BT201TransactionTask::instance->PlayTrack("TICK", 1);
      }
      break;

    case ClockState::PlayingAlarm:
      if (!taskScheduler->IsTaskActive("BT201Transaction"))
        clockState = ClockState::SetTimer;
      break;
  }
}

void ClockTask::OnToggle(void* instance)
{
  static_cast<ClockTask*>(instance)->HandleSenseSwitch();
}

void ClockTask::HandleSenseSwitch()
{
  if (IsSenseSwitchActive(CONTINUE))
  {
    if (clockState == ClockState::DisplayTime)
      clockState = ClockState::DisplayTemperature;
    else if (clockState == ClockState::DisplayTemperature)
      clockState = ClockState::SetTimer;
    else
      clockState = ClockState::DisplayTime;
    return;
  }

  if (clockState == ClockState::DisplayTime && IsSenseSwitchActive(LOAD))
    clockState = ClockState::SetTime;
  else if (clockState == ClockState::SetTimer && IsSenseSwitchActive(DEPOSIT))
  {
    lastSecond = rtc.now().second();
    clockState = ClockState::CountdownTimer;
  }
}

uint16_t ClockTask::ToOctalDigitFormat(uint8_t val)
{
  return ((val / 10) << 6) | (val % 10);
}

void ClockTask::UpdateTempDisplay()
{
  int temperature = rtc.getTemperature() * 1.8 + 32 + (tempCal - 16);
  if (temperature < 0)
    temperature = 0;
  DisplayRegister = (ToOctalDigitFormat(temperature / 10) << 6) | (temperature % 10);
}

bool ClockTask::SecondsRolledOver(DateTime now)
{
  if (now.second() == lastSecond)
    return false;
  lastSecond = now.second();
  return true;
}

const char* ClockTask::ChimeFolder(uint8_t selection)
{
  static const char* chimeFolders[] = { "CHIMES1", "CHIMES2", "CHIMES3" };
  if (selection >= 1 && selection <= 3)
    return chimeFolders[selection - 1];
  return nullptr;
}

uint8_t ClockTask::ChimeTrack(uint8_t twelveHour, uint8_t minutes, uint8_t seconds)
{
  if (seconds != 0 || minutes % 15 != 0)
    return 0; // 0 = no track
  return (minutes == 0) ? (3 + twelveHour) : (minutes / 15);
}