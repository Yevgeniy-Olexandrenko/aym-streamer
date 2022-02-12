#include <iostream>
#include <iomanip>
#include <chrono>

#include <indicators/cursor_control.hpp>
#include <indicators/termcolor.hpp>
#include <indicators/progress_bar.hpp>

#include "module/Filelist.h"
#include "module/Module.h"
#include "module/Player.h"

#include "decoders/DecodeVTX.h"
#include "decoders/DecodePT3.h"
#include "decoders/DecodePSG.h"

#include "output/AYMStreamer/AYMStreamer.h"
#include "output/AY38910/AY38910.h"


const std::string k_folder = "D:\\Projects\\github\\aym-streamer\\chiptunes\\Power Blade\\";
const std::string k_file = "stealth_2_ts.pt3";
const std::string k_filelist = "playlist.m3u";
const std::string k_output = "output.txt";
const int k_comPortIndex = 4;

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

bool DecodeFileToModule(const std::string& filePath, Module& module)
{
    std::shared_ptr<Decoder> decoders[]{
        std::shared_ptr<Decoder>(new DecodeVTX()),
        std::shared_ptr<Decoder>(new DecodePT3()),
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
    std::ofstream file;
    file.open(k_output);

    if (file)
    {
        auto delimiter = [&file]()
        {
            for (int i = 0; i < 48; ++i) file << '-';
            file << std::endl;
        };

        delimiter();
        file << module;
        delimiter();

        file << "frame  [ r7|r1r0 r8|r3r2 r9|r5r4 rA|rCrB rD|r6 ]" << std::endl;
        delimiter();

        for (FrameId i = 0; i < module.frames.count(); ++i)
        {
            if (i && module.loop.available() && i == module.loop.frameId()) delimiter();
            file << std::setfill('0') << std::setw(5) << i;
            file << "  [ " << module.frames.get(i) << " ]" << std::endl;
            
        }
        delimiter();
        file.close();
    }
}

////////////////////////////////////////////////////////////////////////////////

int main()
{
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
    indicators::show_console_cursor(false);
    size_t w = indicators::terminal_width() - 1;

    m_filelist.reset(new Filelist("pt3|psg|vtx", k_filelist));
    if (!m_filelist->empty())
    {
        m_filelist->shuffle();

        std::string path;
        while (!m_filelist->next(path))
        {
            m_module.reset(new Module());
            if (DecodeFileToModule(path, *m_module))
            {
                std::cout << termcolor::bright_cyan << std::string(w, '-') << termcolor::reset << std::endl;
                std::cout << termcolor::bright_red << "PSG Tools v1.0" << termcolor::reset << std::endl;
                std::cout << termcolor::bright_red << "by Yevgeniy Olexandrenko" << termcolor::reset << std::endl;
                std::cout << termcolor::bright_cyan << std::string(w, '-') << termcolor::reset << std::endl;
                std::cout << *m_module;
                std::cout << termcolor::bright_cyan << std::string(w, '-') << termcolor::reset << std::endl;

                m_output.reset(new AY38910(*m_module));
                m_player.reset(new Player(*m_output));

                if (m_player->Init(*m_module))
                {
                    FrameId oldFrame = -1;

                    m_player->Play();
                    while (m_player->IsPlaying())
                    {
                        FrameId newFrame = m_player->GetFrameId();
                        if (newFrame != oldFrame)
                        {
                            oldFrame = newFrame;
                            std::cout << '\r' << "Frame: " << newFrame << "     " << '\r';
                        }
                        Sleep(1);
                    }
                    m_player.reset();
                    m_output.reset();
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

//
//
//    Module module;
//    if (!DecodeFileToModule(k_folder + k_file, module))
//    {
//        return - 1;
//    }
////    SaveModuleDebugOutput(module);
//
//    ////////////////////////////////////////////////////////////////////////////
//
//    indicators::show_console_cursor(false);
//    size_t w = indicators::terminal_width() - 1;
//
//#if 0
//    AYMStreamer output(module, k_comPortIndex);
//#else
//    AY38910 output(module);
//#endif
//    Player player(output);
//
//    std::cout << termcolor::bright_cyan << std::string(w, '-') << termcolor::reset << std::endl;
//    std::cout << termcolor::bright_red << "PSG Tools v1.0" << termcolor::reset << std::endl;
//    std::cout << termcolor::bright_red << "by Yevgeniy Olexandrenko" << termcolor::reset << std::endl;
//    std::cout << termcolor::bright_cyan << std::string(w, '-') << termcolor::reset << std::endl;
//    std::cout << module;
//    std::cout << termcolor::bright_cyan << std::string(w, '-') << termcolor::reset << std::endl;
//
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
//
//    g_player = &player;
//    if (player.Init(module))
//    {
//        FrameId oldFrame = -1;
//        auto start = std::chrono::steady_clock::now();
//        player.Play();
//
//        while (player.IsPlaying())
//        {
////#if 0
////            
////            bool left  = (GetAsyncKeyState(VK_LEFT ) & 0x1) != 0;
////            bool right = (GetAsyncKeyState(VK_RIGHT) & 0x1) != 0;
////            bool up    = (GetAsyncKeyState(VK_UP   ) & 0x1) != 0;
////            bool down  = (GetAsyncKeyState(VK_DOWN ) & 0x1) != 0;
////#else
////            bool left  = (GetKeyState(VK_LEFT  ) & 0x80) != 0;
////            bool right = (GetKeyState(VK_RIGHT ) & 0x80) != 0;
////            bool up    = (GetKeyState(VK_UP    ) & 0x80) != 0;
////            bool down  = (GetKeyState(VK_DOWN  ) & 0x80) != 0;
////            bool enter = (GetAsyncKeyState(VK_RETURN) & 0x1) != 0;
////#endif
////
////            if (right)
////            {
////                player.Play(+10);
////            }
////            else if (left)
////            {
////                player.Play(-10);
////            }
////            else
////            {
////                player.Play(+1);
////            }
//
//            //if (enter)
//            //{
//            //    if (player.IsPaused())
//            //        player.Play();
//            //    else 
//            //        player.Stop();
//            //}
//
//            FrameId newFrame = player.GetFrameId();
//            if (newFrame != oldFrame)
//            {
//                oldFrame = newFrame;
//                bar.set_progress(newFrame * 100 / module.playback.framesCount());
//                std::cout << '\r' << "Frame: " << newFrame << "     " << '\r';
//            }
//            Sleep(200);
//        }
//
//        uint32_t duration = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
//        int ms = duration % 1000; duration /= 1000;
//        int ss = duration % 60;   duration /= 60;
//        int mm = duration % 60;   duration /= 60;
//        int hh = duration;
//
//        std::cout << std::endl;
//        std::cout << "Duration: " <<
//            std::setfill('0') << std::setw(2) << hh << ':' <<
//            std::setfill('0') << std::setw(2) << mm << ':' <<
//            std::setfill('0') << std::setw(2) << ss << '.' <<
//            std::setfill('0') << std::setw(3) << ms << std::endl;
//    }
    
    indicators::show_console_cursor(true);
}
