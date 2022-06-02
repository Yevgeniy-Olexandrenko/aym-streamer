#pragma once

#include <Windows.h>

class SerialPort
{
public:
	enum class BaudRate
	{
		_9600   = CBR_9600,
		_14400  = CBR_14400,
		_19200  = CBR_19200,
		_38400  = CBR_38400,
		_56000  = CBR_56000,
		_57600  = CBR_57600,
		_115200 = CBR_115200,
		_128000 = CBR_128000,
		_256000 = CBR_256000
	};

	enum class DataBits
	{
		_5 = 5,
		_6 = 6,
		_7 = 7,
		_8 = 8
	};

	enum class StopBits
	{
		ONE = ONESTOPBIT,
		TWO = TWOSTOPBITS,
		ONE_HALF = ONE5STOPBITS
	};

	enum class Parity
	{
		NO = NOPARITY,
		ODD = ODDPARITY,
		EVEN = EVENPARITY,
		MARK = MARKPARITY,
		SPACE = SPACEPARITY
	};

	SerialPort();
	~SerialPort();

public:
	void Open(int index);
	void Close();

	bool SetBaudRate(BaudRate rate);
	bool SetDataBits(DataBits databits);
	bool SetStopBits(StopBits stopbits);
	bool SetParity(Parity parity);

	int GetBoudRate();
	int GetDataBits();
	int GetStopBits();
	int GetParity();

	int SendText(const char* data);
	int SendBinary(const char* data, int size);
	int ReciveText(char* buffer, int size);

private:
	HANDLE m_port;
};
