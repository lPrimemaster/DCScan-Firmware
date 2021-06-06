#include "mbed-os/mbed.h"
#include <vector>

#define SSRight 0
#define SSLeft 1

constexpr uint8_t operator""Hz(unsigned long long f)
{
    int v = (uint8_t)(1000.0f / f);
    return v > 0 ? v : 1000;
}

// No need for a decimal point, unless a finer-tuned value is needed
struct DDPinOut
{
    PinName a_pin;
    PinName b_pin;
    PinName c_pin;
    PinName d_pin;
    PinName e_pin;
    PinName f_pin;
    PinName g_pin;
};

class SSDisplay
{
public:
    SSDisplay(uint8_t ms, uint8_t num_displays, DDPinOut pins, PinName* muxs);
    ~SSDisplay();

    void show(uint8_t display, uint8_t value);
    void start();

private:
    void interrupt();
    void display(uint8_t n, uint8_t d);

private:
    uint8_t ms;
    uint8_t num_displays;
    uint8_t c_digit;
    uint8_t c_number;

    uint8_t* display_data = nullptr;

    // Pins for 7seg
    DigitalOut pin_a;
    DigitalOut pin_b;
    DigitalOut pin_c;
    DigitalOut pin_d;
    DigitalOut pin_e;
    DigitalOut pin_f;
    DigitalOut pin_g;

    // Mux pins for all digits
    std::vector<DigitalOut> muxs;
    
    // TODO : Stop using IRQs and create a thread since the hardware is running a RTOS
    // Controls the mux via irqs
    Ticker ticker;

    // Sets the digits from the segments
    const uint8_t SEG[10] = {
        0x7E, // 0
        0x30, // 1
        0x6D, // 2
        0x79, // 3
        0x33, // 4
        0x5B, // 5
        0x5F, // 6
        0x70, // 7
        0x7F, // 8
        0x7B  // 9
    };
};
