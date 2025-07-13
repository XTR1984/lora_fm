/* based on code by Amon Schumann / DL9AS   https://github.com/DL9AS
*/ 
#ifndef __SX127x__H__
#define __SX127x__H__

#include<Arduino.h>
#define SX127x_FREQUENCY_CORRECTION 0
#define SX127x_CRYSTAL_FREQ 32000000

void SX127x_begin(void);
void SX127x_enable(void);
void SX127x_disable(void);
void SX127x_sleep(void);
void SX127x_reset(void);

void SX127x_standby(void);
void SX127x_scan_mode(uint8_t gain, uint8_t avgsamples, uint8_t rxbwmant, uint8_t rxbwexp);
void SX127x_write_reg(uint8_t address, uint8_t reg_value);
uint8_t SX127x_read_reg(uint8_t address);
void SX127x_set_frequency(uint64_t *freq);
void SX127x_set_TX_power(uint8_t pwr, bool pa_boost_mode_20dbm);
int8_t SX127x_getRSSI(void);
uint8_t SX127x_getRSSI_raw(void);
#endif