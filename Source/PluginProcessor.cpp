#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <limits>

//==============================================================================
// Parameter IDs
namespace ParamID
{
    static const juce::String highPassFreq   { "highPassFreq"   };
    static const juce::String gateThreshold  { "gateThreshold"  };
    static const juce::String gateAttack     { "gateAttack"     };
    static const juce::String gateRelease    { "gateRelease"    };
    static const juce::String compThreshold  { "compThreshold"  };
    static const juce::String compRatio      { "compRatio"      };
    static const juce::String compAttack     { "compAttack"     };
    static const juce::String compRelease    { "compRelease"    };
    static const juce::String deEsserFreq    { "deEsserFreq"    };
    static const juce::String deEsserThresh  { "deEsserThresh"  };
    static const juce::String outputGain     { "outputGain"     };
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
    VocalCleanupAudioProcessor::createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    // High-pass filter: 20 – 500 Hz, default 80 Hz
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::highPassFreq, 1 },
        "HP Freq",
        NormalisableRange<float> (20.0f, 500.0f, 1.0f, 0.5f),
        80.0f,
        AudioParameterFloatAttributes{}.withLabel ("Hz")));

    // Noise gate threshold: -80 – 0 dBFS, default -40 dB
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::gateThreshold, 1 },
        "Gate Thresh",
        NormalisableRange<float> (-80.0f, 0.0f, 0.1f),
        -40.0f,
        AudioParameterFloatAttributes{}.withLabel ("dB")));

    // Gate attack: 0.1 – 100 ms, default 5 ms
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::gateAttack, 1 },
        "Gate Attack",
        NormalisableRange<float> (0.1f, 100.0f, 0.1f, 0.5f),
        5.0f,
        AudioParameterFloatAttributes{}.withLabel ("ms")));

    // Gate release: 10 – 2000 ms, default 150 ms
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::gateRelease, 1 },
        "Gate Release",
        NormalisableRange<float> (10.0f, 2000.0f, 1.0f, 0.5f),
        150.0f,
        AudioParameterFloatAttributes{}.withLabel ("ms")));

    // Compressor threshold: -60 – 0 dBFS, default -18 dB
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::compThreshold, 1 },
        "Comp Thresh",
        NormalisableRange<float> (-60.0f, 0.0f, 0.1f),
        -18.0f,
        AudioParameterFloatAttributes{}.withLabel ("dB")));

    // Compressor ratio: 1:1 – 20:1, default 3:1
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::compRatio, 1 },
        "Comp Ratio",
        NormalisableRange<float> (1.0f, 20.0f, 0.1f, 0.5f),
        3.0f,
        AudioParameterFloatAttributes{}.withLabel (":1")));

    // Compressor attack: 0.1 – 200 ms, default 10 ms
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::compAttack, 1 },
        "Comp Attack",
        NormalisableRange<float> (0.1f, 200.0f, 0.1f, 0.5f),
        10.0f,
        AudioParameterFloatAttributes{}.withLabel ("ms")));

    // Compressor release: 10 – 2000 ms, default 100 ms
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::compRelease, 1 },
        "Comp Release",
        NormalisableRange<float> (10.0f, 2000.0f, 1.0f, 0.5f),
        100.0f,
        AudioParameterFloatAttributes{}.withLabel ("ms")));

    // De-esser centre frequency: 3 kHz – 16 kHz, default 8 kHz
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::deEsserFreq, 1 },
        "DeEss Freq",
        NormalisableRange<float> (3000.0f, 16000.0f, 10.0f, 0.5f),
        8000.0f,
        AudioParameterFloatAttributes{}.withLabel ("Hz")));

    // De-esser threshold: -60 – 0 dBFS, default -20 dB
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::deEsserThresh, 1 },
        "DeEss Thresh",
        NormalisableRange<float> (-60.0f, 0.0f, 0.1f),
        -20.0f,
        AudioParameterFloatAttributes{}.withLabel ("dB")));

    // Output gain: -24 – +12 dB, default 0 dB
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::outputGain, 1 },
        "Output Gain",
        NormalisableRange<float> (-24.0f, 12.0f, 0.1f),
        0.0f,
        AudioParameterFloatAttributes{}.withLabel ("dB")));

    return { params.begin(), params.end() };
}

