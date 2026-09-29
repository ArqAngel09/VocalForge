#include "PluginEditor.h"
#include "AMRLogo.h"

using APVTS = juce::AudioProcessorValueTreeState;

namespace
{
    const juce::Colour BG(0xff080d12), PANEL(0xff10171d), PANEL2(0xff171f26);
    const juce::Colour EDGE(0xff303b43), CYAN(0xff22c8ff), CYAN2(0xffa8efff);
    const juce::Colour WHITE(0xfff1f5f7), MUTED(0xff8e9aa2), ORANGE(0xffff9a2e);
    const juce::Colour GREEN(0xff40df9a), PURPLE(0xffa56be1);

    void labelText(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size,
                   juce::Colour c, juce::Justification j = juce::Justification::left)
    {
        auto f = juce::Font(juce::FontOptions{}.withHeight(size));
        f.setFallbackEnabled(true);
        f.setPreferredFallbackFamilies({ "Segoe UI", "Arial", "Noto Sans" });
        g.setFont(f);
        g.setColour(c);
        g.drawFittedText(s, r.toNearestInt(), j, 1);
    }

    void hideSlider(juce::Slider& s) { s.setVisible(false); }
}

AMRVocalLookAndFeel::AMRVocalLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId, PANEL2);
    setColour(juce::ComboBox::outlineColourId, EDGE);
    setColour(juce::ComboBox::textColourId, WHITE);
    setColour(juce::PopupMenu::backgroundColourId, PANEL2);
    setColour(juce::PopupMenu::textColourId, WHITE);
}

void AMRVocalLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                            float pos, float start, float end, juce::Slider& s)
{
    const float cx = x + w * .5f, cy = y + h * .45f;
    const float r = juce::jmin(w, h) * .31f;
    const float a = start + pos * (end - start);
    const auto accent = s.getName().containsIgnoreCase("Saturation") ||
                        s.getName().containsIgnoreCase("Magic") ||
                        s.getName().containsIgnoreCase("Exciter") ? PURPLE : CYAN;

    // Deep outer chassis + soft shadow.
    g.setColour(juce::Colour(0x99000000));
    g.fillEllipse(cx-r-10, cy-r-10, (r+10)*2, (r+10)*2);
    g.setColour(juce::Colour(0xff0b141b));
    g.fillEllipse(cx-r-7, cy-r-7, (r+7)*2, (r+7)*2);

    // Metallic body: two-stage radial gradient gives the control a real hardware feel.
    juce::ColourGradient body(juce::Colour(0xff6f8794), cx-r*.72f, cy-r*.82f,
                              juce::Colour(0xff0a1218), cx+r*.78f, cy+r*.82f, true);
    g.setGradientFill(body);
    g.fillEllipse(cx-r, cy-r, r*2, r*2);
    g.setColour(juce::Colour(0xff91a5ae));
    g.drawEllipse(cx-r+1.0f, cy-r+1.0f, (r-1.0f)*2, (r-1.0f)*2, 1.0f);
    g.setColour(juce::Colour(0xff1b2931));
    g.drawEllipse(cx-r+5.0f, cy-r+5.0f, (r-5.0f)*2, (r-5.0f)*2, 1.0f);

    // Top-left specular highlight.
    g.setColour(juce::Colour(0x7a9eefff));
    g.fillEllipse(cx-r*.58f, cy-r*.68f, r*.42f, r*.22f);

    juce::Path track, value;
    track.addCentredArc(cx, cy, r+7, r+7, 0.0f, start, end, true);
    value.addCentredArc(cx, cy, r+7, r+7, 0.0f, start, a, true);
    g.setColour(juce::Colour(0xff26343d)); g.strokePath(track, juce::PathStrokeType(5.0f));
    g.setColour(accent); g.strokePath(value, juce::PathStrokeType(5.0f));

    g.setColour(WHITE);
    g.drawLine(cx + std::cos(a)*(r-10), cy + std::sin(a)*(r-10),
               cx + std::cos(a)*(r-2), cy + std::sin(a)*(r-2), 2.2f);
}

void AMRVocalLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b,
                                                const juce::Colour&, bool hover, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced(.5f);
    const bool active = b.getToggleState();
    g.setColour(active ? juce::Colour(0xff173b4b) : (hover ? juce::Colour(0xff202d35) : PANEL2));
    g.fillRoundedRectangle(r, 7.0f);
    g.setColour(active || down ? CYAN : EDGE);
    g.drawRoundedRectangle(r, 7.0f, active ? 1.6f : 1.0f);
}

void AMRVocalLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    labelText(g, b.getButtonText(), b.getLocalBounds().toFloat(), 11.5f,
              b.getToggleState() ? CYAN2 : WHITE, juce::Justification::centred);
}

void AMRVocalLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(b.getToggleState() ? juce::Colour(0xff173b4b) : PANEL2);
    g.fillRoundedRectangle(r, 7.0f);
    g.setColour(b.getToggleState() ? CYAN : EDGE);
    g.drawRoundedRectangle(r, 7.0f, 1.2f);
    labelText(g, b.getButtonText(), r, 11.0f, b.getToggleState() ? CYAN2 : MUTED,
              juce::Justification::centred);
}

void AMRVocalLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float>(0,0,(float)w,(float)h).reduced(.5f);
    g.setColour(PANEL2); g.fillRoundedRectangle(r, 6.0f);
    g.setColour(EDGE); g.drawRoundedRectangle(r, 6.0f, 1.0f);
    g.setColour(CYAN2);
    juce::Path p; p.startNewSubPath(w-18.0f,h*.42f); p.lineTo(w-13.0f,h*.58f); p.lineTo(w-8.0f,h*.42f);
    g.strokePath(p, juce::PathStrokeType(1.5f));
}

juce::Font AMRVocalLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    auto f = juce::Font(juce::FontOptions{}.withHeight(12.0f));
    f.setFallbackEnabled(true);
    f.setPreferredFallbackFamilies({ "Segoe UI", "Arial", "Noto Sans" });
    return f;
}

