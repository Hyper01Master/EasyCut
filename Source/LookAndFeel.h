#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace theme
{
const juce::Colour bg         { 0xff121214 };
const juce::Colour panel      { 0xff1e1e22 };
const juce::Colour panelEdge  { 0xff2c2c32 };
const juce::Colour display    { 0xff17171a };
const juce::Colour accent     { 0xffa78bfa };   // lavender, DualCut-style
const juce::Colour accentDim  { 0xff4a3f73 };
const juce::Colour text       { 0xff8c8c96 };
const juce::Colour textBright { 0xffd9d6e8 };
const juce::Colour knobTop    { 0xff2f2f35 };
const juce::Colour knobBottom { 0xff17171a };
} // namespace theme

class EasyCutLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);
        const float arcR = r - 2.0f;
        const bool discrete = s.getInterval() >= 1.0;

        if (discrete)
        {
            const int steps = (int) (s.getMaximum() - s.getMinimum()) + 1;
            for (int i = 0; i < steps; ++i)
            {
                const float a = startAngle + (float) i / (float) (steps - 1) * (endAngle - startAngle);
                const juce::Point<float> p (c.x + arcR * std::sin (a), c.y - arcR * std::cos (a));
                const bool on = i <= juce::roundToInt (pos * (float) (steps - 1));
                g.setColour (on ? theme::accent : theme::accentDim.withAlpha (0.6f));
                g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (p));
            }
        }
        else
        {
            juce::Path track;
            track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
            g.setColour (theme::accentDim.withAlpha (0.5f));
            g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            juce::Path val;
            val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
            g.setColour (theme::accent);
            g.strokePath (val, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const float bodyR = r - 12.0f;
        g.setGradientFill (juce::ColourGradient (theme::knobTop, c.x, c.y - bodyR, theme::knobBottom, c.x, c.y + bodyR, false));
        g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.5f);

        const juce::Point<float> a (c.x + bodyR * 0.5f * std::sin (angle), c.y - bodyR * 0.5f * std::cos (angle));
        const juce::Point<float> e (c.x + bodyR * 0.88f * std::sin (angle), c.y - bodyR * 0.88f * std::cos (angle));
        g.setColour (theme::textBright);
        g.drawLine ({ a, e }, 3.0f);
    }
};
