#include "encosdrive.h"
#include <math.h>

#define KP_MIN  0.0f
#define KP_MAX  500.0f
#define KD_MIN  0.0f
#define KD_MAX  5.0f

//extern int float_to_uint(float x, float x_min, float x_max, int bits);
//extern float uint_to_float(int x_int, float x_min, float x_max, int bits);

int float_to_uint(float x, float x_min, float x_max, int bits)
{
    /// Converts a float to an unsigned int, given range and number of bits ///
    if (x > x_max)
        x = x_max;
    if (x < x_min)
        x = x_min;
    float span = x_max - x_min;
    float offset = x_min;
    return (int) ((x-offset)*((float)((1<<bits)-1))/span);
}

float uint_to_float(int x_int, float x_min, float x_max, int bits)
{
    /// converts unsigned int to float, given range and number of bits ///
    float span = x_max - x_min;
    float offset = x_min;
    return ((float)x_int + .5f)*span/((float)((1<<bits)-1)) + offset;
}

EncosDrive::EncosDrive(CanInterface *can, int id, int fifo_id) :
    AbstractDrive(id),
    m_can(can)
{
    m_fifoId = fifo_id < 0? m_id: fifo_id;
    can->configureFilter(CanInterface::StdId, m_id, 0xFFF, m_fifoId);
}

void EncosDrive::update(float dt)
{
//    m_present = receiveState();
    if (m_tensoPtr)
        m_measuredTorque = *m_tensoPtr;
    
    m_dt = dt;
    
    //! @todo Implement common message FIFO for messages from ID=0x7FF instead of waiting
    if (m_activeCommand)
    {
        if (m_cmdTicks > 0)
            --m_cmdTicks;
        else
        {
//            sendCommand(m_activeCommand);
            m_activeCommand = NoCommand;
        }
        return;
    }
    
    if (enabled && m_present)
    {
        if (mode == ModeTorqueOpenLoop)
        {
            m_controlTorque = targetTorque;
            impKp = impKd = 0;
            sendImpedanceControl();
        }
        else if (mode == ModeTorqueClosedLoop)
        {
            float err = targetTorque - m_measuredTorque;
            m_controlTorque = targetTorque + 0.1f*err;
            impKp = impKd = 0;
            sendImpedanceControl();
        }
        else if (mode == ModeImpedance)
        {
            float t_fr = copysignf(Tfr, m_speed) + Tfrw * m_speed;
            float t_J = J * m_accel;
            m_controlTorque = targetTorque + t_fr + t_J;            
            sendImpedanceControl();
        }
    }    
    else
    {
        m_controlTorque = 0;
        impKp = impKd = 0;
        sendImpedanceControl();
    }
}

void EncosDrive::sendImpedanceControl()
{
    int p_int = float_to_uint(targetPos + zeroPos, -posMax, posMax, 16);
    int v_int = float_to_uint(targetSpeed, -velMax, velMax, 12);
    int kp_int = float_to_uint(impKp, KP_MIN, KP_MAX, 12);
    int kd_int = float_to_uint(impKd, KD_MIN, KD_MAX, 9);
    int t_int = float_to_uint(m_controlTorque, -torqueMax, torqueMax, 12);

    uint8_t data[8];
    data[0] = (kp_int >> 7) & 0x1F;
    data[1] = ((kp_int & 0x7F) << 1) | ((kd_int & 0x100) >> 8);
    data[2] = kd_int & 0xFF;
    data[3] = p_int >> 8;
    data[4] = p_int & 0xFF;
    data[5] = v_int >> 4;
    data[6] = ((v_int & 0xF) << 4) | (t_int >> 8);
    data[7] = t_int & 0xFF;
    m_can->transmitMessage(CanInterface::StdId, m_id, data, 8);
}

bool EncosDrive::receiveState()
{
    uint8_t data[8];
    uint32_t can_id;
    if (m_can->receiveMessage(&can_id, data, 8, m_fifoId) == 8)
    {
//        if (msg.StdId != m_id)
//            return false;

        int p_int = (data[1] << 8) | data[2];
        int v_int = (data[3] << 4) | (data[4] >> 4);
        int t_int = ((data[4] & 0xF) << 8) | data[5];
        int temp_int = data[6];
        
        m_pos = uint_to_float(p_int, -posMax, posMax, 16) - zeroPos;
        m_speed = uint_to_float(v_int, -velMax, velMax, 12);
        m_torque = uint_to_float(t_int, -torqueMaxIn, torqueMaxIn, 12);
        m_temperature = (temp_int - 50) * .5f;
        
//        for (int i=0; i<8; i++)
//            m_poses[i] = m_poses[i+1];
//        m_poses[8] = m_pos;
//        const float *y = m_poses;
//        
//        const float a[9] = {-21, 14, 39, 54, 59, 54, 39, 14, -21};
//        const float b[9] = {86, -142, -193, -126, 0, 126, 193, 142, -86};
//        const float c[9] = {28, 7, -8, -17, -20, -17, -8, 7, 28};
//        
//        float golay[3] = {0, 0, 0};
//        for (int i=0; i<9; i++)
//        {
//            golay[0] += a[i] * y[i];
//            golay[1] += b[i] * y[i];
//            golay[2] += c[i] * y[i];
//        }
//        
//        golayPos = golay[0] / 231.f;
//        golaySpeed = golay[1] / (1188.f * m_dt);
//        golayAcc = golay[2] / (462.f * m_dt * m_dt);
        
        // or
//        golay[0] = 1.f/35.f *      (-3*y[0] + 12*y[1] + 17*y[2] + 12*y[3] - 3*y[4]);
//        golay[1] = 1.f/(12.f*m_dt) * (y[0] - 8*y[1] + 8*y[3] - y[4]);
//        golay[2] = 1.f/(7.f*m_dt*m_dt) * (2*y[0] - y[1] - 2*y[2] - y[3] + 2*y[4]);
        
        float dp = m_pos - m_oldPos;
        if (dp > posMax)
            dp -= 2 * posMax;
        else if (dp < -posMax)
            dp += 2 * posMax;
        float sp = dp / m_dt;
        m_oldPos = m_pos;
        extSpeed += (sp - extSpeed) * extSpeedFilter;
        
        float acc = (m_speed - m_oldSpeed) / m_dt;
        m_oldSpeed = m_speed;
        m_accel += (acc - m_accel) * accFilter; 

        m_present = true;
        return true;
    }
    m_present = false;
    return false;
}

void EncosDrive::sendCommand(Command cmd, uint16_t param)
{
    uint8_t data[6];
    data[0] = m_id >> 8;
    data[1] = m_id & 0xFF;
    data[2] = 0x00;
    data[3] = cmd;
    data[4] = param >> 8;
    data[5] = param & 0xFF;
    
    int size;
    switch (cmd)
    {
    case ResetId:
        data[0] = data[1] = data[4] = data[5] = 0x7F;
    case SetId:
        size = 6;
        break;
        
    case ScanId:
        data[0] = data[1] = 0xFF;
    default:
        size = 4;
    }
    
    m_activeCommand = cmd;
    m_can->transmitMessage(CanInterface::StdId, 0x7FF, data, size);
}

void EncosDrive::setZero()
{
    m_cmdTicks = 500; // wait for response for 500 updates
    //! @todo Implement common message FIFO for messages from ID=0x7FF instead of waiting
    sendCommand(SetZero);
}