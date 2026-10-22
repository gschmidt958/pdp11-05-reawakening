#include <TaskScheduler.h>
#include <const char*.h>

TaskList::TaskList()
  : taskCount(0)
  , currentTask(0)
{
  Reset();
  for (int i = 0; i < maxTasks; i++)
    taskList[i] = nullptr;
}

bool TaskList::AddTask(Task* task)
{
  if (taskCount >= maxTasks)
    return false;

  taskList[taskCount++] = task;
  return true;
}

Task* TaskList::RemoveTask(const char* name)
{
  if (taskCount == 0)
    return nullptr;

  for (unsigned int taskIndex = 0; taskIndex < taskCount; taskIndex++)
  {
    if (strcmp(taskList[taskIndex]->Name(), name) == 0)
    {
      Task* task = taskList[taskIndex];

      // Close the hole by shifting down
      for (unsigned int shiftTaskIndex = taskIndex; shiftTaskIndex < taskCount - 1; shiftTaskIndex++)
        taskList[shiftTaskIndex] = taskList[shiftTaskIndex + 1];

      --taskCount;
      Reset();
      return task;
    }
  }
  return nullptr;
}

Task* TaskList::GetTask(const char* name)
{
  for (unsigned int taskIndex = 0; taskIndex < taskCount; taskIndex++)
  {
    if (strcmp(taskList[taskIndex]->Name(), name) == 0)
      return taskList[taskIndex];
  }
  return nullptr;
}

void TaskList::Reset()
{
  currentTask = 0;
}

Task* TaskList::Next()
{
  Task* task = taskList[currentTask++];
  currentTask = currentTask % taskCount;
  return task;
}

TaskScheduler::TaskScheduler()
{
}

bool TaskScheduler::AddTask(Task* task)
{
  task->SetTaskScheduler(this);
  task->Setup();
  return inactiveTasks.AddTask(task);
}

bool TaskScheduler::StartTask(const char* name)
{
  Task* task = inactiveTasks.RemoveTask(name);
  if (task == nullptr)
    return false;

  task->Init();
  return activeTasks.AddTask(task);
}

bool TaskScheduler::EndTask(const char* name)
{
  Task* task = activeTasks.RemoveTask(name);
  if (task == nullptr)
    return false;

  task->Cleanup();
  return inactiveTasks.AddTask(task);
}

Task* TaskScheduler::GetTask(const char* name)
{
  Task* task = activeTasks.GetTask(name);
  if (task == nullptr)
    task = inactiveTasks.GetTask(name);

  return task;
}

bool TaskScheduler::IsTaskActive(const char* name)
{
  return activeTasks.GetTask(name) != nullptr;
}

void TaskScheduler::Run()
{
  Task* task = activeTasks.Next();
  if (task != nullptr)
    task->Run();
}
