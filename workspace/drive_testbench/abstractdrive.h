#pragma once

#include <stdint.h>
#include <functional>

class AbstractDrive
{
public:
    AbstractDrive(uint8_t id) : m_id(id) {}
    
    inline uint8_t id() const {return m_id;}
    
    enum ControlMode
    {
        ModeOff,
        ModePWM,
        ModeTorqueOpenLoop, //  the current loop
        ModeSpeed,
        ModeServo,
        ModeTorqueClosedLoop,
        ModeImpedance
    };
    
    void setPWM(float value_percent)
    {
        targetPWM = value_percent;
        mode = ModePWM;
    }
    
    void setTorque(float value_Nm)
    {
        m_controlTorque = value_Nm;
        mode = ModeTorqueOpenLoop;
    }
    
    void setSpeed(float value_rad_s)
    {
        targetSpeed = value_rad_s;
        mode = ModeSpeed;
    }
    
    void setPos(float value_rad, float max_speed_rad_s = 0)
    {
        targetPos = value_rad;
        targetSpeed = max_speed_rad_s;
        mode = ModeServo;
    }
    
    void setImpedanceControl(float pos, float speed, float Tff, float Kp, float Kd)
    {
        targetPos = pos;
        targetSpeed = speed;
        targetTorque = Tff;
        impKp = Kp;
        impKd = Kd;
        mode = ModeImpedance;
    }
    
    ControlMode mode = ModeOff;
    bool enabled = false;
    float targetPos = 0;
    float targetSpeed = 0;
    float targetTorque = 0; 
    float targetPWM = 0;
    float impKp = 0;
    float impKd = 0;
    
    float Tfr = 0;
    float Tfrw = 0;
    float J = 0;
    float accFilter = 1.0f;// 0.5f; 1.0f - no filter
    
    float zeroPos = 0;
    
    float gearRatio = 1;
    float torqueConstant = 1;
    
    const float &pos = m_pos;
    const float &speed = m_speed;
    const float &torque = m_torque;
    const float &voltage = m_voltage;
    const float &temperature = m_temperature;
    const bool &present = m_present;
    const float &accel = m_accel;
    const float &controlTorque = m_controlTorque;
    const float &measuredTorque = m_measuredTorque;
    
    virtual int errorCode() const {return 0;}
    
    virtual void update(float dt) = 0;
    virtual bool receiveState() = 0;
   
    bool m_present = false;
    
    virtual void setZero() {zeroPos += m_pos;}
    
    std::function<void(void)> onStateReceive;
     
protected:
    uint8_t m_id = 0;
    
    bool m_enabled = false;
    virtual void setEnabled(bool enabled)
    {
        m_enabled = enabled;
    }
    
//    bool m_present = false;
    float m_pos = 0;
    float m_speed = 0;
    float m_torque = 0; // it is motor current multiplied by Kt
    float m_voltage = 0;
    float m_temperature = 0;
    float m_accel = 0;
    
    float m_measuredTorque = 0; // measurement result from the torque sensor
    float m_controlTorque = 0; // it is sent to the drive
};
