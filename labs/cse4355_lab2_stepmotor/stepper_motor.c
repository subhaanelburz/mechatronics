#include <stdint.h>
#include <math.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "wait.h"
#include "stepper_motor.h"
#include "pwm_m0gen3.h"

// coil A signals
// Enable A is the two blues tied together
// DIR A is white
// NOT_DIR A is other white
#define PWM_ENABLE_A    (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 4*4))) // PC4 (not used anymore, PWM used)
#define DIR_IN1_A       (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 1*4))) // PE1
#define NOT_DIR_IN2_A   (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 2*4))) // PE2

// coil B signals
// Enable B is the other two blues tied together
// DIR B is white
// NOT_DIR B is other white
#define PWM_ENABLE_B    (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 5*4))) // PC5 (not used anymore, PWM used)
#define DIR_IN1_B       (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 4*4))) // PE4
#define NOT_DIR_IN2_B   (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 5*4))) // PE5

// input to phototransistor collector (purple)
#define PT_COLLECTOR    (*((volatile uint32_t *)(0x42000000 + (0x400073FC-0x40000000)*32 + 6*4))) // PD6

// calibrated number of steps to go backwards to balance the
// beam once the phototransistor collector reads 1
// this is in terms of 1.8 degree steps
#define CALIBRATED_STEPS 22

#define LUT_SIZE 32         // microstep LUT size; 4 steps * 8 microsteps per step = 32 microstep table
#define PI 3.14159265359    // value of PI used in equations

// struct to store the values of coils easily
typedef struct
{
    uint8_t en_A;
    uint8_t dir_A;
    uint8_t ndir_A;
    uint8_t en_B;
    uint8_t dir_B;
    uint8_t ndir_B;
}
coil_signals;

// struct to store the LUT values of a single coil
// the LUT is cosine, but sine will be calculated
// sin(x) = cos(x + 270 deg)
typedef struct
{
    uint16_t pwm_duty_cycle;
    uint8_t  dir;
    uint8_t  not_dir;
}
microstep_signal;

microstep_signal microstep_lut[LUT_SIZE];

// lookup table to do the electrical steps
// 4 steps of 1.8 degrees = 1 full electrical step (7.2 degrees)
// index 0 -> 3 moves counterclockwise
coil_signals step_table[4] = {  {1, 1, 0, 0, 0, 0},     // 0 degrees, EnA = 1, DirA = 1, NDirA = 0 for driving N/S = +/-
                                {0, 0, 0, 1, 1, 0},     // 90 degrees, EnB = 1, DirB = 1, NDirB = 0 for driving W/E = +/-
                                {1, 0, 1, 0, 0, 0},     // 180 degrees, EnA = 1, DirA = 0, NDirA = 1 for driving N/S = -/+
                                {0, 0, 0, 1, 0, 1}  };  // 270 degrees, EnB = 1, DirB = 0, NDirB = 1 for driving W/E = -/+

// index to the step table to see current coil values
uint8_t current_index = 0;

// number of 1.8 degree steps we are from the zero position
int32_t current_step_count = 0;

// index to the microstep LUT to see current coil values
uint8_t current_microstep_index = 0;

// number of microsteps we are from the zero position
int32_t current_microstep_count = 0;

// LUTs to find the next/previous index to step to
uint8_t next_step[4] = {1, 2, 3, 0};
uint8_t prev_step[4] = {3, 0, 1, 2};

// initialize the LUT for microstepping
// we have 8 microsteps per step
// so 4 steps * 8 microsteps per step = 32 total microstep LUT
void init_microstep_lut(void)
{
    uint32_t i;

    for (i = 0; i < LUT_SIZE; i++)
    {
        // split unit circle into 32 angles for the LUT
        float angle = ( (2.0f * PI * i) / LUT_SIZE );

        // get the cosine value at the angle
        float cos = cosf(angle);

        // now calculate the PWM duty cycle value from the angle
        microstep_lut[i].pwm_duty_cycle = (uint16_t) roundf(1000.0f * fabsf(cos));

        // then set the dir and not dir coil values
        // conditional same as board
        microstep_lut[i].dir = (cos >= 0) ? 1 : 0;
        microstep_lut[i].not_dir = !microstep_lut[i].dir;
    }
}

// set the coil signals to make a microstep
void set_microstep_signals(uint8_t index)
{
    // calculate the sine index (270 degrees ahead)
    // 32 is full 3/4 of it is 24, then mask it so it doesnt go out of bounds
    uint8_t sine_index = (index + 24) & 0x1F;

    microstep_signal a = microstep_lut[index];
    microstep_signal b = microstep_lut[sine_index];

    // set coil A signals
    set_m0pwm6_duty_cycle(a.pwm_duty_cycle);
    DIR_IN1_A = a.dir;
    NOT_DIR_IN2_A = a.not_dir;

    // set coil B signals
    set_m0pwm7_duty_cycle(b.pwm_duty_cycle);
    DIR_IN1_B = b.dir;
    NOT_DIR_IN2_B = b.not_dir;
}

