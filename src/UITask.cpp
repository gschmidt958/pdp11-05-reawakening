#include "UITask.h"
#include "SwitchScanTask.h"

UITask::UITask(const char* name, unsigned int period)
  : Task(name, period)
  , newTaskName(nullptr)
  , currentTaskName(nullptr)
  , preemptedTaskName(nullptr)
{
}

void UITask::Setup()
{
  AddPatternTasks();
}

void UITask::Init()
{
  // On startup, run whatever task the switches currently select.
  // After this, task changes only happen when START is pressed.
  panelLockTask = (PanelLockTask*)taskScheduler->GetTask("PanelLock");
 
  const char* initial = GetTaskName(GetSwitchSelection());
  newTaskName = initial ? initial : "DisplaySwitches";
  uiState = ChangeMode;

  SwitchScanTask::instance->Subscribe(UITask::OnToggle, this);
}

void UITask::Cleanup()
{
  taskScheduler->EndTask(currentTaskName);
  SwitchScanTask::instance->Unsubscribe(this);
}


void UITask::Execute()
{
  switch (uiState)
  {
    case Idle:
    {
      panelLockActive = panelLockTask->IsPanelLockActive();

      if (panelLockActive)
      {
        Serial.println("UI - PL Detected");
        newTaskName = "PanelLock";
        preemptedTaskName = currentTaskName;
        taskScheduler->GetTask(currentTaskName)->Shutdown(this);
        uiState = WaitForTaskShutDown;
      }
    }
    break;

    case WaitForTaskShutDown:
      if (!taskScheduler->IsTaskActive(currentTaskName))
        uiState = ChangeMode;
      break;

    case ChangeMode:
      Serial.print("New task to start: ");
      Serial.println(newTaskName);

      taskScheduler->StartTask(newTaskName);
      currentTaskName = newTaskName;
      uiState = panelLockActive ? PanelLockActive : Idle;
      break;

    case PanelLockActive:
    {
      auto panelLock = (PanelLockTask*)taskScheduler->GetTask("PanelLock");
      if (!panelLock->IsPanelLockActive())
      {
        taskScheduler->GetTask(currentTaskName)->Shutdown(this);

        newTaskName       = preemptedTaskName;
        preemptedTaskName = nullptr;
        panelLockActive   = false;
        uiState           = WaitForTaskShutDown;
      }
    }
    break;
  }
}

uint16_t UITask::GetSwitchSelection()
{
  return SwitchRegister & 07;
}

void UITask::OnToggle(void* instance)
{
  static_cast<UITask*>(instance)->HandleSenseSwitch();
}

void UITask::HandleSenseSwitch()
{
  if (uiState != Idle)
    return;

  if (IsSenseSwitchActive(START))
  {
    randomSeed(millis());
    const char* candidate = GetTaskName(GetSwitchSelection());
    if (candidate)
    {
      Serial.print("current task to Shutdown: ");
      Serial.println(currentTaskName);

      newTaskName = candidate;
      taskScheduler->GetTask(currentTaskName)->Shutdown(this);
      uiState = WaitForTaskShutDown;
    }
  }
}

const char* UITask::GetTaskName(uint16_t selection)
{
  static const struct {
    uint16_t    id;
    const char* name;
  } taskMap[] = {
    { 0, "DisplaySwitches" },
    { 1, "Clock"           },
    { 2, "Computer"        },
    { 3, "Soundboard"      },
    { 4, "KnightRider"     },
    { 5, "Jukebox"         },
    { 6, "Bluetooth"       },
    { 7, "Flash"           }
  };

  for (auto& entry : taskMap)
    if (entry.id == selection)
      return entry.name;

  return nullptr;
}