#ifndef PATTERNTASKS_H
#define PATTERNTASKS_H

/*
 * PatternTasks
 *
 * Top-level assembly point for all user-selectable pattern tasks.
 * Composes sequencer-driven and algorithmic pattern tasks and
 * registers them with the scheduler via AddPatternTasks().
 *
 * See also:
 *   SequencerPatternTasks.h -- sequencer-driven bit array patterns
 *   AlgorithmicPatternTasks.h -- algorithmically generated patterns
 */

extern void AddPatternTasks();

#endif // PATTERNTASKS_H