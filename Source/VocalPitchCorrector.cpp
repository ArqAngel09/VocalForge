#include "VocalPitchCorrector.h"

namespace
{
constexpr float kMinHz = 70.0f;
constexpr float kMaxHz = 1100.0f;

static bool isInScale(int pc, int root, int type) noexcept
{
    if (type == 2) return true;
    static constexpr int major[] = { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr int minor[] = { 0, 2, 3, 5, 7, 8, 10 };
    const auto* scale = type == 1 ? minor : major;
    for (int i = 0; i < 7; ++i)
        if ((root + scale[i]) % 12 == pc) return true;
    return false;
}
}

void VocalPitchCorrector::prepare(double sr, int maxBlockSize)
{
    sampleRate_ = juce::jmax(8000.0, sr);
    grainSize_ = juce::jlimit(512, 2048, (int) std::round(sampleRate_ * 0.02322));
    hopSize_ = juce::jmax(256, grainSize_ / 2);

    ringSize_ = 1;
    while (ringSize_ < grainSize_ * 8) ringSize_ <<= 1;
    ringMask_ = ringSize_ - 1;

    ring_.assign((size_t) ringSize_, 0.0f);
    analysisBuffer_.assign((size_t) grainSize_, 0.0f);
    differenceBuffer_.assign((size_t) grainSize_ + 1, 0.0f);
    cmndfBuffer_.assign((size_t) grainSize_ + 1, 1.0f);

    outBuffer_.setSize(1, juce::jmax(1, maxBlockSize));
    weightBuffer_.setSize(1, juce::jmax(1, maxBlockSize));
    reset();
}

void VocalPitchCorrector::reset()
{
    std::fill(ring_.begin(), ring_.end(), 0.0f);
    std::fill(analysisBuffer_.begin(), analysisBuffer_.end(), 0.0f);
    writePos_ = 0;
    hopCounter_ = 0;
    currentRatio_ = 1.0f;
    nextGrainReadPos_ = 0.0;
    pitchReadInitialised_ = false;
    stableMidi_ = 0.0f;
    stableTarget_ = 0.0f;
    stableFrames_ = 0;
    for (auto& g : grains_) g = {};

    detectedMidi_.store(0.0f, std::memory_order_relaxed);
    targetMidi_.store(0.0f, std::memory_order_relaxed);
    confidence_.store(0.0f, std::memory_order_relaxed);
}

float VocalPitchCorrector::detectPitch(float* x, int n, float& confidence) const noexcept
{
    confidence = 0.0f;
    if (n < 512) return 0.0f;

    // YIN-style difference function. Unlike a plain autocorrelation maximum,
    // this explicitly searches for the first strong periodicity, which greatly
    // reduces octave-up/octave-down errors on voiced vocals.
    double mean = 0.0;
    for (int i = 0; i < n; ++i) mean += x[i];
    mean /= (double) n;

    double energy = 0.0;
    for (int i = 0; i < n; ++i)
    {
        x[i] -= (float) mean;
        energy += (double) x[i] * x[i];
    }

    if (energy < 1.0e-7)
        return 0.0f;

    const int minLag = juce::jmax(2, (int) std::floor(sampleRate_ / kMaxHz));
    const int maxLag = juce::jmin(n / 2, (int) std::ceil(sampleRate_ / kMinHz));
    if (maxLag <= minLag + 2)
        return 0.0f;

    auto* difference = differenceBuffer_.data();
    auto* cmndf = cmndfBuffer_.data();
    std::fill(difference, difference + maxLag + 1, 0.0f);
    std::fill(cmndf, cmndf + maxLag + 1, 1.0f);

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double d = 0.0;
        for (int i = 0; i < n - lag; i += 2)
        {
            const double delta = (double) x[i] - x[i + lag];
            d += delta * delta;
        }
        difference[lag] = (float) d;
    }

    double running = 0.0;
    int bestLag = 0;
    float bestScore = 1.0f;
    const float yinThreshold = 0.14f;

