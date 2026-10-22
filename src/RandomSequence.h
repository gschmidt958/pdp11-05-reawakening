#ifndef RANDOMSEQUENCE_H
#define RANDOMSEQUENCE_H

#include "esp_random.h"

/*
 * RandomSequence
 *
 * Generates a non-repeating cycle of integers 0..(length-1).
 * Each call to Next() picks one of the remaining values at
 * random, moves it to the used end of the internal pool,
 * and returns it. Once all values have been used the pool
 * resets automatically for a new cycle.
 *
 * Uses a fixed internal buffer of MAX_SIZE — no heap
 * allocation. Length is clamped to MAX_SIZE at construction
 * and on Reset().
 */

class RandomSequence
{
public:
  static constexpr size_t MAX_SIZE = 64;

  // Construct with desired length (clamped to MAX_SIZE). Length must be > 0.
  RandomSequence(size_t length)
    : sequenceLength(length <= MAX_SIZE ? length : MAX_SIZE)
    , remainingCount(sequenceLength)
  {
    for (unsigned int i = 0; i < sequenceLength; ++i)
      pool[i] = i;
  }

  // Reinitialize with a new length, resetting the cycle.
  void Reset(size_t newLength)
  {
    sequenceLength = newLength <= MAX_SIZE ? newLength : MAX_SIZE;
    remainingCount = sequenceLength;
    for (unsigned int i = 0; i < sequenceLength; ++i)
      pool[i] = i;
  }

  // Returns true when all values in the current cycle have been used.
  bool IsComplete() const
  {
    return remainingCount == 0;
  }

  // Returns the next random value in [0..sequenceLength-1] without
  // repeats until the cycle exhausts. Resets automatically when empty.
  int Next()
  {
    if (remainingCount == 0)
      remainingCount = sequenceLength;

    size_t idx = esp_random() % remainingCount;
    int result = pool[idx];

    pool[idx] = pool[remainingCount - 1];
    pool[remainingCount - 1] = result;

    --remainingCount;
    return result;
  }

private:
  size_t sequenceLength;
  size_t remainingCount;
  unsigned int pool[MAX_SIZE];
};

#endif // RANDOMSEQUENCE_H