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

class VADComponent : public juce::Component
{
public:
    void setState(bool isVoiceDetected)
    {
        voiceDetected = isVoiceDetected;
        repaint();
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(voiceDetected ? juce::Colours::green : juce::Colours::red);
    }
private:
    bool voiceDetected = false;
};

class VocalCleanupPluginAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                               public juce::Slider::Listener,
                                               public juce::AudioProcessorValueTreeState::Listener,
                                               private juce::Timer
{
public:
    VocalCleanupPluginAudioProcessorEditor(VocalCleanupPluginAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~VocalCleanupPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    // MUST implement these when inheriting from the listeners
    void sliderValueChanged(juce::Slider* slider) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void timerCallback() override;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    VocalCleanupPluginAudioProcessor& audioProcessor;
	VADComponent VAD; // Custom component to visualize voice activity detection

    juce::AudioProcessorValueTreeState& valueTreeState;
    
    juce::Slider thresholdSlider;
    juce::Label thresholdLabel;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalCleanupPluginAudioProcessorEditor)
};

