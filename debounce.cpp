#include "debounce.h"

#include <Arduino.h>

Debounce::Debounce(int delay)
{
    this->_delay = delay;
    this->_last_key = 0;
    this->_last_time = 0;
}

bool Debounce::debounce(int64_t key)
{

    unsigned long now = millis();

    if (key != this->_last_key)
    {
        this->_last_key = key;
        this->_last_time = now;
        return false;
    }

    if (now - this->_last_time < this->_delay)
    {
        return true;
    }

    this->_last_time = now;

    return false;
}