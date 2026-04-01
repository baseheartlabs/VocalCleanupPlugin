/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#pragma region Constructor/Destructor
VocalCleanupPluginAudioProcessor::VocalCleanupPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

VocalCleanupPluginAudioProcessor::~VocalCleanupPluginAudioProcessor()
{
}

#pragma endregion

#pragma region JUCE Boilerplate Implementation
const juce::String VocalCleanupPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VocalCleanupPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VocalCleanupPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VocalCleanupPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VocalCleanupPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VocalCleanupPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int VocalCleanupPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VocalCleanupPluginAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String VocalCleanupPluginAudioProcessor::getProgramName (int index)
{
    return {};
}

void VocalCleanupPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}
#pragma endregion

#pragma region Main Processing Methods

void VocalCleanupPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	currentSampleRate = sampleRate;

    // Reset state
    currentGain = floorGain;

    // Pre-calculate coefficients
    // Reference: 1-3ms attack prevents clipping consonants
	// 100-300ms release allows natural decay of sustained notes
    attackCoeff = calculateCoefficient(2.0f, sampleRate); // 3ms attack
	releaseCoeff = calculateCoefficient(150.0f, sampleRate); // 200ms release
}

void VocalCleanupPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VocalCleanupPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void VocalCleanupPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // CRITICAL: No memory allocation (new/malloc), no UI calls, and no locks in this method!

    juce::ScopedNoDenormals noDenormals;  // Prevents CPU spikes from tiny floating point numbers
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't contain input data.
    // Ex. If you have 2 inputs and 4 outputs, this silences outputs 3 and 4 to prevent garbage noise.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

	// Main Audio Processing Loop
    
    int numSamples = buffer.getNumSamples();
	auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = totalNumInputChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    // Iterate through every sample in the buffer
    for (int sample = 0; sample < numSamples; sample++)
    {
        //Take absolute value of left channel (or right, or mixed- can be changed)
        float inputLevel = std::abs(leftChannel[sample]);

        // Full gain above threshold, floor gain below
		float targetGain = (inputLevel > thresholdLinear) ? 1.0f : floorGain; 

        // Apply One-Pole Filter
        // If target is higher than current, use Attack. If lower, use Release.
        if (targetGain > currentGain)
        {
            currentGain = (attackCoeff * currentGain) + ((1.0f - attackCoeff) * targetGain);
        }
        else
        {
            currentGain = (releaseCoeff * currentGain) + ((1.0f - releaseCoeff) * targetGain);
        }

        // Apply the smoothed gain to the actual audio
        leftChannel[sample] *= currentGain;
        if (rightChannel != nullptr)
        {
            rightChannel[sample] *= currentGain;
        }
    }

}

#pragma endregion

#pragma region Custom Helper Functions
float VocalCleanupPluginAudioProcessor::calculateCoefficient(float timeInMs, double sampleRate)
{
    if(timeInMs <= 0.0f) return 0.0f; // Avoid division by zero

    // Convert time from milliseconds to seconds
    float timeInSeconds = timeInMs / 1000.0f;

    // Calculate the coefficient using the formula: coeff = exp(-1 / (time * sampleRate))
    float coeff = std::exp(-1.0f / (timeInSeconds * static_cast<float>(sampleRate)));

    return coeff;
}

#pragma endregion

#pragma region Plugin Editor

bool VocalCleanupPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* VocalCleanupPluginAudioProcessor::createEditor()
{
    return new VocalCleanupPluginAudioProcessorEditor (*this);
}

#pragma endregion

#pragma region State Information
void VocalCleanupPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void VocalCleanupPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

#pragma endregion

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalCleanupPluginAudioProcessor();
}
