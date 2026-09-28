#include "VocalPitchCorrector.h"

namespace
{
constexpr float kMinHz = 70.0f;
constexpr float kMaxHz = 1100.0f;

static bool isInScale(int pc, int root, int type)
{
    static constexpr int major[] = { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr int minor[] = { 0, 2, 3, 5, 7, 8, 10 };
    const auto* scale = type == 1 ? minor : major;
    for (int i = 0; i < 7; ++i)
        if ((root + scale[i]) % 12 == pc) return true;
    return false;
}
}

void VocalPitchCorrector::prepare(double sr, int)
{
    sampleRate_ = sr;
    grainSize_ = juce::jlimit(512, 2048, (int) std::round(sr * 0.02322));
    hopSize_ = grainSize_ / 2;
    ringSize_ = 1;
    while (ringSize_ < grainSize_ * 8) ringSize_ <<= 1;
    ringMask_ = ringSize_ - 1;
    ring_.assign((size_t) ringSize_, 0.0f);
    grainBuffer_.assign((size_t) grainSize_, 0.0f);
    reset();
}

void VocalPitchCorrector::reset()
{
    std::fill(ring_.begin(), ring_.end(), 0.0f);
    writePos_ = 0;
    hopCounter_ = 0;
    currentRatio_ = 1.0f;
    for (auto& g : grains_) g = {};
    detectedMidi_.store(0.0f);
    targetMidi_.store(0.0f);
    confidence_.store(0.0f);
}

float VocalPitchCorrector::detectPitch(const float* x, int n, float& confidence) const
{
    if (n < 256) { confidence = 0.0f; return 0.0f; }

    const int minLag = (int) std::floor(sampleRate_ / kMaxHz);
    const int maxLag = (int) std::ceil(sampleRate_ / kMinHz);
    const int usableMax = juce::jmin(maxLag, n - 2);
    double energy = 0.0;
    for (int i = 0; i < n; ++i) energy += (double)x[i] * x[i];
    if (energy < 1.0e-5) { confidence = 0.0f; return 0.0f; }

    int bestLag = minLag;
    double best = -1.0;
    for (int lag = minLag; lag <= usableMax; ++lag)
    {
        double sum = 0.0, e1 = 0.0, e2 = 0.0;
        for (int i = 0; i < n - lag; i += 2)
        {
            const double a = x[i], b = x[i + lag];
            sum += a * b;
            e1 += a * a;
            e2 += b * b;
        }
        const double corr = sum / std::sqrt(e1 * e2 + 1.0e-12);
        if (corr > best) { best = corr; bestLag = lag; }
    }

    confidence = (float) juce::jlimit(0.0, 1.0, (best - 0.25) / 0.65);
    if (confidence < 0.18f) return 0.0f;

    return (float) (sampleRate_ / (double) bestLag);
}

float VocalPitchCorrector::quantizeMidi(float midi) const
{
    if (midi <= 0.0f) return midi;
    const int base = (int) std::floor(midi);
    float best = (float) base;
    float bestDistance = 999.0f;

    for (int oct = -1; oct <= 1; ++oct)
        for (int pc = 0; pc < 12; ++pc)
        {
            const int candidate = base + oct * 12 + pc - (base % 12);
            if (!isInScale((candidate % 12 + 12) % 12, root_, scaleType_)) continue;
            const float d = std::abs(midi - (float) candidate);
            if (d < bestDistance) { bestDistance = d; best = (float) candidate; }
        }

    return best;
}

float VocalPitchCorrector::smoothTarget(float target)
{
    if (target <= 0.0f) return currentRatio_;
    const float currentCents = 1200.0f * std::log2(currentRatio_);
    const float targetCents = (target - detectedMidi_.load()) * 100.0f * correction_;
    const float delta = targetCents - currentCents;
    const float tau = juce::jmax(0.005f, speedMs_ * 0.001f);
    const float alpha = 1.0f - std::exp(-1.0f / (float)(sampleRate_ * tau));
    const float newCents = currentCents + delta * alpha;
    currentRatio_ = std::pow(2.0f, newCents / 1200.0f);
    return currentRatio_;
}

void VocalPitchCorrector::process(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (!enabled_ || numSamples <= 0 || buffer.getNumChannels() <= 0) return;

    const int channels = juce::jmin(2, buffer.getNumChannels());
    std::vector<float> mono((size_t) numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        float s = buffer.getSample(0, i);
        if (channels > 1) s = 0.5f * (s + buffer.getSample(1, i));
        mono[(size_t)i] = s;
        ring_[(size_t)writePos_] = s;
        writePos_ = (writePos_ + 1) & ringMask_;
    }

    const int analysisN = juce::jmin(grainSize_, ringSize_ - 1);
    std::vector<float> analysis((size_t) analysisN);
    int p = (writePos_ - analysisN + ringSize_) & ringMask_;
    for (int i = 0; i < analysisN; ++i) analysis[(size_t)i] = ring_[(size_t)((p + i) & ringMask_)];

    float conf = 0.0f;
    const float hz = detectPitch(analysis.data(), analysisN, conf);
    confidence_.store(conf);
    if (hz > 0.0f)
    {
        const float midi = 69.0f + 12.0f * std::log2(hz / 440.0f);
        const float target = quantizeMidi(midi);
        detectedMidi_.store(midi);
        targetMidi_.store(target);
        smoothTarget(target);
    }
    else
    {
        confidence_.store(0.0f);
        currentRatio_ = 1.0f + (currentRatio_ - 1.0f) * 0.995f;
    }

    if (std::abs(currentRatio_ - 1.0f) < 0.002f) return;

    // Two-window OLA pitch shifter. It deliberately uses short grains so Live mode
    // remains responsive while overlap reduces the obvious "robot" stepping.
    std::vector<float> out((size_t) numSamples, 0.0f);
    std::vector<float> weight((size_t) numSamples, 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        const int global = hopCounter_++;
        if (global % hopSize_ == 0)
        {
            Grain& g = grains_[(global / hopSize_) & 1];
            g.readPos = (double)((writePos_ - grainSize_ + ringSize_) & ringMask_);
            g.increment = currentRatio_;
            g.age = 0;
            g.active = true;
        }

        for (auto& g : grains_)
        {
            if (!g.active || g.age >= grainSize_) continue;
            int idx = (int) std::floor(g.readPos) & ringMask_;
            int idx2 = (idx + 1) & ringMask_;
            float frac = (float)(g.readPos - std::floor(g.readPos));
            float s = ring_[(size_t)idx] + (ring_[(size_t)idx2] - ring_[(size_t)idx]) * frac;
            const float phase = (float)g.age / (float)grainSize_;
            const float w = std::sin(juce::MathConstants<float>::pi * phase);
            out[(size_t)i] += s * w;
            weight[(size_t)i] += w;
            g.readPos += g.increment;
            ++g.age;
        }
    }

    for (int i = 0; i < numSamples; ++i)
    {
        if (weight[(size_t)i] > 1.0e-4f)
        {
            const float y = out[(size_t)i] / weight[(size_t)i];
            for (int c = 0; c < channels; ++c)
                buffer.setSample(c, i, y);
        }
    }
}
