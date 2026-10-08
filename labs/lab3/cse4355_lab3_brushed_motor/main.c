#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "gpio.h"
#include "uart0.h"
#include "wait.h"
#include "strings.h"
#include "pwm_m0gen3.h"
#include "adc0.h"
#include "timer0.h"
#include "timer1.h"
#include "timer2.h"
#include "wtimer1.h"
#include "buttons.h"
#include "brushed_motor.h"

#define SYS_CLK     40000000    // System Clock

void init_HW(void)
{
    initSystemClockTo40Mhz();   // init system clock

    initUart0();
    setUart0BaudRate(115200, SYS_CLK);

    init_buttons();     // init on board push buttons

    // flash blue and green LED at start to ensure it reset
    OB_BLUE = 1;
    waitMicrosecond(250000);
    OB_BLUE = 0;
    waitMicrosecond(250000);
    OB_GREEN = 1;
    waitMicrosecond(250000);
    OB_GREEN = 0;

    init_brushed_motor();   // init everything for brushed motor
}

void print_status(void)
{
    char buffer[12];

    // part 1: measuring data with optical switch
    putsUart0("\r\n");
    putsUart0("\r\nDuty cycle   : ");
    putsUart0(toAsciiDec(buffer, motor_get_duty_percent()));
    putsUart0(" %");
    putsUart0("\r\nOptical freq : ");
    putsUart0(toAsciiDec(buffer, motor_get_optical_freq()));
    putsUart0(" Hz");
    putsUart0("\r\nOptical rpm  : ");
    putsUart0(toAsciiDec(buffer, motor_get_optical_rpm()));
    putsUart0(" rpm");

    // part 2: measuring data with back emf and ADC
#ifdef PART2
    putsUart0("\r\nRaw ADC      : ");
    putsUart0(toAsciiDec(buffer, motor_get_bemf_raw()));
    putsUart0("\r\nDrain volts  : ");
    putsUart0(toAsciiDec(buffer, motor_get_drain_mv()));
    putsUart0(" mV");
    putsUart0("\r\nBEMF volts   : ");
    putsUart0(toAsciiDec(buffer, motor_get_bemf_mv()));
    putsUart0(" mV");
    putsUart0("\r\nBEMF rpm     : ");
    putsUart0(toAsciiDec(buffer, motor_get_bemf_rpm()));
    putsUart0(" rpm");
#endif
}

int main(void)
{
    init_HW();  // initialize all hardware

    putsUart0("\r\nStarting Mechatronics Lab 3...");

    while (1)
    {
        process_buttons();

        if (motor_optical_ready())
        {
            print_status();
        }
    }

    // return 0;
}
