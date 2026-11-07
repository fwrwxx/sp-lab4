#include "rtos_tasks.h"
#include "main.h"
#include "app.h"
#include "board.h"
#include "perf.h"
#include "app_tasks.h"
#include "cmd.h"
#include <string.h>

extern UART_HandleTypeDef huart2;
extern uint32_t t_start;

static void vEventTask(void *arg);
static void vSensorTask(void *arg);
static void vCmdTask(void *arg);
static void vLoggerTask(void *arg);
static void vLongOpTask(void *arg);

/* Глобальні дескриптори */
QueueHandle_t qLog;
SemaphoreHandle_t busMutex;
StreamBufferHandle_t sbUartRx;
TaskHandle_t hEventTask, hSensorTask, hCmdTask, hLoggerTask, hLongOpTask;

/* ---------- Допоміжні функції виводу ---------- */
void uart_puts(const char *s)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)s, (uint16_t)strlen(s), 100);
}

void uart_put_u32(uint32_t v)
{
    char buf[11];
    int i = 10;
    buf[10] = '\0';
    if (v == 0) { uart_puts("0"); return; }
    while (v > 0 && i > 0) { buf[--i] = '0' + (v % 10); v /= 10; }
    uart_puts(&buf[i]);
}

/* ---------- Ініціалізація IPC ---------- */
void Rtos_Init(void)
{
    qLog = xQueueCreate(16, sizeof(LogRecord_t));
    busMutex = xSemaphoreCreateMutex();
    sbUartRx = xStreamBufferCreate(64, 1);
    configASSERT(qLog && busMutex && sbUartRx);
}

/* ---------- Створення задач ---------- */
void Rtos_StartTasks(void)
{
    xTaskCreate(vEventTask,   "event",   128, NULL, 4, &hEventTask);
    xTaskCreate(vSensorTask,  "sensor",  192, NULL, 3, &hSensorTask);
    xTaskCreate(vCmdTask,     "cmd",     256, NULL, 2, &hCmdTask);
    xTaskCreate(vLoggerTask,  "logger",  192, NULL, 1, &hLoggerTask);
    xTaskCreate(vLongOpTask,  "longop",  192, NULL, 1, &hLongOpTask);
}

/* ---------- Задача event ---------- */
static void vEventTask(void *arg)
{
    (void)arg;
    uart_puts("BOOT ");
    uart_put_u32(perf_now() - t_start);
    uart_puts(" us\r\n");

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        button_task();
        
        if (st_btn_latency.count > 0) {
            uart_puts("BTN_LATENCY max_us=");
            uart_put_u32(perf_cycles_to_us(st_btn_latency.max));
            uart_puts("\r\n");
        }
        
        xTaskNotifyGive(hLongOpTask);
    }
}

/* ---------- Задача sensor ---------- */
static void vSensorTask(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(10));
        if (xSemaphoreTake(busMutex, 0) == pdTRUE) {
            imu_task();
            xSemaphoreGive(busMutex);
        }
    }
}

/* ---------- Задача cmd ---------- */
static void vCmdTask(void *arg)
{
    (void)arg;
    char line[64];
    size_t len = 0;
    uint8_t c;
    for (;;) {
        if (xStreamBufferReceive(sbUartRx, &c, 1, portMAX_DELAY) == 1) {
            if (c == '\n' || c == '\r') {
                line[len] = '\0';
                if (len > 0) {
                    cmd_handle_line(line);
                }
                len = 0;
            } else if (len < sizeof(line) - 1) {
                line[len++] = (char)c;
            }
        }
    }
}

/* ---------- Задача logger ---------- */
static void vLoggerTask(void *arg)
{
    (void)arg;
    LogRecord_t r;
    for (;;) {
        if (xQueueReceive(qLog, &r, portMAX_DELAY) == pdTRUE) {
            (void)r;
            if (xSemaphoreTake(busMutex, portMAX_DELAY) == pdTRUE) {
                flash_log_task();
                xSemaphoreGive(busMutex);
            }
        }
    }
}

/* ---------- Задача long_op ---------- */
static void vLongOpTask(void *arg)
{
    (void)arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (xSemaphoreTake(busMutex, portMAX_DELAY) == pdTRUE) {
            long_op_start();
            xSemaphoreGive(busMutex);
        }
    }
}

/* ---------- Виклики з ISR ---------- */
void Rtos_OnButtonISR(void)
{
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(hEventTask, &woken);
    portYIELD_FROM_ISR(woken);
}

void Rtos_OnUartByte(uint8_t b)
{
    BaseType_t woken = pdFALSE;
    xStreamBufferSendFromISR(sbUartRx, &b, 1, &woken);
    portYIELD_FROM_ISR(woken);
}

/* ---------- Хуки ---------- */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    taskDISABLE_INTERRUPTS();
    uart_puts("\r\nFATAL: stack overflow in ");
    uart_puts(pcTaskName);
    uart_puts("\r\n");
    for (;;) {}
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    uart_puts("\r\nFATAL: malloc failed\r\n");
    for (;;) {}
}

void vApplicationTickHook(void) {}

/* ---------- Команди вимірювання ---------- */
void Rtos_CmdTasks(void)
{
    TaskStatus_t st[6];
    UBaseType_t n = uxTaskGetSystemState(st, 6, NULL);
    uart_puts("=== TASK STATE ===\r\n");
    for (UBaseType_t i = 0; i < n; i++) {
        uart_puts(st[i].pcTaskName);
        uart_puts(" prio="); uart_put_u32(st[i].uxCurrentPriority);
        uart_puts(" state="); uart_put_u32(st[i].eCurrentState);
        uart_puts(" stack="); uart_put_u32(st[i].usStackHighWaterMark);
        uart_puts("\r\n");
    }
}

void Rtos_CmdStack(void)
{
    TaskStatus_t st[6];
    UBaseType_t n = uxTaskGetSystemState(st, 6, NULL);
    for (UBaseType_t i = 0; i < n; i++) {
        uart_puts("STACK ");
        uart_puts(st[i].pcTaskName);
        uart_puts(" free_words=");
        uart_put_u32(st[i].usStackHighWaterMark);
        uart_puts("\r\n");
    }
}

void Rtos_CmdHeap(void)
{
    uart_puts("HEAP total=");
    uart_put_u32((uint32_t)configTOTAL_HEAP_SIZE);
    uart_puts(" free=");
    uart_put_u32((uint32_t)xPortGetFreeHeapSize());
    uart_puts(" min_ever_free=");
    uart_put_u32((uint32_t)xPortGetMinimumEverFreeHeapSize());
    uart_puts("\r\n");
}