#include "PCA9624.h"

#include <Wire.h>

// Registers
#define MODE1       0x00
#define MODE2       0x01
#define PWM0        0x02
#define PWM1        0x03
#define PWM2        0x04
#define PWM3        0x05
#define PWM4        0x06
#define PWM5        0x07
#define PWM6        0x08
#define PWM7        0x09
#define GRPPWM      0x0A
#define GRPFREQ     0x0B
#define LEDOUT0     0x0C
#define LEDOUT1     0x0D
#define SUBADR1     0x0E
#define SUBADR2     0x0F
#define SUBADR3     0x10
#define ALLCALLADR  0x11


PCA9624::PCA9624(void)
{}

bool PCA9624::begin(uint8_t i2c_addr)
{
  addr = i2c_addr;
  Wire.begin();
  // Initialise register MODE1
  uint8_t cmd_mode1[] = {MODE1, 0x00};
  Wire.beginTransmission(addr);
  Wire.write(cmd_mode1, sizeof(cmd_mode1));
  Wire.endTransmission();
  // Initialise registers LEDOUT0 and LEDOUT1
  uint8_t cmd_ledout0[] = {LEDOUT0, 0xAA};
  Wire.beginTransmission(addr);
  Wire.write(cmd_ledout0, sizeof(cmd_ledout0));
  uint8_t cmd_ledout1[] = {LEDOUT1, 0xAA};
  Wire.endTransmission();
  Wire.beginTransmission(addr);
  Wire.write(cmd_ledout1, sizeof(cmd_ledout1));
  return Wire.endTransmission() == 0;
}

bool PCA9624::set_pwm(int led, uint8_t duty_cycle)
{
  uint8_t cmd_set_pwm[] = {(uint8_t)(PWM0 + led), duty_cycle};
  Wire.beginTransmission(addr);
  Wire.write(cmd_set_pwm, sizeof(cmd_set_pwm));
  return Wire.endTransmission() == 0;
}

void PCA9624::clear_all(void)
{
  for (int i = 0; i < LED_MAX; i++) {
    set_pwm(i, 0);
  }
}

const int PCA9624::LED_MAX = 8;
const int PCA9624::DUTY_CYCLE_MAX = 255;
