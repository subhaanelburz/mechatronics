#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "timer2.h"

#define SYS_CLK 40000000    // System Clock

// both freq are inputted as Hz
void init_timer2(uint32_t freq)
{
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R2;  // enable clock for timer 2
    _delay_cycles(3);                           // standard delay after enabling clock

    TIMER2_CTL_R &= ~TIMER_CTL_TAEN;            // (1) disable the timer before making changes

    // default is a 32 bit timer (its 0)
    TIMER2_CFG_R = TIMER_CFG_32_BIT_TIMER;      // (2) configure for 32-bit mode

    TIMER2_TAMR_R &= ~TIMER_TAMR_TAMR_M;        // (3) configure Periodic Mode (0x2) + mask to clear
    TIMER2_TAMR_R |= TIMER_TAMR_TAMR_PERIOD;

    // direction down
    TIMER2_TAMR_R &= ~TIMER_TAMR_TACDIR;        // (4) set direction as a down counter

    uint32_t load_value = (SYS_CLK / freq) - 1; // (5) calculate the load value
    TIMER2_TAILR_R = load_value;

    TIMER2_ICR_R = TIMER_ICR_TATOCINT;          // (6) set up interrupts, clear previous flags
    TIMER2_IMR_R |= TIMER_IMR_TATOIM;           //     enable interrupt when times out (counts to 0)

    // interrupt corresponds to 23rd bit in EN0
    NVIC_EN0_R |= (1 << 23);                    // (7) NVIC enable + priority
    NVIC_PRI5_R &= ~NVIC_PRI5_INT23_M;          //     clear previous priority
    NVIC_PRI5_R |= (uint32_t) (7 << NVIC_PRI5_INT23_S);  // set to lowest priority (default/0 is highest)
                                                         // can shift 0 to 7, with 7 being lowest priority

    TIMER2_CTL_R |= TIMER_CTL_TAEN;             // (8) enable timer 2 (as 32-bit down counter so timers 2a and timers 2b)
}

void set_timer2_freq(uint32_t freq)
{
    TIMER2_CTL_R &= ~TIMER_CTL_TAEN;            // (1) disable the timer before making changes

    uint32_t load_value = (SYS_CLK / freq) - 1; // (2) re-calculate the load value
    TIMER2_TAILR_R = load_value;

    TIMER2_CTL_R |= TIMER_CTL_TAEN;             // (3) re-enable the timer
}
