# LUPE — Roadmap de implementación

**Estado:** Base de planificación  
**Plataforma inicial:** Android  
**Documento vivo:** este roadmap puede cambiar a medida que la implementación y las pruebas revelen nuevas necesidades.

---

# Objetivo general

LUPE se construirá por capas.

Cada fase debe dejar una parte del sistema funcional y verificable antes de avanzar a la siguiente. El orden busca reducir riesgo: primero audio estable y timing, después grabación y loop, luego manipulación de read-heads, después multipista, y finalmente herramientas de performance, persistencia y acabado.

La regla principal del roadmap es:

> No avanzar una fase crítica mientras la anterior todavía tenga problemas de sincronización, clicks, deriva temporal, latencia o estabilidad.

---

# Fase 0 — Repositorio y arquitectura base

## Objetivo

Crear la estructura mínima del proyecto y separar desde el inicio la UI del motor de audio.

## Implementación

```text
Android App
│
├── Kotlin / Jetpack Compose
│   ├── UI
│   ├── Session state
│   └── File/session management
│
├── JNI bridge
│
└── C++ Audio Engine
    └── Oboe / AAudio
```

Preparar:

- repositorio Git;
- proyecto Android;
- build C++ con NDK;
- integración Oboe;
- JNI;
- logging del motor;
- manejo básico del ciclo de vida de audio;
- detección de sample rate;
- detección de buffer size;
- selección de input/output disponible.

## Criterio de salida

La aplicación abre, inicia el motor y muestra correctamente:

```text
sample rate
buffer size
input device
output device
```

El audio engine puede arrancar y detenerse sin crash ni bloqueo.

---

# Fase 1 — Audio duplex mínimo

## Objetivo

Conseguir entrada y salida simultáneas estables.

## Implementación

Crear un callback duplex capaz de:

```text
INPUT
  ↓
buffer
  ↓
OUTPUT
```

Añadir monitorización directa configurable.

Registrar:

- sample rate real;
- frames por callback;
- underruns;
- xruns;
- cambios de dispositivo;
- desconexión/reconexión de audio.

## Hardware

Probar primero con el audio interno del teléfono.

Cuando esté disponible:

```text
Behringer UCA222
```

repetir las pruebas mediante USB audio.

## Criterio de salida

Entrada y salida funcionan durante sesiones prolongadas sin:

- glitches recurrentes;
- bloqueos;
- memory leaks;
- pérdida del stream;
- cambios inesperados de sample rate.

---

# Fase 2 — Transporte maestro

## Objetivo

Crear el reloj musical del sistema.

## Estado interno

Mantener al menos:

```text
masterFrame
musicalPosition
BPM
timeSignature
transportState
```

El motor debe poder traducir tiempo de audio a:

```text
beats
bars
phase
```

sin depender de timers de Android.

## Comportamiento

El transporte deriva directamente del audio callback.

La UI sólo representa ese estado y puede refrescar a menor frecuencia.

## Criterio de salida

Con metrónomo temporal de prueba:

- el beat permanece estable;
- no aparece deriva después de cientos de compases;
- cambios de BPM mantienen continuidad musical;
- la posición musical es sample-accurate dentro del callback.

---

# Fase 3 — Single Track Core

## Objetivo

Construir el primer loop funcional.

## Flujo

```text
configurar BPM
↓
configurar compás
↓
configurar duración
↓
REC
↓
capturar exactamente N beats
↓
fin de grabación
↓
PLAY automático
↓
loop continuo
```

La longitud del buffer debe derivarse del transporte musical.

## Controles mínimos

```text
BPM
TIME SIGNATURE
LENGTH
REC
PLAY
STOP
```

La UI puede ser completamente provisional.

## Criterio de salida

Un loop grabado:

- comienza donde corresponde;
- termina donde corresponde;
- vuelve al inicio sin deriva;
- permanece sincronizado durante cientos o miles de vueltas.

