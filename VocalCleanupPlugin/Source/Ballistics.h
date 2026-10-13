/*
  ==============================================================================

    Ballistics.h
    Created: 6 Sep 2026 5:22:19pm
    Author:  mbase

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

// Compressor ballistics: smooths a target gain (dB) toward attack/release
class Ballistics
{
public:
    Ballistics() = default;

    //==========================================================================
    void prepare(double sampleRate); // Recompute attackCoef/releaseCoef here
    void reset();                    // Reset gainStateDb to 0 (unity)

    // Feed one target gain (dB) per sample from the gain computer;
    // returns the ballistics-smoothed gain (dB) to apply to the signal.
    float process(float targetGainDb);

    //==========================================================================
    // Setters
    void setAttackTime(float tauMs);
    void setReleaseTime(float tauMs);

private:
    //==========================================================================
    // Sample rate dependent configs (recompute in prepare())
    double sampleRate = 44100.0;
    float attackCoef = 0.0f;
    float releaseCoef = 0.0f;

    //==========================================================================
    // Configs
    float attackTimeSec = 0.01f;  
    float releaseTimeSec = 0.15f;

    //==========================================================================
    // State (updated per-sample in process)
    float gainStateDb = 0.0f; // 0 dB = unity, the smoothed gain being applied
};
