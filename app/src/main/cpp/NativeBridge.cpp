#include <jni.h>

#include <memory>
#include <mutex>

#include "AudioEngine.h"

namespace {
std::mutex gMutex;
std::unique_ptr<lupe::AudioEngine> gEngine;

lupe::AudioEngine& engine() {
    std::scoped_lock lock(gMutex);
    if (!gEngine) {
        gEngine = std::make_unique<lupe::AudioEngine>();
    }
    return *gEngine;
}
} // namespace

extern "C" JNIEXPORT jboolean JNICALL
Java_com_infamedavid_lupe_NativeAudioEngine_start(JNIEnv*, jobject) {
    return engine().start() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_infamedavid_lupe_NativeAudioEngine_stop(JNIEnv*, jobject) {
    engine().stop();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_infamedavid_lupe_NativeAudioEngine_isRunning(JNIEnv*, jobject) {
    return engine().isRunning() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_infamedavid_lupe_NativeAudioEngine_sampleRate(JNIEnv*, jobject) {
    return engine().sampleRate();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_infamedavid_lupe_NativeAudioEngine_framesPerBurst(JNIEnv*, jobject) {
    return engine().framesPerBurst();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_infamedavid_lupe_NativeAudioEngine_bufferSizeInFrames(JNIEnv*, jobject) {
    return engine().bufferSizeInFrames();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_infamedavid_lupe_NativeAudioEngine_deviceId(JNIEnv*, jobject) {
    return engine().deviceId();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_infamedavid_lupe_NativeAudioEngine_inputAvailable(JNIEnv*, jobject) {
    return engine().inputAvailable() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_infamedavid_lupe_NativeAudioEngine_inputSampleRate(JNIEnv*, jobject) {
    return engine().inputSampleRate();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_infamedavid_lupe_NativeAudioEngine_inputDeviceId(JNIEnv*, jobject) {
    return engine().inputDeviceId();
}

extern "C" JNIEXPORT void JNICALL
Java_com_infamedavid_lupe_NativeAudioEngine_setTestToneEnabled(
        JNIEnv*,
        jobject,
        jboolean enabled) {
    engine().setTestToneEnabled(enabled == JNI_TRUE);
}

extern "C" JNIEXPORT void JNICALL
Java_com_infamedavid_lupe_NativeAudioEngine_setInputMonitorEnabled(
        JNIEnv*,
        jobject,
        jboolean enabled) {
    engine().setInputMonitorEnabled(enabled == JNI_TRUE);
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_infamedavid_lupe_NativeAudioEngine_inputPeakAndReset(JNIEnv*, jobject) {
    return engine().inputPeakAndReset();
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_infamedavid_lupe_NativeAudioEngine_callbackCount(JNIEnv*, jobject) {
    return static_cast<jlong>(engine().callbackCount());
}

extern "C" JNIEXPORT jint JNICALL
Java_com_infamedavid_lupe_NativeAudioEngine_outputXRunCount(JNIEnv*, jobject) {
    return engine().outputXRunCount();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_infamedavid_lupe_NativeAudioEngine_inputXRunCount(JNIEnv*, jobject) {
    return engine().inputXRunCount();
}
