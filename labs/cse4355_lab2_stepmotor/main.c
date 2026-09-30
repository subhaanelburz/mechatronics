#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "gpio.h"
#include "uart0.h"
#include "wait.h"
#include "uart_shell.h"
#include "common_terminal_interface.h"
#include "strings.h"
#include "stepper_motor.h"
#include "pwm_m0gen3.h"

#define SYS_CLK     40000000    // System Clock

#define OB_BLUE     (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4))) //PF2
#define OB_GREEN    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4))) //PF3

void init_HW(void)
{
    initSystemClockTo40Mhz();   // init system clock

    initUart0();                // init uart0 for shell interface
    setUart0BaudRate(115200, SYS_CLK);

    init_PWM_m0gen3();          // init PWM module 0, generator 3
    init_motor();               // init all pins for the stepper motor / H bridges

    // enable on-board LEDs
    enablePort(PORTF);
    selectPinPushPullOutput(PORTF, 2);
    selectPinPushPullOutput(PORTF, 3);

    // flash blue and green LED at start to ensure it reset
    OB_BLUE = 1;
    waitMicrosecond(250000);
    OB_BLUE = 0;
    waitMicrosecond(250000);
    OB_GREEN = 1;
    waitMicrosecond(250000);
    OB_GREEN = 0;
}

void shell(void)
{
    USER_DATA input;

    help_cmd();                 // print help menu
    putsUart0("> ");

    while (1)
    {
        getsUart0(&input);      // get input string from receiving FIFO
        parseFields(&input);    // tokenize

        process_shell(&input);  // now process the shell
    }
}

int main(void)
{
    init_HW();  // initialize all hardware
    shell();    // start the shell

    // return 0;
}
