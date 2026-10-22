#ifndef KNIGHTRIDERTASK_H
#define KNIGHTRIDERTASK_H

#include "AudioPlayerTask.h"
#include "Registers.h"

/*
 * KnightRiderTask
 *
 * Plays the single track from the KNIGHTR folder while
 * running the KnightRiderDisplay effect. Both audio and
 * display are started together on Init() and stopped
 * together on Cleanup().
 */

class KnightRiderTask : public AudioPlayerTask
{
public:
  KnightRiderTask(const char* name, unsigned int period = 0)
    : AudioPlayerTask(nullptr, 0, name, period)
  {
  }

  void Init() override
  {
    SetPath("KNIGHTR", 1);
    AudioPlayerTask::Init();
    PlayTrack(1);
    taskScheduler->StartTask("KnightRiderDisplay");
  }

  void Cleanup() override
  {
    AudioPlayerTask::Cleanup();
    taskScheduler->EndTask("KnightRiderDisplay");
    DisplayRegister = 0;
  }
};

#endif // KNIGHTRIDERTASK_H