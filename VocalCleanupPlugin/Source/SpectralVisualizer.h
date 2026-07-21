/*
  ==============================================================================

    SpectralVisualizer.h
    Created: 20 Jul 2026

    Draws the real-time magnitude spectrum produced by SpectralAnalyzer.
    Analysis-only: reads scope data on the UI thread and paints it.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class SpectralVisualizer : public juce::Component, private juce::Timer
{
public:
    SpectralVisualizer(VocalCleanupPluginAudioProcessor& p);
    ~SpectralVisualizer() override;

    void paint(juce::Graphics& g) override;
    void timerCallback() override;

private:
    VocalCleanupPluginAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralVisualizer)
};
