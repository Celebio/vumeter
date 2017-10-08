# VuMeter

A small project that listens from your microphone and displays a vumeter on your TV.

![VuMeter](https://i.imgur.com/BWHC7o2.jpg)

## Philosophy

The idea is to lose nothing from the input buffer. See [Time waits for nothing](http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing) for further details.

## Installation

- Clone the repository.
- In the directory containing the `Makefile`, run `make`.
- Run `bin/vumeter`

## Tested on

- Mac Book Air Mid-2013
- Raspberry pi 3 with Jabra Speaker 510

The prefered devices are hardcoded in `vumeter.cpp`. It will look for "Jabra SPEAK 510 USB", "Built-in Microphone" and "Built-in Output". You may want to change it to your available devices (it displays all available devices at initialization).

## Third-party libraries

PortAudio
SDL
[The awesome lock-free queue from cameron314](https://github.com/cameron314/readerwriterqueue)
