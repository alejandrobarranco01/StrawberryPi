#!/usr/bin/python3

import subprocess
import time
import mysql.connector
import drivers
import socket
from datetime import datetime
import threading

display = drivers.Lcd()


# Create socket
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Define the server address and port
server_socket.bind(('127.0.0.1', 8080))

# Start listening for connections
server_socket.listen(1)
print("Waiting for connection...")

# Accept incoming connection
client_socket, client_address = server_socket.accept()
print(f"Connection established with {client_address}")

try:
    db_conn = mysql.connector.connect(
        host="localhost",
        user="root",
        password="root",
        database="strawberrypi"
    )
    print("Connection successful!")
    cursor = db_conn.cursor()
except mysql.connector.Error as err:
    print(f"Error: {err}")


# Function to parse and extract temperature and humidity from the received data
def parse_data(data):
    try:
        if data == "ERROR":
            return None, None, None
        # Assuming data is in the format "temperature humidity% light"
        temp_str, humi_str, light_str = data.split()  # Split by space
        temperature = float(temp_str)  # Convert temperature to float
        humidity = float(humi_str.replace('%', ''))  # Remove '%' and convert humidity to float
        light_levels = int(light_str)
        return temperature, humidity, light_levels
    except Exception as e:
        print("Error parsing data:", data)
        return None, None, None


def fetch_weekly_trends():
    query = """
    SELECT 
        AVG(temperature) AS avg_temp,
        MIN(temperature) AS min_temp,
        MAX(temperature) AS max_temp,
        AVG(humidity) AS avg_humidity,
        MIN(humidity) AS min_humidity,
        MAX(humidity) AS max_humidity,
        AVG(light) AS avg_light,
        MIN(light) AS min_light,
        MAX(light) AS max_light
    FROM SensorData
    WHERE timestamp >= NOW() - INTERVAL 7 DAY
    """
    
    cursor.execute(query)

    avg_temp, min_temp, max_temp, avg_humidity, min_humidity, max_humidity, avg_light, min_light, max_light = cursor.fetchone()  # Fetch the result of the query

    # Prepare the display data
    display_data = (
        f"Avg Temp: {avg_temp:.1f} C        "
        f"Min Temp: {min_temp:.1f} C        Max Temp: {max_temp:.1f}C        "                
        f"Avg Humidity: {avg_humidity:.1f}%        "
        f"Min Humidity: {min_humidity:.1f}%        Max Humidity: {max_humidity:.1f}%        "
        f"Avg Light: {avg_light}        "
        f"Min Light: {min_light}        Max Light: {max_light}        "
    )

    # Display data on the LCD
    display.lcd_clear()
    display.scroll_text(f"{display_data}", 0.5)

# Watchdog timer (run python lcd.py and ./main commands if an issue is detected)
def watchdog():
    while True:
        time.sleep(10)  # Check every 10 seconds

        # Here you can define your condition to check if things are going wrong.
        # For simplicity, I'm just running the reset command if any error occurs.
        try:
            # Run the commands from the specified directory
            hi = 1
            
        except subprocess.CalledProcessError as e:
            print(f"Error running commands: {e}")
            reset_system()

def reset_system():
    # Reset all critical components by running the necessary commands again
    print("Resetting system by re-running the necessary commands.")

    display.lcd_clear()  # Clear the LCD display when the program ends
    client_socket.close() 
    server_socket.close()
    cursor.close()
    db_conn.close()
    
    try:
        # Re-run the required commands from the specified directory
        subprocess.run(['python3', 'lcd.py'], check=True, cwd=working_directory)
        subprocess.run(['./main'], check=True, cwd=working_directory)
        print("System successfully reset.")
    except subprocess.CalledProcessError as e:
        print(f"Error during system reset: {e}")

# Start watchdog thread
watchdog_thread = threading.Thread(target=watchdog, daemon=True)
watchdog_thread.start()

try:
    print("Listening for incoming messages...")
    while True:
        # Receive the message from the client (blocks until data is received)
        data = client_socket.recv(1024).decode('utf-8')

        if data:
            display.lcd_backlight(1)

            temperature, humidity, light_level = parse_data(data)  # Parse the data into temperature and humidity
            
            if temperature is not None and humidity is not None:
                temperature_F = (temperature * 9/5) + 32
                 # Get the current time
                current_time = datetime.now().strftime("%B %d, %Y %I:%M %p") 

                # Print temperature, humidity, and time
                print(f"Time: {current_time}")
                print(f"Temp: {temperature:.1f} C")
                print(f"Humi: {humidity:.1f} %")
                print(f"Temp: {temperature_F:.1f} F")
                print(f"Light Level: {light_level}\n")

                display.scroll_text(f"{current_time}                ", 0.5)

                display.lcd_clear()

                # Display the received temperature data on the LCD
                display.lcd_display_string(f"Temp: {temperature:.1f} C", 1)
                # Display the received humidity data on the LCD
                display.lcd_display_string(f"Humidity: {humidity:.1f}%", 2)
                time.sleep(15)

                display.lcd_clear()

                display.lcd_display_string(f"Temp: {temperature_F:.1f} F", 1)
                display.lcd_display_string(f"Light Level: {light_level}", 2)
                time.sleep(15)

                display.lcd_clear()

                fetch_weekly_trends()
                display.lcd_clear()

                display.lcd_backlight(0)
            else:
                display.scroll_text("Error reading from sensors...        ", 0.5)
                time.sleep(1)
                display.lcd_clear()
                display.scroll_text("Will try again momentarily...                ", 0.5)
                time.sleep(1)
                display.lcd_clear()
                display.lcd_backlight(0)

except KeyboardInterrupt:
    print("Keyboard interrupt detected. Cleaning up...")

except Exception as e:
    print(f"Error: {e}")

finally:
    print("Cleaning up...")
    display.lcd_clear()  # Clear the LCD display when the program ends
    client_socket.close() 
    server_socket.close()
    cursor.close()
    db_conn.close()
    print("Socket closed. Exiting program.")
