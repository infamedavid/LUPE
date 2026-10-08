# LUPE — Notas de diseño

**Nombre provisional:** LUPE  
**Plataforma inicial:** Android  
**Estado:** Diseño preliminar  
**Fecha:** 2026-10-04

LUPE es un **looper multipista para Android** basado en pistas de audio independientes que comparten una referencia musical común. Cada pista puede grabar o cargar audio, tener su propia longitud y reproducirse con un comportamiento distinto al de las demás.

El núcleo de LUPE es la manipulación del playback. Reverse, varispeed global y por pista, modulación de velocidad, scrub, offset, resync y repeater/glitch actúan sobre la posición, dirección y velocidad de lectura de cada loop.

La señal puede llegar desde el micrófono, una interfaz, otra aplicación o hardware externo, limpia o previamente procesada. Una vez dentro de LUPE, cada grabación se convierte en material que puede repetirse, desplazarse, invertirse, acelerarse, desacelerarse y reorganizarse rítmicamente durante la interpretación.

La sesión comienza con una pista y puede crecer añadiendo tantas pistas como permitan los recursos del dispositivo, manteniendo para cada una su propia longitud y estado de reproducción.

---

## 1. Inicio de una sesión

Al iniciar una sesión existe inicialmente **un solo track**.

Ese track puede:

1. grabar una señal de audio;
2. cargar un archivo de audio pregrabado.

Antes de grabar se establece:

- BPM;
- compás / time signature;
- duración del track en compases;
- opcionalmente un count-in.

Ejemplo:

```text
BPM: 120
Compás: 4/4
Duración: 4 compases
```

La duración del loop se conoce de antemano.

Al terminar la grabación, el track pasa automáticamente a reproducción en loop.

---

## 2. Pistas de longitud independiente

Cada nuevo track puede tener una longitud diferente.

Ejemplo:

```text
Track 1 = 4 compases
Track 2 = 1 compás
Track 3 = 3 compases
Track 4 = 8 compases
```

No existe la obligación de que todos los tracks tengan la misma longitud.

Esto permite relaciones polimétricas de manera natural.

Ejemplo:

```text
Track A = 4 compases
Track B = 3 compases
```

Ambos vuelven a coincidir completamente cada 12 compases.

---

## 3. Transporte maestro

El primer track **no es el master**.

El primer track simplemente inicializa el transporte y la referencia musical de la sesión.

El sistema mantiene un transporte maestro independiente, sin necesidad de reproducir ningún archivo de audio por debajo.

Conceptualmente:

```text
MASTER TRANSPORT
0 bar
1 bar
2 bar
3 bar
4 bar
5 bar
...
```

Este transporte es únicamente un contador / referencia matemática.

---

## 4. Tiempo de audio y posición musical

Conviene separar dos conceptos:

### 5.1 Audio frame counter

Un contador absoluto de frames de audio:

```text
0
1
2
3
...
```

Puede representarse mediante un entero de 64 bits.

Ejemplo conceptual:

```cpp
uint64_t masterFrame;
```

A 48 kHz, cada segundo representa 48 000 frames.

No es necesario crear un timer que ejecute 48 000 veces por segundo. El callback de audio ya procesa bloques de frames, por lo que el contador puede avanzar por bloques.

Ejemplo:

```text
callback recibe 192 frames
masterFrame += 192
```

### 5.2 Musical transport

Además del contador físico de audio, existe una posición musical que avanza según:

- BPM;
- compás;
- transporte.

Esto permite cambiar BPM sin reinterpretar incorrectamente todo el tiempo transcurrido previamente.

---

## 5. Fase esperada de cada pista

Cada track tiene una **posición esperada** respecto al transporte maestro.

Para un track sincronizado:

```text
expectedPosition =
masterMusicalPosition modulo trackLength
```

Cada track puede tener una longitud distinta, pero todos derivan su posición correcta del mismo transporte maestro.

---

## 6. Posición esperada y posición real

Cada track debe separar:

```text
EXPECTED POSITION
```

de:

