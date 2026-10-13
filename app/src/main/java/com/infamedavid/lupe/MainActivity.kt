package com.infamedavid.lupe

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.delay
import kotlin.math.log10

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContent {
            MaterialTheme {
                Surface(modifier = Modifier.fillMaxSize()) {
                    var running by remember { mutableStateOf(NativeAudioEngine.isRunning()) }
                    var message by remember { mutableStateOf("Engine idle") }
                    var testTone by remember { mutableStateOf(false) }
                    var monitor by remember { mutableStateOf(false) }

                    var inputPeak by remember { mutableStateOf(0.0f) }
                    var callbacks by remember { mutableStateOf(0L) }
                    var outputXruns by remember { mutableStateOf(-1) }
                    var inputXruns by remember { mutableStateOf(-1) }

                    fun startEngine() {
                        running = NativeAudioEngine.start()
                        message = if (running) {
                            if (NativeAudioEngine.inputAvailable()) {
                                "Duplex audio running"
                            } else {
                                "Output running · input unavailable"
                            }
                        } else {
                            "Could not start audio engine"
                        }
                    }

                    val permissionLauncher = rememberLauncherForActivityResult(
                        ActivityResultContracts.RequestPermission()
                    ) { granted ->
                        if (granted) {
                            startEngine()
                        } else {
                            message = "Microphone permission denied"
                        }
                    }

                    LaunchedEffect(running) {
                        while (running) {
                            inputPeak = NativeAudioEngine.inputPeakAndReset()
                            callbacks = NativeAudioEngine.callbackCount()
                            outputXruns = NativeAudioEngine.outputXRunCount()
                            inputXruns = NativeAudioEngine.inputXRunCount()
                            delay(250)
                        }
                    }

                    Column(
                        modifier = Modifier
                            .fillMaxSize()
                            .padding(20.dp),
                        verticalArrangement = Arrangement.spacedBy(12.dp)
                    ) {
                        Text("LUPE", style = MaterialTheme.typography.headlineMedium)
                        Text("Phase 1 · duplex audio diagnostics")

                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.spacedBy(12.dp)
                        ) {
                            Button(
                                enabled = !running,
                                onClick = {
                                    if (checkSelfPermission(Manifest.permission.RECORD_AUDIO) ==
                                        PackageManager.PERMISSION_GRANTED
                                    ) {
                                        startEngine()
                                    } else {
                                        permissionLauncher.launch(Manifest.permission.RECORD_AUDIO)
                                    }
                                }
                            ) {
                                Text("START AUDIO")
                            }

                            Button(
                                enabled = running,
                                onClick = {
                                    NativeAudioEngine.setTestToneEnabled(false)
                                    NativeAudioEngine.setInputMonitorEnabled(false)
                                    NativeAudioEngine.stop()
                                    testTone = false
                                    monitor = false
                                    running = false
                                    message = "Engine stopped"
                                }
                            ) {
                                Text("STOP AUDIO")
                            }
                        }

                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.spacedBy(12.dp)
                        ) {
                            Button(
                                enabled = running,
                                onClick = {
                                    testTone = !testTone
                                    NativeAudioEngine.setTestToneEnabled(testTone)
                                }
                            ) {
                                Text(if (testTone) "TONE ON" else "TEST TONE")
                            }

                            Button(
                                enabled = running && NativeAudioEngine.inputAvailable(),
                                onClick = {
                                    monitor = !monitor
                                    NativeAudioEngine.setInputMonitorEnabled(monitor)
                                }
                            ) {
                                Text(if (monitor) "MONITOR ON" else "INPUT MONITOR")
                            }
                        }

                        Spacer(Modifier.height(4.dp))
                        Text(message)

                        if (running) {
                            val peakDb = if (inputPeak > 0.0f) {
                                (20.0 * log10(inputPeak.toDouble())).coerceAtLeast(-120.0)
                            } else {
                                -120.0
                            }

                            Text("Output: ${NativeAudioEngine.sampleRate()} Hz")
                            Text("Frames / burst: ${NativeAudioEngine.framesPerBurst()}")
                            Text("Buffer: ${NativeAudioEngine.bufferSizeInFrames()} frames")
                            Text("Output device ID: ${NativeAudioEngine.deviceId()}")

                            if (NativeAudioEngine.inputAvailable()) {
                                Text("Input: ${NativeAudioEngine.inputSampleRate()} Hz")
                                Text("Input device ID: ${NativeAudioEngine.inputDeviceId()}")
                                Text("Input peak: %.1f dBFS".format(peakDb))
                            } else {
                                Text("Input: unavailable")
                            }

                            Text("Audio callbacks: $callbacks")
                            Text("Output xruns: $outputXruns")
                            Text("Input xruns: $inputXruns")

                            if (monitor) {
                                Text("Input monitor is live. Use headphones or an audio interface to avoid feedback.")
                            }
                        }
                    }
                }
            }
        }
    }

    override fun onDestroy() {
        NativeAudioEngine.stop()
        super.onDestroy()
    }
}
