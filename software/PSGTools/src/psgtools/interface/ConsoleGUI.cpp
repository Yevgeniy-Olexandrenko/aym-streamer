#include <iomanip>
#include <sstream>
#include <array>

#include "ConsoleGUI.h"
#include "PrintBuffer.h"

namespace gui
{
    using namespace terminal;

	PrintBuffer   m_framesBuffer(k_consoleWidth, 12);
    KeyboardInput m_keyboardInput;

    bool SetConsoleSize(SHORT x, SHORT y)
    {
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

    void Init(const std::wstring& title)
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
        SetConsoleTitle(title.c_str());
        SetConsoleSize(k_consoleWidth, k_consoleHeight);
    }

	void Update()
	{
        cursor::show(false);
		m_keyboardInput.Update();
	}

    void Clear(size_t height)
    {
        Clear(-int(height), height);
    }

    void Clear(int offset, size_t height)
	{
        if (offset < 0) cursor::move_up(-offset);
        else cursor::move_down(offset);

        for (size_t i = 0; i < height; ++i)
        {
            cursor::erase_line();
            cursor::move_down(1);
        }
        cursor::move_up(int(height));
	}

	const KeyState& GetKeyState(int key)
	{
		return m_keyboardInput.GetKeyState(key);
	}

    ////////////////////////////////////////////////////////////////////////////

