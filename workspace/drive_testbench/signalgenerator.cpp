#include "signalgenerator.h"

using namespace Objnet;

SignalGenerator::SignalGenerator() :
    out(u),
    curFreq(chirp_freq),
    curPhase(chirp_phase)
{
}

Objnet::ObjnetNode *SignalGenerator::createOnbNode(Objnet::ObjnetInterface *iface)
{
    ObjnetNode *node = new ObjnetNode(iface);
    node->setClassId(cidController | cidGeneric | 0x23);
    node->setName("SigGen");
    node->setFullName("Signal generator");
    node->bindOutput("out", out);
    node->BindMethodEx("step", this, &SignalGenerator::step);
    node->BindMethodEx("pulse", this, &SignalGenerator::pulse);
    node->BindMethodEx("ramp", this, &SignalGenerator::ramp);
    node->BindMethodEx("sine", this, &SignalGenerator::sine);
    node->BindMethodEx("square", this, &SignalGenerator::square);
    node->BindMethodEx("sawtooth", this, &SignalGenerator::sawtooth);
    node->BindMethodEx("halfsaw", this, &SignalGenerator::halfSawtooth);
    node->BindMethodEx("triangle", this, &SignalGenerator::triangle);
    node->BindMethodEx("chirp", this, &SignalGenerator::chirp);
    node->bindVariable("amplitude", amplitude);
    node->bindVariable("duration", duration);
    node->bindVariable("duty cycle", dutyCycle);
    return node;
}

float SignalGenerator::update(float dt)
{
    m_dt = dt;
    if (m_time > 0)
    {
        if (!m_started)
        {
            m_started = true;
            if (onStart)
                onStart();
        }
        
        if (chirp_freq)
        {           
            chirp_phase += 2*M_PI * chirp_freq * dt;
            if (chirp_phase >= 2*M_PI)
            {
                chirp_phase -= 2*M_PI;
                if (onPhaseWrap)
                    onPhaseWrap();
            }
            
            float ph = remainderf(chirp_phase, 2*M_PI);
            float t = ph / M_PI;
            
            switch (m_waveform)
            {
            case Sine:
                u = amplitude * sinf(chirp_phase);
                break;
            case Square:
                u = (chirp_phase < M_PI)? amplitude: -amplitude;
                break;
            case Sawtooth:
                u = amplitude * t;
                break;
            case HalfSawtooth:
                u = (t >= 0)? amplitude * t: amplitude * (-1.0f - t);
                break;
            case Triangle:
                u = (chirp_phase < M_PI/2  )? 2 * amplitude * t:
                    (chirp_phase < M_PI    )? 2 * amplitude * (1.0f - t):
                    (chirp_phase < 3*M_PI/2)? 2 * amplitude * (-1.0f - t):
                                              2 * amplitude * t;
                break;
            }
            
            if (t > dutyCycle || (t < 0 && (-1.0f - t) < -dutyCycle))
                u = 0;
            chirp_freq *= m_rate;
        }
        else if (m_rate)
        {
            u += m_rate * dt;
        }
        else
        {
            u = amplitude;
        }
        
        if (m_ptr)
            *m_ptr = u;
        
        m_time -= dt;
    }
    else 
    {        
        m_time = 0;
        u = 0;
        if (m_ptr && m_started)
            *m_ptr = 0;
        chirp_freq = 0;
        
        if (m_started)
        {
            m_started = false;
            if (onEnd)
                onEnd();
        }
    }
    return u;
}

void SignalGenerator::step()
{
    m_time = duration;
    m_rate = 0;
}

void SignalGenerator::pulse()
{
    m_time = m_dt;
    m_rate = 0;
}

void SignalGenerator::ramp()
{
    m_time = duration;
    m_rate = amplitude / duration;
}

void SignalGenerator::sine(float freq)
{
    chirp_freq = freq;
    chirp_phase = 0;
    // align duration within frequency
    m_time = floorf(freq * duration + m_dt) / freq;
    m_rate = 1.f;
    m_waveform = Sine;
}

void SignalGenerator::square(float freq)
{
    sine(freq);
    m_waveform = Square;
}

void SignalGenerator::sawtooth(float freq)
{
    sine(freq);
    m_waveform = Sawtooth;
}

void SignalGenerator::halfSawtooth(float freq)
{
    sine(freq);
    m_waveform = HalfSawtooth;
}

void SignalGenerator::triangle(float freq)
{
    sine(freq);
    m_waveform = Triangle;
}

void SignalGenerator::chirp(float max_freq)
{
    chirp_freq = 10; // initialize
    chirp_phase = 0;
    m_time = duration;
    m_rate = powf(max_freq / chirp_freq, m_dt / duration);
    m_waveform = Sine;
}

void SignalGenerator::chirp2(float init_freq, float max_freq)
{
    chirp_freq = init_freq; // initialize
    chirp_phase = 0;
    m_time = duration;
    m_rate = powf(max_freq / chirp_freq, m_dt / duration);
    m_waveform = Sine;
}

void SignalGenerator::stop()
{
    m_time = 0;
    u = 0;
    if (m_ptr)
        *m_ptr = 0;
}