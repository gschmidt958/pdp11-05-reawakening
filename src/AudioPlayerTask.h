#ifndef AUDIOPLAYERTASK_H
#define AUDIOPLAYERTASK_H

#include "BT201ParentTask.h"
#include "BT201TransactionTask.h"
#include "FlashTask.h"
#include "RandomSequence.h"
#include "SwitchScanTask.h"

/*
 * AudioPlayerTask
 *
 * Provides automatic track playback with pause, skip, and
 * loop control via sense switches.
 *
 * PLAYBACK MODES
 *   Sequential: tracks play in order, wrapping to track 1
 *               at the end.
 *   Random:     tracks play in shuffled cycles. A full cycle
 *               completes before repeating.
 *
 * HALT_ENABLE SWITCH
 *   A soft "stop when done" flag. When set, playback stops
 *   at the natural end of a cycle or sequence rather than
 *   looping. A CONTINUE press overrides it, but HALT_ENABLE
 *   reasserts at the next boundary.
 *
 * SENSE SWITCH CONTROLS
 *   LOAD:     Toggle pause/resume. Only effective while a
 *             track is actively playing.
 *   DEPOSIT:  Fast-forward the current track.
 *   CONTINUE: Skip to the next track, unpausing first if
 *             paused. In sequential mode, wraps to track 1
 *             if at the end or stopped.
 *
 * SUBCLASS EXTENSION POINTS
 *   TrackStarted()     - Called when a new track begins.
 *   OnTrackFinished()  - Called when a track ends. Return
 *                        false to suppress the default
 *                        advance to the next track.
 *   HandleSenseSwitch() - Override to customize control
 *                         behavior.
 */
 
class AudioPlayerTask : public BT201ParentTask
{
protected:
    AudioPlayerTask(const char* folder, uint16_t trackCount, const char* name, unsigned int period = 0);

    void Init() override;
    void Execute() override;
    void Cleanup() override;

    virtual void TrackStarted();
    virtual bool OnTrackFinished();  // return false to suppress StartNextTrack transition

    void SetPath(const char* newPath, uint16_t trackCount);
    void StartSequential(uint8_t startTrack, uint8_t count);
    void PlayTrack(uint8_t trackNumber);
    void TogglePause();
    void Step();

    bool IsTrackFinished();
    bool IsPaused() const { return playerState == Paused; }
    bool IsSequenceComplete() const { return trackSequence.IsComplete(); }

    virtual void HandleSenseSwitch();

private:
    enum PlayerState
    {
      StartNextTrack,
      TrackPlaying,
      Paused,
      Stopped,
    };

    PlayerState playerState;

    const char* folder;
    uint8_t currentTrackNumber;
    uint8_t trackCount;
    bool sequential;
    bool bypassHaltOnce ;

    uint8_t stepCount;

    RandomSequence trackSequence;

    static void OnToggle(void* instance);
};

#endif // AUDIOPLAYERTASK_H