---

# Fase 4 — Seam fade y cierre limpio del loop

## Objetivo

Eliminar clicks en el punto de unión.

## Implementación

Añadir microfade técnico alrededor de:

```text
END → START
```

El fade debe ser corto y prácticamente transparente.

La duración exacta se ajustará mediante prueba auditiva.

## Criterio de salida

Loops con:

- señales sostenidas;
- ondas simples;
- guitarra;
- percusión;
- señal cercana a zero crossing o lejos de él;

pueden repetirse sin clicks digitales evidentes.

---

# Fase 5 — TAIL XFADE

## Objetivo

Implementar la cola musical del loop.

## Comportamiento

Valor por defecto:

```text
TAIL XFADE = 4 beats
```

Configurable por pista:

```text
OFF
menos de 4 beats
4 beats
más de 4 beats
```

La grabación queda estructurada como:

```text
|--------- LOOP NOMINAL ---------|---- TAIL ----|
```

El tail no aumenta la duración musical del loop.

Durante la siguiente vuelta:

```text
TAIL anterior   100% → 0%
HEAD siguiente    0% → 100%
```

## Criterio de salida

Una nota o señal sostenida puede atravesar el final del loop sin corte brusco y sin modificar la duración del ciclo.

---

# Fase 6 — Modelo de fase y read-head

## Objetivo

Separar formalmente:

```text
EXPECTED POSITION
```

de:

```text
ACTUAL READ POSITION
```

Cada pista debe conocer en todo momento:

```text
expectedPhase
readPosition
direction
speed
offset
```

## Criterio de salida

El motor puede mover el read-head independientemente del transporte sin perder la referencia de dónde debería estar sincronizado.

---

# Fase 7 — Reverse sincronizado

## Objetivo

Añadir reproducción inversa manteniendo fase musical.

## Comportamiento

```text
FWD position = phase
REV position = 1 - phase
```

Cambiar:

```text
FWD ↔ REV
```

no cambia la fase musical.

Si `REV` está activo antes de grabar, el loop entra en reproducción inversa inmediatamente al terminar la toma.

## Criterio de salida

Cambiar dirección durante playback:

- no desplaza el track musicalmente;
- no genera clicks fuertes;
- conserva sincronía con el transporte.

---

# Fase 8 — RESYNC

## Objetivo

Permitir que cualquier pista vuelva instantáneamente a su fase correcta.

## Comportamiento

```text
RESYNC
↓
readPosition = expectedPosition
```

Debe respetar:

```text
direction
track length
current session position
```

Si la pista está en reverse, continúa en reverse.

Usar un pequeño crossfade para ocultar el salto.

## Criterio de salida

Después de manipular la posición del read-head, `RESYNC` devuelve el track al punto musical correcto sin modificar dirección ni velocidad configurada.

---

# Fase 9 — Scrub y superficie de posición

## Objetivo

Permitir interacción directa con la posición del audio.

## UI

Usar una superficie rectangular sin necesidad de waveform.

```text
┌───────────────────────────────┐
│                               │
└───────────────────────────────┘
```

## Gestos

```text
Tap  → jump
Drag → scrub
```

La posición horizontal corresponde a la posición relativa dentro del clip.

## Criterio de salida

El usuario puede recorrer manualmente el audio sin:

- crashes;
- discontinuidades graves;
- pérdida de referencia del master.

---

# Fase 10 — Offset de fase

## Objetivo

Añadir desplazamiento persistente respecto al master.

## Comportamiento

```text
trackPhase =
masterPhase + offset
```

con wrap-around.

Offset y scrub permanecen conceptos separados.

## Pendiente a decidir durante implementación

Definir con pruebas si:

```text
RESYNC
```

debe:

```text
A) conservar offset
```

o:

```text
B) resetear offset
```

## Criterio de salida

Una pista puede mantener un desplazamiento estable respecto al transporte durante tiempo indefinido.

