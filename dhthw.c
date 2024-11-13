#include <stdio.h>
#include <wiringPi.h>
#include <string.h>
#include <time.h>

#include "dhthw.h"

// Declare dht11_dat as a global variable
int dht11_dat[5] = {0, 0, 0, 0, 0};

// Initialize DHT11 sensor (send start signal)
void init_dht11(void)
{
    pinMode(DHT11_PIN, OUTPUT);
    digitalWrite(DHT11_PIN, LOW); // Pull pin down for 18 milliseconds
    delay(18);
    digitalWrite(DHT11_PIN, HIGH); // Then pull it up for 40 microseconds
    delayMicroseconds(40);
    pinMode(DHT11_PIN, INPUT); // Set pin to input mode to read data
}

// Parse the DHT11 data by shifting the bits into the dht11_dat array
void parse_dht11_data(uint8_t *bit, uint8_t microseconds)
{
    // printf("DAT array before: %s\n", dht11_dat);
    dht11_dat[*bit / 8] <<= 1; // Divide every 8 bits for correct processing and shift by one to make room for other bits
    // printf("Bit: %d\n", *bit);
    // printf("microseconds: %d\n", microseconds);
    if (microseconds > 16) // If the microseconds is large, it’s a 1 bit
    {
        dht11_dat[*bit / 8] |= 1; // Set bit to a 1
    }
    // printf("DAT array after: %s\n", dht11_dat);
    (*bit)++;
}

// Verify if the checksum is correct
int verify_checksum(void)
{
    return (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF));
}

int *printData(void)
{
    uint8_t laststate = HIGH;
    uint8_t microseconds = 0;
    uint8_t bit = 0, i;
    float f; /* fahrenheit */

    dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;

    init_dht11();

    /* detect change and read data */
    // Runs for 40 bits * 2 Transitions Per Bit + Some Buffer
    for (i = 0; i < ITERATIONS; i++)
    {

        microseconds = 0;
        while (digitalRead(DHT11_PIN) == laststate)
        {
            microseconds++;
            delayMicroseconds(1);
            if (microseconds == 255)
                break;
        }
        laststate = digitalRead(DHT11_PIN); // Store previous state of pin

        if (microseconds == 255)
            break; // Break out if takes longer than 255 microseconds

        // Ignore first few transitions and only process the length of the the half of the data (the high part)
        if ((i >= 4) && (i % 2 == 0))
        {
            parse_dht11_data(&bit, microseconds);
        }
    }

    /*
     * check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
     * print it out if data is good
     */
    if ((bit >= 40) && verify_checksum())
    {
        f = dht11_dat[2] * 9. / 5. + 32;
        // printf("Humidity = %d.%d %% Temperature = %d.%d *C (%.1f *F)\n",
        //  dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3], f);
        // Get the current time
        time_t rawtime;
        struct tm *timeinfo;
        char timeBuffer[80];

        time(&rawtime);                 // Get current time
        timeinfo = localtime(&rawtime); // Convert to local time

        // Format the time and store it in timeBuffer
        strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", timeinfo);

        // Print the time and data
        printf("Data collected at %s\n", timeBuffer);

        // Return the array with data
        return dht11_dat;
    }
    else
    {
        printf("Data not good, skip\n");

        return NULL;
    }
}