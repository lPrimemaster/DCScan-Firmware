#include "SSDisplay.h"
#include <Kernel.h>

DigitalOut led(LED1);

SSDisplay::SSDisplay(uint8_t ms, uint8_t num_displays, DDPinOut pins, PinName* muxs)
    : ms(ms), num_displays(num_displays), c_digit(SSRight), c_number(0), 
      pin_a(pins.a_pin),
      pin_b(pins.b_pin),
      pin_c(pins.c_pin),
      pin_d(pins.d_pin),
      pin_e(pins.e_pin),
      pin_f(pins.f_pin),
      pin_g(pins.g_pin)
{
    display_data = new uint8_t[num_displays * 2]; // This is inefficient but is easier (just wastes ram wtv, keep unless we are on a strech)
    
    for(uint8_t i = 0; i < num_displays * 2; i++) // Muxes can be shortened if necessary (no left over pins)
    {
        this->muxs.push_back(muxs[i]);
    }
}

SSDisplay::~SSDisplay()
{
    delete[] display_data;
}

void SSDisplay::show(uint8_t display, uint8_t value)
{
    assert(display < num_displays);
    display_data[2 * display    ] = value % 10;
    display_data[2 * display + 1] = value / 10;
}

void SSDisplay::start()
{
    ticker.attach(callback(this, &SSDisplay::interrupt), std::chrono::milliseconds(ms));
}

void SSDisplay::interrupt()
{
    display(c_number, c_digit);
    bool cond = (c_digit == SSRight);
    c_digit  = cond ? SSLeft : SSRight;
    c_number = cond ? c_number : (c_number + 1) % num_displays;
}

void SSDisplay::display(uint8_t n, uint8_t d)
{
    uint8_t curr_digit = 2 * n + d;

    // Assume all other mux pins are off by design
    muxs[curr_digit].write(1);
    uint8_t dig = display_data[curr_digit];
    uint8_t data = SEG[dig];


    // Negate for common anode config (mcu max sink spec = 80 mA)
    pin_a.write(!((data & 0b01000000) == 0b01000000));
    pin_b.write(!((data & 0b00100000) == 0b00100000));
    pin_c.write(!((data & 0b00010000) == 0b00010000));
    pin_d.write(!((data & 0b00001000) == 0b00001000));
    pin_e.write(!((data & 0b00000100) == 0b00000100));
    pin_f.write(!((data & 0b00000010) == 0b00000010));
    pin_g.write(!((data & 0b00000001) == 0b00000001));

    //int us = (ms * 1000 - timer.read_us());

    //if(us > 0)
    wait_us(ms * 500.0); // Aprox 50% duty cycle (implement all this code in a thread instead)

    muxs[curr_digit].write(0);
}