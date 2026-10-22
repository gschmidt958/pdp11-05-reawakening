#ifndef TASK_H
#define TASK_H

#include <WString.h>

class TaskScheduler;

class Task
{
  public:
    Task(const char* name, unsigned int period = 0);

    virtual void Setup() {};    // one time only initial setup (called when task is first added)
    virtual void Init() {};     // initialization that should occur whenever the task becomes active
    void Run();                 // called for each time slice
    virtual void Execute() {};
    virtual void Cleanup() {};  // called when task becomes inactive (e.g. stop playing music)

    // Orderly task shut down
    virtual void Shutdown(Task* parentTask);
    virtual void ShutdownComplete(Task* childTask);

    //void Suspend();
    //void Resume();
    //bool IsSuspended();
    const char* Name();
    void SetPeriod(unsigned int period);
    //unsigned int GetPreviousTime(); TODO
    void SetTaskScheduler(TaskScheduler* taskScheduler);

  protected:
    TaskScheduler* taskScheduler;

  private:
    const char* name;
    unsigned long period;
    unsigned long previousTime;
    //bool suspended;
};

#endif // TASK_H
