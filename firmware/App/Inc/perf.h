#ifndef PERF_H
#define PERF_H

#include <stdint.h>
#include "main.h"

/* STM32C0 (Cortex-M0+) НЕ має DWT CYCCNT — використовуємо TIM3 (1 МГц) з
 * програмним розширенням до 32 біт. TIM3 переповнюється кожні 20 мс. */

void        perf_init(void);
uint32_t    perf_now(void);              /* поточний час, мкс (32-біт, з overflow) */
uint32_t    perf_cycles_to_us(uint32_t cyc); /* для сумісності — те саме, що perf_now */
void        perf_on_tim_overflow(void);  /* викликається з HAL_TIM_PeriodElapsedCallback */

typedef struct {
    const char *name;
    uint32_t count, min, max;
    uint64_t sum;
} perf_stat_t;

static inline void perf_add(perf_stat_t *s, uint32_t cyc)
{
    if (cyc > 5000000u) return;  /* > 5 с — явно сміття, пропускаємо */
    if (s->count == 0 || cyc < s->min) s->min = cyc;
    if (cyc > s->max) s->max = cyc;
    s->sum += cyc;
    s->count++;
}

static inline uint32_t perf_avg_us(const perf_stat_t *s)
{
    if (!s->count) return 0;
    return (uint32_t)(s->sum / s->count);
}

#endif