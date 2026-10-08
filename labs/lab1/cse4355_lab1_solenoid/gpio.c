// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL with LCD/Keyboard Interface
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// GPIO APB ports A-F

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"

/*
 *  a couple things to recall:
 *
 *  Bit_Band_Address = Bit_Band_Base + ( (OG_Address - OG_Address_Base) * 32 ) + ( (Bit_Number) * 4 )
 *
 *  Bit_Band_Base = 4200.0000 (page 98, for GPIO/peripheral)
 *  OG_Address_Base = 4000.0000
 *
 *  The GPIO Data Register Address has offset from 0x000 to 0x400. If you convert it to bits you have
 *  0000.0000.0000. From this offset we can control which pin number to control. Specifically,
 *  bits 2 to 9 so 00|00.0000.00|00. We choose 3FC so that we can activate all the pins, so it
 *  becomes 00|11.1111.11|00. And choose bit 4 to modify bit 4. (page 654)
 *
 *  these macros are basically just the offset between the
 *  GPIO data registers and each of the registers listed
 *  they match all of their respective register pages (starting pg 663)
 *
 *  for example, OFS_DATA_TO_DIR
 *  previously, we said the offset of data = 0x3FC
 *  datasheet page 663 tells us dir offser = 0x400
 *  0x400 - 0x3FC = 0x004 = 4 bytes because 1 address = 1 byte
 *  4 bytes = 4*8 = 32 bits as the offset
 *
 *  by the way, the PORT port ENUM is literally just an ENUM containing
 *  the bit banded addresses to each GPIO Port Data Register bit 0
 */

// Bit offset of the registers relative to bit 0 of DATA_R at 3FCh
// reg offset x 4 bytes / reg x 8 bits / byte

#define OFS_DATA_TO_DIR    1*4*8 // = 32    ( (0x400 - 0x3FC) = 4   * 8 )
#define OFS_DATA_TO_IS     2*4*8 // = 64    ( (0x404 - 0x3FC) = 8   * 8 )
#define OFS_DATA_TO_IBE    3*4*8 // = 96    ( (0x408 - 0x3FC) = 12  * 8 )
#define OFS_DATA_TO_IEV    4*4*8 // = 128   ( (0x40C - 0x3FC) = 16  * 8 )
#define OFS_DATA_TO_IM     5*4*8 // = 160   ( (0x410 - 0x3FC) = 20  * 8 )
#define OFS_DATA_TO_IC     8*4*8 // = 256   ( (0x41C - 0x3FC) = 32  * 8 )
#define OFS_DATA_TO_AFSEL  9*4*8 // = 288   ( (0x420 - 0x3FC) = 36  * 8 )
#define OFS_DATA_TO_ODR   68*4*8 // = 2176  ( (0x50C - 0x3FC) = 272 * 8 )
#define OFS_DATA_TO_PUR   69*4*8 // = 2208  ( (0x510 - 0x3FC) = 276 * 8 )
#define OFS_DATA_TO_PDR   70*4*8 // = 2240  ( (0x514 - 0x3FC) = 280 * 8 )
#define OFS_DATA_TO_DEN   72*4*8 // = 2304  ( (0x51C - 0x3FC) = 288 * 8 )
#define OFS_DATA_TO_CR    74*4*8 // = 2368  ( (0x524 - 0x3FC) = 296 * 8 )
#define OFS_DATA_TO_AMSEL 75*4*8 // = 2400  ( (0x528 - 0x3FC) = 300 * 8 )

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// this function literally just enables the correct GPIO
// Port clocks, and it forces it to use the APB (Advanced Peripheral Bus)
// instead of the AHB (Advanced High-Performance Bus)
void enablePort(PORT port)
{
    switch(port)
    {
        case PORTA:
            SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0;
            SYSCTL_GPIOHBCTL_R &= ~1;
            break;
        case PORTB:
            SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;
            SYSCTL_GPIOHBCTL_R &= ~2;
            break;
        case PORTC:
            SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2;
            SYSCTL_GPIOHBCTL_R &= ~4;
            break;
        case PORTD:
            SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R3;
            SYSCTL_GPIOHBCTL_R &= ~8;
            break;
        case PORTE:
            SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4;
            SYSCTL_GPIOHBCTL_R &= ~16;
            break;
        case PORTF:
            SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
            SYSCTL_GPIOHBCTL_R &= ~32;
    }
    _delay_cycles(3);
}

