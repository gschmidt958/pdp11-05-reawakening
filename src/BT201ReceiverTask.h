#ifndef BT201RECEIVERTASK_H
#define BT201RECEIVERTASK_H

#include "Task.h"
#include "CharacterBuffer.h"
#include "IBT201SyncResponseClient.h"

/*
 * BT201ReceiverTask
 *
 * Owns the serial receive loop for the BT201 module.
 * Accumulates incoming characters into a CharacterBuffer and
 * classifies complete lines (\n terminated) as either:
 *
 *   SYNC (pushed directly to the registered IBT201SyncResponseClient):
 *     OK          — command acknowledgement.
 *     MV, MD, MO  — track-end sequence (sync, owned by PlayTrack).
 *     QM          — mode query response.
 *     ER          — error returns.
 *
 *   ASYNC (fired to pub/sub subscribers):
 *     Q*          — query responses / power-on status burst.
 *     T*          — Bluetooth status returns.
 *     $*          — BLE SPP pass-through commands.
 *     M* (others) — MU, MB, MC, M1, M2, MT, MK, MF, MP etc.
 *
 * Sync responses are pushed directly into the transaction task via
 * IBT201SyncResponseClient::OnSyncResponse().
 * The protocol guarantees a sync response only arrives when the
 * transaction task is explicitly waiting for one, so direct push
 * is always safe.
 *
 * Character timeout: if a partial line is being accumulated
 * and no new character arrives within CHAR_TIMEOUT_MS, the
 * partial line is discarded and the buffer is reset.
 *
 * \r is stripped by CharacterBuffer::AppendCharacter() —
 * lines delivered to the sync client and subscribers are clean strings.
 *
 * Subscribers receive the line that triggered the notification.
 * Unsubscribe() keys on the function pointer.
 */

class BT201ReceiverTask : public Task
{
public:
    static const uint8_t       MAX_SUBSCRIBERS = 4;
    static const unsigned long CHAR_TIMEOUT_MS = 50;

    static inline BT201ReceiverTask* instance = nullptr;

    BT201ReceiverTask(const char* name, unsigned int period);

    void Init()    override;
    void Execute() override;

    // Register the transaction task as the sync response recipient.
    // Pass nullptr to detach.
    void SetSyncClient(IBT201SyncResponseClient* client);

    // Async subscriber management.
    void Subscribe(void (*callback)(const CharacterBuffer&));
    void Unsubscribe(void (*callback)(const CharacterBuffer&));

private:
    CharacterBuffer           receiveBuffer;
    IBT201SyncResponseClient* syncClient;

    unsigned long lastCharTime;
    bool          receiving;

    struct Subscriber
    {
        void (*callback)(const CharacterBuffer&) = nullptr;
    };

    Subscriber subscribers[MAX_SUBSCRIBERS];
    uint8_t    subscriberCount;

    bool ClassifyLine(const CharacterBuffer& line);
    void FireSubscribers(const CharacterBuffer& line);
};

#endif // BT201RECEIVERTASK_H