Motorized and Arduino controlled Filament Winder
===

##  Version 1.01

## Description

Software for an motorized and Arduino controlled DIY filament winder, called "Pastamatic".
A cooperation of Gregor Trierscheid (mechanical parts) and Werner Riemann  (software part 
and side movement concept). The software controlls the filament guide for the layers of the
rewinded filament spool.

The movement for the filament guide is detected by a hall sensor and is independent from the filament
spool drive. Each revolution of the spool results in a movement of 1.75mm, triggert by the hall sensor.

## Operation

After power up, the application starts in setup mode. Red LED is on.

### Setup mode

- hold left or right push button for moving to left or right direction to adjust the endswitches

### Auto mode

- push start/stop button

Press start/stop button. The red LED turns off and the green LED turns on. The filament guid automatically moves
to the left startpoint. The green LED starts flashing. The filement rewinder is ready now and response to pulse 
of the hall sensor.

Press start/stop button at any time you want to stop the auto mode and switch back to setup mode.


Download 3d print files: https://www.printables.com/model/1500320-completely-motorized-filament-respooler-ctd









