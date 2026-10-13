/*
  ==============================================================================

    GainProcessors.cpp
    Created: 15 Jun 2026 8:51:28pm
    Author:  mbase

  ==============================================================================
*/

#include "GainProcessors.h"

//==============================================================================
void GainProcessors::prepare(const juce::dsp::ProcessSpec& spec)  // Called when host needs to reconfigure audio stream (playback starts, buffer size changes, Fs changes, plugin is added, bus changes, etc)
{
    sampleRate = spec.sampleRate;

    // Recalculate smoothing coefficients from time constants/sample Rate
    tauDrop = std::exp(-1.0 / (dropTimeSec * sampleRate));
    tauClimb = std::exp(-1.0 / (climbTimeSec * sampleRate));

    // Calculate alpha 
    levelAlpha = 1.0f - std::exp(-1.0f / (static_cast<float>(levelTimeSec * sampleRate)));

    // Recalculate hangover duration from ms
    hangoverDurationSamples = static_cast<int>(sampleRate * hangoverTimeMs / 1000.0);

    ballisticsModule.prepare(sampleRate);

    reset();
}

//==============================================================================
void GainProcessors::process(const juce::dsp::ProcessContextReplacing<float>& context)
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

        // Target output level (dB) from the transfer function; convert the
        // level change into a linear gain and apply it to every channel.
        float outputDb = computeOutputDb(sampleDb, currentMode, thresholdDb, ratio);
        float targetGainDb = outputDb - sampleDb;

        targetGainDb = ballisticsModule.process(targetGainDb);

        // Safety 1: bound the gain change so the curve can't boost without limit
        float gainDb = juce::jlimit(maxCutDb, maxBoostDb, targetGainDb);
        float gainLinear = juce::Decibels::decibelsToGain(gainDb);
        lastGainDb = gainDb;
        

        // Safety 3: neutralize any non-finite gain before it reaches the audio
        if (! std::isfinite(gainLinear))
            gainLinear = 1.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            // Safety 2: brick-wall the output so it can never exceed the ceiling
            float out = block.getSample(ch, i) * gainLinear;
            block.setSample(ch, i, juce::jlimit(-outputCeiling, outputCeiling, out));
        }
    }

    // Store final level for visualizer
    lastBlockDb = juce::Decibels::gainToDecibels(std::sqrt(smoothedSquares), -100.0f);
}

//==============================================================================
void GainProcessors::reset()
{
    currentNoiseFloorDb = -100.0f;
    activeThresholdDb = -80.0f;
    lastBlockDb = -100.0f;
    hangoverCounter = 0;
    gateOpen = false;
    smoothedSquares = 0.0f;
    ratio = 2.0f;
    thresholdDb = -80.0f;
    ballisticsModule.reset();
}

//==============================================================================
// Setters
void GainProcessors::setThresholdOffset(float offsetDb)
{
    thresholdOffsetDb = offsetDb;
}

void GainProcessors::setHangoverTime(float ms)
{
    hangoverTimeMs = ms;
    hangoverDurationSamples = static_cast<int>(sampleRate * ms / 1000.0);
}

void GainProcessors::setMode(int modeIndex)
{
    currentMode = modeIndex;
}

void GainProcessors::setThreshold(float newThresholdDb)
{
    thresholdDb = newThresholdDb;
}

void GainProcessors::setRatio(float newRatio)
{
    ratio = newRatio;
}

void GainProcessors::setAttackTime(float ms)
{
    ballisticsModule.setAttackTime(ms);
}

void GainProcessors::setReleaseTime(float ms)
{
    ballisticsModule.setReleaseTime(ms);
}

//==============================================================================
// Static transfer function shared by process() and the GUI curve
float GainProcessors::computeOutputDb(float inputDb, int mode, float thresholdDb, float ratio)
{
    float outputGain = inputDb;
    float overshoot = inputDb - thresholdDb;

    switch (mode)
    {
        case 0: // Limiter
            if (inputDb > thresholdDb)
                outputGain = thresholdDb;
            break;
        case 1: // Gate
            if (inputDb < thresholdDb)
                outputGain = -100.0f; // effectively mute the signal below the gate threshold
            break;
        case 2: // UpwardCompressor
            if (inputDb < thresholdDb)
                outputGain = thresholdDb + (overshoot / ratio);
            break;
        case 3: // DownwardCompressor
            if (inputDb > thresholdDb)
                outputGain = thresholdDb + (overshoot / ratio);
            break;
        case 4: // UpwardExpander
            if (inputDb > thresholdDb)
                outputGain = thresholdDb + (overshoot * ratio);
            break;
        case 5: // DownwardExpander
            if (inputDb < thresholdDb)
                outputGain = thresholdDb + (overshoot * ratio);
            break;
        default:
            break;
    }

    return outputGain;
}

//==============================================================================
// Accessors for visualizer
float GainProcessors::getSignalDb() const { return lastBlockDb; }
float GainProcessors::getNoiseFloorDb() const { return currentNoiseFloorDb; }
float GainProcessors::getActiveThresholdDb() const { return activeThresholdDb; }
float GainProcessors::getRatio() const { return ratio; }
float GainProcessors::getGainDb() const { return lastGainDb; }
bool GainProcessors::isOpen() const { return gateOpen; }
