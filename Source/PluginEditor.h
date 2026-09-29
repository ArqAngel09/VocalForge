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
    void setupCombo(juce::ComboBox&, const juce::StringArray&);
    void drawPanel(juce::Graphics&, juce::Rectangle<float>, float = 9.0f);
    void drawTitle(juce::Graphics&, const juce::String&, float, float, float, float);
    void drawKnobInfo(juce::Graphics&, juce::Slider&, const juce::String&);
    void drawPitchGraph(juce::Graphics&, juce::Rectangle<float>);
    void drawSpectrum(juce::Graphics&, juce::Rectangle<float>);
    void drawMeters(juce::Graphics&, juce::Rectangle<float>);
    void drawAssistRing(juce::Graphics&, juce::Rectangle<float>);
    void drawEqGraph(juce::Graphics&, juce::Rectangle<float>);
    void drawLogo(juce::Graphics&, juce::Rectangle<float>);
    juce::String valueText(const juce::Slider&) const;
    void setPage(int);
    void setModule(int);
    void layoutKnobGrid(std::initializer_list<juce::Slider*>);

    VocalForgeAudioProcessor& processor;
    AMRVocalLookAndFeel lookAndFeel;

    juce::TextButton analyzeButton { "ESCUCHAR" }, autoButton { "AUTO" };
    juce::TextButton vocalAssistTab { "VOCAL ASSIST" }, simpleTab { "SIMPLE" }, advancedTab { "ADVANCED" };
    juce::TextButton bypassButton { "BYPASS" }, compareButton { "A/B" }, retryButton { "REINICIAR" }, autoGainButton { "AUTO GAIN" };
    juce::TextButton dynamicsTab { "DYNAMICS" }, eqTab { "EQ" }, spaceTab { "SPACE" }, characterTab { "CHARACTER" }, graphTab { "EQ GRAPH" };
    juce::Label title, subtitle, status, presetLabel;
    juce::ComboBox preset, key, scale, mode, style, reverbType, delaySubdivision;

    juce::Slider retune, speed, body, presence, air, comp, sat, deess, space, delay, output;
    juce::Slider magic, color, eqLow, eqLowMid, eqHighMid, eqHigh, spaceTime, exciter, doubler, denoise, resonance, multiband;
    juce::Slider delayFeedback, delayTone, deessFocus, compAttack, compRelease;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoAttach, bypassAttach, autoGainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        retuneAttach, speedAttach, bodyAttach, presenceAttach, airAttach, compAttach, satAttach, deessAttach,
        spaceAttach, delayAttach, outputAttach, magicAttach, colorAttach, eqLowAttach, eqLowMidAttach,
        eqHighMidAttach, eqHighAttach, spaceTimeAttach, exciterAttach, doublerAttach, denoiseAttach,
        resonanceAttach, multibandAttach, delayFeedbackAttach, delayToneAttach, deessFocusAttach,
        compAttackAttach, compReleaseAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        keyAttach, scaleAttach, modeAttach, styleAttach, reverbTypeAttach, delaySubdivisionAttach;

    int activePage = 0;
    int activeModule = 0;
    float phase = 0.0f;
    float inMeter = 0.0f, outMeter = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalForgeAudioProcessorEditor)
};