```text
ACTUAL READ POSITION
```

La primera representa dónde **debería** estar el track si estuviera perfectamente sincronizado.

La segunda representa dónde está realmente el read-head después de manipulaciones como:

- scrub;
- offset;
- varispeed local;
- reverse;
- otras operaciones de playback.

Ejemplo:

```text
Expected position: 63%
Actual read head:  18%
```

El transporte maestro no obliga continuamente a corregir la pista. Simplemente mantiene una referencia de dónde debería estar.

---

## 7. RESYNC

Cada track puede tener un botón `RESYNC`.

Su función es devolver el read-head a la posición musical que le corresponde según el transporte maestro.

No se sincroniza contra otro track. Se sincroniza contra la referencia maestra de sesión.

Conceptualmente:

```text
RESYNC
↓
actualPosition = expectedPosition
```

Se recomienda realizar un crossfade muy corto para evitar clicks al saltar entre posiciones distintas del waveform.

---

## 8. Reverse

Cada track puede reproducirse:

```text
FWD
REV
```

Reverse debe mantener la fase musical.

Si el track ha consumido el 25% de su ciclo en forward y se cambia a reverse, debe posicionarse en el punto equivalente del recorrido inverso.

Ejemplo:

```text
Musical phase = 25%

FWD physical position = 25%
REV physical position = 75%
```

Regla conceptual:

```text
FWD:
position = phase

REV:
position = 1 - phase
```

Por lo tanto cambiar `FWD ↔ REV` no debe cambiar la fase musical del track.

---

## 9. Reverse desde el inicio

Si un track tiene `REV` activado antes de grabar:

1. se realiza la grabación normalmente;
2. al terminar la ventana de grabación;
3. el track entra inmediatamente en reproducción reverse;
4. comienza desde la posición correcta para mantenerse sincronizado.

---

## 10. RESYNC en reverse

`RESYNC` no cambia la dirección de reproducción.

Si el track está en reverse, después de resincronizar debe continuar en reverse.

Sólo cambia la posición física del read-head.

Ejemplo:

```text
expectedPhase = 0.25

FWD:
readHead = 0.25

REV:
readHead = 0.75
```

Principio:

> La dirección pertenece al playback. La sincronización pertenece a la fase. Son propiedades independientes.

---

## 11. Superficie de posición / scrub

Cada track puede tener un rectángulo táctil para navegar por el loop.

No es obligatorio mostrar waveform.

La posición horizontal del dedo representa la posición dentro del audio.

Se discutieron dos acciones:

### Tap

Salta inmediatamente a esa posición.

### Drag

Realiza scrub continuo.

El rectángulo puede representar posiciones como inicio, mitad, final, 1/4, 1/8, etc.

La ausencia de waveform refuerza la idea de que el programa es un instrumento / looper y no un editor de audio.

---

## 12. Offset de fase

Cada track puede tener un desplazamiento permanente de fase.

Ejemplo:

```text
OFFSET +25%
```

Conceptualmente:

```text
trackPhase =
masterPhase + offset
```

con wrap-around.

Diferencia importante:

```text
Scrub  = cambiar la posición actual.
Offset = mantener una diferencia de fase respecto al master.
```

---

## 13. Grabación y cierre del loop

La duración musical del track se define antes de grabar.

Ejemplo:

```text
4 compases
```

Cuando se alcanza el final nominal de esa duración, LUPE puede continuar capturando una cola adicional destinada al cruce con la siguiente vuelta. Esa cola no aumenta la duración del loop ni desplaza su siguiente inicio.

### Seam fade

Siempre puede existir un microfade técnico alrededor del punto de unión para evitar clicks causados por una discontinuidad entre el último y el primer sample.

Es una medida de continuidad de audio y debe mantenerse prácticamente transparente para el usuario.

### TAIL XFADE

`TAIL XFADE` es un crossfade musical entre la cola grabada después del final nominal y el comienzo de la siguiente vuelta.

Valor inicial:

```text
TAIL XFADE = 4 beats
```

El valor es configurable por track y puede ser:

```text
OFF
menos de 4 beats
4 beats
más de 4 beats
```

La selección se expresa en beats, no como una fracción obligatoria de la duración del loop.

Ejemplo con un loop de 4 compases en 4/4:

```text
|------------- LOOP: 16 beats -------------|---- TAIL: 4 beats ----|
                                            ^
                                      final nominal
```

Durante la siguiente vuelta:

```text
TAIL anterior   100% ─────────► 0%
HEAD siguiente    0% ─────────► 100%
```

El ciclo sigue midiendo exactamente 16 beats. Los 4 beats adicionales existen únicamente como material de cruce.

Esto permite que notas sostenidas, resonancias, voces, instrumentos acústicos o señal procesada externamente atraviesen el límite del loop sin ser cortados bruscamente.

`TAIL XFADE = OFF` desactiva esta cola musical, pero no obliga a eliminar el pequeño seam fade técnico utilizado para evitar clicks.

Cada pista puede utilizar un valor diferente.

Ejemplo:

```text
Track 1   TAIL XFADE  4 beats
Track 2   TAIL XFADE  OFF
Track 3   TAIL XFADE  1 beat
Track 4   TAIL XFADE  8 beats
```


## 14. Count-in

La grabación puede tener count-in opcional.

Posibles formas:

- click audible;
- indicador visual;
- vibración.

La vibración puede ser especialmente útil cuando se graba con el micrófono del teléfono, evitando que el click quede grabado.

---

## 15. Multipista en lugar de overdub destructivo

En lugar de mezclar sucesivos overdubs dentro del mismo archivo, cada nueva toma puede crear una pista independiente.

Ejemplo:

```text
Track 1 = guitarra
Track 2 = bajo
Track 3 = percusión
Track 4 = voz
```

Ventajas:

- mute independiente;
- volumen independiente;
- exportación por pista;
- borrado individual;
- regrabación individual;
- manipulación independiente de playback.

Más adelante podría existir bounce/merge, pero no es necesario para el concepto inicial.

---

## 16. Volumen

Cada track debe tener control de volumen.

---

## 17. Pan

Pan es técnicamente sencillo pero no se considera esencial todavía.

Puede quedar como opción futura o secundaria.

---

## 18. Importación

Un track puede usar:

```text
RECORD
```

o:

```text
IMPORT
```

Se puede cargar audio pregrabado, por ejemplo drums, loops, texturas o stems.

Inicialmente no es necesario implementar time-stretch automático.

---

## 19. Exportación

Debe contemplarse:

- exportar cada track individualmente;
- exportar mezcla master;
- guardar/cargar sesión.

---

# Motor de playback

## 20. Varispeed global

La sesión puede tener control global de varispeed.

Al cambiar velocidad:

```text
tempo cambia
pitch cambia
```

Es comportamiento de cinta.

Relación:

```text
playbackRate =
newBpm / originalBpm
```

Cambio de pitch:

```text
semitones =
12 × log2(newBpm / originalBpm)
```

Ejemplo:

```text
120 BPM → 60 BPM
= 0.5x
= -12 semitonos
```

No se necesita timestretch para este comportamiento.

---

## 21. BPM de grabación por pista

Cada track puede guardar:

```text
recordedBpm
```

Si el BPM global cambia:

```text
trackPlaybackRate =
currentSessionBpm / track.recordedBpm
```

Esto permite grabar nuevos tracks cuando la sesión ya está funcionando a otra velocidad.

---

## 22. Varispeed local por pista

Además del varispeed global, cada track puede tener su propio varispeed.

Ejemplos:

```text
0.25x
0.5x
1x
2x
4x
```

El varispeed local puede deliberadamente romper la relación normal de duración del track y generar relaciones polirrítmicas o asincrónicas.

---

## 23. Varispeed continuo

La velocidad puede representarse de forma continua.

Ejemplo:

```text
0.73x
```

El read-head debe poder avanzar por posiciones fraccionales, por lo que será necesario interpolar entre samples.

Posible primera implementación:

- interpolación cúbica;
- Hermite;
- otra interpolación eficiente.

No es necesario comenzar con un resampler sinc complejo si no aporta una mejora audible clara.

---

## 24. Velocidad negativa

El sistema puede modelar reverse mediante velocidad negativa.

Ejemplo:

```text
+1.0x = forward
+0.5x = forward half speed
 0.0x = detenido
-0.5x = reverse half speed
-1.0x = reverse normal
-2.0x = reverse double speed
```

Esto unifica reverse y varispeed dentro del mismo modelo de read-head.

---

## 25. LFO sobre varispeed local

Cada track puede tener un LFO modulando su velocidad.

Objetivo:

- wow;
- flutter;
- cassette wobble;
- aceleración y desaceleración continua;
- scrub automático;
- efectos de cinta.

Ejemplo:

```text
CENTER = 1.0x
DEPTH  = ±3 semitonos
RATE   = 0.4 Hz
```

---

## 26. LFO en semitonos

Musicalmente puede ser más útil expresar profundidad en semitonos.

Relación:

```text
rate =
2^(semitones / 12)
```

Esto permite una modulación simétrica desde el punto de vista musical.

---

## 27. Rate del LFO

Posibles modos:

### Hz

Útil para wow/flutter.

### Sync

Ejemplos:

```text
1 BAR
1/2
1/4
1/8
```

Útil para modulaciones musicales sincronizadas.

---

## 28. LFO atravesando cero

Una posibilidad interesante es permitir que la modulación de velocidad atraviese `0x`.

Ejemplo:

```text
+1.0
+0.5
0.0
-0.5
-1.0
-0.5
0.0
+0.5
+1.0
```

Esto produce avance, desaceleración, parada y reproducción inversa continua.

Todavía debe evaluarse musical y técnicamente.

---

# Repeater / Glitch

## 29. Referencia: Protoseq Rack BEACON

El looper puede reutilizar conceptualmente el comportamiento del repeater desarrollado previamente en Protoseq Rack / BEACON.

Idea general:

1. se define una ventana temporal;
2. se captura/referencia esa región;
3. esa región comienza a repetirse;
4. al terminar el repeater, el track vuelve a su posición correcta en el transporte.

En el looper no es obligatorio copiar físicamente el audio a un buffer nuevo porque el clip completo ya existe.

Puede utilizarse un read-head temporal sobre una región del clip.

---

## 30. Repeater sincronizado

El repeater no debe desincronizar el track.

Mientras el repeater produce:

```text
ABC ABC ABC ABC
```

el transporte maestro continúa avanzando.

Al soltar el repeater:

```text
REPEATER OFF
↓
volver a la posición correcta
según el transporte maestro
```

---

## 31. Tamaños del Repeater

Se puede conservar la filosofía de BEACON:

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

En el looper, `1/1` puede referirse directamente a un compás porque la aplicación conoce la métrica.

Ejemplo 4/4:

```text
1/1  = 4 beats
1/2  = 2 beats
1/4  = 1 beat
1/8  = 1/2 beat
...
```

---

## 32. Repeater EVEN / TRIP / DOT

Puede conservarse:

```text
EVEN = ×1
TRIP = ×2/3
DOT  = ×3/2
```

---

## 33. Dirección independiente del Repeater

La dirección del repeater es independiente de la dirección principal del track.

El track puede estar en `REV` mientras el repeater usa `FWD`, o viceversa.

Esto permite, por ejemplo, capturar una frase que está sonando en reverse y volver a invertirla dentro del repeater.

---

## 34. Modos de dirección del Repeater

Se discutieron cuatro modos útiles:

```text
FWD
REV
ALT
RND
```

### FWD

Cada ciclo se reproduce hacia adelante.

### REV

Cada ciclo se reproduce hacia atrás.

### ALT

Alterna la dirección:

```text
FWD
REV
FWD
REV
...
```

### RND

Cada ciclo decide aleatoriamente entre forward y reverse.

---

## 35. GLTCH

GLTCH puede reutilizar el mismo motor del repeater.