VocalForgeAudioProcessorEditor::VocalForgeAudioProcessorEditor(VocalForgeAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lookAndFeel);
    setSize(1120, 720);
    setResizable(true, true);
    setResizeLimits(980, 650, 1700, 1050);

    for (auto* b : { &vocalAssistTab, &simpleTab, &advancedTab, &analyzeButton, &bypassButton,
                     &compareButton, &retryButton, &autoGainButton, &dynamicsTab, &eqTab, &spaceTab, &characterTab, &graphTab })
        addAndMakeVisible(b);

    vocalAssistTab.setClickingTogglesState(true);
    simpleTab.setClickingTogglesState(true);
    advancedTab.setClickingTogglesState(true);
    dynamicsTab.setClickingTogglesState(true);
    eqTab.setClickingTogglesState(true);
    spaceTab.setClickingTogglesState(true);
    characterTab.setClickingTogglesState(true);
    graphTab.setClickingTogglesState(true);

    vocalAssistTab.onClick = [this]{ setPage(0); };
    simpleTab.onClick = [this]{ setPage(1); };
    advancedTab.onClick = [this]{ setPage(2); };
    dynamicsTab.onClick = [this]{ setModule(0); };
    eqTab.onClick = [this]{ setModule(1); };
    spaceTab.onClick = [this]{ setModule(2); };
    characterTab.onClick = [this]{ setModule(3); };
    graphTab.onClick = [this]{ setModule(4); };

    analyzeButton.setButtonText("ESCUCHAR Y ANALIZAR");
    analyzeButton.onClick = [this]{ processor.triggerAnalysis(); };
    retryButton.onClick = [this]{ processor.triggerAnalysis(); };
    compareButton.setClickingTogglesState(true);
    compareButton.onClick = [this]
    {
        const bool bypassed = compareButton.getToggleState();
        if (auto* param = processor.apvts.getParameter("bypass"))
            param->setValueNotifyingHost(bypassed ? 1.0f : 0.0f);
    };

    bypassButton.setClickingTogglesState(true);
    bypassAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, "bypass", bypassButton);
    autoGainButton.setClickingTogglesState(true);
    autoGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, "autoGain", autoGainButton);

    title.setText("AMR Vocal Mix", juce::dontSendNotification);
    subtitle.setText("PRO VOCAL PROCESSOR", juce::dontSendNotification);
    presetLabel.setText("Preset", juce::dontSendNotification);
    for (auto* l : { &title, &subtitle, &presetLabel, &status }) addAndMakeVisible(l);

    preset.addItem("Voz Principal - Profesional", 1);
    preset.addItem("Voz Principal - Natural", 2);
    preset.addItem("Backing Vocal", 3);
    preset.addItem("Reggaeton", 4);
    preset.setSelectedId(1);
    preset.onChange = [this]
    {
        const int id = preset.getSelectedId();
        const auto set = [this](const char* p, float v)
        {
            if (auto* param = processor.apvts.getParameter(p))
                param->setValueNotifyingHost(param->convertTo0to1(v));
        };
        switch (id)
        {
            case 1: // Professional lead
                set("retune", 62.0f); set("speed", 72.0f); set("comp", 42.0f);
                set("magic", 24.0f); set("space", 10.0f); set("delay", 0.0f);
                set("body", 0.0f); set("presence", 1.5f); set("air", 1.5f); set("drive", 6.0f); break;
            case 2: // Natural lead
                set("retune", 42.0f); set("speed", 105.0f); set("comp", 30.0f);
                set("magic", 12.0f); set("space", 8.0f); set("delay", 0.0f);
                set("body", 0.0f); set("presence", 1.0f); set("air", 1.0f); set("drive", 3.0f); break;
            case 3: // Backing
                set("retune", 48.0f); set("speed", 95.0f); set("comp", 50.0f);
                set("magic", 18.0f); set("space", 18.0f); set("delay", 8.0f);
                set("body", -0.5f); set("presence", 0.5f); set("air", 1.5f); set("drive", 4.0f); break;
            case 4: // Reggaeton
                set("retune", 78.0f); set("speed", 55.0f); set("comp", 58.0f);
                set("magic", 32.0f); set("space", 10.0f); set("delay", 12.0f);
                set("body", 0.5f); set("presence", 2.0f); set("air", 2.5f); set("drive", 10.0f); break;
            default: break;
        }
    };
    addAndMakeVisible(preset);

    setupSlider(retune,"Retune"); setupSlider(speed,"Speed");
    setupSlider(body,"Body"); setupSlider(presence,"Presence"); setupSlider(air,"Air");
    setupSlider(comp,"Compression"); setupSlider(sat,"Saturation"); setupSlider(deess,"De-Esser");
    setupSlider(space,"Reverb"); setupSlider(delay,"Delay"); setupSlider(output,"Output");
    setupSlider(magic,"Magic"); setupSlider(color,"Color");
    setupSlider(eqLow,"Low"); setupSlider(eqLowMid,"Low-Mid"); setupSlider(eqHighMid,"High-Mid"); setupSlider(eqHigh,"High");
    setupSlider(spaceTime,"Time"); setupSlider(exciter,"Exciter"); setupSlider(doubler,"Doubler");
    setupSlider(denoise,"Denoise"); setupSlider(resonance,"Resonance"); setupSlider(multiband,"Multiband");
    setupSlider(delayFeedback,"Feedback"); setupSlider(delayTone,"Tone"); setupSlider(deessFocus,"Focus");
    setupSlider(compAttack,"Attack"); setupSlider(compRelease,"Release");

    retuneAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"retune",retune);
    speedAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"speed",speed);
    bodyAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"body",body);
    presenceAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"presence",presence);
    airAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"air",air);
    compAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"comp",comp);
    satAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"drive",sat);
    deessAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"deess",deess);
    spaceAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"space",space);
    delayAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"delay",delay);
    outputAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"output",output);
    magicAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"magic",magic);
    colorAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"color",color);
    eqLowAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"eqLow",eqLow);
    eqLowMidAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"eqLowMid",eqLowMid);
    eqHighMidAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"eqHighMid",eqHighMid);
    eqHighAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"eqHigh",eqHigh);
    spaceTimeAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"spaceTime",spaceTime);
    exciterAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"exciter",exciter);
    doublerAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"doubler",doubler);
    denoiseAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"denoise",denoise);
    resonanceAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"resonance",resonance);
    multibandAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"multiband",multiband);
    delayFeedbackAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"delayFeedback",delayFeedback);
    delayToneAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"delayTone",delayTone);
    deessFocusAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"deessFocus",deessFocus);
    compAttackAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"compAttack",compAttack);
    compReleaseAttach = std::make_unique<APVTS::SliderAttachment>(processor.apvts,"compRelease",compRelease);

    key.addItemList({"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"},1);
    scale.addItemList({"Major","Minor","Chromatic"},1);
    mode.addItemList({"Live","Studio"},1);
    style.addItemList({"Clean","Warm","Bright","Aggressive"},1);
    reverbType.addItemList({"Room","Plate","Large"},1);
    delaySubdivision.addItemList({"1/16","1/8","1/4"},1);
    for (auto* c : { &key,&scale,&mode,&style,&reverbType,&delaySubdivision }) addAndMakeVisible(c);
    keyAttach = std::make_unique<APVTS::ComboBoxAttachment>(processor.apvts,"root",key);
    scaleAttach = std::make_unique<APVTS::ComboBoxAttachment>(processor.apvts,"scale",scale);
    modeAttach = std::make_unique<APVTS::ComboBoxAttachment>(processor.apvts,"mode",mode);
    styleAttach = std::make_unique<APVTS::ComboBoxAttachment>(processor.apvts,"style",style);
    reverbTypeAttach = std::make_unique<APVTS::ComboBoxAttachment>(processor.apvts,"reverbType",reverbType);
    delaySubdivisionAttach = std::make_unique<APVTS::ComboBoxAttachment>(processor.apvts,"delaySubdivision",delaySubdivision);

    setPage(0);
    setModule(0);
    startTimerHz(30);
}

