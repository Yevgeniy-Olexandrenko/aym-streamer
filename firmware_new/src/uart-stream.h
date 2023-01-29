#pragma once

class SinglePSG;

class uart_stream
{
public:
    void Start(SinglePSG& psg);
    void Stop();
};

extern uart_stream UARTStream;