    for (int lag = 1; lag <= maxLag; ++lag)
    {
        running += difference[(size_t) lag];
        if (lag < minLag || running <= 1.0e-12)
            continue;

        const float score = difference[(size_t) lag] * (float) lag / (float) running;
        cmndf[lag] = score;

        // Select the first sufficiently deep valley. This is the key octave
        // protection: the fundamental is preferred over its 2nd harmonic.
        if (score < yinThreshold)
        {
            int refined = lag;
            while (refined + 1 <= maxLag && cmndf[refined + 1] < cmndf[refined])
                ++refined;
            bestLag = refined;
            bestScore = cmndf[(size_t) refined];
            break;
        }

        if (score < bestScore)
        {
            bestScore = score;
            bestLag = lag;
        }
    }

    if (bestLag <= 0)
        return 0.0f;

    double refinedLag = (double) bestLag;
    if (bestLag > minLag && bestLag < maxLag)
    {
        const double ym = cmndf[bestLag - 1];
        const double y0 = cmndf[bestLag];
        const double yp = cmndf[bestLag + 1];
        const double denom = ym - 2.0 * y0 + yp;
        if (std::abs(denom) > 1.0e-9)
            refinedLag += 0.5 * (ym - yp) / denom;
    }

    confidence = juce::jlimit(0.0f, 1.0f, 1.0f - bestScore / 0.45f);
    if (confidence < 0.18f)
        return 0.0f;

    return (float) (sampleRate_ / juce::jmax(1.0, refinedLag));
}

float VocalPitchCorrector::quantizeMidi(float midi) const noexcept
{
    if (midi <= 0.0f || scaleType_ == 2) return midi;

    const int center = (int)std::lround(midi);
    float best = (float)center;
    float bestDistance = 1000.0f;

    for (int note = center - 12; note <= center + 12; ++note)
    {
        const int pc = (note % 12 + 12) % 12;
        if (!isInScale(pc, root_, scaleType_)) continue;
        const float d = std::abs(midi - (float)note);
        if (d < bestDistance)
        {
            bestDistance = d;
            best = (float)note;
        }
    }
    return best;
}

float VocalPitchCorrector::smoothTarget(float target, float detected, float confidence) noexcept
{
    if (confidence < 0.20f || target <= 0.0f || detected <= 0.0f)
        return currentRatio_;

    if (stableMidi_ <= 0.0f)
    {
        stableMidi_ = detected;
        stableTarget_ = target;
        stableFrames_ = 1;
    }
    else
    {
        // Do not wrap this comparison at the octave. C4 -> C5 is a real octave
        // change and must never be treated as a zero-distance pitch movement.
        const float distance = std::abs(detected - stableMidi_);
        if (distance > 0.65f)
        {
            stableMidi_ = detected;
            stableFrames_ = 0;
        }
        else
        {
            stableMidi_ = 0.88f * stableMidi_ + 0.12f * detected;
        }

        const float targetDistance = std::abs(target - stableTarget_);
        if (targetDistance > 1.0f)
            stableTarget_ = target;
        else
            stableTarget_ = 0.92f * stableTarget_ + 0.08f * target;

        ++stableFrames_;
    }

    // Preserve natural vibrato by letting fast pitch motion through while correcting
    // slower drift toward the scale note. Faster speeds intentionally retain less motion.
    const float vibratoPreserve = juce::jmap(speedMs_, 5.0f, 250.0f, 0.08f, 0.72f);
    const float desiredCents = (stableTarget_ - detected) * 100.0f * correction_;
    const float preservedCents = desiredCents * (1.0f - vibratoPreserve * 0.35f);

    const float currentCents = 1200.0f * std::log2(juce::jmax(0.25f, currentRatio_));
    const float tau = juce::jmax(0.005f, speedMs_ * 0.001f);
    const float alpha = 1.0f - std::exp(-1.0f / (float)(sampleRate_ * tau));
    const float newCents = currentCents + (preservedCents - currentCents) * alpha;
    currentRatio_ = std::pow(2.0f, newCents / 1200.0f);
    return currentRatio_;
}

