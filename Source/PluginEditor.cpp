/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"

//==============================================================================
SchroeDistortionAudioProcessorEditor::SchroeDistortionAudioProcessorEditor (SchroeDistortionAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)


{
    
    getLookAndFeel().setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::cyan);
    getLookAndFeel().setColour (juce::Slider::thumbColourId, juce::Colours::cyan);
    getLookAndFeel().setColour (juce::Slider::textBoxTextColourId, juce::Colours::cyan);
    getLookAndFeel().setColour (juce::Label::textColourId, juce::Colours::cyan);
    
    // configure each Slider/Label combo. the ranges and initial values of each slider are managed automatically by the processor's APVTS, so setup is much more simple!
    mWetDryLabel.setText ("Dry / Wet", juce::dontSendNotification);
    addAndMakeVisible (mWetDryLabel);
    mWetDryLabel.attachToComponent (&mWetDrySlider, true);
    mWetDrySlider.setTextValueSuffix (" %");
    addAndMakeVisible (mWetDrySlider);

    mDecayLabel.setText ("Decay", juce::dontSendNotification);
    addAndMakeVisible (mDecayLabel);
    mDecayLabel.attachToComponent (&mDecaySlider, true);
    mDecaySlider.setTextValueSuffix (" %");
    addAndMakeVisible (mDecaySlider);

    mDampingLabel.setText ("Damping", juce::dontSendNotification);
    addAndMakeVisible (mDampingLabel);
    mDampingLabel.attachToComponent (&mDampingSlider, true);
    mDampingSlider.setTextValueSuffix (" %");
    addAndMakeVisible (mDampingSlider);

    mRingModFreqLabel.setText ("RMFreq", juce::dontSendNotification);
    addAndMakeVisible (mRingModFreqLabel);
    mRingModFreqLabel.attachToComponent (&mRingModFreqSlider, true);
    mRingModFreqSlider.setTextValueSuffix (" Hz");
    addAndMakeVisible (mRingModFreqSlider);

    mOverdriveLabel.setText ("Overdrive", juce::dontSendNotification);
    addAndMakeVisible (mOverdriveLabel);
    mOverdriveLabel.attachToComponent (&mOverdriveSlider, true);
    mOverdriveSlider.setTextValueSuffix (" dB");
    addAndMakeVisible (mOverdriveSlider);

    // create a SliderAttachment instance for each of our parameters. this will ensure that the Slider/ID combo that's passed in will be linked and automatically in sync.
    mWetDryAttachment = std::make_unique<SliderAttachment> (audioProcessor.mParameters, SchroeDistortionAudioProcessor::wetDryId, mWetDrySlider);
    mDecayAttachment = std::make_unique<SliderAttachment> (audioProcessor.mParameters, SchroeDistortionAudioProcessor::decayId, mDecaySlider);
    mDampingAttachment = std::make_unique<SliderAttachment> (audioProcessor.mParameters, SchroeDistortionAudioProcessor::dampingId, mDampingSlider);
    mRingModAttachment = std::make_unique<SliderAttachment> (audioProcessor.mParameters, SchroeDistortionAudioProcessor::rmFreqId, mRingModFreqSlider);
    mOverdriveAttachment = std::make_unique<SliderAttachment> (audioProcessor.mParameters, SchroeDistortionAudioProcessor::overdriveId, mOverdriveSlider);
    
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (480, 350);
}

SchroeDistortionAudioProcessorEditor::~SchroeDistortionAudioProcessorEditor()
{
}

//==============================================================================
void SchroeDistortionAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::cyan);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("SchroeDrive", getLocalBounds().removeFromTop (60), juce::Justification::centred, 1);
}

void SchroeDistortionAudioProcessorEditor::resized()
{
    // to keep things simple, we can just use hard-coded positions and sizes for the sliders
    mWetDrySlider.setBounds (140, 80, 300, 24);
    mDecaySlider.setBounds (140, 120, 300, 24);
    mDampingSlider.setBounds (140, 160, 300, 24);
    mRingModFreqSlider.setBounds (140, 220, 300, 24);
    mOverdriveSlider.setBounds (140, 260, 300, 24);
}
