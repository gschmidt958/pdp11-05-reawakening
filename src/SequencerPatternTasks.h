#ifndef SEQUENCERPATTERNTASKS_H
#define SEQUENCERPATTERNTASKS_H

#include "Sequencer.h"
#include "KnightRiderTask.h"

/*
 * SequencerPatternTasks
 *
 * Declares all sequencer-driven display pattern tasks. Each task
 * is driven by a static bit array defining a full animation cycle
 * as a sequence of 16-bit display register values. The period
 * parameter controls the frame rate in milliseconds.
 *
 * Pattern arrays and task instances are defined in
 * SequencerPatternTasks.cpp.
 */

extern SequencerDisplayTask knightRiderDisplayTask;
extern SequencerDisplayTask crissCrossDisplayTask;
extern SequencerDisplayTask checkerboardDisplayTask;
extern SequencerDisplayTask wigwagDisplayTask;
extern SequencerDisplayTask diamondDisplayTask;
extern SequencerDisplayTask dissipateDisplayTask;
extern SequencerDisplayTask chaserDisplayTask;
extern SequencerDisplayTask radiateDisplayTask;
extern SequencerDisplayTask pulseDisplayTask;
extern SequencerDisplayTask snakeDisplayTask;
extern SequencerDisplayTask implodeDisplayTask;
extern SequencerDisplayTask implode2DisplayTask;
extern SequencerDisplayTask splitDisplayTask;
extern SequencerDisplayTask squeezeDisplayTask;
extern SequencerDisplayTask marquisDisplayTask;
extern SequencerDisplayTask chopsticksDisplayTask;
extern SequencerDisplayTask panelDisplayTask;
extern SequencerDisplayTask octalCountDisplayTask;
extern SequencerDisplayTask octalFillDisplayTask;

#endif // SEQUENCERPATTERNTASKS_H