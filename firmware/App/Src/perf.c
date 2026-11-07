#include "perf.h"
#include "main.h"
#include "FreeRTOS.h"

extern TIM_HandleTypeDef htim3;

/* ---------- Лічильник для perf_now() (32-бітний, сумісний з ЛР3) ---------- */
static volatile uint32_t tim3_overflows;

/* ---------- 64-бітний лічильник для FreeRTOS run-time stats ---------- */
static volatile uint64_t rtos_ticks;
static volatile uint32_t rtos_last_cnt;

/* TIM3: Prescaler=47 -> 1 MHz, Period=19999 -> 20000 мкс на цикл */
#define TIM3_PERIOD_US   20000u

void perf_init(void)
{
    tim3_overflows = 0;
    rtos_ticks = 0;
    rtos_last_cnt = 0;

    /* TIM3 вже запущений у main.c через HAL_TIM_PWM_Start(...).
     * Вмикаємо переривання по update (переповнення): */
    __HAL_TIM_ENABLE_IT(&htim3, TIM_IT_UPDATE);
    HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
}

/* Викликати з HAL_TIM_PeriodElapsedCallback у stm32c0xx_it.c */
void perf_on_tim_overflow(void)
{
    tim3_overflows++;
    rtos_ticks += TIM3_PERIOD_US;
    rtos_last_cnt = 0;
}

/* 32-бітний час у мкс (як у ЛР3, переповнюється кожні ~71 хв) */
uint32_t perf_now(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint32_t ov  = tim3_overflows;
    uint32_t cnt = TIM3->CNT;
    /* Якщо поки читали CNT, сталося переповнення — врахуємо */
    if ((TIM3->SR & TIM_SR_UIF) && cnt < 0x1000) {
        ov++;
    }
    __set_PRIMASK(primask);

    return (ov * TIM3_PERIOD_US) + cnt;
}

uint32_t perf_cycles_to_us(uint32_t cyc)
{
    return cyc;   /* уже в мкс */
}

/* ---------- Інтерфейс для FreeRTOS run-time stats ---------- */

/* Викликається один раз при старті планувальника (через portCONFIGURE_TIMER_FOR_RUN_TIME_STATS) */
void perf_rtos_timer_init(void)
{
    rtos_ticks = 0;
    rtos_last_cnt = TIM3->CNT;
}

/* Повертає монотонний 64-бітний лічильник у мкс */
uint64_t perf_get_runtime_counter(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint64_t base = rtos_ticks;
    uint32_t cnt  = TIM3->CNT;
    /* Якщо переповнення сталося між читанням rtos_ticks і CNT,
     * UIF встановлено, а CNT ще не встиг обнулитися — додаємо період. */
    if ((TIM3->SR & TIM_SR_UIF) && cnt < 0x1000) {
        base += TIM3_PERIOD_US;
    }
    __set_PRIMASK(primask);
    return base + cnt;
}