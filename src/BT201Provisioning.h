#ifndef BT201PROVISIONING_H
#define BT201PROVISIONING_H

#include <Arduino.h>

// BT201Provisioning.h
//
// A primarily development-only utility class for working with the BT201 Bluetooth audio module.
//
// Two primary use cases:
//
// 1. Initial chip setup: The BT201 stores several settings in non-volatile memory that
//    must be configured once on a new or factory-reset chip. SetChipModes() sends the
//    desired AT commands.  This should be performed following a factory reset.
//    A power cycle should occur following the reset.  Another power cycle should occur
//    following setting the chip modes.
//
// 2. AT command exploration: The BT201 manual is not always explicit about which reply
//    strings a given command produces or under what conditions. This class provides a
//    convenient way to send arbitrary AT commands and observe the raw responses via
//    the debug serial port before committing them to production code.
//
// Usage: Wire Serial2 to the BT201 UART, call ConfigureUSBSerial() once, then call
// whichever provisioning or test methods are needed. Monitor output on Serial.

class BT201Provisioning
{
public:
    // Initializes Serial2 for communication with the BT201.
    static void ConfigureUSBSerial(unsigned int RXPin, unsigned int TXPin);

    // Factory reset followed by flash settings updates.
    void FactoryResetAndProvision(const char* btAudioName);

    // Resets the chip to factory defaults. Whether a power cycle is required
    // before subsequent settings will take effect is not yet confirmed.
    void RestoreFactoryDefaults();
 
    // --- Non-volatile settings ---
    // The following commands write to flash and persist across power cycles.
    // A power cycle may be required after each for the setting to take effect.
    void SetChipModes(const char* btAudioName);

    // Sets the Bluetooth audio device name visible to pairing clients.
    void SetBTAudioName(const char* name);

    // Automatically enter BT mode when powering on.
    void PowerOnEntersBTMode();

    // Allow BT to run in the background.
    void EnableBTBackground();

    // On startup, wait for an explicit mode instruction rather than auto-selecting.
    void WaitForModeInstructionOn();

    // Suppress welcome tones and voice prompts.
    void MuteAudioPrompts();

    // Emit verbose serial output during power up.
    void ChattyPowerUp();

    // Emit minimal serial output during power up (only QM+09).
    void QuietPowerUp();

    // Enable unsolicited return data so track completion events are reported.
    void AutoReturnDataOn();

    // Disable unsolicited return data. Only use if polling for track completion manually.
    void AutoReturnDataOff();

    // Play a single track and stop rather than looping.
    void SinglePlay();

    // Sets volume level (0-30).
    void SetVolume(uint8_t level);

    // Queries the current volume level from the chip and logs it to Serial.
    // Returns the volume level parsed from the QA+xx response, or -1 on failure.
    int QueryVolume();

    // Returns true if the chip is provisioned with the expected BT audio name.
    // Can be used during startup to determine whether SetChipModes() needs to be run.
    bool IsProvisioned(const char* btAudioName);

    // --- Serial I/O ---

    // Reads and logs all complete lines from the BT201 until the line is idle
    // for at least timeoutMs. Resets on every received byte so partial lines
    // are never lost. Call after every SendCommand().
    void FlushSerial();

    // Send an arbitrary command string and consume the response.
    void SendCommand(const char* command);

private:
    unsigned long lastByteTime = 0;
    const unsigned int timeoutMs = 100;
};

#endif // BT201PROVISIONING_H