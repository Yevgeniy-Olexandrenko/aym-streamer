#include "Functions.h"

// module decoders
#include "decoders/modules/DecodePT3.h"
#include "decoders/modules/DecodePT2.h"
#include "decoders/modules/DecodeSTC.h"
#include "decoders/modules/DecodeASC.h"
#include "decoders/modules/DecodeSTP.h"
#include "decoders/modules/DecodeSQT.h"

// stream decoders
#include "decoders/streams/DecodeVTX.h"
#include "decoders/streams/DecodePSG.h"
#include "decoders/streams/DecodeVGM.h"
#include "decoders/streams/DecodeYM.h"

// specific encoders
#include "encoders/specific/EncodeTXT.h"

namespace Functions
{
    bool DecodeFile(const std::filesystem::path& path, Stream& stream)
    {
        std::shared_ptr<Decoder> decoders[]{
            // modules
            std::shared_ptr<Decoder>(new DecodePT3()),
            std::shared_ptr<Decoder>(new DecodePT2()),
            std::shared_ptr<Decoder>(new DecodeSTC()),
            std::shared_ptr<Decoder>(new DecodeASC()),
            std::shared_ptr<Decoder>(new DecodeSTP()),
            std::shared_ptr<Decoder>(new DecodeSQT()),

            // streams
            std::shared_ptr<Decoder>(new DecodeYM()),
            std::shared_ptr<Decoder>(new DecodePSG()),
            std::shared_ptr<Decoder>(new DecodeVTX()),
            std::shared_ptr<Decoder>(new DecodeVGM()),
        };

        stream.file = path;
        for (std::shared_ptr<Decoder> decoder : decoders)
        {
            if (decoder->Open(stream))
            {
                Frame frame;
                while (decoder->Decode(frame))
                {
                    stream.frames.add(frame);
                    frame.ResetChanges();
                }

                decoder->Close(stream);
                return true;
            }
        }
        return false;
    }

    bool EncodeFile(const std::filesystem::path& path, Stream& stream)
    {
        std::shared_ptr<Encoder> encoders[]{
            std::shared_ptr<Encoder>(new EncodeTXT()),
        };

        stream.file = path;
        for (std::shared_ptr<Encoder> encoder : encoders)
        {
            if (encoder->Open(stream))
            {
                for (FrameId id = 0; id < stream.frames.count(); ++id)
                {
                    encoder->Encode(id, stream.frames.get(id));
                }

                encoder->Close(stream);
                return true;
            }
        }
        return false;
    }

}