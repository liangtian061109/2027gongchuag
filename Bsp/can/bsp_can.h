#ifndef BSP_CAN_H
#define BSP_CAN_H

#include "main.h"
#include <stdint.h>

typedef struct CAN_S
{
  CAN_HandleTypeDef *handle;
  CAN_FilterTypeDef filter;
  CAN_RxHeaderTypeDef rx_header;
  uint8_t rx_data[8];
  void (*rx_callback)(struct CAN_S *can);
  void *id;

} CAN_Instance;

typedef struct
{
  
  CAN_HandleTypeDef *handle;
  CAN_FilterTypeDef filter;
  void (*rx_callback)(struct CAN_S *can);
  void *id;

} CAN_Init_Config_s;

CAN_Instance *CAN_Register(CAN_Init_Config_s *conf);
void CAN_Start(CAN_Instance *can);
void CAN_Transmit(CAN_Instance *can, uint32_t id, uint8_t *data, uint8_t len);
#endif