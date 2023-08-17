#include "canvas.h"

#include <TFT_eSPI.h>
#include <SPI.h>

#define LINE_HEIGHT 20
#define GAP 2

struct Canvas::Internal
{
    TFT_eSPI *tft;
};

Canvas::Canvas(int screenWidth, int screenHeight, int marginX, int marginY)
{
    this->_internal = new Internal();
    this->_internal->tft = new TFT_eSPI();
    this->_x = 0;
    this->_y = 0;
    this->_w = screenWidth;
    this->_h = screenHeight;
    this->_initX = marginX;
    this->_initY = marginY;
    this->_nextX = 0;
    this->_nextY = 0;
}

Canvas::~Canvas()
{
    delete this->_internal->tft;
    delete this->_internal;
}

int Canvas::x()
{
    return this->_x;
}

int Canvas::y()
{
    return this->_y;
}

void Canvas::setup()
{
    this->_internal->tft->init();
    this->_internal->tft->setRotation(3);
    this->_internal->tft->fillScreen(TFT_BLACK);
}

void Canvas::refresh()
{
    this->_internal->tft->fillScreen(TFT_BLACK);
    this->_internal->tft->setTextColor(TFT_WHITE);
    this->reset();
}

Canvas *Canvas::reset()
{
    return this->set(this->_initX, this->_initY);
}

Canvas *Canvas::resetX()
{
    return this->setX(this->_initX);
}

Canvas *Canvas::resetY()
{
    return this->setY(this->_initY);
}

Canvas *Canvas::set(int x, int y)
{
    if (x < 0)
    {
        x = this->_w + x % this->_w;
    }
    if (y < 0)
    {
        y = this->_h + y % this->_h;
    }
    this->_x = x;
    this->_y = y;
    return this;
}

Canvas *Canvas::setX(int x)
{
    return this->set(x, this->_y);
}

Canvas *Canvas::setY(int y)
{
    return this->set(this->_x, y);
}

Canvas *Canvas::add(int dx, int dy)
{
    return this->set(this->_x + dx, this->_y + dy);
}

Canvas *Canvas::addX(int dx)
{
    return this->add(dx, 0);
}

Canvas *Canvas::addY(int dy)
{
    return this->add(0, dy);
}

Canvas *Canvas::string(const char *str)
{
    this->_internal->tft->drawString(str, this->_x, this->_y, 2);
    this->_nextX = this->_internal->tft->textWidth(str, 2);
    this->_nextY = LINE_HEIGHT;
    return this;
}

Canvas *Canvas::stringf(const char *str, ...)
{
    va_list args;
    va_start(args, str);
    vsnprintf(this->_buf, sizeof(this->_buf), str, args);
    va_end(args);
    return this->string(this->_buf);
}

Canvas *Canvas::next()
{
    return this->add(this->_nextX, this->_nextY);
}

Canvas *Canvas::nextX()
{
    return this->addX(this->_nextX);
}

Canvas *Canvas::nextY()
{
    return this->addY(this->_nextY);
}

Canvas *Canvas::progress(int value, int max)
{
    this->resetX();
    int w = this->_w - 2 * this->_initX - 2 * GAP;
    int h = LINE_HEIGHT - 2 * GAP;
    int x = this->_x + GAP;
    int y = this->_y + GAP;
    this->_internal->tft->drawRect(x, y, w, h, TFT_WHITE);
    this->_internal->tft->fillRect(x + 1, y + 1, (w - 2) * float(value) / float(max), h - 2, TFT_WHITE);
    this->_nextX = this->_w - 2 * this->_initX;
    this->_nextY = LINE_HEIGHT;
    return this;
}

Canvas *Canvas::cube(int opt)
{
    int fill = opt & 1;
    int underscore = opt & (1 << 1);
    int w = LINE_HEIGHT - 2 * GAP;
    int h = LINE_HEIGHT - 2 * GAP;
    int x = this->_x + GAP;
    int y = this->_y + GAP;
    this->_internal->tft->drawRect(x, y, w, h, TFT_WHITE);
    if (fill)
    {
        this->_internal->tft->drawLine(x + 1, y + 1, x + w - 2, y + h - 2, TFT_WHITE);
        this->_internal->tft->drawLine(x + 1, y + h - 2, x + w - 2, y + 1, TFT_WHITE);
    }
    if (underscore)
    {
        this->_internal->tft->drawRect(x + 1, y + h - 2 * GAP, w - 2, GAP * 3, TFT_WHITE);
    }
    this->_nextX = LINE_HEIGHT;
    this->_nextY = LINE_HEIGHT;
    return this;
}
