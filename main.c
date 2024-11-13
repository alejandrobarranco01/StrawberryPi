#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <wiringPi.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <mariadb/mysql.h>
#include <wiringPiI2C.h>

#include "dhthw.h"

#define DHT11_PIN 21            // GPIO pin number for DHT11
#define ADS7830_ADDR 0x4b       // I2C address for the ADS7830 (can be 0x48 or 0x49)
#define PHOTORESISTOR_CHANNEL 0 // Channel 0 for photoresistor (LDR)

#define SERVER_PORT 8080 // Server port number
#define SQL_PORT 3306
#define DB_NAME "strawberrypi"
#define DB_USERNAME "root"
#define DB_PASSWORD "root"

int sockfd; // Socket file descriptor
struct sockaddr_in server;

// Function to read from ADS7830 ADC
int read_adc(int fd, int channel)
{
    // Ensure the channel is between 0 and 7
    if (channel < 0 || channel > 7)
    {
        fprintf(stderr, "Invalid channel number!\n");
        return -1;
    }

    // The ADS7830 is a 8-bit ADC with a default voltage range (0-3.3V)
    // Build the command byte for the selected channel
    int command = 0x84 | (channel << 4); // 0x84 is the start command for ADS7830

    // Write the command to start the conversion
    wiringPiI2CWrite(fd, command);
    usleep(10000); // Wait for the conversion to complete (10 ms)

    // Read the result (8-bit ADC value)
    int value = wiringPiI2CRead(fd);
    return value;
}

void setup_socket()
{
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    // Define server address
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // Localhost

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Connection failed");
        exit(1);
    }

    printf("Connected to the Python server.\n");
}

int main(void)
{
    // Initialize WiringPi
    if (wiringPiSetupGpio() == -1)
    {
        printf("Failed to initialize WiringPi.\n");
        return 1; // Exit with an error code
    }

    setup_socket(); // Set up the socket server

    MYSQL *con = mysql_init(NULL);

    if (!con)
    {
        printf("Here\n");
        fprintf(stderr, "mysql_init() failed\n");
        exit(1);
    }

    if (mysql_real_connect(con, "localhost", DB_USERNAME, DB_PASSWORD,
                           DB_NAME, SQL_PORT, NULL, 0) == NULL)
    {
        printf("Error connecting to DB\n");
    }

    // Open the I2C device (ADS7830)
    int fd = wiringPiI2CSetup(ADS7830_ADDR);
    if (fd == -1)
    {
        fprintf(stderr, "Failed to initialize I2C device\n");
        return 1;
    }

    while (1)
    {
        int *dht11_dat = printData(); // Get the data from the sensor

        if (dht11_dat != NULL)
        {

            float temperature = dht11_dat[2] + dht11_dat[3] / 10.0; // Convert to float
            float humidity = dht11_dat[0] + dht11_dat[1] / 10.0;

            int light_level = read_adc(fd, PHOTORESISTOR_CHANNEL);

            // Construct SQL query
            char query[256];
            snprintf(query, sizeof(query),
                     "INSERT INTO SensorData (timestamp, temperature, humidity, light) VALUES (NOW(), %.1f, %.1f, %d)",
                     temperature, humidity, light_level);

            if (mysql_query(con, query))
            {
                printf("here\n");
            }

            // Prepare the temp to send to the Python server
            char data[20];
            snprintf(data, sizeof(data), "%.1f %.1f%% %d", temperature, humidity, light_level);

            // Send the temp to the server
            send(sockfd, data, strlen(data), 0);
        }
        else
        {
            // Create a null message
            char message[20];
            snprintf(message, sizeof(message), "ERROR");

            // Send the temp to the server
            send(sockfd, message, strlen(message), 0);
        }

        // Sleep for 60 seconds before reading again
        sleep(60 * 10);
    }

    return 0;
}
