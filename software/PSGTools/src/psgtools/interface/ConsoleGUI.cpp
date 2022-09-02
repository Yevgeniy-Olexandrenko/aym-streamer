#include <iomanip>
#include <sstream>

#include "stream/Stream.h"
#include "output/Output.h"

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

    enum RegColorType { Highlight, Changed, WithNoise, WithEnvelope, Hidden, Unchanged };
    SHORT RegColors[] { BG_DARK_MAGENTA | FG_WHITE, FG_GREEN, FG_CYAN, FG_YELLOW, FG_RED, FG_DARK_GREY };

    void printNibble(uint8_t nibble)
    {
        nibble &= 0x0F;
        m_framesBuffer.draw(char((nibble >= 0x0A ? 'A' - 0x0A : '0') + nibble));
    };

    void printRegisterValue(int chip, const Frame& frame, int reg, RegColorType regColorType)
    {
        if (frame.IsChanged(chip, reg) || regColorType == Highlight)
        {
            m_framesBuffer.color(RegColors[regColorType]);
            uint8_t data = frame.data(chip, reg);
            printNibble(data >> 4);
            printNibble(data);
        }
        else
        {
            m_framesBuffer.color(RegColors[Unchanged]);
            m_framesBuffer.draw("..");
        }
    }

    void printRegistersValuesForCompatibleMode(int chip, const Frame& frame, bool highlight)
    {
        uint8_t mixer = frame.Read(chip, Mixer);
        uint8_t vol_a = frame.Read(chip, A_Volume);
        uint8_t vol_b = frame.Read(chip, B_Volume);
        uint8_t vol_c = frame.Read(chip, C_Volume);

        bool enableAT = !(mixer & 0b00000001);
        bool enableBT = !(mixer & 0b00000010);
        bool enableCT = !(mixer & 0b00000100);
        bool enableAN = !(mixer & 0b00001000);
        bool enableBN = !(mixer & 0b00010000);
        bool enableCN = !(mixer & 0b00100000);
        bool enableAE = (vol_a & 0x10);
        bool enableBE = (vol_b & 0x10);
        bool enableCE = (vol_c & 0x10);
        bool enableAM = ((enableAT || enableAN) && (enableAE || vol_a));
        bool enableBM = ((enableBT || enableBN) && (enableBE || vol_b));
        bool enableCM = ((enableCT || enableCN) && (enableCE || vol_c));

        bool enableM = (enableAM || enableBM || enableCM);
        bool enableA = (enableAT || enableAN || enableAE);
        bool enableB = (enableBT || enableBN || enableBE);
        bool enableC = (enableCT || enableCN || enableCE);
        bool enableE = (enableAE || enableBE || enableCE);
        bool enableN = (enableAN || enableBN || enableCN);

        RegColorType mixerColorType = (highlight ? Highlight : (enableM ? Changed : Hidden));
        RegColorType channelAColorType = (highlight ? Highlight : (enableA ? (enableAE ? WithEnvelope : (enableAN ? WithNoise : Changed)) : Hidden));
        RegColorType channelBColorType = (highlight ? Highlight : (enableB ? (enableBE ? WithEnvelope : (enableBN ? WithNoise : Changed)) : Hidden));
        RegColorType channelCColorType = (highlight ? Highlight : (enableC ? (enableCE ? WithEnvelope : (enableCN ? WithNoise : Changed)) : Hidden));
        RegColorType envelopeColorType = (highlight ? Highlight : (enableE ? Changed : Hidden));
        RegColorType noiseColorType = (highlight ? Highlight : (enableN ? Changed : Hidden));

        uint16_t color = (highlight ? BG_DARK_MAGENTA | FG_CYAN : FG_CYAN);
        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, Mixer, mixerColorType);

        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, A_Coarse, channelAColorType);
        printRegisterValue(chip, frame, A_Fine, channelAColorType);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, A_Volume, channelAColorType);

        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, B_Coarse, channelBColorType);
        printRegisterValue(chip, frame, B_Fine, channelBColorType);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, B_Volume, channelBColorType);

        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, C_Coarse, channelCColorType);
        printRegisterValue(chip, frame, C_Fine, channelCColorType);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, C_Volume, channelCColorType);

        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, E_Coarse, envelopeColorType);
        printRegisterValue(chip, frame, E_Fine, envelopeColorType);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, E_Shape, envelopeColorType);

        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, N_Period, noiseColorType);
        m_framesBuffer.color(color).draw('|');
    }

    void printRegistersValuesForExpandedMode(int chip, const Frame& frame, bool highlight)
    {
        uint8_t mixer = frame.Read(chip, Mixer);
        uint8_t vol_a = frame.Read(chip, A_Volume);
        uint8_t vol_b = frame.Read(chip, B_Volume);
        uint8_t vol_c = frame.Read(chip, C_Volume);

        bool enableAT = !(mixer & 0b00000001);
        bool enableBT = !(mixer & 0b00000010);
        bool enableCT = !(mixer & 0b00000100);
        bool enableAN = !(mixer & 0b00001000);
        bool enableBN = !(mixer & 0b00010000);
        bool enableCN = !(mixer & 0b00100000);
        bool enableAE = (vol_a & 0x20);
        bool enableBE = (vol_b & 0x20);
        bool enableCE = (vol_c & 0x20);
        bool enableAM = ((enableAT || enableAN) && (enableAE || vol_a));
        bool enableBM = ((enableBT || enableBN) && (enableBE || vol_b));
        bool enableCM = ((enableCT || enableCN) && (enableCE || vol_c));

        bool enableM = (enableAM || enableBM || enableCM);
        bool enableA = (enableAT || enableAN || enableAE);
        bool enableB = (enableBT || enableBN || enableBE);
        bool enableC = (enableCT || enableCN || enableCE);
        bool enableN = (enableAN || enableBN || enableCN);

        RegColorType mixerColorType = (highlight ? Highlight : (enableM ? Changed : Hidden));
        RegColorType channelAColorType = (highlight ? Highlight : (enableA ? (enableAE ? WithEnvelope : (enableAN ? WithNoise : Changed)) : Hidden));
        RegColorType channelBColorType = (highlight ? Highlight : (enableB ? (enableBE ? WithEnvelope : (enableBN ? WithNoise : Changed)) : Hidden));
        RegColorType channelCColorType = (highlight ? Highlight : (enableC ? (enableCE ? WithEnvelope : (enableCN ? WithNoise : Changed)) : Hidden));
        RegColorType envelopeAColorType = (highlight ? Highlight : (enableAE ? Changed : Hidden));
        RegColorType envelopeBColorType = (highlight ? Highlight : (enableBE ? Changed : Hidden));
        RegColorType envelopeCColorType = (highlight ? Highlight : (enableCE ? Changed : Hidden));
        RegColorType noiseColorType = (highlight ? Highlight : (enableN ? Changed : Hidden));

        uint16_t color = (highlight ? BG_DARK_MAGENTA | FG_CYAN : FG_CYAN);
        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, Mixer, mixerColorType);

        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, A_Coarse, channelAColorType);
        printRegisterValue(chip, frame, A_Fine, channelAColorType);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, A_Volume, channelAColorType);
        printRegisterValue(chip, frame, A_Duty, channelAColorType);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, EA_Coarse, envelopeAColorType);
        printRegisterValue(chip, frame, EA_Fine, envelopeAColorType);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, EA_Shape, envelopeAColorType);

        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, B_Coarse, channelBColorType);
        printRegisterValue(chip, frame, B_Fine, channelBColorType);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, B_Volume, channelBColorType);
        printRegisterValue(chip, frame, B_Duty, channelBColorType);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, EB_Coarse, envelopeBColorType);
        printRegisterValue(chip, frame, EB_Fine, envelopeBColorType);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, EB_Shape, envelopeBColorType);

        m_framesBuffer.color(color).draw('|');
        printRegisterValue(chip, frame, C_Coarse, channelCColorType);
        printRegisterValue(chip, frame, C_Fine, channelCColorType);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, C_Volume, channelCColorType);
        printRegisterValue(chip, frame, C_Duty, channelCColorType);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, EC_Coarse, envelopeCColorType);
        printRegisterValue(chip, frame, EC_Fine, envelopeCColorType);
        m_framesBuffer.draw(' ');
        printRegisterValue(chip, frame, EC_Shape, envelopeCColorType);
        m_framesBuffer.color(color).draw('|');

        printRegisterValue(chip, frame, N_AndMask, noiseColorType);
        printRegisterValue(chip, frame, N_OrMask, noiseColorType);
        printRegisterValue(chip, frame, N_Period, noiseColorType);       
        m_framesBuffer.color(color).draw('|');
    }

    void printRegistersValues(bool isExpMode, int chip, const Frame& frame, bool highlight)
    {
        if (frame.IsExpMode(chip))
            printRegistersValuesForExpandedMode(chip, frame, highlight);
        else
            printRegistersValuesForCompatibleMode(chip, frame, highlight);
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

	size_t PrintStreamFrames(const Stream& stream, int frameId)
	{
        size_t height = m_framesBuffer.h;
        size_t range1 = (height - 2) / 2;
        size_t range2 = (height - 2) - range1;
        bool isTS = (stream.chip.count() == Chip::Count::TwoChips);
        bool isExpMode = stream.IsExpModeUsed();

        // prepare console for drawing
        m_framesBuffer.clear();
        cursor::show(false);
        for (int i = 0; i < m_framesBuffer.h; ++i) std::cout << std::endl;
        terminal::cursor::move_up(m_framesBuffer.h);

        // print header
        int regs_w = isExpMode ? k_headerForExpMode.length() : k_headerForComMode.length();
        int offset = ((m_framesBuffer.w - 2 - 6) - (isTS ? 2 * regs_w + 1 : regs_w)) / 2;
        m_framesBuffer.position(offset, 0).color(FG_DARK_CYAN).draw("FRAME").move(1, 0);
        printRegistersHeader(isExpMode);
        if (isTS)
        {
            m_framesBuffer.move(1, 0);
            printRegistersHeader(isExpMode);
        }

        // prepare fake frame
        Frame fakeFrame;
        fakeFrame.SetExpMode(0, isExpMode);
        fakeFrame.SetExpMode(1, isExpMode);
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
            printRegistersValues(isExpMode, 0, frame, highlight);
            if (isTS)
            {
                m_framesBuffer.draw(' ');
                printRegistersValues(isExpMode, 1, frame, highlight);
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
