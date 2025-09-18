#ifndef _APP_H
#define _APP_H

#define APP_VERSION     0x010a
#define APP_CLASS       (cidController | cidMechanical | cidLeggedRobot | 0x02)
#define APP_BURNCOUNT   0

//#define M_PI    3.1415926f

#include <stdlib.h>
#include "core/application.h"
#include "ethernet/ethernet.h"
#include "led.h"
#include "core/timer.h"
#include "usart.h"
#include "hardwaretimer.h"

#include "objnet/objnetnode.h"
#include "objnet/genericonbinterface.h"
#include "objnet/objnetmaster.h"
//#include "objnet/uartonbinterface.h"
#include "objnet/udponbinterface.h"
#include "objnet/onbvirtualinterface.h"
#include "objnet/canonbinterface.h"
#include "objnet/uartonbinterface.h"

#include "can.h"
#include "cansocket.h"

#include "motors/canopendrive.h"
#include "signalgenerator.h"
#include "linearanalyzer.h"

using namespace Objnet;

class App : public Application
{
private:
    ObjnetNode *onode;
    Ethernet *eth;
    Timer *timer;
    HardwareTimer *controlLoopTimer;
    int controlLoopFreq = 1000;
    int controlLoopWatchdog = 0;
    
    bool isMotorUpdating = false;
    
    Led *ledGreen, *ledRed;
    
    AbstractDrive *drive = nullptr;
    SignalGenerator *gen = nullptr;
    LinearAnalyzer *anal = nullptr;
    
    ObjnetMaster *virtMaster = nullptr;
    
    OnbVirtualInterfacePool *vip;
    
    bool enable = false;
    bool oldEnable = false;
    
public:
    App();
    void mainTask();
    void onControlTimer();
    void controlLoop(float dt);
    void onStateUpdated();
    
    void setDrivesEnabled(bool enabled);
    void onValueChanged(std::string name);
    
    void writeSdo();
    void readSdo();
    void resetDrive();

    void showException(Exception::Reason e);
};

APP_DECLARE_BOOT_INFO();

#endif