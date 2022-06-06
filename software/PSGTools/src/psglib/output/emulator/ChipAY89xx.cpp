#include "ChipAY89xx.h"
#include <string.h>
#include <math.h>

bool ChipAY89xx::Configure(double clockRate, int sampleRate)
{
    m_counter = 0;

    memset(&m_interpolatorL, 0, sizeof(m_interpolatorL));
    memset(&m_interpolatorR, 0, sizeof(m_interpolatorR));

    memset(&m_firL, 0, sizeof(m_firL));
    memset(&m_firR, 0, sizeof(m_firR));
    m_firIndex = 0;

    memset(&m_dcFilterL, 0, sizeof(m_dcFilterL));
    memset(&m_dcFilterR, 0, sizeof(m_dcFilterR));
    m_dcFilterIndex = 0;

	InternalReset();

	m_step = (clockRate / (sampleRate * 8 * DECIMATE_FACTOR));
	return (m_step < 1);
}

void ChipAY89xx::SetPan(int chan, double pan, int is_eqp)
{
    if (is_eqp) 
        InternalSetPan(chan, sqrt(1 - pan), sqrt(pan));
    else 
        InternalSetPan(chan, 1 - pan, pan);
}

void ChipAY89xx::Reset()
{
    InternalReset();
}

void ChipAY89xx::Write(byte reg, byte data)
{
    InternalWrite(reg, data);
}

void ChipAY89xx::Process()
{
    double* c_L = m_interpolatorL.c;
    double* y_L = m_interpolatorL.y;
    double* c_R = m_interpolatorR.c;
    double* y_R = m_interpolatorR.y;

    double* firL = &m_firL[FIR_SIZE - m_firIndex * DECIMATE_FACTOR];
    double* firR = &m_firR[FIR_SIZE - m_firIndex * DECIMATE_FACTOR];
    m_firIndex = (m_firIndex + 1) % (FIR_SIZE / DECIMATE_FACTOR - 1);

    double y;
    for (int i = DECIMATE_FACTOR - 1; i >= 0; --i) 
    {
        m_counter += m_step;
        if (m_counter >= 1) 
        {
            m_counter -= 1;

            y_L[0] = y_L[1];
            y_L[1] = y_L[2];
            y_L[2] = y_L[3];

            y_R[0] = y_R[1];
            y_R[1] = y_R[2];
            y_R[2] = y_R[3];

            m_outL = m_outR = 0;
            InternalUpdate(m_outL, m_outR);
            y_L[3] = m_outL;
            y_R[3] = m_outR;

            y = y_L[2] - y_L[0];
            c_L[0] = 0.50 * y_L[1] + 0.25 * (y_L[0] + y_L[2]);
            c_L[1] = 0.50 * y;
            c_L[2] = 0.25 * (y_L[3] - y_L[1] - y);

            y = y_R[2] - y_R[0];
            c_R[0] = 0.50 * y_R[1] + 0.25 * (y_R[0] + y_R[2]);
            c_R[1] = 0.50 * y;
            c_R[2] = 0.25 * (y_R[3] - y_R[1] - y);
        }

        firL[i] = (c_L[2] * m_counter + c_L[1]) * m_counter + c_L[0];
        firR[i] = (c_R[2] * m_counter + c_R[1]) * m_counter + c_R[0];
    }

    m_outL = Decimate(firL);
    m_outR = Decimate(firR);
}

void ChipAY89xx::RemoveDC()
{
    m_outL = FilterDC(m_dcFilterL, m_dcFilterIndex, m_outL);
    m_outR = FilterDC(m_dcFilterR, m_dcFilterIndex, m_outR);
    m_dcFilterIndex = (m_dcFilterIndex + 1) & (DC_FILTER_SIZE - 1);
}

double ChipAY89xx::GetOutL() const
{
    return m_outL;
}

double ChipAY89xx::GetOutR() const
{
    return m_outR;
}

