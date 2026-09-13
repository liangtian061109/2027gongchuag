# 底盘开发进展（13）

## 八方向运动参数表

观察视角仍为：人站在车后、面向车头。

所有数值单位：前进/横移为 `mm/s`，旋转为 `rad/s`。下表均不旋转，因此最后一个参数为 `0.0f`。

| 运动方向 | 调用参数 |
| --- | --- |
| 前 | `Chassis_Kinematics(100.0f, 0.0f, 0.0f, &Chassis_Motor_Speed);` |
| 后 | `Chassis_Kinematics(-100.0f, 0.0f, 0.0f, &Chassis_Motor_Speed);` |
| 左 | `Chassis_Kinematics(0.0f, 100.0f, 0.0f, &Chassis_Motor_Speed);` |
| 右 | `Chassis_Kinematics(0.0f, -100.0f, 0.0f, &Chassis_Motor_Speed);` |
| 前左 | `Chassis_Kinematics(70.7f, 70.7f, 0.0f, &Chassis_Motor_Speed);` |
| 前右 | `Chassis_Kinematics(70.7f, -70.7f, 0.0f, &Chassis_Motor_Speed);` |
| 后左 | `Chassis_Kinematics(-70.7f, 70.7f, 0.0f, &Chassis_Motor_Speed);` |
| 后右 | `Chassis_Kinematics(-70.7f, -70.7f, 0.0f, &Chassis_Motor_Speed);` |

## 为什么斜向填 70.7 而非 100

速度是二维向量。若前进和横移都填 `100`，实际合速度会变成约 `141 mm/s`，比直行快。填 `70.7` 后，合速度仍约为 `100 mm/s`，便于各方向速度一致。
