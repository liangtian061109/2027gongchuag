# X 型全向轮底盘重构笔记

## 1. 当前目标

使用四个张大头 Y42 闭环步进电机搭建 X 型全向轮底盘，逐步实现：

1. 读取四轮多圈实时位置。
2. 计算单轮实际滚动距离。
3. 通过 X 型正运动学计算车体前进、横移和旋转量。
4. 通过 X 型逆运动学把车体目标速度拆成四轮速度。
5. 使用速度模式同步控制四个电机。
6. 使用位置 PID 实现走固定距离。
7. 加入陀螺仪航向 PID，抑制直线行驶时的偏航。

当前已完成第 1 至第 5 步的代码实现，其中第 4、5 步等待实车验证。

## 2. 统一约定

### 2.1 电机编号

| CAN 地址 | 车轮位置 | 代码名称 |
| --- | --- | --- |
| 1 | 左前轮 | `Chassis_Motor_Left_Front` |
| 2 | 右前轮 | `Chassis_Motor_Right_Front` |
| 3 | 左后轮 | `Chassis_Motor_Left_Rear` |
| 4 | 右后轮 | `Chassis_Motor_Right_Rear` |

目前约定四台电机的 `dir = 0` 都对应车体向前运动时的正方向。后续必须通过实际运行确认编码器返回值的正负方向是否与此一致。

### 2.2 车体坐标

- 前进方向为 `x` 正方向。
- 向右横移为 `y` 正方向。
- 逆时针旋转为正方向。
- 距离单位使用 `mm`。
- 线速度单位使用 `mm/s`。
- 角度单位使用 `rad`。
- 角速度单位使用 `rad/s`。

### 2.3 底盘参数

当前代码暂定：

```c
#define CHASSIS_WHEEL_DIAMETER_MM 85.0f
#define CHASSIS_COUNTS_PER_REVOLUTION 65536.0f
#define CHASSIS_WHEEL_BASE_MM 250.0f
#define CHASSIS_TRACK_WIDTH_MM 250.0f
```

其中：

- `WHEEL_DIAMETER`：轮子直径。
- `COUNTS_PER_REVOLUTION`：电机转一圈对应的实时位置增量。
- `WHEEL_BASE`：前后轮中心距离。
- `TRACK_WIDTH`：左右轮中心距离。

这些值后续需要根据实车测量和标定结果调整。

## 3. 为什么使用 S_CPOS

底盘使用：

```c
Emm_V5_Read_Sys_Params(addr, S_CPOS);
```

`S_CPOS` 的功能码是 `0x36`，返回格式为：

```text
0x36 + 符号 + uint32 实时位置 + 0x6B
```

驱动器返回的是带符号的多圈实时位置。数值经过一圈后会继续累计，例如：

```text
65000 -> 66000 -> 131072
```

因此主控不需要处理单圈编码器的 `65535 -> 0` 回绕。

每个轮子的相对位置直接计算为：

```text
相对计数 = 当前多圈位置 - 启动时多圈位置
```

这种方法即使漏掉中间一次 CAN 采样，也不会丢失累计路程。

## 4. 四轮异步轮询

`Chassis_Init()` 完成：

1. 清空四轮状态。
2. 清除四轮位置更新标志。
3. 从 1 号电机开始请求 `S_CPOS`。

`Chassis_Update()` 按以下顺序运行：

```text
请求1号 -> 等待1号回包 -> 更新1号
请求2号 -> 等待2号回包 -> 更新2号
请求3号 -> 等待3号回包 -> 更新3号
请求4号 -> 等待4号回包 -> 更新4号
重新请求1号
```

一次只请求一台电机，可以避免四台电机同时应答造成 CAN 总线拥挤。

每个轮子保存：

```c
typedef struct {
    int32_t Origin_Position;
    int32_t Current_Position;
    int32_t Relative_Count;
    uint8_t Initialized;
} Chassis_Wheel_Position_t;
```

第一次收到位置时保存为 `Origin_Position`。后续计算：

```c
Relative_Count = Current_Position - Origin_Position;
```

## 5. 编码器计数转换为轮子距离

一圈对应 `65536` 个计数：

