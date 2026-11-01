#pragma once

#include "PluginProcessor.h"
#include "LookAndFeel.h"

//==============================================================================
class Chain7stAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit Chain7stAudioProcessorEditor (Chain7stAudioProcessor&);
    ~Chain7stAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label caption;
        std::unique_ptr<SliderAttachment> attachment;   // se declara al final: se destruye primero
    };

    struct Choice
    {
        juce::ComboBox box;
        juce::Label caption;
        std::unique_ptr<ComboBoxAttachment> attachment;
    };

    void setupKnob (Knob&, const juce::String& paramId, const juce::String& captionText);
    void setupChoice (Choice&, const juce::String& paramId, const juce::String& captionText);
    static void placeKnob (Knob&, juce::Rectangle<int> cell);

    Chain7stAudioProcessor& proc;
    Chain7stLookAndFeel laf;   // declarado antes que los componentes para que viva mas que ellos

    Choice keyChoice, scaleChoice;
    Knob speed, amount;                       // PITCH
    Knob hpf, eqLow, eqMid, eqHigh;           // EQ
    Knob deEss, thr, ratio, attack, release, makeup;   // DYNAMICS
    Knob rvSize, rvMix, dlTime, dlFb, dlMix;  // SPACE

    juce::ComboBox presetBox;
    juce::Label presetCaption;

    juce::Rectangle<int> pitchPanel, eqPanel, dynPanel, spacePanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Chain7stAudioProcessorEditor)
};
