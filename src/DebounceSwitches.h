#ifndef DEBOUNCESWITCHES_H
#define DEBOUNCESWITCHES_H

#include <Arduino.h>

/*
 * DebounceSwitches
 *
 * Debounces a 16-bit switch register by requiring the input
 * to hold a stable value for a configurable number of
 * consecutive samples before accepting it as a real change.
 *
 * USAGE
 *   Call Update() each scan cycle with the raw switch input.
 *   Returns true only when a new stable value has been
 *   confirmed. Retrieve the accepted value with
 *   GetStableValue().
 *
 * GLITCH DETECTION
 *   If the input changes before the stability threshold is
 *   reached, the pending value is discarded and a glitch
 *   is logged to Serial. Stabilization restarts from the
 *   new input value.
 */

class DebounceSwitches
{
public:
  DebounceSwitches();
  DebounceSwitches(uint8_t limit);

  void Reset();
  bool Update(uint16_t input);
  void SetLimit(uint8_t limit);

  uint16_t GetStableValue() const { return stableValue; }

private:
  uint16_t lastSample;    // Most recent raw input
  uint16_t stableValue;   // Last accepted debounced value
  uint8_t debounceCount;  // How long input has been stable
  uint8_t debounceLimit;  // Required stable samples
  bool pendingChange;     // True when a new value is being evaluated
};

#endif // DEBOUNCESWITCHES_H