---

# Fase 11 — Varispeed global

## Objetivo

Permitir cambiar la velocidad musical de toda la sesión.

## Relación

```text
playbackRate =
currentBpm / recordedBpm
```

Pitch y duración cambian juntos.

Ejemplo:

```text
120 BPM → 60 BPM
0.5x
-12 st
```

## Criterio de salida

Cambiar BPM durante playback altera correctamente:

- velocidad;
- pitch;
- duración;

sin romper la sincronización entre pistas ya compatibles.

---

# Fase 12 — Varispeed por pista

## Objetivo

Permitir velocidad independiente por track.

## Ejemplos

```text
0.25x
0.5x
1x
2x
4x
```

El read-head debe aceptar posiciones fraccionales.

## DSP necesario

Implementar interpolación/resampling eficiente.

Primera opción:

```text
cubic / Hermite
```

Evaluar calidad antes de considerar algoritmos más costosos.

## Criterio de salida

Una pista puede reproducirse a distintas velocidades sin artefactos intolerables ni consumo excesivo.

---

# Fase 13 — Velocidad negativa unificada

## Objetivo

Unificar dirección y velocidad en un único modelo matemático.

```text
+2.0x
+1.0x
+0.5x
0.0x
-0.5x
-1.0x
-2.0x
```

Esto prepara el motor para modulación continua y cambios de dirección suaves.

## Criterio de salida

El read-head puede atravesar correctamente:

```text
positivo → cero → negativo
```

sin estados inconsistentes.

---

# Fase 14 — LFO de varispeed

## Objetivo

Modular automáticamente la velocidad local.

## Parámetros iniciales

```text
RATE
DEPTH
SHAPE
```

Posibles shapes:

```text
SINE
TRI
S&H
```

Rate:

```text
HZ
SYNC
```

Depth preferiblemente expresado en semitonos.

Relación:

```text
rate = 2^(semitones / 12)
```

## Criterio de salida

El LFO puede producir:

- wow;
- flutter;
- wobble;
- aceleraciones;
- desaceleraciones;

sin comprometer estabilidad del motor.

---

# Fase 15 — Multipista

## Objetivo

Generalizar el motor de una pista a múltiples pistas.

## Cada pista mantiene

```text
buffer
length
recordedBpm
expectedPhase
readPosition
direction
speed
offset
volume
TAIL XFADE
```

Cada pista puede tener distinta longitud.

Ejemplo:

```text
Track 1 = 4 bars
Track 2 = 1 bar
Track 3 = 3 bars
Track 4 = 8 bars
```

## Criterio de salida

Varias pistas reproducen simultáneamente durante sesiones largas sin:

- deriva;
- pérdida de sincronía del master;
- glitches por mezcla;
- crecimiento descontrolado de CPU.

---

# Fase 16 — Mixer básico

## Objetivo

Controlar el nivel de cada pista y el master.

## Controles

```text
TRACK VOLUME
MUTE
SOLO
MASTER VOLUME
```

Pan queda opcional hasta evaluar su utilidad real.

## Criterio de salida

El usuario puede equilibrar una sesión multipista sin alterar el timing.

---

# Fase 17 — Repeater Core

## Objetivo

Añadir microloops temporales dentro de una pista.

El transporte principal continúa corriendo mientras el repeater está activo.

## Tamaños iniciales

```text
1/1
1/2
1/4
1/8
1/16
1/32
1/64
1/128
```

## Timing

```text
EVEN
TRIP
DOT
```

## Criterio de salida

Al liberar el repeater, la pista vuelve inmediatamente a la posición que le corresponde según el master.

---

# Fase 18 — Dirección del Repeater

## Objetivo

Separar dirección del track y dirección del repeater.

## Modos

```text
FWD
REV
ALT
RND
```

Ejemplo:

```text
TRACK DIR = REV
REP DIR   = FWD
```

debe ser válido.

