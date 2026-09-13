#ifndef BSP_IIC_H
#define BSP_IIC_H

#include "main.h"
#include <stdint.h>

typedef struct IIC_S
{
  I2C_HandleTypeDef *handle;
  uint8_t dev_addr;
  uint8_t *rx_buffer;
  uint16_t rx_size;
  void (*rx_callback)(struct IIC_S *iic, uint8_t *data, uint16_t size);
  void *id;
} IIC_Instance;

typedef struct
{
  I2C_HandleTypeDef *handle;
  uint8_t dev_addr;
  void (*callback)(struct IIC_S *iic, uint8_t *data, uint16_t size);
  void *id;
} IIC_Init_Config_s;

IIC_Instance *IIC_Register(IIC_Init_Config_s *conf);
void IIC_Transmit(IIC_Instance *iic, uint8_t *data, uint16_t size);
void IIC_Receive(IIC_Instance *iic, uint8_t *data, uint16_t size);

#endif
