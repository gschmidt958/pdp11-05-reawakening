#ifndef TASKSCHEDULE_H
#define TASKSCHEDULE_H

#include <TaskScheduler.h>

/*
 * TaskSchedule
 *
 * Declares the global TaskScheduler instance used throughout
 * the application. Defined in TaskSchedule.cpp. If a second
 * scheduler is added in the future to run tasks on core 1,
 * it would be declared here alongside Scheduler.
 */

extern TaskScheduler Scheduler;

#endif // TASKSCHEDULE_H