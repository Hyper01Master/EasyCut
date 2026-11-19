#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Dsp.h"

class EasyCutProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    EasyCutProcessor();
    ~EasyCutProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "EasyCut"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static const juce::StringArray& slopeNames();

    juce::AudioProcessorValueTreeState apvts;

    // analyser feeds (mono mix before / after the filter)
    easycut::SampleFifo preFifo, postFifo;

private:
    void timerCallback() override;
    void buildImpulseResponse (float freq);
    void pushMono (const juce::AudioBuffer<float>&, easycut::SampleFifo&);
    void updateCoefficients (float freq, int numSections);

    std::atomic<float>* freqParam = nullptr;
    std::atomic<float>* slopeParam = nullptr;

    std::atomic<double> currentRate { 44100.0 };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> freq;
    std::array<easycut::Biquad, easycut::kMaxSections> sections;
    int lastSlope = -1;
    bool needsUpdate = true;

    juce::dsp::Convolution convolution;
    std::atomic<bool> irDirty { true };
    float lastIrFreq = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EasyCutProcessor)
};
