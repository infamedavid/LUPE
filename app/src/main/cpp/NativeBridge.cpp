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
