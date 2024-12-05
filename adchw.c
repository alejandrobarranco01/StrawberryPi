#include "adchw.h"

// Function to read from ADS7830 ADC
int read_adc(int *fd, int channel)
{
    // Just to make sure the channel used is between 0 and 8
    if (channel < 0 || channel > 7)
    {
        fprintf(stderr, "Invalid channel number!\n");
        return -1;
    }

    int command = 0x84 | (channel << 4); // 0x84 is the start command for ADS7830

    wiringPiI2CWrite(*fd, command);
    usleep(10000);

    int value = wiringPiI2CRead(*fd);
    return value;
}

int *initializeADC()
{
    int *fd = (int *)malloc(sizeof(int));
    *fd = wiringPiI2CSetup(ADS7830_ADDR);
    if (*fd == -1)
    {
        fprintf(stderr, "Failed to initialize I2C device\n");
        return NULL;
    }

    return fd;
}