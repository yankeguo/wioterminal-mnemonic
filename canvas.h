#ifndef CANVAS_H
#define CANVAS_H

class Canvas
{
public:
    Canvas(int w, int h, int initX, int initY);
    ~Canvas();

    int x();

    int y();

    void setup();

    void refresh();

    Canvas *reset();

    Canvas *resetX();

    Canvas *resetY();

    Canvas *add(int dx, int dy);

    Canvas *addX(int dx);

    Canvas *addY(int dx);

    Canvas *set(int x, int y);

    Canvas *setX(int x);

    Canvas *setY(int y);

    Canvas *next();

    Canvas *nextX();

    Canvas *nextY();

    Canvas *string(const char *str);

    Canvas *stringf(const char *str, ...);

    Canvas *progress(int value, int max);

    Canvas *cube(int fill);

private:
    int _x;
    int _y;
    int _w;
    int _h;
    int _initX;
    int _initY;
    int _nextX;
    int _nextY;

    char _buf[512];

    struct Internal;
    Internal *_internal;
};

#endif