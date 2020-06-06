# A fan controller

This allows a standard PC to control an external fan with a brushless
ESC.

## Driving the fan

The fan is being controlled from a standard PC fan controller card. This
generates and output in the range 0-12V, which would normally we used to power
a simple analogue fan (where lower voltage goes slower) within the computer.

However, our fan (taken from a Honda Accord) is driven by an external Turnigy
Brushed ESC.  This takes as an input a 50Hz PWD in servo format.  In this case
a write cycle of 1000us corresponds to zero RPM and 2000us corresponds to
maximum RPM (estimated 2500 RPM for this particular fan).

## Reporting the RPM back to the PC

We need to send a signal back to the PC fan controller card, to report on the
RPM being used.  This a frequency modulated (FM) signal - expecting two pulses
per rotation of the fan (known as "tone").  However the lowest frequency that
can be sent is 31Hz (corresponding to 31 / 2 * 60 = 930 RPM).

The PC keeps a rolling average of the frequency data it is sent.  Thus to
report 465 RPM we can alternate reporting zero and 930 RPM.
