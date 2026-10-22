#include <Arduino.h>
#include <TaskScheduler.h>
#include "BT201TransactionTask.h"
#include "BT201Log.h"

BT201TransactionTask* BT201TransactionTask::instance = nullptr;

BT201TransactionTask::BT201TransactionTask(const char* name, unsigned int period)
  : Task(name, period),
    transactionQueue(),
    oobQueue(),
    notificationTask(nullptr),
    parentTask(nullptr),
    responseStartTime(0),
    cachedVolumeLevel(0)
{
}

// ---------------------------------------------------------------------------
//  Setup / Init
// ---------------------------------------------------------------------------

void BT201TransactionTask::Setup()
{
    instance = this;
    Serial2.begin(115200, SERIAL_8N1, RX2, TX2);
}

void BT201TransactionTask::Init()
{
    parentTask = nullptr;
    currentOOBTransaction.state = OOBState::Inactive;
    transactionState = TransactionState::ProcessTransaction;
}

// ---------------------------------------------------------------------------
//  Commands
// ---------------------------------------------------------------------------

void BT201TransactionTask::SoundMode()
{
    InitiateTransaction("AT+CM04", "OK-QM");
}

void BT201TransactionTask::BluetoothMode()
{
    InitiateTransaction("AT+CM01", "OK-QM");
}

void BT201TransactionTask::AutoSwitchBluetooth(bool enable)
{
    InitiateTransaction(enable ? "AT+CK01" : "AT+CK00", "OK"); // May send QM prior to OK; it will be discarded.
}

void BT201TransactionTask::TFMode()
{
    InitiateTransaction("AT+CM03", "OK-QM");
}

void BT201TransactionTask::PlayTrack(const char* path, uint16_t trackNumber)
{
    // Clear any stale OOB state from a previous track.
    oobQueue.clear();
    currentOOBTransaction.state = OOBState::Inactive;

    char command[64];
    snprintf(command, sizeof(command), "AT+AJ/%s/%05d.MP3", path, trackNumber);
    InitiateTransaction(command, "OK-MVMDMO");  // OK, then MV, MD, MO at track end.
}

void BT201TransactionTask::SetVolume(uint8_t level)
{
    if (level > 30) level = 30;

    char command[32];
    snprintf(command, sizeof(command), "AT+CA%02d", level);
    InitiateTransaction(command, "OK");
    cachedVolumeLevel = level;
}

uint8_t BT201TransactionTask::GetVolume()
{
    return cachedVolumeLevel;
}

// ---------------------------------------------------------------------------
//  Playback control
// ---------------------------------------------------------------------------

bool BT201TransactionTask::TrackIsPlaying()
{
    return transactionState == TransactionState::WaitForExtendedResponse;
}

void BT201TransactionTask::Stop()
{
    if (TrackIsPlaying())
    {
        transactionQueue.clear();
        oobQueue.clear();
        EnqueueStopOOB();
    }
}

void BT201TransactionTask::TogglePause()
{
    if (TrackIsPlaying())
        EnqueueOOB("AT+CB");
}

void BT201TransactionTask::FastForward(uint8_t count)
{
    if (TrackIsPlaying())
        EnqueueOOB("AT+AA04", count);
}

void BT201TransactionTask::Rewind(uint8_t count)
{
    if (TrackIsPlaying())
        EnqueueOOB("AT+AA05", count);
}

void BT201TransactionTask::Shutdown(Task* task)
{
    this->parentTask = task;
    Stop();
}

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

void BT201TransactionTask::SetNotificationTask(Task* task)
{
    this->notificationTask = task;
}

void BT201TransactionTask::InitiateTransaction(const char* command, const char* responseTemplate)
{
    BT201Transaction t;
    t.Setup(command, responseTemplate);
    if (!transactionQueue.write(t))
        ERROR_println("[BT201] Transaction queue full — command dropped");

    taskScheduler->StartTask(Name());
}

void BT201TransactionTask::EnqueueOOB(const char* cmd, uint8_t count)
{
    BT201OOBTransaction o;
    o.Setup(cmd, count);
    o.state = OOBState::Active;
    if (!oobQueue.write(o))
        ERROR_println("[BT201] OOB queue full — command dropped");
}

void BT201TransactionTask::EnqueueStopOOB()
{
    BT201OOBTransaction o;
    o.Setup("AT+AA00");
    o.state = OOBState::StopActive;
    if (!oobQueue.write(o))
        ERROR_println("[BT201] OOB queue full — stop command dropped");
}

void BT201TransactionTask::SendNextOOB()
{
    if (!oobQueue.read(currentOOBTransaction))
        return;
    Serial2.println(currentOOBTransaction.GetCommand());
    DEBUG_print("[BT201] OOB sent: ");
    DEBUG_println(currentOOBTransaction.GetCommand());
    responseStartTime = millis();
}

bool BT201TransactionTask::ResponseTimedOut() const
{
    return millis() - responseStartTime > RESPONSE_TIMEOUT_MS;
}

void BT201TransactionTask::FlushSerial()
{
    while (Serial2.available())
    {
        char c = Serial2.read();
        DEBUG_print(c);
    }
}

// ---------------------------------------------------------------------------
//  FSM — tick responsibilities only
//
//  Response handling has moved to OnSyncResponse(), called directly
//  by BT201ReceiverTask when a sync line arrives. Execute() is
//  responsible only for sending commands, dispatching OOB, and
//  guarding against timeouts.
// ---------------------------------------------------------------------------

