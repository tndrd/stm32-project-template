#include "app.h"
#include <precisetimer.h>

bool frictionControl = false;
bool pidControl = false;

float noise = 0;

uint16_t reg = 0;
uint8_t subid = 0;
uint32_t val = 0;
int cosize = 4;

int startupCnt = 1000;

uint8_t nmtState;

App::App() : Application()
{          
    // test pins:
//    Gpio::config(Gpio::PE12, Gpio::Output);
//    Gpio::config(Gpio::PE13, Gpio::Output);
//    Gpio::config(Gpio::PE14, Gpio::Output);
//    Gpio::config(Gpio::PE15, Gpio::Output);
    
    ledGreen = new Led(Gpio::PD12, true);
    ledGreen->on();
    ledRed = new Led(Gpio::PD14, true);
    
    Can *can = new Can(Gpio::CAN1_RX_PD0, Gpio::CAN1_TX_PD1, 500000);
    can->open();
    
    /// @todo implement CanOpenDrive class (according to CAN DS-402 manual)
    /// The class should be inherited from CanOpenProxy and AbstractDrive

    CanOpenDrive *drv = new CanOpenDrive(can, 127);
    drive = drv;
//    drive->mode = AbstractDrive::ModeImpedance;
    
    drive->onStateReceive = std::bind(&App::onStateUpdated, this);
    
    /// @note App::onStateUpdated() must be called
    /// when the drive receives response.
    /// This is necessary for the analyzer to work properly!
    
   
    Ethernet::rxBufCount = 32;
    Ethernet::txBufCount = 32;
    Ethernet::rxBufSize = 512;
    Ethernet::txBufSize = 512;
    RMII rmii;
    rmii.phyAddress = 0x01;
    rmii.pinReset = Gpio::PA0;
    eth = Ethernet::instance(rmii);
    eth->setupNetworkInterface("192.168.0.100", "255.255.255.0", "192.168.0.1");

    // create ONB node on the UDP interface
    ObjnetInterface *iface = new UdpOnbInterface;
    onode = new ObjnetNode(iface);
    if (onode->busAddress() == 0xFF)
        onode->setBusAddress(1);
    onode->setClassId();
    onode->setName("DrvTest");
    onode->setFullName("Drive testing and identification tool");
    onode->onPolling = CLOSURE(ledGreen, &Led::toggle);
    onode->onObjectValueChanged = EVENT(&App::onValueChanged);
    
    onode->bindIO("enable", enable);
    onode->bindSetting("loop frequency", controlLoopFreq);
    onode->bindSetting("frictionControl", frictionControl);
    onode->bindVariable("reg", reg);
    onode->bindVariable("subid", subid);
    onode->bindVariable("val", val);
    onode->bindVariable("size", cosize);
    onode->BindMethodEx("read", this, &App::readSdo);
    onode->BindMethodEx("write", this, &App::writeSdo);
    onode->BindMethodEx("reset drive", this, &App::resetDrive);
    onode->bindOutput("NMT state", nmtState);
    
    drv->onSdoReceive = [=](uint16_t id, uint8_t subid, uint32_t value)
    {
        val = value;
        onode->sendForced(5);
    };
    
    char ipaddrstr[16];
    sprintf(ipaddrstr, "192.168.0.%d", 100 + onode->busAddress());
    eth->setAddress(ipaddrstr);
    
    // create interface for internal virtual devices
    vip = new OnbVirtualInterfacePool;
    virtMaster = new ObjnetMaster(new OnbVirtualInterface(vip));
    
    // create and attach drive virtual node
    ObjnetNode *node = new ObjnetNode(new OnbVirtualInterface(vip));
    node->setBusAddress(1);
    node->setClassId(cidBrushlessMotor);
    node->setName("drive");
    node->setFullName("The drive");
    if (drive)
    {
        node->bindOutput("present", drive->present);
        node->bindIO("enable", drive->enabled);
        node->bindOutput("pos", drive->pos);
        node->bindOutput("speed", drive->speed);
        node->bindOutput("torque", drive->torque);
        node->bindOutput("torqueSensor", drive->measuredTorque);
        node->bindIO("targetPos", drive->targetPos);
        node->bindIO("targetSpeed", drive->targetSpeed);
        node->bindIO("targetTorque", drive->targetTorque);
        node->bindIO("controlTorque", drive->controlTorque);
        node->bindIO("Kp", drive->impKp);
        node->bindIO("Kd", drive->impKd);
        node->bindOutput("accel", drive->accel);
        node->bindVariable("accFilter", drive->accFilter);
        node->bindVariable("Tfr", drive->Tfr);
        node->bindVariable("Tfrw", drive->Tfrw);
        node->bindVariable("J", drive->J);
        node->BindMethodEx("set zero", drive, &AbstractDrive::setZero);
    }
    
    // create and attach SignalGenerator node
    gen = new SignalGenerator();
    ObjnetNode *gennode = gen->createOnbNode(new OnbVirtualInterface(vip));
    gennode->setBusAddress(15);
    gennode->onObjectValueChanged = EVENT(&App::onValueChanged);
    
    // create and attach LinearAnalyzer node
    anal = new LinearAnalyzer(gen);
    if (drive)
    {
        gen->bindOutput(&drive->targetTorque);
        anal->bindInputSignal("torque", drive->torque);
        anal->bindInputSignal("position", drive->pos);
        anal->bindInputSignal("speed", drive->speed);
        anal->bindInputSignal("accel", drive->accel);
        anal->bindInputSignal("targetTorque", drive->targetTorque);
        anal->bindInputSignal("controlTorque", drive->controlTorque);
        anal->bindInputSignal("torqueSensor", drive->measuredTorque);
        anal->setInput("torque");
    }
    ObjnetNode *analnode = anal->createOnbNode(new OnbVirtualInterface(vip));
    analnode->setBusAddress(14);
    
    // connect virtual device network to the upper node
    onode->connect(virtMaster);

    // create control loop timer
    controlLoopTimer = new HardwareTimer(HardwareTimer::Tim2);
    controlLoopTimer->setFrequency(controlLoopFreq);
    controlLoopTimer->setUpdateEvent(EVENT(&App::onControlTimer));
    controlLoopTimer->start();
    
    registerTaskEvent(EVENT(&App::mainTask));
}