double ChipAY89xx::Decimate(double* x) const
{
    double y = 
        -0.0000046183113992051936 * ( x[1] + x[191]) +
        -0.0000111776164088722500 * ( x[2] + x[190]) +
        -0.0000186102645020054320 * ( x[3] + x[189]) +
        -0.0000251345861356310120 * ( x[4] + x[188]) +
        -0.0000284942816906661970 * ( x[5] + x[187]) +
        -0.0000263968287932751590 * ( x[6] + x[186]) +
        -0.0000170942125588021560 * ( x[7] + x[185]) +
        +0.0000237981935769668660 * ( x[9] + x[183]) +
        +0.0000512811602422021830 * (x[10] + x[182]) +
        +0.0000776219782624342700 * (x[11] + x[181]) +
        +0.0000967594266641204160 * (x[12] + x[180]) +
        +0.0001024022930039340200 * (x[13] + x[179]) +
        +0.0000893446142180771060 * (x[14] + x[178]) +
        +0.0000548757001189491830 * (x[15] + x[177]) +
        -0.0000698390822106801650 * (x[17] + x[175]) +
        -0.0001447966132360757000 * (x[18] + x[174]) +
        -0.0002115845291770830800 * (x[19] + x[173]) +
        -0.0002553506910655054400 * (x[20] + x[172]) +
        -0.0002622871437432210400 * (x[21] + x[171]) +
        -0.0002225880592702779900 * (x[22] + x[170]) +
        -0.0001332323049569570400 * (x[23] + x[169]) +
        +0.0001618257876705520600 * (x[25] + x[167]) +
        +0.0003284617538509658100 * (x[26] + x[166]) +
        +0.0004704561157618486300 * (x[27] + x[165]) +
        +0.0005571385145753094400 * (x[28] + x[164]) +
        +0.0005621256512151872600 * (x[29] + x[163]) +
        +0.0004690191855396247800 * (x[30] + x[162]) +
        +0.0002762486683895298600 * (x[31] + x[161]) +
        -0.0003256417948683862200 * (x[33] + x[159]) +
        -0.0006518231028671038800 * (x[34] + x[158]) +
        -0.0009212778730931929800 * (x[35] + x[157]) +
        -0.0010772534348943575000 * (x[36] + x[156]) +
        -0.0010737727700273478000 * (x[37] + x[155]) +
        -0.0008855664539039263400 * (x[38] + x[154]) +
        -0.0005158189609076553400 * (x[39] + x[153]) +
        +0.0005954876719379527700 * (x[41] + x[151]) +
        +0.0011803558710661009000 * (x[42] + x[150]) +
        +0.0016527320270369871000 * (x[43] + x[149]) +
        +0.0019152679330965555000 * (x[44] + x[148]) +
        +0.0018927324805381538000 * (x[45] + x[147]) +
        +0.0015481870327877937000 * (x[46] + x[146]) +
        +0.0008947069583494130600 * (x[47] + x[145]) +
        -0.0010178225878206125000 * (x[49] + x[143]) +
        -0.0020037400552054292000 * (x[50] + x[142]) +
        -0.0027874356824117317000 * (x[51] + x[141]) +
        -0.0032103299880219430000 * (x[52] + x[140]) +
        -0.0031540624117984395000 * (x[53] + x[139]) +
        -0.0025657163651900345000 * (x[54] + x[138]) +
        -0.0014750752642111449000 * (x[55] + x[137]) +
        +0.0016624165446378462000 * (x[57] + x[135]) +
        +0.0032591192839069179000 * (x[58] + x[134]) +
        +0.0045165685815867747000 * (x[59] + x[133]) +
        +0.0051838984346123896000 * (x[60] + x[132]) +
        +0.0050774264697459933000 * (x[61] + x[131]) +
        +0.0041192521414141585000 * (x[62] + x[130]) +
        +0.0023628575417966491000 * (x[63] + x[129]) +
        -0.0026543507866759182000 * (x[65] + x[127]) +
        -0.0051990251084333425000 * (x[66] + x[126]) +
        -0.0072020238234656924000 * (x[67] + x[125]) +
        -0.0082672928192007358000 * (x[68] + x[124]) +
        -0.0081033739572956287000 * (x[69] + x[123]) +
        -0.0065831115395702210000 * (x[70] + x[122]) +
        -0.0037839040415292386000 * (x[71] + x[121]) +
        +0.0042781252851152507000 * (x[73] + x[119]) +
        +0.0084176358598320178000 * (x[74] + x[118]) +
        +0.0117256605746305500000 * (x[75] + x[117]) +
        +0.0135504766477886720000 * (x[76] + x[116]) +
        +0.0133881893699974960000 * (x[77] + x[115]) +
        +0.0109795012423412590000 * (x[78] + x[114]) +
        +0.0063812749416854130000 * (x[79] + x[113]) +
        -0.0074212296041538880000 * (x[81] + x[111]) +
        -0.0148645630434021300000 * (x[82] + x[110]) +
        -0.0211435846221781040000 * (x[83] + x[109]) +
        -0.0250427505875860900000 * (x[84] + x[108]) +
        -0.0254735309425472010000 * (x[85] + x[107]) +
        -0.0216273100178821960000 * (x[86] + x[106]) +
        -0.0131043233832255430000 * (x[87] + x[105]) +
        +0.0170651339899804760000 * (x[89] + x[103]) +
        +0.0369789192644519520000 * (x[90] + x[102]) +
        +0.0582331806209395800000 * (x[91] + x[101]) +
        +0.0790720120814059490000 * (x[92] + x[100]) +
        +0.0976759987169523170000 * (x[93] + x[99] ) +
        +0.1123604593695093200000 * (x[94] + x[98] ) +
        +0.1217634357728773100000 * (x[95] + x[97] ) +
        +0.125 * x[96];

    memcpy(&x[FIR_SIZE - DECIMATE_FACTOR], x, DECIMATE_FACTOR * sizeof(double));
    return y;
}

