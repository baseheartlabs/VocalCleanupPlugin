/*
  ==============================================================================

    Ballistics.cpp
    Created: 6 Sep 2026 5:22:19pm
    Author:  mbase

  ==============================================================================
*/

#include "Ballistics.h"

//==============================================================================
void Ballistics::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;

    attackCoef = std::exp(-1.0f / (attackTimeSec * static_cast<float>(sampleRate)));
    releaseCoef = std::exp(-1.0f / (releaseTimeSec * static_cast<float>(sampleRate)));

    reset();
}

//==============================================================================
float Ballistics::process(float targetGainDb)
{
    if (targetGainDb < gainStateDb)
        gainStateDb = attackCoef * (gainStateDb - targetGainDb) + targetGainDb;
    else
        gainStateDb = releaseCoef * (gainStateDb - targetGainDb) + targetGainDb;

    return gainStateDb;
}

//==============================================================================
void Ballistics::reset()
{
    gainStateDb = 0.0f;
}

//==============================================================================
// Setters
void Ballistics::setAttackTime(float ms)
{
    attackTimeSec = ms / 1000.0f;
    attackCoef = std::exp(-1.0f / (attackTimeSec * static_cast<float>(sampleRate)));
}

void Ballistics::setReleaseTime(float ms)
{
    releaseTimeSec = ms / 1000.0f;
    releaseCoef = std::exp(-1.0f / (releaseTimeSec * static_cast<float>(sampleRate)));
}
