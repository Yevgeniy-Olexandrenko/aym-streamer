#include "Frame.h"

Frame::Frame()
{
}

Frame::Frame(const Frame& other)
	: m_registers(other.m_registers)
{
}

Frame::operator bool() const
{
	for (const Register& reg : m_registers)
	{
		if (reg.IsChanged()) return true;
	}
	return false;
}

Register& Frame::operator[](uint8_t index)
{
	return m_registers[index];
}

const Register& Frame::operator[](uint8_t index) const
{
	return m_registers[index];
}

void Frame::SetUnchanged()
{
	for (Register& reg : m_registers)
	{
		reg = Register(reg.GetData());
	}
}

void Frame::FixValues()
{
	for (uint8_t i = 0; i < 16; ++i)
	{
		Register& reg = m_registers[i];
		if (reg.IsChanged())
		{
			uint8_t data = reg.GetData();
			switch (i)
			{
			case TonA_PeriodH:
			case TonB_PeriodH:
			case TonC_PeriodH:
			case Env_Shape:
				data &= 0x0F;
				break;

			case VolA_EnvFlg:
			case VolB_EnvFlg:
			case VolC_EnvFlg:
			case Noise_Period:
				data &= 0x1F;
				break;

			case Mixer_Flags:
				data &= 0x3F;
				break;

			case PortA_Data:
			case PortB_Data:
				data = 0x00;
				break;
			}
			reg.OverrideData(data);
		}
	}
}

std::ostream& operator<<(std::ostream& stream, const Frame& frame)
{
	auto out_nibble = [&](uint8_t nibble)
	{
		nibble &= 0x0F;
		stream << char((nibble >= 0x0A ? 'A' - 0x0A : '0') + nibble);
	};

	auto out_byte = [&](uint8_t data)
	{
		out_nibble(data >> 4);
		out_nibble(data);
	};

	auto out_reg = [&](uint8_t index)
	{
		const Register& reg = frame[index];
		if (reg.IsChanged())
			out_byte(reg.GetData());
		else
			stream << "--";
	};

	out_reg(Mixer_Flags);  stream << '|';
	out_reg(TonA_PeriodH);
	out_reg(TonA_PeriodL); stream << ' ';
	out_reg(VolA_EnvFlg);  stream << '|';
	out_reg(TonB_PeriodH);
	out_reg(TonB_PeriodL); stream << ' ';
	out_reg(VolB_EnvFlg);  stream << '|';
	out_reg(TonC_PeriodH);
	out_reg(TonC_PeriodL); stream << ' ';
	out_reg(VolC_EnvFlg);  stream << '|';
	out_reg(Env_PeriodH);
	out_reg(Env_PeriodL);  stream << ' ';
	out_reg(Env_Shape);    stream << '|';
	out_reg(Noise_Period);

	return stream;
}
