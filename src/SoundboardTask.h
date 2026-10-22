#ifndef SOUNDBOARDTASK_H
#define SOUNDBOARDTASK_H

#include "BT201ParentTask.h"
#include "BT201TransactionTask.h"
#include "VUMeterTask.h"
#include "RandomSequence.h"
#include "SwitchScanTask.h"
#include "Registers.h"
#include "FlashTask.h"

/*
 * SoundboardTask
 *
 * Plays themed audio tracks on a timed schedule, with an
 * optional idle display effect between tracks. Derives from
 * BT201ParentTask to ensure clean audio shutdown on task exit.
 *
 * THEMES
 *   Each theme defines a folder, track count, and default VU
 *   meter style. The theme is selected from the theme selection
 *   bits of SwitchRegister (bits 3-8) at Init() and again at
 *   the end of each track cycle. A valid selection pins the
 *   task to that theme; any other value enables random theme
 *   cycling. The mode can switch between fixed and random at
 *   any cycle boundary by adjusting the switch register.
 *
 * TRACK SCHEDULING
 *   Tracks within a theme play in random non-repeating order.
 *   After all tracks in a theme have played, the next theme
 *   is selected (randomly or from the switch register) and
 *   the cycle repeats. The gap between tracks is a fixed
 *   interval plus a random additional delay, both loaded
 *   from flash at Init().
 *
 * IDLE DISPLAY
 *   While waiting between tracks, an optional display effect
 *   runs. The effect is selected by the idlePattern flash
 *   setting: 0 = off, 1 = rotation, 2 = random dot.
 *
 * VU METER
 *   Each theme carries a default VU meter style. The EXAMINE
 *   sense switch cycles through voice styles manually; once
 *   changed manually the theme default is no longer applied
 *   until the task restarts.
 *
 * PAUSE
 *   When HALT_ENABLE is active the task suspends scheduling
 *   and waits. Playback resumes when HALT_ENABLE is cleared.
 */

class SoundboardTask : public BT201ParentTask
{
public:
  SoundboardTask(const char* name, unsigned int period = 0);

  void Init() override;
  void Execute() override;
  void Cleanup() override;

private:
  struct Theme
  {
    const char* folder;
    uint8_t trackCount;
    VUMeterTask::Style style;
  };

  static const Theme Themes[];
  static uint8_t ThemeCount();

  enum State { Idle, Playing, Paused };

  void LoadTheme();
  void PlayNextTrack();
  void ScheduleNextPlay();
  void AdvanceVUMeterStyle();
  void IdleRotationEffect();
  void IdleRandomDotEffect();
  void HandleSenseSwitch();
  static void OnToggle(void* instance);

  State state;
  uint8_t currentTheme;
  RandomSequence trackSequence;
  RandomSequence themeSequence;
  bool randomThemeMode;
  uint16_t fixedInterval;
  uint16_t randomInterval;
  uint16_t idlePattern;
  uint32_t nextPlayTime;
  bool styleChanged;
  uint8_t idleRotationIndex;
};

#endif // SOUNDBOARDTASK_H