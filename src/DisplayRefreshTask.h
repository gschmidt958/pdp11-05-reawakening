#ifndef DISPLAYREFRESH_H
#define DISPLAYREFRESH_H

#include <Task.h>
#include <SPI.h>

#include "Registers.h"
#include "PowerUp.h"
#include "FlashTask.h"

#define USE_SPI

/*
 * DisplayRefreshTask
 *
 * Drives a chain of 74HCT595 shift registers that control
 * the 16-bit LED display panel. Monitors DisplayRegister
 * each tick and pushes an update only when the value has
 * changed.
 *
 * HARDWARE
 *   Data is clocked out MSB-first across two chained shift
 *   registers (high byte first, then low byte). The latch
 *   pin commits the shifted data to the outputs. Pin
 *   assignments target the ESP32 VSPI bus:
 *     CLK  - pin 18 (VSPI_CLK)
 *     DATA - pin 23 (VSPI_MOSI)
 *     LATCH - pin 5 (VSPI_CS)
 *
 * SPI MODE
 *   When USE_SPI is defined, the transfer uses the hardware
 *   SPI peripheral for speed. Undefine USE_SPI to fall back
 *   to a software shiftOut() implementation, which is useful
 *   for debugging or hardware bring-up.
 *
 * SETTLE TIME
 *   A configurable stabilization pulse (settleTime, loaded
 *   from flash) is inserted before and after each latch
 *   transition to allow the shift register outputs to settle.
 */

class DisplayRefreshTask : public Task
{
  // 74HCT595 shift register pin assignments (VSPI bus)
  static const int ClockPin = 18;  // VSPI_CLK
  static const int DataPin = 23;   // VSPI_MOSI
  static const int LatchPin = 5;   // VSPI_CS

public:
  DisplayRefreshTask(const char* name, unsigned int period = 0)
    : Task(name, period)
  {
  }

  void Setup() override
  {
    #ifdef USE_SPI
    SPI.begin();
    //SPI.beginTransaction(SPISettings(24000000, MSBFIRST, SPI_MODE0));
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    #endif // USE_SPI

    #ifndef USE_SPI
    pinMode(ClockPin, OUTPUT);
    pinMode(DataPin, OUTPUT);
    #endif // USE_SPI

    pinMode(LatchPin, OUTPUT);
    settleTime = FlashSettings::instance->Get("settleTime");

    DisplayRegister = 0;
    DisplayRefresh(); // Preload the register with 0.
    PowerUp::Setup(); // Initialize the panel.
    PowerUp::Enable(); // Enable the panel.
    delay(100);       // Time for the panel's clock and shift registers to settle in.
  }

  void Init() override
  {
    DisplayRegister = 0;
  }

  void Execute() override
  {
    static unsigned int previousDisplayRegister;

    if (DisplayRegister != previousDisplayRegister)
    {
      previousDisplayRegister = DisplayRegister;
//      Serial.println(DisplayRegister, BIN);
      DisplayRefresh();
    }
  }

private:
  static void DisplayRefresh()
  {
    volatile unsigned int displayRegister = DisplayRegister; // *** thread safe atomic assignment ***
    digitalWrite(LatchPin, LOW);        // Enable shifting.
    delayMicroseconds(settleTime);      // Stabilization pulse
    #ifdef USE_SPI
      // Fast shift register load using SPI
      SPI.transfer(displayRegister >> 8);
      SPI.transfer(displayRegister);
    #else
      // "Conventional" way to load the shift register
      shiftOut(DataPin, ClockPin, MSBFIRST, displayRegister >> 8);
      shiftOut(DataPin, ClockPin, MSBFIRST, displayRegister);
    #endif // USE_SPI
    digitalWrite(LatchPin, HIGH);       // Done shifting, write new data to the latch outputs.
    delayMicroseconds(settleTime);      // Stabilization pulse
  }

  static int settleTime;
};

int DisplayRefreshTask::settleTime = 0;

extern volatile unsigned int DisplayRegister;

#endif // DISPLAYREFRESH_H