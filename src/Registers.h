#ifndef REGISTERS_H
#define REGISTERS_H

#include "Arduino.h"

// PDP-11/05 KY11-J Panel Sense Switch Bits
typedef enum : byte {
    START       = 0x04,  // Switch 2 - Start
    DEPOSIT     = 0x08,  // Switch 3 - Deposit
    LOAD        = 0x10,  // Switch 4 - Load
    EXAMINE     = 0x20,  // Switch 5 - Examine
    CONTINUE    = 0x40,  // Switch 6 - Continue
    HALT_ENABLE = 0x80,  // Switch 7 - Halt/Enable (not spring loaded)
    ANY = START | DEPOSIT | LOAD | EXAMINE | CONTINUE | HALT_ENABLE  // 0xFC
} SenseSwitch;

// Functions
bool IsSenseSwitchActive(SenseSwitch senseSwitch);
bool IsAnySenseSwitchActive();
// Shared "registers" used by tasks or potentially across cores.
// Defined as native integers for atomic access.
// Declared volatile to inhibit compiler optimizations and reflect hardware state.

extern volatile unsigned int SwitchRegister;
extern volatile byte         SenseSwitches;

extern volatile unsigned int DisplayRegister;
extern volatile byte         DisplaySense;

#endif // REGISTERS_H
