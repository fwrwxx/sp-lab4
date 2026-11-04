#ifndef CMD_H
#define CMD_H
#include <stdint.h>

void cmd_rx_byte(uint8_t b);   /* викликати з USART2 ISR */
void cmd_poll(void);           /* викликати з super loop */
void cmd_handle_line(char *line);

#endif