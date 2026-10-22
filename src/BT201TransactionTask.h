#ifndef BT201TRANSACTIONTASK_H
#define BT201TRANSACTIONTASK_H

#include "Task.h"
#include "BT201Transaction.h"
#include "BT201CircularBuffer.h"
#include "CharacterBuffer.h"
#include "IBT201SyncResponseClient.h"

/*
 * BT201TransactionTask
 *
 * Manages the AT command transaction FSM for the BT201 module.
 * Sends commands over Serial2 and handles incoming sync responses
 * pushed directly by BT201ReceiverTask via IBT201SyncResponseClient.
 *
 * FSM STATES
 *   ProcessTransaction      — dequeue and send the next command.
 *   WaitForResponse         — awaits the immediate ACK; times out if none arrives.
 *   WaitForExtendedResponse — awaits track completion (MV/MD/MO); manages OOB
 *                             commands in flight; times out stale OOB only.
 *
 * RESPONSE DISPATCH  (OnSyncResponse → HandleResponse / HandleExtendedResponse)
 *   Sync responses are pushed directly from the receiver with no queue or
 *   polling. The protocol guarantees a response only arrives when this task
 *   is waiting for one, so push is always safe and immediate.
 *
 * DISPATCHER PRIORITY (WaitForExtendedResponse only)
 *   1. ER          → hard error; reset and notify parent.
 *   2. OOB active  → route OK to OOB completion / repeat / stop.
 *   3. Template    → match MV/MD/MO track-end sequence.
 *   4. No match    → discard.
 *
 * Execute() tick responsibilities
 *   ProcessTransaction      — send next command or end task.
 *   WaitForResponse         — timeout guard only.
 *   WaitForExtendedResponse — OOB dispatch and OOB timeout guard only.
 *
 * OOB commands (Stop, FastForward, Rewind, TogglePause) are sent
 * only when no other OOB is in flight. Their state is carried
 * entirely by BT201OOBTransaction::state — no additional flags needed.
 *
 * On PlayTrack, the OOB queue and current OOB state are always
 * cleared to prevent stale commands from a previous track bleeding
 * into the new one.
 */

class BT201TransactionTask : public Task,
                             public IBT201SyncResponseClient
{
public:
    static const unsigned long RESPONSE_TIMEOUT_MS = 2000;

    static BT201TransactionTask* instance;

    BT201TransactionTask(const char* name, unsigned int period);

    void Setup()   override;
    void Init()    override;
    void Execute() override;
    void Shutdown(Task* parentTask);

    // IBT201SyncResponseClient
    void OnSyncResponse(const CharacterBuffer& line) override;

    void SetNotificationTask(Task* notificationTask);

    // Commands
    void SoundMode();
    void BluetoothMode();
    void AutoSwitchBluetooth(bool enable);
    void TFMode();
    void PlayTrack(const char* path, uint16_t trackNumber);
    void SetVolume(uint8_t level);
    uint8_t GetVolume();

    // Playback control
    void Stop();
    void TogglePause();
    void FastForward(uint8_t count);
    void Rewind(uint8_t count);
    bool TrackIsPlaying();

    static void FlushSerial();

private:
    enum class TransactionState
    {
        ProcessTransaction,
        WaitForResponse,
        WaitForExtendedResponse
    };

    BT201::CircularBuffer<BT201Transaction,    8> transactionQueue;
    BT201::CircularBuffer<BT201OOBTransaction, 4> oobQueue;

    BT201Transaction    currentTransaction;
    BT201OOBTransaction currentOOBTransaction;

    TransactionState transactionState;

    Task* notificationTask;
    Task* parentTask;

    unsigned long responseStartTime;
    uint8_t       cachedVolumeLevel;

    // Sync response handlers — called from OnSyncResponse().
    void HandleResponse(const CharacterBuffer& line);
    void HandleExtendedResponse(const CharacterBuffer& line);

    void InitiateTransaction(const char* command, const char* responseTemplate);
    void EnqueueOOB(const char* cmd, uint8_t count = 1);
    void EnqueueStopOOB();
    void SendNextOOB();
    bool ResponseTimedOut() const;
};

#endif // BT201TRANSACTIONTASK_H