No necesita un motor separado.

Su función es generar activaciones automáticas / probabilísticas del repeater.

Conceptualmente puede decidir:

- si ocurre una repetición;
- qué SIZE usa;
- cuántos ciclos dura.

La dirección puede obedecer al modo `REP DIR`.

---

# Otras operaciones de playback

## 36. Ping-pong global

Se discutió pero **no se considera particularmente atractivo para el playback principal**.

No se descarta completamente, pero no es prioridad.

La alternancia de dirección sí tiene más sentido dentro del repeater mediante `ALT`.

---

## 37. Tape Start / Tape Stop

Podría implementarse más adelante como automatización de varispeed.

Ejemplo:

```text
1x → 0x
```

o:

```text
0x → 1x
```

No requiere un efecto DSP separado.

No es prioridad actual.

---

# Arquitectura conceptual de una pista

## 38. Flujo conceptual

```text
                 MASTER TRANSPORT
                        │
                        ▼
                  EXPECTED PHASE
                        │
          ┌─────────────┼─────────────┐
          │             │             │
        OFFSET        SCRUB         RESYNC
          │             │             │
          └─────────────┼─────────────┘
                        ▼
                 TRACK READ POSITION
                        │
                        ▼
                    DIRECTION
                    FWD / REV
                        │
                        ▼
                    VARISPEED
                  manual + LFO
                        │
                        ▼
                 MAIN READ HEAD
                        │
                        ▼
                    REPEATER
             SIZE / TIME / DIR / GLTCH
                        │
                        ▼
                      VOLUME
                        │
                        ▼
                       MIX
```

---

## 39. Principio técnico central

Reverse, offset, scrub, varispeed y repeater no deben considerarse efectos DSP independientes.

Todos pueden entenderse principalmente como:

> Manipulación de uno o varios read-heads sobre un buffer de audio.

Esto permite un diseño mucho más coherente y eficiente.

---

# Motor de audio

## 40. Android

El proyecto está pensado inicialmente para Android.

Arquitectura probable:

```text
Kotlin / Jetpack Compose
        │
        ▼
       JNI
        │
        ▼
C++ Audio Engine
        │
        ▼
   Oboe / AAudio
```

La UI, sesiones y archivos pueden manejarse en Kotlin.

El motor de audio y reproducción sample-accurate conviene implementarlos en C++.

---

## 41. Callback de audio

El callback de audio debe mantenerse simple.

No debería realizar:

- escritura de archivos;
- operaciones bloqueantes;
- asignaciones de memoria;
- mutexes innecesarios;
- tareas de UI.

Conceptualmente:

```text
INPUT
  ↓
record buffer

TRACK 1 ─┐
TRACK 2 ─┤
TRACK 3 ─┼──► MIX ───► OUTPUT
TRACK n ─┘
```

---

## 42. Mezcla

Conviene utilizar:

- un único stream de salida;
- mixer interno de tracks.

Cada track alimenta el mixer.

---

## 43. Precisión temporal

El transporte musical y los read-heads deben derivar del mismo reloj del audio.

La UI puede refrescar a 30 o 60 Hz sin afectar precisión musical.

El audio puede seguir siendo sample-accurate.

---

## 44. CPU y coste de la fase maestra

Mantener el transporte y la posición esperada de cada track tiene un coste prácticamente despreciable.

Operaciones como contador de frames, módulo por track y cálculo de fase son muy baratas.

Los costes reales estarán principalmente en:

- resampling;
- interpolación;
- múltiples read-heads;
- mezcla;
- audio I/O;
- eventualmente escritura/lectura de archivos.

La referencia maestra no requiere un archivo de audio reproduciéndose.

---

## 45. Hardware de prueba inicial

Las primeras pruebas prácticas de entrada y salida de audio utilizarán una **Behringer UCA222** conectada al dispositivo Android.

Servirá como referencia real para comprobar:

- audio USB;
- grabación y monitorización;
- estabilidad del stream;
- latencia de entrada y salida;
- compensación de grabación;
- comportamiento del motor con una interfaz externa.

