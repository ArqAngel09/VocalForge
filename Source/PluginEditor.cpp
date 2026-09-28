#include "PluginEditor.h"
#include "AMRLogo.h"

namespace
{
    const juce::Colour BG(0xff03070b), PANEL(0xff06111a), PANEL2(0xff081620);
    const juce::Colour BORDER(0xff087bb9), CYAN(0xff08d9ff), CYAN2(0xff7edfff);
    const juce::Colour WHITE(0xffedf8ff), MUTED(0xffa9c2d1), PURPLE(0xff8b32ff), GREEN(0xff1df27a);

    void labelText(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size,
                   juce::Colour c, juce::Justification j = juce::Justification::left)
    {
        g.setColour(c);
        auto font = juce::Font(juce::FontOptions{}.withHeight(size));
        font.setFallbackEnabled(true);
        font.setPreferredFallbackFamilies({ "Segoe UI", "Arial", "Noto Sans" });
        g.setFont(font);
        g.drawFittedText(s, r.toNearestInt(), j, 1);
    }
}

AMRVocalLookAndFeel::AMRVocalLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId, PANEL2);
    setColour(juce::ComboBox::outlineColourId, BORDER);
    setColour(juce::ComboBox::textColourId, WHITE);
    setColour(juce::PopupMenu::backgroundColourId, PANEL2);
    setColour(juce::PopupMenu::textColourId, WHITE);
}

void AMRVocalLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                            float pos, float start, float end, juce::Slider& s)
{
    const float cx = x + w * 0.5f, cy = y + h * 0.47f;
    const float r = juce::jmin(w, h) * 0.34f;
    const float a = start + pos * (end - start);
    const auto accent = s.getName() == "Saturation" ? PURPLE : CYAN;

    g.setColour(juce::Colour(0xff02070c)); g.fillEllipse(cx-r-5, cy-r-5, (r+5)*2, (r+5)*2);
    g.setColour(juce::Colour(0xff102532)); g.fillEllipse(cx-r, cy-r, r*2, r*2);

    juce::Path track, value;
    track.addCentredArc(cx, cy, r+7, r+7, 0.0f, start, end, true);
    value.addCentredArc(cx, cy, r+7, r+7, 0.0f, start, a, true);
    g.setColour(juce::Colour(0xff163140));
    g.strokePath(track, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(accent);
    g.strokePath(value, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(juce::Colour(0xff091722)); g.fillEllipse(cx-r+5, cy-r+5, (r-5)*2, (r-5)*2);
    g.setColour(CYAN2);
    g.drawLine(cx + std::cos(a)*(r-10), cy + std::sin(a)*(r-10),
               cx + std::cos(a)*(r-2),  cy + std::sin(a)*(r-2), 2.0f);
}

void AMRVocalLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b,
                                                const juce::Colour&, bool hover, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(hover ? juce::Colour(0xff092c3c) : PANEL2); g.fillRoundedRectangle(r, 7.0f);
    g.setColour(down ? CYAN2 : BORDER); g.drawRoundedRectangle(r, 7.0f, 1.2f);
}

void AMRVocalLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    labelText(g, b.getButtonText(), b.getLocalBounds().toFloat(), 13.0f, CYAN2, juce::Justification::centred);
}

void AMRVocalLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(b.getToggleState() ? juce::Colour(0xff073445) : PANEL2); g.fillRoundedRectangle(r, 7.0f);
    g.setColour(b.getToggleState() ? CYAN : BORDER); g.drawRoundedRectangle(r, 7.0f, 1.1f);
    labelText(g, b.getButtonText(), r, 12.0f, b.getToggleState() ? CYAN : MUTED, juce::Justification::centred);
}

void AMRVocalLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float>(0, 0, (float) w, (float) h).reduced(0.5f);
    g.setColour(PANEL2); g.fillRoundedRectangle(r, 6.0f);
    g.setColour(BORDER); g.drawRoundedRectangle(r, 6.0f, 1.0f);
    g.setColour(CYAN2);
    juce::Path p; p.startNewSubPath((float) w-18, h*0.42f); p.lineTo((float) w-13, h*0.58f); p.lineTo((float) w-8, h*0.42f);
    g.strokePath(p, juce::PathStrokeType(1.6f));
}

