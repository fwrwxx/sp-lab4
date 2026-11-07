/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32c0xx_it.c
  * @brief   Interrupt Service Routines (Lab 4: FreeRTOS)
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32c0xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FreeRTOS.h"
#include "task.h"
#include "app_tasks.h"
#include "rtos_tasks.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim14;
extern void perf_on_tim_overflow(void);
/* USER CODE END EV */

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */
  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */
  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/******************************************************************************/
/* STM32C0xx Peripheral Interrupt Handlers                                    */
/******************************************************************************/

/* USER CODE BEGIN 1 */

/* ---------- USART2: прийом байта -> Stream Buffer -> задача cmd ---------- */
void USART2_IRQHandler(void)
{
    if (USART2->ISR & USART_ISR_RXNE_RXFNE) {
        uint8_t b = (uint8_t)(USART2->RDR & 0xFF);
        Rtos_OnUartByte(b);
    }
}

/* ---------- EXTI: кнопка -> сповіщення задачі event ---------- */
void EXTI4_15_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_13) {
        tasks_on_button_isr();
        Rtos_OnButtonISR();
    }
}

/* ---------- TIM3: PWM для серво + джерело часу run-time stats ---------- */
void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim3);
}

/* ---------- TIM14: HAL timebase (замінює SysTick для HAL) ---------- */
void TIM14_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim14);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM14) {
        HAL_IncTick();
    }
    if (htim->Instance == TIM3) {
        perf_on_tim_overflow();
    }
}
/* USER CODE END 1 */