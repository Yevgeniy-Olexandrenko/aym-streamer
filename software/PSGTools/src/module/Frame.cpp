#include "Frame.h"

Frame::Frame()
{
}

Frame::Frame(const Frame& other)
{
	for (uint8_t i = 0; i < 16; ++i)
	{
		m_registers[i].first = other.m_registers[i].first;
		m_registers[i].second = other.m_registers[i].second;
	}
}

RegisterPair& Frame::operator[](uint8_t index)
{
	return m_registers[index];
}

const RegisterPair& Frame::operator[](uint8_t index) const
{
	return m_registers[index];
}

bool Frame::changed(uint8_t chip, uint8_t index) const
{
	return (chip ? m_registers[index].second.changed() : m_registers[index].first.changed());
}

uint8_t Frame::data(uint8_t chip, uint8_t index) const
{
	return (chip ? m_registers[index].second.data() : m_registers[index].first.data());
}

bool Frame::IsChanged() const
{
	for (const RegisterPair& reg : m_registers)
	{
		if (reg.first.changed()) return true;
		if (reg.second.changed()) return true;
	}
	return false;
}

void Frame::SetUnchanged()
{
	for (RegisterPair& reg : m_registers)
	{
		reg.first = Register(reg.first.data());
		reg.second = Register(reg.second.data());
	}
}

void Frame::FixValues()
{
	auto DoFix = [](uint8_t i, Register& reg)
	{
		if (reg.changed())
		{
			uint8_t data = reg.data();
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
			reg.override(data);
		}
	};

	for (uint8_t i = 0; i < 16; ++i)
	{
		DoFix(i, m_registers[i].first);
		DoFix(i, m_registers[i].second);
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
		const Register& reg = frame[index].first;
		if (reg.changed())
			out_byte(reg.data());
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
