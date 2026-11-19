#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>
#include <complex>

namespace easycut
{
constexpr int kBrickWall = 4;   // slope choice index of the FIR brick wall
constexpr int kMaxSections = 6; // 72 dB/oct = 12th order = 6 biquads

inline int sectionsFor (int slope)
{
    static constexpr int s[] = { 1, 2, 4, 6, 0 };
    return s[juce::jlimit (0, 4, slope)];
}

// Q of section i of an even-order Butterworth filter
inline double butterQ (int order, int i)
{
    return 1.0 / (2.0 * std::cos (juce::MathConstants<double>::pi * (2 * i + 1) / (2.0 * order)));
}

// Linear-phase FIR half length (= latency in samples)
inline int firHalfLength (double sampleRate) { return sampleRate > 60000.0 ? 4096 : 2048; }

struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1[2] = {}, z2[2] = {};

    void setHighPass (double sr, double fc, double q)
    {
        const double w0 = 2.0 * juce::MathConstants<double>::pi * fc / sr;
        const double c = std::cos (w0), s = std::sin (w0), alpha = s / (2.0 * q);
        const double a0 = 1.0 + alpha;
        b0 = (1.0 + c) * 0.5 / a0;
        b1 = -(1.0 + c) / a0;
        b2 = b0;
        a1 = -2.0 * c / a0;
        a2 = (1.0 - alpha) / a0;
    }

    float process (int ch, float x) noexcept
    {
        const double y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return (float) y;
    }

    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0.0; }

    double magnitude (double sr, double f) const
    {
        const double w = 2.0 * juce::MathConstants<double>::pi * f / sr;
        const auto p1 = std::polar (1.0, -w), p2 = std::polar (1.0, -2.0 * w);
        return std::abs ((b0 + b1 * p1 + b2 * p2) / (1.0 + a1 * p1 + a2 * p2));
    }
};

// Magnitude response used by the editor curve
inline double highPassMagnitude (double sr, double fc, int slope, double f)
{
    if (slope == kBrickWall)
        return 1.0 / std::sqrt (1.0 + std::pow (fc / f, 96.0));

    const int n = sectionsFor (slope);
    double mag = 1.0;
    for (int i = 0; i < n; ++i)
    {
        Biquad b;
        b.setHighPass (sr, fc, butterQ (2 * n, i));
        mag *= b.magnitude (sr, f);
    }
    return mag;
}

// Single-producer / single-consumer sample FIFO for the analyser
struct SampleFifo
{
    static constexpr int capacity = 1 << 15;
    juce::AbstractFifo fifo { capacity };
    std::array<float, capacity> data {};

    void push (const float* src, int n)
    {
        int s1, z1, s2, z2;
        fifo.prepareToWrite (n, s1, z1, s2, z2);
        std::copy (src, src + z1, data.begin() + s1);
        std::copy (src + z1, src + z1 + z2, data.begin() + s2);
        fifo.finishedWrite (z1 + z2);
    }

    int pop (float* dst, int maxN)
    {
        int s1, z1, s2, z2;
        fifo.prepareToRead (maxN, s1, z1, s2, z2);
        std::copy (data.begin() + s1, data.begin() + s1 + z1, dst);
        std::copy (data.begin() + s2, data.begin() + s2 + z2, dst + z1);
        fifo.finishedRead (z1 + z2);
        return z1 + z2;
    }
};
} // namespace easycut