## 46. Latencia

Uno de los problemas importantes será la compensación de latencia entre:

- output que escucha el músico;
- input que se está grabando.

La arquitectura debe permitir un offset de grabación.

Posible opción futura:

```text
Recording Offset
AUTO / manual
```

y eventualmente un procedimiento de calibración mediante loopback.

---

# Criterios de diseño

## 47. Mantenerlo fuera del terreno de un DAW

El proyecto debe evitar crecer hacia:

- edición destructiva compleja;
- timeline tradicional;
- mixer de estudio completo;
- plugins internos;
- cadenas de efectos;
- waveform editor detallado.

La herramienta debe sentirse más como:

> un instrumento de loops y manipulación de playback.

---

## 48. No depender del waveform

El waveform puede omitirse.

La navegación del audio puede hacerse mediante:

- rectángulo táctil;
- posición relativa;
- scrub;
- phase offset;
- referencias musicales.

Esto reduce distracción visual y mantiene la interfaz compacta.

---

## 49. Identidad de LUPE

La combinación actual empieza a definir un carácter propio:

- looper multipista;
- tracks de longitudes independientes;
- reverse sample-accurate;
- varispeed global;
- varispeed local;
- LFO de velocidad;
- scrub;
- offset;
- resync;
- repeater;
- glitch;
- repeater con dirección independiente;
- manipulación de read-heads.

Más que un looper tradicional con efectos, el proyecto puede convertirse en:

> un looper multipista tipo cinta, centrado en manipulación temporal y rítmica de read-heads sobre loops sincronizados.

---

# Estado actual de las decisiones

## 50. Acordado o fuertemente preferido

- Android como plataforma inicial.
- Multipista.
- Un solo track inicial.
- Tracks posteriores con longitudes independientes.
- BPM y métrica globales.
- Transporte maestro matemático independiente del audio.
- Posición esperada por track.
- Posición real/read-head independiente.
- RESYNC contra master, no contra otro track.
- Reverse mantiene sincronía.
- RESYNC conserva reverse.
- Volume por track.
- Sin efectos tradicionales.
- Varispeed global.
- Varispeed local.
- LFO para varispeed local.
- Reverse.
- Seam fade técnico independiente del crossfade musical.
- `TAIL XFADE` por pista, configurable en beats.
- `TAIL XFADE` de 4 beats por defecto y opción `OFF`.
- Scrub mediante superficie táctil.
- No depender de waveform.
- Offset de fase.
- Repeater.
- Repeater sincronizado.
- Repeater con dirección independiente.
- Modos de repeater FWD / REV / ALT / RND.
- GLTCH basado en repeater.
- Importación de audio.
- Exportación por track.
- Exportación master.

---

# Abierto / por definir

## 51. Temas pendientes

- Nombre del proyecto.
- Diseño exacto de UI.
- Duración exacta del seam crossfade.
- Valores definitivos del count-in.
- Si habrá pan.
- Formatos exactos de import/export.
- Qué ocurre con audio importado de BPM desconocido.
- Límites de cantidad de tracks.
- Límites de memoria.
- Rango máximo/mínimo de varispeed.
- Curvas y límites del LFO.
- Si el LFO puede cruzar 0x.
- Interpolador/resampler definitivo.
- Comportamiento exacto del scrub cerca de boundaries.
- Semántica exacta de offset vs scrub persistente.
- Si `RESYNC` elimina o conserva offset.
- Reglas definitivas del GLTCH.
- Duración de crossfade al hacer RESYNC.
- Si habrá Tape Start / Stop.
- Compensación/calibración de latencia.
- Manejo de audio estéreo vs mono.
- Política de almacenamiento de sesiones.
- Persistencia de parámetros y automatizaciones.

---

## 52. Principio de diseño a conservar

Siempre que sea posible:

> Resolver las funciones mediante posición, dirección y velocidad de read-heads, en vez de añadir DSP y efectos independientes.

Esto mantiene el motor más coherente, más barato computacionalmente y más cercano a la identidad del proyecto.
