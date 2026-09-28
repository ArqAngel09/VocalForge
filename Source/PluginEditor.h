#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class VocalForgeAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VocalForgeAudioProcessorEditor(VocalForgeAudioProcessor&);
    ~VocalForgeAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupSlider(juce::Slider&, const juce::String&);
    void setupChoice(juce::ComboBox&, const juce::String&);

    VocalForgeAudioProcessor& processor;
    juce::TextButton analyzeButton { "ANALYZE / SMART MIX" };
    juce::ToggleButton autoButton { "Auto after 15 seconds" };
    juce::Label title, subtitle, status, pitchLabel, keyLabel;
    juce::Slider retune, speed, body, presence, air, comp, sat, deess, space, output;
    juce::ComboBox key, scale, mode, style;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> retuneAttach, speedAttach, bodyAttach, presenceAttach, airAttach, compAttach, satAttach, deessAttach, spaceAttach, outputAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAttach, scaleAttach, modeAttach, styleAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalForgeAudioProcessorEditor)
};