// set all the coil signals to make a step
void set_coil_signals(uint8_t index)
{
    coil_signals step = step_table[index];

    // set coil A signals
    set_m0pwm6_duty_cycle(step.en_A ? 1000 : 0);
    DIR_IN1_A = step.dir_A;
    NOT_DIR_IN2_A = step.ndir_A;

    // set coil B signals
    set_m0pwm7_duty_cycle(step.en_B ? 1000 : 0);
    DIR_IN1_B = step.dir_B;
    NOT_DIR_IN2_B = step.ndir_B;
}

// step 9 code just wants to rotate the motor 4 electrical steps CW
// then another 4 electrical steps CCW, with a 1 second delay
// 1 electrical step = 7.2 degrees
// 16 * 1.8 = 28.8 degrees
// so 4 full electrical steps: 7.2 * 4 = 28.8 degrees
void step_9(void)
{
    uint8_t i, j;

    // runs 16 times, 16 * 1.8 degrees
    // 4 electrical steps clockwise
    for (i = 0; i < 4; i++)
    {
        for (j = 4; j > 0; j--)
        {
            // call to make step of 1.8 degrees
            set_coil_signals(j - 1);
            waitMicrosecond(250000);
        }
    }

    // runs 16 times, 16 * 1.8 degrees
    // 4 electrical steps counterclockwise
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            // call to make step of 1.8 degrees
            set_coil_signals(j);
            waitMicrosecond(250000);
        }
    }
}

void step_11(void)
{
    uint8_t exit = 0, i, j;

    // loop until the collector reads high, then stop there
    // this section goes clockwise (index goes from 3 -> 0)
    while (1)
    {
        // loop makes one electrical step (7.2 degrees)
        // in each loop, it makes 4 steps of 1.8 degrees
        for (i = 4; i > 0; i--)
        {
            // call to make step of 1.8 degrees
            set_coil_signals(i - 1);
            waitMicrosecond(250000);

            if (PT_COLLECTOR)
            {
                // save index for coil signal values
                j = i - 1;
                exit = 1;
                break;
            }
        }

        if (exit)
        {
            break;
        }
    }

    // now go backwards the calibrated num of steps to level the beam
    // this section goes clockwise
    for (i = 0; i < CALIBRATED_STEPS; i++)
    {
        j = next_step[j];
        set_coil_signals(j);
        waitMicrosecond(250000);
    }

    // save the index to the step table we finish at to record the zero deg location
    current_index = j;
    current_step_count = 0;
    current_microstep_index = j * 8;
    current_microstep_count = 0;
}

void step_12(float angle_deg)
{
    // first convert the angle to the num steps we need to move
    int32_t target_step_count = (int32_t) roundf(angle_deg / 1.8f);

    // then move towards the target angle one step at a time
    while (current_step_count != target_step_count)
    {
        if (target_step_count > current_step_count)
        {
            current_index = next_step[current_index];
            current_step_count++;
        }
        else
        {
            current_index = prev_step[current_index];
            current_step_count--;
        }

        set_coil_signals(current_index);
        waitMicrosecond(250000);
    }
}

void step_14(float angle_deg)
{
    // convert the full stepping (1.8 deg) to the microstep angle
    float microstep_angle = 1.8f / 8;

    // first convert the angle to the num microsteps we need to move
    int32_t target_microstep_count = (int32_t) roundf(angle_deg / microstep_angle);

    // then move towards the target angle one microstep at a time
    while (current_microstep_count != target_microstep_count)
    {
        if (target_microstep_count > current_microstep_count)
        {
            current_microstep_index = (current_microstep_index + 1) & 0x1F;
            current_microstep_count++;
        }
        else
        {
            current_microstep_index = (current_microstep_index + LUT_SIZE - 1) & 0x1F;
            current_microstep_count--;
        }

        set_microstep_signals(current_microstep_index);
        waitMicrosecond(250000);
    }
}

void init_motor(void)
{
    enablePort(PORTE);
    enablePort(PORTD);

    // set coil signals to be outputs
    selectPinPushPullOutput(PORTE, 1);
    selectPinPushPullOutput(PORTE, 2);
    selectPinPushPullOutput(PORTE, 4);
    selectPinPushPullOutput(PORTE, 5);

    // set the phototransistor collector to be an input
    // so we can read when the light hits it for calibration
    selectPinDigitalInput(PORTD, 6);

    // initialize the microstep LUT
    init_microstep_lut();

    // initially clear all values
    DIR_IN1_A = 0;
    NOT_DIR_IN2_A = 0;
    DIR_IN1_B = 0;
    NOT_DIR_IN2_B = 0;
}
