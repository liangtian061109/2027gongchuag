$ErrorActionPreference = 'Stop'
$target = 'D:\gk_rm_demo\iic_demo'
$backup = Join-Path $target ('Backups\oled-app-layout-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))

function Save-Text([string]$relative, [string]$content) {
    $path = Join-Path $target $relative
    if (Test-Path -LiteralPath $path) {
        $backupPath = Join-Path $backup $relative
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($backupPath)) | Out-Null
        Copy-Item -LiteralPath $path -Destination $backupPath
    }
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path)) | Out-Null
    Set-Content -LiteralPath $path -Value $content -Encoding UTF8
}

Save-Text 'APP/app_init.c' @'
#include "app_init.h"
#include "app_oled.h"

static volatile App_Status_t app_status = APP_NOT_INITIALIZED;

bool App_Init(void)
{
    /* 顶层只安排各个 APP 的初始化，不拥有任何具体设备实例。 */
    if (app_status != APP_NOT_INITIALIZED) {
        return app_status == APP_READY;
    }

    if (!App_OLED_Init()) {
        app_status = APP_OLED_INIT_FAILED;
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
'@

Save-Text 'APP/app_oled.h' @'
#ifndef APP_OLED_H
#define APP_OLED_H

#include <stdbool.h>
#include <stdint.h>

/* OLED APP 自己完成硬件配置、模块注册、设备初始化和首帧显示。 */
bool App_OLED_Init(void);
/* 每隔 500 ms 更新计数和进度条；返回 false 表示刷新失败。 */
bool App_OLED_Update(uint32_t now_ms);

#endif
'@

Save-Text 'APP/app_oled.c' @'
#include "app_oled.h"
#include "i2c.h"
#include "oled.h"

#define APP_OLED_PERIOD_MS 500U

/* 设备实例属于 OLED APP，不能暴露给 app_init 或其他 APP。 */
static OLED_Instance *display;
static uint32_t counter;
static uint32_t last_update_ms;
static bool clock_started;

static bool App_OLED_Draw_Frame(void)
{
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

bool App_OLED_Init(void)
{
    /* 板级选择属于 APP：OLED 模块本身不绑定 hi2c1 或 0x3C。 */
    const OLED_Init_Config_s oled_conf = {
        .iic_conf = {
            .handle = &hi2c1,
            .dev_addr = 0x3CU,
            .timeout_ms = 100U
        }
    };

    /* OLED_Register 内部会调用 IIC_Register，APP 不重复注册 I2C。 */
    display = OLED_Register(&oled_conf);
    if (display == NULL || !OLED_Init(display)) {
        display = NULL;
        return false;
    }

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
    /* 无符号减法可以正确处理 HAL_GetTick() 回绕。 */
    if ((uint32_t)(now_ms - last_update_ms) < APP_OLED_PERIOD_MS) {
        return true;
    }
    last_update_ms = now_ms;
    counter = (counter + 1U) % 1000000U;
    return App_OLED_Draw_Frame();
}
'@

Save-Text 'Modules/MessageCenter/message_center.h' @'
#ifndef MESSAGE_CENTER_H
#define MESSAGE_CENTER_H

#include <stdbool.h>
#include <stdint.h>

#define MESSAGE_CENTER_MAX_TOPICS 8U
#define MESSAGE_CENTER_MAX_SUBSCRIBERS 16U
#define MESSAGE_CENTER_QUEUE_DEPTH 4U
#define MESSAGE_CENTER_TOPIC_NAME_SIZE 24U
#define MESSAGE_CENTER_MAX_DATA_SIZE 64U

typedef struct MessageTopic_s MessageTopic_t;
typedef struct MessageSubscriber_s MessageSubscriber_t;

/* APP 发布者只保存一个话题句柄；话题的存储由消息中心管理。 */
typedef struct {
    MessageTopic_t *topic;
} MessagePublisher_t;

/* 每个订阅者有独立队列，互不抢消息。 */
typedef struct {
    MessageSubscriber_t *subscriber;
} MessageSubscriberHandle_t;

bool MessageCenter_Init(void);
bool MessageCenter_Publisher_Register(MessagePublisher_t *publisher,
                                      const char *name, uint16_t data_size);
bool MessageCenter_Subscriber_Register(MessageSubscriberHandle_t *subscriber,
                                       const char *name, uint16_t data_size);
bool MessageCenter_Publish(const MessagePublisher_t *publisher, const void *data);
bool MessageCenter_Subscribe_Get(MessageSubscriberHandle_t *subscriber, void *data);

#endif
'@

Save-Text 'Modules/MessageCenter/message_center.c' @'
#include "message_center.h"
#include <stddef.h>
#include <string.h>

struct MessageSubscriber_s {
    MessageTopic_t *topic;
    uint8_t queue[MESSAGE_CENTER_QUEUE_DEPTH][MESSAGE_CENTER_MAX_DATA_SIZE];
    uint8_t front;
    uint8_t back;
    uint8_t count;
};

struct MessageTopic_s {
    bool used;
    char name[MESSAGE_CENTER_TOPIC_NAME_SIZE];
    uint16_t data_size;
    MessageSubscriber_t *subscribers[MESSAGE_CENTER_MAX_SUBSCRIBERS];
    uint8_t subscriber_count;
};

static MessageTopic_t topics[MESSAGE_CENTER_MAX_TOPICS];
static MessageSubscriber_t subscribers[MESSAGE_CENTER_MAX_SUBSCRIBERS];
static uint8_t subscriber_count;

bool MessageCenter_Init(void)
{
    memset(topics, 0, sizeof(topics));
    memset(subscribers, 0, sizeof(subscribers));
    subscriber_count = 0U;
    return true;
}

static MessageTopic_t *FindTopic(const char *name)
{
    for (uint8_t i = 0U; i < MESSAGE_CENTER_MAX_TOPICS; ++i) {
        if (topics[i].used && strcmp(topics[i].name, name) == 0) {
            return &topics[i];
        }
    }
    return NULL;
}

static MessageTopic_t *GetOrCreateTopic(const char *name, uint16_t data_size)
{
    MessageTopic_t *topic = FindTopic(name);
    if (topic != NULL) {
        return topic->data_size == data_size ? topic : NULL;
    }
    if (name == NULL || name[0] == '\0' || data_size == 0U ||
        data_size > MESSAGE_CENTER_MAX_DATA_SIZE) {
        return NULL;
    }
    for (uint8_t i = 0U; i < MESSAGE_CENTER_MAX_TOPICS; ++i) {
        if (!topics[i].used) {
            topic = &topics[i];
            memset(topic, 0, sizeof(*topic));
            topic->used = true;
            topic->data_size = data_size;
            strncpy(topic->name, name, MESSAGE_CENTER_TOPIC_NAME_SIZE - 1U);
            return topic;
        }
    }
    return NULL;
}

bool MessageCenter_Publisher_Register(MessagePublisher_t *publisher,
                                      const char *name, uint16_t data_size)
{
    if (publisher == NULL || publisher->topic != NULL) {
        return false;
    }
    MessageTopic_t *topic = GetOrCreateTopic(name, data_size);
    if (topic == NULL) {
        return false;
    }
    publisher->topic = topic;
    return true;
}

bool MessageCenter_Subscriber_Register(MessageSubscriberHandle_t *subscriber,
                                       const char *name, uint16_t data_size)
{
    if (subscriber == NULL || subscriber->subscriber != NULL ||
        subscriber_count >= MESSAGE_CENTER_MAX_SUBSCRIBERS) {
        return false;
    }
    MessageTopic_t *topic = GetOrCreateTopic(name, data_size);
    if (topic == NULL || topic->subscriber_count >= MESSAGE_CENTER_MAX_SUBSCRIBERS) {
        return false;
    }
    MessageSubscriber_t *instance = &subscribers[subscriber_count++];
    memset(instance, 0, sizeof(*instance));
    instance->topic = topic;
    topic->subscribers[topic->subscriber_count++] = instance;
    subscriber->subscriber = instance;
    return true;
}

bool MessageCenter_Publish(const MessagePublisher_t *publisher, const void *data)
{
    if (publisher == NULL || publisher->topic == NULL || data == NULL) {
        return false;
    }
    MessageTopic_t *topic = publisher->topic;
    for (uint8_t i = 0U; i < topic->subscriber_count; ++i) {
        MessageSubscriber_t *subscriber = topic->subscribers[i];
        if (subscriber->count == MESSAGE_CENTER_QUEUE_DEPTH) {
            subscriber->front = (uint8_t)((subscriber->front + 1U) %
                                          MESSAGE_CENTER_QUEUE_DEPTH);
            subscriber->count--;
        }
        memcpy(subscriber->queue[subscriber->back], data, topic->data_size);
        subscriber->back = (uint8_t)((subscriber->back + 1U) %
                                     MESSAGE_CENTER_QUEUE_DEPTH);
        subscriber->count++;
    }
    return true;
}

bool MessageCenter_Subscribe_Get(MessageSubscriberHandle_t *subscriber, void *data)
{
    if (subscriber == NULL || subscriber->subscriber == NULL || data == NULL) {
        return false;
    }
    MessageSubscriber_t *instance = subscriber->subscriber;
    if (instance->count == 0U) {
        return false;
    }
    memcpy(data, instance->queue[instance->front], instance->topic->data_size);
    instance->front = (uint8_t)((instance->front + 1U) % MESSAGE_CENTER_QUEUE_DEPTH);
    instance->count--;
    return true;
}
'@

Save-Text 'Examples/message_center_demo.c' @'
/* 这是一个不参与 OLED 工程编译的示例，展示两个 APP 如何通过话题通信。 */
#include "message_center.h"

typedef struct {
    float vx;
    float vy;
} ChassisCommand_t;

static MessagePublisher_t cmd_publisher;
static MessageSubscriberHandle_t chassis_subscriber;

void MessageCenter_Demo_Init(void)
{
    MessageCenter_Init();
    MessageCenter_Publisher_Register(&cmd_publisher, "chassis_cmd",
                                     sizeof(ChassisCommand_t));
    MessageCenter_Subscriber_Register(&chassis_subscriber, "chassis_cmd",
                                      sizeof(ChassisCommand_t));
}

void RobotCmd_App_Run(void)
{
    ChassisCommand_t command = {.vx = 1.0f, .vy = 0.0f};
    MessageCenter_Publish(&cmd_publisher, &command);
}

void Chassis_App_Run(void)
{
    ChassisCommand_t command;
    if (MessageCenter_Subscribe_Get(&chassis_subscriber, &command)) {
        /* 根据 command.vx / command.vy 计算并设置底盘目标。 */
    }
}
'@

Save-Text 'Modules/MessageCenter/README.md' @'
# Message Center

消息中心用于 APP 与 APP 之间的发布/订阅通信。它不是 OLED、I2C 等设备驱动的必需层。

```text
robot_cmd APP  --发布 chassis_cmd-->  message_center  --队列--> chassis APP
```

发布者和订阅者只需要约定话题名和数据结构，不需要互相包含头文件：

```c
typedef struct { float vx; float vy; } ChassisCommand_t;

MessagePublisher_t cmd_pub;
MessageSubscriberHandle_t chassis_sub;
MessageCenter_Init();
MessageCenter_Publisher_Register(&cmd_pub, "chassis_cmd", sizeof(ChassisCommand_t));
MessageCenter_Subscriber_Register(&chassis_sub, "chassis_cmd", sizeof(ChassisCommand_t));
```

发布者把一份数据复制到每个订阅者自己的队列；一个订阅者取走数据不会影响其他订阅者。
队列满时丢弃最旧的一份消息。这个示例使用固定大小静态池：最多 8 个话题、16 个订阅者、
每个订阅者 4 条消息、每条消息最多 64 字节，不使用 `malloc`。

`Examples/message_center_demo.c` 是两个 APP 的最小用法，未加入 OLED 的 CMake 构建。
示例实现适合单核、单生产者/单消费者的教学用途；真正接入 FreeRTOS 时，发布和取消息
需要在任务之间增加临界区、队列锁或直接使用 FreeRTOS queue。
'@

$readme = Join-Path $target 'README.md'
$readmeText = Get-Content -LiteralPath $readme -Raw
$readmeText = $readmeText.Replace('2. `APP/app_init.c`：集中配置总线、地址和超时，注册 OLED 实例并初始化。', '2. `APP/app_init.c`：只汇总和安排 APP 初始化。\n3. `APP/app_oled.c`：配置 I2C 和地址，注册 OLED，初始化屏幕并编写显示逻辑。')
$readmeText = $readmeText.Replace('3. `APP/app_oled.c`：屏幕上显示什么，以及何时更新。', '4. `Modules/OLED/oled.c`：SSD1306 命令、显存和文本/画线接口。')
$readmeText = $readmeText.Replace('4. `Modules/OLED/oled.c`：SSD1306 命令、显存和文本/画线接口。', '5. `Modules/OLED/oled.c`：SSD1306 命令、显存和文本/画线接口。')
$readmeText = $readmeText.Replace('5. `Bsp/iic/bsp_iic.c`：I2C 设备实例池、地址转换和 HAL 同步收发。', '6. `Bsp/iic/bsp_iic.c`：I2C 设备实例池、地址转换和 HAL 同步收发。')
$readmeText = $readmeText.Replace('main -> App_Init -> OLED_Register -> IIC_Register\n                 -> OLED_Init\n                 -> App_OLED_Init', 'main -> App_Init -> App_OLED_Init -> OLED_Register -> IIC_Register\n                                  -> OLED_Init')
$readmeText += @'

## APP 初始化和消息中心

`App_Init()` 只负责汇总各 APP 的初始化；具体 APP 自己配置和注册它拥有的模块。
例如本例程是 `App_Init() -> App_OLED_Init() -> OLED_Register() -> IIC_Register()`。
OLED 只有一个 APP，不需要经过消息中心。消息中心用于 `robot_cmd`、`chassis`、
`gimbal` 等平行 APP 之间交换命令和反馈，最小固定池示例见
`Modules/MessageCenter/` 和 `Examples/message_center_demo.c`；它未加入本例程构建。
'@
Save-Text 'README.md' $readmeText

Write-Output "Updated OLED APP layout and added message center example. Backup: $backup"
