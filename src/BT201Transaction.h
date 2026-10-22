#ifndef BT201TRANSACTION_H
#define BT201TRANSACTION_H

#include <Arduino.h>
#include "CharacterBuffer.h"

/*
 * BT201Transaction
 *
 * Represents a single AT command transaction with the BT201
 * module, pairing a command string with a response template
 * that describes the expected reply.
 *
 * RESPONSE TEMPLATE FORMAT
 *   Token pairs of two characters describe expected response lines.
 *   A '-' marks the boundary between the initial response phase
 *   and the extended response phase. '**' is a wildcard token —
 *   matches any line.
 *
 *   Examples:
 *     "OK"         — one initial line "OK", done.
 *     "OK-MVMDMO"  — one initial OK line, then three extended lines
 *                    MV, MD, MO (natural track end).
 *
 * USAGE
 *   Call Setup() to load a command and its template.
 *   Feed each complete received line to ConsumeLine().
 *   The returned ResponseStatus drives the FSM:
 *
 *     LineMismatch           — line did not match; discard and keep waiting.
 *     SegmentComplete        — line matched; more lines expected in this phase.
 *     AwaitsExtendedResponse — initial phase complete; enter extended phase.
 *     ResponseComplete       — all template lines consumed; transaction done.
 */

enum ResponseStatus
{
    LineMismatch,
    SegmentComplete,
    AwaitsExtendedResponse,
    ResponseComplete
};

class BT201Transaction
{
public:
    BT201Transaction() : templateIndex(0) { command[0] = '\0'; responseTemplate[0] = '\0'; }

    void Setup(const char* cmd, const char* tmpl)
    {
        strncpy(command, cmd,  sizeof(command) - 1);
        command[sizeof(command) - 1] = '\0';
        strncpy(responseTemplate, tmpl, sizeof(responseTemplate) - 1);
        responseTemplate[sizeof(responseTemplate) - 1] = '\0';
        templateIndex = 0;
    }

    const char* GetCommand() const { return command; }

    // Feed a complete received line. Returns the current match status.
    // Does NOT advance templateIndex on a mismatch — caller discards
    // the line and calls again with the next one.
    //
    // Template tokens are two characters wide. '-' is a single-char
    // phase delimiter and is skipped automatically.
    ResponseStatus ConsumeLine(const CharacterBuffer& line)
    {
        // Skip phase delimiter if at one
        if (responseTemplate[templateIndex] == '-')
            ++templateIndex;

        const char* expected = &responseTemplate[templateIndex];
        const char* actual   = line.ToString();

        // Wildcard '**' matches any line
        bool matched = (expected[0] == '*' && expected[1] == '*') ||
                       (expected[0] == actual[0] && expected[1] == actual[1]);

        if (!matched)
            return ResponseStatus::LineMismatch;

        // Matched — advance past this two-char token
        templateIndex += 2;

        if (responseTemplate[templateIndex] == '\0')
            return ResponseStatus::ResponseComplete;

        if (responseTemplate[templateIndex] == '-')
            return ResponseStatus::AwaitsExtendedResponse;

        return ResponseStatus::SegmentComplete;
    }

    bool IsExtended() const
    {
        return strchr(responseTemplate, '-') != nullptr;
    }

private:
    char command[64];
    char responseTemplate[24];  // "OK-MVMDMO" = 9 chars + null
    uint8_t  templateIndex;
};

/*
 * BT201OOBTransaction
 *
 * Represents a single out-of-band command (Stop, FastForward,
 * Rewind, TogglePause) sent directly to the BT201 while a
 * track is playing. These bypass the main transaction queue
 * and are managed by a dedicated OOB queue.
 *
 * OOB STATE
 *   Inactive   — slot is idle, not in flight.
 *   Active     — command in flight; OK ack clears to Inactive
 *                and resumes WaitForExtendedResponse.
 *   StopActive — command in flight; OK ack exits extended phase
 *                entirely and transitions to ProcessTransaction.
 */

enum class OOBState
{
    Inactive,
    Active,
    StopActive
};

class BT201OOBTransaction
{
public:

    void Setup(const char* cmd, uint8_t count = 1)
    {
        strncpy(command, cmd, sizeof(command) - 1);
        command[sizeof(command) - 1] = '\0';
        repeatCount = count;
        state       = OOBState::Inactive;
    }

    const char* GetCommand() const { return command; }

    bool IsActive()     const { return state != OOBState::Inactive; }
    bool IsStopActive() const { return state == OOBState::StopActive; }

    // Feed a received line. Returns ResponseComplete if this is the expected
    // OK acknowledgment for this OOB command, LineMismatch otherwise.
    ResponseStatus ConsumeLine(const CharacterBuffer& line) const
    {
        if (IsActive() && line.StartsWithToken("OK"))
            return ResponseStatus::ResponseComplete;
        return ResponseStatus::LineMismatch;
    }

    OOBState state; // TODO - consider making private
    uint8_t  repeatCount;

private:
    char     command[16];
};

#endif // BT201TRANSACTION_H