```text
圈数 = 相对计数 / 65536
```

轮子周长：

```text
轮子周长 = pi * 轮子直径
```

单轮滚动距离：

```text
轮子距离 = 相对计数 / 65536 * pi * 轮子直径
```

代码入口：

```c
float Chassis_Get_Wheel_Distance(uint8_t addr);
```

返回值单位为 `mm`，负数表示轮子向约定正方向的反方向转动。

## 6. X 型底盘正运动学

四个轮子的滚动距离命名为：

```text
left_front_mm  左前轮距离
right_front_mm 右前轮距离
left_rear_mm   左后轮距离
right_rear_mm  右后轮距离
```

### 6.1 前进距离

```text
forward =
    (left_front + right_front + left_rear + right_rear)
    / (2 * sqrt(2))
```

### 6.2 横移距离

```text
lateral =
    (-left_front + right_front + left_rear - right_rear)
    / (2 * sqrt(2))
```

### 6.3 旋转角度

旋转等效半径：

```text
rotation_radius = (wheel_base + track_width) / 2
```

编码器推算旋转角：

```text
rotation =
    (-left_front + right_front - left_rear + right_rear)
    / (2 * sqrt(2) * rotation_radius)
```

结果单位为 `rad`。

每当 4 号电机完成更新后，调用一次 `Chassis_Odometry_Update()`，保证本轮四个位置都已经刷新。

当前读取接口：

```c
float Chassis_Get_Forward_Distance(void);
float Chassis_Get_Lateral_Distance(void);
float Chassis_Get_Rotation_Rad(void);
```

## 7. 当前数据流

```text
S_CPOS 多圈实时位置
        |
        v
相对启动位置的计数
        |
        v
四个轮子的滚动距离
        |
        v
X 型正运动学
        |
        +--> 前进距离 mm
        +--> 横移距离 mm
        +--> 编码器推算旋转角 rad
```

## 8. X 型底盘逆运动学

已加入以下接口：

```c
void Chassis_Compute_Wheel_Velocities(
    float forward_mm_s,
    float lateral_mm_s,
    float rotation_rad_s,
    Chassis_Wheel_Velocity_t *wheel_velocity);

void Chassis_Set_Velocity(
    float forward_mm_s,
    float lateral_mm_s,
    float rotation_rad_s,
    uint8_t acc,
    bool synchronous);
```

逆运动学公式为：

```text
rotation_velocity = wz * (wheel_base + track_width) / 2

left_front  = (vx - vy - rotation_velocity) / sqrt(2)
right_front = (vx + vy + rotation_velocity) / sqrt(2)
left_rear   = (vx + vy - rotation_velocity) / sqrt(2)
right_rear  = (vx - vy + rotation_velocity) / sqrt(2)
```

`Chassis_Set_Velocity()` 会把轮子线速度换算成 RPM 和方向。若任一轮超过
`5000 RPM`，四轮会同比例缩放，以保持合成运动方向不变。同步模式下先向
四台电机发送带同步标志的速度命令，再用地址 `0` 广播触发同步运动。

## 9. 下一步

先用低速悬空测试验证三个基本动作：

```text
前进：Chassis_Set_Velocity(100, 0, 0, acc, true)
右移：Chassis_Set_Velocity(0, 100, 0, acc, true)
逆时针：Chassis_Set_Velocity(0, 0, 0.2, acc, true)
停止：Chassis_Set_Velocity(0, 0, 0, acc, true)
```

确认四轮方向和 `CPOS` 正负号符合约定后，再编写距离 PID。陀螺仪加入后，编码器负责测量平移距离，陀螺仪负责主要航向反馈。

## 10. 注意事项

1. 目前公式建立在四轮安装方向和编码器符号符合约定的前提下。
2. 如果某个轮子向前转动时 `CPOS` 减小，需要为该轮加入方向修正系数 `-1`。
3. 编码器推算角度会受到轮胎打滑影响，不能替代陀螺仪航向角。
4. 走固定距离时应使用速度模式加软件闭环，而不是到达阈值后简单立即停车。
5. PID 初期先使用 P 控制，确认方向正确后再逐步加入 D，最后评估是否需要 I。
