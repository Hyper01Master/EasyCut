#pragma once
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"

// Live pre/post spectrum with the filter curve and a cutoff pill on top
class SpectrumDisplay : public juce::Component, private juce::Timer
{
public:
    explicit SpectrumDisplay (EasyCutProcessor& p) : proc (p)
    {
        history[0].assign (fftSize, 0.0f);
        history[1].assign (fftSize, 0.0f);
        db[0].assign (fftSize / 2, floorDb);
        db[1].assign (fftSize / 2, floorDb);
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        g.setColour (theme::display);
        g.fillRoundedRectangle (area, 18.0f);

        auto plot = area.reduced (14.0f, 12.0f);
        g.saveState();
        g.reduceClipRegion (area.reduced (2.0f).toNearestInt());

        const double sr = juce::jmax (8000.0, proc.getSampleRate());

        // grid
        g.setFont (juce::FontOptions (11.0f));
        for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
        {
            const float x = freqToX (f, plot);
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
            g.setColour (theme::text.withAlpha (0.55f));
            g.drawText (f >= 1000.0f ? juce::String (f / 1000.0f, 0) + "k" : juce::String ((int) f),
                        juce::Rectangle<float> (x - 20.0f, plot.getBottom() - 14.0f, 40.0f, 12.0f),
                        juce::Justification::centred);
        }

        // spectra
        drawSpectrum (g, plot, db[0], sr, theme::text.withAlpha (0.22f), theme::text.withAlpha (0.12f));
        drawSpectrum (g, plot, db[1], sr, theme::accent, theme::accent.withAlpha (0.22f));

        // filter curve
        const float fc = proc.apvts.getRawParameterValue ("freq")->load();
        const int slope = juce::roundToInt (proc.apvts.getRawParameterValue ("slope")->load());
        const float yZero = plot.getY() + plot.getHeight() * 0.38f;
        const float yBottom = plot.getBottom();

        juce::Path curve;
        for (float x = plot.getX(); x <= plot.getRight(); x += 2.0f)
        {
            const double mag = easycut::highPassMagnitude (sr, fc, slope, (double) xToFreq (x, plot));
            const float dbv = (float) juce::jmax (-80.0, 20.0 * std::log10 (juce::jmax (1.0e-6, mag)));
            const float y = yZero + (-dbv / 60.0f) * (yBottom - yZero);
            const float yc = juce::jmin (y, yBottom);
            if (x == plot.getX()) curve.startNewSubPath (x, yc); else curve.lineTo (x, yc);
        }
        g.setColour (theme::textBright);
        g.strokePath (curve, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // cutoff marker + pill
        const float xc = freqToX (fc, plot);
        g.setColour (theme::accent.withAlpha (0.35f));
        g.drawVerticalLine ((int) xc, plot.getY() + 24.0f, plot.getBottom());
        auto pill = juce::Rectangle<float> (64.0f, 20.0f).withCentre ({ xc, plot.getY() + 12.0f });
        pill = pill.withX (juce::jlimit (plot.getX(), plot.getRight() - pill.getWidth(), pill.getX()));
        g.setColour (theme::display);
        g.fillRoundedRectangle (pill, 10.0f);
        g.setColour (theme::accent);
        g.drawRoundedRectangle (pill, 10.0f, 1.5f);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (juce::String (juce::roundToInt (fc)) + " Hz", pill, juce::Justification::centred);

        g.restoreState();
    }

private:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr float floorDb = -110.0f;
    static constexpr float topDb = -6.0f, bottomDb = -90.0f;

    EasyCutProcessor& proc;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> history[2], db[2];
    std::array<float, 4096> scratch {};
    std::array<float, 2 * fftSize> fftData {};

    static float freqToX (float f, juce::Rectangle<float> r)
    {
        return juce::jmap (std::log10 (f), std::log10 (20.0f), std::log10 (20000.0f), r.getX(), r.getRight());
    }
    static float xToFreq (float x, juce::Rectangle<float> r)
    {
        return std::pow (10.0f, juce::jmap (x, r.getX(), r.getRight(), std::log10 (20.0f), std::log10 (20000.0f)));
    }
    static float tilt (float f) { return 4.5f * std::log2 (f / 1000.0f); }

    void drawSpectrum (juce::Graphics& g, juce::Rectangle<float> r, const std::vector<float>& d, double sr,
                       juce::Colour line, juce::Colour fill) const
    {
        juce::Path p;
        for (float x = r.getX(); x <= r.getRight(); x += 2.0f)
        {
            const float f = xToFreq (x, r);
            const float bin = juce::jlimit (0.0f, (float) d.size() - 2.0f, f / (float) sr * (float) fftSize);
            const int i = (int) bin;
            const float v = juce::jmap (bin - (float) i, d[(size_t) i], d[(size_t) i + 1]) + tilt (f);
            const float y = juce::jmap (juce::jlimit (bottomDb, topDb, v), bottomDb, topDb, r.getBottom(), r.getY());
            if (x == r.getX()) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        juce::Path filled (p);
        filled.lineTo (r.getRight(), r.getBottom());
        filled.lineTo (r.getX(), r.getBottom());
        filled.closeSubPath();
        g.setColour (fill);
        g.fillPath (filled);
        g.setColour (line);
        g.strokePath (p, juce::PathStrokeType (1.4f));
    }

    void analyse (easycut::SampleFifo& fifo, std::vector<float>& hist, std::vector<float>& out)
    {
        const int got = fifo.pop (scratch.data(), (int) scratch.size());
        if (got > 0)
        {
            const int n = juce::jmin (got, fftSize);
            std::copy (hist.begin() + n, hist.end(), hist.begin());
            std::copy (scratch.begin() + (got - n), scratch.begin() + got, hist.end() - n);
        }
        std::copy (hist.begin(), hist.end(), fftData.begin());
        std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
        window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
        fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

        for (size_t i = 0; i < out.size(); ++i)
        {
            const float level = juce::Decibels::gainToDecibels (fftData[i] / ((float) fftSize * 0.25f), floorDb);
            out[i] = level > out[i] ? level : juce::jmax (level, out[i] - 2.0f); // fast attack, slow release
        }
    }

    void timerCallback() override
    {
        analyse (proc.preFifo, history[0], db[0]);
        analyse (proc.postFifo, history[1], db[1]);
        repaint();
    }
};
