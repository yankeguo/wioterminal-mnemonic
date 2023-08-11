#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include "stdint.h"

class Debounce
{
public:
    Debounce(int delay);

    bool debounce(int64_t key);

private:
    int _delay;
    int64_t _last_key;
    uint64_t _last_time;
};

#endif