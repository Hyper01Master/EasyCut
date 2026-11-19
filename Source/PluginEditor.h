#pragma once
#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "SpectrumDisplay.h"

class EasyCutEditor : public juce::AudioProcessorEditor
{
public:
    explicit EasyCutEditor (EasyCutProcessor&);
    ~EasyCutEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void updateReadouts();

    EasyCutProcessor& proc;
    EasyCutLookAndFeel lnf;
    SpectrumDisplay display;

    juce::Slider freqKnob, slopeKnob;
    juce::Label freqTitle, slopeTitle, freqValue, slopeValue;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> freqAtt, slopeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EasyCutEditor)
};
