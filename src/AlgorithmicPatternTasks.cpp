#include "AlgorithmicPatternTasks.h"

// ===========================================================================
// BinaryCounterDisplayTask
// ===========================================================================

BinaryCounterDisplayTask::BinaryCounterDisplayTask(const char* name, unsigned int period)
  : Task(name, period)
{
}

void BinaryCounterDisplayTask::Init()
{
  DisplayRegister = 0;
}

void BinaryCounterDisplayTask::Cleanup()
{
  DisplayRegister = 0;
}

void BinaryCounterDisplayTask::Execute()
{
  ++DisplayRegister;
}

// ===========================================================================
// UpDownCounterDisplayTask
// ===========================================================================

UpDownCounterDisplayTask::UpDownCounterDisplayTask(const char* name, unsigned int period)
  : Task(name, period)
{
}

void UpDownCounterDisplayTask::Init()
{
  up = 0; down = 0;
  DisplayRegister = 0;
}

void UpDownCounterDisplayTask::Cleanup()
{
  DisplayRegister = 0;
}

void UpDownCounterDisplayTask::Execute()
{
  up++; down--;
  DisplayRegister = ((down & 0xff) << 8) | (up & 0xff);
}

// ===========================================================================
// RandomDisplayTask
// ===========================================================================

RandomDisplayTask::RandomDisplayTask(const char* name, unsigned int period)
  : Task(name, period)
{
}

void RandomDisplayTask::Init()
{
  DisplayRegister = 0;
}

void RandomDisplayTask::Cleanup()
{
  DisplayRegister = 0;
}

void RandomDisplayTask::Execute()
{
  DisplayRegister = (uint16_t)random(0, 65536);
}

// ===========================================================================
// RandomBitFillDisplayTask
// ===========================================================================

RandomBitFillDisplayTask::RandomBitFillDisplayTask(const char* name, unsigned int period)
  : Task(name, period)
  , bitSequence(16)
{
}

void RandomBitFillDisplayTask::Init()
{
  addingBits      = true;
  DisplayRegister = 0;
  bitSequence.Reset(16);
}

void RandomBitFillDisplayTask::Cleanup()
{
  DisplayRegister = 0;
}

void RandomBitFillDisplayTask::Execute()
{
  int bit = bitSequence.Next();

  if (addingBits)
    DisplayRegister |= (1u << bit);
  else
    DisplayRegister &= ~(1u << bit);

  if (bitSequence.IsComplete())
    addingBits = !addingBits;
}

// ===========================================================================
// OverrideCountdownDisplayTask
// ===========================================================================

OverrideCountdownDisplayTask::OverrideCountdownDisplayTask(const char* name, unsigned int period, int startCount)
  : Task(name, period)
  , startCount(startCount)
{
}

void OverrideCountdownDisplayTask::Init()
{
  count = startCount;
  DisplayRegister = count;
}

void OverrideCountdownDisplayTask::Cleanup()
{
  DisplayRegister = 0;
}

void OverrideCountdownDisplayTask::Execute()
{
  if (count > 0)
    count--;

  if (count >= barCount + 1)
  {
    // Binary countdown phase
    DisplayRegister = count;
  }
  else
  {
    // Collapsing bar phase
    DisplayRegister = (1 << count) - 1;
  }
}

// ===========================================================================
// FinalCountdownDisplayTask
// ===========================================================================

FinalCountdownDisplayTask::FinalCountdownDisplayTask(const char* name, unsigned int period)
  : Task(name, period)
{
}

void FinalCountdownDisplayTask::Init()
{
  count = 30;
  DisplayRegister = count;
}

void FinalCountdownDisplayTask::Cleanup()
{
  DisplayRegister = 0;
}

void FinalCountdownDisplayTask::Execute()
{
  if (count > 0)
    --count;

  if (count >= barCount + 1)
  {
    // Binary countdown phase
    DisplayRegister = count;
  }
  else
  {
    // Collapsing bar phase: barCount down to 0
    // count==barCount -> all barCount bits set
    // count==1        -> 0b0000000000000001
    // count==0        -> 0b0000000000000000
    DisplayRegister = (1 << count) - 1;
  }
}

// ===========================================================================
// DamagedRadiateDisplayTask
// ===========================================================================

// Radiate pattern frames (all 16 bits, same as radiateDisplayTask)
static const uint16_t damagedRadiatePattern[] = {
  0b0000000000000000,
  0b0000000110000000,
  0b0000001111000000,
  0b0000011111100000,
  0b0000111111110000,
  0b0001111111111000,
  0b0011111111111100,
  0b0111111111111110,
  0b1111111111111111
};
static const int damagedRadiateFrameCount = sizeof(damagedRadiatePattern) / sizeof(uint16_t);

// Kill schedule: one entry per explosion phase.
// count      — number of pixels killed in this phase.
// startMs    — when the phase begins (ms from task start).
//              For phases with count == 1, the pixel fires at startMs exactly.
// durationMs — time window over which kills are evenly spaced.
//              Ignored when count == 1.
struct KillPhase
{
  int           count;
  unsigned long startMs;
  unsigned long durationMs;
};

// Total kill count across all phases must not exceed RandomSequence::MAX_SIZE (currently 64).
static const KillPhase killSchedule[] = {
  {  7,     0, 18000 },  // Phase 1: 7 kills evenly spaced over  0s – 18s
  {  8, 18000, 15000 },  // Phase 2: 8 kills evenly spaced over 18s – 33s
  {  1, 35000,     0 },  // Phase 3: 1 kill at 35s
};
static const int killScheduleCount = sizeof(killSchedule) / sizeof(KillPhase);

DamagedRadiateDisplayTask::DamagedRadiateDisplayTask(const char* name, unsigned int period)
  : Task(name, period)
  , pixelSequence(16)
{
}

void DamagedRadiateDisplayTask::Init()
{
  frameIndex   = 0;
  pixelMask    = 0xFFFF;
  pixelsKilled = 0;
  startTime    = millis();
  pixelSequence.Reset(16);

  // Precompute kill timestamps (ms from task start) from the kill schedule
  int slot = 0;
  for (int p = 0; p < killScheduleCount; p++)
  {
    const KillPhase& phase = killSchedule[p];
    if (phase.count == 1)
       killTimes[slot++] = phase.startMs;
    else
    {
      for (int i = 0; i < phase.count; i++)
        killTimes[slot++] = phase.startMs + (unsigned long)(i + 1) * (phase.durationMs / phase.count);
    }
  }

  DisplayRegister = damagedRadiatePattern[frameIndex] & pixelMask;
}

void DamagedRadiateDisplayTask::Cleanup()
{
  DisplayRegister = 0;
}

void DamagedRadiateDisplayTask::Execute()
{
  unsigned long now     = millis();
  unsigned long elapsed = now - startTime;

  // Advance radiate frame
  frameIndex = (frameIndex + 1) % damagedRadiateFrameCount;

  // Kill next pixel if its scheduled time has arrived
  if (pixelsKilled < 16 && elapsed >= killTimes[pixelsKilled])
  {
    int bit = pixelSequence.Next();
    pixelMask &= ~(1 << bit);
    pixelsKilled++;
  }

  DisplayRegister = damagedRadiatePattern[frameIndex] & pixelMask;
}