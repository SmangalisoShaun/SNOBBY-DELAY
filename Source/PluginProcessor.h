#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <vector>

class SnobbyDelayProcessor : public juce::AudioProcessor
{
public:
    SnobbyDelayProcessor();
    ~SnobbyDelayProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Snobby Delay"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static const char* noteName (int index);

    juce::AudioProcessorValueTreeState apvts;

    // Read by the editor
    std::atomic<float> displayDelayMs { 350.0f };
    std::atomic<float> meterDb { -60.0f };
    std::atomic<float> duckGainShown { 1.0f };

private:
    struct TwoPole
    {
        float z1 = 0.0f, z2 = 0.0f, a = 0.1f;

        void reset() { z1 = z2 = 0.0f; }
        void setCutoff (float hz, double sampleRate)
        {
            a = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * hz / (float) sampleRate);
        }
        float lowpass (float x)
        {
            z1 += a * (x - z1);
            z2 += a * (z1 - z2);
            return z2;
        }
    };

    float readLine (const std::vector<float>& line, float delaySamples) const;

    std::vector<float> lineL, lineR;
    int writePos = 0;
    int lineSize = 0;
    double sr = 44100.0;

    juce::SmoothedValue<float> delaySm, fbSm, mixSm, outSm;
    TwoPole hpf[2], lpf[2], dark[2];

    float lfoPhase = 0.0f, wowPhase = 0.0f;
    float detEnv = 0.0f, duckGain = 1.0f;
    int currentPreset = 0;

    std::atomic<float>* pDelayMs = nullptr;
    std::atomic<float>* pTimeMode = nullptr;
    std::atomic<float>* pNote = nullptr;
    std::atomic<float>* pBpm = nullptr;
    std::atomic<float>* pFeedback = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pModDepth = nullptr;
    std::atomic<float>* pModRate = nullptr;
    std::atomic<float>* pHpf = nullptr;
    std::atomic<float>* pLpf = nullptr;
    std::atomic<float>* pAnalog = nullptr;
    std::atomic<float>* pPing = nullptr;
    std::atomic<float>* pDuckOn = nullptr;
    std::atomic<float>* pDuckMode = nullptr;
    std::atomic<float>* pDuckThresh = nullptr;
    std::atomic<float>* pDuckAmount = nullptr;
    std::atomic<float>* pDuckAttack = nullptr;
    std::atomic<float>* pDuckRelease = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnobbyDelayProcessor)
};