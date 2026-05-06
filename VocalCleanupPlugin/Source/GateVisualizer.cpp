/*
  ==============================================================================

    GateVisualizer.cpp
    Created: 5 May 2026 7:25:18pm
    Author:  mbase

  ==============================================================================
*/

#include "GateVisualizer.h"


//==============================================================================
GateVisualizer::GateVisualizer(VocalCleanupPluginAudioProcessor& p) : processor(p)
{
    // Pre-allocate the history array and fill it with "silence" 
    // so the graph draws immediately without waiting for the audio thread.
    displayHistory.resize(800, { -100.0f, -100.0f, -100.0f, false });

    // Update the screen 60 times per second
    startTimerHz(60);
}

GateVisualizer::~GateVisualizer()
{
    // Always stop timers in the destructor to prevent crashes on plugin close
    stopTimer();
}

//==============================================================================
void GateVisualizer::timerCallback()
{
    // 1. Get the current write index from the processor safely
    int currentWriteIndex = processor.writeIndex.load();

    // 2. Pull new frames from the processor into our local history
    // We only loop if there is new data (readIndex != currentWriteIndex)
    while (readIndex != currentWriteIndex)
    {
        // Remove the oldest frame at the front
        if (!displayHistory.empty()) {
            displayHistory.erase(displayHistory.begin());
        }

        // Add the newest frame to the back
        displayHistory.push_back(processor.visualBuffer[readIndex]);

        // Advance the read index, wrapping around the circular buffer
        readIndex = (readIndex + 1) % processor.bufferSize;
    }

    // 3. Trigger a redraw of the component
    repaint();
}

//==============================================================================
void GateVisualizer::paint(juce::Graphics& g)
{
    // 1. Draw a dark background
    g.fillAll(juce::Colour(0xff111111));

    if (displayHistory.empty()) return;

    auto bounds = getLocalBounds().toFloat();
    float width = bounds.getWidth();
    float height = bounds.getHeight();

    // 2. Define the visual scale (adjust minDb if you want to see further down)
    const float minDb = -80.0f;
    const float maxDb = 0.0f;

    juce::Path signalPath;
    juce::Path thresholdPath;
    juce::Path noiseFloorPath;

    // 3. Build the paths point-by-point
    for (size_t i = 0; i < displayHistory.size(); ++i)
    {
        // Calculate X position (maps vector index to component width)
        float x = juce::jmap((float)i, 0.0f, (float)(displayHistory.size() - 1), 0.0f, width);

        // Calculate Y positions (maps dB to component height, inverted since Y=0 is the top)
        float ySignal = juce::jmap(displayHistory[i].signalDb, minDb, maxDb, height, 0.0f);
        float yThreshold = juce::jmap(displayHistory[i].activeThresholdDb, minDb, maxDb, height, 0.0f);
        float yNoise = juce::jmap(displayHistory[i].noiseFloorDb, minDb, maxDb, height, 0.0f);

        // Clamp Y values to the screen so extreme signals don't draw outside the bounds
        ySignal = juce::jlimit(0.0f, height, ySignal);
        yThreshold = juce::jlimit(0.0f, height, yThreshold);
        yNoise = juce::jlimit(0.0f, height, yNoise);

        if (i == 0)
        {
            signalPath.startNewSubPath(x, ySignal);
            thresholdPath.startNewSubPath(x, yThreshold);
            noiseFloorPath.startNewSubPath(x, yNoise);
        }
        else
        {
            signalPath.lineTo(x, ySignal);
            thresholdPath.lineTo(x, yThreshold);
            noiseFloorPath.lineTo(x, yNoise);
        }

        // Draw a subtle vertical green highlight when the gate is OPEN
        if (displayHistory[i].isGateOpen)
        {
            g.setColour(juce::Colours::limegreen.withAlpha(0.15f));
            g.drawLine(x, height, x, 0.0f);
        }
    }

    // 4. Render the paths to the screen with specific colors and thicknesses

    // Noise Floor (Faint grey line)
    g.setColour(juce::Colours::grey.withAlpha(0.5f));
    g.strokePath(noiseFloorPath, juce::PathStrokeType(1.0f));

    // Active Threshold (Red line)
    g.setColour(juce::Colours::red.withAlpha(0.8f));
    g.strokePath(thresholdPath, juce::PathStrokeType(2.0f));

    // Actual Audio Signal (Crisp white line)
    g.setColour(juce::Colours::white);
    g.strokePath(signalPath, juce::PathStrokeType(1.5f));
}