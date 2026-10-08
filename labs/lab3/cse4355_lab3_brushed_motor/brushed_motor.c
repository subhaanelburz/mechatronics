#include <stdint.h>
#include <stdbool.h>
#include "brushed_motor.h"
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "wait.h"
#include "pwm_m0gen3.h"
#include "adc0.h"
#include "timer1.h"
#include "timer2.h"
#include "wtimer1.h"

// timer1 fires every second, so 1 Hz for the time interval
// this timer is the same as the 32 bit count down timer in the freq_time example
#define TIMER1_RATE 1

// timer2 fires 50 times a second, so it is at 50 Hz
// and every time it fires we measure the back emf voltage by disabling PWM and using ADC
#define TIMER2_RATE 50

// time in microseconds to wait after turning PWM off before reading the ADC
#define PWM_WAIT 800

// initial duty cycle for PWM; 500 = 50% duty cycle
static volatile uint32_t duty_cycle = 500;

static volatile uint32_t optical_freq = 0;  // the frequency measured in the timer1 handler
static volatile bool optical_ready = false; // flag to update when a new measurement is available

#ifdef PART2
static volatile uint32_t bemf_raw = 0;  // the latest raw ADC value updated in the timer2 handler
#endif

void timer1_handler(void)
{
    TIMER1_ICR_R = TIMER_ICR_TATOCINT;  // first clear the interrupt

    // update the optical frequency, clear the count, and set the flag
    optical_freq = get_wtimer1_count() * TIMER1_RATE;
    clear_wtimer1_count();
    optical_ready = true;
}

void init_brushed_motor(void)
{
    // init PWM module 0, generator 3 (both PC4 and PC5)
    init_PWM_m0gen3();

    // set PC4 (M0PWM6) to 50% duty cycle
    set_m0pwm6_duty_cycle(duty_cycle);

    // init wide timer 1 and set PC6 as WT1CCP0 pin
    // used to count edges for the optical switch
    init_wtimer1();

    // initialize ADC on AIN3 pin (PE0)
#ifdef PART2
    enablePort(PORTE);
    selectPinAnalogInput(PORTE, 0);
    initAdc0Ss3();
    // Use AIN3 input with N=4 hardware sampling
    setAdc0Ss3Mux(3);
    setAdc0Ss3Log2AverageCount(2);
#endif

    // start the frequency mode from freq_time example
    clear_wtimer1_count();
    init_timer1(TIMER1_RATE);

    // initialize back emf sampling
#ifdef PART2
    init_timer2(TIMER2_RATE);
#endif
}

// increase duty cycle by 10% (+100) if not over max PWM value (1000)
void motor_duty_up(void)
{
    if (duty_cycle + 100 <= 1000)
    {
        duty_cycle += 100;
        set_m0pwm6_duty_cycle(duty_cycle);
    }
}

// decrease duty cycle by 10% (-100) if greater than min PWM value (0)
void motor_duty_down(void)
{
    if (duty_cycle > 0)
    {
        duty_cycle -= 100;
        set_m0pwm6_duty_cycle(duty_cycle);
    }
}

// simply 500 duty cycle / 10 = 50 or 50% duty cycle
uint32_t motor_get_duty_percent(void)
{
    return duty_cycle / 10;
}

// simply once the measurement is ready return true and clear the flag
bool motor_optical_ready(void)
{
    if (optical_ready)
    {
        optical_ready = false;
        return true;
    }
    return false;
}

// simply return the optical freq
uint32_t motor_get_optical_freq(void)
{
    return optical_freq;
}

uint32_t motor_get_optical_rpm(void)
{
    // first convert the optical_freq from pulses per second to pulses per minute
    uint32_t pulses_per_min = optical_freq * 60;
    uint32_t rounding_offset = 32 / 2;

    // then calculate the revolutions per minute
    // dividing the pulses per minute by the pulses per revolution
    uint32_t calculated_rpm = (pulses_per_min + rounding_offset) / 32;

    return calculated_rpm;
}

void timer2_handler(void)
{
    TIMER2_ICR_R = TIMER_ICR_TATOCINT;  // first clear the interrupt

    // turn off PWM, wait for it to settle, read the ADC value, and turn PWM back on
#ifdef PART2
    set_m0pwm6_duty_cycle(0);
    waitMicrosecond(PWM_WAIT);
    bemf_raw = (uint32_t) readAdc0Ss3();
    set_m0pwm6_duty_cycle(duty_cycle);
#endif
}

#ifdef PART2
// simply return the raw back emf value
uint32_t motor_get_bemf_raw(void)
{
    return bemf_raw;
}

// simply returns the drain voltage in mV, converting from the raw ADC value
uint32_t motor_get_drain_mv(void)
{
    // save the raw adc voltage
    uint32_t raw_adc = bemf_raw;

    // convert the raw adc value to mV
    // formula: raw value * ref voltage / total ADC resolution steps
    uint32_t adc_pin_mv = (raw_adc * 3300) / 4096;

    // then lastly account for the 47k/10k voltage divider
    // R1 = 47k
    // R2 = 10k (to GND)
    // Vo = Vin * R2 / (R1+R2)
    // Vin = Vo * (R1+R2) / R2 = Vo * 57 / 10
    uint32_t drain_mv = (adc_pin_mv * 57) / 10;

    return drain_mv;
}

// simply return the back emf voltage in mV
// the back emf voltage = supply voltage - drain voltage
uint32_t motor_get_bemf_mv(void)
{
    uint32_t supply_mv = 10000; // 10 V supply
    uint32_t drain_mv = motor_get_drain_mv();

    // if the drain reads higher than supply, clamp to 0
    if (drain_mv > supply_mv)
    {
        return 0;
    }

    return supply_mv - drain_mv;
}

uint32_t motor_get_bemf_rpm(void)
{
    float stopped_raw = 2147.0f;        // raw ADC reading when motor is at 0 rpm
    float rpm_per_count = 0.782f;       // rpm for every 1 ADC count

    // save the raw adc value
    float raw_adc = (float) bemf_raw;

    // how many counts the reading has dropped since the motor was stopped
    float bemf_counts = stopped_raw - raw_adc;

    // if it is negative, clamp it to zero
    if (bemf_counts < 0.0f)
    {
        bemf_counts = 0.0f;
    }

    // convert the back emf counts to rpm
    float calculated_rpm = bemf_counts * rpm_per_count;

    // round to the nearest whole rpm
    return (uint32_t) (calculated_rpm + 0.5f);
}
#endif
