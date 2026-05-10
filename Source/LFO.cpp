#include "LFO.h"

#include <JuceHeader.h>
#include <algorithm>
#include <cmath>

LFO::LFO()
{
    mSampleRate = 48000.0;
    mPhase = 0.0;
    mPhaseIncrement = 0.0;
    mFrequencyHz = 1.0f;
    mWaveform = Waveform::sine;

    updatePhaseIncrement();
}

void LFO::prepare (double sampleRate)
{
    mSampleRate = std::max (sampleRate, 1.0);
    updatePhaseIncrement();
}

void LFO::reset (float startPhase)
{
    mPhase = std::clamp (static_cast<double> (startPhase), 0.0, 1.0);
}

void LFO::setFrequencyHz (float newFrequencyHz)
{
    // allow positive or negative frequencies so the LFO phase can move forward or backward.
    mFrequencyHz = newFrequencyHz;
    updatePhaseIncrement();
}

float LFO::getFrequencyHz() const
{
    return mFrequencyHz;
}

void LFO::setWaveform (Waveform newWaveform)
{
    mWaveform = newWaveform;
}

LFO::Waveform LFO::getWaveform() const
{
    return mWaveform;
}

float LFO::processSample()
{
    const auto sample = renderWaveform();

    // advance the phase by one sample according to the current frequency. negative frequencies will move the phase backward.
    mPhase += mPhaseIncrement;

    // if the phase grows past the top of the 0-1 range, wrap it back down into range.
    while (mPhase >= 1.0)
        mPhase -= 1.0;

    // if the phase drops below 0 because of a negative frequency, wrap it back up into range.
    while (mPhase < 0.0)
        mPhase += 1.0;

    return sample;
}

float LFO::getCurrentValue() const
{
    return renderWaveform();
}

void LFO::updatePhaseIncrement()
{
    mPhaseIncrement = static_cast<double> (mFrequencyHz) / mSampleRate;
}

float LFO::renderWaveform() const
{
    switch (mWaveform)
    {
        case Waveform::sine:
            return static_cast<float> (std::sin (juce::MathConstants<double>::twoPi * mPhase));

        case Waveform::saw:
            return static_cast<float> ((2.0 * mPhase) - 1.0);

        case Waveform::triangle:
        {
            const auto triangle = 1.0 - (4.0 * std::abs (mPhase - 0.5));
            return static_cast<float> (triangle);
        }
    }

    return 0.0f;
}
