#include <iostream>
#include <windows.h>
#include "module/Module.h"
#include "decoders/DecodePSG.h"
#include "player/Player.h"

const std::string k_file = "sample.psg";
const std::string k_output = "output.txt";
const int k_comPortIndex = 4;

int main()
{
    std::ifstream file;
    file.open(k_file, std::fstream::in | std::fstream::binary);

    std::ofstream output;
    output.open(k_output, std::fstream::out);

    Module module;

    if (file && output)
    {
        DecodePSG decoder;
        if (decoder.InitModule(file, module))
        {
            Frame frame;
            while (true)
            {
                frame.MarkChanged(false);
                if (decoder.DecodeFrame(file, frame))
                {
                    frame.FixValues();
                    module.AddFrame(frame);

//                  output << frame << std::endl;
                }
                else break;
            }

//          output << "Frames: " << module.GetFrameCount() << std::endl;
        }
    }

    file.close();
    output.close();

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
