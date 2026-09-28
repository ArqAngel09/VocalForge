#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

static juce::StringArray qualityChoices() { return { "Clean", "Warm", "Bright", "Aggressive" }; }

VocalForgeAudioProcessor::VocalForgeAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                   .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
}

APVTS::ParameterLayout VocalForgeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>("input", "Input", juce::NormalisableRange<float>(-12.f, 12.f, 0.01f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("body", "Body", juce::NormalisableRange<float>(-6.f, 6.f, 0.01f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("presence", "Presence", juce::NormalisableRange<float>(-6.f, 8.f, 0.01f), 2.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("air", "Air", juce::NormalisableRange<float>(-6.f, 10.f, 0.01f), 2.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("comp", "Compression", juce::NormalisableRange<float>(0.f, 100.f, 0.01f), 55.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("drive", "Saturation", juce::NormalisableRange<float>(0.f, 100.f, 0.01f), 12.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("deess", "DeEss", juce::NormalisableRange<float>(0.f, 100.f, 0.01f), 35.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("space", "Space", juce::NormalisableRange<float>(0.f, 100.f, 0.01f), 16.f));
    p.push_back(std::make_unique<juce::AudioParameterBool>("auto", "Auto 15s", true));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("style", "Style", qualityChoices(), 1));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("output", "Output", juce::NormalisableRange<float>(-12.f, 6.f, 0.01f), -0.5f));
    return { p.begin(), p.end() };
}

void VocalForgeAudioProcessor::prepareToPlay(double sr, int samplesPerBlock)
{
    currentSampleRate = sr;
    juce::dsp::ProcessSpec spec { sr, (juce::uint32)samplesPerBlock, 1 };
    hpFilterL.prepare(spec); hpFilterR.prepare(spec);
    presenceFilterL.prepare(spec); presenceFilterR.prepare(spec);
    airFilterL.prepare(spec); airFilterR.prepare(spec);
    compressorL.prepare(spec); compressorR.prepare(spec);
    limiterL.prepare(spec); limiterR.prepare(spec);
    reverb.setSampleRate(sr);
    wetBuffer.setSize(2, samplesPerBlock);
    wetBuffer.clear();
    inputGain.reset(sr, 0.03); outputGain.reset(sr, 0.03); drive.reset(sr, 0.03);
    reverbMix.reset(sr, 0.05);
    analysis.reset(); progress.store(0.f); analysisReady.store(false);
}

void VocalForgeAudioProcessor::releaseResources() {}

bool VocalForgeAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void VocalForgeAudioProcessor::analyseBlock(const juce::AudioBuffer<float>& b)
{
    const int n = b.getNumSamples();
    if (n <= 0 || b.getNumChannels() == 0 || analysis.seconds >= 15.0) return;
    const float* l = b.getReadPointer(0);
    const float* r = b.getNumChannels() > 1 ? b.getReadPointer(1) : l;
    for (int i = 0; i < n; ++i)
    {
        float x = 0.5f * (l[i] + r[i]);
        float ax = std::abs(x);
        analysis.sumSq += (double)x * x;
        analysis.peak = std::max(analysis.peak, (double)ax);
        const float hp2k = std::abs(x - (i > 0 ? l[i - 1] : x));
        const float hp6k = std::abs(x - 0.65f * (i > 0 ? l[i - 1] : x));
        analysis.high += hp6k;
        analysis.mid += hp2k;
        analysis.low += std::abs(x);
    }
    analysis.samples += (uint64_t)n;
    analysis.seconds = (double)analysis.samples / currentSampleRate;
    progress.store((float)juce::jlimit(0.0, 1.0, analysis.seconds / 15.0));

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
    auto p = progress.load();
    if (p < 1.f) return "Listening: " + juce::String((int)std::round(p * 15.0f)) + "/15s";
    return "Vocal profile captured";
}

void VocalForgeAudioProcessor::applySmartMix()
{
    const double rms = analysis.samples > 0 ? std::sqrt(analysis.sumSq / (double)analysis.samples) : 0.1;
    const double crest = analysis.peak / std::max(0.0001, rms);
    const double bright = analysis.high / std::max(1.0, analysis.mid);
    const float body = (float)juce::jmap((float)juce::jlimit(0.05, 0.8, rms), 0.05f, 0.8f, 3.5f, -2.0f);
    const float comp = (float)juce::jlimit(25.0, 82.0, 72.0 - crest * 8.0);
    const float air = (float)juce::jlimit(-1.0, 7.0, 2.0 + (bright - 0.35) * 2.5);
    const float deess = (float)juce::jlimit(15.0, 75.0, 30.0 + bright * 25.0);
    const float presence = (float)juce::jlimit(0.0, 6.0, 3.0 + (0.25 - bright) * 5.0);

    auto set = [this](const char* id, float v)
    {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(v));
    };
    set("body", body); set("presence", presence); set("air", air);
    set("comp", comp); set("deess", deess);
    set("drive", (float)(10.0 + juce::jlimit(0.0, 25.0, (crest - 3.0) * 3.0)));
    set("space", 14.0f + (float)juce::jlimit(0.0, 16.0, bright * 8.0));
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
    if (ch == 0) return;

    auto val = [this](const char* id) { return apvts.getRawParameterValue(id)->load(); };
    const float inDb = val("input"), outDb = val("output");
    const float presDb = val("presence"), airDb = val("air");
    const float comp = val("comp"), sat = val("drive"), deess = val("deess"), space = val("space");

    inputGain.setTargetValue(juce::Decibels::decibelsToGain(inDb));
    outputGain.setTargetValue(juce::Decibels::decibelsToGain(outDb));
    drive.setTargetValue(sat / 100.f);

    hpFilterL.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, 75.0);
    hpFilterR.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, 75.0);
    presenceFilterL.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, 3200.0, 0.8f, juce::Decibels::decibelsToGain(presDb));
    presenceFilterR.coefficients = presenceFilterL.coefficients;
    airFilterL.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(currentSampleRate, 8500.0, 0.7f, juce::Decibels::decibelsToGain(airDb));
    airFilterR.coefficients = airFilterL.coefficients;

    compressorL.setThreshold(-18.0f - comp * 0.10f); compressorR.setThreshold(-18.0f - comp * 0.10f);
    compressorL.setRatio(1.2f + comp * 0.035f); compressorR.setRatio(1.2f + comp * 0.035f);
    compressorL.setAttack(5.0f); compressorR.setAttack(5.0f);
    compressorL.setRelease(90.0f); compressorR.setRelease(90.0f);
    limiterL.setThreshold(-1.2f); limiterR.setThreshold(-1.2f);
    limiterL.setRelease(80.0f); limiterR.setRelease(80.0f);

    juce::dsp::IIR::Filter<float>* f[2] = { &hpFilterL, &hpFilterR };
    for (int c = 0; c < juce::jmin(2, ch); ++c)
    {
        auto* data = buffer.getWritePointer(c);
        for (int i = 0; i < n; ++i)
        {
            float x = data[i] * inputGain.getNextValue();
            x = f[c]->processSample(x);
            const float d = drive.getNextValue();
            x = std::tanh(x * (1.0f + d * 2.5f)) / std::tanh(1.0f + d * 2.5f);
            data[i] = x;
        }
    }

    if (ch >= 2)
    {
        juce::dsp::AudioBlock<float> oneL(buffer.getArrayOfWritePointers(), 1, n);
        juce::dsp::AudioBlock<float> oneR(buffer.getArrayOfWritePointers() + 1, 1, n);
        juce::dsp::ProcessContextReplacing<float> cl(oneL), cr(oneR);
        presenceFilterL.process(cl); presenceFilterR.process(cr);
        airFilterL.process(cl); airFilterR.process(cr);
        compressorL.process(cl); compressorR.process(cr);
        limiterL.process(cl); limiterR.process(cr);
    }
    else
    {
        juce::dsp::AudioBlock<float> mono(buffer.getArrayOfWritePointers(), 1, n);
        juce::dsp::ProcessContextReplacing<float> cm(mono);
        presenceFilterL.process(cm); airFilterL.process(cm); compressorL.process(cm); limiterL.process(cm);
    }

    const float deessAmt = deess / 100.f;
    for (int i = 0; i < n; ++i)
    {
        float l = buffer.getSample(0, i);
        float r = ch > 1 ? buffer.getSample(1, i) : l;
        float previousL = buffer.getSample(0, juce::jmax(0, i - 1));
        float previousR = ch > 1 ? buffer.getSample(1, juce::jmax(0, i - 1)) : previousL;
        float high = 0.5f * ((l - previousL) + (r - previousR));
        float reduction = 1.0f - juce::jlimit(0.0f, 0.55f, std::abs(high) * deessAmt * 2.0f);
        float gain = outputGain.getNextValue();
        buffer.setSample(0, i, l * reduction * gain);
        if (ch > 1) buffer.setSample(1, i, r * reduction * gain);
    }

    reverbMix.setTargetValue(space / 100.f * 0.18f);
    reverbParams = juce::Reverb::Parameters();
    reverbParams.roomSize = 0.32f + space * 0.004f;
    reverbParams.damping = 0.55f;
    reverbParams.wetLevel = 0.65f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = 0.9f;
    reverb.setParameters(reverbParams);

    if (space > 0.01f)
    {
        wetBuffer.makeCopyOf(buffer, true);
        if (ch > 1)
        {
            reverb.processStereo(wetBuffer.getWritePointer(0), wetBuffer.getWritePointer(1), n);
            const float wm = reverbMix.getNextValue();
            for (int i = 0; i < n; ++i)
            {
                buffer.addSample(0, i, wetBuffer.getSample(0, i) * wm);
                buffer.addSample(1, i, wetBuffer.getSample(1, i) * wm);
            }
        }
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
