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
    deEssEnvelope = 0.0f;
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
        analysisReady.store(true);
}

void VocalForgeAudioProcessor::triggerAnalysis()
{
    analysis.reset();
    progress.store(0.f);
    analysisReady.store(false);
    analysisRequested.store(true);
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

    auto set = [this](const char* id, float v)
    {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->getNormalisableRange().convertTo0to1(v));
    };

    set("body", body);
    set("presence", presence);
    set("air", air);
    set("comp", comp);
    set("deess", deess);
    set("drive", (float) (8.0 + juce::jlimit(0.0, 18.0, (crest - 3.0) * 2.2)));
    set("space", 12.0f + (float) juce::jlimit(0.0, 12.0, bright * 6.0));
    set("output", -0.8f);
}

void VocalForgeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (analysisRequested.exchange(false)) analysis.reset();

    analyseBlock(buffer);
    if (isAnalysisReady()) applySmartMix();

    const int n = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();
    if (n <= 0 || ch == 0) return;

    auto value = [this](const char* id) { return apvts.getRawParameterValue(id)->load(); };
    const float inDb = value("input");
    const float retune = value("retune") / 100.0f;
    const float speed = value("speed");
    const int root = (int) value("root");
    const int scale = (int) value("scale");
    const int mode = (int) value("mode");

    inputGain.setTargetValue(juce::Decibels::decibelsToGain(inDb));
    outputGain.setTargetValue(juce::Decibels::decibelsToGain(value("output")));
    drive.setTargetValue(value("drive") / 100.0f);

    for (int i = 0; i < n; ++i)
    {
        const float g = inputGain.getNextValue();
        for (int c = 0; c < ch; ++c)
            buffer.setSample(c, i, buffer.getSample(c, i) * g);
    }

    pitchCorrector.setEnabled(retune > 0.001f);
    pitchCorrector.setCorrection(retune);
    pitchCorrector.setSpeed(mode == 0 ? speed : speed * 0.72f);
    pitchCorrector.setScale(root, scale);
    pitchCorrector.process(buffer, n);

    const float bodyDb = value("body");
    const float presDb = value("presence");
    const float airDb = value("air");
    const float comp = value("comp");
    const float sat = value("drive");
    const float deess = value("deess");
    const float space = value("space");

    hpFilterL.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, 70.0);
    hpFilterR.coefficients = hpFilterL.coefficients;
    bodyFilterL.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(currentSampleRate, 180.0, 0.65f, juce::Decibels::decibelsToGain(bodyDb));
    bodyFilterR.coefficients = bodyFilterL.coefficients;
    presenceFilterL.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, 3200.0, 0.8f, juce::Decibels::decibelsToGain(presDb));
    presenceFilterR.coefficients = presenceFilterL.coefficients;
    airFilterL.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(currentSampleRate, 9000.0, 0.7f, juce::Decibels::decibelsToGain(airDb));
    airFilterR.coefficients = airFilterL.coefficients;
    deEssFilterL.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, 5500.0);
    deEssFilterR.coefficients = deEssFilterL.coefficients;

    const float threshold = -19.0f - comp * 0.09f;
    compressorL.setThreshold(threshold); compressorR.setThreshold(threshold);
    compressorL.setRatio(1.25f + comp * 0.032f); compressorR.setRatio(1.25f + comp * 0.032f);
    compressorL.setAttack(5.0f); compressorR.setAttack(5.0f);
    compressorL.setRelease(85.0f); compressorR.setRelease(85.0f);
    limiterL.setThreshold(-1.1f); limiterR.setThreshold(-1.1f);
    limiterL.setRelease(70.0f); limiterR.setRelease(70.0f);

    auto processOne = [this, n, bodyDb, presDb, airDb, comp, sat, deess](juce::AudioBuffer<float>& b, int c,
                                                                          juce::dsp::IIR::Filter<float>& hp,
                                                                          juce::dsp::IIR::Filter<float>& body,
                                                                          juce::dsp::IIR::Filter<float>& pres,
                                                                          juce::dsp::IIR::Filter<float>& air,
                                                                          juce::dsp::IIR::Filter<float>& ds,
                                                                          juce::dsp::Compressor<float>& compProc,
                                                                          juce::dsp::Limiter<float>& lim)
    {
        juce::dsp::AudioBlock<float> block(b.getArrayOfWritePointers() + c, 1, (size_t)n);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        hp.process(ctx); body.process(ctx); pres.process(ctx); air.process(ctx);
        compProc.process(ctx);

        auto* data = b.getWritePointer(c);
        const float driveAmount = sat / 100.0f;
        for (int i = 0; i < n; ++i)
        {
            const float x = data[i];
            const float shaped = std::tanh(x * (1.0f + driveAmount * 2.2f));
            data[i] = shaped / std::max(1.0f, std::tanh(1.0f + driveAmount * 2.2f));
        }

        // Musical de-essing: detect only the upper band and smoothly reduce it.
        for (int i = 0; i < n; ++i)
        {
            const float high = std::abs(ds.processSample(data[i]));
            const float target = juce::jlimit(0.0f, 0.62f, high * (deess / 100.0f) * 1.65f);
            deEssEnvelope = 0.992f * deEssEnvelope + 0.008f * target;
            const float gain = 1.0f - deEssEnvelope;
            data[i] *= gain;
        }

        juce::dsp::AudioBlock<float> block2(b.getArrayOfWritePointers() + c, 1, (size_t)n);
        juce::dsp::ProcessContextReplacing<float> ctx2(block2);
        lim.process(ctx2);
    };

    if (ch >= 2)
    {
        processOne(buffer, 0, hpFilterL, bodyFilterL, presenceFilterL, airFilterL, deEssFilterL, compressorL, limiterL);
        processOne(buffer, 1, hpFilterR, bodyFilterR, presenceFilterR, airFilterR, deEssFilterR, compressorR, limiterR);
    }
    else
        processOne(buffer, 0, hpFilterL, bodyFilterL, presenceFilterL, airFilterL, deEssFilterL, compressorL, limiterL);

    reverbMix.setTargetValue(space / 100.0f * 0.16f);
    reverbParams.roomSize = 0.28f + space * 0.004f;
    reverbParams.damping = 0.55f;
    reverbParams.wetLevel = 0.62f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = 0.9f;
    reverb.setParameters(reverbParams);

    if (space > 0.01f && ch > 1)
    {
        wetBuffer.makeCopyOf(buffer, true);
        reverb.processStereo(wetBuffer.getWritePointer(0), wetBuffer.getWritePointer(1), n);
        const float wm = reverbMix.getNextValue();
        for (int i = 0; i < n; ++i)
        {
            buffer.addSample(0, i, wetBuffer.getSample(0, i) * wm);
            buffer.addSample(1, i, wetBuffer.getSample(1, i) * wm);
        }
    }

    for (int i = 0; i < n; ++i)
    {
        const float g = outputGain.getNextValue();
        for (int c = 0; c < ch; ++c)
            buffer.setSample(c, i, buffer.getSample(c, i) * g);
    }
}

void VocalForgeAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VocalForgeAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* VocalForgeAudioProcessor::createEditor()
{
    return new VocalForgeAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalForgeAudioProcessor();
}