void BT201TransactionTask::Execute()
{
    switch (transactionState)
    {
        case TransactionState::ProcessTransaction:
        {
            if (transactionQueue.available())
            {
                transactionQueue.read(currentTransaction);
                DEBUG_print("[BT201] Sent: ");
                DEBUG_println(currentTransaction.GetCommand());
                Serial2.println(currentTransaction.GetCommand());
                responseStartTime = millis();
                transactionState  = TransactionState::WaitForResponse;
            }
            else
            {
                if (parentTask)
                    parentTask->ShutdownComplete(this);
                taskScheduler->EndTask(Name());
            }
        }
        break;

        case TransactionState::WaitForResponse:
        {
            if (ResponseTimedOut())
            {
                ERROR_println("[BT201] Timeout waiting for initial response");
                transactionState = TransactionState::ProcessTransaction;
            }
        }
        break;

        case TransactionState::WaitForExtendedResponse:
        {
            if (!currentOOBTransaction.IsActive() && oobQueue.available() > 0)
                SendNextOOB();

            if (currentOOBTransaction.IsActive() && ResponseTimedOut())
            {
                ERROR_println("[BT201] OOB timeout");
                currentOOBTransaction.state = OOBState::Inactive;
            }
        }
        break;
    }
}

// ---------------------------------------------------------------------------
//  Sync response dispatch — called directly by BT201ReceiverTask.
// ---------------------------------------------------------------------------

void BT201TransactionTask::OnSyncResponse(const CharacterBuffer& line)
{
    switch (transactionState)
    {
        case TransactionState::WaitForResponse:
            HandleResponse(line);
            break;

        case TransactionState::WaitForExtendedResponse:
            HandleExtendedResponse(line);
            break;

        default:
            ERROR_print("[BT201] Protocol violation — unexpected response: ");
            ERROR_println(line.ToString());
            break;
    }
}

// ---------------------------------------------------------------------------
//  HandleResponse — WaitForResponse handler
// ---------------------------------------------------------------------------

void BT201TransactionTask::HandleResponse(const CharacterBuffer& line)
{
    if (line.StartsWithToken("ER"))
    {
        ERROR_print("[BT201] Command error: ");
        ERROR_println(line.ToString());
        transactionState = TransactionState::ProcessTransaction;
        return;
    }

    ResponseStatus r = currentTransaction.ConsumeLine(line);

    if (r == ResponseStatus::AwaitsExtendedResponse)
    {
        DEBUG_print("[BT201] Initial OK: ");
        DEBUG_println(line.ToString());
        responseStartTime = millis();
        transactionState  = TransactionState::WaitForExtendedResponse;
    }
    else if (r == ResponseStatus::ResponseComplete)
    {
        DEBUG_print("[BT201] Response complete: ");
        DEBUG_println(line.ToString());
        transactionState = TransactionState::ProcessTransaction;
    }
    else
    {
        DEBUG_print("[BT201] Discarding: ");
        DEBUG_println(line.ToString());
    }
}

// ---------------------------------------------------------------------------
//  HandleExtendedResponse — WaitForExtendedResponse handler
//
//  Priority:
//    1. ER           → hard error; reset and notify parent.
//    2. OOB active   → route OK to OOB completion / repeat / stop.
//    3. Template     → match MV/MD/MO track-end sequence.
//    4. No match     → discard.
// ---------------------------------------------------------------------------

void BT201TransactionTask::HandleExtendedResponse(const CharacterBuffer& line)
{
    responseStartTime = millis();

    if (line.StartsWithToken("ER"))
    {
        ERROR_print("[BT201] Track error: ");
        ERROR_println(line.ToString());
        oobQueue.clear();
        currentOOBTransaction.state = OOBState::Inactive;
        transactionState = TransactionState::ProcessTransaction;
        if (parentTask)
            parentTask->ShutdownComplete(this);
        return;
    }

    if (currentOOBTransaction.ConsumeLine(line) == ResponseStatus::ResponseComplete)
    {
        DEBUG_print("[BT201] OOB ack: ");
        DEBUG_println(line.ToString());

        if (--currentOOBTransaction.repeatCount > 0)
        {
            Serial2.println(currentOOBTransaction.GetCommand());
            DEBUG_print("[BT201] OOB repeat: ");
            DEBUG_println(currentOOBTransaction.GetCommand());
            responseStartTime = millis();
        }
        else if (currentOOBTransaction.IsStopActive())
        {
            currentOOBTransaction.state = OOBState::Inactive;
            transactionState = TransactionState::ProcessTransaction;
            if (parentTask)
                parentTask->ShutdownComplete(this);
        }
        else
        {
            currentOOBTransaction.state = OOBState::Inactive;
        }
        return;
    }

    ResponseStatus r = currentTransaction.ConsumeLine(line);

    switch (r)
    {
        case ResponseStatus::SegmentComplete:
            DEBUG_print("[BT201] Track line: ");
            DEBUG_println(line.ToString());
            break;

        case ResponseStatus::ResponseComplete:
            DEBUG_print("[BT201] Track complete: ");
            DEBUG_println(line.ToString());
            transactionState = TransactionState::ProcessTransaction;
            break;

        case ResponseStatus::LineMismatch:
            DEBUG_print("[BT201] Discarding: ");
            DEBUG_println(line.ToString());
            break;

        default:
            break;
    }
}