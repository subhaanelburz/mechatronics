#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "timer0.h"

#define SYS_CLK 40000000    // System Clock

// both freq are inputted as Hz
void init_timer0(uint32_t freq)
{
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R0;  // enable clock for timer 0
    _delay_cycles(3);                           // standard delay after enabling clock

    TIMER0_CTL_R &= ~TIMER_CTL_TAEN;            // (1) disable the timer before making changes

    // default is a 32 bit timer (its 0)
    TIMER0_CFG_R = TIMER_CFG_32_BIT_TIMER;      // (2) configure for 32-bit mode

    TIMER0_TAMR_R &= ~TIMER_TAMR_TAMR_M;        // (3) configure Periodic Mode (0x2) + mask to clear
    TIMER0_TAMR_R |= TIMER_TAMR_TAMR_PERIOD;

    // direction down
    TIMER0_TAMR_R &= ~TIMER_TAMR_TACDIR;        // (4) set direction as a down counter

    uint32_t load_value = (SYS_CLK / freq) - 1; // (5) calculate the load value
    TIMER0_TAILR_R = load_value;

    TIMER0_ICR_R = TIMER_ICR_TATOCINT;          // (6) set up interrupts, clear previous flags
    TIMER0_IMR_R |= TIMER_IMR_TATOIM;           //     enable interrupt when times out (counts to 0)

    // interrupt corresponds to 19th bit in EN0
    NVIC_EN0_R |= (1 << 19);                    // (7) NVIC enable + priority
    NVIC_PRI4_R &= ~NVIC_PRI4_INT19_M;          //     clear previous priority
    NVIC_PRI4_R |= (uint32_t) (7 << NVIC_PRI4_INT19_S);  // set to lowest priority (default/0 is highest)
                                                         // can shift 0 to 7, with 7 being lowest priority

    TIMER0_CTL_R |= TIMER_CTL_TAEN;             // (8) enable timer 0 (as 32-bit down counter so timers 0a and timers 0b)
}

void set_timer0_freq(uint32_t freq)
{
    TIMER0_CTL_R &= ~TIMER_CTL_TAEN;            // (1) disable the timer before making changes

    uint32_t load_value = (SYS_CLK / freq) - 1; // (2) re-calculate the load value
    TIMER0_TAILR_R = load_value;

    TIMER0_CTL_R |= TIMER_CTL_TAEN;             // (3) re-enable the timer
}
