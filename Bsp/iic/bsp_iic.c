#include "bsp_iic.h"
#include <stdint.h>
#include <string.h>

/*静态数组存储iic实例*/
#define Max_IIC_Instance 1
static IIC_Instance IIC_Instances[Max_IIC_Instance];
static uint8_t IIC_Instance_count = 0;

/*注册iic实例*/
IIC_Instance *IIC_Register(IIC_Init_Config_s *conf)
{
    // 检查是否达到最大实例数
    if (IIC_Instance_count >= Max_IIC_Instance)
    {
        return NULL;
    }
    // 创建新的IIC实例
    IIC_Instance *New_Instance = &IIC_Instances[IIC_Instance_count];
    // memset清零
    memset(New_Instance, 0, sizeof(IIC_Instance));
    // 存储配置参数
    New_Instance->handle = conf->handle;
    New_Instance->dev_addr = conf->dev_addr << 1;
    New_Instance->rx_callback = conf->callback;
    New_Instance->id = conf->id;
    // 将新实例添加到数组中
    IIC_Instance_count++;
    return New_Instance;
}

void IIC_Transmit(IIC_Instance *iic, uint8_t *data, uint16_t size)
{
    // 阻塞发送(OLED 刷新用栈 buffer,必须阻塞;DMA 会读栈垃圾)
    HAL_I2C_Master_Transmit(iic->handle, iic->dev_addr, data, size, 100);
}

void IIC_Receive(IIC_Instance *iic, uint8_t *data, uint16_t size)
{
    HAL_I2C_Master_Receive(iic->handle, iic->dev_addr, data, size, 100);
}
