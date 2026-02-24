#include "PluginEditor.h"

namespace
{
    constexpr int kKnobSize    = 80;
    constexpr int kLabelHeight = 20;
    constexpr int kPadding     = 10;
    constexpr int kSectionLabelHeight = 18;

    // Layout constants
    constexpr int kKnobStep    = kKnobSize + kPadding;
    constexpr int kRowHeight   = kSectionLabelHeight + kKnobSize + kLabelHeight + kPadding * 2;
}

//==============================================================================
VocalCleanupAudioProcessorEditor::VocalCleanupAudioProcessorEditor (
    VocalCleanupAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // High-pass
    setupSlider (highPassFreqSlider,   "HP Freq",      "highPassFreq",   highPassFreqAttach);

    // Gate
    setupSlider (gateThreshSlider,     "Gate Thresh",  "gateThreshold",  gateThreshAttach);
    setupSlider (gateAttackSlider,     "Gate Attack",  "gateAttack",     gateAttackAttach);
    setupSlider (gateReleaseSlider,    "Gate Release", "gateRelease",    gateReleaseAttach);

    // Compressor
    setupSlider (compThreshSlider,     "Comp Thresh",  "compThreshold",  compThreshAttach);
    setupSlider (compRatioSlider,      "Comp Ratio",   "compRatio",      compRatioAttach);
    setupSlider (compAttackSlider,     "Comp Attack",  "compAttack",     compAttackAttach);
    setupSlider (compReleaseSlider,    "Comp Release", "compRelease",    compReleaseAttach);

    // De-esser
    setupSlider (deEsserFreqSlider,    "DeEss Freq",   "deEsserFreq",    deEsserFreqAttach);
    setupSlider (deEsserThreshSlider,  "DeEss Thresh", "deEsserThresh",  deEsserThreshAttach);

    // Output gain
    setupSlider (outputGainSlider,     "Output Gain",  "outputGain",     outputGainAttach);

    // 4 sections across: HP | Gate(3) | Comp(4) | DeEss(2) + Out(1)
    // Lay out in two rows of knobs
    // Row 1 (top):  [HP] [Gate-Thresh] [Gate-Atk] [Gate-Rel] [Comp-Thresh]
    // Row 2 (bottom): [Comp-Ratio] [Comp-Atk] [Comp-Rel] [DeEss-Freq] [DeEss-Thresh] [Out-Gain]
    const int totalCols = 6;
    const int totalRows = 2;
    const int editorW   = kPadding + totalCols * kKnobStep + kPadding;
    const int editorH   = kPadding + totalRows * kRowHeight + kPadding;

    setSize (editorW, editorH);
}

//==============================================================================
void VocalCleanupAudioProcessorEditor::setupSlider (
    LabelledSlider& ls,
    const juce::String& labelText,
    const juce::String& paramID,
    std::unique_ptr<SliderAttachment>& attachment)
{
    ls.slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    ls.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, kKnobSize, kLabelHeight);
    addAndMakeVisible (ls.slider);

    ls.label.setText (labelText, juce::dontSendNotification);
    ls.label.setJustificationType (juce::Justification::centred);
    ls.label.setFont (juce::Font (12.0f, juce::Font::bold));
    addAndMakeVisible (ls.label);

    attachment = std::make_unique<SliderAttachment> (
        processorRef.apvts, paramID, ls.slider);
}

//==============================================================================
void VocalCleanupAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e2e));

    // Draw section backgrounds
    const auto bounds = getLocalBounds().toFloat();
    const float rowH  = float (kRowHeight);

    // Section header colours
    auto drawSectionBg = [&] (int col, int row, int numCols, const juce::String& title,
                               juce::Colour colour)
    {
        const float x = float (kPadding + col * kKnobStep);
        const float y = float (kPadding + row * kRowHeight);
        const float w = float (numCols * kKnobStep - kPadding);
        const float h = rowH - float (kPadding);

        juce::Rectangle<float> sectionRect (x, y, w, h);
        g.setColour (colour.withAlpha (0.15f));
        g.fillRoundedRectangle (sectionRect, 6.0f);
        g.setColour (colour.withAlpha (0.6f));
        g.drawRoundedRectangle (sectionRect, 6.0f, 1.0f);

        g.setColour (colour);
        g.setFont (juce::Font (11.0f, juce::Font::bold));
        g.drawText (title, int (x) + 4, int (y) + 2, int (w) - 8, kSectionLabelHeight,
                    juce::Justification::left, false);
    };

    // Row 0: [HP(1)] [Gate(3)] [Comp-first-2(2)]
    drawSectionBg (0, 0, 1, "HIGH-PASS",  juce::Colours::cadetblue);
    drawSectionBg (1, 0, 3, "NOISE GATE", juce::Colours::mediumpurple);
    drawSectionBg (4, 0, 2, "COMPRESSOR", juce::Colours::coral);

    // Row 1: [Comp-last-2(2)] [De-esser(2)] [Output(1)] + one spare
    drawSectionBg (0, 1, 2, "COMPRESSOR", juce::Colours::coral);
    drawSectionBg (2, 1, 2, "DE-ESSER",   juce::Colours::mediumseagreen);
    drawSectionBg (4, 1, 1, "OUTPUT",     juce::Colours::gold);

    juce::ignoreUnused (bounds, rowH);
}

void VocalCleanupAudioProcessorEditor::resized()
{
    // Row 0: HP | Gate x3 | Comp(thresh+ratio)
    // Row 1: Comp(atk+rel) | DeEss x2 | OutputGain
    auto placeKnob = [&] (LabelledSlider& ls, int col, int row)
    {
        const int x = kPadding + col * kKnobStep;
        const int y = kPadding + row * kRowHeight + kSectionLabelHeight + kPadding;
        ls.label.setBounds  (x, y - kLabelHeight, kKnobSize, kLabelHeight);
        ls.slider.setBounds (x, y,                kKnobSize, kKnobSize + kLabelHeight);
    };

    // Row 0
    placeKnob (highPassFreqSlider,  0, 0);
    placeKnob (gateThreshSlider,    1, 0);
    placeKnob (gateAttackSlider,    2, 0);
    placeKnob (gateReleaseSlider,   3, 0);
    placeKnob (compThreshSlider,    4, 0);
    placeKnob (compRatioSlider,     5, 0);

    // Row 1
    placeKnob (compAttackSlider,    0, 1);
    placeKnob (compReleaseSlider,   1, 1);
    placeKnob (deEsserFreqSlider,   2, 1);
    placeKnob (deEsserThreshSlider, 3, 1);
    placeKnob (outputGainSlider,    4, 1);
}
