#define MQ5_CHANNEL 7

#include "adchw.h"

float ratioToCOPPM(float ratio);
float ratioToLPGPPM(float ratio);
float calibrateMQ5(int *fd);
float *readMQ5(int *fd, float *r0);