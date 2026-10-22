#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <Arduino.h>
#include "Task.h"

/*
 * Sequencer / SequencerDisplayTask
 *
 * Sequencer steps through a caller-supplied array of 16-bit
 * patterns in order, wrapping back to the start after the
 * last entry. NextPattern() advances the index and returns
 * the current value.
 *
 * SequencerDisplayTask wraps a Sequencer as a Task, writing
 * the next pattern to DisplayRegister on each Execute() tick.
 * The frame rate is controlled by the period parameter passed
 * to the constructor.
 */

class Sequencer
{
public:
  Sequencer(uint16_t* pattern, unsigned int size);
  void Reset();
  uint16_t NextPattern();

private:
  uint16_t* pattern;
  unsigned int size;
  unsigned int index;
};

class SequencerDisplayTask : public Task
{
public:
  SequencerDisplayTask(const char* name, unsigned int period, uint16_t* pattern, unsigned int size);

  void Init() override;
  void Cleanup() override;
  void Execute() override;

private:
  Sequencer sequencer;
};

#endif // SEQUENCER_H