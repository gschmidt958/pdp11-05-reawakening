#include "PatternTasks.h"
#include "AlgorithmicPatternTasks.h"
#include "SequencerPatternTasks.h"
#include "TaskSchedule.h"

BinaryCounterDisplayTask binaryCounterDisplayTask("BinaryCounterDisplay", 50);
UpDownCounterDisplayTask upDownCounterDisplayTask("UpDownCounterDisplay", 100);
RandomDisplayTask randomDisplayTask("RandomDisplay", 100);
RandomBitFillDisplayTask randomBitFillDisplayTask("RandomBitFillDisplay", 100);
OverrideCountdownDisplayTask overrideCountdownTask("OverrideCountdownDisplay", 1310, 30);
FinalCountdownDisplayTask finalCountdownTask("FinalCountdownDisplay", 1212);
DamagedRadiateDisplayTask damagedRadiateTask("DamagedRadiateDisplay", 100);

void AddPatternTasks()
{
  // Sequencer-driven patterns
  Scheduler.AddTask(&knightRiderDisplayTask);
  Scheduler.AddTask(&crissCrossDisplayTask);
  Scheduler.AddTask(&checkerboardDisplayTask);
  Scheduler.AddTask(&wigwagDisplayTask);
  Scheduler.AddTask(&diamondDisplayTask);
  Scheduler.AddTask(&dissipateDisplayTask);
  Scheduler.AddTask(&chaserDisplayTask);
  Scheduler.AddTask(&radiateDisplayTask);
  Scheduler.AddTask(&snakeDisplayTask);
  Scheduler.AddTask(&implodeDisplayTask);
  Scheduler.AddTask(&implode2DisplayTask);
  Scheduler.AddTask(&splitDisplayTask);
  Scheduler.AddTask(&squeezeDisplayTask);
  Scheduler.AddTask(&marquisDisplayTask);
  Scheduler.AddTask(&chopsticksDisplayTask);
  Scheduler.AddTask(&panelDisplayTask);
  Scheduler.AddTask(&octalCountDisplayTask);
  Scheduler.AddTask(&octalFillDisplayTask);
  Scheduler.AddTask(&pulseDisplayTask);

  // Algorithmic patterns
  Scheduler.AddTask(&binaryCounterDisplayTask);
  Scheduler.AddTask(&upDownCounterDisplayTask);
  Scheduler.AddTask(&randomDisplayTask);
  Scheduler.AddTask(&randomBitFillDisplayTask);
  Scheduler.AddTask(&overrideCountdownTask);
  Scheduler.AddTask(&finalCountdownTask);
  Scheduler.AddTask(&damagedRadiateTask);
}