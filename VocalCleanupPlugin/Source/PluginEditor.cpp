/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VocalCleanupPluginAudioProcessorEditor::VocalCleanupPluginAudioProcessorEditor (
    VocalCleanupPluginAudioProcessor& p, 
    juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p), valueTreeState (vts)
{
    addAndMakeVisible(VAD);
    startTimerHz(30); // Update display at 30 FPS

    // Configure slider
    thresholdSlider.setSliderStyle(juce::Slider::LinearVertical);
    thresholdSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    thresholdSlider.setRange(-60.0, 0.0, 0.1);
    
    // Set initial value from parameter (before adding listener!)
    thresholdSlider.setValue(*valueTreeState.getRawParameterValue("threshold"), 
                             juce::dontSendNotification);
    
    addAndMakeVisible(thresholdSlider);

    // Add label
    thresholdLabel.setText("Threshold", juce::dontSendNotification);
    thresholdLabel.setJustificationType(juce::Justification::centred);
    thresholdLabel.attachToComponent(&thresholdSlider, false);
    addAndMakeVisible(thresholdLabel);
    
    // Register listeners AFTER setting initial value
    thresholdSlider.addListener(this);
    valueTreeState.addParameterListener("threshold", this);

    setSize (400, 300);
}

VocalCleanupPluginAudioProcessorEditor::~VocalCleanupPluginAudioProcessorEditor()
{
    stopTimer();

    // CRITICAL: Must remove listeners to avoid crashes
    thresholdSlider.removeListener(this);
    valueTreeState.removeParameterListener("threshold", this);
}

//==============================================================================
void VocalCleanupPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void VocalCleanupPluginAudioProcessorEditor::resized()
{
    thresholdSlider.setBounds(150, 50, 100, 200);
    VAD.setBounds(50, 50, 50, 50);
}

//==============================================================================
// Called when user moves the slider
void VocalCleanupPluginAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &thresholdSlider)
    {
        // Get the parameter
        auto* param = valueTreeState.getParameter("threshold");
        
        if (param != nullptr)
        {
            // Convert slider value to normalized [0.0, 1.0] range
            float normalizedValue = valueTreeState.getParameterRange("threshold")
                                        .convertTo0to1(static_cast<float>(slider->getValue()));
            
            // Update the parameter (notifies host and processor)
            param->setValueNotifyingHost(normalizedValue);
        }
    }
}

// Called when parameter changes (automation, preset load, etc.)
void VocalCleanupPluginAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "threshold")
    {
        // This callback can happen on the audio thread!
        // Use MessageManager to safely update the UI
        juce::MessageManager::callAsync([this, newValue]()
        {
            // Update slider without triggering sliderValueChanged
            thresholdSlider.setValue(newValue, juce::dontSendNotification);
        });
    }
}

// Timer callback to poll voice detection state from processor
void VocalCleanupPluginAudioProcessorEditor::timerCallback()
{
    bool voiceDetected = audioProcessor.isVoiceDetected.load(std::memory_order_relaxed);
    VAD.setState(voiceDetected);
}


