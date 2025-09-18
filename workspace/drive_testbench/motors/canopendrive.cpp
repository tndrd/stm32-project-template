#include "canopendrive.h"
#include "canopen/objdict.h"
#include <math.h>

using namespace CanOpen;
using namespace CanOpen::ObjDict;
using namespace CanOpen::ObjDict::CiA402;

CanOpenDrive::CanOpenDrive(CanInterface *can, uint8_t nodeId) :
    CanOpenProxy(can, nodeId),
    AbstractDrive(nodeId)
{
    gearRatio = 0.001f * 6.5f / (1 << 17);
    torqueConstant = 2.3f;
    
    timer = new Timer();
    timer->onTimeout = EVENT(&CanOpenDrive::onTimer);
    timer->setInterval(200);
    
    reset();
}

void CanOpenDrive::reset()
{
    nmtModuleControl(NMT_ResetNode);
    m_initState = NotInitialized;
}

void CanOpenDrive::setEnabled(bool enabled)
{
    if (enabled)
    {
        sdoWrite<ControlWord>(ControlWord::CmdPowerOff);
        sdoWrite<ControlWord>(ControlWord::CmdPowerOn);
        sdoWrite<ControlWord>(ControlWord::CmdEnable);
    }
    else
    {
        sdoWrite<TargetTorque>(0);
        sdoWrite<ControlWord>(ControlWord::CmdQuickStop);
        sdoWrite<ControlWord>(ControlWord::CmdPowerOff);
    }
    AbstractDrive::setEnabled(enabled);
}

void CanOpenDrive::clearFault()
{
//    if (isFault())
//        sendControlWord(???);
//    m_errorState = 0;
}

void CanOpenDrive::initialize()
{
//    nmtModuleControl(NMT_EnterPreOperationalState);    
    /*
    configPdo(PDO1_TX, {0x60640020, 0x606C0020}, 1, true);
    // PDO2_TX doesn't work!!!! but WHY?
    configPdo(PDO3_TX, {0x60770010, 0x60780010, 0x22050110, 0x60410010}, 1, true);
    
    configPdo(PDO1_RX, {0x60400010});//, 1, true);
    configPdo(PDO3_RX, {0x60710010});//, 1, true);
    */

    configPDOSync<TPDO1, Pdo1Tx>(1);
    configPDOSync<TPDO3, Pdo2Tx>(1);

    configPDOSync<RPDO1, Pdo1Rx>(1);
    configPDOSync<RPDO3, Pdo3Rx>(1);
    
    sdoWrite<TargetTorque>(0);
    sdoWrite<SetOpMode>(SetOpMode::PTorque);
    
    setEnabled(true);
    
    sdoWrite<ProfileVelocity>(3500000);
    sdoWrite<ProfileAcceleration>(200000000);
    sdoWrite<ProfileDeceleration>(200000000);
        
    // 1. Start Slave Station Frame
    nmtModuleControl(NMT_StartRemoteNode);
    
    /// @attention DIRTY HACK to go to initialized state
    sdoWrite<TargetPos>(1000);
  
}

bool CanOpenDrive::isPresent() const
{
    return m_present;
}

bool CanOpenDrive::isReady() const
{
    return (m_initState == Initialized) && (statusWord() & (1 << 0));
}

bool CanOpenDrive::isFault() const
{
    return statusWord() & (1 << 3);
}

void CanOpenDrive::sendControlWord(uint16_t word)
{
    sdoWrite<ControlWord>(word);
}

void CanOpenDrive::setZero()
{
    /// @todo use command to set zero inside the drive
    AbstractDrive::setZero();
}

//void CanOpenDrive::setTargetSpeed(int value)
//{
//    if (m_initState != Initialized)
//        return;
//    
//    Pdo2Rx pdo = {value};
//    ByteArray ba(&pdo, sizeof(Pdo2Rx));
//    pdoWrite(2, ba);
//}

void CanOpenDrive::setTargetTorque(int16_t value)
{
    if (m_initState != Initialized)
        return;
    
    Pdo3Rx pdo = {value};
    ByteArray ba(&pdo, sizeof(Pdo3Rx));
    pdoWrite(3, ba);
}

void CanOpenDrive::sdoWritten(uint16_t id, uint8_t subid)
{
    if (id == 0x607A)
    {
        /// @attention DIRTY HACK!
        m_initState = Initialized;
//        timer->start();
    }
//    if (id == 0x6060)
//    {
//        sdoWrite32(0x60ff, 0x00, 0); // set Target_Speed to 0
//        nmtModuleControl(NMT_StartRemoteNode);
//    }
}

void CanOpenDrive::nmtStateChanged()
{
    m_guardResponse = true;
    
    if (nmtState() == Bootup)
    {
        if (m_initState == NotInitialized)
        {
            m_initState = Initializing;
            initialize();
        }    
    }
    else if (nmtState() == Operational)
    {
        if (m_initState == Initializing)
        {
            // write 0x000F to PDO1 (Control_Word)
            // 0x0006
            Pdo1Rx pdo = {0x000f};
            ByteArray ba(&pdo, sizeof(Pdo1Rx));
            pdoWrite(1, ba);
            m_initState = Initialized;
        }
    }
    
    if (m_initState == Reset)
    {
        nmtModuleControl(NMT_ResetNode);
        m_initState = NotInitialized;
    }
}

void CanOpenDrive::pdoReceived(uint8_t pdo, const ByteArray &value)
{
    switch (pdo)
    {
    case 1:
      m_state1 = *reinterpret_cast<const Pdo1Tx *>(value.data());
      m_speed = m_state1.realSpeed * gearRatio;
      m_pos = m_state1.actualPos * gearRatio;
      break;
      
    case 3: /// @attention should be case 2, but TPDO2 doesn't work
      m_state2 = *reinterpret_cast<const Pdo2Tx *>(value.data());
      m_torque = m_state2.torque * torqueConstant;
      m_measuredTorque = m_state2.torqueSensor + 450; /// @todo remove this HARDCODE!
      if (onStateReceive)
          onStateReceive();
      if (m_state2.statusWord & (1 << 3))
      {
          //sdoRead16(0x2601, 0x00); // read ErrorState
      }
      break;
      
//    case 3:
//      m_state3 = *reinterpret_cast<const Pdo3Tx *>(value.data());
//      break;
      
    default: return;
    }
}

void CanOpenDrive::sdoReceived(uint16_t id, uint8_t subid, uint32_t value)
{
    if (id == 0x2601 && subid == 0x00)
        m_errorState = value;
    else if (id == 0x100B && subid == 0x00)
        readNodeId = value;
    
    if (onSdoReceive)
        onSdoReceive(id, subid, value);
}

void CanOpenDrive::onTimer()
{    
    m_present = m_guardResponse;
    m_guardResponse = false;
    nmtErrorControl();
}


void CanOpenDrive::update(float dt)
{
    m_dt = dt;
    
    if (enabled != m_enabled)
        setEnabled(enabled);
        
    if (m_enabled)
    {
        float t_fr = copysignf(Tfr, m_speed) + Tfrw * m_speed;
        float t_J = J * m_accel;
        float tau = (targetPos - pos) * impKp + (targetSpeed - speed) * impKd + targetTorque;
        m_controlTorque = tau + t_fr + t_J;
        setTargetTorque(static_cast<int>(m_controlTorque / torqueConstant));       
    }
    
    float acc = (m_speed - m_oldSpeed) / m_dt;
    m_oldSpeed = m_speed;
    m_accel += (acc - m_accel) * accFilter; 
}