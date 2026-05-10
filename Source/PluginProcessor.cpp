/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SchroeDistortionAudioProcessor::SchroeDistortionAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#else
     :
#endif
       mParameters (*this, nullptr, "Parameters", createParameterLayout())
{
    mSampleRate = 48000.0;
    // this is an output gain scalar that will be applied to the final wet signal after all of the early reflection, ring modulation, comb filter, and all-pass filter processing. since that's a fairly dense processing chain, we'll drop the gain by a fifth.
    mOutputGain = 0.2f;
    
    // we can initialize our parameter member variables with zeros here first, but they'll be updated with the APVTS default values when updateParameterValues() is called at the end of the constructor.
    mWetDryMix = 0.0f;
    mDecay = 0.0f;
    mDamping = 0.0f;
    mRingModFreq = 0.0f;
    mOverdrive = 0.0f;
    
    // early reflection taps use short, uneven times to create a small room-like cluster before the late reverb.
    mEarlyReflectionDelayTimesMs = { 7.1f, 11.3f, 17.9f, 23.7f, 31.1f };
    mEarlyReflectionGains = { 0.55f, 0.42f, 0.34f, 0.27f, 0.21f };

    // comb delay times should be long enough to avoid a flanging effect
    mCombDelayTimesMs = { 29.7f, 37.1f, 41.1f, 43.7f };
    mCombFeedbackGains = { 0.805f, 0.827f, 0.783f, 0.764f };
    
    // all pass gains are shorter for smearing and diffusion
    mAllPassDelayTimesMs = { 5.0f, 1.7f };
    mAllPassGains = { 0.7f, 0.7f };
    
    // these will be calculated in prepareToPlay() once we know the host sampling rate
    mMaxEarlyReflectionDelayTimeSamples = 1;
    mMaxCombDelayTimeSamples = 1;
    mMaxAllPassDelayTimeSamples = 1;
    mEarlyReflectionDelayTimesSamples = { 1, 1, 1, 1, 1 };
    mCombDelayTimesSamples = { 1, 1, 1, 1 };
    mAllPassDelayTimesSamples = { 1, 1 };
    mCombFilterStates = { 0.0f, 0.0f, 0.0f, 0.0f };
    
    // call updateParameterValues at the end of the constructor to load parameters from the APVTS and update the corresponding member variables
    updateParameterValues();
}

SchroeDistortionAudioProcessor::~SchroeDistortionAudioProcessor()
{
}

//==============================================================================
const juce::String SchroeDistortionAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SchroeDistortionAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SchroeDistortionAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SchroeDistortionAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SchroeDistortionAudioProcessor::getTailLengthSeconds() const
{
    return 3.0;
}

int SchroeDistortionAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SchroeDistortionAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SchroeDistortionAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SchroeDistortionAudioProcessor::getProgramName (int index)
{
    return {};
}

void SchroeDistortionAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

float SchroeDistortionAudioProcessor::processRingMod (float inputSample)

{
    // get a sample from the LFO
    const auto lfoSample = mRingModLfo.processSample();
    // multiply the input sample by the LFO sample for ring modulation
    return inputSample * lfoSample;
}

float SchroeDistortionAudioProcessor::processSoftClip (float inputSample)
{
    // convert the overdrive amount from dB to linear gain
    const auto driveGain = juce::Decibels::decibelsToGain (mOverdrive);

    // use the tanh function to soft-clip the overdriven sample. we can divide by the tanh of the driveGain value to keep the magnitude of the clipped sample in check
    return std::tanh (inputSample * driveGain) / std::tanh (driveGain);
}

