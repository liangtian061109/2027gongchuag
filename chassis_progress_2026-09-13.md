# 底盘开发进展更新（2026-09-13）

本文件补充《交流记录_底盘开发.md》。

## 已完成

- Chassis_Load_Motor_Speed() 已建立。
- Chassis_Send_Motor_Speeds() 已建立：
  - 装载1、2、3、4号电机命令。
  - 最后调用 Emm_V5_Multi_Motor_Cmd(0)。
- Chassis.h 已加入 stdint.h。

## 当前待修正

1. Chassis_Load_Motor_Speed() 已删除 snf 参数，因此最后一行应为：

    Emm_V5_MMCL_Vel_Control(addr, dir, vel, acc, false);

2. RPM限速暂按手册改为3000：

    if (rpm > 3000.0f)
    {
      rpm = 3000.0f;
    }

3. Chassis.h 中删除 static Chassis_Load_Motor_Speed(...) 声明。
   static函数只供Chassis.c内部使用，不放到头文件。

4. Chassis_Send_Motor_Speeds(...) 在Chassis.h中保留一份声明即可。

5. main.c当前只做Chassis_Kinematics()，尚未调用Chassis_Send_Motor_Speeds()，电机还不会动作。

## 接下来的顺序

1. 修正以上四点。
2. 编译。
3. 在main.c增加低速、架空测试。
4. 观察四轮方向，再做前进、横移、旋转测试。

