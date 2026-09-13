#ifndef CHASSIS_H
#define CHASSIS_H

#include <stdint.h>

typedef struct
{
  float Motor1_Speed; // 1号：左前轮
  float Motor2_Speed; // 2号：右前轮
  float Motor3_Speed; // 3号：左后轮
  float Motor4_Speed; // 4号：右后轮
} Chassis_Motor_Speed_s;

void Chassis_Kinematics(float Forward_Speed, float Translational_Speed,
                        float Rotate_Speed, Chassis_Motor_Speed_s *Motor_Speed);
void Chassis_Send_Motor_Speeds(const Chassis_Motor_Speed_s *Motor_Speed,
                               uint8_t acc);
#endif