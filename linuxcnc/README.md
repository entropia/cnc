# LinuxCNC

LinuxCNC is a real time operating system that can directly interface with the mills parallel port. The mill itself does not provide any controller.

`hal` contains a custom driver to speak with the mills spindel.

`config` contains a dump of the initial LinuxCNC configuration files we confirmed to be working.

Some more (unsorted) information in german can be found in the [wiki of entropia](https://entropia.de/CNC-Fr%C3%A4se).

## Setting up LinuxCNC

⚠️ The mill requires a parallel port to talk to LinuxCNC! USB-Adapters most likely will not work.

### 1. Installing the OS

1. Download the LinuxCNC ISO from the official webpage [linuxcnc.org/downloads/](https://linuxcnc.org/downloads/). (Last version confirmed working: `2.8.4`)
2. Create a bootable USB stick from the ISO.
3. Boot from the stick and setup LinuxCNC (it is a modified debian).

### 2. Stepconfig

Now we need to setup the stepconfig, some basic settings for LinuxCNC to get to know the mill.

1. Open the "Stepconf Wizard" (hidden in the LinuxCNC menu). 
2. Follow the wizard, you can copy the values from [diadrice2000.stepconf](./config/diadrive2000.stepconf).

Pin assignments are as follows:

| PIN   | Functionality                                      |
| ----- | -------------------------------------------------- |
| 1     | unused                                             |
| 2     | x step                                             |
| 3     | x direction                                        |
| 4     | y step                                             |
| 5     | y direction                                        |
| 6     | z step                                             |
| 7     | z direction                                        |
| 8     | spindle on/off                                     |
| 9     | coolant (air in our case, controls the compressor) |
| 10    | minimum + limit x                                  |
| 11    | maximum + limit z                                  |
| 12    | minimum + limit y                                  |
| 13    | unused                                             |
| 14    | unused (custom driver uses this as clock)          |
| 15    | unused                                             |
| 16    | unused (custom driver uses this as data)           |
| 17    | unused (custom driver uses this as select)         |
| 18-26 | unused/GND                                         |

⚠️ The x axis must be inverted, you can simply tick the "invert" checkbox for the corresponding pin. 

### 3. Install custom driver

LinuxCNC can talk natively to the axis and endstops, though the spindle is controlled via a SPI interface which LinuxCNC is not capable of. Therefore, we need to load a custom driver into LinuxCNC.

1. Checkout this repository on the LinuxCNC machine.
2. Open a terminal and navigate into `linuxcnc/hal`. Execute the following command to compile and load the driver: 

```
halcompile --install spigen.c
```
3. Now we need to wire up the custom driver with data from LinuxCNC, open `/home/linuxcnc/configs/diadrive2000/diadrive2000.hal` (might be named differently depending on how you did name the mill in LinuxCNC).
4. Copy the blocks with a `# SPI spindle` comment from this repositories [diadrive2000.hal](./config/diadrive2000/diadrive2000.hal) to the opened file. This will ensure data will be send to our custom driver and from there to the parallel port pins.

### 4. Setting home position

1. Open the `/home/linuxcnc/configs/diadrive2000/diadrive2000.ini` file and add the `HOME_OFFSET` setting as done in this repositories [diadrive2000.ini](./config/diadrive2000/diadrive2000.ini) file. This will ensure the mill will set the home position 5 mm away from the endswitches, avoiding triggering them under normal operation (and therefore bringing the mill to a stop).