## Criterio de salida

La dirección del repeater puede cambiar sin alterar la dirección principal del track.

---

# Fase 19 — GLTCH

## Objetivo

Automatizar activaciones del repeater.

## Base

Inspirarse en el comportamiento ya desarrollado para BEACON / Protoseq.

GLTCH puede decidir:

```text
trigger
SIZE
hold cycles
```

La dirección la determina:

```text
REP DIR
```

## Criterio de salida

GLTCH produce variaciones rítmicas controlables sin romper sincronía ni apropiarse permanentemente del playback.

---

# Fase 20 — Importación de audio

## Objetivo

Permitir crear pistas desde archivos existentes.

## Función

```text
+ TRACK
  ├── RECORD
  └── IMPORT
```

## Parámetros necesarios

Al importar:

```text
track length
recorded BPM
musical start
```

Si el BPM del archivo es desconocido, debe poder introducirse manualmente.

## Criterio de salida

Un archivo importado puede participar en:

- playback;
- reverse;
- varispeed;
- offset;
- repeater;
- resync.

---

# Fase 21 — Exportación

## Objetivo

Sacar material de LUPE sin pérdida innecesaria.

## Exportación mínima

```text
EXPORT TRACK
EXPORT ALL STEMS
EXPORT MASTER
```

Decidir posteriormente los formatos definitivos.

WAV debe ser el formato de referencia inicial.

## Criterio de salida

Los archivos exportados:

- mantienen duración correcta;
- mantienen sample rate;
- no introducen clicks;
- coinciden temporalmente con la sesión.

---

# Fase 22 — Save / Load Session

## Objetivo

Persistir una sesión completa.

## Guardar

```text
tempo
time signature
tracks
audio paths / audio data
track length
direction
speed
offset
volume
TAIL XFADE
repeater state
LFO parameters
```

## Criterio de salida

Cerrar y volver a abrir LUPE restaura la sesión de forma musicalmente idéntica.

---

# Fase 23 — Gestión de memoria y sesiones grandes

## Objetivo

Evaluar límites reales de teléfono/tablet.

## Medir

```text
RAM por minuto de audio
CPU por track
CPU por resampler
CPU del LFO
CPU del repeater
disk I/O
```

Decidir a partir de mediciones si conviene:

```text
audio completo en RAM
```

o:

```text
streaming desde almacenamiento
```

para pistas largas.

## Criterio de salida

LUPE maneja sesiones razonables sin cierres por memoria ni audio inestable.

---

# Fase 24 — Latencia y compensación

## Objetivo

Alinear correctamente lo que el músico toca con lo que ya está escuchando.

## Soporte

```text
Recording Offset
```

manual inicialmente.

Más adelante:

```text
AUTO CALIBRATION
```

mediante loopback.

La Behringer UCA222 será una de las referencias principales para estas pruebas.

## Criterio de salida

Una nueva toma grabada sobre un loop existente cae donde el músico la interpretó, dentro de una tolerancia musicalmente aceptable.

---

# Fase 25 — Audio device handling

## Objetivo

Hacer robusto el cambio de hardware.

Gestionar:

- audio interno;
- USB audio;
- desconexión de interfaz;
- reconexión;
- cambio de sample rate;
- cambio de buffer;
- pérdida temporal del dispositivo.

## Criterio de salida

Un cambio de dispositivo no corrompe la sesión ni deja el motor en un estado inválido.

---

# Fase 26 — UI funcional completa

## Objetivo

Construir la interfaz real sobre el motor ya probado.

## Áreas principales

```text
TRANSPORT
TRACK LIST
TRACK CONTROLS
POSITION / SCRUB PAD
VARISPEED
LFO
REPEATER
GLTCH
MIX
IMPORT / EXPORT
SESSION
```

Mantener la interfaz compacta y legible en teléfono.

El waveform no es requisito.

## Criterio de salida

