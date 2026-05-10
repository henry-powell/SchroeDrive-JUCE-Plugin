/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class SchroeDistortionAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SchroeDistortionAudioProcessorEditor (SchroeDistortionAudioProcessor&);
    ~SchroeDistortionAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SchroeDistortionAudioProcessor& audioProcessor;
    
    // create sliders for all of our params
    juce::Slider mWetDrySlider;
    juce::Slider mDecaySlider;
    juce::Slider mDampingSlider;
    juce::Slider mRingModFreqSlider;
    juce::Slider mOverdriveSlider;
    
    // make labels for all of our sliders
    juce::Label mWetDryLabel;
    juce::Label mDecayLabel;
    juce::Label mDampingLabel;
    juce::Label mRingModFreqLabel;
    juce::Label mOverdriveLabel;

    // make an alias for the rather long SliderAttachment class name
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    
    void sliderValueChanged (juce::Slider* slider);
    
    // create SliderAttachment instances for each parameter
    std::unique_ptr<SliderAttachment> mWetDryAttachment;
    std::unique_ptr<SliderAttachment> mDecayAttachment;
    std::unique_ptr<SliderAttachment> mDampingAttachment;
    std::unique_ptr<SliderAttachment> mRingModAttachment;
    std::unique_ptr<SliderAttachment> mOverdriveAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SchroeDistortionAudioProcessorEditor)
};
