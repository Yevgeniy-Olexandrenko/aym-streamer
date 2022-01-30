#include "SerialPort.h"

SerialPort::SerialPort()
	: m_port(NULL)
{
}

SerialPort::~SerialPort()
{
	Close();
}

void SerialPort::Open(int index)
{
	TCHAR comName[100];
	wsprintf(comName, TEXT("\\\\.\\COM%d"), index);

	HANDLE hComm = CreateFile(
		comName,      //port name 
		GENERIC_READ | GENERIC_WRITE,		 
		0,            // No Sharing                               
		NULL,         // No Security                              
		OPEN_EXISTING,// Open existing port only                     
		0,            // Non Overlapped I/O                           
		NULL);        // Null for Comm Devices

	if (hComm == INVALID_HANDLE_VALUE) return;

	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;

	if (SetCommTimeouts(hComm, &timeouts) == FALSE) return;
	if (SetCommMask(hComm, EV_RXCHAR) == FALSE) return;

	Close();
	m_port = hComm;
}

void SerialPort::Close()
{
	if (m_port)
	{
		CloseHandle(m_port);
		m_port = NULL;
	}
}

bool SerialPort::SetBaudRate(BaudRate rate)
{
	DCB serialParams = { 0 };
	serialParams.DCBlength = sizeof(serialParams);

	BOOL status = GetCommState(m_port, &serialParams);
	if (status == FALSE) return FALSE;

	serialParams.BaudRate = DWORD(rate);
	status = SetCommState(m_port, &serialParams);

	return status;
}

bool SerialPort::SetDataBits(DataBits databits)
{
	DCB serialParams = { 0 };
	serialParams.DCBlength = sizeof(serialParams);

	BOOL status = GetCommState(m_port, &serialParams);
	if (status == FALSE) return FALSE;

	serialParams.ByteSize = BYTE(databits);
	status = SetCommState(m_port, &serialParams);

	return status;
}

bool SerialPort::SetStopBits(StopBits stopbits)
{
	DCB serialParams = { 0 };
	serialParams.DCBlength = sizeof(serialParams);

	BOOL status = GetCommState(m_port, &serialParams);
	if (status == FALSE) return FALSE;

	serialParams.StopBits = BYTE(stopbits);
	status = SetCommState(m_port, &serialParams);

	return status;
}

bool SerialPort::SetParity(Parity parity)
{
	DCB serialParams = { 0 };
	serialParams.DCBlength = sizeof(serialParams);

	BOOL status = GetCommState(m_port, &serialParams);
	if (status == FALSE) return FALSE;

	serialParams.Parity = BYTE(parity);
	status = SetCommState(m_port, &serialParams);

	return status;
}

int SerialPort::GetBoudRate()
{
	DCB serialParams = { 0 };
	serialParams.DCBlength = sizeof(serialParams);

	BOOL status = GetCommState(m_port, &serialParams);
	if (status == FALSE) return -1;

	return serialParams.BaudRate;
}

int SerialPort::GetDataBits()
{
	DCB serialParams = { 0 };
	serialParams.DCBlength = sizeof(serialParams);

	BOOL status = GetCommState(m_port, &serialParams);
	if (status == FALSE) return -1;

	return serialParams.ByteSize;
}

int SerialPort::GetStopBits()
{
	DCB serialParams = { 0 };
	serialParams.DCBlength = sizeof(serialParams);

	BOOL status = GetCommState(m_port, &serialParams);
	if (status == FALSE) return -1;

	return serialParams.StopBits;
}

int SerialPort::GetParity()
{
	DCB serialParams = { 0 };
	serialParams.DCBlength = sizeof(serialParams);

	BOOL status = GetCommState(m_port, &serialParams);
	if (status == FALSE) return -1;

	return serialParams.Parity;
}

int SerialPort::SendText(const char* data)
{
	int size = int(strlen(data));
	return SendBinary(data, size);
}

int SerialPort::SendBinary(const char* data, int size)
{
	DWORD bytesToWrite = size;
	DWORD bytesWritten;

	BOOL status = WriteFile(m_port, data, bytesToWrite, &bytesWritten, NULL);
	if (status == FALSE) return -1;

	return bytesWritten;
}

int SerialPort::ReciveText(char* buffer, int size)
{
	DWORD eventMask;
	DWORD bytesRead;

	BOOL status = WaitCommEvent(m_port, &eventMask, NULL);
	if (status == FALSE) return FALSE;
	
	status = ReadFile(m_port, buffer, size, &bytesRead, NULL);
	if (status == FALSE) return -1;
	
	buffer[bytesRead] = 0;
	return bytesRead;
}