juce::Font AMRVocalLookAndFeel::getComboBoxFont(juce::ComboBox&) { auto f = juce::Font(juce::FontOptions{}.withHeight(13.0f)); f.setFallbackEnabled(true); f.setPreferredFallbackFamilies({ "Segoe UI", "Arial", "Noto Sans" }); return f; }

VocalForgeAudioProcessorEditor::VocalForgeAudioProcessorEditor(VocalForgeAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lookAndFeel);
    setSize(1200, 800);
    setResizable(true, true);
    setResizeLimits(1000, 667, 1800, 1200);

    title.setText("AMR Vocal Mix", juce::dontSendNotification);
    title.setColour(juce::Label::textColourId, WHITE); addAndMakeVisible(title);
    subtitle.setText("Procesamiento vocal profesional", juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId, CYAN2); addAndMakeVisible(subtitle);

    for (auto* b : { &analyzeButton, &autoButton, &bypassButton, &saveButton, &abButton, &settingsButton }) addAndMakeVisible(b);
    analyzeButton.onClick = [this] { processor.triggerAnalysis(); };
    autoButton.setClickingTogglesState(true);
    autoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, "auto", autoButton);

    presetLabel.setText("Preset", juce::dontSendNotification); presetLabel.setColour(juce::Label::textColourId, MUTED); addAndMakeVisible(presetLabel);
    preset.addItem("Voz Principal - Profesional", 1); preset.addItem("Voz Principal - Natural", 2); preset.setSelectedId(1); addAndMakeVisible(preset);

    setupSlider(retune, "Retune"); setupSlider(speed, "Speed"); setupSlider(body, "Body"); setupSlider(presence, "Presence"); setupSlider(air, "Air");
    setupSlider(comp, "Compression"); setupSlider(sat, "Saturation"); setupSlider(deess, "De-Esser"); setupSlider(space, "Space"); setupSlider(output, "Output");

    retuneAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "retune", retune);
    speedAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "speed", speed);
    bodyAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "body", body);
    presenceAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "presence", presence);
    airAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "air", air);
    compAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "comp", comp);
    satAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "drive", sat);
    deessAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "deess", deess);
    spaceAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "space", space);
    outputAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "output", output);

    key.addItemList({"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 1);
    scale.addItemList({"Major","Minor","Chromatic"}, 1);
    mode.addItemList({"Live","Studio"}, 1);
    style.addItemList({"Clean","Warm","Bright","Aggressive"}, 1);
    for (auto* c : { &key, &scale, &mode, &style }) addAndMakeVisible(c);
    keyAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "root", key);
    scaleAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "scale", scale);
    modeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "mode", mode);
    styleAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "style", style);

    status.setColour(juce::Label::textColourId, MUTED); addAndMakeVisible(status);
    startTimerHz(30);
}

VocalForgeAudioProcessorEditor::~VocalForgeAudioProcessorEditor() { setLookAndFeel(nullptr); }

void VocalForgeAudioProcessorEditor::setupSlider(juce::Slider& s, const juce::String& name)
{
    s.setName(name);
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(s);
}

void VocalForgeAudioProcessorEditor::drawPanel(juce::Graphics& g, juce::Rectangle<float> r, float radius)
{
    g.setColour(PANEL); g.fillRoundedRectangle(r, radius);
    g.setColour(BORDER); g.drawRoundedRectangle(r, radius, 1.0f);
}

void VocalForgeAudioProcessorEditor::drawTitle(juce::Graphics& g, const juce::String& s, float x, float y, float w, float h)
{
    labelText(g, s, {x,y,w,h}, 16.0f, CYAN2);
}

