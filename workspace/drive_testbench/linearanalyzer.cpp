#include "linearanalyzer.h"

using namespace Objnet;

LinearAnalyzer::LinearAnalyzer(SignalGenerator *gen) :
    m_gen(gen)
{
}

void LinearAnalyzer::bindInputSignal(const std::string &name, const float &var)
{
    if (!m_inputMap.count(name))
        m_inputNames.push_back(name);
    m_inputMap[name] = &var;
    m_input = &var;
}

Objnet::ObjnetNode *LinearAnalyzer::createOnbNode(Objnet::ObjnetInterface *iface)
{
    ObjnetNode *node = new ObjnetNode(iface);
    node->setClassId(cidController | cidGeneric | 0x24);
    node->setName("Analyzer");
    node->setFullName("Linear analyzer");
    node->bindVariable("freqStart", freqStart);
    node->bindVariable("freqEnd", freqEnd);
    node->bindVariable("targetAmplitude", m_gen->amplitude);
    node->bindVariable("duration", m_gen->duration);
    node->BindMethodEx("start/stop", this, &LinearAnalyzer::startStop);
    node->bindOutput("active", m_gen->isStarted());
    node->bindOutput("freq", m_gen->curFreq);
    node->bindOutput("magnitude", m_mag);
    node->bindOutput("phase", m_phase);
    node->bindOutput("phaseDetectorOutput", m_pdout);
//    node->bindOutput("avg", m_avg);
    node->bindOutput("rms", m_rms);
//    node->bindOutput("trigPhase", m_trigPhase);
    node->bindOutput("amplitude", m_ampl);
    node->bindVariable("Kd", Kd);
//    node->bindVariable("magFilter", amplFilter);
//    node->bindVariable("phaseFilter", phaseFilter);
    node->bindObject(ObjectInfo("inputs", m_inputNames, ObjectInfo::ReadOnly));
    node->BindMethodEx("set input", this, &LinearAnalyzer::setInput);
    return node;
}

void LinearAnalyzer::startStop()
{
    if (m_gen->isActive())
        m_gen->stop();
    else
        m_gen->chirp2(freqStart, freqEnd);
}

void LinearAnalyzer::update(float dt)
{
    float in = 0;
    if (m_input)
        in = *m_input;
    
    if (!m_gen->isActive())
    {
        m_phase = NAN;
        m_mag = NAN;
        m_oldPhase = NAN;
        m_avg = NAN;
        m_int = 0;
        m_rms = 0;
        m_cnt = 0;
        return;
    }
    
    if (!isnan(m_oldPhase) && m_gen->curPhase < m_oldPhase)
        newWave();
    m_oldPhase = m_gen->curPhase;
    
    float phase = m_phase;
    if (isnan(phase))
        phase = 0;
    
    float cf = cosf(m_gen->curPhase);
    float sf = sinf(m_gen->curPhase);
    
    // calculate VCO output
    m_vcoout = m_gen->amplitude * cosf(m_gen->curPhase + phase);//cf;
    // calculate Phase Detector output
    m_pdout = m_vcoout * in;
    // update loop filter
    m_int += m_pdout;
    m_cnt++;
    
    // accumulate complex amplitude
    m_cfint += 2 * cf * in;
    m_sfint += 2 * sf * in;
    
    if (m_maxInput < in)
    {
        m_maxInput = in;
        m_maxPhase = M_PI/2 - m_gen->curPhase;
    }
}

void LinearAnalyzer::newWave()
{
    m_trigPhase = m_maxPhase;
    float gain = m_maxInput / m_gen->amplitude;
    
    if (m_cnt)
    {
        m_avg = m_int / (m_cnt * m_gen->amplitude);
//        m_rms = (m_rmsint / m_cnt) / m_gen->amplitude;
        gain = sqrtf(m_cfint*m_cfint + m_sfint*m_sfint) / m_cnt / m_gen->amplitude;
        float phase = atan2f(m_cfint, m_sfint);
        if (m_phase - phase > M_PI)
            m_rms = phase + 2 * M_PI;
        else if (m_phase - phase < -M_PI)
            m_rms = phase - 2 * M_PI;
        else
            m_rms = phase;
    }
    else
    {
        m_avg = 0;
        m_rms = 0;
    }
    m_int = 0;
    m_rmsint = 0;
    m_cfint = 0;
    m_sfint = 0;
    m_cnt = 0;
    
    m_ampl = m_maxInput;
    m_mag = 20 * log10f(gain);
    
    // calculate PLL error
    float sinph = 0;
    float a = m_ampl;
    if (a < m_gen->amplitude * 0.1f)
        a = m_gen->amplitude * 0.1f;
    sinph = m_avg / a;
    sinph = BOUND(-1, sinph, 1);
    if (isnan(m_phase))
        m_phase = m_rms;//maxPhase;
    else
        m_phase += Kd * asinf(sinph);
    
    m_maxInput = *m_input;
    
    if (onSampleReady)
        onSampleReady();
}

void LinearAnalyzer::setInput(std::string name)
{
    if (m_inputMap.count(name))
        m_input = m_inputMap.at(name);
}
