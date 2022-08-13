#ifndef TERMINAL_CURSOR
#define TERMINAL_CURSOR

#if defined(_MSC_VER)
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#else
#include <cstdio>
#endif

namespace terminal {
namespace cursor {

#if defined(_MSC_VER)

    static inline void show(bool show) 
    {
        if (auto hStdout = GetStdHandle(STD_OUTPUT_HANDLE))
        {
            CONSOLE_CURSOR_INFO cursorInfo;
            GetConsoleCursorInfo(hStdout, &cursorInfo);

            cursorInfo.bVisible = show;
            SetConsoleCursorInfo(hStdout, &cursorInfo);
        }
    }

    static inline void erase_line() 
    {
        if (auto hStdout = GetStdHandle(STD_OUTPUT_HANDLE))
        {
            CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
            GetConsoleScreenBufferInfo(hStdout, &csbiInfo);

            COORD cursor;
            cursor.X = 0;
            cursor.Y = csbiInfo.dwCursorPosition.Y;

            DWORD count = 0;
            FillConsoleOutputCharacterA(hStdout, ' ', csbiInfo.dwSize.X, cursor, &count);
            FillConsoleOutputAttribute(hStdout, csbiInfo.wAttributes, csbiInfo.dwSize.X, cursor, &count);
            SetConsoleCursorPosition(hStdout, cursor);
        }
    }

    static inline void get(int& x, int& y)
    {
        if (auto hStdout = GetStdHandle(STD_OUTPUT_HANDLE))
        {
            CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
            GetConsoleScreenBufferInfo(hStdout, &csbiInfo);

            x = csbiInfo.dwCursorPosition.X;
            y = csbiInfo.dwCursorPosition.Y;
        }
    }

    static inline void set(int x, int y)
    {
        if (auto hStdout = GetStdHandle(STD_OUTPUT_HANDLE))
        {
            COORD cursor;
            cursor.X = x;
            cursor.Y = y;
            SetConsoleCursorPosition(hStdout, cursor);
        }
    }

    static inline void move(int dx, int dy) 
    {
        if (auto hStdout = GetStdHandle(STD_OUTPUT_HANDLE))
        {
            CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
            GetConsoleScreenBufferInfo(hStdout, &csbiInfo);

            COORD cursor;
            cursor.X = csbiInfo.dwCursorPosition.X + dx;
            cursor.Y = csbiInfo.dwCursorPosition.Y + dy;
            SetConsoleCursorPosition(hStdout, cursor);
        }
    }

    static inline void move_up(int lines) { move(0, -lines); }
    static inline void move_down(int lines) { move(0, lines); }
    static inline void move_right(int cols) { move(cols, 0); }
    static inline void move_left(int cols) { move(-cols, 0); }

#else

    static inline void show(bool const show) 
    {
        std::fputs(show ? "\033[?25h" : "\033[?25l", stdout);
    }

    static inline void erase_line() 
    {
        std::fputs("\r\033[K", stdout);
    }

    static inline void move_up(int lines) { std::cout << "\033[" << lines << "A"; }
    static inline void move_down(int lines) { std::cout << "\033[" << lines << "B"; }
    static inline void move_right(int cols) { std::cout << "\033[" << cols << "C"; }
    static inline void move_left(int cols) { std::cout << "\033[" << cols << "D"; }

#endif

} // namespace cursor
}

#endif