#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "wait.h"
#include "uart0.h"
#include "uart_shell.h"
#include "common_terminal_interface.h"
#include "strings.h"
#include "stepper_motor.h"

void reboot_cmd(void)
{
    putsUart0("\r\nRebooting TM4C...\r\n");
    waitMicrosecond(10000);

    // datasheet page 164
    // to write to the register, you need to write 0x05FA to upper 16 bits (register key)
    // then write 1 to the System Reset Request field (bit 2)
    NVIC_APINT_R = (0x05FA << 16) | (1 << 2);
}

static void microstep_cmd(USER_DATA *input)
{
    if (str_cmp(getFieldString(input, 1), "angle") == 0)
    {
        float angle = getFieldFloat(input, 2);
        step_14(angle);
    }
    else
    {
        putsUart0("\r\nInvalid Command: microstep angle [deg]\r\n");
    }
}

static void angle_cmd(USER_DATA *input)
{
    float angle = getFieldFloat(input, 1);
    step_12(angle);
}

void help_cmd(void)
{
    putsUart0("\r\n------------------- CSE 4355 Lab 2: Stepper Motors Shell Interface -------------------\r\n");
    putsUart0(" Command List:\r\n");
    putsUart0("  reboot                 : reboots the microcontroller\r\n");
    putsUart0("  step9                  : rotate motor 16 steps CW then 16 steps CCW\r\n");
    putsUart0("  zero                   : calibrate with phototransistor to zero the beam\r\n");
    putsUart0("  angle [deg]            : moves the beam to the input angle (from zero reference)\r\n");
    putsUart0("  microstep angle [deg]  : moves the beam to the input angle using 0.225 deg microsteps\r\n");
    putsUart0("  help                   : display this menu\r\n");
    putsUart0("--------------------------------------------------------------------------------------\r\n");
}

void process_shell(USER_DATA *input)
{
    if (isCommand(input, "reboot", 0))
    {
        reboot_cmd();
    }
    else if ((isCommand(input, "help", 0)))
    {
        help_cmd();
    }
    else if (isCommand(input, "step9", 0))
    {
        step_9();
    }
    else if (isCommand(input, "zero", 0))
    {
        step_11();
    }
    else if (isCommand(input, "angle", 1))
    {
        angle_cmd(input);
    }
    else if (isCommand(input, "angle", 0))
    {
        putsUart0("\r\nInvalid Command: angle [deg]\r\n");
    }
    else if (isCommand(input, "microstep", 2))
    {
        microstep_cmd(input);
    }
    else if (isCommand(input, "microstep", 0))
    {
        putsUart0("\r\nInvalid Command: microstep angle [deg]\r\n");
    }
    else
    {
        putsUart0("\r\nInvalid command: unknown command \r\n");
    }

    putsUart0("> ");
}
