#include "PluginEditor.h"

VocalForgeAudioProcessorEditor::VocalForgeAudioProcessorEditor(VocalForgeAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(900, 590);

    title.setText("VOCALFORGE 2.0", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions{}.withHeight(28.0f).withStyle("Bold")));
    addAndMakeVisible(title);

    subtitle.setText("Natural pitch correction • adaptive vocal finishing", juce::dontSendNotification);
    subtitle.setFont(juce::Font(juce::FontOptions{}.withHeight(14.0f)));
    addAndMakeVisible(subtitle);

    analyzeButton.onClick = [this] { processor.triggerAnalysis(); };
    addAndMakeVisible(analyzeButton);

    autoButton.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(autoButton);
    autoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, "auto", autoButton);

    setupSlider(retune, "Retune");
    setupSlider(speed, "Speed");
    setupSlider(body, "Body");
    setupSlider(presence, "Presence");
    setupSlider(air, "Air");
    setupSlider(comp, "Compression");
    setupSlider(sat, "Saturation");
    setupSlider(deess, "De-Esser");
    setupSlider(space, "Space");
    setupSlider(output, "Output");

    key.addItemList({ "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" }, 1);
    scale.addItemList({ "Major","Minor","Chromatic" }, 1);
    mode.addItemList({ "Live","Studio" }, 1);
    style.addItemList({ "Clean","Warm","Bright","Aggressive" }, 1);
    for (auto* c : { &key, &scale, &mode, &style }) addAndMakeVisible(c);

    keyAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "root", key);
    scaleAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "scale", scale);
    modeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "mode", mode);
    styleAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "style", style);

    pitchLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(15.0f).withStyle("Bold")));
    addAndMakeVisible(pitchLabel);
    keyLabel.setText("KEY / MODE", juce::dontSendNotification);
    keyLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
    addAndMakeVisible(keyLabel);
    addAndMakeVisible(status);

    startTimerHz(20);
}

void VocalForgeAudioProcessorEditor::setupSlider(juce::Slider& s, const juce::String&)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 74, 20);
    addAndMakeVisible(s);
}

void VocalForgeAudioProcessorEditor::timerCallback()
{
    const float d = processor.getDetectedMidi();
    const float t = processor.getTargetMidi();
    const float c = processor.getPitchConfidence();

    if (c > 0.18f && d > 0.0f)
        pitchLabel.setText("PITCH  " + juce::String(d, 1) + " → " + juce::String(t, 1) + " MIDI", juce::dontSendNotification);
    else
        pitchLabel.setText("PITCH  listening…", juce::dontSendNotification);

    status.setText(processor.getAnalysisSummary(), juce::dontSendNotification);
    repaint();
}

void VocalForgeAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0b0d10));
    g.setColour(juce::Colour(0xff15181d));
    g.fillRoundedRectangle(18, 18, (float)getWidth()-36, 128.0f, 18.0f);
    g.setColour(juce::Colour(0xff1b1f25));
    g.fillRoundedRectangle(18, 160, (float)getWidth()-36, (float)getHeight()-178, 18.0f);

    g.setColour(juce::Colour(0xffe8edf3));
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
    const juce::String names[] = { "RETUNE","SPEED","BODY","PRESENCE","AIR","COMPRESSION","SATURATION","DE-ESSER","SPACE","OUTPUT" };
    juce::Slider* ss[] = { &retune,&speed,&body,&presence,&air,&comp,&sat,&deess,&space,&output };
    for (int i = 0; i < 10; ++i)
        g.drawFittedText(names[i], ss[i]->getX(), ss[i]->getY()-2, ss[i]->getWidth(), 18, juce::Justification::centred, 1);

    g.setColour(juce::Colour(0xff30353e));
    g.fillRoundedRectangle(36, 132, 430, 6, 3.0f);
    g.setColour(juce::Colour(0xff7ee787));
    g.fillRoundedRectangle(36, 132, 430 * processor.getAnalysisProgress(), 6, 3.0f);
}

void VocalForgeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(28);

    title.setBounds(area.removeFromTop(38));
    subtitle.setBounds(area.removeFromTop(24));
    auto header = area.removeFromTop(52);
    analyzeButton.setBounds(header.removeFromLeft(210).reduced(4));
    autoButton.setBounds(header.removeFromLeft(190).reduced(4));
    pitchLabel.setBounds(header.removeFromLeft(250).reduced(4));
    status.setBounds(header.reduced(4));

    auto choices = area.removeFromTop(38);
    key.setBounds(choices.removeFromLeft(150).reduced(4));
    scale.setBounds(choices.removeFromLeft(150).reduced(4));
    mode.setBounds(choices.removeFromLeft(150).reduced(4));
    style.setBounds(choices.removeFromLeft(150).reduced(4));

    auto top = area.removeFromTop(190);
    auto bottom = area;
    juce::Slider* topSliders[] = { &retune,&speed,&body,&presence,&air };
    juce::Slider* bottomSliders[] = { &comp,&sat,&deess,&space,&output };

    const int topWidth = top.getWidth() / 5;
    const int bottomWidth = bottom.getWidth() / 5;
    for (int i = 0; i < 5; ++i)
    {
        topSliders[i]->setBounds(top.removeFromLeft(topWidth).reduced(8));
        bottomSliders[i]->setBounds(bottom.removeFromLeft(bottomWidth).reduced(8));
    }
}
