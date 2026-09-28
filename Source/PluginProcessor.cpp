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
    p.push_back(std::make_unique<juce::AudioParameterFloat>("delay", "Delay", juce::NormalisableRange<float>(0.f, 100.f, 0.01f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("output", "Output", juce::NormalisableRange<float>(-12.f, 6.f, 0.01f), -0.8f));
    p.push_back(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));
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
    delayBuffer.setSize(2, (int)std::ceil(sr * 2.0));
    delayBuffer.clear();
    delayWritePos = 0;

    inputGain.reset(sr, 0.03);
    outputGain.reset(sr, 0.03);
    vocalMakeupGain.reset(sr, 0.05);
    drive.reset(sr, 0.03);
    reverbMix.reset(sr, 0.05);
    delayMix.reset(sr, 0.05);
    deEssGain.reset(sr, 0.02);

    pitchCorrector.prepare(sr, samplesPerBlock);
    analysis.reset();
    progress.store(0.f);
    analysisReady.store(false);
    analysisRequested.store(false);
    analysisRunning.store(false);
    bypass.store(false, std::memory_order_release);
    filtersInitialised = false;
    reverbInitialised = false;
    lastBodyDb = lastPresDb = lastAirDb = lastComp = lastSpace = 999.0f;
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
    if (!analysisRunning.load(std::memory_order_acquire)) return;
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

    if (analysis.seconds >= 15.0)
    {
        analysisRunning.store(false, std::memory_order_release);
        analysisReady.store(true, std::memory_order_release);
    }
}

void VocalForgeAudioProcessor::triggerAnalysis()
{
    // Explicit user action starts a fresh 15-second capture.
    // The analysis remains idle until this method is called.
    analysis.reset();
    progress.store(0.0f, std::memory_order_release);
    analysisReady.store(false, std::memory_order_release);
    smartMixActive.store(false, std::memory_order_release);
    analysisRunning.store(true, std::memory_order_release);
    analysisRequested.store(true, std::memory_order_release);
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

    // Smart Mix may attenuate a hot recording, but never boosts the input stage.
    const float inputTrim = (float) juce::jlimit(-6.0, 0.0, 20.0 * std::log10(0.18 / std::max(0.025, rms)));
    const float body = (float) juce::jmap((float) juce::jlimit(0.05, 0.8, rms), 0.05f, 0.8f, 3.0f, -1.5f);
    const float comp = (float) juce::jlimit(20.0, 82.0, 68.0 - crest * 7.0);
    const float air = (float) juce::jlimit(-1.0, 7.0, 1.8 + (bright - 0.35) * 2.4);
    const float deess = (float) juce::jlimit(15.0, 72.0, 28.0 + bright * 23.0);
    const float presence = (float) juce::jlimit(1.0, 5.5, 3.0 + (0.25 - bright) * 4.5);
    const float driveValue = (float) (8.0 + juce::jlimit(0.0, 18.0, (crest - 3.0) * 2.2));
    const float spaceValue = 12.0f + (float) juce::jlimit(0.0, 12.0, bright * 6.0);

    smartInputTrim.store(inputTrim, std::memory_order_relaxed);
    smartBody.store(body, std::memory_order_relaxed);
    smartPresence.store(presence, std::memory_order_relaxed);
    smartAir.store(air, std::memory_order_relaxed);
    smartComp.store(comp, std::memory_order_relaxed);
    smartDrive.store(driveValue, std::memory_order_relaxed);
    smartDeess.store(deess, std::memory_order_relaxed);
    smartSpace.store(spaceValue, std::memory_order_relaxed);
    smartMixActive.store(true, std::memory_order_release);
}



void VocalForgeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const bool isBypassed = apvts.getRawParameterValue("bypass")->load(std::memory_order_relaxed) > 0.5f;
    if (isBypassed)
    {
        analysisRunning.store(false, std::memory_order_release);
        analysisRequested.store(false, std::memory_order_release);
        return;
    }

    if (analysisRequested.exchange(false, std::memory_order_acq_rel))
    {
        analysis.reset();
        progress.store(0.0f, std::memory_order_relaxed);
    }

    if (analysisRunning.load(std::memory_order_acquire))
        analyseBlock(buffer);
    if (isAnalysisReady())
        applySmartMix();

    const int n = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();
    if (n <= 0 || ch == 0) return;

    auto value = [this](const char* id) { return apvts.getRawParameterValue(id)->load(std::memory_order_relaxed); };

    const float inDb = value("input");
    const float retune = value("retune") / 100.0f;
    const float speed = value("speed");
    const int root = (int) value("root");
    const int scale = (int) value("scale");
    const int mode = (int) value("mode");
    const int style = (int) value("style");

    const bool useSmart = smartMixActive.load(std::memory_order_acquire);

    const float smartTrimDb = useSmart ? smartInputTrim.load(std::memory_order_relaxed) : 0.0f;
    float bodyDb = useSmart ? smartBody.load(std::memory_order_relaxed) : value("body");
    float presDb = useSmart ? smartPresence.load(std::memory_order_relaxed) : value("presence");
    float airDb = useSmart ? smartAir.load(std::memory_order_relaxed) : value("air");
    float comp = useSmart ? smartComp.load(std::memory_order_relaxed) : value("comp");
    float sat = useSmart ? smartDrive.load(std::memory_order_relaxed) : value("drive");
    float deess = useSmart ? smartDeess.load(std::memory_order_relaxed) : value("deess");
    float space = useSmart ? smartSpace.load(std::memory_order_relaxed) : value("space");

    switch (style)
    {
        case 0: break; // Clean
        case 1: bodyDb += 0.8f; presDb -= 0.4f; break; // Warm
        case 2: presDb += 0.9f; airDb += 1.1f; break; // Bright
        case 3: presDb += 0.7f; comp += 10.0f; sat += 6.0f; deess += 4.0f; break; // Aggressive
        default: break;
    }

    bodyDb = juce::jlimit(-6.0f, 6.0f, bodyDb);
    presDb = juce::jlimit(-6.0f, 8.0f, presDb);
    airDb = juce::jlimit(-6.0f, 10.0f, airDb);
    comp = juce::jlimit(0.0f, 100.0f, comp);
    sat = juce::jlimit(0.0f, 100.0f, sat);
    deess = juce::jlimit(0.0f, 100.0f, deess);
    space = juce::jlimit(0.0f, 100.0f, space);

    inputGain.setTargetValue(juce::Decibels::decibelsToGain(inDb + smartTrimDb));
    outputGain.setTargetValue(juce::Decibels::decibelsToGain(value("output")));

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

    // Rebuild IIR coefficients only when a relevant parameter actually changes.
    // Allocating new coefficient objects every audio block is unnecessarily expensive.
    if (!filtersInitialised || bodyDb != lastBodyDb || presDb != lastPresDb || airDb != lastAirDb)
    {
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
        lastBodyDb = bodyDb; lastPresDb = presDb; lastAirDb = airDb;
        filtersInitialised = true;
    }

    const float threshold = -19.0f - comp * 0.09f;
    const float ratio = 1.25f + comp * 0.032f;
    // Never add makeup gain in the normal/default path. Smart Mix can add
    // controlled makeup only after the user explicitly runs the analysis.
    const float makeupDb = useSmart ? juce::jmap(comp, 20.0f, 82.0f, 1.0f, 3.0f) : 0.0f;
    vocalMakeupGain.setTargetValue(juce::Decibels::decibelsToGain(makeupDb));
    const float makeupGain = vocalMakeupGain.getNextValue();
    if (comp != lastComp || !filtersInitialised)
    {
        compressorL.setThreshold(threshold); compressorR.setThreshold(threshold);
        compressorL.setRatio(ratio); compressorR.setRatio(ratio);
        compressorL.setAttack(5.0f); compressorR.setAttack(5.0f);
        lastComp = comp;
    }
    if (!filtersInitialised)
    {
        limiterL.setThreshold(-1.1f); limiterR.setThreshold(-1.1f);
        limiterL.setRelease(70.0f); limiterR.setRelease(70.0f);
    }

    auto processOne = [this, n, bodyDb, presDb, airDb, comp, sat, deess, makeupGain]
        (juce::AudioBuffer<float>& b, int c,
         juce::dsp::IIR::Filter<float>& hp,
         juce::dsp::IIR::Filter<float>& body,
         juce::dsp::IIR::Filter<float>& pres,
         juce::dsp::IIR::Filter<float>& air,
         juce::dsp::IIR::Filter<float>& ds,
         juce::dsp::Compressor<float>& compProc,
         juce::dsp::Limiter<float>& lim,
         float& deEssEnvelope)
    {
        juce::dsp::AudioBlock<float> block(b.getArrayOfWritePointers() + c, 1, (size_t)n);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        hp.process(ctx);
        body.process(ctx);
        pres.process(ctx);
        air.process(ctx);
        compProc.process(ctx);

        auto* data = b.getWritePointer(c);
        const float driveAmount = sat / 100.0f;
        const float driveNorm = std::max(1.0f, std::tanh(1.0f + driveAmount * 2.2f));

        for (int i = 0; i < n; ++i)
        {
            const float x = data[i] * makeupGain;
            const float shaped = std::tanh(x * (1.0f + driveAmount * 2.2f));
            data[i] = shaped / driveNorm;
        }

        for (int i = 0; i < n; ++i)
        {
            const float high = std::abs(ds.processSample(data[i]));
            const float target = juce::jlimit(0.0f, 0.62f, high * (deess / 100.0f) * 1.65f);
            deEssEnvelope = 0.992f * deEssEnvelope + 0.008f * target;
            data[i] *= (1.0f - deEssEnvelope);
        }

        juce::dsp::AudioBlock<float> block2(b.getArrayOfWritePointers() + c, 1, (size_t)n);
        juce::dsp::ProcessContextReplacing<float> ctx2(block2);
        lim.process(ctx2);
    };

    if (ch >= 2)
    {
        processOne(buffer, 0, hpFilterL, bodyFilterL, presenceFilterL, airFilterL,
                   deEssFilterL, compressorL, limiterL, deEssEnvelopeL);
        processOne(buffer, 1, hpFilterR, bodyFilterR, presenceFilterR, airFilterR,
                   deEssFilterR, compressorR, limiterR, deEssEnvelopeR);
    }
    else
    {
        processOne(buffer, 0, hpFilterL, bodyFilterL, presenceFilterL, airFilterL,
                   deEssFilterL, compressorL, limiterL, deEssEnvelopeL);
    }

    if (!reverbInitialised || space != lastSpace)
    {
        reverbMix.setTargetValue(space / 100.0f * 0.16f);
        reverbParams.roomSize = 0.28f + space * 0.004f;
        reverbParams.damping = 0.55f;
        reverbParams.wetLevel = 0.62f;
        reverbParams.dryLevel = 0.0f;
        reverbParams.width = 0.9f;
        reverb.setParameters(reverbParams);
        lastSpace = space;
        reverbInitialised = true;
    }

    if (space > 0.01f && ch > 1 && n <= wetBuffer.getNumSamples())
    {
        wetBuffer.copyFrom(0, 0, buffer, 0, 0, n);
        wetBuffer.copyFrom(1, 0, buffer, 1, 0, n);
        reverb.processStereo(wetBuffer.getWritePointer(0), wetBuffer.getWritePointer(1), n);

        for (int i = 0; i < n; ++i)
        {
            const float wm = reverbMix.getNextValue();
            buffer.addSample(0, i, wetBuffer.getSample(0, i) * wm);
            buffer.addSample(1, i, wetBuffer.getSample(1, i) * wm);
        }
    }

    // Independent, lightweight stereo delay. Default is 0%, so it never changes
    // the sound until the user enables it.
    delayMix.setTargetValue(delay / 100.0f * 0.18f);
    if (delay > 0.01f && delayBuffer.getNumSamples() > n)
    {
        const int delaySamples = juce::jlimit(1, delayBuffer.getNumSamples() - 1,
            (int)std::round((0.12 + (delay / 100.0) * 0.38) * currentSampleRate));
        const float mix = delayMix.getNextValue();
        for (int i = 0; i < n; ++i)
        {
            const int write = (delayWritePos + i) % delayBuffer.getNumSamples();
            const int read = (write - delaySamples + delayBuffer.getNumSamples()) % delayBuffer.getNumSamples();
            const float inL = buffer.getSample(0, i);
            const float inR = ch > 1 ? buffer.getSample(1, i) : inL;
            const float echoL = delayBuffer.getSample(0, read);
            const float echoR = delayBuffer.getSample(1, read);
            delayBuffer.setSample(0, write, inL + echoL * 0.42f);
            delayBuffer.setSample(1, write, inR + echoR * 0.42f);
            buffer.addSample(0, i, echoL * mix);
            if (ch > 1) buffer.addSample(1, i, echoR * mix);
        }
        delayWritePos = (delayWritePos + n) % delayBuffer.getNumSamples();
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
