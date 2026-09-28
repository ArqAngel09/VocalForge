#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

static juce::StringArray styleChoices() { return { "Clean", "Warm", "Bright", "Aggressive" }; }
static juce::StringArray rootChoices() { return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; }
static juce::StringArray scaleChoices() { return { "Major", "Minor", "Chromatic" }; }
static juce::StringArray modeChoices() { return { "Live", "Studio" }; };

VocalForgeAudioProcessor::VocalForgeAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                   .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
}

APVTS::ParameterLayout VocalForgeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back(std::make_unique<juce::AudioParameterFloat>("input", "Input", juce::NormalisableRange<float>(-18.f, 12.f, 0.01f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("retune", "Retune", juce::NormalisableRange<float>(0.f, 100.f, 0.01f), 72.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("speed", "Retune Speed", juce::NormalisableRange<float>(5.f, 250.f, 0.1f), 55.f));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("root", "Key", rootChoices(), 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("scale", "Scale", scaleChoices(), 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("mode", "Mode", modeChoices(), 0));

    p.push_back(std::make_unique<juce::AudioParameterFloat>("body", "Body", juce::NormalisableRange<float>(-6.f, 6.f, 0.01f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("presence", "Presence", juce::NormalisableRange<float>(-6.f, 8.f, 0.01f), 2.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("air", "Air", juce::NormalisableRange<float>(-6.f, 10.f, 0.01f), 2.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("comp", "Compression", juce::NormalisableRange<float>(0.f, 100.f, 0.01f), 52.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("drive", "Saturation", juce::NormalisableRange<float>(0.f, 100.f, 0.01f), 10.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("deess", "De-Esser", juce::NormalisableRange<float>(0.f, 100.f, 0.01f), 32.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("space", "Space", juce::NormalisableRange<float>(0.f, 100.f, 0.01f), 14.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("output", "Output", juce::NormalisableRange<float>(-12.f, 6.f, 0.01f), -0.8f));
    p.push_back(std::make_unique<juce::AudioParameterBool>("auto", "Auto after 15 seconds", true));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("style", "Style", styleChoices(), 1));
    return { p.begin(), p.end() };
}

void VocalForgeAudioProcessor::prepareToPlay(double sr, int samplesPerBlock)
{
    currentSampleRate = sr;
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) samplesPerBlock, 1 };

    hpFilterL.prepare(spec); hpFilterR.prepare(spec);
    bodyFilterL.prepare(spec); bodyFilterR.prepare(spec);
    presenceFilterL.prepare(spec); presenceFilterR.prepare(spec);
    airFilterL.prepare(spec); airFilterR.prepare(spec);
    deEssFilterL.prepare(spec); deEssFilterR.prepare(spec);
    compressorL.prepare(spec); compressorR.prepare(spec);
    limiterL.prepare(spec); limiterR.prepare(spec);

    reverb.setSampleRate(sr);
    wetBuffer.setSize(2, samplesPerBlock);
    wetBuffer.clear();

    inputGain.reset(sr, 0.03);
    outputGain.reset(sr, 0.03);
    drive.reset(sr, 0.03);
    reverbMix.reset(sr, 0.05);
    deEssGain.reset(sr, 0.02);

    pitchCorrector.prepare(sr, samplesPerBlock);
    analysis.reset();
    progress.store(0.f);
    analysisReady.store(false);
    analysisRequested.store(false);
    deEssEnvelopeL = 0.0f;
    deEssEnvelopeR = 0.0f;
}

void VocalForgeAudioProcessor::releaseResources() {}

bool VocalForgeAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
        && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void VocalForgeAudioProcessor::analyseBlock(const juce::AudioBuffer<float>& b)
{
    const int n = b.getNumSamples();
    if (n <= 0 || b.getNumChannels() == 0 || analysis.seconds >= 15.0) return;

    const float* l = b.getReadPointer(0);
    const float* r = b.getNumChannels() > 1 ? b.getReadPointer(1) : l;

    for (int i = 0; i < n; ++i)
    {
        const float x = 0.5f * (l[i] + r[i]);
        const float ax = std::abs(x);
        analysis.sumSq += (double)x * x;
        analysis.peak = std::max(analysis.peak, (double)ax);

        const float d = std::abs(x - (i > 0 ? l[i - 1] : x));
        analysis.high += d;
        analysis.mid += std::abs(x - 0.7f * (i > 0 ? l[i - 1] : x));
        analysis.low += ax;
    }

    analysis.samples += (uint64_t) n;
    analysis.seconds = (double) analysis.samples / currentSampleRate;
    progress.store((float) juce::jlimit(0.0, 1.0, analysis.seconds / 15.0));

    if (analysis.seconds >= 15.0 && apvts.getRawParameterValue("auto")->load() > 0.5f)
        analysisReady.store(true, std::memory_order_release);
}

void VocalForgeAudioProcessor::triggerAnalysis()
{
    analysis.reset();
    progress.store(0.f);
    analysisReady.store(false);
    analysisRequested.store(true, std::memory_order_release);
    smartMixActive.store(false, std::memory_order_release);
}

float VocalForgeAudioProcessor::getAnalysisProgress() const { return progress.load(); }

juce::String VocalForgeAudioProcessor::getAnalysisSummary() const
{
    const auto p = progress.load();
    const auto conf = pitchCorrector.getConfidence();
    const auto midi = pitchCorrector.getTargetMidi();

    if (p < 1.f)
        return "Vocal profile: " + juce::String((int) std::round(p * 15.0f)) + "/15s";
    if (conf > 0.18f && midi > 0.0f)
        return "Pitch locked • confidence " + juce::String((int) std::round(conf * 100.0f)) + "%";
    return "Vocal profile captured";
}

void VocalForgeAudioProcessor::applySmartMix()
{
    const double rms = analysis.samples > 0 ? std::sqrt(analysis.sumSq / (double) analysis.samples) : 0.1;
    const double crest = analysis.peak / std::max(0.0001, rms);
    const double bright = analysis.high / std::max(1.0, analysis.mid);

    const float body = (float) juce::jmap((float) juce::jlimit(0.05, 0.8, rms), 0.05f, 0.8f, 3.0f, -1.5f);
    const float comp = (float) juce::jlimit(20.0, 82.0, 68.0 - crest * 7.0);
    const float air = (float) juce::jlimit(-1.0, 7.0, 1.8 + (bright - 0.35) * 2.4);
    const float deess = (float) juce::jlimit(15.0, 72.0, 28.0 + bright * 23.0);
    const float presence = (float) juce::jlimit(0.0, 5.5, 2.6 + (0.25 - bright) * 4.5);
    const float driveValue = (float) (8.0 + juce::jlimit(0.0, 18.0, (crest - 3.0) * 2.2));
    const float spaceValue = 12.0f + (float) juce::jlimit(0.0, 12.0, bright * 6.0);

    smartBody.store(body, std::memory_order_relaxed);
    smartPresence.store(presence, std::memory_order_relaxed);
    smartAir.store(air, std::memory_order_relaxed);
    smartComp.store(comp, std::memory_order_relaxed);
    smartDrive.store(driveValue, std::memory_order_relaxed);
    smartDeess.store(deess, std::memory_order_relaxed);
    smartSpace.store(spaceValue, std::memory_order_relaxed);
    smartMixActive.store(true, std::memory_order_release);
}

