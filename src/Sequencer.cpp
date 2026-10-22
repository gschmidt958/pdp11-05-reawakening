#include "Sequencer.h"
#include "Registers.h"

Sequencer::Sequencer(uint16_t* pattern, unsigned int size)
  : pattern(pattern)
  , size(size)
{
  Reset();
}

void Sequencer::Reset()
{
  index = 0;
}

uint16_t Sequencer::NextPattern()
{
  uint16_t pattern = this->pattern[index++];
  index %= size;
  return pattern;
}

SequencerDisplayTask::SequencerDisplayTask(const char* name, unsigned int period, uint16_t* pattern, unsigned int size)
  : Task(name, period)
  , sequencer(pattern, size)
{
}

void SequencerDisplayTask::Init()
{
  DisplayRegister = 0;
  sequencer.Reset();
}

void SequencerDisplayTask::Cleanup()
{
  DisplayRegister = 0;
}

void SequencerDisplayTask::Execute()
{
  DisplayRegister = sequencer.NextPattern();
}
