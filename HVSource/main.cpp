#include "mbed.h"
#include "BufferedSerial.h"
#include "USBSerial.h"
#include <iostream>
#include <stdarg.h>

// The original author is someone (unkown) from the University of Aveiro.
// This code is, however, heavily modified to be adapted to work with both an LCD and Serial COM at the same time.
// Existing code refactoring was also heavily performed.
// Author : César Godinho - June 2021 - Licensed under the LGPL

// Firmware features:
// - DAC Voltage Ramp
// - Target V
// - Target Max I
// - Display Target V and Target I
// - Display Real V and Real I
// - Get/Set all values via Serial COM.
// - Get error messages via Serial COM.
// - Enable/Disable sources via Serial COM.

// Serial Commands Formating : sCCv\n 
// (s = HV source numbered in the back [1,2], CC = Two commands characters (see command table below), v = command specific value) 

// Examples 
// Powering HV Source 1 On                   - 1PO1\n
// Setting max current on Source 2 to 150 uA - 2SI150.0\n
// Setting target voltage on Source 2 to 2kV - 2SV2000\n
// Get last error string                     - 1EE0\n

void wait(float v)
{
    ThisThread::sleep_for(std::chrono::milliseconds((long)(v * 1000)));
}

void wait_ms(float v)
{
    ThisThread::sleep_for(std::chrono::milliseconds((long)(v)));
}

#define SERIAL_STOP_BYTE '\r'

#define VSERIAL

#ifndef VSERIAL
BufferedSerial pc_serial(USBTX, USBRX);
#else
USBSerial pc_serial;
#endif
BufferedSerial lcd(PB_10,PC_5);
SPI spi(PA_7, PA_6, PA_5); // mosi, miso, sclk

DigitalOut cs(PB_5);

DigitalOut en2(PA_3);
DigitalOut en1(PA_9);

AnalogIn val1(PA_0);
AnalogIn val2(PA_1);
AnalogIn val3(PC_1);
AnalogIn val4(PC_0);

char last_error[256];

float dac_value1 = 0.0f;
float dac_value2 = 0.0f;
float dac_value3 = 0.0f;
float dac_value4 = 0.0f;

float ref_imon_value1 = 0.0f;
float ref_vmon_value1 = 0.0f;
float ref_imon_value2 = 0.0f;
float ref_vmon_value2 = 0.0f;

const float SlopeRiseTime = 1000.0f;

bool isSloped1 = false;
bool isSloped2 = false;

class RGBLed
{
public:
    RGBLed(PinName redpin, PinName greenpin, PinName bluepin);
    void write(float red,float green, float blue);
private:
    PwmOut _redpin;
    PwmOut _greenpin;
    PwmOut _bluepin;
};

RGBLed::RGBLed (PinName redpin, PinName greenpin, PinName bluepin)
    : _redpin(redpin), _greenpin(greenpin), _bluepin(bluepin)
{
    //50Hz PWM clock default a bit too low, go to 2000Hz (less flicker)
    _redpin.period(0.0005);
}

void RGBLed::write(float red,float green, float blue)
{
    _redpin = red;
    _greenpin = green;
    _bluepin = blue;
}

//Setup RGB led using PWM pins and class
RGBLed myRGBled(PB_9,PB_8,PB_6); //RGB PWM pins

const char DAC_addr[4] = { 0b00110000, 0b01110000, 0b10110000, 0b11110000 };
const short int Vref = 3300; // 3.3V given by arduino
const short int DAC_res = 4096;

// Calculation of numeric value of the binary input code
const float kratio = (float)DAC_res / Vref;

void set_dac_ref(float ref, char dac)
{
    int v = dac - 'A';
    short int value = kratio * ref;
    char lowbyte = value & 0x00FF;                         // Mask out lower 8 bits
    char highbyte = ((value >> 8) & 0x00FF) | DAC_addr[v]; // Shift upper 8 bits and mask
    cs = 0;
    spi.write(highbyte);
    spi.write(lowbyte);
    cs = 1;
    wait_ms(10);
}

// This is from legacy code (UA Student) and should not be used. Unless for testing purposes only, or when disabling.
// Use the dac voltage ramp mode instead.
void dac(float ref1, float ref2, float ref3, float ref4)
{
    //////////////////////////////////////////////////
    // Provide reference voltage to comparators (DAC):

    // DAC Reset High, Lock low
    //     digitalWrite(RESET,HIGH);  // HIGH - não faz reset; LOW - põe todos os registos internos a 0V.
    //     digitalWrite(LOCK, LOW);
    set_dac_ref(ref1, 'A');
    set_dac_ref(ref2, 'B');
    set_dac_ref(ref3, 'C');
    set_dac_ref(ref4, 'D');
}