    void trim(std::string& str)
    {
        str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int c) { return !std::isspace(c); }));
        str.erase(std::find_if(str.rbegin(), str.rend(), [](int c) { return !std::isspace(c); }).base(), str.end());
    };

    ////////////////////////////////////////////////////////////////////////////

	size_t PrintInputFile(const Stream& stream, int index, int amount, bool isFavorite)
	{
        cursor::show(false);
        std::string numberStr = std::to_string(index + 1);
        std::string totalStr = std::to_string(amount);

        size_t strLen = numberStr.length() + 1 + totalStr.length() + stream.file.filename().string().length();
        size_t delLen = std::max(int(terminal_width() - 2 - strLen - 7), 2);

        std::cout << ' ' << color::bright_blue << std::string(delLen, '-');
        std::cout << color::bright_cyan << "[ ";
        std::cout << color::bright_yellow << numberStr << color::bright_cyan << '/' << color::bright_yellow << totalStr;
        std::cout << ' ' << (isFavorite ? color::bright_magenta : color::bright_white) << stream.file.stem().string();
        std::cout << color::bright_grey << stream.file.extension().string();
        std::cout << color::bright_cyan << " ]";
        std::cout << color::bright_blue << "--";
        std::cout << color::reset << std::endl;
        return 1;
	}

    ////////////////////////////////////////////////////////////////////////////

    void printPropertyLabel(const std::string& label)
    {
        std::cout << ' ' << color::bright_yellow << label;
        std::cout << color::bright_grey << std::string(9 - label.length(), '.');
        std::cout << color::bright_magenta << ": ";
    };

    size_t printModuleProperty(const std::string& label, const Stream& stream, Stream::Property property)
    {
        std::string str = stream.ToString(property);
        trim(str);

        if (!str.empty())
        {
            printPropertyLabel(label);
            if (property == Stream::Property::Title)
                std::cout << color::bright_green;
            else
                std::cout << color::bright_white;
            std::cout << str << color::reset << std::endl;
            return 1;
        }
        return 0;
    };

    size_t printOutputProperty(const std::string& label, const Output& output)
    {
        std::string str = output.toString();
        trim(str);

        if (!str.empty())
        {
            printPropertyLabel(label);
            std::cout << color::bright_cyan;
            std::cout << str << color::reset << std::endl;
            return 1;
        }
        return 0;
    };

	size_t PrintStreamInfo(const Stream& stream, const Output& output)
	{
        size_t height = 0;
        height += printModuleProperty("Title", stream, Stream::Property::Title);
        height += printModuleProperty("Artist", stream, Stream::Property::Artist);
        height += printModuleProperty("Comment", stream, Stream::Property::Comment);
        height += printModuleProperty("Type", stream, Stream::Property::Type);
        height += printModuleProperty("Chip", stream, Stream::Property::Chip);
        height += printModuleProperty("Frames", stream, Stream::Property::Frames);
        height += printModuleProperty("Duration", stream, Stream::Property::Duration);
        height += printOutputProperty("Output", output);
        height += 1; std::cout << std::endl;
        return height;
	}

	////////////////////////////////////////////////////////////////////////////

    const std::string k_headerForExpMode = "|07|0100 0816 0C0B 0D|0302 0917 1110 14|0504 0A18 1312 15|191A06|";
    const std::string k_headerForComMode = "|R7|R1R0 R8|R3R2 R9|R5R4 RA|RCRB RD|R6|";

    class Coloring
    {
        SHORT m_color0;
        SHORT m_color1;
        bool m_playing;

    public:
        Coloring(bool playing, bool enabled, bool withEnvelope, bool withNoise, bool withAccent)
            : m_color0(FG_DARK_GREY)
            , m_color1(FG_DARK_GREY)
            , m_playing(playing)
        {
            if (playing)
            {
                if (enabled) m_color1 = FG_WHITE;
                m_color1 |= BG_DARK_MAGENTA;
            }
            else if (enabled)
            {
                m_color1 = (withEnvelope ? FG_YELLOW : (withNoise ? FG_CYAN : FG_GREEN));
                if (withAccent)
                {
                    m_color0 |= BG_DARK_BLUE;
                    m_color1 |= BG_DARK_BLUE;
                }
            }
        }

        bool IsPlaying() const { return m_playing; }
        SHORT GetColor(bool isChanged) const { return (isChanged ? m_color1 : m_color0); }
    };

    void printNibble(uint8_t nibble)
    {
        nibble &= 0x0F;
        m_framesBuffer.draw(char((nibble >= 0x0A ? 'A' - 0x0A : '0') + nibble));
    };

    void printRegisterValue(int chip, const Frame& frame, Register reg, const Coloring& coloring)
    {
        if (frame[chip].IsChanged(reg) || coloring.IsPlaying())
        {
            m_framesBuffer.color(coloring.GetColor(true));
            uint8_t data = frame[chip].GetData(reg);
            printNibble(data >> 4);
            printNibble(data);
        }
        else
        {
            m_framesBuffer.color(coloring.GetColor(false));
            m_framesBuffer.draw("..");
        }
    }

    void printRegistersValuesForCompatibleMode(int chip, const Frame& frame, bool playing, const Output::Enables& enables)
    {
        uint8_t mixer = frame[chip].Read(Mixer);
        uint8_t vol_a = frame[chip].Read(A_Volume);
        uint8_t vol_b = frame[chip].Read(B_Volume);
        uint8_t vol_c = frame[chip].Read(C_Volume);

        bool enableNA = !(mixer & 0b00001000);
        bool enableNB = !(mixer & 0b00010000);
        bool enableNC = !(mixer & 0b00100000);
        bool enableEA = (vol_a & 0x10);
        bool enableEB = (vol_b & 0x10);
        bool enableEC = (vol_c & 0x10);

        Coloring coloringM
        {
            playing,
            (enables[0] || enables[1] || enables[2] || enables[3]),
            false,
            enables[3] && (enableNA || enableNB || enableNC),
            false
        };
        Coloring coloringA
        {
            playing,
            enables[0],
            enables[4] && enableEA,
            enables[3] && enableNA,
            frame[chip].IsChangedPeriod(A_Period) && frame[chip].IsChanged(A_Volume)
        };
        Coloring coloringB
        { 
            playing,
            enables[1],
            enables[4] && enableEB,
            enables[3] && enableNB,
            frame[chip].IsChangedPeriod(B_Period) && frame[chip].IsChanged(B_Volume)
        };
        Coloring coloringC
        { 
            playing,
            enables[2],
            enables[4] && enableEC,
            enables[3] && enableNC,
            frame[chip].IsChangedPeriod(C_Period) && frame[chip].IsChanged(C_Volume)
        };
        Coloring coloringN
        { 
            playing,
            enables[3],
            false, 
            (enableNA || enableNB || enableNC),
            false 
        };
        Coloring coloringE
        { 
            playing,
            enables[4],
            (enableEA || enableEB || enableEC),
            false, 
            frame[chip].IsChangedPeriod(E_Period) && frame[chip].IsChanged(E_Shape)
        };

        uint16_t color = (playing ? BG_DARK_MAGENTA | FG_CYAN : FG_CYAN);
        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, Mixer, coloringM);

        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, A_Coarse, coloringA);
        printRegisterValue(chip, frame, A_Fine,   coloringA);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, A_Volume, coloringA);

        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, B_Coarse, coloringB);
        printRegisterValue(chip, frame, B_Fine,   coloringB);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, B_Volume, coloringB);

        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, C_Coarse, coloringC);
        printRegisterValue(chip, frame, C_Fine,   coloringC);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, C_Volume, coloringC);

        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, E_Coarse, coloringE);
        printRegisterValue(chip, frame, E_Fine,   coloringE);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, E_Shape,  coloringE);

        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, N_Period, coloringN);
        m_framesBuffer.color(color).draw('|');
    }

    void printRegistersValuesForExpandedMode(int chip, const Frame& frame, bool playing, const Output::Enables& enables)
    {
        uint8_t mixer = frame[chip].Read(Mixer);
        uint8_t vol_a = frame[chip].Read(A_Volume);
        uint8_t vol_b = frame[chip].Read(B_Volume);
        uint8_t vol_c = frame[chip].Read(C_Volume);

        bool enableNA = !(mixer & 0b00001000);
        bool enableNB = !(mixer & 0b00010000);
        bool enableNC = !(mixer & 0b00100000);
        bool enableEA = (vol_a & 0x20);
        bool enableEB = (vol_b & 0x20);
        bool enableEC = (vol_c & 0x20);

        Coloring coloringM
        {
            playing,
            (enables[0] || enables[1] || enables[2] || enables[3]),
            false,
            enables[3] && (enableNA || enableNB || enableNC),
            false
        };
        Coloring coloringA
        {
            playing,
            enables[0],
            enables[4] && enableEA,
            enables[3] && enableNA,
            frame[chip].IsChangedPeriod(A_Period) && frame[chip].IsChanged(A_Volume)
        };
        Coloring coloringB
        {
            playing,
            enables[1],
            enables[4] && enableEB,
            enables[3] && enableNB,
            frame[chip].IsChangedPeriod(B_Period) && frame[chip].IsChanged(B_Volume)
        };
        Coloring coloringC
        {
            playing,
            enables[2],
            enables[4] && enableEC,
            enables[3] && enableNC,
            frame[chip].IsChangedPeriod(C_Period) && frame[chip].IsChanged(C_Volume)
        };
        Coloring coloringN
        {
            playing,
            enables[3],
            false,
            (enableNA || enableNB || enableNC),
            false
        };
        Coloring coloringEA
        {
            playing,
            enables[4],
            enableEA,
            false,
            frame[chip].IsChangedPeriod(EA_Period) && frame[chip].IsChanged(EA_Shape)
        };
        Coloring coloringEB
        {
            playing,
            enables[4],
            enableEB,
            false,
            frame[chip].IsChangedPeriod(EB_Period) && frame[chip].IsChanged(EB_Shape)
        };
        Coloring coloringEC
        {
            playing,
            enables[4],
            enableEC,
            false,
            frame[chip].IsChangedPeriod(EC_Period) && frame[chip].IsChanged(EC_Shape)
        };
       
        uint16_t color = (playing ? BG_DARK_MAGENTA | FG_CYAN : FG_CYAN);
        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, Mixer, coloringM);

        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, A_Coarse,  coloringA);
        printRegisterValue(chip, frame, A_Fine,    coloringA);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, A_Volume,  coloringA);
        printRegisterValue(chip, frame, A_Duty,    coloringA);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, EA_Coarse, coloringEA);
        printRegisterValue(chip, frame, EA_Fine,   coloringEA);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, EA_Shape,  coloringEA);

        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, B_Coarse,  coloringB);
        printRegisterValue(chip, frame, B_Fine,    coloringB);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, B_Volume,  coloringB);
        printRegisterValue(chip, frame, B_Duty,    coloringB);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, EB_Coarse, coloringEB);
        printRegisterValue(chip, frame, EB_Fine,   coloringEB);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, EB_Shape,  coloringEB);

        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, C_Coarse,  coloringC);
        printRegisterValue(chip, frame, C_Fine,    coloringC);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, C_Volume,  coloringC);
        printRegisterValue(chip, frame, C_Duty,    coloringC);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, EC_Coarse, coloringEC);
        printRegisterValue(chip, frame, EC_Fine,   coloringEC);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, EC_Shape,  coloringEC);
        m_framesBuffer.color(color).draw('|');

        printRegisterValue(chip, frame, N_AndMask, coloringN);
        printRegisterValue(chip, frame, N_OrMask,  coloringN);
        printRegisterValue(chip, frame, N_Period,  coloringN);
        m_framesBuffer.color(color).draw('|');
    }

    void printRegistersValues(int chip, const Frame& frame, bool playing, const Output::Enables& enables)
    {
        if (frame[chip].IsExpMode())
            printRegistersValuesForExpandedMode(chip, frame, playing, enables);
        else
            printRegistersValuesForCompatibleMode(chip, frame, playing, enables);
    }

    void printRegistersHeaderForMode(const std::string& str)
    {
        bool regIdx = false;
        for (size_t i = 0; i < str.length(); ++i)
        {
            if (str[i] == '|' || str[i] == ' ')
                m_framesBuffer.color(FG_CYAN);
            else
            {
                if (regIdx)
                    m_framesBuffer.color(FG_GREY);
                else
                    m_framesBuffer.color(FG_DARK_CYAN);
                regIdx ^= true;
            }
            m_framesBuffer.draw(str[i]);
        }
    }

    void printRegistersHeader(bool isExpMode)
    {
        if (isExpMode)
            printRegistersHeaderForMode(k_headerForExpMode);
        else
            printRegistersHeaderForMode(k_headerForComMode);
    }

	size_t PrintStreamFrames(const Stream& stream, int frameId, const Output::Enables& enables)
	{
        size_t height = m_framesBuffer.h;
        size_t range1 = (height - 2) / 2;
        size_t range2 = (height - 2) - range1;
        bool isTwoChips = stream.IsSecondChipUsed();
        bool isExpMode  = stream.IsExpandedModeUsed();

        // prepare console for drawing
        m_framesBuffer.clear();
        cursor::show(false);
        for (int i = 0; i < m_framesBuffer.h; ++i) std::cout << std::endl;
        terminal::cursor::move_up(m_framesBuffer.h);

        // print header
        int regs_w = isExpMode ? k_headerForExpMode.length() : k_headerForComMode.length();
        int offset = ((m_framesBuffer.w - 2 - 6) - (isTwoChips ? 2 * regs_w + 1 : regs_w)) / 2;
        m_framesBuffer.position(offset, 0).color(FG_DARK_CYAN).draw("FRAME").move(1, 0);
        printRegistersHeader(isExpMode);
        if (isTwoChips)
        {
            m_framesBuffer.move(1, 0);
            printRegistersHeader(isExpMode);
        }

        // prepare fake frame
        Frame fakeFrame;
        fakeFrame[0].SetExpMode(isExpMode);
        fakeFrame[1].SetExpMode(isExpMode);
        fakeFrame.ResetChanges();

        // print frames
        for (int i = int(frameId - range1), y = 1; i <= int(frameId + range2); ++i, ++y)
        {
            bool highlight = (i == frameId);
            bool useFakeFrame = (i < 0 || i >= int(stream.play.framesCount()));
            const Frame& frame = useFakeFrame ? fakeFrame : stream.play.GetFrame(i);

            // print frame number
            m_framesBuffer.position(offset, y);
            m_framesBuffer.color(highlight ? BG_DARK_MAGENTA | FG_WHITE : FG_DARK_GREY);
            if (useFakeFrame)
                m_framesBuffer.draw(std::string(5, '-'));
            else
            {
                std::stringstream ss;
                ss << std::setfill('0') << std::setw(5) << i;
                m_framesBuffer.draw(ss.str());
            }
            m_framesBuffer.draw(' ');

            // print frame registers
            printRegistersValues(0, frame, highlight, enables);
            if (isTwoChips)
            {
                m_framesBuffer.draw(' ');
                printRegistersValues(1, frame, highlight, enables);
            }
        }
        m_framesBuffer.render();
        cursor::move_down(height);
        return height;
	}

	////////////////////////////////////////////////////////////////////////////

	size_t PrintPlaybackProgress(const Stream& stream, int frameId)
	{
        int hh = 0, mm = 0, ss = 0;
        size_t playbackFrames  = stream.play.framesCount();
        size_t remainingFrames = (playbackFrames - frameId);
        stream.play.ComputeDuration(remainingFrames, hh, mm, ss);

        static const std::string k_spinner = R"(_\|/)";
        char spin = k_spinner[frameId >> 2 & 3];

        auto range = size_t(terminal_width() - 1 - 2 - 2 - 10 - 2 - 2 - 1);
        auto size1 = size_t(float(frameId * range) / playbackFrames + 0.5f);
        auto size2 = size_t(range - size1);

        std::cout << ' ' << color::bright_blue << std::string(2 + size1, '-');
        std::cout << color::bright_cyan << "[ ";
        std::cout << color::bright_magenta << spin << ' ';
        std::cout << color::bright_white <<
            std::setfill('0') << std::setw(2) << hh << ':' <<
            std::setfill('0') << std::setw(2) << mm << ':' <<
            std::setfill('0') << std::setw(2) << ss;
        std::cout << color::bright_cyan << " ]";
        std::cout << color::bright_blue << std::string(size2 + 2, '-');
        std::cout << color::reset << std::endl;
		return 1;
	}
}
