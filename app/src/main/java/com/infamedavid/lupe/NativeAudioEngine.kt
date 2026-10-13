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

    external fun inputAvailable(): Boolean
    external fun inputSampleRate(): Int
    external fun inputDeviceId(): Int

    external fun setTestToneEnabled(enabled: Boolean)
    external fun setInputMonitorEnabled(enabled: Boolean)

    external fun inputPeakAndReset(): Float
    external fun callbackCount(): Long
    external fun outputXRunCount(): Int
    external fun inputXRunCount(): Int
}
