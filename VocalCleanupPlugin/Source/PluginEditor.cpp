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
    addAndMakeVisible(spectralVisualizer);

    setSize (400, 500);
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

    // Split the remaining area: gate view on top, spectrum analyzer below.
    auto contentX = 70;
    auto contentW = r.getWidth() - 80;
    auto topH = (r.getHeight() - 30) / 2;

    gateVisualizer.setBounds(contentX, 10, contentW, topH);
    spectralVisualizer.setBounds(contentX, 10 + topH + 10, contentW, r.getHeight() - topH - 30);
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


