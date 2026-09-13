# 底盘开发进展更新 03（2026-09-13）

## 编译结果

多电机底盘代码已编译通过：

- Chassis_Load_Motor_Speed() 是Chassis.c内的私有static函数。
- Chassis_Send_Motor_Speeds() 会装载4台电机命令。
- Emm_V5_Multi_Motor_Cmd(0) 发送整条多电机命令。
- 本次编译无底盘模块警告和错误。

## 下一次实机测试

只在main.c初始化完成后执行一次：

    Chassis_Kinematics(100.0f, 0.0f, 0.0f,
                       &Chassis_Motor_Speed);

    Chassis_Send_Motor_Speeds(&Chassis_Motor_Speed, 20);

    HAL_Delay(1000);

    Emm_V5_Stop_Now(0, false);

含义：

- 100 mm/s：逻辑前进速度。
- 0、0：不横移、不旋转。
- acc=20：不要直接启动，先低加速度测试。
- 延时1000 ms：只作为一次性测试的运行时间。
- 地址0的立即停止命令：广播停止全部电机。

## 安全要求

- 必须让四个轮子架空。
- 手和线缆远离轮子。
- 确认有快速断电方式。
- 第一次仅观察转向，不在地面跑。

