#pragma once
#include <JuceHeader.h>
#include "VocalPitchCorrector.h"

class VocalForgeAudioProcessor : public juce::AudioProcessor
{
public:
    VocalForgeAudioProcessor();
    ~VocalForgeAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "AMR Vocal Mix"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

    void triggerAnalysis();
    bool isAnalysisReady() { return analysisReady.exchange(false, std::memory_order_acq_rel); }
    float getAnalysisProgress() const;
    juce::String getAnalysisSummary() const;
    float getDetectedMidi() const noexcept { return pitchCorrector.getDetectedMidi(); }
    float getTargetMidi() const noexcept { return pitchCorrector.getTargetMidi(); }
    float getPitchConfidence() const noexcept { return pitchCorrector.getConfidence(); }

private:
    struct Analysis
    {
        double seconds = 0.0;
        double sumSq = 0.0;
        double peak = 0.0;
        double low = 0.0, mid = 0.0, high = 0.0;
        uint64_t samples = 0;
        void reset() { *this = {}; }
    };

    void analyseBlock(const juce::AudioBuffer<float>& buffer);
    void applySmartMix();

    double currentSampleRate = 44100.0;
    juce::dsp::IIR::Filter<float> hpFilterL, hpFilterR;
    juce::dsp::IIR::Filter<float> bodyFilterL, bodyFilterR;
    juce::dsp::IIR::Filter<float> presenceFilterL, presenceFilterR;
    juce::dsp::IIR::Filter<float> airFilterL, airFilterR;
    juce::dsp::IIR::Filter<float> eqLowL, eqLowR, eqLowMidL, eqLowMidR, eqHighMidL, eqHighMidR, eqHighL, eqHighR;
    juce::dsp::IIR::Filter<float> deEssFilterL, deEssFilterR;
    juce::dsp::Compressor<float> compressorL, compressorR;
    juce::dsp::Limiter<float> limiterL, limiterR;
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;
    juce::AudioBuffer<float> wetBuffer;
    juce::AudioBuffer<float> delayBuffer;
    int delayWritePos = 0;
    int doublerWritePos = 0;

    VocalPitchCorrector pitchCorrector;

    juce::SmoothedValue<float> inputGain, outputGain, drive, reverbMix, delayMix, deEssGain, vocalMakeupGain, exciterMix, doublerMix;
    float deEssEnvelopeL = 0.0f;
    float deEssEnvelopeR = 0.0f;

    std::atomic<bool> smartMixActive { false };
    std::atomic<float> smartInputTrim { 0.0f };
    std::atomic<float> smartBody { 0.0f };
    std::atomic<float> smartPresence { 2.0f };
    std::atomic<float> smartAir { 2.0f };
    std::atomic<float> smartComp { 52.0f };
    std::atomic<float> smartDrive { 10.0f };
    std::atomic<float> smartDeess { 32.0f };
    std::atomic<float> smartSpace { 14.0f };

    float lastEqLow = 999.0f, lastEqLowMid = 999.0f, lastEqHighMid = 999.0f, lastEqHigh = 999.0f;
    float lastColor = 999.0f, lastDeessFocus = 999.0f, lastReverbDecay = 999.0f;
    bool advancedFiltersInitialised = false;

    std::atomic<float> progress { 0.0f };
    std::atomic<bool> analysisReady { false };
    std::atomic<bool> analysisRequested { false };
    std::atomic<bool> analysisRunning { false };
    std::atomic<bool> bypass { false };
    Analysis analysis;

    float lastBodyDb = 999.0f, lastPresDb = 999.0f, lastAirDb = 999.0f;
    float lastComp = 999.0f, lastDeess = 999.0f, lastSpace = 999.0f;
    float lastOutputDb = 999.0f;
    int lastStyle = -1;
    bool filtersInitialised = false;
    bool reverbInitialised = false;
    juce::AudioBuffer<float> doublerBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalForgeAudioProcessor)
};