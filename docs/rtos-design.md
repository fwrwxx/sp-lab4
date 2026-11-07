# Архітектура FreeRTOS (ЛР4)

## Розподіл задач та пріоритетів

| Задача | Пріоритет | Стек (слів) | Період/Тригер | Механізм взаємодії |
| :--- | :--- | :--- | :--- | :--- |
| `vEventTask` | 4 | 128 | EXTI (кнопка) | `ulTaskNotifyTake` |
| `vSensorTask` | 3 | 192 | 10 мс | `vTaskDelayUntil`, `busMutex` |
| `vCmdTask` | 2 | 256 | Stream Buffer | `xStreamBufferReceive` |
| `vLoggerTask` | 1 | 192 | Черга `qLog` | `xQueueReceive` |
| `vLongOpTask` | 1 | 192 | Сповіщення | `ulTaskNotifyTake`, `busMutex` |

## Обґрунтування пріоритетів
Пріоритети призначено за принципом Rate-Monotonic. Найвищий пріоритет (4) має `vEventTask`, оскільки вимога PRD до реакції на кнопку — <10 мс. Задача `vSensorTask` має пріоритет 3 через жорсткий період 10 мс. Довга операція `vLongOpTask` має найнижчий пріоритет, щоб не блокувати інші задачі.

## Механізми IPC
- **М'ютекс (`busMutex`)** захищає спільну шину I2C/SPI.
- **Черга (`qLog`)** використовується для передачі записів від `vLongOpTask` до `vLoggerTask`.
- **Stream Buffer (`sbUartRx`)** використовується для передачі байтів від UART ISR до `vCmdTask`.
- **Сповіщення (`xTaskNotify`)** використовуються для швидкої передачі подій між ISR та задачами.