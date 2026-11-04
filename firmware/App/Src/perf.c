#include "perf.h"
#include "main.h"

extern TIM_HandleTypeDef htim3;

static volatile uint32_t tim3_overflows;

void perf_init(void)
{
    tim3_overflows = 0;
    /* TIM3 уже запущений у main.c через HAL_TIM_PWM_Start(...).
     * Вмикаємо переривання по update (переповнення): */
    __HAL_TIM_ENABLE_IT(&htim3, TIM_IT_UPDATE);
    HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
}

/* Викликати з HAL_TIM_PeriodElapsedCallback у stm32c0xx_it.c (або з TIM3_IRQHandler) */
void perf_on_tim_overflow(void)
{
    tim3_overflows++;
}

uint32_t perf_now(void)
{
    /* Атомарне читання: забороняємо переривання, щоб overflows і CNT не
     * «розсинхронізувались» між двома читаннями. */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint32_t ov  = tim3_overflows;
    uint32_t cnt = TIM3->CNT;
    /* Якщо поки читали CNT, сталося переповнення — врахуємо */
    if ((TIM3->SR & TIM_SR_UIF) && cnt < 0x1000) {
        ov++;
    }
    __set_PRIMASK(primask);

    /* TIM3: Prescaler=47 → 1 МГц, Period=19999 → 20 000 тіків на цикл.
     * Отже, повний час = (ov * 20000 + cnt) мкс. */
    return (ov * 20000u) + cnt;
}

uint32_t perf_cycles_to_us(uint32_t cyc)
{
    return cyc;   /* уже в мкс */
}