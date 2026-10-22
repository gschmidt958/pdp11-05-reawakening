#ifndef ALGORITHMICPATTERNTASKS_H
#define ALGORITHMICPATTERNTASKS_H

#include "Registers.h"
#include "Sequencer.h"
#include "Bits.h"
#include "RandomSequence.h"

/*
 * AlgorithmicPatternTasks
 *
 * A collection of display tasks that animate the display
 * register using simple algorithmic patterns.
 *
 * BINARYCOUNTERDISPLAYTASK
 *   Increments the display register by one each tick,
 *   producing a straightforward binary count.
 *
 * UPDOWNCOUNTERDISPLAYTASK
 *   Runs two 8-bit counters simultaneously — one counting
 *   up, one counting down — and combines them into the
 *   display register (high byte / low byte).
 *
 * RANDOMDISPLAYTASK
 *   Writes a new random 16-bit value to the display
 *   register each tick.
 *
 * RANDOMBITFILLDISPLAYTASK
 *   Fills the display register one random bit at a time
 *   until all bits are set, then clears bits one at a time
 *   in the same fashion, alternating indefinitely.
 *   Uses a Fisher-Yates shuffle over bit indices to ensure
 *   each bit is visited exactly once per fill or clear pass.
 *
 * OverrideCountdownDisplayTask
 *   Counts down from a caller-supplied starting value to 0.
 *   When count reaches barCount (default 10, range 0–16),
 *   switches to a collapsing bar in bits (barCount-1):0 that
 *   shrinks left-to-right one bit per tick. Set barCount to 0
 *   for binary countdown only with no bar phase. Holds 0 on
 *   the display after the count reaches zero until the task
 *   is ended externally.
 *
 * FinalCountdownDisplayTask
 *   Counts down from 29, displaying the binary count directly
 *   in the display register. When count reaches barCount (default
 *   10, range 0–16), switches to a collapsing bar in bits
 *   (barCount-1):0 that shrinks left-to-right one bit per tick,
 *   reaching all zeros at ~10:00. Set barCount to 0 for binary
 *   countdown only with no bar phase. Holds 0 until ended
 *   externally. Tick period is tuned empirically to synchronize
 *   with the audio track.
 *
 * DamagedRadiateDisplayTask
 *   Runs the radiate animation internally while progressively
 *   killing pixels across three explosion phases timed from
 *   task start (10:00). Kill timestamps are precomputed in
 *   Init() so each pixel death fires at a precise elapsed
 *   time with no interval arithmetic or carryover at runtime.
 *
 *   Phase 1 (0s-18s):    7 pixels killed, evenly spaced.
 *   Phase 2 (18s-33s):   8 pixels killed, evenly spaced.
 *   Phase 3 (33s-40.5s): 1 pixel killed at 40500ms.
 *
 *   After the last pixel dies the display holds blank until
 *   ended externally by PanelLockTask when audio ends.
 */

// ---------------------------------------------------------------------------
// BinaryCounterDisplayTask
// ---------------------------------------------------------------------------
class BinaryCounterDisplayTask : public Task
{
  public:
    BinaryCounterDisplayTask(const char* name, unsigned int period = 0);

    void Init() override;
    void Cleanup() override;
    void Execute() override;
};

// ---------------------------------------------------------------------------
// UpDownCounterDisplayTask
// ---------------------------------------------------------------------------
class UpDownCounterDisplayTask : public Task
{
  public:
    UpDownCounterDisplayTask(const char* name, unsigned int period = 0);

    void Init() override;
    void Cleanup() override;
    void Execute() override;

  private:
    uint8_t up;
    uint8_t down;
};

// ---------------------------------------------------------------------------
// RandomDisplayTask
// ---------------------------------------------------------------------------
class RandomDisplayTask : public Task
{
  public:
    RandomDisplayTask(const char* name, unsigned int period = 0);

    void Init() override;
    void Cleanup() override;
    void Execute() override;
};

// ---------------------------------------------------------------------------
// RandomBitFillDisplayTask
// ---------------------------------------------------------------------------
class RandomBitFillDisplayTask : public Task
{
  public:
    RandomBitFillDisplayTask(const char* name, unsigned int period = 0);

    void Init() override;
    void Cleanup() override;
    void Execute() override;

  private:
    bool           addingBits;
    RandomSequence bitSequence;
};

// ---------------------------------------------------------------------------
// OverrideCountdownDisplayTask
// ---------------------------------------------------------------------------
class OverrideCountdownDisplayTask : public Task
{
  public:
    OverrideCountdownDisplayTask(const char* name, unsigned int period = 0, int startCount = 60);

    void Init() override;
    void Cleanup() override;
    void Execute() override;

    void SetOverrideBarCount(int count) { barCount = count; }

  private:
    int startCount;
    int count;
    int barCount = 10;
};

// ---------------------------------------------------------------------------
// FinalCountdownDisplayTask
// ---------------------------------------------------------------------------
class FinalCountdownDisplayTask : public Task
{
  public:
    FinalCountdownDisplayTask(const char* name, unsigned int period = 0);

    void Init() override;
    void Cleanup() override;
    void Execute() override;

    void SetFinalBarCount(int count) { barCount = count; }

  private:
    int count;
    int barCount = 10;
};

// ---------------------------------------------------------------------------
// DamagedRadiateDisplayTask
// ---------------------------------------------------------------------------
class DamagedRadiateDisplayTask : public Task
{
  public:
    DamagedRadiateDisplayTask(const char* name, unsigned int period = 0);

    void Init() override;
    void Cleanup() override;
    void Execute() override;

  private:
    int            frameIndex;
    uint16_t       pixelMask;
    int            pixelsKilled;
    unsigned long  startTime;
    unsigned long  killTimes[RandomSequence::MAX_SIZE];  // sized to MAX_SIZE; total kills across all phases in killSchedule must not exceed this
    RandomSequence pixelSequence;
};

#endif // ALGORITHMICPATTERNTASKS_H