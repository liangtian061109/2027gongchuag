#include "bsp_can.h"
#include <stdint.h>
#include <string.h>

/*静态数组存储can实例*/
#define Max_CAN_Instance 8
static CAN_Instance CAN_Instances[Max_CAN_Instance];
static uint8_t CAN_Instance_count = 0;

/*注册can实例*/
CAN_Instance *CAN_Register(CAN_Init_Config_s *conf)
{
  // 检查是否达到最大实例数
  if (CAN_Instance_count >= Max_CAN_Instance)
    {
        return NULL;
  }
  // 创建新的CAN实例
  CAN_Instance *New_Instance = &CAN_Instances[CAN_Instance_count];
  // memset清零
  memset(New_Instance, 0, sizeof(CAN_Instance));
  // 存储配置参数
  New_Instance->handle = conf->handle;
  New_Instance->filter = conf->filter;
  HAL_CAN_ConfigFilter(New_Instance->handle, &New_Instance->filter);
  New_Instance->rx_callback = conf->rx_callback;
  New_Instance->id = conf->id;
  // 将新实例添加到数组中
  CAN_Instance_count++;
  return New_Instance;
}

void CAN_Start(CAN_Instance *can)
{
    // 启动CAN接收中断
    HAL_CAN_Start(can->handle);
    HAL_CAN_ActivateNotification(can->handle, CAN_IT_RX_FIFO0_MSG_PENDING);
}

void CAN_Transmit(CAN_Instance *can, uint32_t id, uint8_t *data, uint8_t len)
{
  CAN_TxHeaderTypeDef Tx_header;

  uint32_t TxMailbox;

  Tx_header.ExtId = id;
  Tx_header.IDE = CAN_ID_EXT;
  Tx_header.RTR = CAN_RTR_DATA;
  Tx_header.DLC = len;
  Tx_header.TransmitGlobalTime = DISABLE;
  while(HAL_CAN_AddTxMessage(can->handle, &Tx_header, data, &TxMailbox));
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  for (uint8_t i = 0; i < CAN_Instance_count; i++)
    {
        if (CAN_Instances[i].handle == hcan)
        {
          if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &CAN_Instances[i].rx_header,
                               CAN_Instances[i].rx_data)== HAL_OK)
          {
            if (CAN_Instances[i].rx_callback != NULL) {
              CAN_Instances[i].rx_callback(&CAN_Instances[i]);
            }
          }
          return;
        }
    }
}

