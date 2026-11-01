#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Presets internos: valores en las unidades reales de cada parametro.
// Los parametros que no aparecen se restauran a su valor por defecto ("Init" = todo transparente).
namespace
{
struct Preset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

const std::vector<Preset>& getPresets()
{
    static const std::vector<Preset> presets =
    {
        { "Init", {} },

        { "Pop Clean",
          { { "pitchKey", 0 }, { "pitchScale", 0 }, { "pitchSpeed", 45 }, { "pitchAmount", 80 },
            { "hpf", 90 }, { "eqLow", -1.0f }, { "eqMid", 1.5f }, { "eqHigh", 2.5f },
            { "deEss", -26 },
            { "compThresh", -20 }, { "compRatio", 3.0f }, { "compAttack", 12 }, { "compRelease", 140 }, { "compMakeup", 3 },
            { "rvSize", 35 }, { "rvMix", 14 }, { "dlTime", 180 }, { "dlFb", 20 }, { "dlMix", 6 } } },

        { "Trap Heavy Pitch",
          { { "pitchKey", 9 }, { "pitchScale", 1 }, { "pitchSpeed", 0 }, { "pitchAmount", 100 },
            { "hpf", 110 }, { "eqLow", -2.0f }, { "eqMid", 2.0f }, { "eqHigh", 3.0f },
            { "deEss", -30 },
            { "compThresh", -24 }, { "compRatio", 4.5f }, { "compAttack", 5 }, { "compRelease", 90 }, { "compMakeup", 5 },
            { "rvSize", 50 }, { "rvMix", 18 }, { "dlTime", 375 }, { "dlFb", 35 }, { "dlMix", 22 } } },

        { "Warm R&B",
          { { "pitchKey", 5 }, { "pitchScale", 0 }, { "pitchSpeed", 120 }, { "pitchAmount", 60 },
            { "hpf", 70 }, { "eqLow", 2.5f }, { "eqMid", -0.5f }, { "eqHigh", 1.0f },
            { "deEss", -24 },
            { "compThresh", -18 }, { "compRatio", 2.5f }, { "compAttack", 18 }, { "compRelease", 220 }, { "compMakeup", 3 },
            { "rvSize", 60 }, { "rvMix", 22 }, { "dlTime", 300 }, { "dlFb", 25 }, { "dlMix", 10 } } },
    };
    return presets;
}
} // namespace

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout Chain7stAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    using Fn = std::function<juce::String (float, int)>;

    auto addF = [&layout] (const char* id, const char* name, float lo, float hi, float step, float skew, float def, Fn fn)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (lo, hi, step, skew), def,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (std::move (fn))));
    };

    const Fn fDb  = [] (float v, int) { return juce::String (v, 1) + " dB"; };
    const Fn fPct = [] (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; };
    const Fn fMs  = [] (float v, int) { return juce::String (juce::roundToInt (v)) + " ms"; };
    const Fn fMs1 = [] (float v, int) { return juce::String (v, 1) + " ms"; };
    const Fn fHz  = [] (float v, int) { return juce::String (juce::roundToInt (v)) + " Hz"; };

    // --- PITCH ---
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "pitchKey", 1 }, "Key",
                juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "pitchScale", 1 }, "Scale",
                juce::StringArray { "Major", "Minor", "Chromatic" }, 0));
    addF ("pitchSpeed",  "Retune Speed", 0.f, 400.f, 1.f, 0.5f, 40.f,
          [] (float v, int) { return v < 0.5f ? juce::String ("INSTANT") : juce::String (juce::roundToInt (v)) + " ms"; });
    addF ("pitchAmount", "Pitch Amount", 0.f, 100.f, 1.f, 1.f, 0.f, fPct);

    // --- HPF + EQ ---
    addF ("hpf",    "HPF Freq", 20.f, 200.f, 1.f, 0.6f, 20.f, fHz);
    addF ("eqLow",  "EQ Low",  -12.f, 12.f, 0.1f, 1.f, 0.f, fDb);
    addF ("eqMid",  "EQ Mid",  -12.f, 12.f, 0.1f, 1.f, 0.f, fDb);
    addF ("eqHigh", "EQ High", -12.f, 12.f, 0.1f, 1.f, 0.f, fDb);

    // --- DE-ESSER (6.5 kHz fijo) ---
    addF ("deEss", "De-Esser Threshold", -50.f, 0.f, 0.1f, 1.f, 0.f,
          [] (float v, int) { return v > -0.5f ? juce::String ("OFF") : juce::String (v, 1) + " dB"; });

    // --- COMPRESOR ---
    addF ("compThresh",  "Comp Threshold", -40.f, 0.f, 0.1f, 1.f, 0.f, fDb);
    addF ("compRatio",   "Comp Ratio", 1.f, 20.f, 0.1f, 0.5f, 1.f, [] (float v, int) { return juce::String (v, 1) + " : 1"; });
    addF ("compAttack",  "Comp Attack", 0.1f, 100.f, 0.1f, 0.4f, 10.f, fMs1);
    addF ("compRelease", "Comp Release", 10.f, 1000.f, 1.f, 0.4f, 150.f, fMs);
    addF ("compMakeup",  "Comp Makeup", 0.f, 24.f, 0.1f, 1.f, 0.f, fDb);

    // --- ESPACIO ---
    addF ("rvSize", "Reverb Size", 0.f, 100.f, 1.f, 1.f, 40.f, fPct);
    addF ("rvMix",  "Reverb Mix",  0.f, 100.f, 1.f, 1.f, 0.f,  fPct);
    addF ("dlTime", "Delay Time",  20.f, 1000.f, 1.f, 0.5f, 250.f, fMs);
    addF ("dlFb",   "Delay Feedback", 0.f, 90.f, 1.f, 1.f, 30.f, fPct);
    addF ("dlMix",  "Delay Mix",   0.f, 100.f, 1.f, 1.f, 0.f,  fPct);

    return layout;
}

