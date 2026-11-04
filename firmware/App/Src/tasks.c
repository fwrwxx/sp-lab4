#include "tasks.h"
#include "bus.h"
#include "mpu6050.h"
#include "w25q64.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

perf_stat_t st_imu          = { .name = "imu" };
perf_stat_t st_btn_latency  = { .name = "btn_latency" };
perf_stat_t st_flash_log    = { .name = "flash_log" };
perf_stat_t st_uart_cmd     = { .name = "uart_cmd" };
perf_stat_t st_long_op      = { .name = "long_op" };
perf_stat_t st_lcd          = { .name = "lcd" };
perf_stat_t st_loop         = { .name = "loop" };

static mpu6050_t g_imu;
static volatile uint32_t btn_t_event;
static volatile uint8_t  btn_pending;
static uint8_t  long_op_active;

static uint32_t imu_t_prev;
static uint32_t flash_t_prev;
static uint32_t lcd_t_prev;
static int32_t  imu_jitter_min = 0x7FFFFFFF;
static int32_t  imu_jitter_max = -0x7FFFFFFF;
static uint32_t imu_missed;

/* --- Службові probe-піни для Logic Analyzer --- */
#define PROBE_IMU_ON()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8,  GPIO_PIN_SET)
#define PROBE_IMU_OFF() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8,  GPIO_PIN_RESET)
#define PROBE_LONG_ON() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9,  GPIO_PIN_SET)
#define PROBE_LONG_OFF()HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9,  GPIO_PIN_RESET)
#define PROBE_BTN_ON()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET)
#define PROBE_BTN_OFF() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET)

void tasks_init(void)
{
    /* Probe-піни */
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    g.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);

    MPU6050_Init(&g_imu);
}

void tasks_on_button_isr(void)
{
    btn_t_event = perf_now();
    btn_pending = 1;
    PROBE_BTN_ON();
}

/* --- IMU --- */
void imu_task(void)
{
    uint32_t now = perf_now();
    if (imu_t_prev) {
        int32_t dt_us = (int32_t)perf_cycles_to_us(now - imu_t_prev);
        int32_t jitter = dt_us - 5000;
        if (jitter < imu_jitter_min) imu_jitter_min = jitter;
        if (jitter > imu_jitter_max) imu_jitter_max = jitter;
        if (dt_us > 10000) imu_missed++;
    }
    imu_t_prev = now;

    PROBE_IMU_ON();
    int16_t ax, ay, az;
    MPU6050_ReadAccelXYZ(&g_imu, &ax, &ay, &az);
    PROBE_IMU_OFF();
    (void)ax; (void)ay; (void)az;
}

/* --- Кнопка: latency від ISR до початку реакції --- */
void button_task(void)
{
    if (!btn_pending) return;
    btn_pending = 0;
    uint32_t latency = perf_now() - btn_t_event;
    perf_add(&st_btn_latency, latency);
    PROBE_BTN_OFF();

    /* Реакція: старт довгої операції (демонстрація блокування super loop) */
    if (!long_op_active) long_op_start();
}

/* --- UART команди: неблокуючий, реальна робота в ISR --- */
void uart_cmd_task(void)
{
    extern void cmd_poll(void);
    cmd_poll();
}

/* --- Flash-журнал: раз на 5 с --- */
void flash_log_task(void)
{
    uint32_t now = perf_now();
    if (flash_t_prev && (perf_cycles_to_us(now - flash_t_prev) < 5000000u)) return;
    flash_t_prev = now;

    uint8_t jedec[3];
    W25Q64_ReadJedec(jedec);
    /* Приклад запису одного байта в адресу 0x0010 */
    static uint16_t addr = 0x0010;
    W25Q64_WriteByte(addr, (uint8_t)(now & 0xFF));
    addr++;
    if (addr > 0x00FF) addr = 0x0010;
}

/* --- Довга операція: навмисно блокуюча, ~50–100 мс --- */
void long_op_start(void)
{
    if (long_op_active) return;
    long_op_active = 1;

    uint32_t t0 = perf_now();
    PROBE_LONG_ON();

    /* Читання JEDEC-байтів дає реалістичну SPI-транзакцію,
     * але замало часу для «довгої». Тому додаємо шину-затримку. */
    for (volatile uint32_t i = 0; i < 40000; i++) { __NOP(); }

    uint8_t jedec[3];
    W25Q64_ReadJedec(jedec);

    PROBE_LONG_OFF();
    perf_add(&st_long_op, perf_now() - t0);
    long_op_active = 0;
}

uint8_t long_op_busy(void) { return long_op_active; }

/* --- LCD: раз на 500 мс --- */
void lcd_task(void)
{
    uint32_t now = perf_now();
    if (lcd_t_prev && (perf_cycles_to_us(now - lcd_t_prev) < 500000u)) return;
    lcd_t_prev = now;
    /* lcd_update(...); — якщо у вас є, викличте тут */
}

/* --- Друк статистики --- */
static void print_stat(const perf_stat_t *s)
{
    if (!s->count) return;
    printf("STAT name=%s n=%lu min_us=%lu avg_us=%lu max_us=%lu\r\n",
           s->name, (unsigned long)s->count,
           (unsigned long)perf_cycles_to_us(s->min),
           (unsigned long)perf_avg_us(s),
           (unsigned long)perf_cycles_to_us(s->max));
}

void tasks_print_stats(void)
{
    print_stat(&st_imu);
    print_stat(&st_btn_latency);
    print_stat(&st_flash_log);
    print_stat(&st_uart_cmd);
    print_stat(&st_long_op);
    print_stat(&st_lcd);
    print_stat(&st_loop);
    printf("IMU jitter_min_us=%ld jitter_max_us=%ld missed=%lu\r\n",
           (long)imu_jitter_min, (long)imu_jitter_max,
           (unsigned long)imu_missed);
}

void tasks_reset_stats(void)
{
    memset(&st_imu, 0, sizeof(st_imu));           st_imu.name = "imu";
    memset(&st_btn_latency, 0, sizeof(st_btn_latency)); st_btn_latency.name = "btn_latency";
    memset(&st_flash_log, 0, sizeof(st_flash_log)); st_flash_log.name = "flash_log";
    memset(&st_uart_cmd, 0, sizeof(st_uart_cmd));   st_uart_cmd.name = "uart_cmd";
    memset(&st_long_op, 0, sizeof(st_long_op));     st_long_op.name = "long_op";
    memset(&st_lcd, 0, sizeof(st_lcd));             st_lcd.name = "lcd";
    memset(&st_loop, 0, sizeof(st_loop));           st_loop.name = "loop";
    imu_missed = 0;
    imu_jitter_min = 0x7FFFFFFF;
    imu_jitter_max = -0x7FFFFFFF;
}

int32_t  tasks_imu_jitter_min(void) { return imu_jitter_min; }
int32_t  tasks_imu_jitter_max(void) { return imu_jitter_max; }
uint32_t tasks_imu_missed(void)     { return imu_missed; }