#ifndef BLINKTASK_H
#define BLINKTASK_H

class BlinkTask : public Task
{
  static const int builtinLEDPin = 2;
  
  public:
    BlinkTask(const char* name, unsigned int period = 0)
      : Task(name, period)
      , toggle(true)
    {
    }

    void Init() override
    {
      toggle = true;
    }

    void Setup() override
    {
      pinMode(builtinLEDPin, OUTPUT);
    }

    void Cleanup() override
    {
      digitalWrite(builtinLEDPin, LOW);
     }

    void Execute() override
    {
      digitalWrite(builtinLEDPin, toggle);
      toggle = !toggle;
    }

  private:
    bool toggle;
};

#endif // BLINKTASK_H