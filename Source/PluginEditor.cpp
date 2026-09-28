#include "PluginEditor.h"
#include "AMRLogo.h"

namespace
{
    const juce::Colour BG(0xff0b1117), PANEL(0xff111b24), PANEL2(0xff17232d);
    const juce::Colour BORDER(0xff2b3e4d), CYAN(0xff19c8ff), CYAN2(0xff8de7ff);
    const juce::Colour WHITE(0xfff3f7fa), MUTED(0xff9eabb5), PURPLE(0xff9b5cff), GREEN(0xff39e58c);
    
    void text(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size,
              juce::Colour c, juce::Justification j = juce::Justification::left)
    {
        auto f = juce::Font(juce::FontOptions{}.withHeight(size));
        f.setFallbackEnabled(true);
        f.setPreferredFallbackFamilies({ "Segoe UI", "Arial", "Noto Sans" });
        g.setFont(f);
        g.setColour(c);
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
    const float cx = x + w * .5f, cy = y + h * .45f;
    const float r = juce::jmin(w, h) * .30f;
    const float a = start + pos * (end - start);
    const auto accent = s.getName() == "Saturation" ? PURPLE : CYAN;

    g.setColour(juce::Colour(0xff0a1117)); g.fillEllipse(cx-r-5, cy-r-5, (r+5)*2, (r+5)*2);
    g.setColour(juce::Colour(0xff1d2b35)); g.fillEllipse(cx-r, cy-r, r*2, r*2);

    juce::Path track, value;
    track.addCentredArc(cx, cy, r+7, r+7, 0.0f, start, end, true);
    value.addCentredArc(cx, cy, r+7, r+7, 0.0f, start, a, true);
    g.setColour(juce::Colour(0xff31414c)); g.strokePath(track, juce::PathStrokeType(5.0f));
    g.setColour(accent); g.strokePath(value, juce::PathStrokeType(5.0f));

    g.setColour(CYAN2);
    g.drawLine(cx + std::cos(a)*(r-10), cy + std::sin(a)*(r-10),
               cx + std::cos(a)*(r-2),  cy + std::sin(a)*(r-2), 2.0f);
}

void AMRVocalLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b,
                                                const juce::Colour&, bool hover, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced(.5f);
    const bool active = b.getToggleState();
    g.setColour(active ? juce::Colour(0xff163a49) : (hover ? juce::Colour(0xff1b303c) : PANEL2));
    g.fillRoundedRectangle(r, 7.0f);
    g.setColour(active || down ? CYAN : BORDER);
    g.drawRoundedRectangle(r, 7.0f, active ? 1.5f : 1.0f);
}

void AMRVocalLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const bool active = b.getToggleState();
    text(g, b.getButtonText(), b.getLocalBounds().toFloat(), 12.0f, active ? CYAN2 : WHITE,
         juce::Justification::centred);
}

void AMRVocalLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(b.getToggleState() ? juce::Colour(0xff163a49) : PANEL2); g.fillRoundedRectangle(r, 7.0f);
    g.setColour(b.getToggleState() ? CYAN : BORDER); g.drawRoundedRectangle(r, 7.0f, 1.1f);
    text(g, b.getButtonText(), r, 12.0f, b.getToggleState() ? CYAN : MUTED, juce::Justification::centred);
}

void AMRVocalLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float>(0,0,(float)w,(float)h).reduced(.5f);
    g.setColour(PANEL2); g.fillRoundedRectangle(r, 6.0f);
    g.setColour(BORDER); g.drawRoundedRectangle(r, 6.0f, 1.0f);
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
    setSize(900, 560);
    setResizable(true, true);
    setResizeLimits(760, 480, 1500, 900);

    title.setText("AMR Vocal Mix", juce::dontSendNotification);
    title.setColour(juce::Label::textColourId, WHITE); addAndMakeVisible(title);
    subtitle.setText("Vocal processing", juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId, MUTED); addAndMakeVisible(subtitle);

    for (auto* b : { &vocalAssistTab, &simpleTab, &advancedTab, &analyzeButton, &autoButton, &bypassButton })
        addAndMakeVisible(b);

    for (auto* tab : { &vocalAssistTab, &simpleTab, &advancedTab })
        tab->setClickingTogglesState(true);

    vocalAssistTab.onClick = [this]{ setPage(0); };
    simpleTab.onClick = [this]{ setPage(1); };
    advancedTab.onClick = [this]{ setPage(2); };

    analyzeButton.onClick = [this]{ processor.triggerAnalysis(); };
    autoButton.setClickingTogglesState(true);
    autoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, "auto", autoButton);

    presetLabel.setText("Preset", juce::dontSendNotification);
    presetLabel.setColour(juce::Label::textColourId, MUTED); addAndMakeVisible(presetLabel);
    preset.addItem("Voz Principal - Profesional", 1);
    preset.addItem("Voz Principal - Natural", 2);
    preset.setSelectedId(1); addAndMakeVisible(preset);

    setupSlider(retune,"Retune"); setupSlider(speed,"Speed");
    setupSlider(body,"Body"); setupSlider(presence,"Presence"); setupSlider(air,"Air");
    setupSlider(comp,"Compression"); setupSlider(sat,"Saturation"); setupSlider(deess,"De-Esser");
    setupSlider(space,"Space"); setupSlider(output,"Output");

    retuneAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,"retune",retune);
    speedAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,"speed",speed);
    bodyAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,"body",body);
    presenceAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,"presence",presence);
    airAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,"air",air);
    compAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,"comp",comp);
    satAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,"drive",sat);
    deessAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,"deess",deess);
    spaceAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,"space",space);
    outputAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,"output",output);

    key.addItemList({"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"},1);
    scale.addItemList({"Major","Minor","Chromatic"},1);
    mode.addItemList({"Live","Studio"},1);
    style.addItemList({"Clean","Warm","Bright","Aggressive"},1);
    for (auto* c : { &key,&scale,&mode,&style }) addAndMakeVisible(c);
    keyAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts,"root",key);
    scaleAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts,"scale",scale);
    modeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts,"mode",mode);
    styleAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts,"style",style);

    status.setColour(juce::Label::textColourId, MUTED); addAndMakeVisible(status);
    setPage(0);
    startTimerHz(30);
}

VocalForgeAudioProcessorEditor::~VocalForgeAudioProcessorEditor() { setLookAndFeel(nullptr); }

void VocalForgeAudioProcessorEditor::setupSlider(juce::Slider& s, const juce::String& name)
{
    s.setName(name);
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    addAndMakeVisible(s);
}

void VocalForgeAudioProcessorEditor::setPage(int page)
{
    activePage = juce::jlimit(0,2,page);
    vocalAssistTab.setToggleState(activePage == 0, juce::dontSendNotification);
    simpleTab.setToggleState(activePage == 1, juce::dontSendNotification);
    advancedTab.setToggleState(activePage == 2, juce::dontSendNotification);

    const bool simple = activePage == 1;
    const bool advanced = activePage == 2;
    const bool controls = simple || advanced;

    analyzeButton.setVisible(true);
    autoButton.setVisible(activePage == 0 || simple);
    retune.setVisible(controls); speed.setVisible(controls);
    body.setVisible(advanced); presence.setVisible(advanced); air.setVisible(advanced);
    comp.setVisible(advanced); sat.setVisible(advanced); deess.setVisible(advanced);
    space.setVisible(advanced); output.setVisible(advanced);
    key.setVisible(advanced); scale.setVisible(advanced); mode.setVisible(advanced); style.setVisible(advanced);
    repaint();
}

void VocalForgeAudioProcessorEditor::drawLogo(juce::Graphics& g, juce::Rectangle<float> r)
{
    std::unique_ptr<juce::Drawable> d(juce::Drawable::createFromImageData(amrLogoSvg,amrLogoSvgSize));
    if (d) d->drawWithin(g,r,juce::RectanglePlacement::centred,1.0f);
}