void set_dac_current(float i1, float i2)
{
    set_dac_ref(i1, 'A');
    set_dac_ref(i2, 'C');
}

void set_dac_voltage_sloped(float v, int source, float T)
{
    static const char S[2] = { 'B', 'D' };
    float lv = 0.0f;
    
    const int steps = 500;
    float t_step = T / steps;
    float x_step = v / steps;
    
    char dacID = S[source - 1];
    
    if(source == 1)
    {
        isSloped1 = true;
    }
    else if(source == 2)
    {
        isSloped2 = true;
    }
    
    for(int i = 0; i < steps; i++)
    {
        lv += x_step;
        set_dac_ref(lv, dacID);
        wait_ms(t_step);
    }
}

bool checkV(float ef)
{
    return ef < 2400.0f && ef >= 0.0f;
}

bool checkI(float ef)
{
    return ef < 500.0f && ef >= 0.0f;
}

// TODO : Use callibrations for both power supply models instead
float convertV(float valorV)
{
    // 27 : 0.0009094 * valorV - 0.004561
    // 28 : 0.0009071 * valorV - 0.002949
    return 0.0009095 * (valorV * 1000) - 0.003413;
}

// TODO : Use callibrations for both power supply models instead
float convertVmon(float Vmon)
{
    // 27 : 1096.500 * Vmon + 1.081400
    // 28 : 1099.000 * Vmon + 1.025200
    return 1096.475 * Vmon + 0.721925;
}

float convertI(float ab)
{
    return (ab * 1000) / 253;
}

float averageI(AnalogIn pl)
{
    float pk = 0.0;
    for (int i = 0; i < 100; i++) {
        pk += pl * 3.3f;
    }
    return (pk / 100) * 253;
}

float averageV(AnalogIn ph)
{
    float pj = 0.0;
    for (int i = 0; i < 100; i++) {
        pj += ph * 3.3f;
    }
    return pj / 100.0f;
}

bool checkImax(float hg, float hf)
{
    if (hg > (hf - 10)) 
    {
        strcpy(last_error, "Max current exceded. Disabling HV sources...");
        
        myRGBled.write(1.0, 0.0, 0.0);
        en1 = 0;
        en2 = 0;
        
        cs = 0;
        dac(0, 0, 0, 0);
        cs = 1;
        
        return true;
    }
    else 
    {
        return false;
    }
}

void final()
{
    char data = 0xFF;
    lcd.write(&data, 1);
    lcd.write(&data, 1);
    lcd.write(&data, 1);
}

void updateLCDRealValues(int v1, int v2, float i1, float i2)
{
    char data[128];
    sprintf(data, "page0.v1r.val=%d", v1);
    lcd.write(data, strlen(data)); final();
    //lcd.printf("page0.v1r.val=%d", v1);

    sprintf(data, "page0.v2r.val=%d", v2);
    lcd.write(data, strlen(data)); final();
    //lcd.printf("page0.v2r.val=%d", v2);

    
    sprintf(data, "page0.i1r.val=%d", (int)(i1 * 100));
    lcd.write(data, strlen(data)); final();
    //lcd.printf("page0.i1r.val=%d", (int)(i1 * 100));

    sprintf(data, "page0.i2r.val=%d", (int)(i2 * 100));
    lcd.write(data, strlen(data)); final();
    //lcd.printf("page0.i2r.val=%d", (int)(i2 * 100));
}

#define LCD_TARGET_KEEP -100

void updateLCDTargetValues(int v1, int v2, float i1, float i2)
{
    char data[128];
    if(v1 > -1) { sprintf(data, "page0.v1t.val=%d", v1); lcd.write(&data, strlen(data)); final(); }
    if(v2 > -1) { sprintf(data, "page0.v2t.val=%d", v2); lcd.write(&data, strlen(data)); final(); }
    
    if(i1 > -1) { sprintf(data, "page0.i1t.val=%d", (int)(i1 * 100)); lcd.write(&data, strlen(data)); final(); }
    if(i2 > -1) { sprintf(data, "page0.i2t.val=%d", (int)(i2 * 100)); lcd.write(&data, strlen(data)); final(); }
}

int convertSource(uint8_t m)
{
    if(m == '1' || m == '2')
    {
        return (int)m - 48;
    }
    else
    {
        return 0;
    }
}

