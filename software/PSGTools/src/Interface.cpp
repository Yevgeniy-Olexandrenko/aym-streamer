#include <iomanip>
#include <sstream>
#include <algorithm>
#include <functional>
#include "Interface.h"

namespace Interface
{
	using namespace terminal;

    void Init()
    {
        // disable console window resize
        HWND consoleWindow = GetConsoleWindow();
        SetWindowLong(consoleWindow, GWL_STYLE, GetWindowLong(consoleWindow, GWL_STYLE) & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);

        // disable edit mode in console
        DWORD mode;
        HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
        GetConsoleMode(handle, &mode);
        SetConsoleMode(handle, mode & ~ENABLE_QUICK_EDIT_MODE);

        // set console buffer and window size
        SetConsoleTitle(L"Test!");
        KeepSize();
    }

    bool KeepSize()
    {
        SHORT x = k_consoleWidth;
        SHORT y = k_consoleHeight;

        HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (handle == INVALID_HANDLE_VALUE) return false;

        // If either dimension is greater than the largest console window we can have,
        // there is no point in attempting the change.
        COORD largestSize = GetLargestConsoleWindowSize(handle);
        if (x > largestSize.X) return false;
        if (y > largestSize.Y) return false;
        

        CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
        if (!GetConsoleScreenBufferInfo(handle, &bufferInfo)) return false;

        SMALL_RECT& winInfo = bufferInfo.srWindow;
        COORD windowSize = 
        { 
            winInfo.Right - winInfo.Left + 1,
            winInfo.Bottom - winInfo.Top + 1
        };

        if (windowSize.X > x || windowSize.Y > y)
        {
            // window size needs to be adjusted before the buffer size can be reduced.
            SMALL_RECT info =
            {
                0,
                0,
                x < windowSize.X ? x - 1 : windowSize.X - 1,
                y < windowSize.Y ? y - 1 : windowSize.Y - 1
            };

            if (!SetConsoleWindowInfo(handle, TRUE, &info)) return false;
        }

        COORD size = { x, y };
        if (!SetConsoleScreenBufferSize(handle, size)) return false;

        SMALL_RECT info = { 0, 0, x - 1, y - 1 };
        if (!SetConsoleWindowInfo(handle, TRUE, &info)) return false;

        return true;
    }

    // -------------------------------------------------------------------------

    short m_keyOldState[256] = { 0 };
    short m_keyNewState[256] = { 0 };
    KeyState m_keys[256];

    KeyState GetKey(int nKeyID)
    {
        return m_keys[nKeyID]; 
    }

    void HandleKeyboardInput() 
    {
        if (GetConsoleWindow() == GetForegroundWindow())
        {
            // Handle Keyboard Input
            for (int i = 0; i < 256; i++)
            {
                m_keyNewState[i] = GetAsyncKeyState(i);

                m_keys[i].pressed = false;
                m_keys[i].released = false;

                if (m_keyNewState[i] != m_keyOldState[i])
                {
                    if (m_keyNewState[i] & 0x8000)
                    {
                        m_keys[i].pressed = !m_keys[i].held;
                        m_keys[i].held = true;
                    }
                    else
                    {
                        m_keys[i].released = true;
                        m_keys[i].held = false;
                    }
                }

                m_keyOldState[i] = m_keyNewState[i];
            }
        }
    }

    // -------------------------------------------------------------------------

