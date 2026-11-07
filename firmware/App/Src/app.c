#include "app.h"
#include "perf.h"
#include "app_tasks.h"
#include "cmd.h"
#include "main.h"
#include <stdio.h>
#include <malloc.h>

extern uint32_t _estack;
extern uint32_t _Min_Stack_Size;
#define STACK_PATTERN 0xDEADBEEFu

static uint32_t *stack_bottom(void)
{
    return (uint32_t *)((uint32_t)&_estack - (uint32_t)&_Min_Stack_Size);
}

void stack_paint(void)
{
    uint32_t *p = stack_bottom();
    uint32_t *sp = (uint32_t *)__get_MSP();
    while (p < sp - 16) *p++ = STACK_PATTERN;
}

uint32_t stack_used_bytes(void)
{
    uint32_t *p = stack_bottom();
    while (p < &_estack && *p == STACK_PATTERN) p++;
    return (uint32_t)&_estack - (uint32_t)p;
}

void heap_report(void)
{
#if defined(__NEWLIB__) && !defined(__PICOLIBC__)
    struct mallinfo mi = mallinfo();
    printf("HEAP arena=%u used=%u free=%u\r\n",
           (unsigned)mi.arena, (unsigned)mi.uordblks, (unsigned)mi.fordblks);
#else
    printf("HEAP not available (picolibc)\r\n");
#endif
}

void App_Init(void)
{
    perf_init();
    stack_paint();
    tasks_init();
    printf("APP init, SystemCoreClock=%lu\r\n", (unsigned long)SystemCoreClock);
}

void App_Loop(void)
{
    printf("=== LAB3: super loop started ===\r\n");
    uint32_t last_stats_print = 0;
    while (1) {
        uint32_t t_loop = perf_now();
        uint32_t t0;

        t0 = perf_now(); imu_task();       perf_add(&st_imu,     perf_now() - t0);
        t0 = perf_now(); button_task();    (void)t0;
        t0 = perf_now(); uart_cmd_task();  perf_add(&st_uart_cmd, perf_now() - t0);
        t0 = perf_now(); flash_log_task(); perf_add(&st_flash_log, perf_now() - t0);
        t0 = perf_now(); lcd_task();       perf_add(&st_lcd,     perf_now() - t0);

        perf_add(&st_loop, perf_now() - t_loop);

        /* Автоматичний вивід статистики кожні 10 с — workaround для Serial Monitor */
        if (perf_cycles_to_us(perf_now() - last_stats_print) > 10000000u) {
            last_stats_print = perf_now();
            tasks_print_stats();
        }
    }
}