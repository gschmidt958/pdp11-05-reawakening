#include "BT201ParentTask.h"
#include <TaskScheduler.h>

BT201ParentTask::BT201ParentTask(const char* name, unsigned int period)
  : Task(name, period)

{
}

void BT201ParentTask::Shutdown(Task *parentTask)
{
  //  If BT201 transaction task is active, tell it to shut down.
  //  Otherwise, if it is inactive, it is safe to simply end our own task.
  if (taskScheduler->IsTaskActive("BT201Transaction"))
    taskScheduler->GetTask("BT201Transaction")->Shutdown(this);
  else
    taskScheduler->EndTask(Name());
}

void BT201ParentTask::ShutdownComplete(Task *childTask)
{
  // Default behavior is to end this task.
  taskScheduler->EndTask(Name());
}