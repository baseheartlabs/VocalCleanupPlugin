/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class VocalCleanupPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    VocalCleanupPluginAudioProcessor();
    ~VocalCleanupPluginAudioProcessor() override;

    #pragma region Main processing methods
	// Main processing methods:
    
    // Called once before playback starts: Filters/envelopes initialized here
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    
    // Called when playback stops: Clear out memory/buffers
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    // Processing Loop
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    #pragma endregion

    #pragma region JUCE overrides
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    #pragma endregion

    // Declare member variables here
	double currentSampleRate = 44100.0; // Default sample rate, will be updated in prepareToPlay

	float currentGain = 0.0f; // Current gain value, will be updated in processBlock

	// Parameter management
    juce::AudioProcessorValueTreeState parameterManager; // Manages parameters

    std::atomic<float>* thresholdDB = nullptr; // Pointer to threshold parameter (in dB)
    float thresholdLinear = 0.01f; // Threshold for vocal cleanup (linear scale, ~-40dB)

	// Parameters not exposed for tuning 
	int hangOverDurationSamples = static_cast<int>(44100.0 * 0.1); // Duration of hangover in samples (e.g., 100ms at 44.1kHz = 4410 samples)

    std::atomic<bool> isVoiceDetected{ false }; // NOT input parameter - updated in processBlock based on RMS level and threshold comparison

    // Helper: Setup parameters
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalCleanupPluginAudioProcessor)
};
