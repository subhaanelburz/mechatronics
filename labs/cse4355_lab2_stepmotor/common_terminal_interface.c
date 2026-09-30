#include <stdint.h>
#include <stdbool.h>
#include "common_terminal_interface.h"
#include "uart0.h"
#include "strings.h"

// this function is what receives the input string from the terminal
void getsUart0(USER_DATA *input)
{
    uint8_t i = 0;
    char temp_char;

    while(1)
    {
        // while there is no data in UART RX FIFO
        while (!kbhitUart0());

        // read each inputted character from terminal and store into temp variable
        temp_char = getcUart0();

        // check if backspace (8 or 127) as long as there is more than 1 character
        // since you should not delete non existent characters
        if (((temp_char == 8) || (temp_char == 127)) && (i > 0))
        {
            i--;
        }
        else if ((temp_char == 13) || (temp_char == 10)) // if carriage return / line feed (enter), then add null terminator and exit
        {
            input->buffer[i] = 0;
            break;
        }
        else if ((temp_char >= 32) && (i < MAX_CHARS)) // if valid character, store it and move onto next character
        {
            // convert all uppercase characters to lowercase for case insensitivity
            if (temp_char >= 'A' && temp_char <= 'Z')
            {
                temp_char += 32;
            }

            input->buffer[i] = temp_char;
            i++;
        }

        // if char limit reached, then add null terminator and exit
        if (i == MAX_CHARS)
        {
            input->buffer[i] = 0;
            break;
        }
    }
}

// this function will basically tokenize / parse the inputted string into tokens / subfields
void parseFields(USER_DATA *input)
{
    input->fieldCount = 0;  // counter for how many tokens are in the inputted string, initially 0
    char prev_type = 'd';   // make the previous type a delimiter initially, so we can count a new field initially

    uint8_t i = 0;

    while (input->buffer[i] != '\0' && input->fieldCount < MAX_FIELDS)
    {
        char current_char = input->buffer[i];

        // We are iterating through the input, and initially the previous character is set to be
        // a delimiter, so we can start the first field.
        // So we will check the character type, and if the previous type is a delimiter, we will
        // make that to be the start of a new field by updating the field type, position, and count

        // Check if ASCII character value is between A to Z OR a to z OR and symbol OR underscore symbol
        if ( (current_char >= 'A' && current_char <= 'Z') || (current_char >= 'a' && current_char <= 'z') || (current_char == '&') || (current_char == '_') )
        {
            if (prev_type == 'd')
            {
                input->fieldType[input->fieldCount] = 'a';
                input->fieldPosition[input->fieldCount] = i;
                input->fieldCount++;
            }
            prev_type = 'a';
        }
        // Check if 0 to 9, negative sign, and if decimal point
        else if ( (current_char >= '0' && current_char <= '9') || (current_char == '-' && prev_type == 'd') || (current_char == '.' && prev_type == 'n') )
        {
            if (prev_type == 'd')
            {
                input->fieldType[input->fieldCount] = 'n';
                input->fieldPosition[input->fieldCount] = i;
                input->fieldCount++;
            }
            prev_type = 'n';
        }
        else
        {
            prev_type = 'd';
            input->buffer[i] = '\0';
            // change any delimiters that are detected to \0 so that we know when each field ends
        }
        i++;
    }
}

// returns the address of a field requested if the field number is in range or NULL otherwise
char* getFieldString(USER_DATA *input, uint8_t fieldNumber)
{
    if (fieldNumber < input->fieldCount)
    {
        return &input->buffer[input->fieldPosition[fieldNumber]];
    }
    else
    {
        return 0; // return NULL but u can just return 0
    }
}

// returns the float value of a field instead of int (exactly like getFieldInteger)
float getFieldFloat(USER_DATA *input, uint8_t fieldNumber)
{
    // if the field number is in the range and the field type is numeric
    if ( (fieldNumber < input->fieldCount) && (input->fieldType[fieldNumber] == 'n') )
    {
        float value = 0;    // we calculate this by processing each digit of the number
        float digit = 0;    // and every time we have a digit we x10 so it can update properly
        float fraction = 1; // after the decimal point we /10
        bool negative = false;
        bool past_decimal = false;

        uint8_t i = input->fieldPosition[fieldNumber];  // i is location in buffer of the number field

        if (input->buffer[i] == '-')
        {
            negative = true;    // if there is negative sign set it to be true
            i++;                // increment ahead of negative sign for numbers
        }

        // this is basically atof()
        while (input->buffer[i] != '\0')
        {
            if (input->buffer[i] == '.')
            {
                past_decimal = true;  // switch to fractional mode once we hit the point
                i++;
                continue;
            }

            digit = input->buffer[i] - '0';     // ex: '7' = 55, '0' = 48: '7' - '0' = 55 - 48 = 7

            if (!past_decimal)
            {
                value = (value * 10) + digit;
            }
            else
            {
                fraction = fraction / 10;
                value = value + (digit * fraction);
            }

            i++;
        }

        if (negative)
        {
            value = (value * -1);
        }

        return value;
    }
    else
    {
        // return 0 if not in range or numeric
        return 0;
    }
}

// returns the integer value of a field if the field number is in range and the field type is numeric or 0 otherwise
int32_t getFieldInteger(USER_DATA *input, uint8_t fieldNumber)
{
    if ( (fieldNumber < input->fieldCount) && (input->fieldType[fieldNumber] == 'n') )
    {
        int32_t value = 0;  // we calculate this by processing each digit of the number
        int32_t digit = 0;  // and every time we have a digit we x10 so it can update properly
        bool negative = false;

        uint8_t i = input->fieldPosition[fieldNumber];  // i is location in buffer of the number field

        if (input->buffer[i] == '-')
        {
            negative = true;    // if there is negative sign set it to be true
            i++;                // increment ahead of negative sign for numbers
        }

        // this is basically atoi()
        while (input->buffer[i] != '\0')
        {
            digit = input->buffer[i] - '0'; // ex: '7' = 55, '0' = 48
            value = (value * 10) + digit;   //     '7' - '0' = 55 - 48 = 7
            i++;
        }
        // ex: 743; i = 0: value = 7, i = 1: value = 74, i = 2: value = 743

        if (negative)
        {
            value = (value * -1);
        }

        return value;
    }
    else
    {
        return 0;
    }
}

// returns true if the command matches the first field and the number of arguments (excluding the command field)
// is greater than or equal to the requested number of minimum arguments
bool isCommand(USER_DATA *input, const char strCommand[], uint8_t minArguments)
{
    char *input_command = getFieldString(input, 0); // get the inputted command (field 0)

    // We will return that it is a valid command if the command is the same string and also
    // if there is the minimum number of arguments excluding the command, so fieldCount - 1
    if ( (str_cmp(input_command, strCommand) == 0) && (input->fieldCount - 1 >= minArguments) )
    {
        return true;
    }
    else
    {
        return false;
    }
}
