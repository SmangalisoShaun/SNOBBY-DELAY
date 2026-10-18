#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    const float kBeats[12] = { 2.0f, 3.0f, 4.0f / 3.0f,
                               1.0f, 1.5f, 2.0f / 3.0f,
                               0.5f, 0.75f, 1.0f / 3.0f,
                               0.25f, 0.375f, 1.0f / 6.0f };

    const char* kNoteNames[12] = { "1/2", "1/2D", "1/2T",
                                   "1/4", "1/4D", "1/4T",
                                   "1/8", "1/8D", "1/8T",
                                   "1/16", "1/16D", "1/16T" };

    struct Preset
    {
        const char* name;
        float delayMs;
        int mode;      // 0 = MS, 1 = BPM, 2 = HOST
        int note;
        float feedback, mix, modDepth, modRate, hpf, lpf;
        int analog;
        bool ping, duck;
        float thresh, amount, attack, release;
    };

    const Preset kPresets[] =
    {
        { "Init",            350.0f, 0, 6, 35.0f, 30.0f,  0.0f, 0.8f,  20.0f, 20000.0f, 0, false, false, -30.0f, 70.0f, 10.0f, 250.0f },
        { "Slapback",        110.0f, 0, 6, 12.0f, 22.0f,  0.0f, 0.8f, 120.0f,  9000.0f, 2, false, false, -30.0f, 70.0f, 10.0f, 250.0f },
        { "Vocal Throw 1/4",   0.0f, 2, 3, 40.0f, 28.0f, 10.0f, 0.6f, 150.0f,  7500.0f, 1, false, true,  -32.0f, 80.0f,  8.0f, 300.0f },
        { "Ping Pong 1/8",     0.0f, 2, 6, 45.0f, 30.0f,  5.0f, 0.7f, 200.0f,  8000.0f, 1, true,  true,  -30.0f, 55.0f, 10.0f, 220.0f },
        { "Dub Echo 1/8D",     0.0f, 2, 7, 62.0f, 35.0f, 12.0f, 0.4f, 250.0f,  3500.0f, 3, false, false, -30.0f, 70.0f, 10.0f, 250.0f },
        { "Chorus Tape",      45.0f, 0, 6, 25.0f, 40.0f, 45.0f, 0.5f,  80.0f,  6000.0f, 2, false, false, -30.0f, 70.0f, 10.0f, 250.0f },
        { "Wide Dotted 1/4D",  0.0f, 2, 4, 38.0f, 26.0f,  8.0f, 0.6f, 150.0f,  9000.0f, 1, true,  true,  -28.0f, 70.0f, 12.0f, 280.0f }
    };

    constexpr int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));
}

const char* SnobbyDelayProcessor::noteName (int index)
{
    return kNoteNames[juce::jlimit (0, 11, index)];
}

