#ifndef JUKEBOXTASK_H
#define JUKEBOXTASK_H

#include "AudioPlayerTask.h"
#include "VUMeterTask.h"
#include "SwitchScanTask.h"
#include "Registers.h"

/*
 * JukeboxTask
 *
 * Extends AudioPlayerTask to provide folder-based music
 * playback with an integrated VU meter display.
 *
 * FOLDER SELECTION
 *   The folder to play is determined at Init() from the
 *   folder selection bits of SwitchRegister (bits 3-8).
 *   A value of 1-4 selects a valid folder. Any other
 *   value is rejected and the task exits. At the end of
 *   each folder cycle the folder selection is re-read,
 *   allowing the user to switch folders between cycles
 *   without interrupting play.
 *
 * TRACK SELECTION
 *   The track selection bits of SwitchRegister (bits 9-14)
 *   are read at Init() and again at the start of each new
 *   cycle. A valid track number drops the needle at that
 *   track and plays sequentially to the end of the folder.
 *   A value of 0 or out of range starts random shuffle
 *   play across the folder; when the cycle completes all
 *   tracks have been heard exactly once, then a new random
 *   cycle begins. Once a cycle is underway the switch is
 *   not re-read until the cycle completes, preserving
 *   uninterrupted LP-style play.
 *
 * RUN LIGHT
 *   On when the folder selection is invalid and the task
 *   cannot play. Off on Cleanup(). Not affected by track
 *   selection.
 *
 * VU METER
 *   The VUMeter task is started and stopped automatically
 *   with JukeboxTask. The display style can be cycled
 *   through all non-voice styles using the EXAMINE switch.
 *   The selected style persists across track changes but
 *   resets to L2RVU on each Init().
 *
 * SENSE SWITCH CONTROLS
 *   LOAD:     Toggle pause/resume (inherited).
 *   DEPOSIT:  Fast-forward (inherited).
 *   CONTINUE: Skip to next track (inherited).
 *   EXAMINE:  Cycle VU meter display style.
 */

struct JukeboxFolderInfo { const char* name; uint8_t trackCount; };

class JukeboxTask : public AudioPlayerTask
{
public:
  JukeboxTask(const char* name, unsigned int period = 0);

  void Setup() override;
  void Init() override;
  void Cleanup() override;
  bool OnTrackFinished() override;
  void HandleSenseSwitch() override;

private:
  static const JukeboxFolderInfo Folders[];
  static uint8_t FolderCount();
  static bool IsValidFolder(uint8_t folder);
  void AdvanceVUMeterStyle();

  uint8_t currentFolder;
  VUMeterTask::Style style;
};

#endif // JUKEBOXTASK_H