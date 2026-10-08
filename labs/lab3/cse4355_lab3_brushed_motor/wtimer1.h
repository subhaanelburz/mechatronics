#ifndef WTIMER1_H_
#define WTIMER1_H_

#include <stdint.h>

void init_wtimer1(void);
void enable_wtimer1_counter(void);
void disable_wtimer1_counter(void);
uint32_t get_wtimer1_count(void);
void clear_wtimer1_count(void);

#endif
