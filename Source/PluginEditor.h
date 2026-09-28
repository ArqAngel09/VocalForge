#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class AMRVocalLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AMRVocalLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
};

class VocalForgeAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VocalForgeAudioProcessorEditor(VocalForgeAudioProcessor&);
    ~VocalForgeAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupSlider(juce::Slider&, const juce::String&);
    void drawPanel(juce::Graphics&, juce::Rectangle<float>, float = 9.0f);
    void drawTitle(juce::Graphics&, const juce::String&, float, float, float, float);
    void drawKnobInfo(juce::Graphics&, juce::Slider&, const juce::String&);
    void drawPitchGraph(juce::Graphics&, juce::Rectangle<float>);
    void drawSpectrum(juce::Graphics&, juce::Rectangle<float>);
    void drawMeters(juce::Graphics&, juce::Rectangle<float>);
    void drawLogo(juce::Graphics&, juce::Rectangle<float>);
    juce::String valueText(const juce::Slider&) const;

    VocalForgeAudioProcessor& processor;
    AMRVocalLookAndFeel lookAndFeel;

    juce::TextButton analyzeButton { "Analizar" }, autoButton { "Auto" };
    juce::TextButton bypassButton { "Bypass" }, saveButton { "▣" }, abButton { "A | B" }, settingsButton { "⚙" };
    juce::Label title, subtitle, status;
    juce::Label presetLabel;
    juce::ComboBox preset, key, scale, mode, style;
    juce::Slider retune, speed, body, presence, air, comp, sat, deess, space, output;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> retuneAttach, speedAttach, bodyAttach, presenceAttach, airAttach, compAttach, satAttach, deessAttach, spaceAttach, outputAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAttach, scaleAttach, modeAttach, styleAttach;

    float phase = 0.0f;
    float inMeter = 0.2f, outMeter = 0.35f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalForgeAudioProcessorEditor)
};
