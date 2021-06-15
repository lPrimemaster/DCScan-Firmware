# DCScan-Firmware
This repository contains firmware for various hardware modules present in the DCScan system.

# Modules
| Module | Firmware Developed | Description |
|:-:|:-:|:-:|
| [HV Sources Controller](#HV-Sources-Controller) | :heavy_check_mark: | Controls 2 HV sources that will power the detectors. |
| [Peltier Controller](#Peltier-Controller) | :hourglass: | Controls 2 peltier devices used to cool down the refractoring crystals. |

## HV Sources Controller
Firmware responsible for running the HV Power source. These are controlled using a STM32F446RE mcu on a custom board designed by the Physics department at the [Aveiro University](https://www.ua.pt/pt/fis/). The firmware allows to control the voltage rising time, max voltage and current provided by the sources, and live LCD display. All controlled via Serial COM.

Sadly, compiling this firware is only possible online, since the Mbed2 command line interface is deprecated and is officially broken. It would be possible to "hack" a way around it, however portability would suffer. So, instead, using the online compiler was chosen, for all the firmwares that use mbed-os, in order to simplify the building process of all of them.

## Peltier Controller
Firmware responsible for running the peltier's PID and 7-segment displays. These are controlled using a NUCLEO-F401RE board from [ST](https://st.com). The firmware allows to control only a target temperature for each peltier module for now. Maximum cooling power is about 30 W per module, for a total of shy of 60 watts.

## Compiling
- To compile the source under the `HVsource` or the `Peltier` directory, create a mbed account at https://mbed.com/.
- After creating an account access the online compiler tab under https://ide.mbed.com/.
- Then select import on the toolbar, and click import from url. As the url, choose this repository, https://github.com/lPrimemaster/DCScan-Firmware.
- Afterwards, erase the folders of the modules you do not wish to compile. Compiling all modules needs to be done separatly, as the online compiler does not support multiple outputs at the same time.
- If all was done correctly, hit `Compile` at the toolbar and a `.bin` should be prompted for saving after the compilation ends.


This file contains all the bytecode to be flashed to the mcu in question.

## Flashing the MCU's
### HV Sources Controller
TODO

### Peltier Controller
TODO

