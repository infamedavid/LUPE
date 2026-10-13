#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

#include <oboe/Oboe.h>

namespace lupe {

class AudioEngine final : public oboe::AudioStreamDataCallback,
                          public oboe::AudioStreamErrorCallback {
public:
    AudioEngine() = default;
    ~AudioEngine() override;

    bool start();
    void stop();

    bool isRunning() const;

    int32_t sampleRate() const;
    int32_t framesPerBurst() const;
    int32_t bufferSizeInFrames() const;
    int32_t deviceId() const;

    bool inputAvailable() const;
    int32_t inputSampleRate() const;
    int32_t inputDeviceId() const;

    void setTestToneEnabled(bool enabled);
    void setInputMonitorEnabled(bool enabled);

    float inputPeakAndReset();
    int64_t callbackCount() const;
    int32_t outputXRunCount() const;
    int32_t inputXRunCount() const;

    oboe::DataCallbackResult onAudioReady(
            oboe::AudioStream* audioStream,
            void* audioData,
            int32_t numFrames) override;

    void onErrorAfterClose(
            oboe::AudioStream* audioStream,
            oboe::Result error) override;

private:
    static constexpr int32_t kMaxInputFrames = 8192;

    bool openOutputStreamLocked();
    bool openInputStreamLocked();
    static int32_t xRunCount(const std::shared_ptr<oboe::AudioStream>& stream);

    mutable std::mutex mutex_;
    std::shared_ptr<oboe::AudioStream> outputStream_;
    std::shared_ptr<oboe::AudioStream> inputStream_;

    std::array<float, kMaxInputFrames> inputBuffer_{};

    std::atomic<bool> testToneEnabled_{false};
    std::atomic<bool> inputMonitorEnabled_{false};
    std::atomic<float> peakSinceRead_{0.0f};
    std::atomic<int64_t> callbackCount_{0};

    double tonePhase_ = 0.0;
};

} // namespace lupe