    void trim(std::string& str)
    {
        str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), str.end());
    };

    void printNibble(uint8_t nibble)
    {
        nibble &= 0x0F;
        std::cout << char((nibble >= 0x0A ? 'A' - 0x0A : '0') + nibble);
    };

    void printByteHex(uint8_t byte)
    {
        printNibble(byte >> 4);
        printNibble(byte);
    }

    void printWordHex(uint16_t word)
    {
        printNibble(uint8_t(word >> 12));
        printNibble(uint8_t(word >> 8));
        printNibble(uint8_t(word >> 4));
        printNibble(uint8_t(word));
    }

    ////////////////////////////////////////////////////////////////////////////

    void PrintInputFile(const Module& module, int index, int total)
    {
        cursor::show(false);
        std::string numberStr = std::to_string(index + 1);
        std::string totalStr = std::to_string(total);

        size_t strLen = numberStr.length() + 1 + totalStr.length() + module.file.nameExt().length();
        size_t delLen = std::max(int(terminal_width() - 2 - strLen - 7), 2);

        std::cout << ' ' << color::bright_cyan;
        std::cout << std::string(delLen, '-') << '[';
        std::cout << ' ' << color::bright_yellow << numberStr;
        std::cout << color::bright_cyan << '/';
        std::cout << color::bright_yellow << totalStr;
        std::cout << ' ' << color::bright_white << module.file.name();
        std::cout << color::bright_magenta << '.';
        std::cout << color::bright_grey << module.file.ext();
        std::cout << ' ' << color::bright_cyan << "]--";
        std::cout << color::reset << std::endl;
    }

    ////////////////////////////////////////////////////////////////////////////

    void printPropertyLabel(const std::string& label)
    {
        std::cout << ' ' << color::bright_yellow << label;
        std::cout << color::bright_grey << std::string(9 - label.length(), '.');
        std::cout << color::bright_magenta << ": ";
    };

    void printModuleProperty(const std::string& label, const Module& module, Module::Property property)
    {
        std::string str = module.property(property);
        trim(str);

        if (!str.empty())
        {
            printPropertyLabel(label);
            if (property == Module::Property::Title)
                std::cout << color::bright_green;
            else
                std::cout << color::bright_white;
            std::cout << str << color::reset << std::endl;
        }
    };

    void printOutputProperty(const std::string& label, const Output& output)
    {
        std::string str = output.name();
        trim(str);

        if (!str.empty())
        {
            printPropertyLabel(label);
            std::cout << color::bright_cyan;
            std::cout << str << color::reset << std::endl;
        }
    };

    void PrintModuleInfo(const Module& module, const Output& output)
    {
        cursor::show(false);
        printModuleProperty("Title", module, Module::Property::Title);
        printModuleProperty("Artist", module, Module::Property::Artist);
        printModuleProperty("Type", module, Module::Property::Type);
        printModuleProperty("Chip", module, Module::Property::Chip);
        printModuleProperty("Frames", module, Module::Property::Frames);
        printModuleProperty("Duration", module, Module::Property::Duration);
        printOutputProperty("Output", output);
    }

    ////////////////////////////////////////////////////////////////////////////

    void printRegistersHeader()
    {
        std::cout << color::bright_cyan   << '[';
        std::cout << color::cyan  << 'R';
        std::cout << color::white << '7';
        std::cout << color::bright_cyan   << '|';
        std::cout << color::cyan  << 'R';
        std::cout << color::white << '1';
        std::cout << color::cyan  << 'R';
        std::cout << color::white << '0';
        std::cout << color::cyan << " R";
        std::cout << color::white << '8';
        std::cout << color::bright_cyan   << '|';
        std::cout << color::cyan  << 'R';
        std::cout << color::white << '3';
        std::cout << color::cyan  << 'R';
        std::cout << color::white << '2';
        std::cout << color::cyan << " R";
        std::cout << color::white << '9';
        std::cout << color::bright_cyan   << '|';
        std::cout << color::cyan  << 'R';
        std::cout << color::white << '5';
        std::cout << color::cyan  << 'R';
        std::cout << color::white << '4';
        std::cout << color::cyan << " R";
        std::cout << color::white << 'A';
        std::cout << color::bright_cyan   << '|';
        std::cout << color::cyan  << 'R';
        std::cout << color::white << 'C';
        std::cout << color::cyan  << 'R';
        std::cout << color::white << 'B';
        std::cout << color::cyan << " R";
        std::cout << color::white << 'D';
        std::cout << color::bright_cyan   << '|';
        std::cout << color::cyan  << 'R';
        std::cout << color::white << '6';
        std::cout << color::bright_cyan   << ']';
    }

    void printRegisterValue(int chipIndex, const Frame& frame, int reg, bool highlight)
    {
        if (frame.changed(chipIndex, reg))
        {
            if (highlight)
                std::cout << color::bright_white;
            else
                std::cout << color::bright_green;
            printByteHex(frame.data(chipIndex, reg));
        }
        else
        {
            std::cout << color::bright_grey << "..";
        }
    }

    void printRegistersValues(int chipIndex, const Frame& frame, bool highlight)
    {
        std::cout << color::bright_cyan << '[';
        printRegisterValue(chipIndex, frame, Mixer_Flags, highlight);
        std::cout << color::bright_cyan << '|';
        printRegisterValue(chipIndex, frame, TonA_PeriodH, highlight);
        printRegisterValue(chipIndex, frame, TonA_PeriodL, highlight);
        std::cout << ' ';
        printRegisterValue(chipIndex, frame, VolA_EnvFlg, highlight);
        std::cout << color::bright_cyan << '|';
        printRegisterValue(chipIndex, frame, TonB_PeriodH, highlight);
        printRegisterValue(chipIndex, frame, TonB_PeriodL, highlight);
        std::cout << ' ';
        printRegisterValue(chipIndex, frame, VolB_EnvFlg, highlight);
        std::cout << color::bright_cyan << '|';
        printRegisterValue(chipIndex, frame, TonC_PeriodH, highlight);
        printRegisterValue(chipIndex, frame, TonC_PeriodL, highlight);
        std::cout << ' ';
        printRegisterValue(chipIndex, frame, VolC_EnvFlg, highlight);
        std::cout << color::bright_cyan << '|';
        printRegisterValue(chipIndex, frame, Env_PeriodH, highlight);
        printRegisterValue(chipIndex, frame, Env_PeriodL, highlight);
        std::cout << ' ';
        printRegisterValue(chipIndex, frame, Env_Shape, highlight);
        std::cout << color::bright_cyan << '|';
        printRegisterValue(chipIndex, frame, Noise_Period, highlight);
        std::cout << color::bright_cyan << ']';
    }

    void PrintModuleFrames(const Module& module, FrameId frameId, size_t height)
    {
        size_t range1 = (height - 2) / 2;
        size_t range2 = (height - 2) - range1;
        bool isTS = (module.chip.count() == Chip::Count::TurboSound);
        cursor::show(false);

        // print header
        std::cout << color::cyan << " FRAME ";
        printRegistersHeader();
        if (isTS) printRegistersHeader();
        std::cout << color::reset << std::endl;

        // print frames
        Frame fakeFrame;
        for (int i = int(frameId - range1); i <= int(frameId + range2); ++i)
        {
            bool highlight = (i == frameId);
            bool useFakeFrame = (i < 0 || i >= int(module.playback.framesCount()));
            const Frame& frame = useFakeFrame ? fakeFrame : module.playback.getFrame(i);

            // print frame number
            std::cout << ' ';
            if (highlight) 
                std::cout << color::on_magenta << color::bright_white;
            else
                std::cout << color::bright_grey;
            if (useFakeFrame)
                std::cout << std::string(5, '-');
            else
                std::cout << std::setfill('0') << std::setw(5) << i;
            std::cout << ' ';

            // print frame registers
            printRegistersValues(0, frame, highlight);
            if (isTS) printRegistersValues(1, frame, highlight);
            std::cout << color::reset << std::endl;
        }
        cursor::move_up(int(height));
    }

    ////////////////////////////////////////////////////////////////////////////

    void PrintBlankArea(int offset, size_t height)
    {
        cursor::show(false);
        if (offset < 0) cursor::move_up(-offset);
        else cursor::move_down(offset);

        for (size_t i = 0; i < height; ++i)
        {
            cursor::erase_line();
            cursor::move_down(1);
        }
        cursor::move_up(int(height));
    }

    ////////////////////////////////////////////////////////////////////////////

    void PrintPlaybackProgress()
    {
        cursor::show(false);
        std::string numberStr = "00:00:00";

        size_t strLen = numberStr.length();
        size_t delLen = std::max(int(terminal_width() - strLen - 8), 2);

        std::cout << ' ' << color::bright_cyan;
        std::cout << std::string(delLen, '-') << '[';
        std::cout << ' ' << color::bright_white << numberStr;
        std::cout << ' ' << color::bright_cyan << "]--";
        std::cout << color::reset << std::endl;
        cursor::move_up(1);
    }

    ////////////////////////////////////////////////////////////////////////////

    PrintBuffer streamPb(k_consoleWidth, 12);

    void printNibble2(uint8_t nibble)
    {
        nibble &= 0x0F;
        streamPb.draw(char((nibble >= 0x0A ? 'A' - 0x0A : '0') + nibble));
    };

    void printRegisterValue2(int chipIndex, const Frame& frame, int reg, bool highlight)
    {
        if (frame.changed(chipIndex, reg))
        {
            streamPb.color(highlight ? BG_DARK_MAGENTA | FG_WHITE : FG_GREEN);

            uint8_t data = frame.data(chipIndex, reg);
            printNibble2(data >> 4);
            printNibble2(data);
        }
        else
        {
            streamPb.color(highlight ? BG_DARK_MAGENTA | FG_DARK_GREY : FG_DARK_GREY);
            streamPb.draw("..");
        }
    }

    void printRegistersValues2(int chipIndex, const Frame& frame, bool highlight)
    {
        uint16_t color = highlight ? BG_DARK_MAGENTA | FG_CYAN : FG_CYAN;
        streamPb.color(color).draw('|');
        printRegisterValue2(chipIndex, frame, Mixer_Flags, highlight);
        streamPb.color(color).draw('|');
        printRegisterValue2(chipIndex, frame, TonA_PeriodH, highlight);
        printRegisterValue2(chipIndex, frame, TonA_PeriodL, highlight);
        streamPb.draw(' ');
        printRegisterValue2(chipIndex, frame, VolA_EnvFlg, highlight);
        streamPb.color(color).draw('|');
        printRegisterValue2(chipIndex, frame, TonB_PeriodH, highlight);
        printRegisterValue2(chipIndex, frame, TonB_PeriodL, highlight);
        streamPb.draw(' ');
        printRegisterValue2(chipIndex, frame, VolB_EnvFlg, highlight);
        streamPb.color(color).draw('|');
        printRegisterValue2(chipIndex, frame, TonC_PeriodH, highlight);
        printRegisterValue2(chipIndex, frame, TonC_PeriodL, highlight);
        streamPb.draw(' ');
        printRegisterValue2(chipIndex, frame, VolC_EnvFlg, highlight);
        streamPb.color(color).draw('|');
        printRegisterValue2(chipIndex, frame, Env_PeriodH, highlight);
        printRegisterValue2(chipIndex, frame, Env_PeriodL, highlight);
        streamPb.draw(' ');
        printRegisterValue2(chipIndex, frame, Env_Shape, highlight);
        streamPb.color(color).draw('|');
        printRegisterValue2(chipIndex, frame, Noise_Period, highlight);
        streamPb.color(color).draw('|');
    }

    void printRegistersHeader2()
    {
        std::string str = "|R7|R1R0 R8|R3R2 R9|R5R4 RA|RCRB RD|R6|";
        for (size_t i = 0; i < str.length(); ++i)
        {
            streamPb.color(FG_GREY);
            if (str[i] == '|') streamPb.color(FG_CYAN);
            if (str[i] == 'R') streamPb.color(FG_DARK_CYAN);
            streamPb.draw(str[i]);
        }
    }

    void PrintModuleFrames2(const Module& module, FrameId frameId, size_t height)
    {
        size_t range1 = (height - 2) / 2;
        size_t range2 = (height - 2) - range1;
        bool isTS = (module.chip.count() == Chip::Count::TurboSound);

        // prepare console for drawing
        streamPb.clear();
        cursor::show(false);
        for (int i = 0; i < streamPb.h; ++i) std::cout << std::endl;
        terminal::cursor::move_up(streamPb.h);

        // print header
        int offset = isTS ? 1 : 20;
        streamPb.position(offset, 0).color(FG_DARK_CYAN).draw("FRAME").move(1, 0);
        printRegistersHeader2();
        if (isTS)
        {
            streamPb.move(1, 0);
            printRegistersHeader2();
        }

        // print frames
        Frame fakeFrame; int y = 1;
        for (int i = int(frameId - range1); i <= int(frameId + range2); ++i, ++y)
        {
            bool highlight = (i == frameId);
            bool useFakeFrame = (i < 0 || i >= int(module.playback.framesCount()));
            const Frame& frame = useFakeFrame ? fakeFrame : module.playback.getFrame(i);

            // print frame number
            streamPb.position(offset, y);
            streamPb.color(highlight ? BG_DARK_MAGENTA | FG_WHITE : FG_DARK_GREY);
            if (useFakeFrame)
                streamPb.draw(std::string(5, '-'));
            else
            {
                std::stringstream ss;
                ss << std::setfill('0') << std::setw(5) << i;
                streamPb.draw(ss.str());
            }
            streamPb.draw(' ');

            // print frame registers
            printRegistersValues2(0, frame, highlight);
            if (isTS)
            {
                streamPb.draw(' ');
                printRegistersValues2(1, frame, highlight);
            }
        }
        //cursor::move_up(int(height));

        streamPb.render();
    }

    ////////////////////////////////////////////////////////////////////////////

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

            for (size_t i = 0; i < count; ++i)
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

            for (size_t i = 0; i < count; ++i)
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

}