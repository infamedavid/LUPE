package com.infamedavid.lupe

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
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
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContent {
            MaterialTheme {
                Surface(modifier = Modifier.fillMaxSize()) {
                    var running by remember { mutableStateOf(NativeAudioEngine.isRunning()) }
                    var message by remember { mutableStateOf("Engine idle") }

                    Column(
                        modifier = Modifier
                            .fillMaxSize()
                            .padding(20.dp),
                        verticalArrangement = Arrangement.spacedBy(12.dp)
                    ) {
                        Text("LUPE", style = MaterialTheme.typography.headlineMedium)
                        Text("Phase 0 · native audio scaffold")

                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.spacedBy(12.dp)
                        ) {
                            Button(
                                enabled = !running,
                                onClick = {
                                    running = NativeAudioEngine.start()
                                    message = if (running) {
                                        "Audio stream running"
                                    } else {
                                        "Could not start audio stream"
                                    }
                                }
                            ) {
                                Text("START AUDIO")
                            }

                            Button(
                                enabled = running,
                                onClick = {
                                    NativeAudioEngine.stop()
                                    running = false
                                    message = "Engine stopped"
                                }
                            ) {
                                Text("STOP AUDIO")
                            }
                        }

                        Spacer(Modifier.height(4.dp))
                        Text(message)

                        if (running) {
                            Text("Sample rate: ${NativeAudioEngine.sampleRate()} Hz")
                            Text("Frames / burst: ${NativeAudioEngine.framesPerBurst()}")
                            Text("Buffer: ${NativeAudioEngine.bufferSizeInFrames()} frames")
                            Text("Device ID: ${NativeAudioEngine.deviceId()}")
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