juce::AudioProcessorValueTreeState::ParameterLayout SnobbyDelayProcessor::createLayout()
{
    using Range = juce::NormalisableRange<float>;
    using PID = juce::ParameterID;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (PID { "delayMs", 1 }, "Delay",
                    Range (1.0f, 3500.0f, 0.1f, 0.4f), 350.0f,
                    juce::AudioParameterFloatAttributes().withLabel (" ms")));

    layout.add (std::make_unique<juce::AudioParameterChoice> (PID { "timeMode", 1 }, "Time Mode",
                    juce::StringArray { "MS", "BPM", "HOST" }, 0));

    layout.add (std::make_unique<juce::AudioParameterChoice> (PID { "noteDiv", 1 }, "Note",
                    juce::StringArray (kNoteNames, 12), 6));

    layout.add (std::make_unique<juce::AudioParameterFloat> (PID { "manualBpm", 1 }, "Tempo",
                    Range (40.0f, 240.0f, 0.1f), 120.0f,
                    juce::AudioParameterFloatAttributes().withLabel (" BPM")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (PID { "feedback", 1 }, "Feedback",
                    Range (0.0f, 100.0f, 0.1f), 35.0f,
                    juce::AudioParameterFloatAttributes().withLabel (" %")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (PID { "mix", 1 }, "Dry/Wet",
                    Range (0.0f, 100.0f, 0.1f), 30.0f,
                    juce::AudioParameterFloatAttributes().withLabel (" %")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (PID { "output", 1 }, "Output",
                    Range (-18.0f, 18.0f, 0.1f), 0.0f,
                    juce::AudioParameterFloatAttributes().withLabel (" dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (PID { "modDepth", 1 }, "Mod Depth",
                    Range (0.0f, 100.0f, 0.1f), 0.0f,
                    juce::AudioParameterFloatAttributes().withLabel (" %")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (PID { "modRate", 1 }, "Mod Rate",
                    Range (0.1f, 10.0f, 0.01f, 0.5f), 0.8f,
                    juce::AudioParameterFloatAttributes().withLabel (" Hz")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (PID { "hpf", 1 }, "High Pass",
                    Range (20.0f, 2000.0f, 1.0f, 0.4f), 20.0f,
                    juce::AudioParameterFloatAttributes().withLabel (" Hz")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (PID { "lpf", 1 }, "Low Pass",
                    Range (1000.0f, 20000.0f, 1.0f, 0.4f), 20000.0f,
                    juce::AudioParameterFloatAttributes().withLabel (" Hz")));

    layout.add (std::make_unique<juce::AudioParameterInt> (PID { "analog", 1 }, "Analog", 0, 3, 0));

    layout.add (std::make_unique<juce::AudioParameterBool> (PID { "pingpong", 1 }, "Ping-Pong", false));

    layout.add (std::make_unique<juce::AudioParameterBool> (PID { "duckOn", 1 }, "Duck", false));

    layout.add (std::make_unique<juce::AudioParameterChoice> (PID { "duckMode", 1 }, "Duck Source",
                    juce::StringArray { "AUTO", "SIDECHAIN" }, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (PID { "duckThresh", 1 }, "Duck Threshold",
                    Range (-60.0f, 0.0f, 0.1f), -30.0f,
                    juce::AudioParameterFloatAttributes().withLabel (" dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (PID { "duckAmount", 1 }, "Duck Amount",
                    Range (0.0f, 100.0f, 0.1f), 70.0f,
                    juce::AudioParameterFloatAttributes().withLabel (" %")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (PID { "duckAttack", 1 }, "Duck Attack",
                    Range (1.0f, 200.0f, 0.1f, 0.5f), 10.0f,
                    juce::AudioParameterFloatAttributes().withLabel (" ms")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (PID { "duckRelease", 1 }, "Duck Release",
                    Range (10.0f, 2000.0f, 1.0f, 0.5f), 250.0f,
                    juce::AudioParameterFloatAttributes().withLabel (" ms")));

    return layout;
}

SnobbyDelayProcessor::SnobbyDelayProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
          .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pDelayMs     = apvts.getRawParameterValue ("delayMs");
    pTimeMode    = apvts.getRawParameterValue ("timeMode");
    pNote        = apvts.getRawParameterValue ("noteDiv");
    pBpm         = apvts.getRawParameterValue ("manualBpm");
    pFeedback    = apvts.getRawParameterValue ("feedback");
    pMix         = apvts.getRawParameterValue ("mix");
    pOutput      = apvts.getRawParameterValue ("output");
    pModDepth    = apvts.getRawParameterValue ("modDepth");
    pModRate     = apvts.getRawParameterValue ("modRate");
    pHpf         = apvts.getRawParameterValue ("hpf");
    pLpf         = apvts.getRawParameterValue ("lpf");
    pAnalog      = apvts.getRawParameterValue ("analog");
    pPing        = apvts.getRawParameterValue ("pingpong");
    pDuckOn      = apvts.getRawParameterValue ("duckOn");
    pDuckMode    = apvts.getRawParameterValue ("duckMode");
    pDuckThresh  = apvts.getRawParameterValue ("duckThresh");
    pDuckAmount  = apvts.getRawParameterValue ("duckAmount");
    pDuckAttack  = apvts.getRawParameterValue ("duckAttack");
    pDuckRelease = apvts.getRawParameterValue ("duckRelease");
}

bool SnobbyDelayProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != out)
        return false;

    const auto sc = layouts.getChannelSet (true, 1);
    return sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo();
}

void SnobbyDelayProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    lineSize = (int) (sampleRate * 4.0) + 8;
    lineL.assign ((size_t) lineSize, 0.0f);
    lineR.assign ((size_t) lineSize, 0.0f);
    writePos = 0;

    delaySm.reset (sampleRate, 0.12);
    fbSm.reset (sampleRate, 0.05);
    mixSm.reset (sampleRate, 0.05);
    outSm.reset (sampleRate, 0.05);

    delaySm.setCurrentAndTargetValue (juce::jlimit (1.0f, 3500.0f, pDelayMs->load()) * 0.001f * (float) sampleRate);
    fbSm.setCurrentAndTargetValue (pFeedback->load() / 100.0f * 0.98f);
    mixSm.setCurrentAndTargetValue (pMix->load() / 100.0f);
    outSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOutput->load()));

    for (int c = 0; c < 2; ++c)
    {
        hpf[c].reset();  hpf[c].setCutoff (pHpf->load(), sampleRate);
        lpf[c].reset();  lpf[c].setCutoff (pLpf->load(), sampleRate);
        dark[c].reset(); dark[c].setCutoff (9000.0f, sampleRate);
    }

    lfoPhase = 0.0f;
    wowPhase = 0.0f;
    detEnv = 0.0f;
    duckGain = 1.0f;
}

float SnobbyDelayProcessor::readLine (const std::vector<float>& line, float delaySamples) const
{
    float rp = (float) writePos - delaySamples;
    while (rp < 0.0f)
        rp += (float) lineSize;

    int i0 = (int) rp;
    const float frac = rp - (float) i0;
    if (i0 >= lineSize) i0 -= lineSize;
    int i1 = i0 + 1;
    if (i1 >= lineSize) i1 -= lineSize;

    return line[(size_t) i0] + frac * (line[(size_t) i1] - line[(size_t) i0]);
}

void SnobbyDelayProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto mainBuf = getBusBuffer (buffer, true, 0);
    const int nCh = mainBuf.getNumChannels();
    const int n = buffer.getNumSamples();
    if (nCh == 0 || n == 0 || lineSize == 0)
        return;

    // Optional sidechain input for ducking
    const float* scL = nullptr;
    const float* scR = nullptr;
    if (pDuckMode->load() > 0.5f)
    {
        if (auto* scBus = getBus (true, 1))
        {
            if (scBus->isEnabled())
            {
                auto sc = getBusBuffer (buffer, true, 1);
                if (sc.getNumChannels() > 0)
                {
                    scL = sc.getReadPointer (0);
                    scR = sc.getNumChannels() > 1 ? sc.getReadPointer (1) : scL;
                }
            }
        }
    }

    // Delay time (ms, manual BPM or host BPM)
    const int mode = (int) pTimeMode->load();
    double bpm = pBpm->load();
    if (mode == 2)
    {
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto b = pos->getBpm())
                    bpm = *b;
    }
    bpm = juce::jlimit (20.0, 400.0, bpm);

    float ms = pDelayMs->load();
    if (mode != 0)
        ms = (float) (60000.0 / bpm) * kBeats[juce::jlimit (0, 11, (int) pNote->load())];
    ms = juce::jlimit (1.0f, 3500.0f, ms);
    displayDelayMs.store (ms);

    delaySm.setTargetValue (ms * 0.001f * (float) sr);
    fbSm.setTargetValue (pFeedback->load() / 100.0f * 0.98f);
    mixSm.setTargetValue (pMix->load() / 100.0f);
    outSm.setTargetValue (juce::Decibels::decibelsToGain (pOutput->load()));

    for (int c = 0; c < 2; ++c)
    {
        hpf[c].setCutoff (pHpf->load(), sr);
        lpf[c].setCutoff (pLpf->load(), sr);
    }
    const bool useHpf = pHpf->load() > 21.0f;     // fully left = off
    const bool useLpf = pLpf->load() < 19500.0f;  // fully right = off

    // Analog colour: 0 = clean digital, 1 to 3 = darker repeats, saturation and tape wobble
    const int analog = juce::jlimit (0, 3, (int) pAnalog->load());
    const float drive = 1.0f + (float) analog * 0.4f;
    const float darkHz = analog == 1 ? 9000.0f : (analog == 2 ? 6000.0f : 4000.0f);
    const float wowDepth = (float) analog * 0.0003f * (float) sr;
    if (analog > 0)
        for (int c = 0; c < 2; ++c)
            dark[c].setCutoff (darkHz, sr);

    // Modulation
    const float modDepthSamples = pModDepth->load() / 100.0f * 0.006f * (float) sr;
    const float lfoInc = juce::MathConstants<float>::twoPi * pModRate->load() / (float) sr;
    const float wowInc = juce::MathConstants<float>::twoPi * 0.7f / (float) sr;

    // Ducking
    const bool duckOn = pDuckOn->load() > 0.5f;
    const float thrLin = juce::Decibels::decibelsToGain (pDuckThresh->load());
    const float duckFloor = 1.0f - pDuckAmount->load() / 100.0f;
    const float atkCoef = std::exp (-1.0f / (pDuckAttack->load() * 0.001f * (float) sr));
    const float relCoef = std::exp (-1.0f / (pDuckRelease->load() * 0.001f * (float) sr));
    const float detRelCoef = std::exp (-1.0f / (0.02f * (float) sr));

    const bool ping = pPing->load() > 0.5f;

    float* chL = mainBuf.getWritePointer (0);
    float* chR = nCh > 1 ? mainBuf.getWritePointer (1) : nullptr;

    float peak = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float inL = chL[i];
        const float inR = chR != nullptr ? chR[i] : inL;

        // Ducking detector: follows the vocal (main input) or the sidechain input
        const float srcL = scL != nullptr ? scL[i] : inL;
        const float srcR = scR != nullptr ? scR[i] : inR;
        const float det = juce::jmax (std::abs (srcL), std::abs (srcR));
        detEnv = det > detEnv ? det : det + (detEnv - det) * detRelCoef;

        const float targetG = (duckOn && detEnv > thrLin) ? duckFloor : 1.0f;
        const float gCoef = targetG < duckGain ? atkCoef : relCoef;
        duckGain = targetG + (duckGain - targetG) * gCoef;

        // Delay lines with modulation
        const float baseD = delaySm.getNextValue();
        const float fb = fbSm.getNextValue();
        const float mixv = mixSm.getNextValue();
        const float outG = outSm.getNextValue();

        const float lfoL = std::sin (lfoPhase);
        const float lfoR = std::sin (lfoPhase + juce::MathConstants<float>::halfPi);
        const float wow = std::sin (wowPhase) * wowDepth;

        lfoPhase += lfoInc;
        if (lfoPhase > juce::MathConstants<float>::twoPi) lfoPhase -= juce::MathConstants<float>::twoPi;
        wowPhase += wowInc;
        if (wowPhase > juce::MathConstants<float>::twoPi) wowPhase -= juce::MathConstants<float>::twoPi;

        const float maxD = (float) lineSize - 4.0f;
        const float dL = juce::jlimit (1.0f, maxD, baseD + lfoL * modDepthSamples + wow);
        const float dR = juce::jlimit (1.0f, maxD, baseD + lfoR * modDepthSamples - wow);

        const float wetRawL = readLine (lineL, dL);
        const float wetRawR = readLine (lineR, dR);

        // Filters (high pass then low pass) shape both the repeats and the wet signal.
        // They keep running when bypassed so there is no click when they are switched in.
        const float hpLowL = hpf[0].lowpass (wetRawL);
        const float hpLowR = hpf[1].lowpass (wetRawR);
        const float hL = useHpf ? wetRawL - hpLowL : wetRawL;
        const float hR = useHpf ? wetRawR - hpLowR : wetRawR;
        const float lpOutL = lpf[0].lowpass (hL);
        const float lpOutR = lpf[1].lowpass (hR);
        const float fL = useLpf ? lpOutL : hL;
        const float fR = useLpf ? lpOutR : hR;

        // Feedback path (analog mode darkens and saturates each repeat)
        float sL = fL, sR = fR;
        if (analog > 0)
        {
            sL = std::tanh (dark[0].lowpass (fL) * drive) / drive;
            sR = std::tanh (dark[1].lowpass (fR) * drive) / drive;
        }

        if (ping)
        {
            const float mono = 0.5f * (inL + inR);
            lineL[(size_t) writePos] = mono + fb * sR;
            lineR[(size_t) writePos] = fb * sL;
        }
        else
        {
            lineL[(size_t) writePos] = inL + fb * sL;
            lineR[(size_t) writePos] = inR + fb * sR;
        }

        if (++writePos >= lineSize)
            writePos = 0;

        // Duck the repeats, then mix
        const float wetL = fL * duckGain;
        const float wetR = fR * duckGain;
        peak = juce::jmax (peak, std::abs (wetL), std::abs (wetR));

        const float dryG = juce::jmin (1.0f, 2.0f * (1.0f - mixv));
        const float wetG = juce::jmin (1.0f, 2.0f * mixv);

        chL[i] = (inL * dryG + wetL * wetG) * outG;
        if (chR != nullptr)
            chR[i] = (inR * dryG + wetR * wetG) * outG;
    }

    // Meter (wet level, after ducking) and duck indicator
    const float peakDb = juce::Decibels::gainToDecibels (peak, -60.0f);
    const float decay = 20.0f * (float) n / (float) sr;
    meterDb.store (juce::jmax (peakDb, meterDb.load() - decay));
    duckGainShown.store (duckGain);
}

juce::AudioProcessorEditor* SnobbyDelayProcessor::createEditor()
{
    return new SnobbyDelayEditor (*this);
}

int SnobbyDelayProcessor::getNumPrograms()
{
    return kNumPresets;
}

const juce::String SnobbyDelayProcessor::getProgramName (int index)
{
    if (index >= 0 && index < kNumPresets)
        return kPresets[index].name;
    return {};
}

void SnobbyDelayProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= kNumPresets)
        return;

    currentPreset = index;
    const auto& p = kPresets[index];

    auto set = [this] (const char* id, float value)
    {
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (value));
    };

    set ("delayMs", p.delayMs > 0.0f ? p.delayMs : 350.0f);
    set ("timeMode", (float) p.mode);
    set ("noteDiv", (float) p.note);
    set ("feedback", p.feedback);
    set ("mix", p.mix);
    set ("modDepth", p.modDepth);
    set ("modRate", p.modRate);
    set ("hpf", p.hpf);
    set ("lpf", p.lpf);
    set ("analog", (float) p.analog);
    set ("pingpong", p.ping ? 1.0f : 0.0f);
    set ("duckOn", p.duck ? 1.0f : 0.0f);
    set ("duckMode", 0.0f);
    set ("duckThresh", p.thresh);
    set ("duckAmount", p.amount);
    set ("duckAttack", p.attack);
    set ("duckRelease", p.release);
}

void SnobbyDelayProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SnobbyDelayProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SnobbyDelayProcessor();
}