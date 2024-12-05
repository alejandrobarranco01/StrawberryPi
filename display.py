#!/usr/bin/python3

import time
import mysql.connector
import drivers
import socket
from datetime import datetime

SERVER_ADDRESS = '127.0.0.1'
SERVER_PORT = 8081
DATABASE_NAME = 'strawberrypi'

display = drivers.Lcd()

# Set up server socket start listening for the C files
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((SERVER_ADDRESS, SERVER_PORT))
server_socket.listen(1)
print("Waiting for C code...")

# Accept the connect with the C files
client_socket, client_address = server_socket.accept()
print(f"Connection established with {client_address}")

try:
    db_conn = mysql.connector.connect(
        host="localhost",
        user="root",
        password="root",
        database=DATABASE_NAME
    )
    print("Connected to DB successfully...")
    cursor = db_conn.cursor()
except mysql.connector.Error as err:
    print(f"Error: {err}")

# Function to separate the data received from main.c
def parse_data(data):
    try:
        if data == "ERROR":
            return None, None, None, None, None
        temp_str, humi_str, light_str, co_ppm_str, lpg_ppm_str = data.split()  
        temperature = float(temp_str) 
        humidity = float(humi_str.replace('%', ''))  
        light_levels = int(light_str)
        co_ppm = float(co_ppm_str)
        lpg_ppm = float(lpg_ppm_str)
        return temperature, humidity, light_levels, co_ppm, lpg_ppm
    except Exception as e:
        print("Error parsing data:", data)
        return None, None, None, None, None

# Function to retrieve rolling 7 day trends
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
        MAX(light) AS max_light,
        AVG(co_ppm) AS avg_co_ppm,
        MIN(co_ppm) AS min_co_ppm,
        MAX(co_ppm) AS max_co_ppm,
        AVG(lpg_ppm) AS avg_lpg_ppm,
        MIN(lpg_ppm) AS min_lpg_ppm,
        MAX(lpg_ppm) AS max_lpg_ppm
    FROM SensorData
    WHERE timestamp >= NOW() - INTERVAL 7 DAY
    """
    
    cursor.execute(query)
    avg_temp, min_temp, max_temp, avg_humidity, min_humidity, max_humidity, avg_light, min_light, max_light, avg_co_ppm, min_co_ppm, max_co_ppm, avg_lpg_ppm, min_lpg_ppm, max_lpg_ppm = cursor.fetchone()  # Fetch the result of the query

    display_data = (
        f"Avg Temp: {avg_temp:.1f} C        "
        f"Min Temp: {min_temp:.1f} C        Max Temp: {max_temp:.1f} C        "                
        f"Avg Humidity: {avg_humidity:.1f}%        "
        f"Min Humidity: {min_humidity:.1f}%        Max Humidity: {max_humidity:.1f}%        "
        f"Avg Light: {avg_light}        "
        f"Min Light: {min_light}        Max Light: {max_light}        "
        f"Avg Carbon Monoxide: {avg_co_ppm:.2f} PPM        "
        f"Min Carbon Monoxide: {min_co_ppm:.2f} PPM        Max Carbon Monoxide: {max_co_ppm:.2f}        "
        f"Avg Liquified Petroleum Gas (LPG): {avg_lpg_ppm:.2f} PPM        "
        f"Min Liquified Petroleum Gas (LPG): {min_lpg_ppm:.2f} PPM        Max Liquified Petroleum Gas (LPG): {max_lpg_ppm:.2f} PPM        "
    )

    display.lcd_clear()
    display.scroll_text(f"{display_data}", 0.5)            

try:
    print("Waiting to receive data...")
    while True:
        data = client_socket.recv(1024).decode('utf-8')

        if data:
            display.lcd_backlight(1)

            temperature, humidity, light_level, co_ppm, lpg_ppm = parse_data(data)  # Parse the data
            
            if temperature is not None and humidity is not None:
                temperature_F = (temperature * 9/5) + 32

                current_time = datetime.now().strftime("%B %d, %Y %I:%M %p") 

                # Print temperature, humidity, and time
                print(f"Time: {current_time}")
                print(f"Temp: {temperature:.1f} C")
                print(f"Humidity: {humidity:.1f} %")
                print(f"Temp: {temperature_F:.1f} F")
                print(f"Light Level: {light_level}")
                print(f"Carbon Monoxide: {co_ppm:.2f} PPM")
                print(f"Liquified Petroleum Gas (LPG): {lpg_ppm:.2f} PPM\n")


                # Display the date and time
                display.scroll_text(f"{current_time}                ", 0.5)
                display.lcd_clear()

                # Display the temperature in C and humidity in the first 15 seconds
                display.lcd_display_string(f"Temp: {temperature:.1f} C", 1)
                display.lcd_display_string(f"Humidity: {humidity:.1f}%", 2)
                time.sleep(10)
                display.lcd_clear()

                # Display the temperature in F and light levels in the next 15 seconds
                display.lcd_display_string(f"Temp: {temperature_F:.1f} F", 1)
                display.lcd_display_string(f"Light Level: {light_level}", 2)
                time.sleep(10)
                display.lcd_clear()

                
                display.lcd_display_string(f"CO: {co_ppm:.2f} PPM", 1)
                display.lcd_display_string(f"LPG: {lpg_ppm:.2f} PPM", 2)
                time.sleep(10)
                display.lcd_clear()

                # Display the rolling 7 day trends
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
    print("Keyboard Interrupt detected...")
except Exception as e:
    print(f"Error: {e}")

finally:
    print("Cleaning up...")
    display.lcd_clear() 
    client_socket.close() 
    server_socket.close()
    cursor.close()
    db_conn.close()
    
