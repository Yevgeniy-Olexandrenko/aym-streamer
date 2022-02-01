#include <iostream>
#include <iomanip>

#include "module/Module.h"
#include "decoders/DecodePT3.h"
#include "decoders/DecodePSG.h"
#include "player/Player.h"

const std::string k_file = "sample.psg";
const std::string k_output = "output.txt";
const int k_comPortIndex = 4;

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

        for (Module::FrameIndex i = 0; i < module.GetFrameCount(); ++i)
        {
            if (i && module.HasLoopFrameIndex() && i == module.GetLoopFrameIndex()) delimiter();
            file << std::setfill('0') << std::setw(5) << i;
            file << "  [ " << module.GetFrame(i) << " ]" << std::endl;
            
        }
        delimiter();
        file.close();
    }
}

int main()
{
    Module module;
    DecodeFileToModule(k_file, module);
    SaveModuleDebugOutput(module);

    ////////////////////////////////////////////////////////////////////////////

    Player player(k_comPortIndex);

    if (player.InitWithModule(module))
    {
        while (true)
        {
            if (!player.PlayModuleFrame()) break;
            Sleep(20);
        }

        player.Mute(true);
    }
}
