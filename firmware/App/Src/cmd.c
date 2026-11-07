#include "cmd.h"
#include "app_tasks.h"
#include "perf.h"
#include "rtos_tasks.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

#define CMD_BUF_SIZE 128
static char cmd_buf[CMD_BUF_SIZE];
static uint32_t cmd_len;
static volatile uint8_t cmd_line_ready;
static char cmd_line[CMD_BUF_SIZE];

/* Стара функція ЛР3 (у ЛР4 не використовується, бо ISR пише в Stream Buffer) */
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

/* Викликається з vCmdTask */
void cmd_handle_line(char *line)
{
    if      (!strcmp(line, "STATS")) tasks_print_stats();
    else if (!strcmp(line, "RESET")) { tasks_reset_stats(); uart_puts("OK RESET\r\n"); }
    else if (!strcmp(line, "LOAD 2")) uart_puts("OK LOAD S2\r\n");
    else if (!strcmp(line, "LOAD 3")) uart_puts("OK LOAD S3\r\n");
    else if (!strcmp(line, "LONG")) {
        xTaskNotifyGive(hLongOpTask);
        uart_puts("OK LONG\r\n");
    }
    else if (!strcmp(line, "STACK"))  {
        extern uint32_t stack_used_bytes(void);
        uart_puts("STACK used=");
        /* простий вивід без printf */
        char buf[12]; int i = 11; buf[11] = '\0';
        uint32_t v = stack_used_bytes();
        if (v == 0) { buf[--i] = '0'; }
        while (v > 0 && i > 0) { buf[--i] = '0' + (v % 10); v /= 10; }
        uart_puts(&buf[i]);
        uart_puts("\r\n");
    } else if (!strcmp(line, "HEAP")) {
        extern void heap_report(void);
        heap_report();
    }
    else if (!strcmp(line, "TASKS"))     Rtos_CmdTasks();
    else if (!strcmp(line, "STACKRTOS")) Rtos_CmdStack();
    else if (!strcmp(line, "HEAPRTOS"))  Rtos_CmdHeap();
    else if (!strcmp(line, "TICK100") || !strcmp(line, "TICK1000")) {
        uart_puts("TICK rate change requires rebuild\r\n");
    }
    else {
        uart_puts("ERR unknown\r\n");
    }
}

/* Стара функція ЛР3 (у ЛР4 не використовується) */
void cmd_poll(void)
{
    if (!cmd_line_ready) return;
    cmd_line_ready = 0;
    cmd_handle_line(cmd_line);
}