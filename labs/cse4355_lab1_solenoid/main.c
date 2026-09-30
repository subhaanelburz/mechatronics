#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "gpio.h"

#define BOTTOM_SWITCH (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 1*4)))
#define TOP_SWITCH (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 2*4)))
#define SOLENOID (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 3*4)))

void init_HW(void)
{
    initSystemClockTo40Mhz();

    enablePort(PORTE);
    selectPinDigitalInput(PORTE, 1);
    selectPinDigitalInput(PORTE, 2);
    selectPinPushPullOutput(PORTE, 3);

    SOLENOID = 0;
}

int main(void)
{
    init_HW();

    while (1)
    {
        SOLENOID = 1;

        while (TOP_SWITCH == 0)
        {
            // wait until top switch is pressed
        }

        SOLENOID = 0;

        while (BOTTOM_SWITCH == 0)
        {
            // wait until bottom switch is pressed
        }
    }

    // return 0;
}
