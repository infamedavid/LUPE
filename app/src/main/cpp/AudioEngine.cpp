#include "AudioEngine.h"

#include <algorithm>

#include <android/log.h>

namespace {
constexpr const char* kLogTag = "LUPE-Audio";

void logError(const char* message, oboe::Result result) {
    __android_log_print(
            ANDROID_LOG_ERROR,
            kLogTag,
            "%s: %s",
            message,
            oboe::convertToText(result));
}
} // namespace

namespace lupe {

AudioEngine::~AudioEngine() {
    stop();
}

bool AudioEngine::start() {
    std::scoped_lock lock(mutex_);

    if (outputStream_ && outputStream_->getState() == oboe::StreamState::Started) {
        return true;
    }

    if (!openOutputStreamLocked()) {
        return false;
    }

    const oboe::Result result = outputStream_->requestStart();
    if (result != oboe::Result::OK) {
        logError("requestStart failed", result);
        outputStream_->close();
        outputStream_.reset();
        return false;
    }

    __android_log_print(
            ANDROID_LOG_INFO,
            kLogTag,
            "Started: %d Hz, %d channels, %d frames/burst, device %d",
            outputStream_->getSampleRate(),
            outputStream_->getChannelCount(),
            outputStream_->getFramesPerBurst(),
            outputStream_->getDeviceId());

    return true;
}

void AudioEngine::stop() {
    std::scoped_lock lock(mutex_);

    if (!outputStream_) {
        return;
    }

    outputStream_->requestStop();
    outputStream_->close();
    outputStream_.reset();

    __android_log_print(ANDROID_LOG_INFO, kLogTag, "Stopped");
}

bool AudioEngine::openOutputStreamLocked() {
    oboe::AudioStreamBuilder builder;

    builder.setDirection(oboe::Direction::Output)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Exclusive)
            ->setFormat(oboe::AudioFormat::Float)
            ->setChannelCount(oboe::ChannelCount::Stereo)
            ->setDataCallback(this)
            ->setErrorCallback(this);

    oboe::Result result = builder.openStream(outputStream_);

    if (result != oboe::Result::OK) {
        __android_log_print(
                ANDROID_LOG_WARN,
                kLogTag,
                "Exclusive output unavailable, retrying shared");

        builder.setSharingMode(oboe::SharingMode::Shared);
        result = builder.openStream(outputStream_);
    }

    if (result != oboe::Result::OK || !outputStream_) {
        logError("openStream failed", result);
        outputStream_.reset();
        return false;
    }

    return true;
}

bool AudioEngine::isRunning() const {
    std::scoped_lock lock(mutex_);
    return outputStream_ && outputStream_->getState() == oboe::StreamState::Started;
}

int32_t AudioEngine::sampleRate() const {
    std::scoped_lock lock(mutex_);
    return outputStream_ ? outputStream_->getSampleRate() : 0;
}

int32_t AudioEngine::framesPerBurst() const {
    std::scoped_lock lock(mutex_);
    return outputStream_ ? outputStream_->getFramesPerBurst() : 0;
}

int32_t AudioEngine::bufferSizeInFrames() const {
    std::scoped_lock lock(mutex_);
    return outputStream_ ? outputStream_->getBufferSizeInFrames() : 0;
}

int32_t AudioEngine::deviceId() const {
    std::scoped_lock lock(mutex_);
    return outputStream_ ? outputStream_->getDeviceId() : oboe::kUnspecified;
}

oboe::DataCallbackResult AudioEngine::onAudioReady(
        oboe::AudioStream* audioStream,
        void* audioData,
        int32_t numFrames) {
    auto* output = static_cast<float*>(audioData);
    const int32_t channels = audioStream->getChannelCount();

    std::fill(output, output + (numFrames * channels), 0.0f);
    return oboe::DataCallbackResult::Continue;
}

void AudioEngine::onErrorAfterClose(
        oboe::AudioStream*,
        oboe::Result error) {
    logError("Audio stream closed after error", error);
}

} // namespace lupe
