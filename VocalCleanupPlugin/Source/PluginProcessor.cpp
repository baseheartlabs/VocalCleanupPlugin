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
                       ),
#else
    :
#endif

parameterManager(*this, nullptr, juce::Identifier("VocalCleanup"), createParameterLayout())
{
}



VocalCleanupPluginAudioProcessor::~VocalCleanupPluginAudioProcessor()
{
}

#pragma endregion

juce::AudioProcessorValueTreeState::ParameterLayout VocalCleanupPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    return { params.begin(), params.end() };
}

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
    hangOverDurationSamples = static_cast<int>(sampleRate * 0.1);

    // TODO: Gate this to happen only on startup or when sampleRate changes
    dropCoef = std::exp(-1.0 / (dropTimeSec * sampleRate));
    climbCoef = std::exp(-1.0 / (climbTimeSec * sampleRate));

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
    juce::ScopedNoDenormals noDenormals;  // Prevents CPU spikes from tiny floating point numbers
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't contain input data.
    // Ex. If you have 2 inputs and 4 outputs, this silences outputs 3 and 4 to prevent garbage noise.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // CRITICAL: No memory allocation (new/malloc), no UI calls, and no locks in this method!

    // Get the current signal level of the block
    float blockRms = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    float blockDb = juce::Decibels::gainToDecibels(blockRms, -100.0f); // Floor at -100dB

    if (blockDb < currentNoiseFloorDb)
    {
        // Signal is QUIETER than the floor. Drop quickly to track it.
        // Formula: Output = Input + Coef * (OldOutput - Input)
        currentNoiseFloorDb = blockDb + dropCoef * (currentNoiseFloorDb - blockDb);
    }
    else
    {
        // Signal is LOUDER than the floor. Climb very slowly.
        // This prevents loud speech from pulling the noise floor up instantly.
        currentNoiseFloorDb = blockDb + climbCoef * (currentNoiseFloorDb - blockDb);
    }

    // Set the active operational threshold for your VAD
    activeThresholdDb = currentNoiseFloorDb + thresholdOffsetDb;

    // State Handling
    if (blockDb > activeThresholdDb) 
    {
        isVoiceDetected.store(true);
        hangOverDurationSamples = static_cast<int>(currentSampleRate * 0.1); // Reset hangover duration
    }
    else
    {
        if (hangOverDurationSamples > 0)
        {
            hangOverDurationSamples = std::max(0, hangOverDurationSamples - buffer.getNumSamples());
        }
        else
        {
            isVoiceDetected.store(false);
        }
    }

    // Live View Support
    // Grab the current write position
        int currentIndex = writeIndex.load();

    // Write the frame data
    visualBuffer[currentIndex] = {
        blockDb,
        currentNoiseFloorDb,
        activeThresholdDb,
        isVoiceDetected // true if signalDb > activeThresholdDb
    };

    // 3. Advance the index, wrapping back to 0 if it hits the end
    writeIndex.store((currentIndex + 1) % bufferSize);

	// Main Audio Processing Loop
    int numSamples = buffer.getNumSamples();
	auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = totalNumInputChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    // Iterate through every sample in the buffer
    for (int sample = 0; sample < numSamples; sample++)
    {
        // Pass through for now
        leftChannel[sample] = leftChannel[sample];

        rightChannel[sample] = rightChannel[sample];
    }

    if (!isVoiceDetected.load(std::memory_order_relaxed))
    {
        // Simple gate: mute when voice NOT detected
        buffer.clear();
    }

}

#pragma endregion

#pragma region Custom Helper Functions

#pragma endregion

#pragma region Plugin Editor

bool VocalCleanupPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* VocalCleanupPluginAudioProcessor::createEditor()
{
    return new VocalCleanupPluginAudioProcessorEditor (*this, parameterManager);
}

#pragma endregion

#pragma region State Information
void VocalCleanupPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameterManager.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VocalCleanupPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameterManager.state.getType()))
            parameterManager.replaceState(juce::ValueTree::fromXml(*xmlState));
}
#pragma endregion

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalCleanupPluginAudioProcessor();
}
