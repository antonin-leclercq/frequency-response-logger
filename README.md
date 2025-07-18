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

In both cases the sampling is done by the internal 12-bit ADC, triggered by a timer. The interval between each trigger varies with the frequency of the test signal.
The conversion results are then copied to RAM using DMA. The average amplitude value is then sent to the computer via Serial communication.
The project uses this [library](https://www.menie.org/georges/embedded/small_printf_source_code.html) to help with formatting.

### Pin configuration

| STM32 Pin | Function |
|-----------|----------|
| PB0       | ADC CH8  |
| PA2       | USART2 TX|
| PA3       | USART2 RX|

### Using FFT

Since the input is strictly real, only an RFFT is necessary. Here the data type is integer, so the RFFT is done by calling `arm_rfft_q15`. The input and output formats depend on the number of input samples and can we found on the [online](https://arm-software.github.io/CMSIS-DSP/v1.14.2/group__RealFFT.html#ga00e615f5db21736ad5b27fb6146f3fc5).
However, since only the variation of amplitude is relevant, the actual format does not matter much. 
For single component signals, the microcontroller is able to find the peak frequency accurately. 
The amplitude however, which is the actual unknown, varies significantly (and seemingly randomly) when slightly varying the frequency (even without a DUT). A windowing function has been applied but without success.
For now, this approach is far from reliable to perform a frequency response. 

### Using RMS

