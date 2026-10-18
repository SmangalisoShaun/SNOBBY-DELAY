#include "PluginEditor.h"
#include <cmath>

namespace
{
    const juce::Colour ink      (0xff2a1a0a);
    const juce::Colour cream    (0xffe8d9ab);
    const juce::Colour amber    (0xffffa51f);
    const juce::Colour blackBox (0xff120d08);

    void useFont (juce::Graphics& g, float size, bool bold = false, float kerning = 0.0f)
    {
        g.setFont (size);
        juce::Font f = g.getCurrentFont();
        if (bold)           f = f.boldened();
        if (kerning > 0.0f) f = f.withExtraKerningFactor (kerning);
        g.setFont (f);
    }
}

// ============================================================ look and feel

SnobbyLook::SnobbyLook()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff15110d));
    setColour (juce::ComboBox::textColourId, amber);
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff6b4423));
    setColour (juce::ComboBox::arrowColourId, cream);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff15110d));
    setColour (juce::PopupMenu::textColourId, cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, amber);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
}

void SnobbyLook::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                   float sliderPos, float startAngle, float endAngle,
                                   juce::Slider&)
{
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    const float size = juce::jmin (b.getWidth(), b.getHeight());
    const auto c = b.getCentre();
    const float R = size * 0.5f;

    // Tick marks
    g.setColour (ink);
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
        const float s = std::sin (a), co = std::cos (a);
        const float r1 = R * 0.86f;
        const float r2 = R * (i % 5 == 0 ? 1.0f : 0.94f);
        g.drawLine (c.x + s * r1, c.y - co * r1, c.x + s * r2, c.y - co * r2, i % 5 == 0 ? 1.8f : 1.2f);
    }

    // Silver ring
    const float ringR = R * 0.80f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeeeeee), c.x - ringR * 0.6f, c.y - ringR * 0.8f,
                                             juce::Colour (0xff6a6a6a), c.x + ringR * 0.6f, c.y + ringR * 0.8f, false));
    g.fillEllipse (c.x - ringR, c.y - ringR, ringR * 2.0f, ringR * 2.0f);

    // Black body
    const float bodyR = R * 0.70f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4d4d4d), c.x - bodyR * 0.4f, c.y - bodyR * 0.5f,
                                             juce::Colour (0xff070707), c.x + bodyR * 0.7f, c.y + bodyR * 0.8f, true));
    g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

    // Pointer
    const float ang = startAngle + sliderPos * (endAngle - startAngle);
    const float s = std::sin (ang), co = std::cos (ang);
    g.setColour (juce::Colours::white);
    g.drawLine (c.x + s * bodyR * 0.30f, c.y - co * bodyR * 0.30f,
                c.x + s * bodyR * 0.94f, c.y - co * bodyR * 0.94f,
                juce::jmax (1.6f, R * 0.07f));
}

void SnobbyLook::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                       bool highlighted, bool down)
{
    const auto r = button.getLocalBounds().toFloat().reduced (0.5f);

    if (button.getName() == "TAP")
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3d3d3d), r.getX(), r.getY(),
                                                 juce::Colour (0xff0a0a0a), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 9.0f);
        g.setColour (down ? amber : juce::Colour (0xff808080));
        g.drawRoundedRectangle (r, 9.0f, 2.0f);
        return;
    }

    juce::Colour fill = button.getToggleState() ? juce::Colour (0xffffc04a) : cream;
    if (down)             fill = fill.darker (0.2f);
    else if (highlighted) fill = fill.brighter (0.1f);

    g.setColour (fill);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (ink);
    g.drawRoundedRectangle (r, 4.0f, 1.2f);
}

void SnobbyLook::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    const bool isTap = button.getName() == "TAP";
    g.setColour (isTap ? cream : ink);
    useFont (g, isTap ? 20.0f : 12.5f, true);
    g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
}

// ============================================================ main panel

