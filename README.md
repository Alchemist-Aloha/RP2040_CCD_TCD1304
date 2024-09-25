**Work in progress!!**

This project utilizes the typical drive circuit from the TCD1304 datasheet, excluding the use of the 74HC04 Hex inverter. The RP2040's ADC operates at 500 ksps, capturing data via DMA, which synchronizes with the 2 MHz Master Clock (MC) of the TCD1304. The integration time (Shift Gate cycle) is currently set to 10 µs, while the full 3648-pixel readout time is approximately 8 ms.

To view the captured spectrum, upload the compiled UF2 file from TCD1304.c, and then run plot.py to plot the data received via the USB serial port.

**To do**

Use the second core to do output work

Signal average

Tunable timing

![image](https://github.com/user-attachments/assets/d66bbdf7-fa45-4507-a215-32223e8315aa)
