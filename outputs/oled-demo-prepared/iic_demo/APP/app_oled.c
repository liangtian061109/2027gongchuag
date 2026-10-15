#include "app_oled.h"
#include <stddef.h>

#define APP_OLED_PERIOD_MS 500U

static OLED_Instance *display;
static uint32_t counter;
static uint32_t last_update_ms;
static bool clock_started;

static bool App_OLED_Draw_Frame(void)
{
    /* 先在 RAM 显存绘制，最后只刷新一次。 */
    OLED_Clear(display);
    OLED_Show_String(display, 0U, 0U, "OLED I2C DEMO");
    OLED_Show_String(display, 0U, 16U, "COUNT:");
    OLED_Show_Num(display, 48U, 16U, counter, 6U);
    OLED_Show_String(display, 0U, 32U, "BSP->MODULE->APP");

    OLED_Draw_Line(display, 0U, 51U, 127U, 51U);
    OLED_Draw_Line(display, 0U, 63U, 127U, 63U);
    OLED_Draw_Line(display, 0U, 51U, 0U, 63U);
    OLED_Draw_Line(display, 127U, 51U, 127U, 63U);
    uint8_t progress = (uint8_t)(counter % 126U);
    for (uint8_t x = 1U; x <= progress; ++x) {
        OLED_Draw_Line(display, x, 54U, x, 60U);
    }
    return OLED_Refresh(display);
}

bool App_OLED_Init(OLED_Instance *oled)
{
    if (oled == NULL) {
        return false;
    }
    display = oled;
    counter = 0U;
    last_update_ms = 0U;
    clock_started = false;
    return App_OLED_Draw_Frame();
}

bool App_OLED_Update(uint32_t now_ms)
{
    if (display == NULL) {
        return false;
    }
    if (!clock_started) {
        last_update_ms = now_ms;
        clock_started = true;
        return true;
    }
    /* 无符号减法可以正确处理毫秒计数器回绕。 */
    if ((uint32_t)(now_ms - last_update_ms) < APP_OLED_PERIOD_MS) {
        return true;
    }
    last_update_ms = now_ms;
    counter = (counter + 1U) % 1000000U;
    return App_OLED_Draw_Frame();
}