void VocalForgeAudioProcessorEditor::drawPanel(juce::Graphics& g, juce::Rectangle<float> r, float radius)
{
    g.setColour(PANEL); g.fillRoundedRectangle(r,radius);
    g.setColour(BORDER); g.drawRoundedRectangle(r,radius,1.0f);
}

void VocalForgeAudioProcessorEditor::drawTitle(juce::Graphics& g,const juce::String& s,float x,float y,float w,float h)
{
    text(g,s,{x,y,w,h},14.0f,CYAN2);
}

juce::String VocalForgeAudioProcessorEditor::valueText(const juce::Slider& s) const
{
    if (s.getName()=="Output") return juce::String(s.getValue(),1)+" dB";
    if (s.getName()=="Speed") return juce::String((int)std::round(s.getValue()))+"%";
    if (s.getName()=="Body" || s.getName()=="Presence" || s.getName()=="Air")
        return juce::String((int)std::round(juce::jmap(s.getValue(),-6.0,10.0,0.0,100.0)))+"%";
    return juce::String((int)std::round(s.getValue()))+"%";
}

void VocalForgeAudioProcessorEditor::drawKnobInfo(juce::Graphics& g,juce::Slider& s,const juce::String& caption)
{
    auto r=s.getBounds().toFloat();
    text(g,s.getName(),{r.getX(),r.getY()-2,r.getWidth(),18},11,WHITE,juce::Justification::centred);
    text(g,valueText(s),{r.getX(),r.getBottom()-36,r.getWidth(),18},13,WHITE,juce::Justification::centred);
    text(g,caption,{r.getX(),r.getBottom()-18,r.getWidth(),16},8.5f,MUTED,juce::Justification::centred);
}

void VocalForgeAudioProcessorEditor::drawPitchGraph(juce::Graphics& g,juce::Rectangle<float> r)
{
    g.setColour(juce::Colour(0xff09131b)); g.fillRoundedRectangle(r,6.0f);
    constexpr float low=24.0f, high=96.0f;
    g.setColour(juce::Colour(0xff21333f));
    for(int i=0;i<=6;++i)
    {
        const float y=r.getBottom()-i*r.getHeight()/6.0f;
        g.drawHorizontalLine((int)y,r.getX(),r.getRight());
        text(g,"C"+juce::String(1+i),{r.getX()+6,y-9,28,18},9,MUTED);
    }
    const float detected=processor.getDetectedMidi(), target=processor.getTargetMidi();
    auto py=[&](float midi){ return r.getBottom()-juce::jlimit(24.0f,96.0f,midi-24.0f)/72.0f*r.getHeight(); };
    if(detected>0.0f)
    {
        juce::Path p;
        for(int i=0;i<70;++i)
        {
            const float x=r.getX()+i*r.getWidth()/69.0f;
            const float y=py(detected+0.05f*std::sin(i*.2f+phase)*12.0f);
            if(i==0)p.startNewSubPath(x,y);else p.lineTo(x,y);
        }
        g.setColour(CYAN); g.strokePath(p,juce::PathStrokeType(2.0f));
    }
    if(target>0.0f)
    {
        const float y=py(target);
        g.setColour(PURPLE); g.drawHorizontalLine((int)y,r.getX()+5,r.getRight()-5);
    }
}

void VocalForgeAudioProcessorEditor::drawSpectrum(juce::Graphics& g,juce::Rectangle<float> r)
{
    g.setColour(juce::Colour(0xff09131b)); g.fillRoundedRectangle(r,6.0f);
    juce::Path p;
    for(int i=0;i<90;++i)
    {
        const float t=i/89.0f, x=r.getX()+t*r.getWidth();
        const float a=.10f+.75f*std::exp(-3.0f*t)+.06f*std::abs(std::sin(i*1.8f+phase));
        const float y=r.getBottom()-a*r.getHeight();
        if(i==0)p.startNewSubPath(x,y);else p.lineTo(x,y);
    }
    g.setColour(CYAN); g.strokePath(p,juce::PathStrokeType(1.5f));
}