float getValue()
{
    char s[32];
    uint8_t v = pc_serial._getc();
    
    if(v == '?')
        return -1;
    
    int i = 0;
    
    while(v != SERIAL_STOP_BYTE)
    {
        s[i++] = v;
        v = pc_serial._getc();
    }
    
    s[i] = '\0';
    return atof(s);
}

void powerOnOff(int source, int value)
{
    if(value >= 0)
    {
        switch(source)
        {
            case 1:
                en1 = value;
                if(en1 && !isSloped1 && dac_value1 > 0.0f) set_dac_voltage_sloped(dac_value1, 1, SlopeRiseTime);
                isSloped1 = false;
                break;
            case 2:
                en2 = value;
                if(en2 && !isSloped2 && dac_value3 > 0.0f) set_dac_voltage_sloped(dac_value3, 2, SlopeRiseTime);
                isSloped2 = false;
                break;
            default:
                break;
        }
        
        if(en1 || en2)
        {
            myRGBled.write(0.0, 0.0, 1.0); // blue
        }
        else
        {
            myRGBled.write(1.0, 0.0, 0.0); // red
        }
    }
    else
    {
        switch(source)
        {
            case 1:
            {
                char data[128];
                sprintf(data, "%d\r\n", en1.read());
                pc_serial.write(data, strlen(data));
            }
            break;
            case 2:
            {
                char data[128];
                sprintf(data, "%d\r\n", en2.read());
                pc_serial.write(data, strlen(data));
            }
            break;
            default:
                break;
        }
    }
}

void setVoltage(int source, int value)
{
    if(value >= 0)
    {
        if(!checkV(value))
        {
            sprintf(last_error, "Desired voltage (%d V) too high (max 2400 V).", value);
            return;
        }
        switch(source)
        {
            case 1:
                dac_value1 = convertV(value);
                updateLCDTargetValues(value, LCD_TARGET_KEEP, LCD_TARGET_KEEP, LCD_TARGET_KEEP);
                wait_ms(10);
                if(en1) set_dac_voltage_sloped(dac_value1, 1, SlopeRiseTime);
                break;
            case 2:
                dac_value3 = convertV(value);
                updateLCDTargetValues(LCD_TARGET_KEEP, value, LCD_TARGET_KEEP, LCD_TARGET_KEEP);
                wait_ms(10);
                if(en2) set_dac_voltage_sloped(dac_value3, 2, SlopeRiseTime);
                break;
            default:
                break;
        }
    }
    else
    {
        switch(source)
        {
            case 1:
            {
                char data[128];
                sprintf(data, "%.2f\r\n", convertVmon(averageV(val2)));
                pc_serial.write(data, strlen(data));
            }
            break;
            case 2:
            {
                char data[128];
                sprintf(data, "%.2f\r\n", convertVmon(averageV(val4)));
                pc_serial.write(data, strlen(data));
            }
            break;
            default:
                break;
        }
    }
}

void setCurrent(int source, float value)
{
    if(value >= 0)
    {
        if(!checkI(value))
        {
            sprintf(last_error, "Desired current (%.2f uA) too high (max 500 uA).", value);
            return;
        }
        switch(source)
        {
            case 1:
                dac_value2 = convertI(value);
                updateLCDTargetValues(LCD_TARGET_KEEP, LCD_TARGET_KEEP, value, LCD_TARGET_KEEP);
                wait_ms(10);
                break;
            case 2:
                dac_value4 = convertI(value);
                updateLCDTargetValues(LCD_TARGET_KEEP, LCD_TARGET_KEEP, LCD_TARGET_KEEP, value);
                wait_ms(10);
                break;
            default:
                break;
        }
        
        set_dac_current(dac_value2, dac_value4);
    }
    else
    {
        switch(source)
        {
            case 1:
            {
                char data[128];
                sprintf(data, "%.2f\r\n", averageI(val1));
                pc_serial.write(data, strlen(data));
            }
            break;
            case 2:
            {
                char data[128];
                sprintf(data, "%.2f\r\n", averageI(val3));
                pc_serial.write(data, strlen(data));
            }
            break;
            default:
                break;
        }
    }
}

void getLastError()
{
    char data[256];
    sprintf(data, "Last Error: %s\r\n", last_error);
    pc_serial.write(data, strlen(data));
}

