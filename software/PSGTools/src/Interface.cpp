#include <iomanip>
#include <algorithm>
#include <functional>
#include "Interface.h"

namespace Interface
{
	using namespace terminal;

    short m_keyOldState[256] = { 0 };
    short m_keyNewState[256] = { 0 };
    sKeyState m_keys[256];

    sKeyState GetKey(int nKeyID)
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

                m_keys[i].bPressed = false;
                m_keys[i].bReleased = false;

                if (m_keyNewState[i] != m_keyOldState[i])
                {
                    if (m_keyNewState[i] & 0x8000)
                    {
                        m_keys[i].bPressed = !m_keys[i].bHeld;
                        m_keys[i].bHeld = true;
                    }
                    else
                    {
                        m_keys[i].bReleased = true;
                        m_keys[i].bHeld = false;
                    }
                }

                m_keyOldState[i] = m_keyNewState[i];
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////

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

    void PrintPlaybackFooter()
    {
        cursor::show(false);
        std::string numberStr = "00:00:00";

        size_t strLen = numberStr.length();
        size_t delLen = std::max(int(terminal_width() - 2 - strLen - 7), 2);

        std::cout << ' ' << color::bright_cyan;
        std::cout << std::string(delLen, '-') << '[';
        std::cout << ' ' << color::bright_white << numberStr;
        std::cout << ' ' << color::bright_cyan << "]--";
        std::cout << color::reset << std::endl;
        cursor::move_up(1);
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

    void PrintFrameStream(const Module& module, FrameId frameId, size_t height)
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
}