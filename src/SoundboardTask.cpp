#include "SoundboardTask.h"

const SoundboardTask::Theme SoundboardTask::Themes[] = {
  //  folder    tracks  VU style
  { "ROBBIE",   10,     VUMeterTask::STYLE_VOXC },
  { "B9",       50,     VUMeterTask::STYLE_B9   }
};

uint8_t SoundboardTask::ThemeCount()
{
  return sizeof(Themes) / sizeof(Themes[0]);
}

SoundboardTask::SoundboardTask(const char* name, unsigned int period)
  : BT201ParentTask(name, period)
  , trackSequence(1)
  , themeSequence(1)
  , currentTheme(0)
  , randomThemeMode(false)
  , state(Idle)
  , fixedInterval(15)
  , randomInterval(15)
  , idlePattern(0)
  , nextPlayTime(0)
  , styleChanged(false)
  , idleRotationIndex(0)
{
}

void SoundboardTask::Init()
{
  fixedInterval  = FlashSettings::instance->Get("fixIntrvl");
  randomInterval = FlashSettings::instance->Get("rndIntrvl");
  idlePattern    = FlashSettings::instance->Get("idlPattrn");

  uint8_t themeSelection = (SwitchRegister >> 3) & 077;

  Serial.print("Soundboard: theme selection = "); Serial.println(themeSelection);

  themeSequence.Reset(ThemeCount());

  if (themeSelection >= 1 && themeSelection <= ThemeCount())
  {
    currentTheme = themeSelection - 1;
    randomThemeMode = false;
  }
  else
  {
    currentTheme = themeSequence.Next();
    randomThemeMode = true;
  }

  styleChanged = false;
  DisplayRegister = 0;
  idleRotationIndex = 0;
  LoadTheme();

  SwitchScanTask::instance->Subscribe(SoundboardTask::OnToggle, this);

  ScheduleNextPlay();
  state = Idle;
}

void SoundboardTask::Execute()
{
  switch (state)
  {
    case Idle:
      if (millis() >= nextPlayTime)
      {
        PlayNextTrack();
        state = Playing;
      }
      else
      {
        switch (idlePattern)
        {
          case 0:
            break;  // off
          case 1:
            IdleRotationEffect();
            break;
          case 2:
            IdleRandomDotEffect();
            break;
        }
        if (IsSenseSwitchActive(HALT_ENABLE))
          state = Paused;
      }
      break;

    case Playing:
      if (!taskScheduler->IsTaskActive("BT201Transaction"))
      {
        taskScheduler->EndTask("VUMeter");
        if (trackSequence.IsComplete())
        {
          if (randomThemeMode)
            currentTheme = themeSequence.Next();
          else
          {
            uint8_t themeSelection = (SwitchRegister >> 3) & 077;
            if (themeSelection >= 1 && themeSelection <= ThemeCount())
              currentTheme = themeSelection - 1;
          }
          LoadTheme();
        }

        ScheduleNextPlay();
        state = Idle;
      }
      break;

    case Paused:
      if (!IsSenseSwitchActive(HALT_ENABLE))
      {
        ScheduleNextPlay();
        state = Idle;
      }
      break;
  }
}

void SoundboardTask::Cleanup()
{
  SwitchScanTask::instance->Unsubscribe(this);
  taskScheduler->EndTask("VUMeter");
  DisplayRegister = 0;
}

void SoundboardTask::LoadTheme()
{
  const Theme& theme = Themes[currentTheme];
  trackSequence.Reset(theme.trackCount);
  Serial.print("Soundboard: theme = "); Serial.print(theme.folder);
  Serial.print(", tracks = "); Serial.println(theme.trackCount);

  if (!styleChanged)
    VUMeterTask::instance->SetStyle(theme.style);
}

void SoundboardTask::PlayNextTrack()
{
  taskScheduler->StartTask("VUMeter");
  uint8_t trackNumber = trackSequence.Next() + 1;
  Serial.print("Soundboard: playing track "); Serial.println(trackNumber);
  BT201TransactionTask::instance->PlayTrack(Themes[currentTheme].folder, trackNumber);
}

void SoundboardTask::ScheduleNextPlay()
{
  uint32_t intervalMs = (uint32_t)fixedInterval * 1000
                      + (uint32_t)(random(randomInterval + 1)) * 1000;
  nextPlayTime = millis() + intervalMs;
  Serial.print("Soundboard: next play in "); Serial.print(intervalMs / 1000); Serial.println("s");
}

void SoundboardTask::AdvanceVUMeterStyle()
{
  VUMeterTask::Style style = VUMeterTask::instance->GetStyle();
  do
  {
    style = (VUMeterTask::Style)(((int)style + 1) % (int)VUMeterTask::STYLE_COUNT);
  }
  while (!VUMeterTask::IsVoiceStyle(style)); // Soundboard shows voice styles only

  VUMeterTask::instance->SetStyle(style);
  styleChanged = true;
}

void SoundboardTask::IdleRotationEffect()
{
  static const uint16_t pattern[] = {
    0000001, 0000002, 0000004, 0000010,
    0000020, 0000040, 0000100, 0000200,
    0000400, 0001000, 0002000, 0004000,
    0010000, 0020000, 0040000, 0100000,
    0040000, 0020000, 0010000, 0004000,
    0002000, 0001000, 0000400, 0000200,
    0000100, 0000040, 0000020, 0000010,
    0000004, 0000002
  };
  static const uint8_t patternSize = sizeof(pattern) / sizeof(pattern[0]);

  DisplayRegister = pattern[idleRotationIndex];
  idleRotationIndex = (idleRotationIndex + 1) % patternSize;
}

void SoundboardTask::IdleRandomDotEffect()
{
  if (DisplayRegister == 0)
    DisplayRegister = 1 << random(16);
  else
    DisplayRegister = 0;
}

void SoundboardTask::HandleSenseSwitch()
{
  if (IsSenseSwitchActive(EXAMINE))
  {
    AdvanceVUMeterStyle();
    return;
  }
}

void SoundboardTask::OnToggle(void* instance)
{
  static_cast<SoundboardTask*>(instance)->HandleSenseSwitch();
}