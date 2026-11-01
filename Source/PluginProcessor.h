#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP.h"

//==============================================================================
// Chain7st - All-in-One Vocal Strip
// Cadena en serie: Pitch -> HPF + EQ 3 bandas -> De-Esser -> Compresor -> Espacio (Reverb + Delay)
class Chain7stAudioProcessor : public juce::AudioProcessor
{
public:
    Chain7stAudioProcessor();
    ~Chain7stAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Chain7st"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    // Los presets internos se exponen como "programas" (el host tambien los ve)
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

private:
    std::atomic<float> *pKey = nullptr, *pScale = nullptr, *pSpeed = nullptr, *pAmount = nullptr,
                       *pHpf = nullptr, *pLow = nullptr, *pMid = nullptr, *pHigh = nullptr,
                       *pDeEss = nullptr,
                       *pThr = nullptr, *pRatio = nullptr, *pAtk = nullptr, *pRel = nullptr, *pMakeup = nullptr,
                       *pRvSize = nullptr, *pRvMix = nullptr, *pDlTime = nullptr, *pDlFb = nullptr, *pDlMix = nullptr;

    c7::PitchCorrector pitch;
    c7::Biquad hpf, lowShelf, midPeak, highShelf;
    c7::DeEsser deEsser;
    c7::Compressor comp;
    c7::SpaceFx space;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Chain7stAudioProcessor)
};
