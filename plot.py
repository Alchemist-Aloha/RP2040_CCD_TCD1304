import matplotlib.pyplot as plt
import serial  # To read data from serial port
import numpy as np

PORT = 'COM23'  # Replace with your COM port
# Reading data from serial (replace with your COM port and baudrate)


# Function to read capture_buf data from the serial port


def read_capture_buf():
    buf = []
    while True:
        try:
            line = ser.readline().decode('utf-8').strip()
        except:
            print("Error reading from serial port.")
        # print(line)
        if line == "Capture finished":
            print("start record")
            break
    while True:
        try:
            line = ser.readline().decode('utf-8').strip()
        except:
            print("Error reading from serial port.")
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


def plot_spectrum(capture_buf,line):
    if not capture_buf:
        print("No data to plot.")
        return

    # # Create the x-axis (index) for plotting
    # x_axis = np.arange(len(capture_buf))

    # plt.figure(figsize=(10, 6))
    # plt.plot(x_axis, capture_buf, label='ADC Values')
    # plt.xlabel('Sample Number')
    # plt.ylabel('ADC Value')
    # plt.ylim(0,255)
    # plt.title('Captured Spectrum')
    # plt.grid(True)
    # plt.legend()
    # plt.show()
    
    # Update the line with new data and redraw the figure
    line.set_ydata(capture_buf)
    plt.draw()
    plt.pause(0.01)  # Pause briefly to allow the plot to update



# Main code to read and plot the spectrum
try:
    capture_buf = []
    ser = serial.Serial(PORT, 115200, timeout=1)
    # Reading capture buffer from serial
    print("Reading capture buffer data...")
    capture_buf = read_capture_buf()
    ser.close()
    plt.ion()  # Turn on interactive mode
    fig, ax = plt.subplots(figsize=(10, 6))
    x_axis = np.arange(len(capture_buf))
    # Initial plot setup
    line, = ax.plot(x_axis, np.zeros(len(capture_buf)), label='ADC Values')  # Initialize with zeros
    ax.set_xlabel('Sample Number')
    ax.set_ylabel('ADC Value')
    ax.set_ylim(0, 255)
    ax.set_title('Captured Spectrum')
    ax.grid(True)
    ax.legend()
    
    while True:
        capture_buf = []
        ser = serial.Serial(PORT, 115200, timeout=1)
        # Reading capture buffer from serial
        print("Reading capture buffer data...")
        capture_buf = read_capture_buf()

        # Plotting the captured buffer
        if capture_buf:
            print(f"Captured {len(capture_buf)} samples.")
            plot_spectrum(capture_buf,line)

        # Wait before the next read (for simulation purposes)
        # input("Press Enter to capture the next spectrum...")
        ser.close()
except KeyboardInterrupt:
    print("Exiting program...")
finally:
    ser.close()
