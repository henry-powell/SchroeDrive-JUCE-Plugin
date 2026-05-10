/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include "LFO.h"

//==============================================================================
/**
*/
class SchroeDistortionAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SchroeDistortionAudioProcessor();
    ~SchroeDistortionAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    // this function sets up the APVTS parameters, and must be passed to the APVTS constructor when it is created
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState mParameters;

    // parameter ID strings to register with the APVTS
    static constexpr const auto wetDryId = "wetDry";
    static constexpr const auto decayId = "decay";
    static constexpr const auto dampingId = "damping";
    static constexpr const auto rmFreqId = "rmFreq";
    static constexpr const auto overdriveId = "overdrive";
    
    // some fixed values for the number of early reflections, comb filters, and all pass filters
    static constexpr size_t numEarlyReflections = 5;
    static constexpr size_t numCombFilters = 4;
    static constexpr size_t numAllPassFilters = 2;

    // some helpful fixed values for the min/max of some parameters
    static constexpr float minRingModFreqHz = 1.0f;
    static constexpr float maxRingModFreqHz = 300.0f;
    static constexpr float minOverdriveDb = 0.0f;
    static constexpr float maxOverdriveDb = 48.0f;
    
private:
    
    // various helper functions for our main DSP processes
  

    float processRingMod (float inputSample);
    float processSoftClip (float inputSample);
    float processRMEarlyReflections (float inputSample);
    float processCombFilter (juce::dsp::DelayLine<float>& delayLine, float& filterState, float inputSample, int delayTimeSamples, float feedbackGain);
    float processAllPassFilter (juce::dsp::DelayLine<float>& delayLine, float inputSample, int delayTimeSamples, float gain);
    
    // a helper function to load APVTS parameters and store the results in our primary member variables below
    void updateParameterValues();

    // member variables to store the current parameter values. we'll use these directly in processBlock()
    double mSampleRate;
    float mOutputGain;
    float mWetDryMix;
    float mDecay;
    float mDamping;
    float mOverdrive;
    float mRingModFreq;
    int mMaxEarlyReflectionDelayTimeSamples;
    int mMaxCombDelayTimeSamples;
    int mMaxAllPassDelayTimeSamples;
    
    // create arrays to hold the various delay times and gains for our early reflection stage, comb filter stage, and all-pass filter stage
    std::array<float, numEarlyReflections> mEarlyReflectionDelayTimesMs;
    std::array<float, numEarlyReflections> mEarlyReflectionGains;
    std::array<float, numCombFilters> mCombDelayTimesMs;
    std::array<float, numCombFilters> mCombFeedbackGains;
    std::array<float, numAllPassFilters> mAllPassDelayTimesMs;
    std::array<float, numAllPassFilters> mAllPassGains;
    
    // these arrays will hold the result of converting the delay times to samples once we know the host sampling rate
    std::array<int, numEarlyReflections> mEarlyReflectionDelayTimesSamples;
    std::array<int, numCombFilters> mCombDelayTimesSamples;
    std::array<int, numAllPassFilters> mAllPassDelayTimesSamples;
    
    // create a DelayLine for the early reflection stage before the comb filters
    juce::dsp::DelayLine<float> mEarlyReflectionDelayLine;
    
    // create arrays of DelayLine objects for the comb and all-pass filters
    std::array<juce::dsp::DelayLine<float>, numCombFilters> mCombDelayLines;
    std::array<float, numCombFilters> mCombFilterStates;
    
    std::array<juce::dsp::DelayLine<float>, numAllPassFilters> mAllPassDelayLines;

    // make an LFO instance for our ring modulation
    LFO mRingModLfo;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SchroeDistortionAudioProcessor)
};
