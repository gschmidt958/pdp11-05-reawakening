#include "JukeboxTask.h"
#include "RunLight.h"

JukeboxTask::JukeboxTask(const char* name, unsigned int period)
  : AudioPlayerTask(nullptr, 0, name, period)
  , currentFolder(0)
  , style(VUMeterTask::STYLE_L2RVU)
{
}

void JukeboxTask::Setup()
{
  AudioPlayerTask::Setup();
  style = VUMeterTask::STYLE_L2RVU;
}

void JukeboxTask::Init()
{
  uint8_t folderSelection = (SwitchRegister >> 3) & 077;
  uint8_t trackSelection  = (SwitchRegister >> 9) & 077;

  if (!IsValidFolder(folderSelection))
  {
    Serial.println("Jukebox: invalid folder selection");
    RunLight::On();
    return;
  }

  currentFolder = folderSelection;
  uint8_t count = Folders[currentFolder - 1].trackCount;

  SetPath(Folders[currentFolder - 1].name, count);
  AudioPlayerTask::Init();

  if (trackSelection > 0 && trackSelection <= count)
  {
    // Valid track selected: drop the needle at that track, play sequentially
    StartSequential(trackSelection, count);
  }
  // Track 0 or out of range: random play handled by AudioPlayerTask

  VUMeterTask::instance->SetStyle(style);
  taskScheduler->StartTask("VUMeter");
}

void JukeboxTask::Cleanup()
{
  AudioPlayerTask::Cleanup();
  taskScheduler->EndTask("VUMeter");
  RunLight::Off();
  DisplayRegister = 0;
}

bool JukeboxTask::OnTrackFinished()
{
  if (!IsSequenceComplete())
    return true; // Mid-cycle: let base class advance to next track normally.

  // End of cycle: check HALT_ENABLE before resetting anything.
  if (IsSenseSwitchActive(HALT_ENABLE))
    return true; // Let base class handle the stop — sequence stays complete.

  // Re-read folder and track selection.
  uint8_t folderSelection = (SwitchRegister >> 3) & 077;
  uint8_t trackSelection  = (SwitchRegister >> 9) & 077;

  if (IsValidFolder(folderSelection))
    currentFolder = folderSelection;

  uint8_t count = Folders[currentFolder - 1].trackCount;
  SetPath(Folders[currentFolder - 1].name, count);

  if (trackSelection > 0 && trackSelection <= count)
  {
    StartSequential(trackSelection, count);
    return false; // We handled it.
  }

  return true; // Random play: let base class start next track.
}

void JukeboxTask::HandleSenseSwitch()
{
  AudioPlayerTask::HandleSenseSwitch();

  if (IsSenseSwitchActive(EXAMINE))
    AdvanceVUMeterStyle();
}

const JukeboxFolderInfo JukeboxTask::Folders[] = {
  { "SA001",    1 },
  { "SA002",    2 },
  { "SA003",    1 },
  { "SACOVERS", 1 },
};

uint8_t JukeboxTask::FolderCount()
{
  return sizeof(Folders) / sizeof(Folders[0]);
}

bool JukeboxTask::IsValidFolder(uint8_t folder)
{
  return folder >= 1 && folder <= FolderCount();
}

void JukeboxTask::AdvanceVUMeterStyle()
{
  do
  {
    style = (VUMeterTask::Style)(((int)style + 1) % (int)VUMeterTask::STYLE_COUNT);
  }
  while (VUMeterTask::IsVoiceStyle(style)); // Jukebox shows music styles only

  VUMeterTask::instance->SetStyle(style);
}