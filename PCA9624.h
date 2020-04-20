#ifndef PCA9624_H
#define PCA9624_H

#include <inttypes.h>


class PCA9624
{
public:
  static const int LED_MAX;
  static const int DUTY_CYCLE_MAX;

  PCA9624(uint8_t all_call_addr);
  bool begin(uint8_t addr);
  bool set_pwm(int led, uint8_t duty_cycle);
  void clear_all(void);
  
private:
  uint8_t all_call_addr;
};

#endif
