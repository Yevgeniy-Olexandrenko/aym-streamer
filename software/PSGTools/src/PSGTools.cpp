#include <iostream>
#include "Module.h"
#include "Frame.h"
#include "decoders/DecodePSG.h"

const std::string k_file = "sample2.psg";
const std::string k_output = "output.txt";

int main()
{
    std::ifstream file;
    file.open(k_file, std::fstream::in | std::fstream::binary);

    std::ofstream output;
    output.open(k_output, std::fstream::out);

    if (file && output)
    {
        Module module;
        DecodePSG decoder;

        if (decoder.InitModule(file, module))
        {
            Frame frame;
            while (true)
            {
                frame.ResChanges();
                if (decoder.DecodeFrame(file, frame))
                {
                    frame.FixValues();
                    module.AddFrame(frame);

                    output << frame << std::endl;
                }
                else break;
            }

            output << "Frames: " << module.GetFrameCount() << std::endl;
        }
    }

    file.close();
    output.close();
}
