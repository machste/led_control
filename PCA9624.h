#ifndef PCA9624_H
#define PCA9624_H

#include <inttypes.h>


class PCA9624
{
public:
  static const int LED_MAX;
  static const int DUTY_CYCLE_MAX;

  PCA9624(uint8_t addr);
  bool begin(void);
  bool set_pwm(int led, uint8_t duty_cycle);
  void clear_all(void);
  
private:
  uint8_t addr;
};

#endif
