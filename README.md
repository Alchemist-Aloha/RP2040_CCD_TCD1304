**Work in progress!!**

This project use the typical drive circuit (without HEX inverter) from TCD1304 datasheet. The rp2040's ADC is running at 500 ksps with DMA capture, matching 2 MHz Master Clock (MC) for TCD1304. Integral time (Shift gate cycle) is currently set to 10 us. Read out time is roughly 8 ms. 

Upload compiled uf2 file from TCD1304.c and run plot.py to plot received spectrum from USB serial port. 

![image](https://github.com/user-attachments/assets/d66bbdf7-fa45-4507-a215-32223e8315aa)
