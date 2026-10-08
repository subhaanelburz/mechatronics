#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <stdint.h>

#define OB_BLUE     (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4))) //PF2
#define OB_GREEN    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4))) //PF3

void init_buttons(void);
void process_buttons(void);

#endif
