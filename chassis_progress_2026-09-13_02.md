# 底盘开发进展更新 02（2026-09-13）

补充前一份进展记录。

## 当前完成

- 多电机装载函数已改为 Emm_V5_MMCL_Vel_Control。
- 四电机发送函数会装载地址1、2、3、4，最后调用 Emm_V5_Multi_Motor_Cmd(0)。
- 运动学四轮公式保持正确。

## 当前必须修正

1. 删除 Chassis.h 中这一整行：

    void Chassis_Load_Motor_Speed(uint8_t addr, float rpm, uint8_t acc);

理由：Chassis_Load_Motor_Speed在Chassis.c中定义为static私有函数，只给该文件内部使用；不应放进头文件。否则头文件声明的外部函数和C文件中的static函数会产生链接属性冲突。

2. Chassis.c中限速改为：

    if (rpm > 3000.0f)
    {
      rpm = 3000.0f;
    }

## 下一步

修正后先编译。通过后才在main.c中加入架空、低速、短时间的速度模式实机测试。

