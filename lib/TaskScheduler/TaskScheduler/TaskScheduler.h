#ifndef TASKSCHEDULER_H
#define TASKSCHEDULER_H

#include "Task.h"

class TaskList
{
  static const unsigned int maxTasks = 64;

  public:
    TaskList();
    bool AddTask(Task* task);
    Task* RemoveTask(const char* name);
    Task* GetTask(const char* name);
    void Reset();
    Task* Next();

  private:
    Task* taskList[maxTasks];
    unsigned int taskCount;
    unsigned int currentTask;
};

class TaskScheduler
{
  public:
    TaskScheduler();
    bool AddTask(Task* task);           // Done once for each task. Adds to inactive task list and calls Setup().
    bool StartTask(const char* name);   // Moves inactive task to active task list.
    bool EndTask(const char* name);     // Moves active task to inactive task list.
    Task* GetTask(const char* name);    // Get a task by name.
    bool IsTaskActive(const char* name); // Is the task by this name on the active task list?
    void Run();                          // Cycles through the active tasks, calling its Run() method.

  private:
    TaskList activeTasks;
    TaskList inactiveTasks;
};

#endif // TASKSCHEDULER_H
