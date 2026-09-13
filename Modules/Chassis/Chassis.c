#include "Chassis.h"
#include "Emm_V5.h"
#define PI   3.1415926f

#define WHEEL_DIAMETER_MM 85.0f

#define CHASSIS_WHEEL_BASE_MM 250.0f  // 前后轮中心距离
#define CHASSIS_TRACK_WIDTH_MM 250.0f // 左右轮中心距离

static float Wheel_ConvertMmPerSecToRpm(float Speed_mm)
{
  float wheel_circumference_mm;
  float Rotation_Per_Second;
  float RPM;

  wheel_circumference_mm = PI * WHEEL_DIAMETER_MM;
  Rotation_Per_Second = Speed_mm / wheel_circumference_mm;
  RPM = Rotation_Per_Second * 60.0f;

  return RPM;
}

static void Chassis_Send_Motor_Speed(uint8_t addr, float rpm,uint8_t acc)
{
  uint8_t dir;
  uint16_t vel;

  if(rpm >= 0.0f)
  {
    dir = 0;
  }
    else
    {
        dir = 1;
        rpm = -rpm;
    }

  if(rpm > 5000.0f)
  {
    rpm = 5000;
  }

  vel = (uint16_t)(rpm + 0.5f);
  /*
   * 先让这条命令进入驱动器的等待队列（snF = true）。
   * 最后由 Emm_V5_Synchronous_motion() 让四台电机同时开始。
   */
  Emm_V5_Vel_Control(addr, dir, vel, acc, true);
}

void Chassis_Send_Motor_Speeds(const Chassis_Motor_Speed_s *Motor_Speed, uint8_t acc)
{
    if (Motor_Speed == 0) {
        return;
    }
    
    Chassis_Send_Motor_Speed(1, Motor_Speed->Motor1_Speed, acc);
    HAL_Delay(10);
    Chassis_Send_Motor_Speed(2, Motor_Speed->Motor2_Speed, acc);
    HAL_Delay(10);
    Chassis_Send_Motor_Speed(3, Motor_Speed->Motor3_Speed, acc);
    HAL_Delay(10);
    Chassis_Send_Motor_Speed(4, Motor_Speed->Motor4_Speed, acc);
    HAL_Delay(10);

    /* 广播同步执行：地址 0 表示所有已等待同步的电机。 */
    Emm_V5_Synchronous_motion(0);
}



void Chassis_Kinematics(float Forward_Speed, float Translational_Speed,
                        float Rotate_Speed, Chassis_Motor_Speed_s *Motor_Speed)
{
  float Forward_rpm;
  float Translational_rpm;
  float Rotate_rpm;
  float Rotate_Linear_Speed;

  if (Motor_Speed == 0) {
    return;
  }

  Forward_rpm = Wheel_ConvertMmPerSecToRpm(Forward_Speed);
  Translational_rpm = Wheel_ConvertMmPerSecToRpm(Translational_Speed);
  Rotate_Linear_Speed = Rotate_Speed * (CHASSIS_WHEEL_BASE_MM / 2.0f + CHASSIS_TRACK_WIDTH_MM / 2.0f);
  Rotate_rpm = Wheel_ConvertMmPerSecToRpm(Rotate_Linear_Speed);
  Motor_Speed->Motor1_Speed = Forward_rpm - Translational_rpm - Rotate_rpm;

  Motor_Speed->Motor2_Speed = Forward_rpm + Translational_rpm + Rotate_rpm;

  Motor_Speed->Motor3_Speed = Forward_rpm + Translational_rpm - Rotate_rpm;

  Motor_Speed->Motor4_Speed = Forward_rpm - Translational_rpm + Rotate_rpm;
}