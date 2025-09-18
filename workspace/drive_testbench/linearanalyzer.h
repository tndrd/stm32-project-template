#pragma once

#include "signalgenerator.h"
#include "objnet/objnetnode.h"
#include <functional>

class LinearAnalyzer
{
public:
    LinearAnalyzer(SignalGenerator *gen);
    void bindInputSignal(const std::string &name, const float &var);
    
    Objnet::ObjnetNode *createOnbNode(Objnet::ObjnetInterface *iface);
    
//    const float &phase;
//    const float &mag;
    
    float freqStart = 10;
    float freqEnd = 1000;
    
    float Kd = 1; // phase detector gain
    
    void update(float dt);
    std::function<void(void)> onSampleReady;
    
    void startStop();
    
    void setInput(std::string name);
    
private:
    SignalGenerator *m_gen;
    const float *m_input = nullptr; // current input
    
    float m_mag = 0; // magnitude
    float m_trigPhase = 0;
    
    // phase detector:
    float m_int = 0;
    float m_cnt = 0;
    float m_pdout = 0; // phase detector output
    float m_avg = 0; // loop filter output
    float m_phase = 0; // phase shift
    float m_vcoout = 0; // VCO output
    float m_ampl = 0;
    float m_rmsint = 0;
    float m_cfint = 0;
    float m_sfint = 0;
    float m_rms = 0;
    
    float m_maxInput = 0;
    float m_maxPhase = 0;
    float m_oldPhase = 0;
    
    std::map<std::string, const float *> m_inputMap;
    std::vector<std::string> m_inputNames;
    
    void newWave();
};