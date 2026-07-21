/*
  ==============================================================================

    SpectralAnalyzer.cpp
    Created: 20 Jul 2026

  ==============================================================================
*/

#include "SpectralAnalyzer.h"

//==============================================================================
SpectralAnalyzer::SpectralAnalyzer()
    : forwardFFT(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
}

//==============================================================================
void SpectralAnalyzer::pushSample(float sample) noexcept
{
    // When the fifo fills up, hand the frame over to the FFT buffer and flag it.
    if (fifoIndex == fftSize)
    {
        if (!nextBlockReady.load())
        {
            // Zero the working buffer and copy the collected frame into it.
            std::fill(fftData.begin(), fftData.end(), 0.0f);
            std::copy(fifo.begin(), fifo.end(), fftData.begin());
            nextBlockReady.store(true);
        }

        fifoIndex = 0;
    }

    fifo[(size_t)fifoIndex++] = sample;
}

//==============================================================================
bool SpectralAnalyzer::processIfBlockReady()
{
    if (!nextBlockReady.load())
        return false;

    drawNextFrameOfSpectrum();
    nextBlockReady.store(false);
    return true;
}

//==============================================================================
void SpectralAnalyzer::drawNextFrameOfSpectrum()
{
    // 1. Apply the window to reduce spectral leakage.
    window.multiplyWithWindowingTable(fftData.data(), fftSize);

    // 2. Render the frequency spectrum (magnitudes) in-place into fftData.
    forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());

    // 3. Map the linear FFT bins onto a log-frequency scope in dB.
    static constexpr float mindB = -100.0f;
    static constexpr float maxdB = 0.0f;

    for (int i = 0; i < scopeSize; ++i)
    {
        // Log-skewed x position so low/mid vocal frequencies get more width.
        auto skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / (float)scopeSize) * 0.2f);
        auto fftDataIndex = juce::jlimit(0, fftSize / 2,
                                         (int)(skewedProportionX * (float)fftSize * 0.5f));

        // Normalise magnitude to dB (FFT sum ~ fftSize) and map to 0..1.
        auto level = juce::jmap(juce::jlimit(mindB, maxdB,
                                             juce::Decibels::gainToDecibels(fftData[(size_t)fftDataIndex])
                                                 - juce::Decibels::gainToDecibels((float)fftSize)),
                                mindB, maxdB, 0.0f, 1.0f);

        scopeData[(size_t)i] = level;
    }
}
