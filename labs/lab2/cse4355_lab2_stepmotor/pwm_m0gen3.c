#include <stdint.h>
#include "pwm_m0gen3.h"
#include "tm4c123gh6pm.h"
#include "wait.h"

#define PC4 0x10    // 0001.0000 = bit 4 is ON
#define PC5 0x20    // 0010.0000 = bit 5 is ON

// reload value for a 10kHz output PWM signal
// 10 MHz (PWM clock) / 10 kHz (desired output) = 1000 for the load value
#define PWM_RELOAD 1000

// the duty cycle has to be inputed as 1000*(duty cycle as decimal)
// so 90% duty cycle has to be inputted as 900
void set_m0pwm6_duty_cycle(uint32_t duty_cycle)
{
    // the duty cycle is calculated as 1000 * (1 - desired_duty_cycle)
    // if we want a 90% duty cycle = 1000 * (1 - 0.9) = 100
    // this means that from the RELOAD value 1000 to 100, the signal will stay on
    // then from 100 to 0, it will turn off until it wraps around again

    uint32_t comparator_value = PWM_RELOAD - duty_cycle;

    // if the value is greater than 999, clamp it to 999
    // this is only for duty_cycle = 0 case
    if (comparator_value > PWM0_3_LOAD_R)
    {
        comparator_value = PWM0_3_LOAD_R;
    }

    // actually set the duty cycle
    PWM0_3_CMPA_R = comparator_value;
}

// same thing as other function but sets it for comparator B
// or in other words, for M0PWM7 (PC5)
void set_m0pwm7_duty_cycle(uint32_t duty_cycle)
{
    // the duty cycle is calculated as 1000 * (1 - desired_duty_cycle)
    // if we want a 90% duty cycle = 1000 * (1 - 0.9) = 100
    // this means that from the RELOAD value 1000 to 100, the signal will stay on
    // then from 100 to 0, it will turn off until it wraps around again

    uint32_t comparator_value = PWM_RELOAD - duty_cycle;

    // if the value is greater than 999, clamp it to 999
    // this is only for duty_cycle = 0 case
    if (comparator_value > PWM0_3_LOAD_R)
    {
        comparator_value = PWM0_3_LOAD_R;
    }

    // actually set the duty cycle
    PWM0_3_CMPB_R = comparator_value;
}

// initialize PWM module 0, generator 3 signals (pg 1347)
// this can be seen in table 23-5 (pg 1351)
// function is modeled same way as in embedded 1 lab 7 that's why no gpio.h
void init_PWM_m0gen3(void)
{
    // initialize clocks for GPIO port C and PWM module 0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2;    // R2 is port C
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0;      // R0 is PWM module 0

    _delay_cycles(3);  // delay after setting clocks

    // pins PC4 and PC5 will be used in alternate function mode for PWM module 0
    // table on page 658 explains that PWM must set the AFSEL high, ODR low, DEN high

    GPIO_PORTC_AFSEL_R |= PC4 | PC5;    // set alternate function select
    GPIO_PORTC_ODR_R &= ~(PC4 | PC5);   // clear open drain select
    GPIO_PORTC_DEN_R |= PC4 | PC5;      // set digital functions

    GPIO_PORTC_AMSEL_R &= ~(PC4 | PC5); // clear analog mode select

    // next we must configure the PCTL register properly, as table on page 1351
    // PC4 = M0PWM6
    // PC5 = M0PWM7

    GPIO_PORTC_PCTL_R &= ~(GPIO_PCTL_PC4_M | GPIO_PCTL_PC5_M);          // first clear everything for the pins
    GPIO_PORTC_PCTL_R |= GPIO_PCTL_PC4_M0PWM6 | GPIO_PCTL_PC5_M0PWM7;   // then map to M0PWM6 and M0PWM7

    // next we have to clear the PWM clock
    SYSCTL_RCC_R &= ~SYSCTL_RCC_PWMDIV_M;   // value is E = 1110, ~E = [000]1, which sets the PWM divisor to default /2

    // we enable the PWM clock and make it (system clock / 4) = (40 MHz / 4) = 10 MHz clock for PWM
    SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_4;

    // before modifying the PWM0 values, we turn it off
    PWM0_CTL_R = 0;

    // we will make it so that when we load the starting counter
    // value, it will start a new cycle at logic level high, and
    // then the state will go low once the counter value hits the
    // CMPA value for M0PWM6 (PC4) or the CMPB value for M0PWM7 (PC5)
    PWM0_3_GENA_R = PWM_3_GENA_ACTLOAD_M | PWM_3_GENA_ACTCMPAD_ZERO;    // PC4 = Generator 3 A output
    PWM0_3_GENB_R = PWM_3_GENB_ACTLOAD_M | PWM_3_GENB_ACTCMPBD_ZERO;    // PC5 = Generator 3 B output

    // Now we will put the reload value for a 10 KHz output signal
    // Calculated as follows: PWM Clock Source / desired clock
    // 10,000,000 / 10,000 = 1000
    // and we subtract 1 since it counts 0 as a value
    PWM0_3_LOAD_R = PWM_RELOAD - 1;

    // initialize both PWM outputs as off until something calls to set the duty cycle
    set_m0pwm6_duty_cycle(0);
    set_m0pwm7_duty_cycle(0);

    // after modifying the values we update PWM0 to be synced
    PWM0_CTL_R |= PWM_CTL_GLOBALSYNC0;

    PWM0_ENABLE_R |= PWM_ENABLE_PWM6EN | PWM_ENABLE_PWM7EN; // enable M0PWM6 and M0PWM7 signals for PC4 and PC5
    PWM0_3_CTL_R |= PWM_3_CTL_ENABLE;                       // enable PWM module 0, generator 3
}
