#ifndef UITASK_H
#define UITASK_H

#include <Arduino.h>
#include "Registers.h"
#include "PatternTasks.h"
#include "PanelLockTask.h"

/*
 * UITask
 *
 * Top-level user interface controller. Manages task
 * selection and switching in response to front panel
 * input, and handles panel lock preemption.
 *
 * TASK SELECTION
 *   On startup, the task corresponding to the current
 *   switch register value (bits 0-2) is started
 *   automatically. Subsequent task changes require the
 *   START switch to be pressed. The switch register is
 *   sampled at the moment START is pressed, allowing the
 *   user to dial in a selection before committing.
 *
 * TASK SWITCHING
 *   Task switches are cooperative — the current task is
 *   asked to shut down gracefully via Shutdown() before
 *   the new task is started. UITask waits in
 *   WaitForTaskShutDown until the current task exits,
 *   then transitions to ChangeMode to start the new task.
 *
 * PANEL LOCK
 *   If the panel lock signal becomes active, UITask
 *   preempts the current task, records its name, and
 *   starts PanelLockTask in its place. When the lock is
 *   released, the preempted task is restored using the
 *   same cooperative shutdown mechanism.
 *
 * STATE MACHINE
 *   IDLE               - Normal operation, monitoring for
 *                        START press or panel lock.
 *   WaitForTaskShutDown - Waiting for the current task
 *                        to finish shutting down.
 *   ChangeMode         - Starts the new task and returns
 *                        to IDLE or PanelLockActive.
 *   PanelLockActive    - Panel lock is in effect,
 *                        monitoring for release.
 */

class UITask : public Task
{
public:
  UITask(const char* name, unsigned int period = 0);

  void Setup() override;
  void Init() override;
  void Cleanup() override;
  void Execute() override;
  void HandleSenseSwitch();

  uint16_t GetSwitchSelection();
  static void OnToggle(void* instance);
  static const char* GetTaskName(uint16_t selection);

private:
  enum UIState
  {
    Idle,
    WaitForTaskShutDown,
    ChangeMode,
    PanelLockActive
  };

  UIState uiState;
  PanelLockTask* panelLockTask;
  bool panelLockActive;

  const char* newTaskName;
  const char* currentTaskName;
  const char* preemptedTaskName;
};

#endif // UITASK_H