void VocalForgeAudioProcessorEditor::drawMeters(juce::Graphics& g,juce::Rectangle<float> r)
{
    auto meter=[&](float x,float v,const juce::String& name)
    {
        text(g,name,{x,r.getY(),42,16},9,MUTED,juce::Justification::centred);
        auto m=juce::Rectangle<float>(x+13,r.getY()+22,16,r.getHeight()-30);
        g.setColour(juce::Colour(0xff0b141a)); g.fillRoundedRectangle(m,3);
        g.setColour(GREEN); g.fillRoundedRectangle(m.withY(m.getBottom()-m.getHeight()*v).withHeight(m.getHeight()*v),3);
    };
    meter(r.getX(),inMeter,"IN"); meter(r.getX()+55,outMeter,"OUT");
}

void VocalForgeAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(BG);
    auto a=getLocalBounds().toFloat().reduced(14.0f);

    // Compact product header inspired by the supplied reference.
    drawPanel(g,{a.getX(),a.getY(),a.getWidth(),62},10);
    drawLogo(g,{a.getX()+10,a.getY()+7,48,48});
    text(g,"AMR", {a.getX()+66,a.getY()+10,55,26},22,WHITE);
    text(g,"Vocal Mix",{a.getX()+120,a.getY()+10,130,26},22,CYAN);
    text(g,"PRO VOCAL PROCESSOR",{a.getX()+66,a.getY()+35,180,14},9,MUTED);

    const float tabsW=310.0f;
    vocalAssistTab.setBounds((int)(a.getRight()-tabsW),(int)a.getY()+11,104,40);
    simpleTab.setBounds((int)(a.getRight()-tabsW+106),(int)a.getY()+11,90,40);
    advancedTab.setBounds((int)(a.getRight()-tabsW+198),(int)a.getY()+11,112,40);

    const float y=a.getY()+76;
    if(activePage==0)
    {
        drawPanel(g,{a.getX(),y,a.getWidth(),a.getHeight()-76},12);
        text(g,"VOCAL ENGINE",{a.getX()+26,y+22,180,22},15,WHITE);
        text(g,"Escucha una toma de voz y deja que AMR Vocal Mix configure el procesamiento.",{a.getX()+26,y+50,a.getWidth()-52,20},11,MUTED);

        const float cx=a.getCentreX(), cy=y+215, radius=112.0f;
        for(int i=0;i<32;++i)
        {
            const float ang=juce::MathConstants<float>::twoPi*i/32.0f;
            const float len=14.0f+24.0f*(.5f+.5f*std::sin(i*.7f+phase));
            g.setColour(i<18?CYAN:juce::Colour(0xff52616a));
            g.drawLine(cx+std::cos(ang)*(radius-len),cy+std::sin(ang)*(radius-len),
                       cx+std::cos(ang)*radius,cy+std::sin(ang)*radius,5.0f);
        }
        g.setColour(juce::Colour(0xff17242e)); g.fillEllipse(cx-78,cy-78,156,156);
        g.setColour(CYAN); g.drawEllipse(cx-78,cy-78,156,156,2.0f);
        g.setColour(juce::Colour(0xff0e1820)); g.fillEllipse(cx-61,cy-61,122,122);
        text(g,"15 s",{cx-30,cy-12,60,20},10,MUTED,juce::Justification::centred);

        const float progress=processor.getAnalysisProgress();
        text(g,juce::String((int)std::round(progress*100.0f))+"%",{cx-40,cy+130,80,24},18,WHITE,juce::Justification::centred);
        text(g,"Escuchar       Analizar       Listo",{cx-155,cy+158,310,18},10,MUTED,juce::Justification::centred);
        g.setColour(juce::Colour(0xff1d2b34)); g.fillRoundedRectangle(cx-175,cy+188,350,46,8);
        text(g,"Original",{cx-160,cy+201,100,18},11,CYAN2,juce::Justification::centred);
        text(g,"Asistida",{cx-55,cy+201,100,18},11,MUTED,juce::Justification::centred);
        text(g,"Relanzar",{cx+55,cy+201,100,18},11,WHITE,juce::Justification::centred);

        text(g,processor.getAnalysisSummary(),{a.getX()+26,a.getBottom()-38,a.getWidth()-52,20},11,GREEN,juce::Justification::centred);
    }
    else if(activePage==1)
    {
        drawPanel(g,{a.getX(),y,a.getWidth(),a.getHeight()-76},12);
        text(g,"QUICK",{a.getX()+26,y+22,120,22},15,WHITE);
        text(g,"Los controles esenciales para ajustar una voz rápidamente.",{a.getX()+26,y+48,360,18},11,MUTED);
        text(g,"AFINACIÓN",{a.getX()+45,y+82,130,18},10,CYAN2);
        text(g,"TONO",{a.getCentreX()+20,y+82,100,18},10,CYAN2);
        text(g,"MIX",{a.getRight()-210,y+82,100,18},10,CYAN2);
        drawPitchGraph(g,{a.getX()+28,y+106,a.getWidth()-56,72});
        drawTitle(g,"CONTROLES",{a.getX()+28,y+194,a.getWidth()-56,20});
        text(g,"Retune",{a.getX()+62,y+224,90,18},10,MUTED,juce::Justification::centred);
        text(g,valueText(retune),{a.getX()+62,y+306,90,18},12,WHITE,juce::Justification::centred);
        text(g,"Speed",{a.getX()+205,y+224,90,18},10,MUTED,juce::Justification::centred);
        text(g,valueText(speed),{a.getX()+205,y+306,90,18},12,WHITE,juce::Justification::centred);
        text(g,"Presence",{a.getCentreX()-45,y+224,90,18},10,MUTED,juce::Justification::centred);
        text(g,valueText(presence),{a.getCentreX()-45,y+306,90,18},12,WHITE,juce::Justification::centred);
        text(g,"Compression",{a.getCentreX()+100,y+224,90,18},10,MUTED,juce::Justification::centred);
        text(g,valueText(comp),{a.getCentreX()+100,y+306,90,18},12,WHITE,juce::Justification::centred);
        text(g,"Output",{a.getRight()-155,y+224,90,18},10,MUTED,juce::Justification::centred);
        text(g,valueText(output),{a.getRight()-155,y+306,90,18},12,WHITE,juce::Justification::centred);
    }
    else
    {
        drawPanel(g,{a.getX(),y,a.getWidth(),a.getHeight()-76},12);
        drawTitle(g,"PRO",a.getX()+22,y+18,150,22);
        text(g,"Afinación, tono y cadena vocal completa",{a.getX()+22,y+42,300,18},10,MUTED);
        drawPitchGraph(g,{a.getX()+22,y+68,a.getWidth()*.43f,78});
        drawSpectrum(g,{a.getX()+a.getWidth()*.46f,y+68,a.getWidth()*.50f,78});
        text(g,"KEY",{a.getX()+22,y+160,45,16},9,MUTED);
        text(g,"SCALE",{a.getX()+90,y+160,55,16},9,MUTED);
        text(g,"MODE",{a.getX()+175,y+160,55,16},9,MUTED);
        text(g,"STYLE",{a.getX()+260,y+160,55,16},9,MUTED);
        text(g,"CADENA VOCAL",{a.getX()+22,y+214,150,20},12,CYAN2);
        const float base=a.getX()+12, gap=6, w=(a.getWidth()-24-gap*7)/8.0f;
        juce::Slider* ss[]={&body,&presence,&air,&comp,&sat,&deess,&space,&output};
        const char* cap[]={"Cuerpo","Presencia","Aire","Comp.","Satur.","De-Esser","Espacio","Salida"};
        for(int i=0;i<8;++i)
        {
            const float x=base+i*(w+gap);
            text(g,cap[i],{x,y+238,w,18},9,MUTED,juce::Justification::centred);
            text(g,valueText(*ss[i]),{x,y+342,w,18},10,WHITE,juce::Justification::centred);
        }
        drawMeters(g,{a.getRight()-120,y+205,100,a.getHeight()-235});
    }

    const float headerRight=a.getRight()-tabsW;
    presetLabel.setBounds((int)(headerRight-170),(int)a.getY()+5,55,16);
    preset.setBounds((int)(headerRight-112),(int)a.getY()+10,112,40);
}

void VocalForgeAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    if(activePage==1)
    {
        drawKnobInfo(g,retune,"Correccion natural");
        drawKnobInfo(g,speed,"Velocidad de respuesta");
        drawKnobInfo(g,presence,"Claridad vocal");
        drawKnobInfo(g,comp,"Control dinamico");
        drawKnobInfo(g,output,"Nivel final");
    }
    else if(activePage==2)
    {
        juce::Slider* ss[]={&body,&presence,&air,&comp,&sat,&deess,&space,&output};
        const char* cap[]={"Cuerpo","Presencia","Aire","Compresion","Calidez","Sibilancia","Ambiente","Nivel final"};
        for(int i=0;i<8;++i) drawKnobInfo(g,*ss[i],cap[i]);
    }
}

void VocalForgeAudioProcessorEditor::resized()
{
    auto a=getLocalBounds().toFloat().reduced(14.0f);
    const float y=a.getY()+76;

    vocalAssistTab.setBounds((int)(a.getRight()-310),(int)a.getY()+11,104,40);
    simpleTab.setBounds((int)(a.getRight()-204),(int)a.getY()+11,90,40);
    advancedTab.setBounds((int)(a.getRight()-112),(int)a.getY()+11,112,40);

    analyzeButton.setBounds((int)a.getCentreX()-78,(int)(y+188),156,46);
    autoButton.setBounds((int)a.getCentreX()+120,(int)y+315,82,44);
    bypassButton.setBounds((int)a.getX()+20,(int)a.getY()+10,80,40);

    retune.setBounds((int)a.getX()+40,(int)y+205,135,105);
    speed.setBounds((int)a.getX()+183,(int)y+205,135,105);
    presence.setBounds((int)a.getCentreX()-68,(int)y+205,135,105);
    comp.setBounds((int)a.getCentreX()+76,(int)y+205,135,105);
    output.setBounds((int)a.getRight()-175,(int)y+205,135,105);

    body.setBounds((int)a.getX()+20,(int)y+240,82,96);
    air.setBounds((int)a.getX()+110,(int)y+240,82,96);
    sat.setBounds((int)a.getX()+200,(int)y+240,82,96);
    deess.setBounds((int)a.getX()+290,(int)y+240,82,96);
    space.setBounds((int)a.getX()+380,(int)y+240,82,96);
    
    key.setBounds((int)a.getX()+20,(int)y+175,58,30);
    scale.setBounds((int)a.getX()+82,(int)y+175,82,30);
    mode.setBounds((int)a.getX()+170,(int)y+175,76,30);
    style.setBounds((int)a.getX()+252,(int)y+175,92,30);

    presetLabel.setBounds((int)(a.getRight()-310-170),(int)a.getY()+5,55,16);
    preset.setBounds((int)(a.getRight()-310-112),(int)a.getY()+10,112,40);
    status.setBounds((int)a.getX()+20,(int)a.getBottom()-25,(int)a.getWidth()-40,18);
}

void VocalForgeAudioProcessorEditor::timerCallback()
{
    phase += .035f;
    const float c=processor.getPitchConfidence();
    inMeter=juce::jlimit(.04f,1.0f,.20f+.60f*c);
    outMeter=juce::jlimit(.04f,1.0f,.30f+.50f*c);
    repaint();
}
