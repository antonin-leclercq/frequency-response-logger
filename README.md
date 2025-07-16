# frequency-response-logger

## Overview
![diagram](./diagram.PNG)

The PC sends LXI commands to the signal generator to obtain a controlled frequency sweep. 

This signal is fed to the Device Under Test and then sampled by the STM32 microcontroller.

The microcontroller runs some algorithm to determine the signal amplitude and sends it back to the PC, which sends a new LXI command and so on.

## pc-code

From the PC console application, it is possible to modify several settings:
- settings likes signal amplitude, frequency range, number of points, etc
- settings like the generator's IP address or the microcontroller's serial port

In this example, the signal generator is a Rigol DS1022Z, and on Windows, you will need to install the Rigol drivers and NI VISA library.

## STM32

Two different approaches are considered on the microcontroller side:
- in `./using-fft`, the ARM DSP library is used to first do an FFT on the samples to obtain the amplitude
- in `./using-rms`, the microcontroller does an averaging of the samples to obtain the amplitude

### Using FFT

### Using RMS