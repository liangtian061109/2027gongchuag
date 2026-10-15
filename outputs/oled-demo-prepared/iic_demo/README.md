# 独立 OLED / I2C 教学例程

这个工程只运行一个 OLED 示例，没有例程切换、电机初始化或 FreeRTOS。
UART、CAN、张大头电机等后续例程应各自建立独立工程。

上电后显示 `OLED I2C DEMO`、六位计数、分层提示和进度条。
计数约每 500 ms 增加一次。100 kHz I2C 全屏刷新需要约 100 ms，因此
500 ms 是刷新间隔安排，不是严格的实时周期；计数不代表精确运行秒数。

## 硬件和接线

本例程针对 **SSD1306、128x64、四针 I2C OLED**。不适用于 SPI 接口、
128x32 或 SH1106 屏幕。CubeMX 当前选型是 STM32F407VET6，168 MHz。

| OLED 引脚 | STM32 / 电源 |
| --- | --- |
| GND | GND，必须共地 |
| VCC | 3.3 V，确认屏幕模块支持该电压 |
| SCL | PB8，I2C1_SCL |
| SDA | PB7，I2C1_SDA |

I2C 为 100 kHz。当前 GPIO 配置没有启用内部上拉，SDA/SCL 需要外部上拉；
若模块板已有上拉无需重复添加，并确保上拉电压为 3.3 V。
默认 **7 位地址 0x3C**；若模块为 0x3D，只修改 `APP/app_init.c` 的 `dev_addr`。
HAL 使用的左移地址在 BSP 内部处理，应用不要填写 0x78。

## 从哪里读代码

1. `Core/Src/main.c`：HAL、时钟、外设初始化，等待上电后调用 `App_Init()`，
   主循环调用 `App_Run(HAL_GetTick())`。
2. `APP/app_init.c`：集中配置总线、地址和超时，注册 OLED 实例并初始化。
3. `APP/app_oled.c`：屏幕上显示什么，以及何时更新。
4. `Modules/OLED/oled.c`：SSD1306 命令、显存和文本/画线接口。
5. `Bsp/iic/bsp_iic.c`：I2C 设备实例池、地址转换和 HAL 同步收发。

```text
main -> App_Init -> OLED_Register -> IIC_Register
                 -> OLED_Init
                 -> App_OLED_Init
main -> App_Run  -> App_OLED_Update -> 绘制显存 -> OLED_Refresh -> IIC_Transmit
```

`Register` 的实现放在对应模块里；具体硬件配置和注册调用放在 APP。
OLED 内部已经注册 I2C 设备，不需要应用再调用一次 `IIC_Register()`。
实例在启动时注册一次，使用静态数组，不使用 malloc。一个 I2C 实例代表
一个地址上的设备；不同设备可以共享同一个硬件句柄。

`OLED_Clear/Show_String/Show_Num/Draw_Line` 只修改 RAM 显存；
一帧绘制完成后再调用 `OLED_Refresh()`，不要每画一个像素就刷屏。
字体固定为 8x16 ASCII，一行最多 16 个字符。基础例程没有中文点阵和无限滚动接口。
8x16 字形沿用原 uart 工程的 `asc2_1608` 字库。

## 编译和烧录

准备 CMake >= 3.22、Ninja、Arm GNU 工具链，并将三个工具的 bin 目录加入 PATH。
也可在 STM32 VS Code 扩展环境中打开工程，使用它提供的构建工具。

```powershell
cmake --preset Debug
cmake --build --preset Debug
```

输出位于 `build/Debug/`：

- `iic_demo.elf`：调试文件。
- `iic_demo.hex` / `iic_demo.bin`：烧录文件，BIN 写入地址为 `0x08000000`。

使用 STM32CubeProgrammer 或现有调试器烧录即可。工程不绑定个人电脑的
工具安装目录，也没有默认自动烧录操作。

## 调试和错误处理

初始化失败会进入 `Error_Handler()`；连接调试器后可调用 `App_Get_Status()`，
或直接查看 `APP/app_init.c` 内的 `app_status`。

| 状态 | 含义 |
| --- | --- |
| APP_READY | 注册、初始化及首次刷新成功 |
| APP_REGISTER_FAILED | 配置无效、重复地址或实例池已满 |
| APP_OLED_INIT_FAILED | 屏幕未应答或初始化传输失败 |
| APP_OLED_REFRESH_FAILED | 首帧或运行中的刷新失败 |

`app_oled->iic->last_status` 保存最后一次 HAL 传输结果。
运行中通信失败会停止后续刷新，不会无限重试；检查接线和地址后重新上电。
I2C 接口采用有限超时的同步传输，仅在主循环/任务使用，不在中断里调用。
本例程只有一个调用者，不包含多任务总线锁；移植到 RTOS 后应统一串行访问总线。

## CubeMX 再生成

对 main 的修改均位于 `USER CODE BEGIN/END` 中；保留 CubeMX 的用户代码选项。
自定义代码独立放在 APP、Bsp 和 Modules 中，只修改根 CMakeLists.txt 添加源文件，
不手工修改 `cmake/stm32cubemx/CMakeLists.txt`。
当前 CubeMX 已配置 I2C TX DMA，但本例程使用同步发送，不调用 DMA 发送。
原配置里的 PB9/I2S2_WS 没有被 OLED 使用，后续可在 CubeMX 中取消该多余配置。

## 新人练习

- 修改 `app_oled.c` 的标题，观察一行 16 个字符的边界。
- 改变计数更新间隔和显示位数。
- 增加斜线或用点绘制图形，保持每帧只刷新一次。
- 修改 `app_init.c` 中的设备地址，观察错误状态，再改回实际地址。

关于二开 RM 框架的建议见 `docs/rm_framework_secondary_development.md`。
