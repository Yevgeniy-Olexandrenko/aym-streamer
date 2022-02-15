#include <iostream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <functional>
#include <terminal/terminal.hpp>

#include "module/Filelist.h"
#include "module/Module.h"
#include "module/Player.h"

#include "decoders/DecodeVTX.h"
#include "decoders/DecodePT3.h"
#include "decoders/DecodePSG.h"

#include "output/Streamer/Streamer.h"
#include "output/Emulator/Emulator.h"


const std::string k_filelist = "D:\\projects\\github\\aym-streamer\\chiptunes\\Mmmc\\selected\\";
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

void PrintDelimiter()
{
    using namespace terminal;
    size_t term_w = terminal_width() - 2;
    std::cout << ' ' << color::bright_cyan;
    std::cout << std::string(term_w, '-');
    std::cout << color::reset << std::endl;
}

void PrintModuleFile(const Module& module, int number, int total)
{
    using namespace terminal;
    std::string filename = module.file.nameExt();
    std::string filenumber = std::to_string(number) + '/' + std::to_string(total);

    size_t strlen = filenumber.length() + filename.length();
    size_t term_w = terminal_width() - 2;

    std::cout << ' ' << color::bright_cyan;
    std::cout << std::string(term_w - strlen - 7, '-')  << "[ ";
    std::cout << color::bright_yellow << filenumber << ' ';
    std::cout << color::bright_white << module.file.name();
    std::cout << color::bright_magenta << '.';
    std::cout << color::white << module.file.ext();
    std::cout << color::bright_cyan << " ]--";
    std::cout << color::reset << std::endl;
}

void PrintModuleInfo(const Module& module)
{
    using namespace terminal;

    auto TrimString = [](std::string& str)
    {
        str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), str.end());
    };

    auto PrintPropertyLabel = [](const std::string& label)
    {
        std::cout << ' ' << color::bright_yellow << label;
        std::cout << color::bright_grey << std::string(9 - label.length(), '.');
        std::cout << color::bright_magenta << ": ";
    };

    auto PrintModuleProperty = [&](const std::string& label, Module::Property property)
    {
        std::string str = module.property(property);
        TrimString(str);

        if (!str.empty())
        {
            PrintPropertyLabel(label);
            if (property == Module::Property::Title)
                std::cout << color::bright_green;
            else
                std::cout << color::bright_white;
            std::cout << str << color::reset << std::endl;
        }
    };

    auto PrintOutputProperty = [&](const std::string& label, const Output& output)
    {
        std::string str = output.name();
        TrimString(str);

        if (!str.empty())
        {
            PrintPropertyLabel(label);
            std::cout << color::bright_cyan;
            std::cout << str << color::reset << std::endl;
        }
    };

    PrintModuleProperty("Title", Module::Property::Title);
    PrintModuleProperty("Artist", Module::Property::Artist);
    PrintModuleProperty("Type", Module::Property::Type);
    PrintModuleProperty("Chip", Module::Property::Chip);
    PrintModuleProperty("Frames", Module::Property::Frames);
    PrintModuleProperty("Duration", Module::Property::Duration);
    PrintOutputProperty("Output", *m_output);
}

void PrintHeader()
{
    using namespace terminal;
    std::cout << color::bright_green << " frame [ R";
    std::cout << color::bright_yellow << "7";
    std::cout << color::bright_green << "|R";
    std::cout << color::bright_yellow << "1";
    std::cout << color::bright_green << "R";
    std::cout << color::bright_yellow << "0";
    std::cout << color::bright_green << " R";
    std::cout << color::bright_yellow << "8";
    std::cout << color::bright_green << "|R";
    std::cout << color::bright_yellow << "3";
    std::cout << color::bright_green << "R";
    std::cout << color::bright_yellow << "2";
    std::cout << color::bright_green << " R";
    std::cout << color::bright_yellow << "9";
    std::cout << color::bright_green << "|R";
    std::cout << color::bright_yellow << "5";
    std::cout << color::bright_green << "R";
    std::cout << color::bright_yellow << "4";
    std::cout << color::bright_green << " R";
    std::cout << color::bright_yellow << "A";
    std::cout << color::bright_green << "|R";
    std::cout << color::bright_yellow << "C";
    std::cout << color::bright_green << "R";
    std::cout << color::bright_yellow << "B";
    std::cout << color::bright_green << " R";
    std::cout << color::bright_yellow << "D";
    std::cout << color::bright_green << "|R";
    std::cout << color::bright_yellow << "6";
    std::cout << color::bright_green << " ]";
    std::cout << color::reset << std::endl;
}

