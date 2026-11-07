#ifndef APP_TASKS_H
#define APP_TASKS_H

#include "perf.h"

extern perf_stat_t st_imu, st_btn_latency, st_flash_log,
                   st_uart_cmd, st_long_op, st_lcd, st_loop;

void tasks_init(void);
void tasks_on_button_isr(void);

void imu_task(void);
void button_task(void);
void uart_cmd_task(void);
void flash_log_task(void);
void lcd_task(void);

void long_op_start(void);
uint8_t long_op_busy(void);
void long_op_poll(void);

void print_stat_func(const perf_stat_t *s);
void tasks_print_stats(void);
void tasks_reset_stats(void);
int32_t tasks_imu_jitter_min(void);
int32_t tasks_imu_jitter_max(void);
uint32_t tasks_imu_missed(void);

#endif