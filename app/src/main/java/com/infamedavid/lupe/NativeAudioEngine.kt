package com.infamedavid.lupe

object NativeAudioEngine {
    init {
        System.loadLibrary("lupe_audio")
    }

    external fun start(): Boolean
    external fun stop()
    external fun isRunning(): Boolean
    external fun sampleRate(): Int
    external fun framesPerBurst(): Int
    external fun bufferSizeInFrames(): Int
    external fun deviceId(): Int
}