VocalForgeAudioProcessorEditor::~VocalForgeAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void VocalForgeAudioProcessorEditor::setupSlider(juce::Slider& s, const juce::String& name)
{
    s.setName(name);
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    if (name == "Retune" || name == "Compression" || name == "Saturation" || name == "De-Esser" ||
        name == "Reverb" || name == "Delay" || name == "Magic" || name == "Color" ||
        name == "Exciter" || name == "Doubler" || name == "Denoise" || name == "Resonance" ||
        name == "Multiband" || name == "Feedback" || name == "Tone")
        s.setRange(0.0, 100.0, 0.1);
    else if (name == "Speed") s.setRange(5.0, 250.0, 0.1);
    else if (name == "Body" || name == "Presence" || name == "Air") s.setRange(-6.0, 10.0, 0.1);
    else if (name == "Output") s.setRange(-12.0, 6.0, 0.1);
    else if (name == "Low" || name == "Low-Mid" || name == "High-Mid" || name == "High") s.setRange(-6.0, 6.0, 0.1);
    else if (name == "Time") s.setRange(0.2, 6.0, 0.01);
    else if (name == "Focus") s.setRange(2000.0, 10000.0, 1.0);
    else if (name == "Attack") s.setRange(1.0, 50.0, 0.1);
    else if (name == "Release") s.setRange(50.0, 500.0, 1.0);
    addAndMakeVisible(s);
}

void VocalForgeAudioProcessorEditor::setupCombo(juce::ComboBox&, const juce::StringArray&) {}

void VocalForgeAudioProcessorEditor::setPage(int page)
{
    activePage = juce::jlimit(0, 2, page);
    vocalAssistTab.setToggleState(activePage == 0, juce::dontSendNotification);
    simpleTab.setToggleState(activePage == 1, juce::dontSendNotification);
    advancedTab.setToggleState(activePage == 2, juce::dontSendNotification);
    resized();
    repaint();
}

void VocalForgeAudioProcessorEditor::setModule(int module)
{
    activeModule = juce::jlimit(0, 4, module);
    dynamicsTab.setToggleState(activeModule == 0, juce::dontSendNotification);
    eqTab.setToggleState(activeModule == 1, juce::dontSendNotification);
    spaceTab.setToggleState(activeModule == 2, juce::dontSendNotification);
    characterTab.setToggleState(activeModule == 3, juce::dontSendNotification);
    graphTab.setToggleState(activeModule == 4, juce::dontSendNotification);
    resized();
    repaint();
}

