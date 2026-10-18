#pragma once
#include "PluginProcessor.h"
#include <memory>
#include <vector>

inline constexpr int kDesignW = 920;
inline constexpr int kDesignH = 490;

class SnobbyLook : public juce::LookAndFeel_V4
{
public:
    SnobbyLook();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class DelayPanel : public juce::Component,
                   private juce::Timer
{
public:
    explicit DelayPanel (SnobbyDelayProcessor&);
    ~DelayPanel() override;

    void paint (juce::Graphics&) override;

private:
    struct Knob
    {
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        juce::String label, minText, maxText;
        juce::Rectangle<int> area;
    };

    void addKnob (const juce::String& paramId, const juce::String& label,
                  const juce::String& minText, const juce::String& maxText,
                  int x, int y, int size);
    void setTimeMode (int mode);
    void tap();
    void timerCallback() override;

    SnobbyDelayProcessor& proc;
    SnobbyLook look;

    std::vector<std::unique_ptr<Knob>> knobs;

    juce::TextButton tapButton { "TAP" };
    juce::TextButton bpmButton { "BPM" };
    juce::TextButton hostButton { "HOST" };
    juce::TextButton msButton { "MS" };
    juce::TextButton pingButton { "PING-PONG" };
    juce::TextButton duckButton { "DUCK" };
    juce::TextButton duckModeButton { "AUTO" };
    juce::ComboBox presetBox;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> pingAttachment, duckAttachment;

    double lastTapMs = 0.0;
    std::vector<double> tapIntervals;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayPanel)
};

class SnobbyDelayEditor : public juce::AudioProcessorEditor
{
public:
    explicit SnobbyDelayEditor (SnobbyDelayProcessor&);
    ~SnobbyDelayEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    DelayPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnobbyDelayEditor)
};