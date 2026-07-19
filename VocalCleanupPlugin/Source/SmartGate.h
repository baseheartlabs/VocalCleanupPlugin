/*
  ==============================================================================

    SmartGate.h
    Created: 15 Jun 2026 8:51:28pm
    Author:  mbase

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class SmartGate : public juce::dsp::ProcessorBase
{
public:
    SmartGate() = default;

    //==========================================================================
	// ProcessorBase overrides:
    void prepare(const juce::dsp::ProcessSpec& spec) override; // Called when sample rate or block size changes
	void process(const juce::dsp::ProcessContextReplacing<float>& context) override; // Called for each audio block, must be real-time safe (no allocations, locks, or UI calls)
    void reset() override; // Called on playback stop/start

    //==========================================================================
    // Setters
    void setThresholdOffset(float offsetDb);
    void setHangoverTime(float ms);

    //==========================================================================
    // Accessors
    float getSignalDb() const;
    float getNoiseFloorDb() const;
    float getActiveThresholdDb() const;
    bool isOpen() const;

private:
    //==========================================================================
    // Sample rate dependent configs (do calculations in prepare/process, store here)
    double sampleRate = 44100.0;
    float dropCoef = 0.0f;
    float climbCoef = 0.0f;
    int hangoverDurationSamples = 0; 

    //==========================================================================
    // Configs
    float thresholdOffsetDb = 12.0f;
    float hangoverTimeMs = 100.0f;  // window to prevent chatter of gate closing immediately after opening, in ms
    float dropTimeSec = 0.05f;
    float climbTimeSec = 3.0f;

    //==========================================================================
    // EMA Vars
    float levelAlpha = 0.0f;          // Computed in prepare from levelTimeSec, smoothing factor (0-1), controls responsiveness
    float smoothedSquares = 0.0f;     // Holds previous value used to calculate RMS
    float levelTimeSec = 0.01f;       // Used along with fs to calculate levelAlpha- here aim for ~10ms averaging window

    //==========================================================================
    // Initialize state (updated per-sample in process)
    float currentNoiseFloorDb = -100.0f;
    float activeThresholdDb = -80.0f;
    float lastBlockDb = -100.0f;
    int hangoverCounter = 0;
    bool gateOpen = false;
};
