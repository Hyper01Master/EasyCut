#include "PluginEditor.h"

namespace
{
void styleLabel (juce::Label& l, const juce::String& text, float size, juce::Colour c, bool bold)
{
    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, c);
    l.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    l.setInterceptsMouseClicks (false, false);
}
} // namespace

EasyCutEditor::EasyCutEditor (EasyCutProcessor& p)
    : AudioProcessorEditor (&p), proc (p), display (p)
{
    setSize (620, 420);

    addAndMakeVisible (display);

    for (auto* s : { &freqKnob, &slopeKnob })
    {
        s->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s->setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        s->setLookAndFeel (&lnf);
        addAndMakeVisible (*s);
    }
    slopeKnob.setMouseDragSensitivity (140);

    styleLabel (freqTitle, "FREQ", 14.0f, theme::text, true);
    styleLabel (slopeTitle, "SLOPE", 14.0f, theme::text, true);
    styleLabel (freqValue, "", 15.0f, theme::textBright, true);
    styleLabel (slopeValue, "", 15.0f, theme::textBright, true);
    for (auto* l : { &freqTitle, &slopeTitle, &freqValue, &slopeValue })
        addAndMakeVisible (*l);

    freqAtt = std::make_unique<SliderAttachment> (proc.apvts, "freq", freqKnob);
    slopeAtt = std::make_unique<SliderAttachment> (proc.apvts, "slope", slopeKnob);

    freqKnob.onValueChange = [this] { updateReadouts(); };
    slopeKnob.onValueChange = [this] { updateReadouts(); };
    updateReadouts();
}

EasyCutEditor::~EasyCutEditor()
{
    freqKnob.setLookAndFeel (nullptr);
    slopeKnob.setLookAndFeel (nullptr);
}

void EasyCutEditor::updateReadouts()
{
    freqValue.setText (juce::String (juce::roundToInt ((float) freqKnob.getValue())) + " Hz", juce::dontSendNotification);
    const auto& names = EasyCutProcessor::slopeNames();
    slopeValue.setText (names[juce::jlimit (0, names.size() - 1, juce::roundToInt ((float) slopeKnob.getValue()))],
                        juce::dontSendNotification);
}

void EasyCutEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::bg);

    auto outer = getLocalBounds().toFloat().reduced (6.0f);
    g.setColour (theme::panel);
    g.fillRoundedRectangle (outer, 26.0f);
    g.setColour (theme::panelEdge);
    g.drawRoundedRectangle (outer, 26.0f, 1.5f);

    g.setColour (theme::textBright);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("EASYCUT", 28, 14, 200, 28, juce::Justification::centredLeft);
}

void EasyCutEditor::resized()
{
    auto area = getLocalBounds().reduced (18);
    area.removeFromTop (34);
    display.setBounds (area.removeFromTop (170));
    area.removeFromTop (10);

    const int knob = 120;
    auto left = area.removeFromLeft (area.getWidth() / 2);
    auto right = area;

    freqKnob.setBounds (left.withSizeKeepingCentre (knob, knob).withY (left.getY()));
    slopeKnob.setBounds (right.withSizeKeepingCentre (knob, knob).withY (right.getY()));

    freqTitle.setBounds (left.getX(), left.getY() + knob, left.getWidth(), 18);
    freqValue.setBounds (left.getX(), left.getY() + knob + 18, left.getWidth(), 20);
    slopeTitle.setBounds (right.getX(), right.getY() + knob, right.getWidth(), 18);
    slopeValue.setBounds (right.getX(), right.getY() + knob + 18, right.getWidth(), 20);
}