float SchroeDistortionAudioProcessor::processRMEarlyReflections (float inputSample)
{
    // initialize our running sum of early reflection samples with the input sample
    float reflectionSum = inputSample;
    
    // get one ring mod sample before we begin the early reflection stages. we'll reuse this value throughout the early reflection loop so the LFO phase only advances once per input sample.
    const auto ringModSample = mRingModLfo.processSample();

    // iterate with a loop that's bounded by the number of early reflections. on each iteration, pop a delayed sample using the delay time in samples for each stage, scale it with the gain for that stage, and accumulate the value into our running sum. note that we need to catch when we're on the final stage, and only advance the read position of the DelayLine for that stage.
    for (size_t i = 0; i < numEarlyReflections; ++i)
    {
        // initialize this boolean to false, because we don't want to advance the read position of the DelayLine until we're on the final early reflection stage
        bool advanceReadPos = false;
        
        // check if we're on the final early reflection stage. if so, we can allow the DelayLine to advance its read position.
        if (i == numEarlyReflections - 1)
            advanceReadPos = true;
            
        const auto tapSample = mEarlyReflectionDelayLine.popSample (0, static_cast<float> (mEarlyReflectionDelayTimesSamples[i]), advanceReadPos);
        
        // whatever stage we're on, we should scale the current tapSample value by its corresponding gain, and accumulate that result into the running sum variable.
        reflectionSum += tapSample * mEarlyReflectionGains[i]; // Error Here - reflections where not adding  
        
        // apply ring modulation after each early reflection stage, so the modulation compounds as the signal moves through the reflection cluster.
        reflectionSum *= ringModSample;
    }

    // push the original input sample into the DelayLine for next time.
    mEarlyReflectionDelayLine.pushSample (0, inputSample);

    // output the final result
    return reflectionSum;
}

float SchroeDistortionAudioProcessor::processCombFilter (juce::dsp::DelayLine<float>& delayLine, float& filterState, float inputSample, int delayTimeSamples, float feedbackGain)
{
    // a feedback comb filter feeds a delayed version of the signal back into the delay line.
    const auto delayedSample = delayLine.popSample (0, static_cast<float> (delayTimeSamples));
    // use a one-pole low-pass filter in the feedback path so high frequencies decay faster.
    filterState = ((1.0f - mDamping) * delayedSample) + (mDamping * filterState);
    const auto delayInput = inputSample + (filterState * feedbackGain);
    delayLine.pushSample (0, delayInput);
    return delayedSample;
}

float SchroeDistortionAudioProcessor::processAllPassFilter (juce::dsp::DelayLine<float>& delayLine, float inputSample, int delayTimeSamples, float gain)
{
    // an all-pass filter diffuses the reverb by combining feedforward and feedback around a delay.
    
    // read a delayed sample from the delay line.
    const auto delayedSample = delayLine.popSample (0, static_cast<float> (delayTimeSamples));
    // combine the delayed sample with a scaled, inverted copy of the input (feedforward).
    const auto outputSample = delayedSample - (gain * inputSample);
    // compute the sample that will be written back into the delay line (feedback)
    const auto delayInput = inputSample + (gain * outputSample);
    // push the new sample into the delay line for future reads.
    delayLine.pushSample (0, delayInput);
    // return the current all-pass filter output.
    return outputSample;
}

// this helper function does all the APVTS loading and assigns the values to member function variables. that way, we can just use the member variables in processBlock()
void SchroeDistortionAudioProcessor::updateParameterValues()
{
    // load the percent-based parameters and divide by 100 to convert to 0-1 range before assigning to our member variables.
    mWetDryMix = mParameters.getRawParameterValue (wetDryId)->load() / 100.0f;
    mDecay = mParameters.getRawParameterValue (decayId)->load() / 100.0f;
    mDamping = mParameters.getRawParameterValue (dampingId)->load() / 100.0f;
    
    // load the RM freq and overdrive values
    mRingModFreq = mParameters.getRawParameterValue (rmFreqId)->load();
    mOverdrive = mParameters.getRawParameterValue (overdriveId)->load();

    // now that the RM frequency is loaded, call the LFO's setFrequencyHz function
    mRingModLfo.setFrequencyHz (mRingModFreq);
}

