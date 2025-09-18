#pragma once

#include "abstractdrive.h"
#include "caninterface.h"

class EncosDrive : public AbstractDrive
{
public:
    EncosDrive(CanInterface *can, int id, int fifo_id=-1);
    void bindTenso(const float *ptr) {m_tensoPtr = ptr;}
    
    virtual void update(float dt) override;
    virtual bool receiveState() override;
    
    virtual void setZero() override;
    
    float extSpeed = 0;
    float extSpeedFilter = 0.5f;
//    float golayPos = 0;
//    float golaySpeed = 0;
//    float golayAcc = 0;
    
    float posMax = 10.0f; // position scale
    float velMax = 20.0f; // velocity scale
    float torqueMax = 90.0f; // target torque scale
    float torqueMaxIn = 125.0f; // real torque scale
    
private:
    void sendImpedanceControl();
    
    CanInterface *m_can;
    int m_fifoId;
    
    float m_dt = 0.001f;
    float m_oldPos = 0;
    float m_oldSpeed = 0;
    const float *m_tensoPtr = nullptr;
    
    float m_poses[9]{0};
    
    int m_cmdTicks = 0;
    enum Command // CAN_ID for commands = 0x7FF
    {
        NoCommand = 0,
        SetAutoResponseMode = 1, // ID_H, ID_L, 0x00, 0x01
        SetRequestResponseMode = 2, // ID_H, ID_L, 0x00, 0x02
        SetZero = 3, // ID_H, ID_L, 0x00, 0x03
        SetId = 4, // ID_H, ID_L, 0x00, 0x04, NEWID_H, NEWID_L
        ResetId = 5, // 0x7F, 0x7F, 0x00, 0x05, 0x7F, 0x7F (reset ID to 1)
        
        GetResponseMode = 0x81, // ID_H, ID_L, 0x00, 0x81 (return 0x01 or 0x02)
        ScanId = 0x82, // 0xFF, 0xFF, 0x00, 0x82 // all motors send their IDs
    } m_activeCommand = NoCommand;
    
    void sendCommand(Command cmd, uint16_t param = 0);
};