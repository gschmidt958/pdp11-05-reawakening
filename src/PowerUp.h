#ifndef POWERUP_H
#define POWERUP_H

/*
 * PowerUp
 *
 * Controls the PUP (Power Up) signal that enables the
 * front panel. A 1k pull-down resistor holds PUP low
 * during power-up, keeping the panel disabled while the
 * interface hardware stabilizes ahead of the
 * microcontroller. Once the microcontroller starts, it
 * asserts PUP high to take control of the display and
 * prevent spurious output during the startup lag.
 */

class PowerUp
{
  static const int PUP_Pin = 14;

public:
  static void Setup()
  {
    pinMode(PUP_Pin, OUTPUT);
    Enable();
  }

  static void Enable()
  {
    digitalWrite(PUP_Pin, HIGH);
  }

  static void Disable()
  {
    digitalWrite(PUP_Pin, LOW);
  }
};

#endif // POWERUP_H