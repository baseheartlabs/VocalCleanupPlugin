/*
  ==============================================================================

    GainProcessors.h
    Created: 15 Jun 2026 8:51:28pm
    Author:  mbase

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "Ballistics.h"

class GainProcessors : public juce::dsp::ProcessorBase
{
public:
    GainProcessors() = default;

    //==========================================================================
	// ProcessorBase overrides:
    void prepare(const juce::dsp::ProcessSpec& spec) override; // Called when sample rate or block size changes
	void process(const juce::dsp::ProcessContextReplacing<float>& context) override; // Called for each audio block, must be real-time safe (no allocations, locks, or UI calls)
    void reset() override; // Called on playback stop/start

    //==========================================================================
    // Setters
    void setThresholdOffset(float offsetDb);
    void setHangoverTime(float ms);
    void setMode(int modeIndex); // called from processBlock, real-time safe
    void setThreshold(float thresholdDb); // user threshold for dynamics, called from processBlock
    void setRatio(float ratio); // user-set compression/expansion ratio for dynamics processors
    void setAttackTime(float ms);
    void setReleaseTime(float ms);

    //==========================================================================
    // Accessors
    float getSignalDb() const;
    float getNoiseFloorDb() const;
    float getActiveThresholdDb() const;
    float getRatio() const;
    float getGainDb() const;
    bool isOpen() const;

    //==========================================================================
    // Static transfer function: maps an input level (dB) to an output level (dB)
    // for the given mode/threshold/ratio. Shared by process() and the GUI curve.
    static float computeOutputDb(float inputDb, int mode, float thresholdDb, float ratio);

private:
    //==========================================================================
    // Sample rate dependent configs (do calculations in prepare/process, store here)
    double sampleRate = 44100.0;
    float tauDrop = 0.0f;
    float tauClimb = 0.0f;
    int hangoverDurationSamples = 0; 

    //==========================================================================
    // Configs
    int currentMode = 0; // index into the plugin state choice parameter
    float thresholdDb = -18.0f; // user-set threshold for the dynamics processors
    float ratio = 2.0f; // user-set compression/expansion ratio for dynamics processors

    //==========================================================================
    // Safety limits
    float maxBoostDb = 12.0f;      // ceiling on how much gain the transfer curve may add
    float maxCutDb = -60.0f;       // floor on gain change (below this is effectively silence)
    float outputCeiling = 0.98f;   // brick-wall limit on the output sample magnitude

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
    float lastGainDb = 0.0f;
    int hangoverCounter = 0;
    bool gateOpen = false;

    //==========================================================================
    // Attack/release smoothing applied to the gain-computer output
    Ballistics ballisticsModule;
};
