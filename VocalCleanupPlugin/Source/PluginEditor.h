/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <functional>
#include "PluginProcessor.h"
#include "GateVisualizer.h"

//==============================================================================
/**
*/

class VADComponent : public juce::Component
{
public:
    void setState(bool isVoiceDetected)
    {
        voiceDetected = isVoiceDetected;
        repaint();
    }
    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xff0c0a09));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 3.0f);
        g.setColour(voiceDetected ? juce::Colour(0xff8fbf58) : juce::Colour(0xff8b473a));
        g.fillEllipse(getLocalBounds().toFloat().reduced(5.0f));
        g.setColour(juce::Colour(0xffd6bd8b));
        g.drawEllipse(getLocalBounds().toFloat().reduced(2.0f), 1.0f);
    }
private:
    bool voiceDetected = false;
};

//==============================================================================
// Draws the current dynamics transfer curve (input dB -> output dB) for the
// selected mode/threshold/ratio, plus a live dot showing where the current
// input level sits on that curve.
class GainCurveVisualizer : public juce::Component, private juce::Timer
{
public:
    GainCurveVisualizer(VocalCleanupPluginAudioProcessor& p);
    ~GainCurveVisualizer() override;

    void setAccentColour(juce::Colour colour);

    void paint(juce::Graphics& g) override;
    void timerCallback() override;

private:
    VocalCleanupPluginAudioProcessor& processor;
    juce::Colour accentColour = juce::Colours::aqua;

    // Displayed dB range for both axes
    static constexpr float minDb = -60.0f;
    static constexpr float maxDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainCurveVisualizer)
};

class BallisticsVisualizer : public juce::Component, private juce::Timer
{
public:
    explicit BallisticsVisualizer(VocalCleanupPluginAudioProcessor& p);
    ~BallisticsVisualizer() override;

    void setAccentColour(juce::Colour colour);

    void paint(juce::Graphics&) override;
    void timerCallback() override;

private:
    VocalCleanupPluginAudioProcessor& processor;
    std::vector<float> history;
    juce::Colour accentColour = juce::Colours::aqua;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BallisticsVisualizer)
};

class ModuleSelector : public juce::Component
{
public:
    ModuleSelector();

    void setSelectedState(int state);
    void setAccentColour(juce::Colour colour);
    void setOnStateChanged(std::function<void(int)> callback);
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    int selectedState = 0;
    juce::Colour accentColour = juce::Colours::orange;
    std::function<void(int)> onStateChanged;

    static const juce::StringArray& labels();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModuleSelector)
};

class VocalCleanupPluginAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                               public juce::Slider::Listener,
                                               public juce::AudioProcessorValueTreeState::Listener,
                                               private juce::Timer
{
public:
    VocalCleanupPluginAudioProcessorEditor(VocalCleanupPluginAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~VocalCleanupPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    // MUST implement these when inheriting from the listeners
    void sliderValueChanged(juce::Slider* slider) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void timerCallback() override;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    VocalCleanupPluginAudioProcessor& audioProcessor;
	VADComponent VAD; // Custom component to visualize voice activity detection
    ModuleSelector moduleSelector;
    GainCurveVisualizer gainCurveVisualizer{ audioProcessor };
    BallisticsVisualizer ballisticsVisualizer{ audioProcessor };

    juce::AudioProcessorValueTreeState& valueTreeState;

    // User-facing controls
    juce::Slider thresholdSlider;
    juce::Slider ratioSlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    juce::Label thresholdLabel;
    juce::Label ratioLabel;
    juce::Label attackLabel;
    juce::Label releaseLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> thresholdAttachment;
    std::unique_ptr<SliderAttachment> ratioAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;


private:
    void updateTheme();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalCleanupPluginAudioProcessorEditor)
};