Todas las funciones principales pueden utilizarse sin depender de controles provisionales o pantallas de debug.

---

# Fase 27 — Performance y optimización

## Objetivo

Reducir consumo sin cambiar comportamiento musical.

## Revisar

- allocations dentro del audio callback;
- locks;
- cache behavior;
- buffers;
- interpolación;
- mezcla;
- JNI;
- UI updates;
- almacenamiento;
- CPU por pista;
- consumo energético.

## Pruebas

Ejecutar sesiones con:

```text
1 track
4 tracks
8 tracks
16 tracks
```

o hasta donde el hardware real permita.

## Criterio de salida

LUPE mantiene audio estable en los dispositivos objetivo con un número práctico de pistas.

---

# Fase 28 — Robustez y recuperación

## Objetivo

Evitar pérdida de trabajo.

## Añadir

- autosave;
- recuperación de sesión;
- validación de archivos;
- manejo de archivos faltantes;
- prevención de corrupción;
- comportamiento ante cierre inesperado.

## Criterio de salida

Un cierre inesperado no destruye innecesariamente una sesión reciente.

---

# Fase 29 — Pruebas musicales

## Objetivo

Probar LUPE como instrumento, no sólo como software.

## Casos

Grabar y manipular:

- guitarra;
- bajo;
- voz;
- percusión;
- sintetizador;
- loops importados;
- material con reverb/delay externo;
- material sostenido;
- material transitorio.

Probar especialmente:

```text
reverse
TAIL XFADE
scrub
offset
resync
global varispeed
local varispeed
LFO
repeater
glitch
```

## Criterio de salida

Las herramientas se sienten previsibles, musicales y rápidas de usar.

---

# Fase 30 — Preparación de v1

## Objetivo

Congelar una primera versión pública coherente.

## Revisión final

- comportamiento de audio;
- estabilidad;
- uso de CPU;
- latencia;
- UX;
- permisos Android;
- gestión de archivos;
- sesiones;
- import/export;
- documentación;
- mensajes de error;
- compatibilidad con USB audio.

## Alcance esperado de LUPE v1

```text
Multipista
Tracks de longitud independiente
Master transport
Record / Play / Stop
Count-in
Seam fade
TAIL XFADE
Reverse
RESYNC
Scrub
Offset
Global varispeed
Local varispeed
LFO
Volume / Mute / Solo
Repeater
FWD / REV / ALT / RND
GLTCH
Import
Export tracks
Export master
Save / Load session
USB audio
Latency compensation
```

---

# Orden resumido de dependencias

```text
Architecture
   ↓
Duplex Audio
   ↓
Master Transport
   ↓
Single Track Loop
   ↓
Seam Fade
   ↓
TAIL XFADE
   ↓
Expected Phase / Read Head
   ↓
Reverse
   ↓
RESYNC
   ↓
Scrub / Offset
   ↓
Global Varispeed
   ↓
Local Varispeed
   ↓
LFO
   ↓
Multitrack
   ↓
Mixer
   ↓
Repeater
   ↓
Repeater Direction
   ↓
GLTCH
   ↓
Import / Export
   ↓
Save / Load
   ↓
Memory / Latency / Device Handling
   ↓
Final UI
   ↓
Optimization
   ↓
Musical Testing
   ↓
LUPE v1
```

---

# Primer milestone inmediato

## LUPE 0.0.1 — Single Track Core

La primera versión útil para desarrollo debe conseguir únicamente:

```text
Android audio duplex
+
master transport
+
BPM
+
time signature
+
loop length
+
record
+
automatic loop playback
+
sample-accurate boundaries
```

El objetivo de esta etapa es comprobar que el núcleo temporal y de audio funciona correctamente antes de construir el resto del producto.

Una vez cerrado este milestone, la siguiente incorporación será:

```text
Seam Fade
↓
TAIL XFADE
```

y después comenzará la capa de manipulación de read-heads.
