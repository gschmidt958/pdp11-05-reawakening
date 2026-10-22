#ifndef RUNLIGHT_H
#define RUNLIGHT_H

/*
 * RunLight
 *
 * Controls the front panel RUN indicator LED.
 */

class RunLight
{
  static const int runLightPin = 32;

public:
  static void Setup()
  {
    pinMode(runLightPin, OUTPUT);
    Off();
  }

  static void On()
  {
    digitalWrite(runLightPin, LOW);
  }

  static void Off()
  {
    digitalWrite(runLightPin, HIGH);
  }
};

#endif // RUNLIGHT_H