//==============================================================================
VocalCleanupAudioProcessor::VocalCleanupAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

//==============================================================================
void VocalCleanupAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = 1;

    leftChain.prepare  (spec);
    rightChain.prepare (spec);

    deEsserDetectL.prepare (spec);
    deEsserDetectR.prepare (spec);
    deEsserCutL.prepare    (spec);
    deEsserCutR.prepare    (spec);

    outputGainL.prepare (spec);
    outputGainR.prepare (spec);

    updateAllParameters();
}

void VocalCleanupAudioProcessor::releaseResources()
{
}

bool VocalCleanupAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Mono or stereo in, same out
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

//==============================================================================
void VocalCleanupAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    updateAllParameters();

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any extra output channels
    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const int numSamples = buffer.getNumSamples();

    // ── Pre-compute de-esser values shared across both channels ─────────────
    const float deEssThreshLinear = juce::Decibels::decibelsToGain (
        apvts.getRawParameterValue (ParamID::deEsserThresh)->load());
    // Coefficients depend only on sample rate – compute once outside the per-sample loops
    const float deEssAttackCoeff  = std::exp (-1.0f / (float (currentSampleRate) * 0.001f));  // ~1 ms
    const float deEssReleaseCoeff = std::exp (-1.0f / (float (currentSampleRate) * 0.050f));  // ~50 ms
    // Use epsilon to guard against near-zero threshold in the blend division below
    const bool  deEssActive = deEssThreshLinear > std::numeric_limits<float>::epsilon();

    // ── Process left (or mono) channel ──────────────────────────────────────
    {
        juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(),
                                            1, 0,
                                            static_cast<size_t> (numSamples));
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        leftChain.process (ctx);

        auto* writePtr = buffer.getWritePointer (0);
        for (int n = 0; n < numSamples; ++n)
        {
            // Run the detector filter on a copy of the sample
            float detected = deEsserDetectL.processSample (writePtr[n]);
            float envIn    = std::abs (detected);

            // Envelope follower
            if (envIn > deEsserEnvL)
                deEsserEnvL = deEssAttackCoeff  * deEsserEnvL + (1.0f - deEssAttackCoeff)  * envIn;
            else
                deEsserEnvL = deEssReleaseCoeff * deEsserEnvL + (1.0f - deEssReleaseCoeff) * envIn;

            // Always run cut filter to maintain state; blend in proportion to excess above threshold
            float cut = deEsserCutL.processSample (writePtr[n]);
            if (deEssActive && deEsserEnvL > deEssThreshLinear)
            {
                const float blend = juce::jmin (1.0f,
                    (deEsserEnvL - deEssThreshLinear) / deEssThreshLinear);
                writePtr[n] = writePtr[n] + blend * (cut - writePtr[n]);
            }
        }

        // Output gain applied last (correct signal order: chain -> de-esser -> gain)
        juce::dsp::AudioBlock<float> outBlock (buffer.getArrayOfWritePointers(),
                                               1, 0,
                                               static_cast<size_t> (numSamples));
        juce::dsp::ProcessContextReplacing<float> outCtx (outBlock);
        outputGainL.process (outCtx);
    }

    // ── Process right channel (if stereo) ────────────────────────────────────
    if (buffer.getNumChannels() > 1)
    {
        juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers() + 1,
                                            1, 0,
                                            static_cast<size_t> (numSamples));
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        rightChain.process (ctx);

        auto* writePtr = buffer.getWritePointer (1);
        for (int n = 0; n < numSamples; ++n)
        {
            float detected = deEsserDetectR.processSample (writePtr[n]);
            float envIn    = std::abs (detected);

            if (envIn > deEsserEnvR)
                deEsserEnvR = deEssAttackCoeff  * deEsserEnvR + (1.0f - deEssAttackCoeff)  * envIn;
            else
                deEsserEnvR = deEssReleaseCoeff * deEsserEnvR + (1.0f - deEssReleaseCoeff) * envIn;

            float cut = deEsserCutR.processSample (writePtr[n]);
            if (deEssActive && deEsserEnvR > deEssThreshLinear)
            {
                const float blend = juce::jmin (1.0f,
                    (deEsserEnvR - deEssThreshLinear) / deEssThreshLinear);
                writePtr[n] = writePtr[n] + blend * (cut - writePtr[n]);
            }
        }

        // Output gain for right channel
        juce::dsp::AudioBlock<float> outBlock (buffer.getArrayOfWritePointers() + 1,
                                               1, 0,
                                               static_cast<size_t> (numSamples));
        juce::dsp::ProcessContextReplacing<float> outCtx (outBlock);
        outputGainR.process (outCtx);
    }
}

