# DCScan-Firmware

This repository contains firmware for the various hardware modules present in the DCScan system.

## Modules
| Module | Firmware Developed | Target MCU | RTOS | Description |
|:-:|:-:|:-:|:-:|:-:|
| [HV Sources Controller](#HV-Sources-Controller) | :heavy_check_mark: | STM32F446RE | MBed OS 6 | Controls 2 HV sources that will power the detectors. |
| [Peltier Controller](#Peltier-Controller) | :hourglass: | NUCLEO_F401RE |  MBed OS 6 | Controls 2 peltier devices used to cool down the refractoring crystals. |

## HV Sources Controller
Firmware responsible for running the dual HV power sources that feed the photodiodes. These are controlled using a STM32F446RE MCU on a custom board designed by the Physics department at the [Aveiro University](https://www.ua.pt/pt/fis/). The firmware allows to control the voltage rising time (ramp steepness), max voltage and current provided by the sources, and live monitoring via a LCD display. All controlled via Serial COM, with a set of defined commands.

### Commands

Syntax: `nCCv<eoc byte>`
- `n`  - Choose which HV source (1 or 2) to adress with the command. [1 byte]
- `CC` - The command to perform (view table below for valid commands). [2 bytes]
- `v`  - The value sent to the controller (command specific). [x bytes]
- `<eoc byte>` - The byte to signal communication end. [default: `\r`]


| Command | Description | `n` type | `v` type | Return type (if asked) | Example |
|:-:|:-:|:-:|:-:|:-:|:-:|
| PO | Switches power supply `n` on/off. | `int` `1` or `2` | `int` `0` - off<br/>`int` `1` - on<br/>`char` `?` - get status | `int` - `0` or `1` | Set source 1 on - `1PO1\r`<br/>Ask source 2 status - `2PO?\r` |
| SV | Set/Get power supply `n` target voltage. | `int` `1` or `2` | `int` `0` to `2400` - set voltage<br/>`char` `?` - get voltage | `int` - `0` to `2400` | Set source 1 target voltage to 1200V - `1SV1200\r`<br/>Ask source 2 current target voltage - `2SV?\r` |
| SI | Set/Get power supply `n` target current. | `int` `1` or `2` | `float` `0` to `500` - set current<br/>`char` `?` - get current | `float` - `0` to `500` | Set source 1 target current to 3.50uA - `1SV3.5\r`<br/>Ask source 2 current target current - `2SI?\r` |
| EE | Get last global error. | Don't care | Don't care | `char[]` - string with last error description | Get last error - `0EE0\r` or `nEEx\r`<br/>for any `n` `int` and any `x` `float/int` |


## Peltier Controller
Firmware responsible for running the two peltier's PID and 7-segment displays. These are controlled using a NUCLEO-F401RE board from [ST](https://st.com). The firmware allows to control only a target temperature for each peltier module for now. Maximum cooling power is about 30 watts per module, for an approximate total of 60 watts.

### Commands

:warning: Commands for this device are not yet available (WIP).

## Compiling

### Automatic install
Compiling any firmware that runs MBed ARM is fairly easy. Download and install the MBed CLI 1 ([download](https://os.mbed.com/docs/mbed-os/v6.11/build-tools/install-and-set-up.html)).

### Manual install
If a manual instalation is prefered, install all dependencies below.

Dependencies:
- Python 3.7.x or higher
- MBed CLI 1 (via `pip install mbed-cli`)
- GCC ARM v9 (any other version might work, v9 was tested and is working properly - [download](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads/9-2019-q4-major))
- Git 1.9.5 or higher
- Mercurial 2.2.2 or higher

> :warning: Ensure that the GCC ARM binary is in the current `PATH`.

---

Now that mbed-cli is properly installed let's go step by step through the compilation commands.

- Navigate to the folder containing the module you wish to modify/compile.
- Open console (or powershell) and type `mbed compile -t GCC_ARM -m <mcu target name>`.
- After it's completed a `BUILD` folder should have been created. Inside `BUILD/<mcu target name>/GCC_ARM/` should now be a file with a `.bin` extension. This file contains all the bytecode to be flashed to the MCU in question.

## Flashing
### HV Sources Controller
1. Connect the ST-Link SWD pins to the correspondig pins inside the controller chassis. These pins are located near the USB connector, on top (or below) of the reset button.
2. Connect the mini USB connector both to the PC and any ST-Link compatible connector.
3. After this, follow step two under the [Flashing, Peltier Controller section](#peltier-controller-1).

Visit the STM boards manual (which come with and ST-Link module) [here](https://www.st.com/resource/en/user_manual/dm00105823-stm32-nucleo-64-boards-mb1136-stmicroelectronics.pdf) and follow the instructions of section 6.2.4 on page 18.

### Peltier Controller
1. Connect the mini USB connector both to the PC and the NUCLEO_F401RE ST-Link compatible connector.
2. A new device should appear under windows device discovery. Drag and drop the binary file to flash the MCU. Success or any errors will be identified by the board and displayed on a `.txt` file.

## Usefull documents
- [MBed OS API](https://os.mbed.com/docs/mbed-os/v6.11/apis/index.html)
- [STM32F446xC/E datasheet](https://www.st.com/resource/en/datasheet/stm32f446re.pdf)
- [STM32F401xD/E datasheet](https://www.st.com/resource/en/datasheet/stm32f401re.pdf)

