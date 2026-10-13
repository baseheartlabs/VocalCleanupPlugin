/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::Colour colourForMode(int mode)
    {
        static const std::array<juce::Colour, 6> colours{
            juce::Colour(0xffe06c4f), juce::Colour(0xff43b7a5),
            juce::Colour(0xffd5a447), juce::Colour(0xff6e9fe8),
            juce::Colour(0xffc46fd1), juce::Colour(0xff78b85a) };
        return colours[(size_t) juce::jlimit(0, 5, mode)];
    }
}

//==============================================================================
VocalCleanupPluginAudioProcessorEditor::VocalCleanupPluginAudioProcessorEditor (
    VocalCleanupPluginAudioProcessor& p, 
    juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p), valueTreeState (vts)
{
    addAndMakeVisible(VAD);
    startTimerHz(30); // Update display at 30 FPS

    addAndMakeVisible(gainCurveVisualizer);
    addAndMakeVisible(ballisticsVisualizer);
    addAndMakeVisible(moduleSelector);

    moduleSelector.setOnStateChanged([this](int state)
    {
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(
                valueTreeState.getParameter(VocalCleanupPluginAudioProcessor::stateParamID)))
        {
            choice->setValueNotifyingHost(choice->convertTo0to1(state));
            updateTheme();
        }
    });

    // Threshold slider (dB)
    thresholdLabel.setText("Threshold", juce::dontSendNotification);
    addAndMakeVisible(thresholdLabel);

    thresholdSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    thresholdSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    thresholdSlider.setTextValueSuffix(" dB");
    addAndMakeVisible(thresholdSlider);
    thresholdAttachment = std::make_unique<SliderAttachment>(
        valueTreeState, VocalCleanupPluginAudioProcessor::thresholdParamID, thresholdSlider);

    // Ratio slider
    ratioLabel.setText("Ratio", juce::dontSendNotification);
    addAndMakeVisible(ratioLabel);

    ratioSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    ratioSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    ratioSlider.setTextValueSuffix(" : 1");
    addAndMakeVisible(ratioSlider);
    ratioAttachment = std::make_unique<SliderAttachment>(
        valueTreeState, VocalCleanupPluginAudioProcessor::ratioParamID, ratioSlider);

    attackLabel.setText("Attack", juce::dontSendNotification);
    addAndMakeVisible(attackLabel);
    attackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    attackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    attackSlider.setTextValueSuffix(" ms");
    addAndMakeVisible(attackSlider);
    attackAttachment = std::make_unique<SliderAttachment>(
        valueTreeState, VocalCleanupPluginAudioProcessor::attackParamID, attackSlider);

    releaseLabel.setText("Release", juce::dontSendNotification);
    addAndMakeVisible(releaseLabel);
    releaseSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    releaseSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    releaseSlider.setTextValueSuffix(" ms");
    addAndMakeVisible(releaseSlider);
    releaseAttachment = std::make_unique<SliderAttachment>(
        valueTreeState, VocalCleanupPluginAudioProcessor::releaseParamID, releaseSlider);

    setSize (820, 520);
    updateTheme();
}

VocalCleanupPluginAudioProcessorEditor::~VocalCleanupPluginAudioProcessorEditor()
{
    stopTimer();

    // CRITICAL: Must remove listeners to avoid crashes
}

//==============================================================================
void VocalCleanupPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    const int mode = (int) valueTreeState.getRawParameterValue(
        VocalCleanupPluginAudioProcessor::stateParamID)->load();
    const auto accent = colourForMode(mode);
    auto panel = getLocalBounds().toFloat().reduced(6.0f);
        g.fillAll(juce::Colour(0xff101a26));
    g.setColour(juce::Colour(0xff111b27));
    g.fillRoundedRectangle(panel, 8.0f);
    g.setColour(juce::Colour(0xff344558));
    g.drawRoundedRectangle(panel.reduced(1.0f), 8.0f, 1.0f);
    g.setColour(accent.withAlpha(0.75f));
    g.drawRoundedRectangle(panel.reduced(4.0f), 6.0f, 1.0f);
}

void VocalCleanupPluginAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced(10);

    // Bottom strip holds the four exposed controls
    auto controls = r.removeFromBottom(128);

    auto selectorArea = r.removeFromLeft(190);
    r.removeFromLeft(12);
    moduleSelector.setBounds(selectorArea);

    VAD.setBounds(r.getX(), r.getY(), 40, 40);
    auto left = r.removeFromLeft(r.getWidth() / 2 - 4);
    r.removeFromLeft(8);
    gainCurveVisualizer.setBounds(left);
    ballisticsVisualizer.setBounds(r);

    // Lay out the controls left-to-right with labels directly above each knob
    const int cellW = controls.getWidth() / 4;

    auto thresholdCell = controls.removeFromLeft(cellW).reduced(8, 0);
    thresholdLabel.setBounds(thresholdCell.getX(), thresholdCell.getY(), thresholdCell.getWidth(), 18);
    thresholdSlider.setBounds(thresholdCell.withTrimmedTop(18));

    auto ratioCell = controls.removeFromLeft(cellW).reduced(8, 0);
    ratioLabel.setBounds(ratioCell.getX(), ratioCell.getY(), ratioCell.getWidth(), 18);
    ratioSlider.setBounds(ratioCell.withTrimmedTop(18));

    auto attackCell = controls.removeFromLeft(cellW).reduced(8, 0);
    attackLabel.setBounds(attackCell.getX(), attackCell.getY(), attackCell.getWidth(), 18);
    attackSlider.setBounds(attackCell.withTrimmedTop(18));

    auto releaseCell = controls.reduced(8, 0);
    releaseLabel.setBounds(releaseCell.getX(), releaseCell.getY(), releaseCell.getWidth(), 18);
    releaseSlider.setBounds(releaseCell.withTrimmedTop(18));
}

//==============================================================================
// Called when user moves the slider
void VocalCleanupPluginAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    
}

// Called when parameter changes (automation, preset load, etc.)
void VocalCleanupPluginAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
   
}

// Timer callback to poll voice detection state from processor
void VocalCleanupPluginAudioProcessorEditor::timerCallback()
{
    bool voiceDetected = audioProcessor.isVoiceDetected.load(std::memory_order_relaxed);
    VAD.setState(voiceDetected);
    moduleSelector.setSelectedState((int) valueTreeState.getRawParameterValue(
        VocalCleanupPluginAudioProcessor::stateParamID)->load());
    updateTheme();
}

void VocalCleanupPluginAudioProcessorEditor::updateTheme()
{
    const int mode = (int) valueTreeState.getRawParameterValue(
        VocalCleanupPluginAudioProcessor::stateParamID)->load();
    const auto accent = colourForMode(mode);

    moduleSelector.setAccentColour(accent);
    gainCurveVisualizer.setAccentColour(accent);
    ballisticsVisualizer.setAccentColour(accent);

    for (auto* slider : { &thresholdSlider, &ratioSlider, &attackSlider, &releaseSlider })
    {
        slider->setColour(juce::Slider::rotarySliderFillColourId, accent);
        slider->setColour(juce::Slider::thumbColourId, accent.brighter(0.3f));
        slider->setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff52657a));
        slider->setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffdbe9f5));
        slider->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff101a26));
        slider->setColour(juce::Slider::textBoxOutlineColourId, accent.withAlpha(0.7f));
    }

    for (auto* label : { &thresholdLabel, &ratioLabel, &attackLabel, &releaseLabel })
    {
        label->setColour(juce::Label::textColourId, juce::Colour(0xffdbe9f5));
        label->setJustificationType(juce::Justification::centred);
        label->setFont(juce::Font("Bahnschrift", 11.0f, juce::Font::bold));
    }

    repaint();
}

//==============================================================================
ModuleSelector::ModuleSelector() = default;

const juce::StringArray& ModuleSelector::labels()
{
    static const juce::StringArray names{
        "Limiter", "Gate", "Upward Compressor", "Downward Compressor",
        "Upward Expander", "Downward Expander" };
    return names;
}

void ModuleSelector::setSelectedState(int state)
{
    const int clamped = juce::jlimit(0, 5, state);
    if (selectedState != clamped)
    {
        selectedState = clamped;
        repaint();
    }
}

void ModuleSelector::setAccentColour(juce::Colour colour)
{
    accentColour = colour;
    repaint();
}

void ModuleSelector::setOnStateChanged(std::function<void(int)> callback)
{
    onStateChanged = std::move(callback);
}

