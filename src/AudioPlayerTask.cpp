#include "AudioPlayerTask.h"
#include "Registers.h"
#include "FlashTask.h"

AudioPlayerTask::AudioPlayerTask(const char* folder, uint16_t trackCount, const char* name, unsigned int period)
  : BT201ParentTask(name, period)
  , folder(folder)
  , trackSequence(trackCount > 0 ? trackCount : 1)
  , currentTrackNumber(0)
  , trackCount(0)
  , sequential(false)
{
}

void AudioPlayerTask::Init()
{
    stepCount = FlashSettings::instance->Get("stepCount");
    SwitchScanTask::instance->Subscribe(AudioPlayerTask::OnToggle, this);
    playerState = StartNextTrack;
    bypassHaltOnce  = false;
}

void AudioPlayerTask::TrackStarted()
{
  // Default implementation does nothing.
}

bool AudioPlayerTask::OnTrackFinished()
{
  return true; // Proceed with StartNextTrack by default.
}

void AudioPlayerTask::SetPath(const char* newPath, uint16_t count)
{
  folder = newPath;
  trackSequence.Reset(count);
  trackCount = count;
}

void AudioPlayerTask::StartSequential(uint8_t startTrack, uint8_t count)
{
  trackCount = count;
  currentTrackNumber = startTrack;
  sequential = true;
  playerState = StartNextTrack;
  PlayTrack(currentTrackNumber);
}

void AudioPlayerTask::PlayTrack(uint8_t trackNumber)
{
  Serial.println(folder);
  BT201TransactionTask::instance->PlayTrack(folder, trackNumber);
  playerState = TrackPlaying;
  TrackStarted();
}

bool AudioPlayerTask::IsTrackFinished()
{
  return !taskScheduler->IsTaskActive("BT201Transaction");
}

void AudioPlayerTask::TogglePause()
{
  if (playerState == TrackPlaying)
  {
    BT201TransactionTask::instance->TogglePause();
    playerState = Paused;
  }
  else if (playerState == Paused)
  {
    BT201TransactionTask::instance->TogglePause();
    playerState = TrackPlaying;
  }
}

void AudioPlayerTask::Step()
{
  if (playerState != TrackPlaying) return;
  if (IsSenseSwitchActive(HALT_ENABLE))
    BT201TransactionTask::instance->Rewind(stepCount);
  else
    BT201TransactionTask::instance->FastForward(stepCount);
}

void AudioPlayerTask::Execute()
{
  // Invalid folder guard.
  if (folder == nullptr)
    return;

  switch (playerState)
  {
    case StartNextTrack:
      if (sequential)
      {
        // Advance to next sequential track
        if (currentTrackNumber < trackCount)
        {
          currentTrackNumber++;
          PlayTrack(currentTrackNumber);
        }
        else
        {
          // End of folder
          if (!IsSenseSwitchActive(HALT_ENABLE) || bypassHaltOnce )
          {
            // HALT_ENABLE off: repeat from track 1
            currentTrackNumber = 1;
            PlayTrack(currentTrackNumber);
            bypassHaltOnce  = false;
          }
          else
          {
            // HALT_ENABLE on: stop
            playerState = Stopped;
          }
        }
      }
      else
      {
        // Random mode: check for cycle completion
        if (trackSequence.IsComplete())
        {
          if (IsSenseSwitchActive(HALT_ENABLE) && !bypassHaltOnce )
          {
            // HALT_ENABLE on: stop at end of cycle
            playerState = Stopped;
            break;
          }
          // HALT_ENABLE off or bypassing halt: repeat — RandomSequence resets itself in Next()
        }
        bypassHaltOnce = false;
        currentTrackNumber = trackSequence.Next() + 1;
        Serial.print("Current Track Number: "); Serial.println(currentTrackNumber);
        PlayTrack(currentTrackNumber);
      }
      break;

    case TrackPlaying:
      if (IsTrackFinished())
      {
        if (OnTrackFinished())
          playerState = StartNextTrack;
      }
      break;

    case Paused:
      break; // Just wait — nothing to check.

    case Stopped:
      break;
  }
}

void AudioPlayerTask::Cleanup()
{
  SwitchScanTask::instance->Unsubscribe(this);
}

void AudioPlayerTask::HandleSenseSwitch()
{
  // 1. Handle non-skip switches
  if (IsSenseSwitchActive(LOAD))
    TogglePause();

  if (IsSenseSwitchActive(DEPOSIT))
    Step();

  // 2. Only proceed if CONTINUE is pressed
  if (!IsSenseSwitchActive(CONTINUE)) return;

  // 3. If a track is playing or paused, stop it — Execute() will pick up via StartNextTrack.
  //    If already stopped, kick off StartNextTrack directly.
  if (playerState == Stopped)
  {
    bypassHaltOnce  = true;
    playerState = StartNextTrack;
  }
  else
  {
    BT201TransactionTask::instance->Stop();
    playerState = TrackPlaying; // Ensure Execute() doesn't ignore the track-finished check
  }
}

void AudioPlayerTask::OnToggle(void* instance)
{
  static_cast<AudioPlayerTask*>(instance)->HandleSenseSwitch();
}