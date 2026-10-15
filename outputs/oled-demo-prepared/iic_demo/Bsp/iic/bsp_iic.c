#include "bsp_iic.h"
#include <stddef.h>

static IIC_Instance iic_instances[IIC_MAX_INSTANCES];
static uint8_t iic_instance_count;

IIC_Instance *IIC_Register(const IIC_Init_Config_s *conf)
{
    if (conf == NULL || conf->handle == NULL || conf->handle->Instance == NULL ||
        conf->dev_addr < 0x08U || conf->dev_addr > 0x77U || conf->timeout_ms == 0U ||
        conf->timeout_ms == HAL_MAX_DELAY || iic_instance_count >= IIC_MAX_INSTANCES) {
        return NULL;
    }

    for (uint8_t i = 0U; i < iic_instance_count; ++i) {
        if (iic_instances[i].handle == conf->handle &&
            iic_instances[i].dev_addr == conf->dev_addr) {
            return NULL;
        }
    }

    IIC_Instance *iic = &iic_instances[iic_instance_count++];
    iic->handle = conf->handle;
    iic->dev_addr = conf->dev_addr;
    iic->timeout_ms = conf->timeout_ms;
    iic->last_status = HAL_OK;
    return iic;
}

HAL_StatusTypeDef IIC_Is_Ready(IIC_Instance *iic)
{
    if (iic == NULL || iic->handle == NULL) {
        return HAL_ERROR;
    }
    iic->last_status = HAL_I2C_IsDeviceReady(iic->handle,
        (uint16_t)iic->dev_addr << 1U, 3U, iic->timeout_ms);
    return iic->last_status;
}

HAL_StatusTypeDef IIC_Transmit(IIC_Instance *iic, const uint8_t *data, uint16_t size)
{
    if (iic == NULL || iic->handle == NULL) {
        return HAL_ERROR;
    }
    if (data == NULL || size == 0U) {
        iic->last_status = HAL_ERROR;
        return iic->last_status;
    }
    /* HAL 的发送原型使用非 const 指针，但不会修改发送缓冲区。 */
    iic->last_status = HAL_I2C_Master_Transmit(iic->handle,
        (uint16_t)iic->dev_addr << 1U, (uint8_t *)data, size, iic->timeout_ms);
    return iic->last_status;
}

HAL_StatusTypeDef IIC_Receive(IIC_Instance *iic, uint8_t *data, uint16_t size)
{
    if (iic == NULL || iic->handle == NULL) {
        return HAL_ERROR;
    }
    if (data == NULL || size == 0U) {
        iic->last_status = HAL_ERROR;
        return iic->last_status;
    }
    iic->last_status = HAL_I2C_Master_Receive(iic->handle,
        (uint16_t)iic->dev_addr << 1U, data, size, iic->timeout_ms);
    return iic->last_status;
}