void VocalPitchCorrector::process(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (!enabled_ || numSamples <= 0 || buffer.getNumChannels() <= 0) return;

    const int channels = juce::jmin(2, buffer.getNumChannels());

    for (int i = 0; i < numSamples; ++i)
    {
        float s = buffer.getSample(0, i);
        if (channels > 1) s = 0.5f * (s + buffer.getSample(1, i));
        ring_[(size_t)writePos_] = s;
        writePos_ = (writePos_ + 1) & ringMask_;
    }

    const int analysisN = juce::jmin(grainSize_, ringSize_ - 1);
    int p = (writePos_ - analysisN + ringSize_) & ringMask_;
    for (int i = 0; i < analysisN; ++i)
        analysisBuffer_[(size_t)i] = ring_[(size_t)((p + i) & ringMask_)];

    float conf = 0.0f;
    const float hz = detectPitch(analysisBuffer_.data(), analysisN, conf);
    confidence_.store(conf, std::memory_order_relaxed);

    if (hz > 0.0f)
    {
        const float midi = 69.0f + 12.0f * std::log2(hz / 440.0f);
        const float target = quantizeMidi(midi);

        detectedMidi_.store(midi, std::memory_order_relaxed);
        targetMidi_.store(target, std::memory_order_relaxed);
        smoothTarget(target, midi, conf);
    }
    else
    {
        confidence_.store(0.0f, std::memory_order_relaxed);
        // Stop pitch shifting quickly when the detector loses a reliable voiced
        // fundamental. This keeps breaths/consonants from being dragged by the
        // previous note and removes a common source of robotic doubling.        currentRatio_ = 1.0f + (currentRatio_ - 1.0f) * 0.70f;
    }

    if (numSamples > outBuffer_.getNumSamples())
    {
        // Hosts normally keep the prepared block size, but never read/write past the
        // preallocated buffers if an unusual host changes it.
        buffer.applyGain(1.0f);
        return;
    }

    auto* out = outBuffer_.getWritePointer(0);
    auto* weight = weightBuffer_.getWritePointer(0);
    juce::FloatVectorOperations::clear(out, numSamples);
    juce::FloatVectorOperations::clear(weight, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        const int global = hopCounter_++;
        if (global % hopSize_ == 0)
        {
            Grain& g = grains_[(global / hopSize_) & 1];

            if (!pitchReadInitialised_)
            {
                nextGrainReadPos_ = (double) ((writePos_ - grainSize_ + ringSize_) & ringMask_);
                pitchReadInitialised_ = true;
            }

            // Consecutive grains must read consecutive sections of the source.
            // The previous implementation started both overlapping grains at the
            // same sample, which is a direct cause of comb-filtering/doubling.
            g.readPos = nextGrainReadPos_;
            g.increment = juce::jlimit(0.50, 2.00, (double) currentRatio_);
            g.age = 0;
            g.active = true;

            nextGrainReadPos_ += (double) hopSize_ * g.increment;

            // Keep the read head safely inside the rolling history. Re-anchoring
            // only when it approaches the write head prevents runaway drift.
            const double wrappedNext = std::fmod(nextGrainReadPos_, (double) ringSize_);
            nextGrainReadPos_ = wrappedNext < 0.0 ? wrappedNext + ringSize_ : wrappedNext;
            const int readIndex = (int) std::floor(nextGrainReadPos_) & ringMask_;
            const int delay = (writePos_ - readIndex + ringSize_) & ringMask_;
            if (delay < hopSize_ || delay > ringSize_ - grainSize_ - hopSize_)
                nextGrainReadPos_ = (double) ((writePos_ - grainSize_ + ringSize_) & ringMask_);
        }

        for (auto& g : grains_)
        {
            if (!g.active || g.age >= grainSize_) continue;

            const int idx = (int)std::floor(g.readPos) & ringMask_;
            const int idx2 = (idx + 1) & ringMask_;
            const float frac = (float)(g.readPos - std::floor(g.readPos));
            const float s = ring_[(size_t)idx] + (ring_[(size_t)idx2] - ring_[(size_t)idx]) * frac;

            const float phase = (float)g.age / (float)grainSize_;
            const float w = std::sin(juce::MathConstants<float>::pi * phase);
            out[i] += s * w;
            weight[i] += w;

            g.readPos += g.increment;
            ++g.age;
            if (g.age >= grainSize_) g.active = false;
        }
    }

    for (int i = 0; i < numSamples; ++i)
    {
        const float w = weight[(size_t)i];
        if (w > 1.0e-4f)
        {
            const float y = out[(size_t)i] / w;
            for (int c = 0; c < channels; ++c)
                buffer.setSample(c, i, y);
        }
    }
}
