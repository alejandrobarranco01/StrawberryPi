#ifndef DHTHW_H
#define DHTHW_H

#include <stdint.h>

// Pin definitions
#define DHT11_PIN 21
#define ITERATIONS 85

// Global variable to store DHT11 data
extern int dht11_dat[5];

void init_dht11(void);
void parse_dht11_data(uint8_t *bit, uint8_t microseconds);
int verify_checksum(void);
int *printData(void);

#endif // DHTHW_H
