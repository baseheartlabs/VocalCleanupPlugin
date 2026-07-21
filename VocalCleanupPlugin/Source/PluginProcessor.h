/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SmartGate.h"

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
	float currentGain = 0.0f; // Current gain value, will be updated in processBlock

	// Parameter management
    juce::AudioProcessorValueTreeState parameterManager; // Manages parameters

	// Parameters not exposed for tuning 
	int hangOverDurationSamples = static_cast<int>(44100.0 * 0.1); // Duration of hangover in samples (e.g., 100ms at 44.1kHz = 4410 samples)

    std::atomic<bool> isVoiceDetected{ false }; // NOT input parameter - updated in processBlock based on RMS level and threshold comparison

    // Active Noise Floor Tracking
    float currentNoiseFloorDb = 0.0f; // Start very low
    float activeThresholdDb = -80.0f;   // The actual gate threshold
    float thresholdOffsetDb = 12.0f;    // How many dB above the noise floor the gate opens - could be exposed as "sensitivity"?
    float dropTimeSec = 0.05f; // Should drop to find noise quickly
    float climbTimeSec = 3.0f; // Should climb slowly to ignore speech (Asymmetric)
    float dropCoef = 0.5f; // Updated when sample rate changes
    float climbCoef = 0.25f; // Updated when sample rate changes

    // Visualizer
    // A struct to hold one "pixel column" of data
    struct VisualFrame {
        float signalDb;
        float noiseFloorDb;
        float activeThresholdDb;
        bool isGateOpen;
    };

    // Circular Buffer
    static constexpr int bufferSize = 2048; // Holds about ~10 seconds of data depending on block size
    std::array<VisualFrame, bufferSize> visualBuffer;
    std::atomic<int> writeIndex{ 0 };

    // Helper: Setup parameters
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Declare Modules
    SmartGate gate;

    // Cached pointers to read-only meter parameters (updated from processBlock)
    juce::AudioParameterFloat* meterSignalParam = nullptr;
    juce::AudioParameterFloat* meterNoiseFloorParam = nullptr;
    juce::AudioParameterFloat* meterThresholdParam = nullptr;

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalCleanupPluginAudioProcessor)
};

