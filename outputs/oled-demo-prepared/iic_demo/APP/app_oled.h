#ifndef APP_OLED_H
#define APP_OLED_H

#include "oled.h"
#include <stdbool.h>
#include <stdint.h>

/* 绑定由 app_init 创建的实例并显示第一帧，不再注册设备。 */
bool App_OLED_Init(OLED_Instance *oled);
/* 每隔 500 ms 更新计数和进度条；返回 false 表示刷新失败。 */
bool App_OLED_Update(uint32_t now_ms);

#endif
