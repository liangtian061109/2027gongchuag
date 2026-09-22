/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "dma.h"
#include "i2c.h"
#include "stm32f4xx_hal.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Chassis.h"
#include "Emm_V5.h"
#include "bsp_can.h"
#include "bsp_iic.h"
#include "bsp_uart.h"
#include "oled.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
UART_Instance *uart1;   // 串口实例
OLED_Instance *oled;    // OLED 屏幕实例
CAN_Instance *can_inst; // CAN 实例(Emm_V5 模块要用)
static uint8_t chassis_test_state = 0;
static uint32_t chassis_test_start_tick = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* OLED 屏幕的注册 */
void UART_RxHandler(UART_Instance *uart, uint16_t size) {
  UART_Transmit(uart, uart->rx_buffer, size); // 把收到的发回去
}

void OLED_Setup(void) {
  HAL_Delay(200); // 等 SSD1306 完成内部上电复位(无 RES 脚,靠延时保上电时序)
  IIC_Init_Config_s iic_conf = {
      .handle = &hi2c1, // 用 I2C1
      .dev_addr = 0x3C, // OLED 7位地址,bsp内部左移成0x78
      .callback = NULL,
      .id = NULL,
  };
  OLED_Init_Config_s oled_conf = {.iic_conf = iic_conf};
  oled = OLED_Register(&oled_conf); // 注册(内部会注册 IIC 实例)
  OLED_Init(oled);                  // 初始化屏幕
  OLED_Clear(oled);                 // 清屏
  OLED_Refresh(oled);               // 刷到屏幕
}
/* 串口的注册 */
void UART_Setup(void) {
  UART_Init_Config_s uart_conf = {
      .handle = &huart1,             // 用 USART1
      .rx_callback = UART_RxHandler, // 接收回调函数
      .id = NULL,
  };
  uart1 = UART_Register(&uart_conf); // 注册(内部会注册 DMA 实例)
  UART_Receive(uart1);               // 启动接收
}
/*CAN的注册*/
void CAN_Setup(void) {
  CAN_Init_Config_s can_conf = {
      .handle = &hcan1,
      .filter.FilterBank = 0,
      .filter.FilterMode = CAN_FILTERMODE_IDMASK,
      .filter.FilterScale = CAN_FILTERSCALE_32BIT,
      .filter.FilterIdHigh = 0x0000,
      .filter.FilterIdLow = 0x0000,
      .filter.FilterMaskIdHigh = 0x0000,
      .filter.FilterMaskIdLow = 0x0000,
      .filter.FilterFIFOAssignment = CAN_FILTER_FIFO0,
      .filter.FilterActivation = ENABLE,
      /* 0~13号滤波器归CAN1，14~27号滤波器归CAN2。 */
      .filter.SlaveStartFilterBank = 14,
      .rx_callback = Emm_V5_RxCallback,
      .id = NULL,
  };
  CAN_Instance *can_instance =
      CAN_Register(&can_conf); // 注册(内部会注册 CAN 实例)
  CAN_Start(can_instance);     // 启动 CAN 接收中断
  can_inst = can_instance;     // 存到全局,Emm_V5 模块要用
}

/* 悬空低速测试：前进约 1 秒后停止，只发送每条命令一次。 */
static void Chassis_Test_Update(void)
{
  if (chassis_test_state == 0)
  {
    Chassis_Set_Velocity(100.0f, 0.0f, 0.0f, 10, true);
    chassis_test_start_tick = HAL_GetTick();
    chassis_test_state = 1;
  }
  else if (chassis_test_state == 1 &&
           (HAL_GetTick() - chassis_test_start_tick) >= 100)
  {
    Chassis_Set_Velocity(0.0f, 0.0f, 0.0f, 10, true);
    chassis_test_state = 2;
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_CAN1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  // OLED_Setup(); // OLED 注册
  UART_Setup();          // UART 注册(标定期用串口输出位置)
  CAN_Setup();           // CAN 注册

  HAL_Delay(1000);
  Emm_V5_Init(can_inst); // 把 CAN 实例交给电机模块

  Chassis_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */
    Chassis_Update();
    Chassis_Test_Update();
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
