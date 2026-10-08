#include <stdint.h>
#include "strings.h"

char* toAsciiHex(char* buffer, uint32_t value)
{
    // value is a 32 bit number like 0x5555AAAA
    // we need to convert it into a string "5555AAAA"

    uint8_t digit = 0;
    uint8_t i = 0;
    uint8_t shift = 0;

    while(i < 8)
    {
        // OK so we shift 28 bits initially to get the MSB because we need to write MSB first in the string
        // ex: 0x1234.5678 >> 28 would be 0x1 but we need to decrement the shift to the next hex digit
        // which would be 4 bits for every iteration of the loop, so by shifting like this we are
        // filling the string MSB to LSB so it looks the same
        shift = 28 - (i * 4);

        // we will get each hex-digit by shifting every 4 bits and ANDing with only 0xF = 0b1111
        digit = (value >> shift) & 0xF;

        if (digit <= 9)
        {
            buffer[i] = (digit + 48); // if digit is between 0 and 9 we add the ASCII offset
        }
        else
        {
            buffer[i] = (digit + 55); // similarly, we add the ASCII offset for A to F
        }

        i++;
    }

    // add the null terminator to the string
    buffer[8] = 0;

    return buffer;
}

char* toAsciiDec(char* buffer, uint32_t value)
{
    // similarly, convert 32 bit number to decimal string (not hex)

    char temp_buffer[12];
    uint8_t i = 0;
    uint8_t j = 0;

    // if the number is 0 just set and return
    if (value == 0)
    {
        buffer[0] = '0';
        buffer[1] = 0;
        return buffer;
    }

    // extract the digits backward from LSB to MSB
    while (value > 0)
    {
        temp_buffer[i] = (value % 10) + '0';
        value = value / 10;
        i++;
    }

    // now reverse the characters to get actual number as string
    while (i > 0)
    {
        i--;
        buffer[j] = temp_buffer[i];
        j++;
    }

    // add the null terminator to the string
    buffer[j] = 0;

    return buffer;
}

// compares strings
// i made them const char because the strCommand[] is const in the isCommand function
// and it was giving me a warning idk, but i dont think it really matters just makes it constant...
uint32_t str_cmp(const char* str1, const char* str2)
{
    uint8_t i = 0;

    while (str1[i] == str2[i])  // check if strings same at each char
    {
        if ( (str1[i] == '\0') || (str2[i] == '\0') )
        {
            break;  // if one string terminates, then exit loop
        }
        i++;
    }

    if ( (str1[i] == '\0') && (str2[i] == '\0') ) // '\0' = 0 in ASCII
    {
        return 0;   // return that they are the same if both strings terminated
    }
    else    // if strings different / one string terminated before the other then
    {
        return str1[i] - str2[i];   // we return the difference of the strings
        // like if str1 finished, and str2 is longer, then u would have 0 - some number
        // if str2 longer than str1, we return num < 0
        // if str1 longer than str2, we return num > 0
    }
}

// implemented str_len as well similar to str_cmp,
// this function pretty simple just iterate through and
// count each char
uint32_t str_len(const char* str)
{
    uint32_t length = 0;

    while (str[length] != '\0')
    {
        length++;
    }

    return length;
}

// needed to copy the original unparsed command
// so created a custom str_cpy
char *str_cpy(char* paste, char* copy)
{
    uint32_t i = 0;

    while(copy[i] != '\0')
    {
        paste[i] = copy[i];
        i++;
    }
    paste[i] = '\0';
    return paste;
}
