#ifndef BT201PARENTTASK_H
#define BT201PARENTTASK_H

#include <Task.h>

/*
 * BT201ParentTask
 *
 * Base class for tasks that play audio via BT201TransactionTask.
 * Ensures that when a task switch is requested, the derived task
 * remains active until any playing audio has fully stopped —
 * preventing audio from bleeding into the next task.
 *
 * USAGE
 *   Derive your task from BT201ParentTask instead of Task
 *   whenever it plays audio via BT201TransactionTask. No other
 *   changes are required — the shutdown handshake is handled
 *   entirely by this base class.
 *
 * SHUTDOWN SEQUENCE
 *   1. Parent (e.g. UITask) calls Shutdown() on this task.
 *   2. Shutdown() instructs BT201TransactionTask to stop audio
 *      and shut down.
 *   3. BT201TransactionTask stops playback and calls back
 *      ShutdownComplete() once audio has terminated.
 *   4. This task exits, guaranteeing silence before the next
 *      task begins.
 */

class BT201ParentTask : public Task
{
  public:
    BT201ParentTask(const char* name, unsigned int period = 0);

    // Called by our parent (e.g. UITask) to request shutdown.
    void Shutdown(Task *parentTask) override;
    // Called by BT201TransactionTask once it has shut itself down.
    void ShutdownComplete(Task *childTask) override;
};

#endif // BT201PARENTTASK_H