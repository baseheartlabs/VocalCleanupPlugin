/*
  ==============================================================================

    GateVisualizer.h
    Created: 5 May 2026 7:25:18pm
    Author:  mbase

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h" // Needed to access the processor's data
#include "PluginProcessor.h"

class GateVisualizer : public juce::Component, private juce::Timer
{
public:
    GateVisualizer(VocalCleanupPluginAudioProcessor& p);
    ~GateVisualizer() override;

    void paint(juce::Graphics& g) override;
    void timerCallback() override;

private:
    VocalCleanupPluginAudioProcessor& processor; // Store a reference
    std::vector<VocalCleanupPluginAudioProcessor::VisualFrame> displayHistory;
    int readIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GateVisualizer)
};