void App::readSdo()
{
    CanOpenDrive *drv = dynamic_cast<CanOpenDrive*>(drive);
    drv->sdoRead(reg, subid, cosize);
}

void App::writeSdo()
{
    CanOpenDrive *drv = dynamic_cast<CanOpenDrive*>(drive);
    drv->sdoWrite(reg, subid, val, cosize);
}

void App::resetDrive()
{
    CanOpenDrive *drv = dynamic_cast<CanOpenDrive*>(drive);
    drv->reset();
}

void App::mainTask()
{
    if (enable != oldEnable)
        setDrivesEnabled(enable);
    oldEnable = enable;
    
    CanOpenDrive *drv = dynamic_cast<CanOpenDrive*>(drive);
    if (drv)
        nmtState = drv->nmtState();
       
////    test[0] = drive->torque;
////    test[1] = drive->targetTorque;
////    test[3] = drive->speed * 10;
////    test[4] = drive->accel * 0.1f;
}

void App::onControlTimer()
{
    controlLoop(1.0f / controlLoopFreq);
}

void App::controlLoop(float dt)
{    
    if (startupCnt)
    {
        --startupCnt;
        return;
    }
    
//    /// @todo make this async on real state reception
//    onStateUpdated();
  
    gen->update(dt);
    
//    // a kind of control may be here...
    
//    test[0] = drive->torque;
//    test[1] = drive->targetTorque;
//    test[3] = drive->speed * 10;
//    test[4] = drive->accel * 0.1f;
//    
    if (drive)
        drive->update(dt);
    
    // issue only one SYNC over all the network!
    CanOpenDrive *drv = dynamic_cast<CanOpenDrive*>(drive);
    if (drv)
        drv->sync();
    
//    test[2] = drive->targetTorque;
  
    noise = rnd(1.0f);
}

void App::onStateUpdated()
{    
//    test[2] = drive->targetTorque; // target torque propagated

    float dt = 1.0f / controlLoopFreq;
    if (anal)
        anal->update(dt);
}

void App::setDrivesEnabled(bool enabled)
{
    if (drive)
        drive->enabled = enabled;
}

void App::onValueChanged(std::string name)
{
    if (name == "loop frequency")
    {
        controlLoopTimer->setFrequency(controlLoopFreq);
    }
}

void App::showException(Exception::Reason e)
{
    while (1)
    {
        ledRed->toggle();
        for (int w=1000000; --w;);
    }
}