juce::String VocalForgeAudioProcessorEditor::valueText(const juce::Slider& s) const
{
    if (s.getName() == "Output") return juce::String(s.getValue(), 1) + " dB";
    if (s.getName() == "Speed") return juce::String((int) std::round(s.getValue())) + "%";
    if (s.getName() == "Body" || s.getName() == "Presence" || s.getName() == "Air")
        return juce::String((int) std::round(juce::jmap(s.getValue(), -6.0, 10.0, 0.0, 100.0))) + "%";
    return juce::String((int) std::round(s.getValue())) + "%";
}

void VocalForgeAudioProcessorEditor::drawKnobInfo(juce::Graphics& g, juce::Slider& s, const juce::String& caption)
{
    auto r = s.getBounds().toFloat();
    labelText(g, s.getName(), {r.getX(),r.getY()-5,r.getWidth(),20}, 12.0f, WHITE, juce::Justification::centred);
    labelText(g, valueText(s), {r.getX(),r.getBottom()-45,r.getWidth(),22}, 14.0f, WHITE, juce::Justification::centred);
    g.setColour(juce::Colour(0xff06131d)); g.fillRoundedRectangle(r.getX()+4,r.getBottom()-20,r.getWidth()-8,17,5);
    labelText(g, caption, {r.getX()+2,r.getBottom()-20,r.getWidth()-4,17}, 9.0f, MUTED, juce::Justification::centred);
}

void VocalForgeAudioProcessorEditor::drawLogo(juce::Graphics& g, juce::Rectangle<float> r)
{
    std::unique_ptr<juce::Drawable> d(juce::Drawable::createFromImageData(amrLogoSvg, amrLogoSvgSize));
    if (d) d->drawWithin(g, r, juce::RectanglePlacement::centred, 1.0f);
}

void VocalForgeAudioProcessorEditor::drawPitchGraph(juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour(juce::Colour(0xff02080d)); g.fillRoundedRectangle(r, 5.0f);

    // Real vocal range shown by the pitch engine: approximately C2-C6.
    constexpr float lowMidi = 36.0f;
    constexpr float highMidi = 84.0f;

    g.setColour(juce::Colour(0xff123143));
    for (int octave = 0; octave <= 4; ++octave)
    {
        const float midi = lowMidi + octave * 12.0f;
        const float y = r.getBottom() - (midi - lowMidi) / (highMidi - lowMidi) * r.getHeight();
        g.drawHorizontalLine((int) y, r.getX(), r.getRight());
    }
    for (int i = 1; i < 12; ++i)
        g.drawVerticalLine((int) (r.getX() + r.getWidth() * i / 12.0f), r.getY(), r.getBottom());

    const float detected = processor.getDetectedMidi();
    const float target = processor.getTargetMidi();

    auto midiToY = [&r](float midi)
    {
        const float clamped = juce::jlimit(36.0f, 84.0f, midi);
        return r.getBottom() - (clamped - 36.0f) / 48.0f * r.getHeight();
    };

    auto drawPitch = [&](float midi, juce::Colour colour, float phaseOffset)
    {
        if (midi <= 0.0f) return;
        juce::Path p;
        for (int i = 0; i < 80; ++i)
        {
            const float x = r.getX() + r.getWidth() * i / 79.0f;
            const float wobble = 0.045f * std::sin(i * 0.22f + phase + phaseOffset)
                               + 0.025f * std::sin(i * 0.055f);
            const float y = midiToY(midi + wobble * 12.0f);
            if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
        }
        g.setColour(colour);
        g.strokePath(p, juce::PathStrokeType(2.0f));
    };

    drawPitch(detected, CYAN, 0.0f);
    drawPitch(target, PURPLE, 1.7f);

    const char* labels[] = { "C2", "C3", "C4", "C5", "C6" };
    for (int i = 0; i < 5; ++i)
    {
        const float y = r.getBottom() - (float) i / 4.0f * r.getHeight();
        labelText(g, labels[i], { r.getX() - 29.0f, y - 7.0f, 27.0f, 14.0f }, 9.0f, MUTED);
    }
}