void PrintFrames(const Module& module, int frameId, int range)
{
    using namespace terminal;

    Frame fakeFrame;
    for (int i = frameId - range; i <= frameId + range; ++i)
    {
        bool isNotFake = (i >= 0 && i < module.playback.framesCount());
        const Frame& frame = isNotFake ? module.playback.getFrame(i) : fakeFrame;

        auto out_nibble = [&](uint8_t nibble)
        {
            nibble &= 0x0F;
            std::cout << char((nibble >= 0x0A ? 'A' - 0x0A : '0') + nibble);
        };

        auto out_byte = [&](uint8_t data)
        {
            if (i == frameId)
            {
                std::cout << color::bright_white;
                out_nibble(data >> 4);
                out_nibble(data);
            }
            else
            {
                std::cout << color::bright_green;
                out_nibble(data >> 4);
                out_nibble(data);
            }
        };

        auto out_reg = [&](uint8_t index)
        {
            const Register& reg = frame[index].first;
            if (reg.changed())
                out_byte(reg.data());
            else
                std::cout << color::bright_grey << "..";
        };

        std::cout << ' ';
        if (i == frameId) std::cout << color::on_blue;

        if (isNotFake)
        {
            std::cout << std::setfill('0') << std::setw(5) << i;
        }
        else
        {
            std::cout << std::string(5, '-');
        }
       
            
        std::cout << color::bright_cyan << " [ ";
        out_reg(Mixer_Flags);  std::cout << color::bright_cyan << '|';
        out_reg(TonA_PeriodH);
        out_reg(TonA_PeriodL); std::cout << ' ';
        out_reg(VolA_EnvFlg);  std::cout << color::bright_cyan << '|';
        out_reg(TonB_PeriodH);
        out_reg(TonB_PeriodL); std::cout << ' ';
        out_reg(VolB_EnvFlg);  std::cout << color::bright_cyan << '|';
        out_reg(TonC_PeriodH);
        out_reg(TonC_PeriodL); std::cout << ' ';
        out_reg(VolC_EnvFlg);  std::cout << color::bright_cyan << '|';
        out_reg(Env_PeriodH);
        out_reg(Env_PeriodL);  std::cout << ' ';
        out_reg(Env_Shape);    std::cout << color::bright_cyan << '|';
        out_reg(Noise_Period);
        std::cout << color::bright_cyan << " ]";
        std::cout << color::reset << std::endl;
    }
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

int main()
{
    using namespace terminal;
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
    cursor::show(false);
    size_t w = terminal_width() - 1;

    PrintDelimiter();
    std::cout << ' ' << color::bright_red << "PSG Tools v1.0" << color::reset << std::endl;
    std::cout << ' ' << color::bright_red << "by Yevgeniy Olexandrenko" << color::reset << std::endl;
    PrintDelimiter();
    std::cout << std::endl;

    m_output.reset(new Emulator());
    m_player.reset(new Player(*m_output));
    m_filelist.reset(new Filelist("pt3|psg|vtx", k_filelist));

    if (!m_filelist->empty())
    {
        m_filelist->shuffle();

        std::string path;
        while (m_filelist->next(path))
        {
            m_module.reset(new Module());
            if (DecodeFileToModule(path, *m_module))
            {
                PrintModuleFile(*m_module, m_filelist->index(), m_filelist->count());
               
                if (m_player->Init(*m_module))
                {
                    PrintModuleInfo(*m_module);
                    PrintHeader();

                    m_player->Play();
                    auto start = std::chrono::steady_clock::now();

                    FrameId oldFrame = -1;
                    while (m_player->IsPlaying())
                    {
                        FrameId newFrame = m_player->GetFrameId();
                        if (newFrame != oldFrame)
                        {
                            oldFrame = newFrame;
                           /* cursor::move_down(1);
                            cursor::erase_line();
                            std::cout << " Frame: " << newFrame;
                            cursor::move_up(1);*/

                            PrintFrames(*m_module, newFrame, 9);
                            cursor::move_up(19);
                        }
                        Sleep(1);
                    }

                    uint32_t duration = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
                            int ms = duration % 1000; duration /= 1000;
                            int ss = duration % 60;   duration /= 60;
                            int mm = duration % 60;   duration /= 60;
                            int hh = duration;
                    
                            std::cout << std::endl;
                            std::cout << " Duration: " <<
                                std::setfill('0') << std::setw(2) << hh << ':' <<
                                std::setfill('0') << std::setw(2) << mm << ':' <<
                                std::setfill('0') << std::setw(2) << ss << '.' <<
                                std::setfill('0') << std::setw(3) << ms << std::endl;

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
}
