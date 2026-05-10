#pragma once

class LFO
{
public:
    enum class Waveform
    {
        sine = 1,
        saw = 2,
        triangle = 3
    };

    LFO();

    void prepare (double sampleRate);
    void reset (float startPhase = 0.0f);

    void setFrequencyHz (float newFrequencyHz);
    float getFrequencyHz() const;

    void setWaveform (Waveform newWaveform);
    Waveform getWaveform() const;

    float processSample();
    float getCurrentValue() const;

private:
    void updatePhaseIncrement();
    float renderWaveform() const;

    double mSampleRate;
    double mPhase;
    double mPhaseIncrement;
    float mFrequencyHz;
    Waveform mWaveform;
};
