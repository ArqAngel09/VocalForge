#include "PluginEditor.h"

VocalForgeAudioProcessorEditor::VocalForgeAudioProcessorEditor(VocalForgeAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(760, 470);
    title.setText("VOCALFORGE", juce::dontSendNotification);
    title.setFont(juce::Font(28.0f, juce::Font::bold));
    title.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(title);

    subtitle.setText("Adaptive vocal mixing assistant", juce::dontSendNotification);
    subtitle.setFont(juce::Font(14.0f));
    addAndMakeVisible(subtitle);

    analyzeButton.onClick = [this] { processor.triggerAnalysis(); };
    addAndMakeVisible(analyzeButton);
    autoButton.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(autoButton);
    autoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, "auto", autoButton);

    setupSlider(body, "Body"); setupSlider(presence, "Presence"); setupSlider(air, "Air"); setupSlider(comp, "Compression");
    setupSlider(sat, "Saturation"); setupSlider(deess, "De-Esser"); setupSlider(space, "Space"); setupSlider(output, "Output");

    bodyAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "body", body);
    presenceAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "presence", presence);
    airAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "air", air);
    compAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "comp", comp);
    satAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "drive", sat);
    deessAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "deess", deess);
    spaceAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "space", space);
    outputAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "output", output);

    addAndMakeVisible(status);
    status.setJustificationType(juce::Justification::centredRight);
    status.setFont(juce::Font(13.0f));
    startTimerHz(15);
}

void VocalForgeAudioProcessorEditor::setupSlider(juce::Slider& s, const juce::String&)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    addAndMakeVisible(s);
}

void VocalForgeAudioProcessorEditor::timerCallback()
{
    status.setText(processor.getAnalysisSummary(), juce::dontSendNotification);
    repaint();
}

void VocalForgeAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0c0d10));
    g.setColour(juce::Colour(0xff17191e));
    g.fillRoundedRectangle(18, 18, getWidth()-36, 110, 18.0f);
    g.setColour(juce::Colour(0xff20242b));
    g.fillRoundedRectangle(18, 144, getWidth()-36, 300, 18.0f);

    g.setColour(juce::Colour(0xffe9edf3));
    g.setFont(juce::Font(11.0f));
    const juce::String names[] = { "BODY", "PRESENCE", "AIR", "COMPRESSION", "SATURATION", "DE-ESSER", "SPACE", "OUTPUT" };
    juce::Slider* sliders[] = { &body,&presence,&air,&comp,&sat,&deess,&space,&output };
    for (int i = 0; i < 8; ++i)
        g.drawFittedText(names[i], sliders[i]->getX(), sliders[i]->getY()-2, sliders[i]->getWidth(), 18, juce::Justification::centred, 1);

    const auto p = processor.getAnalysisProgress();
    g.setColour(juce::Colour(0xff30343d));
    g.fillRoundedRectangle(36, 112, 430, 6, 3.0f);
    g.setColour(juce::Colour(0xff7ee787));
    g.fillRoundedRectangle(36, 112, 430 * p, 6, 3.0f);
}

void VocalForgeAudioProcessorEditor::resized()
{
    title.setBounds(36, 32, 300, 34);
    subtitle.setBounds(38, 70, 300, 24);
    analyzeButton.setBounds(500, 36, 220, 40);
    autoButton.setBounds(500, 82, 220, 26);
    status.setBounds(500, 104, 220, 22);

    const int left = 36, top = 185, w = 160, h = 110, gapX = 12, gapY = 30;
    juce::Slider* s[] = { &body,&presence,&air,&comp,&sat,&deess,&space,&output };
    for (int i = 0; i < 8; ++i)
    {
        int col = i % 4, row = i / 4;
        s[i]->setBounds(left + col * (w + gapX), top + row * (h + gapY), w, h);
    }
}