#include <iostream>
#include <chrono>

//#define _WIN32_WINNT 0x0500
#include <Windows.h>

#include <terminal/terminal.hpp>

#include "module/Filelist.h"
#include "module/Module.h"
#include "module/Player.h"

#include "decoders/DecodePT3.h"
#include "decoders/DecodePT2.h"
#include "decoders/DecodeVTX.h"
#include "decoders/DecodePSG.h"

#include "output/Streamer/Streamer.h"
#include "output/Emulator/Emulator.h"
#include "Interface.h"

const std::string k_filelist = "D:\\downloads\\MUSIC\\Tr_Songs++\\Magazines\\";
const std::string k_output = "output.txt";
const int k_comPortIndex = 14;

////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<Filelist> m_filelist;
std::shared_ptr<Module>   m_module;
std::shared_ptr<Output>   m_output;
std::shared_ptr<Player>   m_player;

static BOOL WINAPI console_ctrl_handler(DWORD dwCtrlType)
{
    if (m_player)
    {
        m_player->Stop();
    }
    return TRUE;
}

void PrintDelimiter()
{
    using namespace terminal;
    size_t term_w = terminal_width() - 2;
    std::cout << ' ' << color::bright_cyan;
    std::cout << std::string(term_w, '-');
    std::cout << color::reset << std::endl;
}

bool DecodeFileToModule(const std::string& filePath, Module& module)
{
    std::shared_ptr<Decoder> decoders[]{
        std::shared_ptr<Decoder>(new DecodePT3()),
        std::shared_ptr<Decoder>(new DecodePT2()),
        std::shared_ptr<Decoder>(new DecodeVTX()),
        std::shared_ptr<Decoder>(new DecodePSG()),
    };

    module.file.dirNameExt(filePath);
    for (std::shared_ptr<Decoder> decoder : decoders)
    {
        if (decoder->Open(module))
        {
            Frame frame;
            while (decoder->Decode(frame))
            {
                frame.FixValues();
                module.frames.add(frame);
                frame.SetUnchanged();
            }

            decoder->Close(module);
            return true;
        }
    }
    return false;
}

void SaveModuleDebugOutput(const Module& module)
{
    //std::ofstream file;
    //file.open(k_output);

    //if (file)
    //{
    //    auto delimiter = [&file]()
    //    {
    //        for (int i = 0; i < 48; ++i) file << '-';
    //        file << std::endl;
    //    };

    //    delimiter();
    //    file << module;
    //    delimiter();

    //    file << "frame  [ r7|r1r0 r8|r3r2 r9|r5r4 rA|rCrB rD|r6 ]" << std::endl;
    //    delimiter();

    //    for (FrameId i = 0; i < module.frames.count(); ++i)
    //    {
    //        if (i && module.loop.available() && i == module.loop.frameId()) delimiter();
    //        file << std::setfill('0') << std::setw(5) << i;
    //        file << "  [ " << module.frames.get(i) << " ]" << std::endl;
    //        
    //    }
    //    delimiter();
    //    file.close();
    //}
}

////////////////////////////////////////////////////////////////////////////////

bool SetConsoleWindowSize(SHORT x, SHORT y)
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

    if (h == INVALID_HANDLE_VALUE)
        return false;

    // If either dimension is greater than the largest console window we can have,
    // there is no point in attempting the change.
    {
        COORD largestSize = GetLargestConsoleWindowSize(h);
        if (x > largestSize.X)
            return false;
        if (y > largestSize.Y)
            return false;
    }


    CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
    if (!GetConsoleScreenBufferInfo(h, &bufferInfo))
        return false;

    SMALL_RECT& winInfo = bufferInfo.srWindow;
    COORD windowSize = { winInfo.Right - winInfo.Left + 1, winInfo.Bottom - winInfo.Top + 1 };

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

        if (!SetConsoleWindowInfo(h, TRUE, &info))
            return false;
    }

    COORD size = { x, y };
    if (!SetConsoleScreenBufferSize(h, size))
        return false;


    SMALL_RECT info = { 0, 0, x - 1, y - 1 };
    if (!SetConsoleWindowInfo(h, TRUE, &info))
        return false;

    return true;
}

