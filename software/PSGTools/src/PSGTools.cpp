#include <iostream>
#include <windows.h>
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

int main()
{
    Module module;
    DecodeFileToModule(k_file, module);

    std::cout << "File: " << module.GetFileName() << std::endl;
    if (module.HasTitle()) std::cout << "Title: " << module.GetTitle() << std::endl;
    if (module.HasArtist()) std::cout << "Artist: " << module.GetArtist() << std::endl;
    std::cout << "Type: " << module.GetType() << std::endl;
    std::cout << "Frames cound: " << module.GetFrameCount() << std::endl;
    if (module.IsLoopFrameAvailable()) std::cout << "Loop frame: " << module.GetLoopFrameIndex() << std::endl;
    std::cout << "Frame rate: " << module.GetFrameRate() << std::endl;

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
