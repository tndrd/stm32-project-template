#ifndef _SIGNALGENERATOR_H
#define _SIGNALGENERATOR_H

#include "core/core.h"
#include "core/advmath.h"
#include "objnet/objnetnode.h"

class SignalGenerator
{
public:
    SignalGenerator();
    Objnet::ObjnetNode *createOnbNode(Objnet::ObjnetInterface *iface);
    
    float amplitude = 1;
    float duration = 0.5; // seconds
    float dutyCycle = 1;
    
    const float &out;
    void bindOutput(float *ptr) {m_ptr = ptr;}
    
    const float &curFreq;
    const float &curPhase;
    
    float curTime() const {return m_time? duration - m_time: 0;}
    
    void step();
    void pulse();
    void ramp();
    void sine(float freq);
    void square(float freq);
    void sawtooth(float freq);
    void halfSawtooth(float freq);
    void triangle(float freq);
    void chirp(float max_freq);
    void chirp2(float init_freq, float max_freq);
    
    void stop();
    
    bool isActive() const {return m_time > 0 || u != 0;}
    const bool &isStarted() const {return m_started;}
    float update(float dt);
    
    NotifyEvent onStart;
    NotifyEvent onPhaseWrap;
    NotifyEvent onEnd;
  
private:
    float m_time = 0;
    float m_rate = 0;
    float chirp_freq = 0;
    float chirp_phase = 0;
    enum Waveform
    {
        Sine,
        Square,
        Sawtooth,
        HalfSawtooth,
        Triangle,
    };
    Waveform m_waveform = Sine;
    float m_dt = 0.001f;
    
    float u = 0;
    float *m_ptr = nullptr;
    bool m_started = false;
};

#endif