void VocalForgeAudioProcessorEditor::drawPanel(juce::Graphics& g, juce::Rectangle<float> r, float radius)
{
    juce::ColourGradient grad(PANEL2, r.getX(), r.getY(), PANEL, r.getRight(), r.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(r, radius);
    g.setColour(EDGE);
    g.drawRoundedRectangle(r, radius, 1.0f);
}

void VocalForgeAudioProcessorEditor::drawTitle(juce::Graphics& g, const juce::String& s, float x, float y, float w, float h)
{
    labelText(g, s, {x,y,w,h}, 14.0f, CYAN2);
}

void VocalForgeAudioProcessorEditor::drawLogo(juce::Graphics& g, juce::Rectangle<float> r)
{
    std::unique_ptr<juce::Drawable> d(juce::Drawable::createFromImageData(amrLogoSvg, amrLogoSvgSize));
    if (d) d->drawWithin(g, r, juce::RectanglePlacement::centred, 1.0f);
}

juce::String VocalForgeAudioProcessorEditor::valueText(const juce::Slider& s) const
{
    const auto n = s.getName();
    if (n == "Output") return juce::String(s.getValue(), 1) + " dB";
    if (n == "Speed") return juce::String((int) std::round(s.getValue())) + " ms";
    if (n == "Time") return juce::String(s.getValue(), 2) + " s";
    if (n == "Focus") return juce::String((int) std::round(s.getValue())) + " Hz";
    if (n == "Attack" || n == "Release") return juce::String((int) std::round(s.getValue())) + " ms";
    if (n == "Low" || n == "Low-Mid" || n == "High-Mid" || n == "High")
        return juce::String(s.getValue(), 1) + " dB";
    return juce::String((int) std::round(s.getValue())) + "%";
}

void VocalForgeAudioProcessorEditor::drawKnobInfo(juce::Graphics& g, juce::Slider& s, const juce::String& caption)
{
    if (!s.isVisible()) return;
    auto r = s.getBounds().toFloat();
    labelText(g, s.getName(), {r.getX(), r.getY()-2, r.getWidth(), 18}, 10.5f, WHITE, juce::Justification::centred);
    labelText(g, valueText(s), {r.getX(), r.getBottom()-30, r.getWidth(), 17}, 11.5f, WHITE, juce::Justification::centred);
    labelText(g, caption, {r.getX(), r.getBottom()-14, r.getWidth(), 14}, 8.5f, MUTED, juce::Justification::centred);
}

void VocalForgeAudioProcessorEditor::drawMeters(juce::Graphics& g, juce::Rectangle<float> r)
{
    const auto meter = [&](float x, float db, const juce::String& name)
    {
        labelText(g, name, {x, r.getY(), 55, 16}, 9, MUTED, juce::Justification::centred);
        auto m = juce::Rectangle<float>(x+20, r.getY()+22, 15, r.getHeight()-46);
        g.setColour(juce::Colour(0xff080c0f)); g.fillRoundedRectangle(m, 3);
        const float level = juce::jlimit(0.0f, 1.0f, (db + 48.0f) / 48.0f);
        g.setColour(GREEN);
        g.fillRoundedRectangle(m.withY(m.getBottom()-m.getHeight()*level).withHeight(m.getHeight()*level), 3);
        labelText(g, db <= -99.0f ? "−∞" : juce::String(db,1), {x, r.getBottom()-20, 55, 17}, 9, WHITE, juce::Justification::centred);
    };
    meter(r.getX(), processor.getInputDb(), "INPUT");
    meter(r.getX()+65, processor.getOutputDb(), "OUTPUT");
}

void VocalForgeAudioProcessorEditor::drawAssistRing(juce::Graphics& g, juce::Rectangle<float> r)
{
    const float cx = r.getCentreX(), cy = r.getCentreY();
    const float radius = juce::jmin(r.getWidth(), r.getHeight()) * .30f;
    const float progress = processor.getAnalysisProgress();
    for (int i=0; i<44; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * i / 44.0f - juce::MathConstants<float>::halfPi;
        const bool lit = i < (int) std::round(progress * 44.0f);
        g.setColour(lit ? ORANGE : juce::Colour(0xff3b444a));
        g.drawLine(cx + std::cos(a)*(radius-8), cy + std::sin(a)*(radius-8),
                   cx + std::cos(a)*radius, cy + std::sin(a)*radius, 5.0f);
    }
    g.setColour(juce::Colour(0xff20272b)); g.fillEllipse(cx-radius*.72f, cy-radius*.72f, radius*1.44f, radius*1.44f);
    g.setColour(juce::Colour(0xff59636a)); g.drawEllipse(cx-radius*.72f, cy-radius*.72f, radius*1.44f, radius*1.44f, 1.5f);
    labelText(g, progress >= 1.0f ? "LISTO" : (progress > 0.0f ? "ANALIZANDO" : "ESCUCHAR"),
              {cx-70,cy-10,140,22}, 12, WHITE, juce::Justification::centred);
    labelText(g, juce::String((int) std::round(progress*100.0f)) + "%",
              {cx-50,cy+radius*.95f,100,24}, 18, WHITE, juce::Justification::centred);
}

void VocalForgeAudioProcessorEditor::drawPitchGraph(juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour(juce::Colour(0xff091117)); g.fillRoundedRectangle(r, 7);
    for (int i=0;i<=6;++i)
    {
        const float y=r.getBottom()-i*r.getHeight()/6.0f;
        g.setColour(juce::Colour(0xff26343d)); g.drawHorizontalLine((int)y,r.getX(),r.getRight());
        labelText(g,"C"+juce::String(i+1),{r.getX()+6,y-8,25,16},8,MUTED);
    }
    const float detected=processor.getDetectedMidi(), target=processor.getTargetMidi();
    auto py=[&](float midi){ return r.getBottom()-juce::jlimit(24.0f,96.0f,midi-24.0f)/72.0f*r.getHeight(); };
    if (detected > 0.0f)
    {
        juce::Path p;
        for (int i=0;i<90;++i)
        {
            const float x=r.getX()+i*r.getWidth()/89.0f;
            const float y=py(detected+0.04f*std::sin(i*.23f+phase)*12.0f);
            if(i==0) p.startNewSubPath(x,y); else p.lineTo(x,y);
        }
        g.setColour(CYAN); g.strokePath(p,juce::PathStrokeType(1.8f));
    }
    if (target > 0.0f)
    {
        g.setColour(PURPLE); g.drawHorizontalLine((int)py(target),r.getX()+4,r.getRight()-4);
    }
}

void VocalForgeAudioProcessorEditor::drawSpectrum(juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour(juce::Colour(0xff091117)); g.fillRoundedRectangle(r,7);
    juce::Path p;
    for (int i=0;i<120;++i)
    {
        const float t=i/119.0f, x=r.getX()+t*r.getWidth();
        const float amp=.12f+.66f*std::exp(-3.2f*t)+.045f*std::abs(std::sin(i*1.7f+phase));
        const float y=r.getBottom()-amp*r.getHeight();
        if(i==0) p.startNewSubPath(x,y); else p.lineTo(x,y);
    }
    g.setColour(CYAN); g.strokePath(p,juce::PathStrokeType(1.4f));
}

void VocalForgeAudioProcessorEditor::drawEqGraph(juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour(juce::Colour(0xff080e13)); g.fillRoundedRectangle(r, 8);
    const float freqs[] = { 20, 100, 1000, 10000, 20000 };
    for (float f : freqs)
    {
        const float x = r.getX() + std::log10(f/20.0f) / 3.0f * r.getWidth();
        g.setColour(juce::Colour(0xff27353e)); g.drawVerticalLine((int)x,r.getY(),r.getBottom());
        labelText(g, f>=1000 ? juce::String(f/1000.0f,0)+"k" : juce::String((int)f),
                  {x-20,r.getBottom()-18,40,16},8,MUTED,juce::Justification::centred);
    }
    g.setColour(juce::Colour(0xff27353e)); g.drawHorizontalLine((int)r.getCentreY(),r.getX(),r.getRight());

    const float gains[] = {(float)eqLow.getValue(),(float)eqLowMid.getValue(),(float)eqHighMid.getValue(),(float)eqHigh.getValue()};
    const float xs[] = {0.20f,0.40f,0.66f,0.90f};
    juce::Path p;
    for(int i=0;i<4;++i)
    {
        const float x=r.getX()+xs[i]*r.getWidth();
        const float y=r.getCentreY()-gains[i]/6.0f*(r.getHeight()*.38f);
        if(i==0)p.startNewSubPath(x,y); else p.lineTo(x,y);
    }
    g.setColour(ORANGE); g.strokePath(p,juce::PathStrokeType(2.2f));
}

void VocalForgeAudioProcessorEditor::layoutKnobGrid(std::initializer_list<juce::Slider*> sliders)
{
    for (auto* s : sliders) s->setVisible(false);
    const int top = 268;
    const auto area = getLocalBounds().reduced(42).withTop(top).withBottom(getHeight()-42);
    const int count = (int) sliders.size();
    const int cols = count <= 3 ? count : 3;
    const int rows = (count + cols - 1) / cols;
    const int gapX = 24, gapY = 24;
    const int cellW = (area.getWidth()-gapX*(cols-1))/cols;
    const int cellH = (area.getHeight()-gapY*(rows-1))/rows;
    int i=0;
    for(auto* s : sliders)
    {
        const int row=i/cols, col=i%cols;
        s->setVisible(true);
        s->setBounds(area.getX()+col*(cellW+gapX)+8,
                     area.getY()+row*(cellH+gapY)+6,
                     cellW-16, juce::jmin(cellH-12,142));
        ++i;
    }
}

void VocalForgeAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(BG);
    auto a = getLocalBounds().toFloat().reduced(14.0f);

    drawPanel(g,{a.getX(),a.getY(),a.getWidth(),74},10);
    drawLogo(g,{a.getX()+10,a.getY()+9,50,50});
    labelText(g,"AMR",{a.getX()+70,a.getY()+13,65,26},22,WHITE);
    labelText(g,"Vocal Mix",{a.getX()+132,a.getY()+13,140,26},22,CYAN);
    labelText(g,"PRO VOCAL PROCESSOR",{a.getX()+70,a.getY()+39,180,14},8.5f,MUTED);

    const float y=a.getY()+86;
    drawPanel(g,{a.getX(),y,a.getWidth(),a.getHeight()-86},12);

    if(activePage==0)
    {
        labelText(g,"VOCAL ASSIST",{a.getX()+28,y+20,180,22},15,WHITE);
        labelText(g,"Escucha 15 segundos de voz y aplica el perfil después de capturar.",{a.getX()+28,y+47,a.getWidth()-56,18},10.5f,MUTED);
        drawAssistRing(g,{a.getX()+170,y+68,a.getWidth()-340,360});
        drawMeters(g,{a.getX()+22,y+92,120,300});
        drawMeters(g,{a.getRight()-142,y+92,120,300});
        labelText(g,"ORIGINAL",{a.getCentreX()-145,y+420,90,18},9,MUTED,juce::Justification::centred);
        labelText(g,"ASISTIDA",{a.getCentreX()-45,y+420,90,18},9,CYAN2,juce::Justification::centred);
        labelText(g,"REINICIAR",{a.getCentreX()+55,y+420,90,18},9,WHITE,juce::Justification::centred);
        labelText(g,processor.getAnalysisSummary(),{a.getX()+120,a.getBottom()-48,a.getWidth()-240,20},10,GREEN,juce::Justification::centred);
        const auto gm = processor.getGainMatchDb();
        labelText(g, processor.isAutoGainEnabled() ? ("AUTO GAIN  " + juce::String(gm,1) + " dB") : "AUTO GAIN OFF",
                  {a.getX()+120,a.getBottom()-25,a.getWidth()-240,18},9, processor.isAutoGainEnabled() ? CYAN2 : MUTED, juce::Justification::centred);
    }
    else if(activePage==1)
    {
        labelText(g,"SIMPLE",{a.getX()+28,y+18,150,22},15,WHITE);
        labelText(g,"Corrección natural, dinámica y espacio en un solo panel.",{a.getX()+28,y+44,520,18},10.5f,MUTED);
        drawPitchGraph(g,{a.getX()+28,y+68,a.getWidth()-56,112});
        // Pitch/key strip is deliberately separated from the knob row.
        labelText(g,"PITCH & KEY",{a.getX()+28,y+190,110,16},9,CYAN2);
    }
    else
    {
        labelText(g,"ADVANCED",{a.getX()+28,y+18,150,22},15,WHITE);
        labelText(g,"Control de mezcla detallado con módulos independientes.",{a.getX()+28,y+44,500,18},10.5f,MUTED);
        drawPitchGraph(g,{a.getX()+28,y+68,a.getWidth()*.57f,112});
        drawSpectrum(g,{a.getX()+a.getWidth()*.60f,y+68,a.getWidth()*.36f,112});
        labelText(g,"PROCESSING MODULES",{a.getX()+28,y+190,180,16},9,CYAN2);
        if(activeModule==4) drawEqGraph(g,{a.getX()+28,y+228,a.getWidth()-56,115});
    }
}

void VocalForgeAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    if(activePage==1)
    {
        drawKnobInfo(g,retune,"Pitch Amount");
        drawKnobInfo(g,speed,"Retune Speed");
        drawKnobInfo(g,comp,"Dynamics");
        drawKnobInfo(g,magic,"Character");
        drawKnobInfo(g,space,"Space");
        drawKnobInfo(g,delay,"Delay");
    }
    else if(activePage==2)
    {
        if(activeModule==0)
        {
            drawKnobInfo(g,comp,"Dynamics");
            drawKnobInfo(g,color,"Tone color");
            drawKnobInfo(g,deessFocus,"De-Esser");
            drawKnobInfo(g,multiband,"Multiband");
            drawKnobInfo(g,compAttack,"Transient");
            drawKnobInfo(g,compRelease,"Envelope");
        }
        else if(activeModule==1)
        {
            drawKnobInfo(g,eqLow,"80 Hz");
            drawKnobInfo(g,eqLowMid,"700 Hz");
            drawKnobInfo(g,eqHighMid,"4 kHz");
            drawKnobInfo(g,eqHigh,"12 kHz");
        }
        else if(activeModule==2)
        {
            drawKnobInfo(g,space,"Reverb");
            drawKnobInfo(g,spaceTime,"Time");
            drawKnobInfo(g,delay,"Delay");
            drawKnobInfo(g,delayFeedback,"Feedback");
            drawKnobInfo(g,delayTone,"Tone");
        }
        else if(activeModule==3)
        {
            drawKnobInfo(g,magic,"Magic");
            drawKnobInfo(g,sat,"Saturation");
            drawKnobInfo(g,exciter,"Exciter");
            drawKnobInfo(g,doubler,"Doubler");
            drawKnobInfo(g,denoise,"Denoise");
            drawKnobInfo(g,resonance,"Resonance");
        }
        else
        {
            drawKnobInfo(g,eqLow,"80 Hz");
            drawKnobInfo(g,eqLowMid,"700 Hz");
            drawKnobInfo(g,eqHighMid,"4 kHz");
            drawKnobInfo(g,eqHigh,"12 kHz");
        }
    }
}

