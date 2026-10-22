#ifndef BT201_CIRCULAR_BUFFER_H
#define BT201_CIRCULAR_BUFFER_H

/*
 * BT201CircularBuffer.h
 *
 * A private circular buffer implementation for use by
 * BT201TransactionTask only. Wrapped in the BT201 namespace
 * to avoid conflicts with any other CircularBuffer library
 * the sketch may use.
 *
 * Statically allocated — buffer size is a compile-time
 * template parameter, so no heap allocation is needed.
 *
 * Derived from CircularBuffer by Majenko Technologies (2014).
 * Modified to use safe read/write semantics (bool return,
 * output-parameter read) eliminating the need for a dummy
 * constructor on stored types.
 */

namespace BT201 {

template <class T, uint32_t Size>
class CircularBuffer
{
public:
    CircularBuffer()
      : _head(0)
      , _tail(0)
    {
    }

    // Read the front item into 'item'. Returns false if empty.
    bool read(T& item)
    {
        if (_head == _tail)
            return false;
        item = _data[_tail];
        _tail = (_tail + 1) % Size;
        return true;
    }

    // Write an item. Returns false if full (item is dropped).
    bool write(const T& item)
    {
        uint32_t newhead = (_head + 1) % Size;
        if (newhead == _tail)
            return false; // Buffer full
        _data[_head] = item;
        _head = newhead;
        return true;
    }

    int available() const
    {
        if (_head >= _tail)
            return _head - _tail;
        return Size - (_tail - _head);
    }

    void clear()
    {
        _head = _tail = 0;
    }

    T getEntry(uint32_t p) const
    {
        return _data[p % Size];
    }

    uint32_t getHead() const { return _head; }
    uint32_t getTail() const { return _tail; }

private:
    T        _data[Size];
    uint32_t _head;
    uint32_t _tail;
};

} // namespace BT201

#endif // BT201_CIRCULAR_BUFFER_H