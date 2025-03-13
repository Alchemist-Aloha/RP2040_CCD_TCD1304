import matplotlib.pyplot as plt
import serial  # To read data from serial port
import numpy as np

PORT = 'COM10'  # Replace with your COM port

# Function to read capture_buf data from the serial port
def read_capture_buf(ser):
    buf = []
    while True:
        try:
            line = ser.readline().decode('utf-8').strip()
        except Exception as e:
            print(f"Error reading from serial port: {e}")
            continue
        if line == "Capture finished":
            print("start record")
            break
    while True:
        try:
            line = ser.readline().decode('utf-8').strip()
        except Exception as e:
            print(f"Error reading from serial port: {e}")
            continue
        if line == "Capture finished":
            print("end record")
            break
        try:
            values = line.split(",")  # Splitting values by commas
            buf += [int(val) for val in values if val.strip()]
        except ValueError:
            pass
    return buf

# Plotting function
def plot_spectrum(capture_buf, line, ax):
    if not capture_buf:
        print("No data to plot.")
        return

    # Update the line with new data and redraw the figure
    line.set_ydata(capture_buf)
    line.set_xdata(np.arange(len(capture_buf)))  # Update x-axis data
    ax.set_xlim(0, len(capture_buf) - 1)  # Update x-axis limits
    ax.set_ylim(0, max(capture_buf) + 10)  # Update y-axis limits based on data
    plt.draw()
    plt.pause(0.01)  # Pause briefly to allow the plot to update

# Main code to read and plot the spectrum
try:
    ser = serial.Serial(PORT, 115200, timeout=1)
    plt.ion()  # Turn on interactive mode
    fig, ax = plt.subplots(figsize=(10, 6))
    x_axis = np.arange(500)
    # Initial plot setup
    line, = ax.plot(x_axis, np.zeros(500), label='ADC Values')  # Initialize with zeros
    ax.set_xlabel('Sample Number')
    ax.set_ylabel('ADC Value')
    ax.set_ylim(0, 255)
    ax.set_title('Captured Spectrum')
    ax.grid(True)
    ax.legend()

    while True:
        # Reading capture buffer from serial
        print("Reading capture buffer data...")
        capture_buf = read_capture_buf(ser)

        # Plotting the captured buffer
        if capture_buf:
            print(f"Captured {len(capture_buf)} samples.")
            plot_spectrum(capture_buf, line, ax)

except KeyboardInterrupt:
    print("Exiting program...")
finally:
    ser.close()