int main()
{
    SetConsoleWindowSize(86, 32);


    HWND consoleWindow = GetConsoleWindow();
    SetWindowLong(consoleWindow, GWL_STYLE, GetWindowLong(consoleWindow, GWL_STYLE) & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);

   
    //HANDLE wHnd = GetStdHandle(STD_OUTPUT_HANDLE);
    //SMALL_RECT windowSize = { 0, 0, 86, 30 };
    //SetConsoleWindowInfo(wHnd, 1, &windowSize);
    //COORD bufferSize = { 86, 30 };
    //SetConsoleScreenBufferSize(wHnd, bufferSize);

    HANDLE hInput;
    DWORD prev_mode;
    hInput = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hInput, &prev_mode);
    SetConsoleMode(hInput, prev_mode & ~ENABLE_QUICK_EDIT_MODE);
    
    


   /* HWND console = GetConsoleWindow();
    RECT ConsoleRect;
    GetWindowRect(console, &ConsoleRect);

    MoveWindow(console, ConsoleRect.left, ConsoleRect.top, 600, 400, TRUE);*/

  //  HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);

  //  DWORD consoleMode =
  //      ENABLE_PROCESSED_OUTPUT |
  ////      ENABLE_WRAP_AT_EOL_OUTPUT |
  //      ENABLE_VIRTUAL_TERMINAL_PROCESSING;

  //  SetConsoleMode(stdOut, consoleMode);




    using namespace terminal;
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
    size_t w = terminal_width() - 1;

    PrintDelimiter();
    std::cout << ' ' << color::bright_red << "PSG Tools v1.0" << color::reset << std::endl;
    std::cout << ' ' << color::bright_red << "by Yevgeniy Olexandrenko" << color::reset << std::endl;
    PrintDelimiter();
    std::cout << std::endl;

#if 0
    m_output.reset(new Streamer(k_comPortIndex));
#else
    m_output.reset(new Emulator());
#endif
    m_player.reset(new Player(*m_output));
    m_filelist.reset(new Filelist("pt2|pt3|psg|vtx", k_filelist));

    if (!m_filelist->empty())
    {
        m_filelist->shuffle();
        bool goToPrev = false;

        std::string path;
        while (true)
        {
            bool isDone = goToPrev ? m_filelist->prev(path) : m_filelist->next(path);
            if (!isDone) break;

            m_module.reset(new Module());
            if (DecodeFileToModule(path, *m_module))
            {
                goToPrev = false; // if decoding OK, move to next by default
                Interface::PrintInputFile(*m_module, m_filelist->index(), m_filelist->count());
               
                if (m_player->Init(*m_module))
                {
                    Interface::PrintModuleInfo(*m_module, *m_output);
                    std::cout << std::endl;

                    m_player->Play();
                    auto start = std::chrono::steady_clock::now();

                    FrameId oldFrame = -1;
                    while (m_player->IsPlaying())
                    {
                        SetConsoleWindowSize(86, 32);
                        Interface::HandleKeyboardInput();

                        FrameId newFrame = m_player->GetFrameId();
                        if (newFrame != oldFrame)
                        {
                            oldFrame = newFrame;
                            Interface::PrintFrameStream(*m_module, newFrame, 12);
                            cursor::move_down(12);
                            Interface::PrintPlaybackFooter();
                            cursor::move_up(12);
                        }

                        if (Interface::GetKey(VK_UP).bPressed)
                        {
                            if (!m_player->IsPaused())
                            {
                                m_player->Stop();
                                goToPrev = true;
                                break;
                            }
                        }

                        if (Interface::GetKey(VK_DOWN).bPressed)
                        {
                            if (!m_player->IsPaused())
                            {
                                m_player->Stop();
                                goToPrev = false;
                                break;
                            }
                        }

                        if (Interface::GetKey(VK_RETURN).bPressed)
                        {
                            if (m_player->IsPaused()) 
                                m_player->Play();
                            else 
                                m_player->Stop();
                        }

                        Sleep(1);
                    }
                    Interface::PrintBlankArea(0, 13);
                    

                    //uint32_t duration = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
                    //        int ms = duration % 1000; duration /= 1000;
                    //        int ss = duration % 60;   duration /= 60;
                    //        int mm = duration % 60;   duration /= 60;
                    //        int hh = duration;
                    //
                    //        std::cout << std::endl;
                    //        std::cout << " Duration: " <<
                    //            std::setfill('0') << std::setw(2) << hh << ':' <<
                    //            std::setfill('0') << std::setw(2) << mm << ':' <<
                    //            std::setfill('0') << std::setw(2) << ss << '.' <<
                    //            std::setfill('0') << std::setw(3) << ms << std::endl;

                }
                else
                {
                    std::cout << "Could not init player with module" << std::endl;
                }
            }
            else
            {
                std::cout << "Could not decode file: " << path << std::endl;
            }
            path.clear();
        }
    }
    cursor::show(true);

//    indicators::ProgressBar bar{
//        indicators::option::BarWidth{50},
//        indicators::option::Start{"["},
//        indicators::option::Fill{"="},
//        indicators::option::Lead{"|"},
//        indicators::option::Remainder{":"},
//        indicators::option::End{"]"},
//    //    indicators::option::PostfixText{"Loading dependency 1/4"},
//        indicators::option::ForegroundColor{indicators::Color::yellow},
//        indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}}
//    };

    return 0;
}
