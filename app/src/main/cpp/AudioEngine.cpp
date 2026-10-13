#include "AudioEngine.h"

#include <algorithm>
#include <cmath>

#include <android/log.h>

namespace {
constexpr const char* kLogTag = "LUPE-Audio";
constexpr double kTwoPi = 6.28318530717958647692;
constexpr double kTestToneHz = 440.0;
constexpr float kTestToneLevel = 0.10f;
constexpr float kMonitorLevel = 0.80f;

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

    callbackCount_.store(0);
    peakSinceRead_.store(0.0f);
    tonePhase_ = 0.0;

    if (!openOutputStreamLocked()) {
        return false;
    }

    if (openInputStreamLocked()) {
        const oboe::Result inputStart = inputStream_->requestStart();
        if (inputStart != oboe::Result::OK) {
            logError("Input requestStart failed", inputStart);
            inputStream_->close();
            inputStream_.reset();
        }
    } else {
        __android_log_print(
                ANDROID_LOG_WARN,
                kLogTag,
                "Input stream unavailable; continuing with output only");
    }

    const oboe::Result outputStart = outputStream_->requestStart();
    if (outputStart != oboe::Result::OK) {
        logError("Output requestStart failed", outputStart);

        if (inputStream_) {
            inputStream_->requestStop();
            inputStream_->close();
            inputStream_.reset();
        }

        outputStream_->close();
        outputStream_.reset();
        return false;
    }

    __android_log_print(
            ANDROID_LOG_INFO,
            kLogTag,
            "Output started: %d Hz, %d ch, %d frames/burst, device %d",
            outputStream_->getSampleRate(),
            outputStream_->getChannelCount(),
            outputStream_->getFramesPerBurst(),
            outputStream_->getDeviceId());

    if (inputStream_) {
        __android_log_print(
                ANDROID_LOG_INFO,
                kLogTag,
                "Input started: %d Hz, %d ch, device %d",
                inputStream_->getSampleRate(),
                inputStream_->getChannelCount(),
                inputStream_->getDeviceId());
    }

    return true;
}

void AudioEngine::stop() {
    std::scoped_lock lock(mutex_);

    testToneEnabled_.store(false);
    inputMonitorEnabled_.store(false);

    if (outputStream_) {
        outputStream_->requestStop();
        outputStream_->close();
        outputStream_.reset();
    }

    if (inputStream_) {
        inputStream_->requestStop();
        inputStream_->close();
        inputStream_.reset();
    }

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
        logError("Output openStream failed", result);
        outputStream_.reset();
        return false;
    }

    return true;
}

bool AudioEngine::openInputStreamLocked() {
    if (!outputStream_) {
        return false;
    }

    oboe::AudioStreamBuilder builder;

    builder.setDirection(oboe::Direction::Input)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Shared)
            ->setFormat(oboe::AudioFormat::Float)
            ->setChannelCount(oboe::ChannelCount::Mono)
            ->setSampleRate(outputStream_->getSampleRate())
            ->setErrorCallback(this);

    const oboe::Result result = builder.openStream(inputStream_);

    if (result != oboe::Result::OK || !inputStream_) {
        logError("Input openStream failed", result);
        inputStream_.reset();
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

bool AudioEngine::inputAvailable() const {
    std::scoped_lock lock(mutex_);
    return static_cast<bool>(inputStream_);
}

int32_t AudioEngine::inputSampleRate() const {
    std::scoped_lock lock(mutex_);
    return inputStream_ ? inputStream_->getSampleRate() : 0;
}

int32_t AudioEngine::inputDeviceId() const {
    std::scoped_lock lock(mutex_);
    return inputStream_ ? inputStream_->getDeviceId() : oboe::kUnspecified;
}

void AudioEngine::setTestToneEnabled(bool enabled) {
    testToneEnabled_.store(enabled);
}

void AudioEngine::setInputMonitorEnabled(bool enabled) {
    inputMonitorEnabled_.store(enabled);
}

float AudioEngine::inputPeakAndReset() {
    return peakSinceRead_.exchange(0.0f);
}

int64_t AudioEngine::callbackCount() const {
    return callbackCount_.load();
}

int32_t AudioEngine::xRunCount(const std::shared_ptr<oboe::AudioStream>& stream) {
    if (!stream) {
        return -1;
    }

    const auto result = stream->getXRunCount();
    return result ? result.value() : -1;
}

int32_t AudioEngine::outputXRunCount() const {
    std::scoped_lock lock(mutex_);
    return xRunCount(outputStream_);
}

int32_t AudioEngine::inputXRunCount() const {
    std::scoped_lock lock(mutex_);
    return xRunCount(inputStream_);
}

oboe::DataCallbackResult AudioEngine::onAudioReady(
        oboe::AudioStream* audioStream,
        void* audioData,
        int32_t numFrames) {
    auto* output = static_cast<float*>(audioData);
    const int32_t outputChannels = audioStream->getChannelCount();

    std::fill(output, output + (numFrames * outputChannels), 0.0f);
    callbackCount_.fetch_add(1, std::memory_order_relaxed);

    int32_t framesRead = 0;

    if (inputStream_) {
        const int32_t framesToRead = std::min(numFrames, kMaxInputFrames);
        const auto readResult = inputStream_->read(
                inputBuffer_.data(),
                framesToRead,
                0);

        if (readResult) {
            framesRead = readResult.value();
        }
    }

    float blockPeak = 0.0f;

    for (int32_t frame = 0; frame < framesRead; ++frame) {
        blockPeak = std::max(blockPeak, std::abs(inputBuffer_[frame]));
    }

    float previousPeak = peakSinceRead_.load(std::memory_order_relaxed);
    while (blockPeak > previousPeak &&
           !peakSinceRead_.compare_exchange_weak(
                   previousPeak,
                   blockPeak,
                   std::memory_order_relaxed)) {
    }

    if (inputMonitorEnabled_.load(std::memory_order_relaxed) && framesRead > 0) {
        for (int32_t frame = 0; frame < framesRead; ++frame) {
            const float sample = inputBuffer_[frame] * kMonitorLevel;
            for (int32_t channel = 0; channel < outputChannels; ++channel) {
                output[(frame * outputChannels) + channel] += sample;
            }
        }
    }

    if (testToneEnabled_.load(std::memory_order_relaxed)) {
        const double phaseIncrement =
                kTwoPi * kTestToneHz / static_cast<double>(audioStream->getSampleRate());

        for (int32_t frame = 0; frame < numFrames; ++frame) {
            const float sample =
                    static_cast<float>(std::sin(tonePhase_)) * kTestToneLevel;

            for (int32_t channel = 0; channel < outputChannels; ++channel) {
                output[(frame * outputChannels) + channel] += sample;
            }

            tonePhase_ += phaseIncrement;
            if (tonePhase_ >= kTwoPi) {
                tonePhase_ -= kTwoPi;
            }
        }
    }

    return oboe::DataCallbackResult::Continue;
}

void AudioEngine::onErrorAfterClose(
        oboe::AudioStream*,
        oboe::Result error) {
    logError("Audio stream closed after error", error);
}

} // namespace lupe