//==============================================================================
void SchroeDistortionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 1;

    // since we're calling the size() method of std::array, declare the for loop variable as size_t. this is the standard unsigned integer type used in array indexing operations. these two loops do the conversion of our filter delay times in ms to samples so that we can initialize the DelayLine objects below.
    for (size_t i = 0; i < mEarlyReflectionDelayTimesMs.size(); ++i)
        mEarlyReflectionDelayTimesSamples[i] = static_cast<int> (std::round ((mEarlyReflectionDelayTimesMs[i] / 1000.0f) * static_cast<float> (sampleRate)));

    for (size_t i = 0; i < mCombDelayTimesMs.size(); ++i)
        mCombDelayTimesSamples[i] = static_cast<int> (std::round ((mCombDelayTimesMs[i] / 1000.0f) * static_cast<float> (sampleRate))); // ERROR HERE - used subtraction instead of multiplication

    for (size_t i = 0; i < mAllPassDelayTimesMs.size(); ++i)
        mAllPassDelayTimesSamples[i] = static_cast<int> (std::round ((mAllPassDelayTimesMs[i] / 1000.0f) * static_cast<float> (sampleRate)));

    // another benefit of using std::array - we can call std::max_element to search it from beginning to end for the largest value. note that it gives us the position of the largest value, not the value itself. that's why we must dereference it with * to get the actual value.
    mMaxEarlyReflectionDelayTimeSamples = *std::max_element (mEarlyReflectionDelayTimesSamples.begin(), mEarlyReflectionDelayTimesSamples.end());
    mMaxCombDelayTimeSamples = *std::max_element (mCombDelayTimesSamples.begin(), mCombDelayTimesSamples.end());
    mMaxAllPassDelayTimeSamples = *std::max_element (mAllPassDelayTimesSamples.begin(), mAllPassDelayTimesSamples.end());

    mEarlyReflectionDelayLine.prepare (spec);
    mEarlyReflectionDelayLine.setMaximumDelayInSamples (mMaxEarlyReflectionDelayTimeSamples + 1);
    mEarlyReflectionDelayLine.reset();

    // use a range-based for loop to go through all elements in mCombDelayLines. each element is a DelayLine object, so the loop sets the max delay, prepares, and resets each one.
    for (auto& delayLine : mCombDelayLines)
    {
        delayLine.prepare (spec);
        delayLine.setMaximumDelayInSamples (mMaxCombDelayTimeSamples + 1);
        delayLine.reset();
    }

    // initialize the previous sample values for the comb filters to silence
    mCombFilterStates.fill (0.0f);

    // prepare the DelayLine instances for the all-pass filtering
    for (auto& delayLine : mAllPassDelayLines)
    {
        delayLine.prepare (spec);
        delayLine.setMaximumDelayInSamples (mMaxAllPassDelayTimeSamples + 1);
        delayLine.reset();
    }

    // prepare the LFO for ring modulation
    mRingModLfo.prepare (sampleRate);
    mRingModLfo.reset();
    mRingModLfo.setWaveform (LFO::Waveform::sine);
}

