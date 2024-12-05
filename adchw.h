#include <stdio.h>
#include <wiringPiI2C.h>
#include <unistd.h>
#include <stdlib.h>

#define ADS7830_ADDR 0x4b

int read_adc(int *fd, int channel);
int *initializeADC();