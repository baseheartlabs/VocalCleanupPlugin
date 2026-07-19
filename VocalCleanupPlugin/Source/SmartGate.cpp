/*
  ==============================================================================

    SmartGate.cpp
    Created: 15 Jun 2026 8:51:28pm
    Author:  mbase

  ==============================================================================
*/

#include "SmartGate.h"

//==============================================================================
void SmartGate::prepare(const juce::dsp::ProcessSpec& spec)  // Called when host needs to reconfigure audio stream (playback starts, buffer size changes, Fs changes, plugin is added, bus changes, etc)
{
    sampleRate = spec.sampleRate;

    // Recalculate smoothing coefficients from time constants/sample Rate
    dropCoef = std::exp(-1.0 / (dropTimeSec * sampleRate));
    climbCoef = std::exp(-1.0 / (climbTimeSec * sampleRate));

    // Calculate alpha 
    levelAlpha = 1.0f - std::exp(-1.0f / (static_cast<float>(levelTimeSec * sampleRate)));

    // Recalculate hangover duration from ms
    hangoverDurationSamples = static_cast<int>(sampleRate * hangoverTimeMs / 1000.0);

    reset();
}

//==============================================================================
void SmartGate::process(const juce::dsp::ProcessContextReplacing<float>& context)
{
    auto& block = context.getOutputBlock();
    auto numSamples = static_cast<int>(block.getNumSamples());
    auto numChannels = static_cast<int>(block.getNumChannels());

    // Process each sample individually for sample-accurate gating
    for (int i = 0; i < numSamples; i++)
    {
        // Update smoothed squares (EMA) for RMS calculation
        float newSample = block.getSample(0, i);
        float squared = newSample * newSample;
        smoothedSquares += levelAlpha * (squared - smoothedSquares);

        // Compute current RMS in dB
        float rms = std::sqrt(smoothedSquares);
        float sampleDb = juce::Decibels::gainToDecibels(rms, -100.0f);

        // Update noise floor (asymmetric smoothing per sample)
        if (sampleDb < currentNoiseFloorDb)
            currentNoiseFloorDb = sampleDb + dropCoef * (currentNoiseFloorDb - sampleDb);
        else
            currentNoiseFloorDb = sampleDb + climbCoef * (currentNoiseFloorDb - sampleDb);

        // Compute active threshold
        activeThresholdDb = currentNoiseFloorDb + thresholdOffsetDb;

        // Gate decision with hangover
        if (sampleDb > activeThresholdDb)
        {
            gateOpen = true;
            hangoverCounter = hangoverDurationSamples;
        }
        else
        {
            if (hangoverCounter > 0)
                --hangoverCounter;
            else
                gateOpen = false;
        }

        // Apply gate per-sample across all channels
        if (!gateOpen)
        {
            for (int ch = 0; ch < numChannels; ++ch)
                block.setSample(ch, i, 0.0f);
        }
    }

    // Store final level for visualizer
    lastBlockDb = juce::Decibels::gainToDecibels(std::sqrt(smoothedSquares), -100.0f);
}

//==============================================================================
void SmartGate::reset()
{
    currentNoiseFloorDb = -100.0f;
    activeThresholdDb = -80.0f;
    lastBlockDb = -100.0f;
    hangoverCounter = 0;
    gateOpen = false;
    smoothedSquares = 0.0f;
}

//==============================================================================
// Setters
void SmartGate::setThresholdOffset(float offsetDb)
{
    thresholdOffsetDb = offsetDb;
}

void SmartGate::setHangoverTime(float ms)
{
    hangoverTimeMs = ms;
    hangoverDurationSamples = static_cast<int>(sampleRate * ms / 1000.0);
}

//==============================================================================
// Accessors for visualizer
float SmartGate::getSignalDb() const { return lastBlockDb; }
float SmartGate::getNoiseFloorDb() const { return currentNoiseFloorDb; }
float SmartGate::getActiveThresholdDb() const { return activeThresholdDb; }
bool SmartGate::isOpen() const { return gateOpen; }
