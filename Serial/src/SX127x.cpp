/* based on code by Amon Schumann / DL9AS   https://github.com/DL9AS
*/ 

#include <Arduino.h>
#include <SPI.h>

#include "SX127x.h"
#include "SX127x_register.h"
#include "pins.h"


// Module functions
void SX127x_write_reg(uint8_t address, uint8_t reg_value)
{
  digitalWrite(SX127x_SS, LOW);  // Select SX127x SPI device
  SPI.transfer(address | 0x80);             
  SPI.transfer(reg_value);
  digitalWrite(SX127x_SS, HIGH);  // Clear SX127x SPI device selection
}

uint8_t SX127x_read_reg(uint8_t address)
{
  digitalWrite(SX127x_SS, LOW);  // Select SX127x SPI device
  SPI.transfer(address & 0x7F);
  uint8_t reg_value = SPI.transfer(0);
  digitalWrite(SX127x_SS, HIGH);  // Clear SX127x SPI device selection

  return reg_value;
}

// Exported functions
void SX127x_begin(void)
{
  SPI.begin(SX127x_SCK, SX127x_MISO, SX127x_MOSI, SX127x_SS); // Begin SPI communication
  SPI.setFrequency(20000000);

  pinMode(SX127x_SS, OUTPUT);
  digitalWrite(SX127x_SS, HIGH);  // Clear SX127x SPI device selection

  pinMode(SX127x_RESET, OUTPUT);
  SX127x_enable();
}

void SX127x_enable(void)
{
  digitalWrite(SX127x_RESET, HIGH); // Enable SX127x
}

void SX127x_disable(void)
{
  digitalWrite(SX127x_RESET, LOW); // Disable SX127x
}

void SX127x_sleep(void)
{
  // Set SX127x operating mode to sleep mode
  // 7: 0->FSK/OOK Mode
  // 6-5: 00->FSK
  // 4: 0 (reserved)
  // 3: 1->Low Frequency Mode
  // 2-0: 000->Sleep Mode
  SX127x_write_reg(REG_OP_MODE, 0x08);
}

void SX127x_reset(void)
{
  digitalWrite(SX127x_RESET, LOW); // Disable SX127x
  delay(5);
  digitalWrite(SX127x_RESET, HIGH); // Enable SX127x
  delay(5);
}

void SX127x_standby(){
  // Set SX127x operating mode
  // 7: 0->FSK/OOK Mode
  // 6-5: 00->FSK   01-ook - not matter
  // 4: 0 (reserved)
  // 3: 1->Low Frequency Mode - not matter
  // 2-0: 001->Standby Mode (RX)
  SX127x_write_reg(REG_OP_MODE,  0x09 );
}

void SX127x_scan_mode( uint8_t gain, uint8_t avgsamples, uint8_t rxbwmant, uint8_t rxbwexp){

  //4  afc
  //3: 0 agcauto  - disable
  //2-3: rxtrigger 0x6
  SX127x_write_reg(REG_RX_CONFIG, 0x6); 

//  SX127x_set_frequency(freq);
  // Set packet mode config
  // 7: unused
  // 6: Data Mode 0->Continuos mode
  // 5: Io Home On 0->off
  // 4: Io Home Pwr Frame 0->off
  // 3: Beacon On 0->off
  // 2-0: Payload length MSB
  SX127x_write_reg(REG_DETECT_OPTIMIZE, 0x00);

  // Set SX127x operating mode
  // 7: 0->FSK/OOK Mode
  // 6-5: 00->FSK   01-ook - not matter
  // 4: 0 (reserved)
  // 3: 0->High Frequency Mode - not matter
  // 2-0: 101->Reciever Mode (RX)
  SX127x_write_reg(REG_OP_MODE,  0x05 );

  //set count samples for rssi
  // default 0x2
  SX127x_write_reg(REG_RSSI_CONFIG,  avgsamples & 7);

  //rxbwmant = 24, exp = 7  -  2,6KHz filter
  SX127x_write_reg(REG_RXBW, (3&rxbwmant) <<3 | (7&rxbwexp) );

  
  

  //gain 
  SX127x_write_reg(REG_LNA,  (gain & 7)<< 5);

  SX127x_write_reg(REG_PLL_HOP, 0xAD); // Set Fast_Hop_On (MSB bit) true to enable fast freq hopping

}

void SX127x_set_TX_power(uint8_t pwr, bool pa_boost_mode_20dbm)
{
  uint8_t TX_out;

  if (pa_boost_mode_20dbm)
  {
    SX127x_write_reg(REG_PA_DAC, 0x87); // Enable the +20dBm option on PA_BOOT
    TX_out = pwr - 5; // TX out is between 0-15 (2bit) -> add 5 to Tx out to get dbm
  }
  else
  {
    SX127x_write_reg(REG_PA_DAC, 0x84); // Do not use the +20dBm option on PA_BOOT
    TX_out = pwr - 2; // TX out is between 0-15 (2bit) -> add 2 to Tx out to get dbm
  }
  
  // PA selection and output power control
  // 7-5: PA select 0 -> RFO pin (14dbm) | 1 -> PA_BOOST pin (20dbm)
  // 6-4: Max Pwr (Pmaxreg = (Pmax - 10.8) / 0.6) -> set to max
  // 3-: Out Pwr
  SX127x_write_reg(REG_PA_CONFIG, (0xF0 | TX_out));
}


void SX127x_set_frequency(uint64_t *freq)
{
  uint64_t freq_tmp = *freq;
  // Frequency value is calculate by:
  // Freg = (Frf * 2^19) / Fxo
  // Resolution is 61.035 Hz if Fxo = 32 MHz
  freq_tmp = (((freq_tmp + SX127x_FREQUENCY_CORRECTION) << 19) / SX127x_CRYSTAL_FREQ);

  SX127x_write_reg(REG_FR_MSB, freq_tmp >> 16);  // Write MSB of RF carrier freq
  SX127x_write_reg(REG_FR_MID, freq_tmp >> 8);  // Write MID of RF carrier freq
  SX127x_write_reg(REG_FR_LSB, freq_tmp);  // Write LSB of RF carrier freq
}

int8_t SX127x_getRSSI(void){
  uint8_t value;
  value = SX127x_read_reg(REG_RSSI_VALUE);
  return -value/2;
}

uint8_t SX127x_getRSSI_raw(void){
  uint8_t value;
  value = SX127x_read_reg(REG_RSSI_VALUE);
  return value;
}

