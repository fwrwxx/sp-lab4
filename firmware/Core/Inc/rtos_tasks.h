#ifndef RTOS_TASKS_H
#define RTOS_TASKS_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "stream_buffer.h"

/* Тип запису логу */
typedef struct {
    uint32_t t_us;
    uint16_t code;
    uint16_t arg;
} LogRecord_t;

/* Глобальні дескриптори */
extern QueueHandle_t qLog;
extern SemaphoreHandle_t busMutex;
extern StreamBufferHandle_t sbUartRx;
extern TaskHandle_t hEventTask;
extern TaskHandle_t hSensorTask;
extern TaskHandle_t hCmdTask;
extern TaskHandle_t hLoggerTask;
extern TaskHandle_t hLongOpTask;

/* Ініціалізація та запуск */
void Rtos_Init(void);
void Rtos_StartTasks(void);

/* Виклики з ISR */
void Rtos_OnButtonISR(void);
void Rtos_OnUartByte(uint8_t b);

/* Команди вимірювання */
void Rtos_CmdTasks(void);
void Rtos_CmdStack(void);
void Rtos_CmdHeap(void);

/* Спільний вивід через UART без printf */
void uart_puts(const char *s);
void uart_put_u32(uint32_t v);   /* <--- ОСЬ ВОНА */

#endif