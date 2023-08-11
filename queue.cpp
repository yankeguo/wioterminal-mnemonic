#include "queue.h"

Queue::Queue(int size)
{
    this->_size = size;
    this->_head = 0;
    this->_tail = 0;
    this->_items = new Item[size];
}

Queue::~Queue()
{
    delete[] this->_items;
}

int Queue::size()
{
    return this->_size;
}

void Queue::send(int kind)
{
    this->send(kind, 0);
}

void Queue::send(int kind, uint64_t value)
{
    this->send(kind, value, 0);
}

void Queue::send(int kind, uint64_t value, int merge)
{
    if (merge)
    {
        for (int i = this->_head; i != this->_tail; i = (i + 1) % this->_size)
        {
            if (this->_items[i].kind == kind)
            {
                this->_items[i].value = value;
                return;
            }
        }
    }
    this->_items[this->_tail] = Item{kind, value};
    this->_tail = (this->_tail + 1) % this->_size;

    if (this->_tail == this->_head)
    {
        this->_head = (this->_head + 1) % this->_size;
    }
}

Queue::Item Queue::retrieve()
{
    if (this->_head == this->_tail)
    {
        return Item{QUEUE_EMPTY, 0};
    }
    Item item = this->_items[this->_head];
    this->_head = (this->_head + 1) % this->_size;
    return item;
}