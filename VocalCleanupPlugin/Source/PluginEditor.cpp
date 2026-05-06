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

    addAndMakeVisible(gateVisualizer);

    setSize (400, 300);
}

VocalCleanupPluginAudioProcessorEditor::~VocalCleanupPluginAudioProcessorEditor()
{
    stopTimer();

    // CRITICAL: Must remove listeners to avoid crashes
}

//==============================================================================
void VocalCleanupPluginAudioProcessorEditor::paint (juce::Graphics& g)
{

    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void VocalCleanupPluginAudioProcessorEditor::resized()
{
    auto r = getLocalBounds();

    // small VAD box in the top-left
    VAD.setBounds(10, 10, 50, 50);

    // Give the GateVisualizer the remaining area
    gateVisualizer.setBounds(70, 10, r.getWidth() - 80, r.getHeight() - 20);
}

//==============================================================================
// Called when user moves the slider
void VocalCleanupPluginAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    
}

// Called when parameter changes (automation, preset load, etc.)
void VocalCleanupPluginAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
   
}

// Timer callback to poll voice detection state from processor
void VocalCleanupPluginAudioProcessorEditor::timerCallback()
{
    bool voiceDetected = audioProcessor.isVoiceDetected.load(std::memory_order_relaxed);
    VAD.setState(voiceDetected);
}