DelayPanel::DelayPanel (SnobbyDelayProcessor& p)
    : proc (p)
{
    setLookAndFeel (&look);
    setSize (kDesignW, kDesignH);

    // Big knobs
    addKnob ("delayMs",  "DELAY",    "1",   "3500", 130, 74, 160);
    addKnob ("feedback", "FEEDBACK", "0",   "100",  585, 74, 160);

    // Display column
    addKnob ("noteDiv",   "NOTE",       "",    "",    341, 208, 58);
    addKnob ("manualBpm", "BPM TEMPO",  "40",  "240", 441, 208, 58);

    // Right column
    addKnob ("mix",    "DRY/WET", "0",   "100", 826, 72,  54);
    addKnob ("output", "OUTPUT",  "-18", "+18", 826, 152, 54);
    addKnob ("analog", "ANALOG",  "OFF", "3",   826, 232, 54);

    // Modulation
    addKnob ("modDepth", "DEPTH", "0",   "100", 40,  336, 70);
    addKnob ("modRate",  "RATE",  "0.1", "10",  130, 336, 70);

    // Filters
    addKnob ("hpf", "HIPASS",  "20",  "2k",  254, 336, 70);
    addKnob ("lpf", "LOPASS",  "1k",  "20k", 344, 336, 70);

    // Ducking
    addKnob ("duckThresh",  "THRESHOLD", "-60", "0",   552, 336, 70);
    addKnob ("duckAmount",  "AMOUNT",    "0",   "100", 632, 336, 70);
    addKnob ("duckAttack",  "ATTACK",    "1",   "200", 712, 336, 70);
    addKnob ("duckRelease", "RELEASE",   "10",  "2k",  792, 336, 70);

    // Buttons
    tapButton.setName ("TAP");
    tapButton.setBounds (28, 96, 88, 88);
    tapButton.onClick = [this] { tap(); };
    addAndMakeVisible (tapButton);

    pingButton.setClickingTogglesState (true);
    pingButton.setBounds (370, 72, 130, 22);
    addAndMakeVisible (pingButton);
    pingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "pingpong", pingButton);

    bpmButton.setBounds (313, 172, 76, 24);
    hostButton.setBounds (397, 172, 76, 24);
    msButton.setBounds (481, 172, 76, 24);
    bpmButton.onClick  = [this] { setTimeMode (1); };
    hostButton.onClick = [this] { setTimeMode (2); };
    msButton.onClick   = [this] { setTimeMode (0); };
    addAndMakeVisible (bpmButton);
    addAndMakeVisible (hostButton);
    addAndMakeVisible (msButton);

    duckButton.setClickingTogglesState (true);
    duckButton.setBounds (466, 342, 70, 26);
    addAndMakeVisible (duckButton);
    duckAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "duckOn", duckButton);

    duckModeButton.setBounds (466, 376, 70, 26);
    duckModeButton.onClick = [this]
    {
        if (auto* prm = proc.apvts.getParameter ("duckMode"))
        {
            const float now = prm->convertFrom0to1 (prm->getValue());
            prm->setValueNotifyingHost (prm->convertTo0to1 (now > 0.5f ? 0.0f : 1.0f));
        }
    };
    addAndMakeVisible (duckModeButton);

    // Presets
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedItemIndex (juce::jmax (0, proc.getCurrentProgram()), juce::dontSendNotification);
    presetBox.setBounds (470, 12, 260, 28);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

    startTimerHz (24);
}

DelayPanel::~DelayPanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void DelayPanel::addKnob (const juce::String& paramId, const juce::String& label,
                          const juce::String& minText, const juce::String& maxText,
                          int x, int y, int size)
{
    auto k = std::make_unique<Knob>();
    k->label = label;
    k->minText = minText;
    k->maxText = maxText;
    k->area = { x, y, size, size };

    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                   juce::MathConstants<float>::pi * 2.75f, true);
    k->slider.setPopupDisplayEnabled (true, true, this);
    k->slider.setBounds (k->area);
    addAndMakeVisible (k->slider);

    k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, k->slider);

    if (auto* prm = proc.apvts.getParameter (paramId))
        k->slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));

    knobs.push_back (std::move (k));
}

void DelayPanel::setTimeMode (int mode)
{
    if (auto* prm = proc.apvts.getParameter ("timeMode"))
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) mode));
}

void DelayPanel::tap()
{
    const double now = juce::Time::getMillisecondCounterHiRes();

    if (lastTapMs > 0.0 && now - lastTapMs < 2500.0)
    {
        tapIntervals.push_back (now - lastTapMs);
        if (tapIntervals.size() > 4)
            tapIntervals.erase (tapIntervals.begin());

        double sum = 0.0;
        for (double v : tapIntervals)
            sum += v;
        const double avg = sum / (double) tapIntervals.size();

        if (auto* prm = proc.apvts.getParameter ("delayMs"))
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) juce::jlimit (1.0, 3500.0, avg)));

        setTimeMode (0);
    }
    else
    {
        tapIntervals.clear();
    }

    lastTapMs = now;
}

void DelayPanel::timerCallback()
{
    const int mode = (int) proc.apvts.getRawParameterValue ("timeMode")->load();
    bpmButton.setToggleState (mode == 1, juce::dontSendNotification);
    hostButton.setToggleState (mode == 2, juce::dontSendNotification);
    msButton.setToggleState (mode == 0, juce::dontSendNotification);

    const bool sc = proc.apvts.getRawParameterValue ("duckMode")->load() > 0.5f;
    duckModeButton.setButtonText (sc ? "SC IN" : "AUTO");
    duckModeButton.setToggleState (sc, juce::dontSendNotification);

    repaint (300, 90, 280, 90);   // display
    repaint (760, 56, 60, 240);   // meter + led
}