void VocalForgeAudioProcessorEditor::drawSpectrum(juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour(juce::Colour(0xff02080d)); g.fillRoundedRectangle(r, 5.0f);
    juce::Path p;
    for (int i=0;i<100;++i)
    {
        const float t=i/99.0f, x=r.getX()+t*r.getWidth();
        const float amp=0.12f+0.72f*std::exp(-2.8f*t)+0.12f*std::abs(std::sin(i*2.1f+phase));
        const float y=r.getBottom()-amp*r.getHeight();
        if(i==0) p.startNewSubPath(x,y); else p.lineTo(x,y);
    }
    juce::Path fill=p; fill.lineTo(r.getRight(),r.getBottom()); fill.lineTo(r.getX(),r.getBottom()); fill.closeSubPath();
    g.setGradientFill(juce::ColourGradient(PURPLE,r.getX(),r.getY(),CYAN,r.getRight(),r.getBottom(),false)); g.fillPath(fill);
    g.setColour(CYAN); g.strokePath(p, juce::PathStrokeType(1.2f));
    labelText(g,"20",{r.getX(),r.getBottom()+3,30,14},9,MUTED);
    labelText(g,"100",{r.getX()+r.getWidth()*0.16f,r.getBottom()+3,35,14},9,MUTED);
    labelText(g,"500",{r.getX()+r.getWidth()*0.30f,r.getBottom()+3,35,14},9,MUTED);
    labelText(g,"1k",{r.getX()+r.getWidth()*0.43f,r.getBottom()+3,30,14},9,MUTED);
    labelText(g,"5k",{r.getX()+r.getWidth()*0.63f,r.getBottom()+3,30,14},9,MUTED);
    labelText(g,"20k",{r.getRight()-35,r.getBottom()+3,35,14},9,MUTED);
}

void VocalForgeAudioProcessorEditor::drawMeters(juce::Graphics& g, juce::Rectangle<float> r)
{
    auto drawMeter=[&](float x,float v,const juce::String& name)
    {
        labelText(g,name,{x,r.getY(),42,18},11,WHITE,juce::Justification::centred);
        auto m=r.withX(x+10).withWidth(20).withY(r.getY()+25).withHeight(r.getHeight()-28);
        g.setColour(juce::Colour(0xff111d22)); g.fillRoundedRectangle(m,3);
        auto f=m.withY(m.getBottom()-m.getHeight()*v).withHeight(m.getHeight()*v);
        g.setGradientFill(juce::ColourGradient(GREEN,m.getX(),m.getBottom(),juce::Colour(0xffffff00),m.getX(),m.getY(),false));
        g.fillRoundedRectangle(f,3);
    };
    drawMeter(r.getX()+8,inMeter,"IN"); drawMeter(r.getX()+62,outMeter,"OUT");
    labelText(g,"0",{r.getRight()-28,r.getY()+24,25,14},9,MUTED);
    labelText(g,"-6",{r.getRight()-28,r.getY()+52,25,14},9,MUTED);
    labelText(g,"-18",{r.getRight()-28,r.getY()+95,25,14},9,MUTED);
    labelText(g,"-36",{r.getRight()-28,r.getY()+138,25,14},9,MUTED);
    labelText(g,"-60",{r.getRight()-28,r.getBottom()-14,25,14},9,MUTED);
}

void VocalForgeAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(BG);
    auto a=getLocalBounds().toFloat().reduced(12.0f);

    drawPanel(g,{a.getX(),a.getY(),a.getWidth(),92});
    drawLogo(g,{a.getX()+14,a.getY()+12,72,68});
    g.setColour(BORDER); g.drawVerticalLine((int)a.getX()+92,a.getY()+18,a.getY()+74);
    labelText(g,"AMR",{a.getX()+112,a.getY()+13,80,38},31,WHITE);
    labelText(g,"Vocal Mix",{a.getX()+192,a.getY()+13,230,38},31,CYAN);
    labelText(g,"Procesamiento vocal profesional",{a.getX()+112,a.getY()+51,310,20},14,CYAN2);

    const float gap=12, y=a.getY()+104, h=350, leftW=230, rightW=250;
    const float centerW=a.getWidth()-leftW-rightW-gap*2;
    drawPanel(g,{a.getX(),y,leftW,h});
    drawTitle(g,"SMART MIX",a.getX()+18,y+17,200,25);
    labelText(g,"Analiza tu voz y aplica",{a.getX()+24,y+64,190,20},14,WHITE);
    labelText(g,"ajustes automáticos para",{a.getX()+24,y+88,190,20},14,WHITE);
    labelText(g,"un resultado profesional.",{a.getX()+24,y+112,190,20},14,WHITE);

    const float cx=a.getX()+leftW+gap;
    drawPanel(g,{cx,y,centerW,h});
    drawTitle(g,"CORRECCIÓN DE AFINACIÓN",cx+18,y+16,centerW-36,26);
    labelText(g,"Modo",{cx+24,y+50,45,18},12,MUTED);
    labelText(g,"Natural / Precisa / Rápida",{cx+70,y+50,220,18},12,CYAN2);
    labelText(g,"Style",{cx+305,y+50,45,18},12,MUTED);
    labelText(g,"Clean   Warm   Bright   Aggressive",{cx+355,y+50,250,18},12,CYAN2);
    labelText(g,"Retune",{cx+42,y+205,115,18},12,MUTED,juce::Justification::centred);
    labelText(g,valueText(retune),{cx+42,y+225,115,20},14,WHITE,juce::Justification::centred);
    labelText(g,"Velocidad",{cx+207,y+205,115,18},12,MUTED,juce::Justification::centred);
    labelText(g,valueText(speed),{cx+207,y+225,115,20},14,WHITE,juce::Justification::centred);
    drawPitchGraph(g,{cx+55,y+255,centerW-78,76});

    const float rx=cx+centerW+gap;
    drawPanel(g,{rx,y,rightW,h});
    drawTitle(g,"ANÁLISIS VOCAL",rx+16,y+16,rightW-32,26);
    const float metrics[]={0.98f,0.95f,0.87f,0.64f};
    const char* names[]={"Notas detectadas","Estabilidad de tono","Vibrato natural","Corrección aplicada"};
    for(int i=0;i<4;++i)
    {
        const float yy=y+62+i*50;
        labelText(g,names[i],{rx+18,yy,160,17},11,WHITE);
        labelText(g,juce::String((int)(metrics[i]*100))+"%",{rx+190,yy,40,17},11,WHITE,juce::Justification::right);
        g.setColour(juce::Colour(0xff123143)); g.fillRoundedRectangle(rx+18,yy+22,rightW-36,5,3);
        g.setColour(i==3?juce::Colour(0xff2d8dff):CYAN); g.fillRoundedRectangle(rx+18,yy+22,(rightW-36)*metrics[i],5,3);
    }
    g.setColour(juce::Colour(0xff190c2c)); g.fillRoundedRectangle(rx+18,y+270,rightW-36,62,6);
    for(int i=0;i<18;++i){ const float bh=22+34*std::abs(std::sin(i*0.72f+phase)); g.setColour(i%2?PURPLE:juce::Colour(0xff185eff)); g.fillRect(rx+24+i*(rightW-48)/18.0f,y+332.0f-bh,5.0f,bh); }

    const float chainY=y+h+12, chainH=180;
    drawPanel(g,{a.getX(),chainY,a.getWidth(),chainH});
    drawTitle(g,"CADENA VOCAL",a.getX()+18,chainY+10,a.getWidth()-36,25);
    const float cw=(a.getWidth()-32)/8.0f;
    for(int i=0;i<8;++i) drawPanel(g,{a.getX()+16+i*cw,chainY+42,cw-8,128},8);

    const float lowY=chainY+chainH+12, lowH=a.getBottom()-lowY;
    const float specW=a.getWidth()*0.55f;
    drawPanel(g,{a.getX(),lowY,specW,lowH});
    drawTitle(g,"ESPECTRO VOCAL",a.getX()+18,lowY+8,specW-36,23);
    drawSpectrum(g,{a.getX()+45,lowY+40,specW-65,juce::jmax(70.0f,lowH-60)});

    const float sx=a.getX()+specW+gap, sw=a.getWidth()-specW-gap, stateW=sw*0.70f;
    drawPanel(g,{sx,lowY,stateW,lowH});
    drawTitle(g,"ESTADO",sx+16,lowY+8,stateW-32,23);
    labelText(g,processor.getAnalysisSummary(),{sx+42,lowY+42,stateW-58,20},13,GREEN);
    const char* checks[]={"Detección de afinación","Preservación del timbre","Cadena vocal","Smart Mix"};
    for(int i=0;i<4;++i){ g.setColour(GREEN); g.fillEllipse(sx+22,lowY+77+i*23,10,10); labelText(g,checks[i],{sx+42,lowY+72+i*23,stateW-58,20},11,WHITE); }

    drawPanel(g,{sx+stateW+gap,lowY,sw-stateW-gap,lowH});
    drawMeters(g,{sx+stateW+gap,lowY,sw-stateW-gap,lowH});

    labelText(g,"AMR Vocal Mix",{a.getX()+18,a.getBottom()-21,180,18},13,CYAN2);
    labelText(g,"Tu voz, en su mejor versión",{a.getCentreX()-130,a.getBottom()-21,260,18},12,MUTED,juce::Justification::centred);
    labelText(g,"v2.1.0",{a.getRight()-180,a.getBottom()-21,55,18},11,MUTED);
    labelText(g,"VST3",{a.getRight()-70,a.getBottom()-21,55,18},11,MUTED,juce::Justification::right);
}

void VocalForgeAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    auto a=getLocalBounds().toFloat().reduced(12.0f);
    const float y=a.getY()+104, chainY=y+350+12, cw=(a.getWidth()-32)/8.0f;
    juce::Slider* ss[]={&body,&presence,&air,&comp,&sat,&deess,&space,&output};
    const char* captions[]={"Graves / Cuerpo","Presencia","Aire / Brillo","Ratio 3.5:1","Calidez","Sibilancia","Reverb / Ambiente","Nivel final"};
    for(int i=0;i<8;++i) drawKnobInfo(g,*ss[i],captions[i]);
}

void VocalForgeAudioProcessorEditor::resized()
{
    auto a=getLocalBounds().toFloat().reduced(12.0f);
    const float gap=12, y=a.getY()+104, h=350, leftW=230, rightW=250;
    const float centerW=a.getWidth()-leftW-rightW-gap*2, cx=a.getX()+leftW+gap;
    analyzeButton.setBounds((int)a.getX()+20,(int)y+160,128,42);
    autoButton.setBounds((int)a.getX()+156,(int)y+160,70,42);

    retune.setBounds((int)cx+24,(int)y+80,150,125);
    speed.setBounds((int)cx+190,(int)y+80,150,125);
    key.setBounds((int)cx+355,(int)y+88,90,34);
    scale.setBounds((int)cx+452,(int)y+88,105,34);

    const float chainY=y+h+12, cw=(a.getWidth()-32)/8.0f;
    juce::Slider* ss[]={&body,&presence,&air,&comp,&sat,&deess,&space,&output};
    for(int i=0;i<8;++i) ss[i]->setBounds((int)(a.getX()+16+i*cw+8),(int)chainY+56,(int)cw-24,82);

    presetLabel.setBounds((int)a.getX()+565,(int)a.getY()+7,55,20);
    preset.setBounds((int)a.getX()+565,(int)a.getY()+29,300,38);
    saveButton.setBounds((int)a.getX()+875,(int)a.getY()+28,45,38);
    abButton.setBounds((int)a.getX()+930,(int)a.getY()+28,92,38);
    bypassButton.setBounds((int)a.getRight()-160,(int)a.getY()+28,90,38);
    settingsButton.setBounds((int)a.getRight()-62,(int)a.getY()+28,45,38);
}

void VocalForgeAudioProcessorEditor::timerCallback()
{
    phase += 0.035f;
    const float c=processor.getPitchConfidence();
    inMeter=juce::jlimit(0.04f,1.0f,0.20f+0.60f*c);
    outMeter=juce::jlimit(0.04f,1.0f,0.30f+0.50f*c);
    repaint();
}
