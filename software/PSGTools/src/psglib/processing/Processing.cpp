#include "Processing.h"
#include <cassert>

void Processing::Reset()
{
    m_frame.ResetData();
    m_frame.ResetChanges();
}

void Processing::Update(const Frame& frame)
{
    m_frame.ResetChanges();
    m_frame += frame;
}

const Frame& Processing::operator()(const Frame& frame)
{
    return frame;
}

// const Frame& ConvertExpToComp::operator()(const Chip& chip, const Frame& frame)
// {
// #ifdef Enable_ConvertExpToComp
//     if (chip.first.model() != Chip::Model::AY8930 || (chip.second.modelKnown() && chip.second.model() != Chip::Model::AY8930))
//     {
//         m_frame.ResetChanges();
//         for (int count = chip.count(), chip = 0; chip < count; ++chip)
//         {
//             if (frame[chip].IsExpMode())
//             {
//                 m_frame[chip].UpdatePeriod(A_Period, frame[chip].ReadPeriod(A_Period));
//                 m_frame[chip].UpdatePeriod(B_Period, frame[chip].ReadPeriod(B_Period));
//                 m_frame[chip].UpdatePeriod(C_Period, frame[chip].ReadPeriod(C_Period));
//                 m_frame[chip].UpdatePeriod(N_Period, frame[chip].ReadPeriod(N_Period));
//                 m_frame[chip].Update(Mixer, frame[chip].Read(Mixer));

//                 // convert volume + envelope flag registers
//                 uint8_t a_volume = frame[chip].Read(A_Volume);
//                 uint8_t b_volume = frame[chip].Read(B_Volume);
//                 uint8_t c_volume = frame[chip].Read(C_Volume);
//                 m_frame[chip].Update(A_Volume, a_volume >> 1);
//                 m_frame[chip].Update(B_Volume, b_volume >> 1);
//                 m_frame[chip].Update(C_Volume, c_volume >> 1);

//                 // choose envelope depending on channels priority
// #if 1
//                 auto e_period = frame[chip].ReadPeriod(EC_Period);
//                 auto e_shape = frame[chip].Read(EC_Shape);
//                 m_frame[chip].UpdatePeriod(E_Period, e_period);
//                 m_frame[chip].Update(E_Shape, e_shape);
//                 if (e_shape != c_unchangedShape)
//                     e_shape &= 0x0F;
// #else
//                 auto e_period = frame.ReadPeriod(chip, EA_Period);
//                 auto e_shape = frame.Read(chip, EA_Shape);
//                 if ((a_volume & 0x20) == 0)
//                 {
//                     if ((b_volume & 0x20) != 0)
//                     {
//                         e_period = frame.ReadPeriod(chip, EB_Period);
//                         e_shape = frame.Read(chip, EB_Shape);
//                     }
//                     else if ((c_volume & 0x20) != 0)
//                     {
//                         e_period = frame.ReadPeriod(chip, EC_Period);
//                         e_shape = frame.Read(chip, EC_Shape);
//                         if (e_shape != k_unchangedShape)
//                             e_shape &= 0x0F;
//                     }
//                 }
//                 if (e_shape != k_unchangedShape) 
//                     e_shape &= 0x0F;
//                 m_frame.UpdatePeriod(chip, E_Period, e_period);
//                 m_frame.Update(chip, E_Shape, e_shape);
// #endif
//             }
//         }
//         return m_frame;
//     }
// #endif
//     return frame;
// }

// const Frame& ConvertToNewClock::operator()(const Chip& chip, const Frame& frame)
// {
// #ifdef Enable_ConvertToNewClock
//     {
//         Update(frame);

//         uint16_t a_period = m_frame[chip].ReadPeriod(A_Period);
//         uint16_t b_period = m_frame[chip].ReadPeriod(B_Period);
//         uint16_t c_period = m_frame[chip].ReadPeriod(C_Period);

//         m_frame[chip].UpdatePeriod(A_Period, a_period << 1);
//         m_frame[chip].UpdatePeriod(B_Period, b_period << 1);
//         m_frame[chip].UpdatePeriod(C_Period, c_period << 1);

//         return m_frame;
//     }
// #endif
//     return frame;
// }

