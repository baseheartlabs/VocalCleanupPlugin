/*
  ==============================================================================

    SpectralVisualizer.cpp
    Created: 20 Jul 2026

  ==============================================================================
*/

#include "SpectralVisualizer.h"

//==============================================================================
SpectralVisualizer::SpectralVisualizer(VocalCleanupPluginAudioProcessor& p) : processor(p)
{
    startTimerHz(30); // Redraw at 30 FPS
}

SpectralVisualizer::~SpectralVisualizer()
{
    stopTimer();
}

//==============================================================================
void SpectralVisualizer::timerCallback()
{
    // Pull a fresh FFT frame (if one is ready) and repaint only when updated.
    if (processor.spectralAnalyzer.processIfBlockReady())
        repaint();
}

//==============================================================================
void SpectralVisualizer::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0a0a0a));

    auto bounds = getLocalBounds().toFloat();
    auto width  = bounds.getWidth();
    auto height = bounds.getHeight();

    const auto& scope = processor.spectralAnalyzer.getScopeData();
    const int scopeSize = (int)scope.size();

    // Build a filled path of the spectrum curve.
    juce::Path spectrumPath;
    spectrumPath.startNewSubPath(0.0f, height);

    for (int i = 0; i < scopeSize; ++i)
    {
        auto x = juce::jmap((float)i, 0.0f, (float)(scopeSize - 1), 0.0f, width);
        auto y = juce::jmap(scope[(size_t)i], 0.0f, 1.0f, height, 0.0f);
        spectrumPath.lineTo(x, y);
    }

    spectrumPath.lineTo(width, height);
    spectrumPath.closeSubPath();

    // Fill under the curve, then stroke the outline.
    g.setColour(juce::Colours::aqua.withAlpha(0.25f));
    g.fillPath(spectrumPath);

    g.setColour(juce::Colours::aqua);
    g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));
}
