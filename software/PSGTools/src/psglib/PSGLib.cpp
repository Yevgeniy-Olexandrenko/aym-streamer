#include "PSGLib.h"

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
#include "encoders/specific/EncodeWAV.h"

// stream encoders
#include "encoders/streams/EncodeAYM.h"
#include "encoders/streams/EncodePSG.h"
#include "encoders/streams/EncodeVGM.h"
#include "encoders/streams/EncodeVTX.h"
#include "encoders/streams/EncodeYM.h"

namespace PSG
{
	bool Decode(const std::filesystem::path& path, Stream& stream)
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
                    stream.AddFrame(frame);
                    frame.ResetChanges();
                }

                decoder->Close(stream);
                return true;
            }
        }
        return false;
	}

	bool Encode(const std::filesystem::path& path, Stream& stream)
	{
        std::shared_ptr<Encoder> encoders[]{
            std::shared_ptr<Encoder>(new EncodePSG()),
            std::shared_ptr<Encoder>(new EncodeAYM()),
            std::shared_ptr<Encoder>(new EncodeTXT()),
        };

        stream.file = path;
        for (std::shared_ptr<Encoder> encoder : encoders)
        {
            if (encoder->Open(stream))
            {
                for (FrameId id = 0; id < stream.framesCount(); ++id)
                {
                    const Frame& frame = stream.GetFrame(id);
                    encoder->Encode(frame);
                }

                encoder->Close(stream);
                return true;
            }
        }
        return false;
	}
}