void ModuleSelector::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(accentColour.darker(0.7f));
    g.fillRoundedRectangle(bounds, 10.0f);
    g.setColour(accentColour.brighter(0.25f));
    g.drawRoundedRectangle(bounds.reduced(1.0f), 10.0f, 1.0f);

    const auto area = getLocalBounds().reduced(10, 12);
    const int rowHeight = juce::jmax(1, area.getHeight() / 3);
    const int columnWidth = juce::jmax(1, area.getWidth() / 2);
    const auto& names = labels();

    g.setFont(juce::Font("Bahnschrift", 10.5f, juce::Font::bold));
    for (int state = 0; state < 6; ++state)
    {
        const int row = state / 2;
        const int column = state % 2;
        auto slot = juce::Rectangle<int>(area.getX() + column * columnWidth + 3,
                                         area.getY() + row * rowHeight + 3,
                                         columnWidth - 6, rowHeight - 6);
        auto toggle = slot.reduced(4, 5);

        if (state == selectedState)
        {
            g.setColour(accentColour.brighter(0.2f));
            g.fillRoundedRectangle(toggle.toFloat(), 4.0f);
            g.setColour(juce::Colours::black.withAlpha(0.35f));
        }
        else
        {
            g.setColour(juce::Colour(0xff0a111a).withAlpha(0.9f));
            g.fillRoundedRectangle(toggle.toFloat(), 4.0f);
            g.setColour(accentColour.withAlpha(0.6f));
        }

        g.drawRoundedRectangle(toggle.toFloat(), 4.0f, 1.0f);
            g.setColour(juce::Colour(0xffdbe9f5).withAlpha(state == selectedState ? 1.0f : 0.72f));
        g.drawFittedText(names[state], toggle.reduced(4, 1),
                         juce::Justification::centred, 2, 0.8f);
    }
}

void ModuleSelector::mouseDown(const juce::MouseEvent& event)
{
    const auto area = getLocalBounds().reduced(10, 12);
    const int rowHeight = juce::jmax(1, area.getHeight() / 3);
    const int columnWidth = juce::jmax(1, area.getWidth() / 2);
    const int row = juce::jlimit(0, 2, (event.y - area.getY()) / rowHeight);
    const int column = juce::jlimit(0, 1, (event.x - area.getX()) / columnWidth);
    const int state = row * 2 + column;

    setSelectedState(state);
    if (onStateChanged)
        onStateChanged(state);
}

//==============================================================================
GainCurveVisualizer::GainCurveVisualizer(VocalCleanupPluginAudioProcessor& p)
    : processor(p)
{
    startTimerHz(30);
}

GainCurveVisualizer::~GainCurveVisualizer()
{
    stopTimer();
}

void GainCurveVisualizer::setAccentColour(juce::Colour colour)
{
    accentColour = colour;
    repaint();
}

void GainCurveVisualizer::timerCallback()
{
    repaint();
}