double ChipAY89xx::FilterDC(DCFilter& filter, int index, double x) const
{
    filter.sum += -filter.delay[index] + x;
    filter.delay[index] = x;
    return (x - (filter.sum / DC_FILTER_SIZE));
}

// Envelope Unit Implementation
////////////////////////////////////////////////////////////////////////////////

namespace
{
    enum class Envelope { SU, SD, HT, HB };

    Envelope envelopes[16][2]
    {
        { Envelope::SD, Envelope::HB },
        { Envelope::SD, Envelope::HB },
        { Envelope::SD, Envelope::HB },
        { Envelope::SD, Envelope::HB },
        { Envelope::SU, Envelope::HB },
        { Envelope::SU, Envelope::HB },
        { Envelope::SU, Envelope::HB },
        { Envelope::SU, Envelope::HB },
        { Envelope::SD, Envelope::SD },
        { Envelope::SD, Envelope::HB },
        { Envelope::SD, Envelope::SU },
        { Envelope::SD, Envelope::HT },
        { Envelope::SU, Envelope::SU },
        { Envelope::SU, Envelope::HT },
        { Envelope::SU, Envelope::SD },
        { Envelope::SU, Envelope::HB },
    };
}

void ChipAY89xx::EnvelopeUnit::SetPeriod(int period)
{
    period &= 0xFFFF;
    m_period = ((period == 0) | period);
}

void ChipAY89xx::EnvelopeUnit::SetShape(int shape)
{
    m_shape = (shape & 0x0F);
    m_counter = 0;
    m_segment = 0;
    ResetSegment();
}

int ChipAY89xx::EnvelopeUnit::GetPeriod() const
{
    return m_period;
}

void ChipAY89xx::EnvelopeUnit::Reset()
{
    SetPeriod(0);
    SetShape(0);
}

int ChipAY89xx::EnvelopeUnit::Update()
{
    if (++m_counter >= m_period)
    {
        m_counter = 0;

        Envelope env = envelopes[m_shape][m_segment];
        bool doneSD = (env == Envelope::SD && --m_envelope < 0x00);
        bool doneSU = (env == Envelope::SU && ++m_envelope > 0x1F);

        if (doneSD || doneSU)
        {
            m_segment ^= 1;
            ResetSegment();
        }
    }
    return m_envelope;
}

void ChipAY89xx::EnvelopeUnit::ResetSegment()
{
    Envelope env = envelopes[m_shape][m_segment];
    m_envelope = (env == Envelope::SD || env == Envelope::HT ? 0x1F : 0x00);
}

// Noise Unit Implementation
////////////////////////////////////////////////////////////////////////////////

void ChipAY89xx::NoiseUnit::SetPeriod(int period)
{
    period &= 0x1F;
    m_period = ((period == 0) | period);
}

void ChipAY89xx::NoiseUnit::Reset()
{
    m_noise = 1;
    SetPeriod(0);
}

int ChipAY89xx::NoiseUnit::Update()
{
    if (++m_counter >= (m_period << 1))
    {
        m_counter = 0;
        int bit0x3 = ((m_noise ^ (m_noise >> 3)) & 1);
        m_noise = (m_noise >> 1) | (bit0x3 << 16);
    }
    return (m_noise & 1);
}

// Tone Unit Implementation
////////////////////////////////////////////////////////////////////////////////

void ChipAY89xx::ToneUnit::SetPeriod(int period)
{
    period &= 0x0FFF;
    m_period = ((period == 0) | period);
}

int ChipAY89xx::ToneUnit::GetPeriod() const
{
    return m_period;
}

void ChipAY89xx::ToneUnit::Reset()
{
    m_tone = 0;
    SetPeriod(0);
}

int ChipAY89xx::ToneUnit::Update()
{
    if (++m_counter >= m_period)
    {
        m_counter = 0;
        m_tone ^= 1;
    }
    return m_tone;
}
