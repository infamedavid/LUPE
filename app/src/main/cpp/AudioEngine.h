#pragma once

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

    oboe::DataCallbackResult onAudioReady(
            oboe::AudioStream* audioStream,
            void* audioData,
            int32_t numFrames) override;

    void onErrorAfterClose(
            oboe::AudioStream* audioStream,
            oboe::Result error) override;

private:
    bool openOutputStreamLocked();

    mutable std::mutex mutex_;
    std::shared_ptr<oboe::AudioStream> outputStream_;
};

} // namespace lupe
