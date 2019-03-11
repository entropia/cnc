spigen
======

This is a HAL component for LinuxCNC that can generate SPI signals on other pins.

Note: Although it follows the HAL component numbering convention, this component only supports a single instance at the moment.

Options
-------

* *wordsize*: The number of bits to send per SPI transfer (must be less than or equal to 32, default: 16)
* *retransmit_count*: The number of times to retransmit a value for safety (default: 3)
* *clock_hz*: The SPI clock frequency in Hz (default: 16000)
* *min_pause*: Minimal pause between transmissions in ns (default: 800000)

Pins
----

Inputs:
* *spigen.0.enable* (bit): Whether the SPI output should be enabled or not
* *spigen.0.value* (u32): The value to be sent

Outputs:
* *spigen.0.cs* (bit): The SPI chip-select/enable line
* *spigen.0.clock* (bit): The SPI clock
* *spigen.0.data* (bit): The SPI MOSI line

Behavior
--------

The only SPI mode implemented is the following:
* CS is active low
* The leading edge of a clock pulse is rising
* Data is sampled on the leading edge of a clock pulse, setup occurs during the trailing edge

If enabled, data is sent out on every change of the *value* input and also on disabled→enabled-transitions.

If the component is not called frequently enough to maintain the configured clock frequency, it uses the highest frequency currently possible.


Compiling/Installing
--------------------
Use *halcompile*, which is kindly provided by LinuxCNC, to compile and install the component. (See also *man 1 halcompile* and http://www.linuxcnc.org/docs/html/hal/comp.html#_compiling)
