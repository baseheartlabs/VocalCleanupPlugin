#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/**
 * VocalCleanupPlugin – AudioProcessor
 *
 * Processing chain (in order):
 *   1. High-pass filter  – removes low-end rumble / mic handling noise
 *   2. Noise gate        – silences quiet passages between vocal phrases
 *   3. Compressor        – tames peak dynamics
 *   4. De-esser          – reduces harsh sibilance ("s", "sh" sounds)
 *   5. Output gain       – final level trim
 */
class VocalCleanupAudioProcessor : public juce::AudioProcessor
{
public:
    //==========================================================================
    VocalCleanupAudioProcessor();
    ~VocalCleanupAudioProcessor() override = default;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==========================================================================
    int  getNumPrograms()    override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    /** The APVTS holds all automatable plugin parameters. */
    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    //==========================================================================
    using Filter = juce::dsp::IIR::Filter<float>;

    // Main processing chain per channel
    using MonoChain = juce::dsp::ProcessorChain<
                          Filter,                         // 0 – high-pass filter
                          juce::dsp::NoiseGate<float>,    // 1 – noise gate
                          juce::dsp::Compressor<float>    // 2 – compressor
                      >;

    MonoChain leftChain, rightChain;

    enum ChainIndex { HighPass = 0, Gate, Comp };

    // De-esser: separate bandpass filter used as sidechain detector,
    // plus a peak-cut filter applied to the main signal when sibilance is detected.
    Filter deEsserDetectL, deEsserDetectR;  // bandpass around sibilance frequency
    Filter deEsserCutL,    deEsserCutR;     // peak-cut applied to main signal

    // Envelope follower state for de-esser detector (one per channel)
    float deEsserEnvL { 0.0f }, deEsserEnvR { 0.0f };

    // Output gain applied after de-esser for correct signal order
    juce::dsp::Gain<float> outputGainL, outputGainR;

    double currentSampleRate { 44100.0 };

    void updateAllParameters();
    void updateHighPass();
    void updateGate();
    void updateCompressor();
    void updateDeEsser();
    void updateOutputGain();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalCleanupAudioProcessor)
};
