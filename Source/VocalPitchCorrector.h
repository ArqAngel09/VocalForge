#pragma once
#include <JuceHeader.h>

class VocalPitchCorrector
{
public:
    VocalPitchCorrector() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setEnabled(bool enabled) noexcept { enabled_ = enabled; }
    void setCorrection(float amount01) noexcept { correction_ = juce::jlimit(0.0f, 1.0f, amount01); }
    void setSpeed(float speedMs) noexcept { speedMs_ = juce::jlimit(5.0f, 250.0f, speedMs); }
    void setScale(int root, int scaleType) noexcept { root_ = (root % 12 + 12) % 12; scaleType_ = juce::jlimit(0, 2, scaleType); }

    void process(juce::AudioBuffer<float>& buffer, int numSamples);

    float getDetectedMidi() const noexcept { return detectedMidi_.load(std::memory_order_relaxed); }
    float getTargetMidi() const noexcept { return targetMidi_.load(std::memory_order_relaxed); }
    float getConfidence() const noexcept { return confidence_.load(std::memory_order_relaxed); }

private:
    struct Grain
    {
        double readPos = 0.0;
        double increment = 1.0;
        int age = 0;
        bool active = false;
    };

    float detectPitch(float* x, int n, float& confidence) noexcept;
    float quantizeMidi(float midi) const noexcept;
    float smoothTarget(float targetMidi, float detectedMidi, float confidence) noexcept;
    static float wrapMidiDistance(float a, float b) noexcept;

    double sampleRate_ = 44100.0;
    int grainSize_ = 1024;
    int hopSize_ = 512;
    int ringSize_ = 16384;
    int ringMask_ = 16383;
    int writePos_ = 0;
    int hopCounter_ = 0;

    std::vector<float> ring_;
    juce::AudioBuffer<float> outBuffer_;
    juce::AudioBuffer<float> weightBuffer_;
    std::vector<float> analysisBuffer_;
    std::vector<float> differenceBuffer_;
    std::vector<float> cmndfBuffer_;

    Grain grains_[2];
    float currentRatio_ = 1.0f;
    double nextGrainReadPos_ = 0.0;
    bool pitchReadInitialised_ = false;
    float correction_ = 1.0f;
    float speedMs_ = 45.0f;
    int root_ = 0;
    int scaleType_ = 0;
    bool enabled_ = true;

    float stableMidi_ = 0.0f;
    float stableTarget_ = 0.0f;
    int stableFrames_ = 0;

    std::atomic<float> detectedMidi_ { 0.0f };
    std::atomic<float> targetMidi_ { 0.0f };
    std::atomic<float> confidence_ { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalPitchCorrector)
};
