#ifndef PWM_M0GEN3_H_
#define PWM_M0GEN3_H_

#include <stdint.h>

void init_PWM_m0gen3(void);
void set_m0pwm6_duty_cycle(uint32_t duty_cycle);
void set_m0pwm7_duty_cycle(uint32_t duty_cycle);

#endif
