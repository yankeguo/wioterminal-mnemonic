#ifndef QUEUE_H
#define QUEUE_H

#include "stdint.h"

#define QUEUE_EMPTY -1

class Queue
{
public:
    struct Item
    {
        int kind;
        uint64_t value;
    };

    Queue(int size);
    ~Queue();

    int size();

    void send(int kind);

    void send(int kind, uint64_t value);

    void send(int kind, uint64_t value, int merge);

    Item retrieve();

private:
    int _size;
    volatile int _head;
    volatile int _tail;
    Item *_items;
};

#endif