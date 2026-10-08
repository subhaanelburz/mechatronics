#include <stdint.h>
#include "buttons.h"
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "wait.h"
#include "strings.h"
#include "uart0.h"
#include "timer0.h"
#include "brushed_motor.h"

#define NUM_BUTTONS 2
#define DEBOUNCE    200 // 200 ms debounce time

static PORT ports[NUM_BUTTONS] = {PORTF, PORTF};
static uint8_t pins[NUM_BUTTONS] = {4, 0};
static uint32_t debounce_count[NUM_BUTTONS] = {0};

static volatile uint8_t pb_pressed = 0;

void init_buttons(void)
{
    enablePort(PORTF);

    selectPinPushPullOutput(PORTF, 2);  // enable blue LED
    selectPinPushPullOutput(PORTF, 3);  // enable green LED

    // unlock PF0 to use second onboard switch (pg 684)
    // gpio library writes special lock value
    // then writes to commit register
    setPinCommitControl(PORTF, 0);

    // set both push buttons as digital inputs
    // with pull ups and falling edge interrupts
    selectPinDigitalInput(PORTF, 4);
    selectPinDigitalInput(PORTF, 0);
    enablePinPullup(PORTF, 4);
    enablePinPullup(PORTF, 0);
    selectPinInterruptFallingEdge(PORTF, 4);
    selectPinInterruptFallingEdge(PORTF, 0);

    // enable interrupts on port F (int 30) on pg 105
    NVIC_PRI7_R |= 0x00E00000;  // lowest priority (7) for port F (int 30)
    NVIC_EN0_R |= 0x40000000;   // enable int 30

    // enable 1ms interrupt for push button debouncing
    init_timer0(1000);
}

void process_buttons(void)
{
    char buffer[12];

    if (pb_pressed & 0x10)
    {
        pb_pressed &= ~0x10;
        motor_duty_down();
        putsUart0("\r\nPB0 pressed! Decreasing PWM by 10%, duty cycle = ");
        putsUart0(toAsciiDec(buffer, motor_get_duty_percent()));
        putsUart0("%");
        OB_GREEN = 1;
        waitMicrosecond(100000);
        OB_GREEN = 0;
    }

    if (pb_pressed & 0x01)
    {
        pb_pressed &= ~0x01;
        motor_duty_up();
        putsUart0("\r\nPB1 pressed! Increasing PWM by 10%, duty cycle = ");
        putsUart0(toAsciiDec(buffer, motor_get_duty_percent()));
        putsUart0("%");
        OB_BLUE = 1;
        waitMicrosecond(100000);
        OB_BLUE = 0;
    }
}

void gpio_portF_handler(void)
{
    if (GPIO_PORTF_MIS_R & 0x10)
    {
        disablePinInterrupt(PORTF, 4);
        clearPinInterrupt(PORTF, 4);
        debounce_count[0] = 0;
        pb_pressed |= 0x10;
    }

    if (GPIO_PORTF_MIS_R & 0x01)
    {
        disablePinInterrupt(PORTF, 0);
        clearPinInterrupt(PORTF, 0);
        debounce_count[1] = 0;
        pb_pressed |= 0x01;
    }
}

void timer0_handler(void)
{
    // clear the timer interrupt or this ISR will loop forever
    TIMER0_ICR_R = TIMER_ICR_TATOCINT;

    // isr will run every ms and count up to 200 ms
    // once it reaches 200 ms all push button interrupts are enabled
    // then when you press a button the push button gpio handler
    // will disable the interrupt, clear it, and set count back to 0
    // basically when a button is pressed we disable interrupts
    // for 200 ms to ensure that no double clicks register

    uint8_t i;

    for (i = 0; i < NUM_BUTTONS; i++)
    {
        if (debounce_count[i] <= DEBOUNCE)
        {
            debounce_count[i]++;

            // if the button has been pressed
            if (debounce_count[i] > DEBOUNCE)
            {
                clearPinInterrupt(ports[i], pins[i]);
                enablePinInterrupt(ports[i], pins[i]);
            }
        }
    }
}
