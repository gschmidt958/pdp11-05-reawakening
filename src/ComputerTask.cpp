#include "ComputerTask.h"

const char* ComputerTask::DisplayEffects[] = {
  "KnightRiderDisplay",     //  1
  "BinaryCounterDisplay",   //  2
  "UpDownCounterDisplay",   //  3
  "CrissCrossDisplay",      //  4
  "RandomDisplay",          //  5
  "CheckerboardDisplay",    //  6
  "WigWagDisplay",          //  7
  "DiamondDisplay",         //  8
  "DissipateDisplay",       //  9
  "ChaserDisplay",          // 10
  "RandomBitFillDisplay",   // 11
  "SnakeDisplay",           // 12
  "SplitDisplay",           // 13
  "ImplodeDisplay",         // 14
  "Implode2Display",        // 15 
  "SqueezeDisplay",         // 16
  "MarquisDisplay",         // 17
  "ChopsticksDisplay",      // 18
  "PanelDisplay",           // 19
  "OctalCountDisplay",      // 20
  "OctalFillDisplay",       // 21
  "PulseDisplay",           // 22
};

ComputerTask::ComputerTask(const char* name, unsigned int period)
  : AudioPlayerTask(nullptr, 0, name, period)
  , currentDisplaySelection(0)
  , displaySequence(EffectCount())
  , startTime(0)
  , patternState(RandomPattern)
  , repeatTrack(0)
{
}

void ComputerTask::Init()
{
  SetPath("COMPUTR", 7);
  AudioPlayerTask::Init();

  cycleTime = FlashSettings::instance->Get("cycleTime");
  Serial.print("cycleTime: "); Serial.println(cycleTime);

  uint8_t audioSelection   = (SwitchRegister >> 3) & 077;
  uint8_t patternSelection = (SwitchRegister >> 9) & 077;

  // Determine patternState FIRST so TrackStarted() sees the correct
  // state when PlayTrack() calls it below.
  if (patternSelection >= 1 && patternSelection <= EffectCount())
    patternState = ManualPattern;
  else
    patternState = RandomPattern;

  // Start audio. If a specific track is selected, record it for
  // repeating. Otherwise leave repeatTrack = 0 for random mode.
  if (audioSelection >= 1 && audioSelection <= 7)
  {
    repeatTrack = audioSelection;
    PlayTrack(repeatTrack);
  }
  else
  {
    repeatTrack = 0;
    // Random audio driven by AudioPlayerTask::Execute() via StartNextTrack.
  }

  // Start the appropriate display effect.
  if (patternState == ManualPattern)
    StartSpecificEffect(patternSelection);
  else
    StartRandomEffect();
}

void ComputerTask::Execute()
{
  // Check for pattern switch change — immediate, no START required.
  uint8_t patternSelection = (SwitchRegister >> 9) & 077;
  if (patternSelection >= 1 && patternSelection <= EffectCount())
  {
    if (patternState != ManualPattern || (patternSelection - 1) != currentDisplaySelection)
    {
      StopCurrentEffect();
      patternState = ManualPattern;
      StartSpecificEffect(patternSelection);
    }
  }
  else if (patternState == ManualPattern)
  {
    // Switched away from manual to random.
    StopCurrentEffect();
    patternState = RandomPattern;
    StartRandomEffect();
  }

  // Audio is fully handled by AudioPlayerTask::Execute() in all modes.
  AudioPlayerTask::Execute();

  // Cycle display effect on timer in random mode.
  if (patternState == RandomPattern && millis() - startTime > cycleTime * 1000)
    ChangeEffect();
}

// Called by AudioPlayerTask::Execute() when a track finishes (pause-aware).
// Returns false in repeat mode to suppress base class StartNextTrack transition.
// This allows for immediate repeat of the track.
bool ComputerTask::OnTrackFinished()
{
  if (repeatTrack == 0)
    return true; // Random mode: let base class advance to next track normally.

  // Repeat mode: replay unless HALT_ENABLE is set.
  if (IsSenseSwitchActive(HALT_ENABLE))
    Serial.println("ComputerTask: HALT_ENABLE - stopping audio.");
  else
    PlayTrack(repeatTrack);

  return false; // Suppress StartNextTrack — we handled it.
}

void ComputerTask::HandleSenseSwitch()
{
  if (repeatTrack == 0)
  {
    // Random mode: base class handles everything correctly.
    AudioPlayerTask::HandleSenseSwitch();
    return;
  }

  // Repeat-track mode: LOAD and DEPOSIT work as normal (pause/fast-forward),
  // but CONTINUE must replay repeatTrack rather than advancing the sequence.
  if (IsSenseSwitchActive(LOAD))
    TogglePause();

  if (IsSenseSwitchActive(DEPOSIT))
    Step();

  if (IsSenseSwitchActive(CONTINUE))
  {
    if (IsPaused())
      TogglePause(); // unpause before stopping
    if (!IsTrackFinished())
      BT201TransactionTask::instance->Stop(); // only valid while track is playing
    PlayTrack(repeatTrack);
  }
}

void ComputerTask::Cleanup()
{
  AudioPlayerTask::Cleanup();
  StopCurrentEffect();
  DisplayRegister = 0;
}

void ComputerTask::TrackStarted()
{
  // Change display effect on each new track in random audio mode.
  if (patternState == RandomPattern && repeatTrack == 0)
    ChangeEffect();
}

const char* ComputerTask::GetDisplayEffectName(uint8_t index)
{
  return DisplayEffects[index % EffectCount()];
}

uint8_t ComputerTask::EffectCount()
{
  return sizeof(DisplayEffects) / sizeof(DisplayEffects[0]);
}

void ComputerTask::StartRandomEffect()
{
  currentDisplaySelection = displaySequence.Next();
  const char* effectName = GetDisplayEffectName(currentDisplaySelection);
  Serial.print("Display Selection: "); Serial.println(effectName);
  taskScheduler->StartTask(effectName);
  startTime = millis();
}

void ComputerTask::StartSpecificEffect(uint8_t index)
{
  currentDisplaySelection = index - 1;
  const char* effectName = GetDisplayEffectName(currentDisplaySelection);
  Serial.print("Display Selection: "); Serial.println(effectName);
  taskScheduler->StartTask(effectName);
  startTime = millis();
}

void ComputerTask::StopCurrentEffect()
{
  taskScheduler->EndTask(GetDisplayEffectName(currentDisplaySelection));
}

void ComputerTask::ChangeEffect()
{
  StopCurrentEffect();
  StartRandomEffect();
}