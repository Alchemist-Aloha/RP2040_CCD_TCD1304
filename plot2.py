import matplotlib.pyplot as plt
import matplotlib.animation as animation
import serial  # To read data from serial port
import numpy as np
import threading

# Assuming capture depth is known
CAPTURE_DEPTH = 3694  # Or use the value from your C code
capture_buf = []

# Reading data from serial (replace with your COM port and baudrate)
ser = serial.Serial('COM23', 115200, timeout=1)

# Function to read capture_buf data from the serial port
def read_capture_buf():
    global capture_buf
    while True:
        line = ser.readline().decode('utf-8').strip()
        if line == "Capture finished":
            print("start record")
            break
    while True:
        line = ser.readline().decode('utf-8').strip()
        if line == "Capture finished":
            print("end record")
            break
        try:
            values = line.split(",")  # Splitting values by commas
            capture_buf += [int(val) for val in values if val.strip()]
        except ValueError:
            pass

# Function to continuously read data in a separate thread
def continuous_read():
    while True:
        read_capture_buf()

# Plotting function
def plot_spectrum(i):
    global capture_buf
    if not capture_buf:
        return

    # Create the x-axis (index) for plotting
    x_axis = np.arange(len(capture_buf))
    
    plt.cla()
    plt.plot(x_axis, capture_buf, label='ADC Values')
    plt.xlabel('Sample Number')
    plt.ylabel('ADC Value')
    plt.title('Captured Spectrum')
    plt.grid(True)
    plt.legend()

# Main code to read and plot the spectrum
try:
    # Start the continuous reading in a separate thread
    read_thread = threading.Thread(target=continuous_read)
    read_thread.daemon = True
    read_thread.start()

    # Set up the plot
    fig = plt.figure(figsize=(10, 6))
    ani = animation.FuncAnimation(fig, plot_spectrum, interval=1000)
    plt.show()

except KeyboardInterrupt:
    print("Exiting program...")
finally:
    ser.close()