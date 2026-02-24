#pragma once

#include "PluginProcessor.h"

//==============================================================================
/**
 * VocalCleanupAudioProcessorEditor
 *
 * Provides a GUI with labelled knobs for all plugin parameters.
 */
class VocalCleanupAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalCleanupAudioProcessorEditor (VocalCleanupAudioProcessor&);
    ~VocalCleanupAudioProcessorEditor() override = default;

    //==========================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==========================================================================
    VocalCleanupAudioProcessor& processorRef;

    // Helper that creates a rotary slider with a matching label
    struct LabelledSlider
    {
        juce::Slider slider;
        juce::Label  label;
    };

    LabelledSlider highPassFreqSlider;
    LabelledSlider gateThreshSlider;
    LabelledSlider gateAttackSlider;
    LabelledSlider gateReleaseSlider;
    LabelledSlider compThreshSlider;
    LabelledSlider compRatioSlider;
    LabelledSlider compAttackSlider;
    LabelledSlider compReleaseSlider;
    LabelledSlider deEsserFreqSlider;
    LabelledSlider deEsserThreshSlider;
    LabelledSlider outputGainSlider;

    // APVTS attachments keep sliders in sync with parameters
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> highPassFreqAttach;
    std::unique_ptr<SliderAttachment> gateThreshAttach;
    std::unique_ptr<SliderAttachment> gateAttackAttach;
    std::unique_ptr<SliderAttachment> gateReleaseAttach;
    std::unique_ptr<SliderAttachment> compThreshAttach;
    std::unique_ptr<SliderAttachment> compRatioAttach;
    std::unique_ptr<SliderAttachment> compAttackAttach;
    std::unique_ptr<SliderAttachment> compReleaseAttach;
    std::unique_ptr<SliderAttachment> deEsserFreqAttach;
    std::unique_ptr<SliderAttachment> deEsserThreshAttach;
    std::unique_ptr<SliderAttachment> outputGainAttach;

    void setupSlider (LabelledSlider& ls, const juce::String& labelText,
                      const juce::String& paramID,
                      std::unique_ptr<SliderAttachment>& attachment);

    // Section label rectangles painted in paint()
    enum class Section { HighPass, Gate, Compressor, DeEsser, Output };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalCleanupAudioProcessorEditor)
};
