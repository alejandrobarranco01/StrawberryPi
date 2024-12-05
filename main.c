#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <wiringPi.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <mariadb/mysql.h>

#include "dhthw.h"
#include "adchw.h"
#include "mq5hw.h"

#define DHT11_PIN 21
#define PHOTORESISTOR_CHANNEL 0

#define SERVER_PORT 8081
#define SQL_PORT 3306
#define DATABASE_NAME "strawberrypi"
#define DATABASE_USERNAME "root"
#define DATABASE_PASSWORD "root"

int sockfd;
struct sockaddr_in server;

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

    if (mysql_real_connect(con, "localhost", DATABASE_USERNAME, DATABASE_PASSWORD,
                           DATABASE_NAME, SQL_PORT, NULL, 0) == NULL)
    {
        printf("Error connecting to DB\n");
    }

    // Open the I2C device (ADS7830)
    int *fd = initializeADC(); // Receive a pointer to the file descriptor

    float r0 = calibrateMQ5(fd);
    printf("r0: %.2f\n", r0);

    while (1)
    {
        int *dht11_dat = printData(); // Get the data from the sensor

        float *MQ5Values = readMQ5(fd, &r0);

        if (dht11_dat && MQ5Values)
        {

            float temperature = dht11_dat[2] + dht11_dat[3] / 10.0; // Convert to float
            float humidity = dht11_dat[0] + dht11_dat[1] / 10.0;

            int light_level = read_adc(fd, PHOTORESISTOR_CHANNEL);

            float co_ppm = MQ5Values[0];
            float lpg_ppm = MQ5Values[1];

            printf("Data Sent:\nTemp: %.1f C\nHumidity: %.1f%%\nLight Level: %d\nCO: %.2f PPM\nLPG: %.2f PPM\n",
                   temperature, humidity, light_level, co_ppm, lpg_ppm);

            // Construct SQL query
            char query[256];
            snprintf(query, sizeof(query),
                     "INSERT INTO SensorData (timestamp, temperature, humidity, light, co_ppm, lpg_ppm) VALUES (NOW(), %.1f, %.1f, %d, %.2f, %.2f)",
                     temperature, humidity, light_level, co_ppm, lpg_ppm);

            if (mysql_query(con, query))
            {
                printf("here\n");
            }

            // Prepare the temp to send to the Python server
            char data[40];
            snprintf(data, sizeof(data), "%.1f %.1f%% %d %.2f %.2f", temperature, humidity, light_level, co_ppm, lpg_ppm);

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
