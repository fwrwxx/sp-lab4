#include "cmd.h"
#include "tasks.h"
#include "perf.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CMD_BUF_SIZE 128
static char cmd_buf[CMD_BUF_SIZE];
static uint32_t cmd_len;
static volatile uint8_t cmd_line_ready;
static char cmd_line[CMD_BUF_SIZE];

void cmd_rx_byte(uint8_t b)
{
    if (b == '\r' || b == '\n' || b == ' ' || b == '\t') {
        if (cmd_len) {
            cmd_buf[cmd_len] = '\0';
            memcpy(cmd_line, cmd_buf, cmd_len + 1);
            cmd_len = 0;
            cmd_line_ready = 1;
        }
    } else if (cmd_len < CMD_BUF_SIZE - 1) {
        cmd_buf[cmd_len++] = b;
    } else {
        cmd_len = 0;
    }
}

/* Викликається з super loop - реальна обробка */
void cmd_poll(void)
{
    if (!cmd_line_ready) return;
    cmd_line_ready = 0;
    cmd_handle_line(cmd_line);
}

void cmd_handle_line(char *line)
{
    if      (!strcmp(line, "STATS")) tasks_print_stats();
    else if (!strcmp(line, "RESET")) { tasks_reset_stats(); printf("OK RESET\r\n"); }
    else if (!strcmp(line, "LOAD 2")) printf("OK LOAD S2\r\n");
    else if (!strcmp(line, "LOAD 3")) printf("OK LOAD S3\r\n");
    else if (!strcmp(line, "LONG"))   { long_op_start(); printf("OK LONG\r\n"); }
    else if (!strcmp(line, "STACK"))  {
        extern uint32_t stack_used_bytes(void);
        printf("STACK used=%lu\r\n", (unsigned long)stack_used_bytes());
    } else if (!strcmp(line, "HEAP")) {
        extern void heap_report(void);
        heap_report();
    } else {
        printf("ERR unknown\r\n");
    }
}