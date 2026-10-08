#ifndef UART_SHELL_H_
#define UART_SHELL_H_

#include "common_terminal_interface.h"

void process_shell(USER_DATA *input);
void reboot_cmd(void);
void help_cmd(void);

#endif
