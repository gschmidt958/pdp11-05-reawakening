#include "Task.h"
#include <Arduino.h>

#include <TaskScheduler.h>


Task::Task(const char* name, unsigned int period)
  : name(name)
  , period(period)
  , previousTime(0)
{
}

void Task::Run()
{
  unsigned long currentTime = millis();
  if (currentTime - previousTime >= period)
  {
    previousTime = currentTime;
    Execute();
  }
}

void Task::Shutdown(Task* parentTask)
{
  // Default behavior is to end this task.
  taskScheduler->EndTask(Name());
}

void Task::ShutdownComplete(Task* childTask)
{
  // Do nothing.
}

const char* Task::Name()
{
  return name;
}

void Task::SetPeriod(unsigned int period)
{
  this->period = period;
}

void Task::SetTaskScheduler(TaskScheduler* taskScheduler)
{
  this->taskScheduler = taskScheduler;
}
