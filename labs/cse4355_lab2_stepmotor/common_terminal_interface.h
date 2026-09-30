#ifndef COMMON_TERMINAL_INTERFACE_H_
#define COMMON_TERMINAL_INTERFACE_H_

#include <stdint.h>
#include <stdbool.h>

#define MAX_CHARS 80
#define MAX_FIELDS 5

typedef struct _USER_DATA
{
    char buffer[MAX_CHARS + 1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
}
USER_DATA;

void getsUart0(USER_DATA *input);
void parseFields(USER_DATA *input);
char* getFieldString(USER_DATA *input, uint8_t fieldNumber);
float getFieldFloat(USER_DATA *input, uint8_t fieldNumber);
int32_t getFieldInteger(USER_DATA *input, uint8_t fieldNumber);
bool isCommand(USER_DATA *input, const char strCommand[], uint8_t minArguments);

#endif
