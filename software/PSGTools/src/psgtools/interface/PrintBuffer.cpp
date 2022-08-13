#include "PrintBuffer.h"

PrintBuffer::PrintBuffer(int w, int h)
    : x(0), y(0)
    , w(w), h(h)
    , buffer(new CHAR_INFO[w * h])
    , col(FG_WHITE)
{
    clear();
}

PrintBuffer::~PrintBuffer()
{
    if (buffer)
    {
        delete[] buffer;
        buffer = nullptr;
    }
}

void PrintBuffer::clear()
{
    memset(buffer, 0, sizeof(CHAR_INFO) * w * h);
}

void PrintBuffer::render()
{
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(handle, &csbi);

    SHORT cx = csbi.dwCursorPosition.X;
    SHORT cy = csbi.dwCursorPosition.Y;

    SMALL_RECT rect = { cx, cy, cx + w - 1, cy + h - 1 };
    WriteConsoleOutput(handle, buffer, { w, h }, { 0,0 }, &rect);
}

PrintBuffer& PrintBuffer::position(int x, int y)
{
    this->x = x;
    this->y = y;
    return *this;
}

PrintBuffer& PrintBuffer::move(int dx, int dy)
{
    this->x += dx;
    this->y += dy;
    return *this;
}

PrintBuffer& PrintBuffer::color(SHORT color)
{
    this->col = color;
    return *this;
}

PrintBuffer& PrintBuffer::draw(std::wstring s)
{
    if (x < w && y < h)
    {
        int index = y * w;
        int count = std::min(int(s.size()), w - x);

        for (int i = 0; i < count; ++i)
        {
            buffer[index + x].Char.UnicodeChar = s[i];
            buffer[index + x].Attributes = col;
            x++;
        }
    }
    return *this;
}

PrintBuffer& PrintBuffer::draw(std::string s)
{
    if (x < w && y < h)
    {
        int index = y * w;
        int count = std::min(int(s.size()), w - x);

        for (int i = 0; i < count; ++i)
        {
            buffer[index + x].Char.AsciiChar = s[i];
            buffer[index + x].Attributes = col;
            x++;
        }
    }
    return *this;
}

PrintBuffer& PrintBuffer::draw(wchar_t c)
{
    if (x < w && y < h)
    {
        int index = y * w + x;
        buffer[index].Char.UnicodeChar = c;
        buffer[index].Attributes = col;
        x++;
    }
    return *this;
}

PrintBuffer& PrintBuffer::draw(char c)
{
    if (x < w && y < h)
    {
        int index = y * w + x;
        buffer[index].Char.AsciiChar = c;
        buffer[index].Attributes = col;
        x++;
    }
    return *this;
}
