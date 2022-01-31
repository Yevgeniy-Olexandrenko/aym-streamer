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

Register& Frame::operator[](size_t index)
{
	return m_registers[index];
}

Register& Frame::operator[](Register::Index index)
{
	return m_registers[size_t(index)];
}

const Register& Frame::operator[](size_t index) const
{
	return m_registers[index];
}

const Register& Frame::operator[](Register::Index index) const
{
	return m_registers[size_t(index)];
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
	for (size_t i = 0; i < size_t(Register::Index::COUNT); ++i)
	{
		Register& reg = m_registers[i];
		if (reg.IsChanged())
		{
			uint8_t data = reg.GetData();
			switch (Register::Index(i))
			{
			case Register::Index::TonA_PeriodH:
			case Register::Index::TonB_PeriodH:
			case Register::Index::TonC_PeriodH:
			case Register::Index::Env_Shape:
				data &= 0x0F;
				break;

			case Register::Index::VolA_EnvFlg:
			case Register::Index::VolB_EnvFlg:
			case Register::Index::VolC_EnvFlg:
			case Register::Index::Noise_Period:
				data &= 0x1F;
				break;

			case Register::Index::Mixer_Flags:
				data &= 0x3F;
				break;

			case Register::Index::PortA_Data:
			case Register::Index::PortB_Data:
				data = 0x00;
				break;
			}
			reg = data;
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

	auto out_reg = [&](Register::Index index)
	{
		const Register& reg = frame[index];
		if (reg.IsChanged())
			out_byte(reg.GetData());
		else
			stream << "--";
	};

	out_reg(Register::Index::Mixer_Flags);  stream << '|';
	out_reg(Register::Index::TonA_PeriodH);
	out_reg(Register::Index::TonA_PeriodL); stream << ' ';
	out_reg(Register::Index::VolA_EnvFlg);  stream << '|';
	out_reg(Register::Index::TonB_PeriodH);
	out_reg(Register::Index::TonB_PeriodL); stream << ' ';
	out_reg(Register::Index::VolB_EnvFlg);  stream << '|';
	out_reg(Register::Index::TonC_PeriodH);
	out_reg(Register::Index::TonC_PeriodL); stream << ' ';
	out_reg(Register::Index::VolC_EnvFlg);  stream << '|';
	out_reg(Register::Index::Env_PeriodH);
	out_reg(Register::Index::Env_PeriodL);  stream << ' ';
	out_reg(Register::Index::Env_Shape);    stream << '|';
	out_reg(Register::Index::Noise_Period);

	return stream;
}
