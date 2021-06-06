#include "mbed-os/mbed.h"
#include "SSDisplay.h"

BufferedSerial pc(USBTX, USBRX);

int main()
{
    DDPinOut pout;

    pout.a_pin = D10;
    pout.b_pin = D11;
    pout.c_pin = D2;
    pout.d_pin = D3; // TODO : Use D4 again
    pout.e_pin = D5;
    pout.f_pin = D6;
    pout.g_pin = D7;

    PinName muxs[8] = { D8, D9/*, D12, D13, PC_8, PC_6, PC_5, PC_12*/ };

    SSDisplay display(1000Hz, 1, pout, muxs); // 1ms
    display.show(0, 17);
    //display.show(1, 98);
    display.start();

    ThisThread::sleep_for(std::chrono::milliseconds(1000));

    pc.write("DONE!\n", 7);

    uint8_t c = 0;

    while (true)
    {
        display.show(0, c++);
        ThisThread::sleep_for(std::chrono::milliseconds(1000));
        c %= 100;
    }

    return 0;
}