void DelayPanel::paint (juce::Graphics& g)
{
    const float W = (float) kDesignW;
    const float H = (float) kDesignH;

    auto text = [&g] (const juce::String& s, float x, float y, float w, float h,
                      juce::Justification j = juce::Justification::centred)
    {
        g.drawText (s, juce::Rectangle<float> (x, y, w, h).toNearestInt(), j, false);
    };

    // Black frame
    g.fillAll (juce::Colour (0xff0d0b09));

    // Orange body
    const juce::Rectangle<float> body (10.0f, 56.0f, W - 20.0f, H - 78.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe08a22), 0.0f, body.getY(),
                                             juce::Colour (0xffbd650f), 0.0f, body.getBottom(), false));
    g.fillRoundedRectangle (body, 10.0f);
    g.setColour (juce::Colour (0xff6b3a08));
    g.drawRoundedRectangle (body, 10.0f, 2.0f);

    // Screws
    auto screw = [&g] (float cx, float cy, float deg)
    {
        g.setColour (juce::Colour (0xff8a8478));
        g.fillEllipse (cx - 6.0f, cy - 6.0f, 12.0f, 12.0f);
        g.setColour (juce::Colour (0xff3a3630));
        g.drawEllipse (cx - 6.0f, cy - 6.0f, 12.0f, 12.0f, 1.0f);
        const float a = juce::degreesToRadians (deg);
        g.drawLine (cx - std::cos (a) * 4.5f, cy - std::sin (a) * 4.5f,
                    cx + std::cos (a) * 4.5f, cy + std::sin (a) * 4.5f, 1.6f);
    };
    screw (24.0f, 70.0f, 30.0f);
    screw (W - 24.0f, 70.0f, 100.0f);
    screw (24.0f, H - 36.0f, 70.0f);
    screw (W - 24.0f, H - 36.0f, 10.0f);

    // Logo and preset label (top bar)
    g.setColour (cream);
    useFont (g, 30.0f, true, 0.05f);
    text ("SNOBBY DELAY", 24.0f, 6.0f, 300.0f, 34.0f, juce::Justification::left);
    g.setColour (amber);
    useFont (g, 11.0f, true, 0.3f);
    text ("DUCKING ANALOG DELAY", 26.0f, 36.0f, 300.0f, 16.0f, juce::Justification::left);
    g.setColour (cream);
    useFont (g, 12.0f, true, 0.25f);
    text ("PRESET", 390.0f, 16.0f, 70.0f, 20.0f, juce::Justification::right);

    // Panel outlines
    auto group = [&] (juce::Rectangle<float> r, const juce::String& title)
    {
        g.setColour (juce::Colour (0xff7a4308));
        g.drawRoundedRectangle (r, 6.0f, 1.6f);
        const float tw = (float) title.length() * 9.5f + 14.0f;
        g.setColour (juce::Colour (0xffcf7413));
        g.fillRect (r.getX() + 12.0f, r.getY() - 2.0f, tw, 4.0f);
        g.setColour (ink);
        useFont (g, 12.0f, true, 0.25f);
        text (title, r.getX() + 12.0f, r.getY() - 9.0f, tw, 18.0f);
    };
    group ({ 24.0f, 318.0f, 200.0f, 140.0f }, "MODULATION");
    group ({ 238.0f, 318.0f, 200.0f, 140.0f }, "FILTERS");
    group ({ 452.0f, 318.0f, 440.0f, 140.0f }, "DUCKING");

    // Knob labels
    for (const auto& k : knobs)
    {
        const auto a = k->area;
        const bool big = a.getWidth() > 100;
        g.setColour (ink);
        useFont (g, big ? 14.0f : 11.5f, true, 0.15f);
        text (k->label, (float) a.getX() - 20.0f, (float) a.getBottom() + (big ? 0.0f : -1.0f),
              (float) a.getWidth() + 40.0f, 16.0f);

        useFont (g, 9.5f, true);
        text (k->minText, (float) a.getX() - 2.0f, (float) a.getBottom() - 11.0f, 28.0f, 12.0f,
              juce::Justification::left);
        text (k->maxText, (float) a.getRight() + 2.0f - 28.0f, (float) a.getBottom() - 11.0f, 28.0f, 12.0f,
              juce::Justification::right);
    }

    // Tap label
    g.setColour (ink);
    useFont (g, 13.0f, true, 0.2f);
    text ("TEMPO TAP", 18.0f, 190.0f, 108.0f, 16.0f);

    // Digital display
    const juce::Rectangle<float> disp (305.0f, 100.0f, 260.0f, 64.0f);
    g.setColour (blackBox);
    g.fillRoundedRectangle (disp, 7.0f);
    g.setColour (juce::Colour (0xff5b3a14));
    g.drawRoundedRectangle (disp, 7.0f, 2.0f);

    const float ms = proc.displayDelayMs.load();
    const juce::String digits = juce::String ((int) std::round (ms)).paddedLeft ('0', 4);

    juce::Font digitFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 48.0f, juce::Font::bold));
    g.setFont (digitFont);
    for (int i = 0; i < 4; ++i)
    {
        const juce::Rectangle<float> cell (disp.getX() + 22.0f + (float) i * 48.0f, disp.getY() + 6.0f, 44.0f, 52.0f);
        g.setColour (juce::Colour (0xff2e1c06));
        g.drawText ("8", cell.toNearestInt(), juce::Justification::centred, false);
        g.setColour (amber);
        g.drawText (digits.substring (i, i + 1), cell.toNearestInt(), juce::Justification::centred, false);
    }

    const int mode = (int) proc.apvts.getRawParameterValue ("timeMode")->load();
    const int note = (int) proc.apvts.getRawParameterValue ("noteDiv")->load();
    g.setColour (amber);
    useFont (g, 11.0f, true);
    text (mode == 0 ? "MS" : juce::String ("MS  ") + SnobbyDelayProcessor::noteName (note),
          disp.getRight() - 90.0f, disp.getBottom() - 16.0f, 84.0f, 14.0f, juce::Justification::right);

    // Level meter
    const juce::Rectangle<float> meter (772.0f, 80.0f, 30.0f, 196.0f);
    g.setColour (juce::Colour (0xff0f0d0b));
    g.fillRoundedRectangle (meter.expanded (4.0f), 6.0f);
    g.setColour (cream);
    g.fillRoundedRectangle (meter, 3.0f);

    const float level = juce::jlimit (0.0f, 1.0f, (proc.meterDb.load() + 48.0f) / 54.0f);
    const int segments = 24;
    const float segH = meter.getHeight() / (float) segments;
    for (int s = 0; s < segments; ++s)
    {
        const float frac = (float) s / (float) segments;
        const bool lit = frac < level;
        juce::Colour c = frac > 0.88f ? juce::Colour (0xffd32f2f)
                       : frac > 0.66f ? juce::Colour (0xffffa000)
                                      : juce::Colour (0xff7cb342);
        g.setColour (lit ? c : juce::Colour (0xffc9b98a));
        g.fillRect (meter.getX() + 4.0f, meter.getBottom() - (float) (s + 1) * segH + 1.0f,
                    meter.getWidth() - 8.0f, segH - 2.0f);
    }
    g.setColour (ink);
    useFont (g, 11.0f, true, 0.15f);
    text ("LEVEL", 762.0f, 282.0f, 50.0f, 14.0f);

    // Duck LED
    const bool ducking = proc.duckGainShown.load() < 0.97f;
    g.setColour (juce::Colour (0xff0f0d0b));
    g.fillEllipse (778.0f, 56.0f, 18.0f, 18.0f);
    g.setColour (ducking ? juce::Colour (0xffff3d2e) : juce::Colour (0xff4a1510));
    g.fillEllipse (781.0f, 59.0f, 12.0f, 12.0f);

    // Ducking button caption
    g.setColour (ink);
    useFont (g, 10.5f, true, 0.1f);
    text ("SOURCE", 466.0f, 404.0f, 70.0f, 14.0f);

    // Footer
    g.setColour (cream);
    useFont (g, 12.0f, true, 0.25f);
    text ("BY BOOMHYPE TECH", W - 320.0f, H - 24.0f, 300.0f, 18.0f, juce::Justification::right);
}

// ============================================================ editor

SnobbyDelayEditor::SnobbyDelayEditor (SnobbyDelayProcessor& p)
    : AudioProcessorEditor (&p), panel (p)
{
    addAndMakeVisible (panel);
    setResizable (true, true);
    setResizeLimits (460, 245, 1840, 980);
    getConstrainer()->setFixedAspectRatio ((double) kDesignW / (double) kDesignH);
    setSize (kDesignW, kDesignH);
}

void SnobbyDelayEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0d0b09));
}

void SnobbyDelayEditor::resized()
{
    const float scale = juce::jmin ((float) getWidth() / (float) kDesignW,
                                    (float) getHeight() / (float) kDesignH);
    panel.setBounds (0, 0, kDesignW, kDesignH);
    panel.setTransform (juce::AffineTransform::scale (scale));
}