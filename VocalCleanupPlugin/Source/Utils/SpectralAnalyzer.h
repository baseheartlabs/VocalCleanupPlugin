/*
  ==============================================================================

    SpectralAnalyzer.h
    Created: 20 Jul 2026

    Analysis-only real-time spectrum analyzer built on juce::dsp::FFT, following
    the JUCE "Visualising the frequencies of a signal in real time" tutorial
    pattern. This class does NOT modify audio - it only observes samples pushed
    from the audio thread and produces a magnitude spectrum for the UI to draw.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class SpectralAnalyzer
{
public:
    // fftOrder 11 => fftSize 2048 bins (~21.5 Hz/bin at 44.1 kHz).
    // scopeSize is the number of points drawn across the display width.
    enum
    {
        fftOrder  = 11,
        fftSize   = 1 << fftOrder,   // 2048
        scopeSize = 512
    };

    SpectralAnalyzer();

    //==========================================================================
    // Called from the AUDIO thread (real-time safe: no allocations/locks).
    // Push one sample at a time; when a full frame is collected the analyzer
    // flags that a new FFT block is ready for the UI to consume.
    void pushSample(float sample) noexcept;

    //==========================================================================
    // Called from the UI thread (e.g. a Timer). If a new block is ready, this
    // windows the frame, runs the forward FFT, converts to a dB scope, and
    // clears the ready flag. Returns true if the scope was updated.
    bool processIfBlockReady();

    //==========================================================================
    // UI thread: read the most recent scope data (scopeSize normalised values
    // in 0..1, already mapped to a log frequency / dB curve for drawing).
    const std::array<float, scopeSize>& getScopeData() const noexcept { return scopeData; }

private:
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;

    // Audio-thread fill buffer + write position
    std::array<float, fftSize> fifo {};
    int fifoIndex = 0;

    // FFT working buffer (real input in first half, FFT writes magnitudes here)
    std::array<float, fftSize * 2> fftData {};

    // Shared flag between audio thread (producer) and UI thread (consumer)
    std::atomic<bool> nextBlockReady { false };

    // UI-thread scope output
    std::array<float, scopeSize> scopeData {};

    void drawNextFrameOfSpectrum();

    JUCE_LEAK_DETECTOR(SpectralAnalyzer)
};
