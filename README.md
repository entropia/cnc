# CNC Mill - Diadrive 2000

The entropia got a CNC mill lent, it is a "Diadrive 2000".

This repository contains information on how to setup the mill with a LinuxCNC system and some other QoL tooling.

## G-Code optimizer

`optimize` contains a G-Code optimizer. See [README](./optimize/README).

## Eagle G-Code generator

`pcb2gcode` contains tooling to get a compatible G-Code file from eagle to mill printed circuit boards.

## Mill driver & LinuxCNC setup

`linuxcnc` contains an additional driver required for the mill and some LinuxCNC config foo. Checkout [the README.md](./linuxcnc/README.md) in the folder for further instructions.