void serialCB()
{
    static int source = 0;
    static uint16_t cmd = 0;
    static uint8_t state = 0;

    if(state == 0 && pc_serial.readable())
    {
        source = convertSource(pc_serial._getc());
        ++state;
    }

    if(state == 1 && pc_serial.readable())
    {
        cmd  = pc_serial._getc() << 8;
        cmd |= pc_serial._getc();
        ++state;
    }
    
    if(state == 2 && pc_serial.readable())
    {
        float value = getValue(); // WARNING : This blocks!
        pc_serial.printf("Got cmd (%d_%#04x_%f).\n\r", source, cmd, value);
        state = 0;

        if(source == 0)
        {
            strcpy(last_error, "Command error. Specified source not available. Possible values [1, 2].");
            return;
        }
        
        switch(cmd)
        {
            case 0x504F: // PO - Set/Get Power On/Off
                powerOnOff(source, (int)value);
                break;
            case 0x5356: // SV - Set/Get Voltage
                setVoltage(source, (int)value);
                break;
            case 0x5349: // SI - Set/Get Current
                setCurrent(source, value);
                break;
            case 0x4545: // EE - Get Last Error String
                getLastError();
                break;
            default:
                sprintf(last_error, "Comand [%c%c] not recognized.", (char)(cmd >> 8 & 0xFF), (char)(cmd & 0xFF));
                break;
        }
    }
}

void updateLCD()
{
    cs = 0;
        
    float ref_imon_value1 = averageI(val1);
    float ref_vmon_value1 = convertVmon(averageV(val2));
    float ref_imon_value2 = averageI(val3);
    float ref_vmon_value2 = convertVmon(averageV(val4));
    
    // NOTE : Or use an interrupt
    updateLCDRealValues((int)ref_vmon_value1, (int)ref_vmon_value2, ref_imon_value1, ref_imon_value2);

    if (en1)
    {
        checkImax(ref_imon_value1, dac_value2);
    }

    if (en2)
    {
        checkImax(ref_imon_value2, dac_value4);
    }
}

void startSignal()
{
    myRGBled.write(0.0f, 0.0f, 1.0f);
    wait(0.15);
    myRGBled.write(0.0f, 0.0f, 0.0f);
    wait(0.15);
    myRGBled.write(0.0f, 1.0f, 0.0f);
    wait(0.15);
    myRGBled.write(0.0f, 0.0f, 0.0f);
    wait(0.15);
    myRGBled.write(1.0f, 0.0f, 0.0f);
    wait(0.15);
    myRGBled.write(0.0f, 0.0f, 0.0f);
}

int main()
{
    #ifndef VSERIAL
    pc_serial.set_baud(9600);
    pc_serial.set_format(
        /* bits */ 8,
        /* parity */ BufferedSerial::None,
        /* stop bit */ 1
    );
    #endif

    startSignal();

    cs = 1;
    en1 = 0;
    en2 = 0;
    spi.format(8, 0);
    spi.frequency(1000000);
    
    strcpy(last_error, "No error.");
    
    while(true) 
    {
        serialCB();
        updateLCD();
        //wait(0.1);
    }
}


// class RGBLed
// {
// public:
//     RGBLed(PinName redpin, PinName greenpin, PinName bluepin);
//     void write(float red,float green, float blue);
// private:
//     PwmOut _redpin;
//     PwmOut _greenpin;
//     PwmOut _bluepin;
// };

// RGBLed::RGBLed (PinName redpin, PinName greenpin, PinName bluepin)
//     : _redpin(redpin), _greenpin(greenpin), _bluepin(bluepin)
// {
//     //50Hz PWM clock default a bit too low, go to 2000Hz (less flicker)
//     _redpin.period(0.0005);
// }

// void RGBLed::write(float red,float green, float blue)
// {
//     _redpin = red;
//     _greenpin = green;
//     _bluepin = blue;
// }

// //Setup RGB led using PWM pins and class
// RGBLed myRGBled(PB_9,PB_8,PB_6); //RGB PWM pins

// int main()
// {
//     USBSerial pc_serial;
//     float red = 1.0f;

//     myRGBled.write(0.0f, 1.0f, 0.0f);

//     ThisThread::sleep_for(500ms);

//     myRGBled.write(0.0f, 0.0f, 0.0f);

//     // In the end my program only accesses these functions of the serial
//     while(true)
//     {
//         pc_serial.printf("%c\r\n", pc_serial._getc());
//         red = 1.0f - red;
//         myRGBled.write(red, 0.0f, 0.0f);
//         //ThisThread::sleep_for(500ms);
//     }
// }
