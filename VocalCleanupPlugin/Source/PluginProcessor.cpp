/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GainProcessors.h"
#include "Ballistics.h"


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
    stateParam = parameterManager.getRawParameterValue(stateParamID);
    thresholdParam = parameterManager.getRawParameterValue(thresholdParamID);
    ratioParam = parameterManager.getRawParameterValue(ratioParamID);
    attackParam = parameterManager.getRawParameterValue(attackParamID);
    releaseParam = parameterManager.getRawParameterValue(releaseParamID);
}



VocalCleanupPluginAudioProcessor::~VocalCleanupPluginAudioProcessor()
{
}

#pragma endregion

juce::AudioProcessorValueTreeState::ParameterLayout VocalCleanupPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Enum state parameter: current operating state of the plugin (6 selectable states)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ stateParamID, 1 },
        "Plugin State",
        juce::StringArray{ "Limiter", "Gate", "UpwardCompressor", "DownwardCompressor", "UpwardExpander", "DownwardExpander" },
        static_cast<int>(PluginState::Limiter)));

    // Threshold (dB): common threshold for all dynamics processors
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ thresholdParamID, 1 },
        "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -18.0f));

    // Ratio: common ratio for all dynamics processors
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ratioParamID, 1 },
        "Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f),
        2.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ attackParamID, 1 },
        "Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.3f),
        10.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ releaseParamID, 1 },
        "Release",
        juce::NormalisableRange<float>(5.0f, 1000.0f, 1.0f, 0.35f),
        150.0f));

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
	juce::dsp::ProcessSpec spec;
	spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
	spec.numChannels = getTotalNumOutputChannels();

    // Prepare each module
	gainProcessorModule.prepare(spec);
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
    // AudioBlock is better here since it doesn't allocate or copy
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    gainProcessorModule.setMode(static_cast<int>(stateParam->load()));
    gainProcessorModule.setThreshold(thresholdParam->load());
    gainProcessorModule.setRatio(ratioParam->load());
    gainProcessorModule.setAttackTime(attackParam->load());
    gainProcessorModule.setReleaseTime(releaseParam->load());
    gainProcessorModule.process(context);

    // Publish the current input level for the GUI gain-curve dot
    currentInputDb.store(gainProcessorModule.getSignalDb(), std::memory_order_relaxed);
    currentGainDb.store(gainProcessorModule.getGainDb(), std::memory_order_relaxed);

    // Visualizer Data Update (thread-safe circular buffer write)
    // Grab the current write position
    int currentIndex = writeIndex.load();

    // Write the frame data
    visualBuffer[currentIndex] = {
        gainProcessorModule.getSignalDb(),
        gainProcessorModule.getNoiseFloorDb(),
        gainProcessorModule.getActiveThresholdDb(),
        gainProcessorModule.getRatio(),
        gainProcessorModule.isOpen() // true if signalDb > activeThresholdDb
    };

    // Advance the index, wrapping back to 0 if it hits the end
    writeIndex.store((currentIndex + 1) % bufferSize);
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
