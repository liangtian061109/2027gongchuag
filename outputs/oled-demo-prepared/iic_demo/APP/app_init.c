#include "app_init.h"
#include "app_oled.h"
#include "i2c.h"
#include "oled.h"
#include <stddef.h>

/* 本例程的硬件选择、实例注册和初始化统一放在这里。
 * 更换 OLED 地址只修改此配置；更换引脚还需在 CubeMX 中配置。
 */
static OLED_Instance *app_oled;
static volatile App_Status_t app_status = APP_NOT_INITIALIZED;

bool App_Init(void)
{
    /* 防止重复调用消耗静态实例池。 */
    if (app_status != APP_NOT_INITIALIZED) {
        return app_status == APP_READY;
    }
    const OLED_Init_Config_s oled_conf = {
        .iic_conf = {
            .handle = &hi2c1,
            .dev_addr = 0x3CU,
            .timeout_ms = 100U
        }
    };
    app_oled = OLED_Register(&oled_conf);
    if (app_oled == NULL) {
        app_status = APP_REGISTER_FAILED;
        return false;
    }
    if (!OLED_Init(app_oled)) {
        app_status = APP_OLED_INIT_FAILED;
        return false;
    }
    if (!App_OLED_Init(app_oled)) {
        app_status = APP_OLED_REFRESH_FAILED;
        return false;
    }
    app_status = APP_READY;
    return true;
}

void App_Run(uint32_t now_ms)
{
    if (app_status == APP_READY && !App_OLED_Update(now_ms)) {
        app_status = APP_OLED_REFRESH_FAILED;
    }
}

App_Status_t App_Get_Status(void)
{
    return app_status;
}
