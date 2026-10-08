#ifndef BRUSHED_MOTOR_H_
#define BRUSHED_MOTOR_H_

#include <stdint.h>
#include <stdbool.h>

// macro for part 2 of lab (steps 6-10)
// comment out for part 1 code only (steps 1-5)
#define PART2

// part 1 functions to measure speed with optical switch
void init_brushed_motor(void);
void motor_duty_up(void);
void motor_duty_down(void);
uint32_t motor_get_duty_percent(void);
bool motor_optical_ready(void);
uint32_t motor_get_optical_freq(void);
uint32_t motor_get_optical_rpm(void);

// part 2 functions to measure speed with back emf (ADC)
#ifdef PART2
uint32_t motor_get_bemf_raw(void);
uint32_t motor_get_drain_mv(void);
uint32_t motor_get_bemf_mv(void);
uint32_t motor_get_bemf_rpm(void);
#endif

#endif