// correspondingly disables the correct port
void disablePort(PORT port)
{
    switch(port)
    {
        case PORTA:
            SYSCTL_RCGCGPIO_R &= ~SYSCTL_RCGCGPIO_R0;
            break;
        case PORTB:
            SYSCTL_RCGCGPIO_R &= ~SYSCTL_RCGCGPIO_R1;
            break;
        case PORTC:
            SYSCTL_RCGCGPIO_R &= ~SYSCTL_RCGCGPIO_R2;
            break;
        case PORTD:
            SYSCTL_RCGCGPIO_R &= ~SYSCTL_RCGCGPIO_R3;
            break;
        case PORTE:
            SYSCTL_RCGCGPIO_R &= ~SYSCTL_RCGCGPIO_R4;
            break;
        case PORTF:
            SYSCTL_RCGCGPIO_R &= ~SYSCTL_RCGCGPIO_R5;
    }
    _delay_cycles(3);
}

/*
 *  this, and all functions below it follow the same structure basically:
 *
 *  You send in the correct PORT, and pin number, then it will correctly set the pin
 *
 *  it works because, the PORT is already the bitbanded address of bit 0
 *  of the DATA register. Then, we can just add the offset from the data register
 *  to it, and we have the correct register. Then, from the bit band formula we
 *  still need to add the bit number, and we can just add the pin.
 *
 *  we can do this because the PORT address is casted as a pointer, so C
 *  does pointer arithmetic and when we add it it will automatically
 *  move 4 bytes, so no need for the (bit_number *4), instead its simply:
 *
 *  correct address to write to = (GPIO Port DATA base) + (Offset from base) + (Pin #)
 *
 */

void selectPinPushPullOutput(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_ODR;
    *p = 0;
    p = (uint32_t *)port + pin + OFS_DATA_TO_DIR;
    *p = 1;
    p = (uint32_t *)port + pin + OFS_DATA_TO_DEN;
    *p = 1;
}

void selectPinOpenDrainOutput(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_ODR;
    *p = 1;
    p = (uint32_t *)port + pin + OFS_DATA_TO_DIR;
    *p = 1;
    p = (uint32_t *)port + pin + OFS_DATA_TO_DEN;
    *p = 1;
}

void selectPinDigitalInput(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_DIR;
    *p = 0;
    p = (uint32_t *)port + pin + OFS_DATA_TO_DEN;
    *p = 1;
    p = (uint32_t *)port + pin + OFS_DATA_TO_AMSEL;
    *p = 0;
}

void selectPinAnalogInput(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_DEN;
    *p = 0;
    p = (uint32_t *)port + pin + OFS_DATA_TO_AMSEL;
    *p = 1;
    p = (uint32_t *)port + pin + OFS_DATA_TO_AFSEL;
    *p = 1;
}

void setPinCommitControl(PORT port, uint8_t pin)
{
    switch(port)
    {
        case PORTA:
            GPIO_PORTA_LOCK_R = GPIO_LOCK_KEY;
            break;
        case PORTB:
            GPIO_PORTB_LOCK_R = GPIO_LOCK_KEY;
            break;
        case PORTC:
            GPIO_PORTC_LOCK_R = GPIO_LOCK_KEY;
            break;
        case PORTD:
            GPIO_PORTD_LOCK_R = GPIO_LOCK_KEY;
            break;
        case PORTE:
            GPIO_PORTE_LOCK_R = GPIO_LOCK_KEY;
            break;
        case PORTF:
            GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
    }
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_CR;
    *p = 1;
}

void enablePinPullup(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_PUR;
    *p = 1;
}

void disablePinPullup(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_PUR;
    *p = 0;
}

void enablePinPulldown(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_PDR;
    *p = 1;
}

void disablePinPulldown(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_PDR;
    *p = 0;
}

/*
 *  the setPinAuxFunction is the function that is called
 *  when we wanna set a GPIO pin to a peripheral like UART or SPI
 *
 *  #define UART_TX PORTA,1
 *  GPIO_PCTL_PA1_U0TX = 0x00000010 = 16
 *
 *  it is helpful to look at an example, so lets call setPinAuxFunction(UART_TX, GPIO_PCTL_PA1_U0TX);
 *
 *  So, in the uart0.c without the library we clear corresponding PCTL pin, then set it, and also set AFSEL.
 *
 *  In the function call, fn > 15 so it means the pin is pre-shifted meaning that we will just mask
 *  just to double check. Then, we go into case PORTA, and we:
 *
 *  (1) we CLEAR the pin first by ANDing with the negation of the shifted 0x0..F
 *  (2) we OR (SET) it to the corresponding peripheral function macro we sent in
 *
 *  At the end, we set the AFSEL if we actually use an aux function
 *
 */