void GainCurveVisualizer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);

    // Background
    g.setColour(juce::Colour(0xff0b121c));
    g.fillRect(bounds);
    g.setColour(juce::Colour(0xff344558));
    g.drawRect(bounds, 1.0f);

    const float left = bounds.getX();
    const float right = bounds.getRight();
    const float top = bounds.getY();
    const float bottom = bounds.getBottom();

    auto dbToX = [=](float db) { return juce::jmap(db, minDb, maxDb, left, right); };
    auto dbToY = [=](float db) { return juce::jmap(db, minDb, maxDb, bottom, top); };

    // Grid + axis labels every 10 dB
    g.setFont(juce::Font("Bahnschrift", 10.0f, juce::Font::plain));
    for (float db = minDb; db <= maxDb + 0.01f; db += 10.0f)
    {
        float x = dbToX(db);
        float y = dbToY(db);

        g.setColour(juce::Colour(0xff8ba8c2).withAlpha(0.1f));
        g.drawVerticalLine((int)x, top, bottom);
        g.drawHorizontalLine((int)y, left, right);

        g.setColour(juce::Colour(0xff9eb7cc));
        g.drawText(juce::String((int)db), (int)x - 14, (int)bottom - 14, 28, 12,
                   juce::Justification::centred);      // input axis (x)
        g.drawText(juce::String((int)db), (int)left + 2, (int)y - 6, 24, 12,
                   juce::Justification::left);          // output axis (y)
    }

    // Unity reference line (input == output)
    g.setColour(juce::Colour(0xff7690a8).withAlpha(0.8f));
    g.drawLine(dbToX(minDb), dbToY(minDb), dbToX(maxDb), dbToY(maxDb), 1.0f);

    // Read the current parameter values
    auto& apvts = processor.parameterManager;
    int mode = (int)apvts.getRawParameterValue(VocalCleanupPluginAudioProcessor::stateParamID)->load();
    float thresholdDb = apvts.getRawParameterValue(VocalCleanupPluginAudioProcessor::thresholdParamID)->load();
    float ratio = apvts.getRawParameterValue(VocalCleanupPluginAudioProcessor::ratioParamID)->load();

    // Threshold marker
    g.setColour(accentColour.withAlpha(0.8f));
    g.drawVerticalLine((int)dbToX(thresholdDb), top, bottom);

    // Transfer curve: sample the shared static function across the input range
    juce::Path curve;
    bool started = false;
    for (float x = left; x <= right; x += 1.0f)
    {
        float inDb = juce::jmap(x, left, right, minDb, maxDb);
        float outDb = GainProcessors::computeOutputDb(inDb, mode, thresholdDb, ratio);
        float y = juce::jlimit(top, bottom, dbToY(outDb));

        if (!started) { curve.startNewSubPath(x, y); started = true; }
        else          curve.lineTo(x, y);
    }
    g.setColour(accentColour);
    g.strokePath(curve, juce::PathStrokeType(2.0f));

    // Live dot: where the current input level sits on the curve
    float inputDb = processor.currentInputDb.load(std::memory_order_relaxed);
    inputDb = juce::jlimit(minDb, maxDb, inputDb);
    float outputDb = GainProcessors::computeOutputDb(inputDb, mode, thresholdDb, ratio);
    float dotX = dbToX(inputDb);
    float dotY = juce::jlimit(top, bottom, dbToY(outputDb));

    g.setColour(juce::Colour(0xfff3c969));
    g.fillEllipse(dotX - 4.0f, dotY - 4.0f, 8.0f, 8.0f);
    g.setColour(juce::Colours::white);
    g.drawEllipse(dotX - 4.0f, dotY - 4.0f, 8.0f, 8.0f, 1.0f);
}

//==============================================================================
BallisticsVisualizer::BallisticsVisualizer(VocalCleanupPluginAudioProcessor& p)
    : processor(p), history(240, 0.0f)
{
    startTimerHz(30);
}

BallisticsVisualizer::~BallisticsVisualizer()
{
    stopTimer();
}

void BallisticsVisualizer::setAccentColour(juce::Colour colour)
{
    accentColour = colour;
    repaint();
}

void BallisticsVisualizer::timerCallback()
{
    history.erase(history.begin());
    history.push_back(processor.currentGainDb.load(std::memory_order_relaxed));
    repaint();
}

void BallisticsVisualizer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colour(0xff0b121c));
    g.fillRect(bounds);
    g.setColour(juce::Colour(0xff344558));
    g.drawRect(bounds, 1.0f);

    const float left = bounds.getX();
    const float right = bounds.getRight();
    const float top = bounds.getY();
    const float bottom = bounds.getBottom();
    const float minDb = -60.0f;
    const float maxDb = 12.0f;
    auto dbToY = [=](float db) { return juce::jmap(db, minDb, maxDb, bottom, top); };

    for (float db = -60.0f; db <= 12.0f; db += 12.0f)
    {
        auto y = dbToY(db);
        g.setColour(juce::Colour(0xff8ba8c2).withAlpha(0.1f));
        g.drawHorizontalLine((int)y, left, right);
        g.setColour(juce::Colour(0xff9eb7cc));
        g.drawText(juce::String((int)db), (int)left + 3, (int)y - 6, 34, 12,
                   juce::Justification::left);
    }

    g.setColour(juce::Colour(0xff7690a8).withAlpha(0.8f));
    g.drawHorizontalLine((int)dbToY(0.0f), left, right);

    juce::Path trace;
    for (size_t i = 0; i < history.size(); ++i)
    {
        float x = juce::jmap((float)i, 0.0f, (float)(history.size() - 1), left, right);
        float y = juce::jlimit(top, bottom, dbToY(history[i]));
        if (i == 0) trace.startNewSubPath(x, y);
        else trace.lineTo(x, y);
    }

    g.setColour(accentColour);
    g.strokePath(trace, juce::PathStrokeType(2.0f));
    g.setColour(juce::Colour(0xfff3c969));
    g.drawText(juce::String(processor.currentGainDb.load(), 1) + " dB",
               (int)right - 75, (int)top + 4, 70, 14, juce::Justification::right);
}


