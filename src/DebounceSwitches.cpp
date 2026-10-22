#include "DebounceSwitches.h"

DebounceSwitches::DebounceSwitches()
    : debounceLimit(5)
{
    Reset();
}

DebounceSwitches::DebounceSwitches(uint8_t limit)
    : debounceLimit(limit)
{
    Reset();
}

void DebounceSwitches::Reset()
{
    debounceCount = 0;
    lastSample = 0;
    stableValue = 0;
    pendingChange = false;
    //firstUpdate = true;   // Prevent false trigger at startup
}

bool DebounceSwitches::Update(uint16_t input)
{
    // Raw input changed
    if (input != lastSample)
    {
        // If we were stabilizing and didn't reach the limit → glitch
        if (pendingChange && debounceCount < debounceLimit)
        {
            Serial.print("*Glitch* : ");
            Serial.println(input, BIN);
        }

        // Begin stabilizing new value
        lastSample = input;
        debounceCount = 0;
        pendingChange = true;
        return false;
    }

    // Raw input is the same as last sample
    if (pendingChange && debounceCount < debounceLimit)
    {
        debounceCount++;

        // Reached stability threshold
        if (debounceCount == debounceLimit)
        {
            stableValue = input;
            pendingChange = false;

            // Ignore the first stabilized value at startup
            /*if (firstUpdate)
            {
                firstUpdate = false;
                return false;
            }*/

            return true;  // Real debounced change
        }
    }

    return false;  // No change
}

void DebounceSwitches::SetLimit(uint8_t limit)
{
    debounceLimit = limit;
    Reset();
}
