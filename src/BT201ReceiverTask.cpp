#include <Arduino.h>
#include "BT201ReceiverTask.h"
#include "BT201Log.h"

BT201ReceiverTask::BT201ReceiverTask(const char* name, unsigned int period)
  : Task(name, period),
    receiveBuffer(),
    syncClient(nullptr),
    lastCharTime(0),
    receiving(false),
    subscriberCount(0)
{
    instance = this;
}

void BT201ReceiverTask::Init()
{
    receiveBuffer.Clear();
    lastCharTime = 0;
    receiving    = false;
}

void BT201ReceiverTask::SetSyncClient(IBT201SyncResponseClient* client)
{
    syncClient = client;
}

// ---------------------------------------------------------------------------
//  Classification
//
//  Returns true  → sync; push to IBT201SyncResponseClient.
//  Returns false → async; fire to pub/sub subscribers.
// ---------------------------------------------------------------------------

bool BT201ReceiverTask::ClassifyLine(const CharacterBuffer& line)
{
    return line.StartsWithToken("ER") ||
           line.StartsWithToken("OK") ||
           line.StartsWithToken("MV") ||
           line.StartsWithToken("MD") ||
           line.StartsWithToken("MO") ||
           line.StartsWithToken("QM");
}

void BT201ReceiverTask::Execute()
{
    // Character timeout — discard partial line if no char arrived in time.
    if (receiving && (millis() - lastCharTime > CHAR_TIMEOUT_MS))
    {
        ERROR_print("[RX] Char timeout, discarding: ");
        ERROR_println(receiveBuffer.ToString());
        receiveBuffer.Clear();
        receiving = false;
    }

    while (Serial2.available())
    {
        char c = Serial2.read();
        lastCharTime = millis();

        if (c == '\n')
        {
            if (!receiveBuffer.IsEmpty())
            {
                if (ClassifyLine(receiveBuffer))
                {
                    DEBUG_print("[RX] Sync: ");
                    DEBUG_println(receiveBuffer.ToString());
                    if (syncClient)
                        syncClient->OnSyncResponse(receiveBuffer);
                }
                else
                {
                    FireSubscribers(receiveBuffer);
                }
            }
            receiveBuffer.Clear();
            receiving = false;
        }
        else
        {
            receiveBuffer.AppendCharacter(c); // \r stripped inside AppendCharacter
            receiving = true;
        }
    }
}

void BT201ReceiverTask::Subscribe(void (*callback)(const CharacterBuffer&))
{
    if (subscriberCount < MAX_SUBSCRIBERS)
    {
        subscribers[subscriberCount].callback = callback;
        subscriberCount++;
    }
}

void BT201ReceiverTask::Unsubscribe(void (*callback)(const CharacterBuffer&))
{
    for (uint8_t i = 0; i < subscriberCount; i++)
    {
        if (subscribers[i].callback == callback)
        {
            for (uint8_t j = i; j < subscriberCount - 1; j++)
                subscribers[j] = subscribers[j + 1];
            subscriberCount--;
            return;
        }
    }
}

void BT201ReceiverTask::FireSubscribers(const CharacterBuffer& line)
{
    DEBUG_print("[RX] Async: ");
    DEBUG_println(line.ToString());
    for (uint8_t i = 0; i < subscriberCount; i++)
        subscribers[i].callback(line);
}