#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "wtimer1.h"

// PortC masks
#define FREQ_IN_MASK 64 // 64 = 0100 0000 = mask for bit 6 (CCP pin PC6)

// initialize the wide timer same as freq_time example
void init_wtimer1(void)
{
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R1;    // enable wide timer 1 clock
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2;        // enable gpio port C clock
    _delay_cycles(3);

    // Configure SIGNAL_IN for frequency and time measurements
    GPIO_PORTC_AFSEL_R |= FREQ_IN_MASK;              // select alternative functions for SIGNAL_IN pin
    GPIO_PORTC_PCTL_R &= ~GPIO_PCTL_PC6_M;           // map alt fns to SIGNAL_IN
    GPIO_PORTC_PCTL_R |= GPIO_PCTL_PC6_WT1CCP0;
    GPIO_PORTC_DEN_R |= FREQ_IN_MASK;                // enable bit 6 for digital input

    enable_wtimer1_counter();
}

// enable the wide timer same as freq_time example
void enable_wtimer1_counter(void)
{
    // Configure Wide Timer 1 as counter of external events on CCP0 pin
    WTIMER1_CTL_R &= ~TIMER_CTL_TAEN;                // turn-off counter before reconfiguring
    WTIMER1_CFG_R = 4;                               // configure as 32-bit counter (A only)
    WTIMER1_TAMR_R = TIMER_TAMR_TAMR_CAP | TIMER_TAMR_TACDIR; // configure for edge count mode, count up
    WTIMER1_CTL_R = TIMER_CTL_TAEVENT_POS;           // count positive edges
    WTIMER1_IMR_R = 0;                               // turn-off interrupts
    WTIMER1_TAV_R = 0;                               // zero counter for first period
    WTIMER1_CTL_R |= TIMER_CTL_TAEN;                 // turn-on counter
}

// disable the wide timer same as freq_time example
void disable_wtimer1_counter(void)
{
    WTIMER1_CTL_R &= ~TIMER_CTL_TAEN;                // turn-off event counter
}

// simply return the count value
uint32_t get_wtimer1_count(void)
{
    return WTIMER1_TAV_R;
}

// reset the count value for the next time period
void clear_wtimer1_count(void)
{
    WTIMER1_TAV_R = 0;
}