void SchroeDistortionAudioProcessor::releaseResources()
{
    // once playback stops, reset the DelayLine objects and set the previous sample values for the comb filters to 0 (silence)
    mEarlyReflectionDelayLine.reset();

    for (auto& delayLine : mCombDelayLines)
        delayLine.reset();

    mCombFilterStates.fill (0.0f);

    mRingModLfo.reset();

    for (auto& delayLine : mAllPassDelayLines)
        delayLine.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SchroeDistortionAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SchroeDistortionAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // before we begin processing, update all APVTS parameter values and assign them to our member variables.
    updateParameterValues();
    
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // initialize the monoInput sample value with silence
        float monoInput = 0.0f;

        // sum the input channels so the schroeder network is driven by a mono signal.  in a perfect world, we'd have a dedicated schroeder network for each input channel. this would add a lot more variety and space in the output, so its something you could expand on in the future!
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
            monoInput += buffer.getSample (channel, sample);

        // average the input so the mono level stays consistent across channel counts. just in case there are 0 input channels, use the juce::jmax function to choose whichever value is greater: 1 or the number of input channels.
        monoInput /= static_cast<float> (juce::jmax (1, totalNumInputChannels));

        // add a fixed cluster of ring-modulated early reflections before the late reverb network.
        const auto earlyReflectionSample = processRMEarlyReflections (monoInput);

        // initialize a variable to hold the running sum of our parallel comb filter output samples
        float combSum = 0.0f;
        
        // run the RM/early reflections sample through parallel comb filters and sum their delayed outputs. we'll use our processCombFilter() helper to do the actual comb processing
        for (size_t i = 0; i < mCombDelayLines.size(); ++i)
        {
            // since this is a parallel process, we accumulate the output of the comb filters into a runnning sum.
            combSum += processCombFilter (mCombDelayLines[i], mCombFilterStates[i], earlyReflectionSample, mCombDelayTimesSamples[i], juce::jlimit (0.0f, 0.99f, mCombFeedbackGains[i] * mDecay));
            // also note that we limit the comb feedback gains to 0.99 since the mDecay parameter can get up to 120%
        }

        // in between the parallel comb filtering and series all-pass filtering, apply soft clipping
        combSum = processSoftClip (combSum);

        // initialize a variable for the series all-pass filtering. we'll start it off as the soft-clipped comb filter sum.
        float outputSample = combSum;
        
        // feed the comb sum through series all-pass filters to increase echo density.
        for (size_t i = 0; i < mAllPassDelayLines.size(); ++i)
        {
            // since this process is series and not parallel, we don't accumulate the output of the all-pass filter helper into a running sum. instead, for each all-pass filter stage, we pass the outputSample value into the filter and overwrite that value with the filter output.
            outputSample = processAllPassFilter (mAllPassDelayLines[i], outputSample, mAllPassDelayTimesSamples[i], mAllPassGains[i]);
        }

        // scale the output sample of the schroeder process by our mOutputGain value to keep the signal in a good range.
        float wetSample = outputSample * mOutputGain;
        
        // mix the mono reverb output with the dry signal and write it to all output channels.
        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            const float drySample = channel < totalNumInputChannels ? buffer.getSample (channel, sample) : 0.0f;
            const float mixedSample = (drySample * (1.0f - mWetDryMix)) + (wetSample * mWetDryMix);
            buffer.setSample (channel, sample, mixedSample);
        }
    }
}

//==============================================================================
bool SchroeDistortionAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SchroeDistortionAudioProcessor::createEditor()
{
    return new SchroeDistortionAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout SchroeDistortionAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { wetDryId, 1},
        "Wet / Dry",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),
        50.0f,
        "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { decayId, 1},
        "Decay",
        juce::NormalisableRange<float> (0.0f, 120.0f, 1.0f),
        100.0f,
        "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { dampingId, 1}, // ERROR HERE - Damping parameter was using decayId
        "Damping",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),
        50.0f,
        "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { rmFreqId, 1},
        "RMFreq",
        juce::NormalisableRange<float> (minRingModFreqHz, maxRingModFreqHz, 0.1f),
        30.0f,
        "Hz"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { overdriveId, 1},
        "Overdrive",
        juce::NormalisableRange<float> (minOverdriveDb, maxOverdriveDb, 1.0f),
        24.0f,
        "dB"));

    return { params.begin(), params.end() };
}

void SchroeDistortionAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // serialize the APVTS state so the host can save the current parameter settings with the session. even though createXml() formats the data as XML, we'll write to a binary blob and not a human-readable XML file. this is the expectation for maintaining saved parameter data between a DAW and plugin.
    if (auto xml = mParameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SchroeDistortionAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // restore the APVTS state when the host reloads a preset or saved session. the mParameters.state.getType() call retrieves the ID that names our entire APVTS. we passed this in as the 3rd argument to the APVTS constructor: "Parameters." if the ID in the XML data matches our ID, then we load the data to mParameters (our APVTS instance).
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (mParameters.state.getType()))
            mParameters.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SchroeDistortionAudioProcessor();
}
