#pragma once
#include <JuceHeader.h>

class VocalPitchCorrector
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setEnabled(bool enabled) noexcept { enabled_ = enabled; }
    void setCorrection(float amount01) noexcept { correction_ = juce::jlimit(0.0f, 1.0f, amount01); }
    void setSpeed(float speedMs) noexcept { speedMs_ = juce::jlimit(5.0f, 250.0f, speedMs); }
    void setScale(int root, int scaleType) noexcept { root_ = root % 12; scaleType_ = scaleType; }

    void process(juce::AudioBuffer<float>& buffer, int numSamples);
    float getDetectedMidi() const noexcept { return detectedMidi_.load(); }
    float getTargetMidi() const noexcept { return targetMidi_.load(); }
    float getConfidence() const noexcept { return confidence_.load(); }

private:
    struct Grain
    {
        double readPos = 0.0;
        double increment = 1.0;
        int age = 0;
        bool active = false;
    };

    float detectPitch(const float* x, int n, float& confidence) const;
    float quantizeMidi(float midi) const;
    float smoothTarget(float targetMidi);

    double sampleRate_ = 44100.0;
    int grainSize_ = 1024;
    int hopSize_ = 512;
    int ringSize_ = 16384;
    int ringMask_ = 16383;
    int writePos_ = 0;
    int hopCounter_ = 0;
    std::vector<float> ring_;
    std::vector<float> grainBuffer_;

    Grain grains_[2];
    float currentRatio_ = 1.0f;
    float correction_ = 1.0f;
    float speedMs_ = 45.0f;
    int root_ = 0;
    int scaleType_ = 0;
    bool enabled_ = true;

    std::atomic<float> detectedMidi_ { 0.0f };
    std::atomic<float> targetMidi_ { 0.0f };
    std::atomic<float> confidence_ { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalPitchCorrector)
};