void VocalForgeAudioProcessorEditor::resized()
{
    auto a=getLocalBounds().reduced(14);
    vocalAssistTab.setBounds(a.getRight()-410,a.getY()+16,125,42);
    simpleTab.setBounds(a.getRight()-280,a.getY()+16,105,42);
    advancedTab.setBounds(a.getRight()-171,a.getY()+16,155,42);
    bypassButton.setBounds(a.getX()+300,a.getY()+16,92,42);
    preset.setBounds(a.getX()+408,a.getY()+21,235,32);

    const int y=a.getY()+86;

    for(auto* s : {&retune,&speed,&body,&presence,&air,&comp,&sat,&deess,&space,&delay,&output,
                   &magic,&color,&eqLow,&eqLowMid,&eqHighMid,&eqHigh,&spaceTime,&exciter,&doubler,
                   &denoise,&resonance,&multiband,&delayFeedback,&delayTone,&deessFocus,&compAttack,&compRelease})
        hideSlider(*s);
    for(auto* c : {&key,&scale,&mode,&style,&reverbType,&delaySubdivision}) c->setVisible(false);
    for(auto* b : {&dynamicsTab,&eqTab,&spaceTab,&characterTab,&graphTab,&analyzeButton,&compareButton,&retryButton,&autoGainButton})
        b->setVisible(false);

    if(activePage==0)
    {
        analyzeButton.setVisible(true); analyzeButton.setBounds(a.getCentreX()-145,y+350,290,48);
        compareButton.setVisible(true); compareButton.setBounds(a.getCentreX()-180,y+410,110,34);
        retryButton.setVisible(true); retryButton.setBounds(a.getCentreX()+70,y+410,110,34);
        autoGainButton.setVisible(true); autoGainButton.setBounds(a.getRight()-180,a.getY()+21,145,32);
    }
    else if(activePage==1)
    {
        // A dedicated, non-overlapping pitch strip.
        key.setVisible(true); scale.setVisible(true); mode.setVisible(true); style.setVisible(true);
        key.setBounds(a.getX()+28,y+207,92,30);
        scale.setBounds(a.getX()+126,y+207,112,30);
        mode.setBounds(a.getX()+244,y+207,100,30);
        style.setBounds(a.getX()+350,y+207,120,30);
        layoutKnobGrid({&retune,&speed,&comp,&magic,&space,&delay});
    }
    else
    {
        dynamicsTab.setVisible(true); eqTab.setVisible(true); spaceTab.setVisible(true); characterTab.setVisible(true); graphTab.setVisible(true);
        const int bx=a.getX()+28, by=y+214;
        dynamicsTab.setBounds(bx,by,118,32); eqTab.setBounds(bx+124,by,76,32);
        spaceTab.setBounds(bx+206,by,88,32); characterTab.setBounds(bx+300,by,112,32); graphTab.setBounds(bx+418,by,104,32);

        key.setVisible(true); scale.setVisible(true); mode.setVisible(true); style.setVisible(true);
        key.setBounds(a.getRight()-420,y+214,92,30);
        scale.setBounds(a.getRight()-322,y+214,112,30);
        mode.setBounds(a.getRight()-204,y+214,92,30);
        style.setBounds(a.getRight()-106,y+214,92,30);

        if(activeModule==0)
            layoutKnobGrid({&comp,&color,&deessFocus,&multiband,&compAttack,&compRelease});
        else if(activeModule==1 || activeModule==4)
            layoutKnobGrid({&eqLow,&eqLowMid,&eqHighMid,&eqHigh});
        else if(activeModule==2)
        {
            layoutKnobGrid({&space,&spaceTime,&delay,&delayFeedback,&delayTone});
            reverbType.setVisible(true); delaySubdivision.setVisible(true);
            reverbType.setBounds(a.getX()+28,y+248,105,30);
            delaySubdivision.setBounds(a.getX()+145,y+248,105,30);
        }
        else
            layoutKnobGrid({&magic,&sat,&exciter,&doubler,&denoise,&resonance});
    }
}

void VocalForgeAudioProcessorEditor::timerCallback()
{
    phase += 0.08f;
    inMeter = juce::jlimit(0.0f,1.0f,(processor.getInputDb()+48.0f)/48.0f);
    outMeter = juce::jlimit(0.0f,1.0f,(processor.getOutputDb()+48.0f)/48.0f);
    repaint();
}