void setPinAuxFunction(PORT port, uint8_t pin, uint32_t fn)
{
    // call with header file shifted values or 4-bit number
    if (fn <= 15)
        fn = fn << (pin*4); // shift to get correct pin to set
    else
        fn = fn & (0x0000000F << (pin*4)); // mask it, if it was already shifted
    switch(port)
    {
        case PORTA:
            GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & ~(0x0000000F << (pin*4))) | fn;
            break;
        case PORTB:
            GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & ~(0x0000000F << (pin*4))) | fn;
            break;
        case PORTC:
            GPIO_PORTC_PCTL_R = (GPIO_PORTC_PCTL_R & ~(0x0000000F << (pin*4))) | fn;
            break;
        case PORTD:
            GPIO_PORTD_PCTL_R = (GPIO_PORTD_PCTL_R & ~(0x0000000F << (pin*4))) | fn;
            break;
        case PORTE:
            GPIO_PORTE_PCTL_R = (GPIO_PORTE_PCTL_R & ~(0x0000000F << (pin*4))) | fn;
            break;
        case PORTF:
            GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R & ~(0x0000000F << (pin*4))) | fn;
    }
    // set AFSEL bit only if using aux function, otherwise clear bit
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_AFSEL;
    *p = (fn > 0);
}

void selectPinInterruptRisingEdge(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_IS;
    *p = 0;
    p = (uint32_t *)port + pin + OFS_DATA_TO_IBE;
    *p = 0;
    p = (uint32_t *)port + pin + OFS_DATA_TO_IEV;
    *p = 1;
}

void selectPinInterruptFallingEdge(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_IS;
    *p = 0;
    p = (uint32_t *)port + pin + OFS_DATA_TO_IBE;
    *p = 0;
    p = (uint32_t *)port + pin + OFS_DATA_TO_IEV;
    *p = 0;
}

void selectPinInterruptBothEdges(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_IS;
    *p = 0;
    p = (uint32_t *)port + pin + OFS_DATA_TO_IBE;
    *p = 1;
}

void selectPinInterruptHighLevel(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_IS;
    *p = 1;
    p = (uint32_t *)port + pin + OFS_DATA_TO_IEV;
    *p = 1;
}

void selectPinInterruptLowLevel(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_IS;
    *p = 1;
    p = (uint32_t *)port + pin + OFS_DATA_TO_IEV;
    *p = 0;
}

void enablePinInterrupt(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_IM;
    *p = 1;
}

void disablePinInterrupt(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_IM;
    *p = 0;
}

void clearPinInterrupt(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin + OFS_DATA_TO_IC;
    *p = 1;
}

void setPinValue(PORT port, uint8_t pin, bool value)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin;
    *p = value;
}

void togglePinValue(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin;
    *p ^= 1;
}

bool getPinValue(PORT port, uint8_t pin)
{
    volatile uint32_t *p;
    p = (uint32_t *)port + pin;
    return *p;
}

void setPortValue(PORT port, uint8_t value)
{
    switch(port)
    {
        case PORTA:
            GPIO_PORTA_DATA_R = value;
            break;
        case PORTB:
            GPIO_PORTB_DATA_R = value;
            break;
        case PORTC:
            GPIO_PORTC_DATA_R = value;
            break;
        case PORTD:
            GPIO_PORTD_DATA_R = value;
            break;
        case PORTE:
            GPIO_PORTE_DATA_R = value;
            break;
        case PORTF:
            GPIO_PORTF_DATA_R = value;
    }
}

uint8_t getPortValue(PORT port)
{
    uint8_t value;
    switch(port)
    {
        case PORTA:
            value = GPIO_PORTA_DATA_R;
            break;
        case PORTB:
            value = GPIO_PORTB_DATA_R;
            break;
        case PORTC:
            value = GPIO_PORTC_DATA_R;
            break;
        case PORTD:
            value = GPIO_PORTD_DATA_R;
            break;
        case PORTE:
            value = GPIO_PORTE_DATA_R;
            break;
        case PORTF:
            value = GPIO_PORTF_DATA_R;
    }
    return value;
}
