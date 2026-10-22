#include "Registers.h"

// Define shared "registers" for switch state and display state
volatile unsigned int SwitchRegister  = 0;
volatile byte         SenseSwitches   = 0;

volatile unsigned int DisplayRegister = 0;
volatile byte         DisplaySense    = 0;

// Sense switches are active-low: function returns true when the specified switch is pressed.
bool IsSenseSwitchActive(SenseSwitch senseSwitch)
{
  // Active low.
  return (SenseSwitches & senseSwitch) == 0;
}

// Check for any active sense switch.
bool IsAnySenseSwitchActive()
{
  return (SenseSwitches & ANY) != ANY;
}