#ifndef IBT201SYNCRESPONSECLIENT_H
#define IBT201SYNCRESPONSECLIENT_H

#include "CharacterBuffer.h"

/*
 * IBT201SyncResponseClient
 *
 * Implemented by any class that wishes to receive synchronous
 * responses from BT201ReceiverTask via direct push.
 *
 * Synchronous responses are lines that belong to an active
 * command/response transaction — OK, MV, MD, MO, QM, ER.
 * Everything else is dispatched asynchronously to pub/sub subscribers.
 */

class IBT201SyncResponseClient
{
public:
    virtual void OnSyncResponse(const CharacterBuffer& line) = 0;
    virtual ~IBT201SyncResponseClient() = default;
};

#endif // IBT201SYNCRESPONSECLIENT_H