//==============================================================================
void VocalCleanupAudioProcessor::updateAllParameters()
{
    updateHighPass();
    updateGate();
    updateCompressor();
    updateDeEsser();
    updateOutputGain();
}

void VocalCleanupAudioProcessor::updateHighPass()
{
    const float freq = apvts.getRawParameterValue (ParamID::highPassFreq)->load();
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, freq);

    *leftChain .get<HighPass>().coefficients = *coeffs;
    *rightChain.get<HighPass>().coefficients = *coeffs;
}

void VocalCleanupAudioProcessor::updateGate()
{
    const float thresholdDb = apvts.getRawParameterValue (ParamID::gateThreshold)->load();
    const float attackMs    = apvts.getRawParameterValue (ParamID::gateAttack)->load();
    const float releaseMs   = apvts.getRawParameterValue (ParamID::gateRelease)->load();

    for (auto* chain : { &leftChain, &rightChain })
    {
        auto& gate = chain->get<Gate>();
        gate.setThreshold (thresholdDb);
        gate.setAttack    (attackMs);
        gate.setRelease   (releaseMs);
    }
}

void VocalCleanupAudioProcessor::updateCompressor()
{
    const float thresholdDb = apvts.getRawParameterValue (ParamID::compThreshold)->load();
    const float ratio       = apvts.getRawParameterValue (ParamID::compRatio)->load();
    const float attackMs    = apvts.getRawParameterValue (ParamID::compAttack)->load();
    const float releaseMs   = apvts.getRawParameterValue (ParamID::compRelease)->load();

    for (auto* chain : { &leftChain, &rightChain })
    {
        auto& comp = chain->get<Comp>();
        comp.setThreshold (thresholdDb);
        comp.setRatio     (ratio);
        comp.setAttack    (attackMs);
        comp.setRelease   (releaseMs);
    }
}

void VocalCleanupAudioProcessor::updateDeEsser()
{
    const float freq = apvts.getRawParameterValue (ParamID::deEsserFreq)->load();
    const float q    = 2.0f;   // moderate bandwidth for detection
    const float gainDb = -12.0f; // peak-cut depth when de-essing

    // Bandpass for detection
    auto detectCoeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass (
        currentSampleRate, freq, q);
    *deEsserDetectL.coefficients = *detectCoeffs;
    *deEsserDetectR.coefficients = *detectCoeffs;

    // Peak-cut for gain reduction
    auto cutCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        currentSampleRate, freq, q, juce::Decibels::decibelsToGain (gainDb));
    *deEsserCutL.coefficients = *cutCoeffs;
    *deEsserCutR.coefficients = *cutCoeffs;
}

void VocalCleanupAudioProcessor::updateOutputGain()
{
    const float gainDb = apvts.getRawParameterValue (ParamID::outputGain)->load();
    outputGainL.setGainDecibels (gainDb);
    outputGainR.setGainDecibels (gainDb);
}

//==============================================================================
void VocalCleanupAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream (destData, true);
    apvts.state.writeToStream (stream);
}

void VocalCleanupAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ValueTree tree = juce::ValueTree::readFromData (data, static_cast<size_t> (sizeInBytes));
    if (tree.isValid())
        apvts.replaceState (tree);
}

//==============================================================================
juce::AudioProcessorEditor* VocalCleanupAudioProcessor::createEditor()
{
    return new VocalCleanupAudioProcessorEditor (*this);
}

//==============================================================================
// Plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalCleanupAudioProcessor();
}
