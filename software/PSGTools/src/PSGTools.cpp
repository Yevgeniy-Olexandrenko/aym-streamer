#include <iostream>
#include <iomanip>
#include <chrono>

#include "module/Module.h"
#include "decoders/DecodePT3.h"
#include "decoders/DecodePSG.h"

#include "output/AYMStreamer/AYMStreamer.h"
#include "output/AY38910/AY38910.h"
#include "module/Player.h"

const std::string k_folder = "../../chiptunes/Mmmc/selected/";
const std::string k_file = "Mmcm - Beg!nSum.pt3";
const std::string k_output = "output.txt";
const int k_comPortIndex = 4;

static Player * g_player = nullptr;

static BOOL WINAPI console_ctrl_handler(DWORD dwCtrlType)
{
    if (g_player) g_player->Stop();
    return TRUE;
}


bool DecodeFileToModule(const std::string& filePath, Module& module)
{
    std::shared_ptr<Decoder> decoders[]{
        std::shared_ptr<Decoder>(new DecodePT3()),
        std::shared_ptr<Decoder>(new DecodePSG()),
    };

    module.SetFilePath(filePath);
    for (std::shared_ptr<Decoder> decoder : decoders)
    {
        if (decoder->Open(module))
        {
            Frame frame;
            while (decoder->Decode(frame))
            {
                frame.FixValues();
                module.AddFrame(frame);
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

        for (FrameId i = 0; i < module.GetFrameCount(); ++i)
        {
            if (i && module.HasLoop() && i == module.GetLoopFrameId()) delimiter();
            file << std::setfill('0') << std::setw(5) << i;
            file << "  [ " << module.GetFrame(i) << " ]" << std::endl;
            
        }
        delimiter();
        file.close();
    }
}

int main()
{
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

    Module module;
    DecodeFileToModule(k_folder + k_file, module);
    //SaveModuleDebugOutput(module);

    ////////////////////////////////////////////////////////////////////////////

    //AYMStreamer output(module, k_comPortIndex);
    AY38910 output(module);
    Player player(output);

    std::cout << std::setfill('-') << std::setw(48) << '-' << std::endl;
    std::cout << "PSG Tools v1.0" << std::endl;
    std::cout << "by Yevgeniy Olexandrenko" << std::endl;
    std::cout << std::setfill('-') << std::setw(48) << '-' << std::endl;
    std::cout << module;
    std::cout << std::setfill('-') << std::setw(48) << '-' << std::endl;

    g_player = &player;
    if (player.Init(module))
    {
        auto start = std::chrono::steady_clock::now();
        player.Play();
        while (player.IsPlaying())
        {
//#if 0
//            
//            bool left  = (GetAsyncKeyState(VK_LEFT ) & 0x1) != 0;
//            bool right = (GetAsyncKeyState(VK_RIGHT) & 0x1) != 0;
//            bool up    = (GetAsyncKeyState(VK_UP   ) & 0x1) != 0;
//            bool down  = (GetAsyncKeyState(VK_DOWN ) & 0x1) != 0;
//#else
//            bool left  = (GetKeyState(VK_LEFT  ) & 0x80) != 0;
//            bool right = (GetKeyState(VK_RIGHT ) & 0x80) != 0;
//            bool up    = (GetKeyState(VK_UP    ) & 0x80) != 0;
//            bool down  = (GetKeyState(VK_DOWN  ) & 0x80) != 0;
//            bool enter = (GetAsyncKeyState(VK_RETURN) & 0x1) != 0;
//#endif
//
//            if (right)
//            {
//                player.Step(+10);
//            }
//            else if (left)
//            {
//                player.Step(-10);
//            }
//            else
//            {
//                player.Step(+1);
//            }
//
//            if (enter)
//            {
//                if (player.IsPaused())
//                    player.Play();
//                else 
//                    player.Stop();
//            }

            FrameId frameId = player.GetFrameId();
            std::cout << "\r" << "Frame: " << frameId << "     ";
            Sleep(20);
        }

        uint32_t duration = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        int ms = duration % 1000; duration /= 1000;
        int ss = duration % 60;   duration /= 60;
        int mm = duration % 60;   duration /= 60;
        int hh = duration;

        std::cout << "Duration: " <<
            std::setfill('0') << std::setw(2) << hh << ':' <<
            std::setfill('0') << std::setw(2) << mm << ':' <<
            std::setfill('0') << std::setw(2) << ss << '.' <<
            std::setfill('0') << std::setw(3) << ms << std::endl;
    }
    std::cout << "Stop" << std::endl;
}
