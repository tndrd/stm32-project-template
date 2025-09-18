#pragma once

#include "abstractdrive.h"
#include "canopen/canopenproxy.h"
#include "canopen/objdict.h"
#include "core/timer.h"
#include <functional>

class CanOpenDrive : public CanOpenProxy, public AbstractDrive
{
public:
    CanOpenDrive(CanInterface *can, uint8_t nodeId);
    void setDirectionInverted(bool value) {m_direction = value? 1: 0;}
    
    uint8_t readNodeId = 0; // for tests
    
    std::function<void(uint16_t, uint8_t, uint32_t)> onSdoReceive;
  
    void reset();
    
    virtual void setEnabled(bool enabled) override;
    void clearFault(); /// @todo implement clearFault()

    virtual void setZero() override; /// @todo implement setZero()
    
    bool isPresent() const;
    bool isReady() const;
    bool isFault() const;
    
    void sendControlWord(uint16_t word);
//    void setTargetSpeed(int value);
    
    const int32_t &realSpeed() const {return m_state1.realSpeed;}
    const int32_t &actualPos() const {return m_state1.actualPos;}
    const int16_t &torque() const {return m_state2.torque;}
    const int16_t &current() const {return m_state2.current;}
    const int16_t &torqueSensor() const {return m_state2.torqueSensor;}
    const uint16_t &statusWord() const {return m_state2.statusWord;}
//    const uint16_t &controlWord() const {return m_state2.controlWord;}
//    const int32_t &targetSpeed() const {return m_state3.targetSpeed;}
//    const uint16_t &dinStatus() const {return m_state3.dinStatus;}
    const uint16_t &errorState() const {return m_errorState;}
    
protected:
    virtual void initialize();
    virtual void update(float dt) override;
    virtual bool receiveState() override {return false;}
    
private:
    virtual void nmtStateChanged() override;
//    virtual void emergency() override {}
    virtual void pdoReceived(uint8_t pdo, const ByteArray &value) override;
    virtual void sdoReceived(uint16_t id, uint8_t subid, uint32_t value) override;
    virtual void sdoWritten(uint16_t id, uint8_t subid) override;
    virtual void sdoError(SDOAbortCode errcode) override {}
    
private: 
    struct Pdo1Tx
    {
        using Layout = PDO<CanOpen::ObjDict::CiA402::PositionActualValue,
                           CanOpen::ObjDict::CiA402::VelocityActualValue>;
        
        int32_t actualPos;
        int32_t realSpeed;
    } __attribute__((packed, aligned(4)));
    
    struct Pdo2Tx
    {
        using Layout = PDO<CanOpen::ObjDict::CiA402::TorqueActualValue,
                           CanOpen::ObjDict::CiA402::CurrentActualValue,
                           CanOpen::ObjDict::AnalogInputMiliVolts,
                           CanOpen::ObjDict::CiA402::StatusWord>;

        int16_t torque;
        int16_t current;
        int16_t torqueSensor;
        uint16_t statusWord;
    } __attribute__((packed, aligned(4)));
    
//    struct Pdo3Tx
//    {
//        int32_t targetSpeed;
//        uint16_t controlWord;
//    } __attribute__((packed, aligned(4)));
    
    struct Pdo1Rx
    {
        using Layout = PDO<CanOpen::ObjDict::CiA402::ControlWord>;
        
        uint16_t controlWord;
    };
    
//    struct Pdo2Rx
//    {
//        int32_t targetPos;
//    };
    
    struct Pdo3Rx
    {
        using Layout = PDO<CanOpen::ObjDict::CiA402::TargetTorque>;
        
        int16_t targetTorque;
    };
    
    Pdo1Tx m_state1 {0};
    Pdo2Tx m_state2 {0};
//    Pdo3Tx m_state3 {0};
    
    uint16_t m_errorState = 0;
    
    bool m_guardResponse = false;
    bool m_present = false;
    uint8_t m_direction = 0;
    
    Timer *timer;
    enum {Reset, NotInitialized, Initializing, Initialized} m_initState = Reset;
    
    void onTimer();
    
    void setTargetTorque(int16_t value);
    float m_oldSpeed = 0;
    float m_dt = 0.001f;
};