//==============================================================================
Chain7stAudioProcessor::Chain7stAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Chain7st", createParameterLayout())
{
    pKey    = apvts.getRawParameterValue ("pitchKey");
    pScale  = apvts.getRawParameterValue ("pitchScale");
    pSpeed  = apvts.getRawParameterValue ("pitchSpeed");
    pAmount = apvts.getRawParameterValue ("pitchAmount");
    pHpf    = apvts.getRawParameterValue ("hpf");
    pLow    = apvts.getRawParameterValue ("eqLow");
    pMid    = apvts.getRawParameterValue ("eqMid");
    pHigh   = apvts.getRawParameterValue ("eqHigh");
    pDeEss  = apvts.getRawParameterValue ("deEss");
    pThr    = apvts.getRawParameterValue ("compThresh");
    pRatio  = apvts.getRawParameterValue ("compRatio");
    pAtk    = apvts.getRawParameterValue ("compAttack");
    pRel    = apvts.getRawParameterValue ("compRelease");
    pMakeup = apvts.getRawParameterValue ("compMakeup");
    pRvSize = apvts.getRawParameterValue ("rvSize");
    pRvMix  = apvts.getRawParameterValue ("rvMix");
    pDlTime = apvts.getRawParameterValue ("dlTime");
    pDlFb   = apvts.getRawParameterValue ("dlFb");
    pDlMix  = apvts.getRawParameterValue ("dlMix");
}

bool Chain7stAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void Chain7stAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    pitch.prepare (sampleRate);
    hpf.reset(); lowShelf.reset(); midPeak.reset(); highShelf.reset();
    deEsser.prepare (sampleRate);
    comp.prepare (sampleRate);
    space.prepare (sampleRate, std::max (samplesPerBlock, 512));
    setLatencySamples (0);   // la latencia real del pitch shifter depende de la nota (aprox. 4-25 ms)
}

void Chain7stAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const int nCh = std::min (buffer.getNumChannels(), 2);
    const int n   = buffer.getNumSamples();
    if (nCh <= 0 || n <= 0) return;

    float* const* data = buffer.getArrayOfWritePointers();
    const double fs = getSampleRate();

    // 1) Pitch correction
    c7::PitchCorrector::Params pp;
    pp.key     = (int) pKey->load();
    pp.scale   = (int) pScale->load();
    pp.speedMs = pSpeed->load();
    pp.amount  = pAmount->load() * 0.01f;
    pitch.process (data, nCh, n, pp);

    // 2) HPF + EQ de 3 bandas (Low shelf 120 Hz / Mid peak 1.5 kHz / High shelf 9 kHz)
    hpf.highPass (fs, pHpf->load(), 0.7071);
    lowShelf.lowShelf (fs, 120.0, pLow->load());
    midPeak.peak (fs, 1500.0, 0.8, pMid->load());
    highShelf.highShelf (fs, 9000.0, pHigh->load());

    for (int ch = 0; ch < nCh; ++ch)
    {
        float* x = data[ch];
        for (int i = 0; i < n; ++i)
        {
            float s = hpf.process (x[i], ch);
            s = lowShelf.process (s, ch);
            s = midPeak.process (s, ch);
            s = highShelf.process (s, ch);
            x[i] = s;
        }
    }

    // 3) De-Esser
    deEsser.process (data, nCh, n, pDeEss->load());

    // 4) Compresor
    comp.setParams (pThr->load(), pRatio->load(), pAtk->load(), pRel->load(), pMakeup->load());
    comp.process (data, nCh, n);

    // 5) Espacio (reverb + delay)
    space.process (data, nCh, n,
                   pRvSize->load() * 0.01f, pRvMix->load() * 0.01f,
                   pDlTime->load(), pDlFb->load() * 0.01f, pDlMix->load() * 0.01f);
}

//==============================================================================
int Chain7stAudioProcessor::getNumPrograms() { return (int) getPresets().size(); }

int Chain7stAudioProcessor::getCurrentProgram()
{
    return juce::jlimit (0, getNumPrograms() - 1, (int) apvts.state.getProperty ("preset", 0));
}

const juce::String Chain7stAudioProcessor::getProgramName (int index)
{
    const auto& p = getPresets();
    return (index >= 0 && index < (int) p.size()) ? juce::String (p[(size_t) index].name) : juce::String();
}

void Chain7stAudioProcessor::setCurrentProgram (int index)
{
    const auto& presets = getPresets();
    if (index < 0 || index >= (int) presets.size()) return;

    for (auto* p : getParameters())
        p->setValueNotifyingHost (p->getDefaultValue());

    for (const auto& kv : presets[(size_t) index].values)
        if (auto* p = apvts.getParameter (kv.first))
            p->setValueNotifyingHost (p->convertTo0to1 (kv.second));

    apvts.state.setProperty ("preset", index, nullptr);
}

//==============================================================================
void Chain7stAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void Chain7stAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* Chain7stAudioProcessor::createEditor()
{
    return new Chain7stAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Chain7stAudioProcessor();
}
