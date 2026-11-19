#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace easycut;

const juce::StringArray& EasyCutProcessor::slopeNames()
{
    static const juce::StringArray names { "12 dB/oct", "24 dB/oct", "48 dB/oct", "72 dB/oct", "Brick Wall" };
    return names;
}

juce::AudioProcessorValueTreeState::ParameterLayout EasyCutProcessor::createLayout()
{
    juce::NormalisableRange<float> freqRange (30.0f, 500.0f, 0.01f);
    freqRange.setSkewForCentre (std::sqrt (30.0f * 500.0f));

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "freq", 1 }, "Freq", freqRange, 120.0f,
                                                             juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "slope", 1 }, "Slope", slopeNames(), 1));
    return layout;
}

EasyCutProcessor::EasyCutProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    freqParam = apvts.getRawParameterValue ("freq");
    slopeParam = apvts.getRawParameterValue ("slope");
    startTimerHz (30);
}

EasyCutProcessor::~EasyCutProcessor() { stopTimer(); }

bool EasyCutProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void EasyCutProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentRate = sampleRate;
    freq.reset (sampleRate, 0.03);
    freq.setCurrentAndTargetValue (freqParam->load());
    for (auto& s : sections) s.reset();
    lastSlope = -1;
    needsUpdate = true;

    convolution.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 2 });
    irDirty = true;
}

void EasyCutProcessor::pushMono (const juce::AudioBuffer<float>& buffer, SampleFifo& fifo)
{
    std::array<float, 512> tmp;
    const int numCh = buffer.getNumChannels();
    const int total = buffer.getNumSamples();
    for (int start = 0; start < total; start += (int) tmp.size())
    {
        const int len = juce::jmin ((int) tmp.size(), total - start);
        for (int i = 0; i < len; ++i)
        {
            float sum = 0.0f;
            for (int ch = 0; ch < numCh; ++ch) sum += buffer.getReadPointer (ch)[start + i];
            tmp[(size_t) i] = sum / (float) juce::jmax (1, numCh);
        }
        fifo.push (tmp.data(), len);
    }
}

void EasyCutProcessor::updateCoefficients (float f, int n)
{
    const double sr = currentRate.load();
    for (int i = 0; i < n; ++i)
        sections[(size_t) i].setHighPass (sr, f, butterQ (2 * n, i));
}

void EasyCutProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numCh = juce::jmin (2, buffer.getNumChannels());
    for (int ch = numCh; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    pushMono (buffer, preFifo);

    const int slope = juce::jlimit (0, 4, juce::roundToInt (slopeParam->load()));
    const float target = freqParam->load();
    if (slope != lastSlope)
    {
        for (auto& s : sections) s.reset();
        needsUpdate = true;
        lastSlope = slope;
    }
    if (target != freq.getTargetValue())
        freq.setTargetValue (target);

    if (slope == kBrickWall)
    {
        freq.skip (numSamples);
        juce::dsp::AudioBlock<float> block (buffer);
        auto sub = block.getSubsetChannelBlock (0, (size_t) numCh);
        convolution.process (juce::dsp::ProcessContextReplacing<float> (sub));
    }
    else
    {
        const int n = sectionsFor (slope);
        float* data[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };
        constexpr int chunk = 16;
        for (int start = 0; start < numSamples; start += chunk)
        {
            const int len = juce::jmin (chunk, numSamples - start);
            const bool moving = freq.isSmoothing();
            const float f = freq.skip (len);
            if (moving || needsUpdate)
            {
                updateCoefficients (f, n);
                needsUpdate = false;
            }
            for (int ch = 0; ch < numCh; ++ch)
                for (int i = 0; i < len; ++i)
                {
                    float x = data[ch][start + i];
                    for (int s = 0; s < n; ++s)
                        x = sections[(size_t) s].process (ch, x);
                    data[ch][start + i] = x;
                }
        }
    }

    pushMono (buffer, postFifo);
}

// Message thread: keeps latency and the brick wall FIR up to date
void EasyCutProcessor::timerCallback()
{
    const int slope = juce::jlimit (0, 4, juce::roundToInt (slopeParam->load()));
    const float f = freqParam->load();
    const double sr = currentRate.load();
    const int latency = slope == kBrickWall ? firHalfLength (sr) : 0;
    if (latency != getLatencySamples())
        setLatencySamples (latency);

    if (slope == kBrickWall && (irDirty.load() || f != lastIrFreq))
    {
        buildImpulseResponse (f);
        lastIrFreq = f;
        irDirty = false;
    }
}

// Linear-phase high-pass: delta - windowed-sinc low-pass (Blackman window)
void EasyCutProcessor::buildImpulseResponse (float f)
{
    const double sr = currentRate.load();
    const int M = firHalfLength (sr);
    const int N = 2 * M + 1;
    const double wc = 2.0 * juce::MathConstants<double>::pi * (double) f / sr;
    const double pi = juce::MathConstants<double>::pi;

    juce::AudioBuffer<float> ir (1, N);
    auto* h = ir.getWritePointer (0);
    for (int n = 0; n < N; ++n)
    {
        const double k = n - M;
        const double lp = k == 0.0 ? wc / pi : std::sin (wc * k) / (pi * k);
        const double win = 0.42 - 0.5 * std::cos (2.0 * pi * n / (N - 1)) + 0.08 * std::cos (4.0 * pi * n / (N - 1));
        h[n] = (float) ((k == 0.0 ? 1.0 : 0.0) - lp * win);
    }

    convolution.loadImpulseResponse (std::move (ir), sr, juce::dsp::Convolution::Stereo::no,
                                     juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
}

juce::AudioProcessorEditor* EasyCutProcessor::createEditor() { return new EasyCutEditor (*this); }

void EasyCutProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void EasyCutProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EasyCutProcessor(); }
