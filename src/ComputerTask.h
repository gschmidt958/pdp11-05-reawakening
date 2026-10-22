#ifndef COMPUTERTASK_H
#define COMPUTERTASK_H

#include "AudioPlayerTask.h"
#include "FlashTask.h"
#include "Registers.h"
#include "RandomSequence.h"

/*
 * ComputerTask
 *
 * Extends AudioPlayerTask to drive the "computer" display
 * effects in sync with audio playback.
 *
 * AUDIO MODES
 *   Random: tracks from the COMPUTR folder play in shuffled
 *           cycles. Display effect changes with each new
 *           track.
 *   Repeat: a specific track plays continuously. Display
 *           effect is independent of audio.
 *   Mode is determined at Init() by the audio selection
 *   bits of SwitchRegister (bits 3-8). A value of 1-7
 *   selects repeat mode; 0 selects random.
 *
 * DISPLAY MODES
 *   Random:  effects cycle automatically on a timer
 *            (cycleTime flash setting).
 *   Manual:  a specific effect is selected via the pattern
 *            selection bits of SwitchRegister (bits 9-14).
 *            A value of 1-N selects an effect directly,
 *            where N is the number of effects in the
 *            DisplayEffects array.
 *   Pattern switches are monitored continuously in
 *   Execute() — changes take effect immediately without
 *   requiring a restart.
 *
 * HALT_ENABLE SWITCH
 *   In repeat mode, stops audio at the end of the current
 *   track rather than replaying. Has no effect on the
 *   display. Inherited random-mode behavior from
 *   AudioPlayerTask applies in random audio mode.
 *
 * SENSE SWITCH CONTROLS
 *   LOAD:     Toggle pause/resume (inherited).
 *   DEPOSIT:  Fast-forward (inherited).
 *   CONTINUE: In random mode, skip to next track
 *             (inherited). In repeat mode, restart the
 *             repeat track from the beginning.
 */

class ComputerTask : public AudioPlayerTask
{
public:
  ComputerTask(const char* name, unsigned int period = 0);

  void Init() override;
  void Execute() override;
  void Cleanup() override;
  bool OnTrackFinished() override;
  void HandleSenseSwitch() override;
  void TrackStarted() override;

private:
  enum PatternState { RandomPattern, ManualPattern };

  static const char* DisplayEffects[];
  static const char* GetDisplayEffectName(uint8_t index);
  static uint8_t EffectCount();

  void StartRandomEffect();
  void StartSpecificEffect(uint8_t index);
  void StopCurrentEffect();
  void ChangeEffect();

  PatternState patternState;
  unsigned long startTime;
  uint16_t cycleTime;
  uint8_t currentDisplaySelection;
  uint8_t repeatTrack; // 0 = random mode, 1-7 = repeat this specific track
  RandomSequence displaySequence;
};

#endif // COMPUTERTASK_H