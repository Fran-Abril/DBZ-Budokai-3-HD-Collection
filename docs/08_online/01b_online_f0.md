# F0 online P2P: verificación estática (DBZ Budokai 3 HD)

Solo lectura. No se ha modificado ningún archivo del proyecto ni ejecutado el juego.
Rutas: relativas a la raíz del proyecto.
`generated\` = código recompilado del guest (`dbz3_recomp.N.cpp`, función `DEFINE_REX_FUNC(sub_XXXX)`).
Las líneas de `generated\` son aproximadas cuando se indica "≈".

## 0. Correcciones al estudio 01 (importante)

1. **El juego NO tiene netplay propio.** El estudio 01 (§3.3) decía "el juego ya hace netplay por sus sockets". Es un error de lectura: esa frase venía de la referencia (Burst Limit, otro juego). En nuestro juego:
   - El guest importa solo `NetDll_WSAStartup` y `NetDll_WSACleanup` (3 sitios cada uno, `generated\*`). No importa `NetDll_socket`, `sendto`, `recvfrom` ni ninguna `XSession*` (búsqueda por `__imp__` en `generated\`).
   - Sin `socket`, el guest no puede abrir red. No hay código de red en `src\` (búsqueda de `socket|WSAStartup|sendto` vacía).
   - En el SDK, `XSessionSearchEx` (`rexglue-sdk-0.10\src\kernel\xam\apps\xgi_app.cpp:213-221`) solo escribe un log y devuelve éxito: es un stub.
   - Conclusión: hay que escribir todo el transporte (UDP) en el host, fuera del guest. El SDK sí tiene sockets XAM (`rexglue-sdk-0.10\src\kernel\xam\xam_net.cpp:581` NetDll_socket, `:909` sendto), pero el juego no los usa, así que no sirven para nada aquí.
2. **La semilla RNG no está "por localizar"**: ya está localizada (ver §3).
3. Lo de "fijar la semilla al empezar un duelo" sigue siendo viable, pero hay que hacerlo en el punto del duelo, no al arrancar (ver §3.3).

## 1. Entrada: dónde se lee el mando de cada jugador

### 1.1 Cadena comprobada (de guest a dispositivos)

| Paso | Dónde | Evidencia |
|---|---|---|
| Bucle principal del juego | `generated\dbz3_recomp.14.cpp` `sub_82165B48` (líneas 12915-13191). Lo llama `xstart` en `generated\dbz3_recomp.20.cpp:23163` | comprobado |
| Sondeo de los 4 mandos por iteración | `sub_820902F8` (`generated\dbz3_recomp.20.cpp:666-700`): bucle de usuario 0..3; llama a `sub_8208FC40(user)` y, si devuelve no-cero, a `sub_8208FD58(user)` | comprobado |
| Sondeo de un usuario | `sub_8208FC40` (`generated\dbz3_recomp.6.cpp:688`, llamada al thunk en `:746`) y `sub_8208FD58` (`generated\dbz3_recomp.1.cpp:662`, thunk en `:697`). Buffer de estado por usuario en `0x8238B680 + 16*user` | comprobado |
| Thunk guest → XAM | `sub_8221C178` (`generated\dbz3_recomp.18.cpp:22668-22676`): `XamInputGetState(user, flags=1 GAMEPAD, state)` (`__imp__XamInputGetState` en `:22675`) | comprobado |
| Export XAM | `rexglue-sdk-0.10\src\kernel\xam\xam_input.cpp:95-116`: `XamInputGetState_entry`. Usuario 0xFF o ANY_USER se fuerza a 0 (`:109-112`). Llama a `is->GetState(actual_user_index, ...)` (`:115`) | comprobado |
| Punto central del SDK | `rexglue-sdk-0.10\src\input\input_system.cpp:232` `InputSystem::GetState`. Comentario en `:342-344`: "guest-facing choke point" | comprobado |
| Fusión por dispositivos | `input_system.cpp:308` `GetStateForUI`: `DevicesForUser` (`:317`), `GetDeviceState` por dispositivo, `active_devices_.Observe` (`:330`), `MergeInto` (`:335`). Sin dispositivos: `X_ERROR_DEVICE_NOT_CONNECTED` (`:339-341`) | comprobado |
| Fusión | `rexglue-sdk-0.10\src\input\state_merge.cpp:31-44`: botones por OR, gatillos por máximo, ejes por mayor magnitud | comprobado |
| Asignación usuario→dispositivo | `rexglue-sdk-0.10\src\input\device_assignment.cpp:16-30` `SlotAssignment::OnDevicesChanged`: los dispositivos sintéticos (teclado+ratón, MnK) van al usuario 0; los mandos reales van por su orden (`ordinal`) al usuario `ordinal`. `:32` `DevicesForUser`. Configurado en `input_system.cpp:479` | comprobado |
| Modo compartido | `device_assignment.cpp:40-52` `SharedAssignment` (todo al usuario 0). No se usa: el arranque solo instala `SlotAssignment` (`input_system.cpp:479`) | comprobado |

### 1.2 Consecuencias para el MVP

- **Jugador 1 = usuario 0** (teclado y mando 1 se fusionan con OR). **Jugador 2 = usuario 1** (solo el mando 2 físico). Si no hay segundo mando, `GetState(1)` devuelve "no conectado" (`input_system.cpp:339-341`). Y el guest lo usa para saber si P2 está presente.
- **Punto de inyección recomendado:** `InputSystem::GetState` (`input_system.cpp:232`), no `GetStateForUI`. Motivo: es el único punto por el que pasa todo lo que lee el guest, y permite forzar "conectado" para el usuario 1 aunque no haya mando físico.
- Hay que decidir antes de `GetStateForUI` (`:244`) porque ahí el usuario 1 sin dispositivo devuelve error. La inyección debe ir antes de esa comprobación.
- **Aviso de hilos:** `GetState` toma `mutex_` (`:243`) y se ejecuta en el hilo del guest. No se debe bloquear esperando red mientras se tiene ese mutex, porque `RefreshDevices` y la UI usan el mismo mutex. La espera de input remoto debe hacerse fuera del mutex.
- **Bloqueos de UI** (`input_system.cpp:236-241`): si hay un diálogo del SDK, el guest recibe ceros. Es local y no afecta a la sincronía, porque los ceros se envían al otro PC igual que cualquier input.
- **Combo de toggle y botones consumidos** (`:246-263`): se aplican después de `GetStateForUI`. Hay que preservarlos.
- **Packet number:** el guest usa el `packet_number` de XInput para detectar cambios. Hay que incrementarlo por tick en la inyección (no comprobado qué hace el guest con él).

### 1.3 Llamadas por iteración

- Una iteración del bucle llama una vez a `sub_820902F8` (`generated\dbz3_recomp.14.cpp:13077`, dentro del cuerpo del bucle en `loc_82165C28` ≈ línea 13051).
- Por usuario, la llamada a XAM puede ser **1 o 2 veces por iteración** (`sub_8208FC40` siempre; `sub_8208FD58` solo si `sub_8208FC40` devuelve no-cero). Por eso no sirve contar llamadas a `GetState` como reloj.

### 1.4 Mecanismo `dbz3_input.req` (ya existente, base para inyección)

- Archivo: `rexglue-sdk-0.10\src\input\mnk\mnk_input_driver.cpp`.
- Cvar `mnk_background_input` (`:40`), por defecto `false`. El launcher no lo activa (búsqueda `mnk_` en `src\`: solo aparecen `dbz3_mnk_*`).
- `GetDeviceState` (`:345`): si `mnk_background_input` está activo, llama a `PumpInjectedKeys(key_down_)` (`:366-367`).
- `PumpInjectedKeys` (`:156-226`): lee `dbz3_input.req` en el directorio de trabajo cada 100 ms (`:174-175`), una línea por pulsación. Cada tecla se mantiene 150 ms (`:170`, por defecto `:194`), o el tiempo indicado tras el nombre, limitado a 50-20000 ms (`:197`). Admite combinaciones con `+` y `set nombre=valor` para cambiar cvars (`:185-192`). Al terminar borra el archivo (`:215`).
- Qué hace: **pulsa teclas virtuales en el teclado local** (`key_down_`). Es entrada del dispositivo MnK, que va al usuario 0 (sintético). No escribe en memoria del guest.
- Usa `steady_clock` (tiempo real, `:157`). Es para pruebas automatizadas, no determinista.
- Para red: no sirve tal cual. Lo aprovechable es el patrón (cola de entradas con marca de tiempo y consumo por `GetDeviceState`). La inyección debe ir en `GetState` por usuario, no como tecla.

## 2. Reloj lógico

### 2.1 Comprobado

- **Reloj del host (SDK):** tiempo real del sistema. `clock.cpp` (`rexglue-sdk-0.10\src\core\clock.cpp`) escala el tiempo del guest con `guest_time_scalar_ = 1.0` (cabecera del archivo). `KeQuerySystemTime` (`rexglue-sdk-0.10\src\kernel\xboxkrnl\xboxkrnl_threading.cpp:397-401`) y `KeQueryPerformanceFrequency` (`:371-374`) devuelven ese tiempo real.
- **Vblank del guest:** worker `GPU VSync` (`rexglue-sdk-0.10\src\graphics\graphics_system.cpp:165-203`). Calcula el intervalo desde el refresco de modo de vídeo (`:171-174`) y lo fuerza a como mínimo 60 Hz (`:184-186`). Dispara `MarkVblank` (`:188`) según **reloj de pared**, no según renderizado. `MarkVblank` (`:362-375`) incrementa el contador de la CP y llama a `DispatchInterruptCallback(0, 2)` (`:374`).
- **Callback de vblank del guest:** registrado con `VdSetGraphicsInterruptCallback` (`generated\dbz3_recomp.38.cpp:24844`, callback `0x82230E48` = `sub_82230E48` en `generated\dbz3_recomp.34.cpp`). Con `r3==1` (vblank) llama a un puntero registrado (`[r4+11028]+20`) y toca flags con spinlock. No espera nada (≈ líneas de `sub_82230E48`).
- **Notas del proyecto** (`docs\HOJA_DE_RUTA_COMUNIDAD.md:98-105`): con `vsync` OFF el vblank iba a ~1000 Hz y "la lógica corre ~16x" (juego acelerado). Por eso se forzó vsync=true ("Game speed: fixed 60 FPS"). Esto indica que **la velocidad del juego depende de la frecuencia de vblank**, o sea de reloj de pared.
- **Notas del proyecto** (`docs\ANALISIS_ESCALADO_RENDIMIENTO_2026-09-14.md:66-68`): el reporte #1 mencionaba "ralentizaciones fuera de Uncapped". No es prueba concluyente.

### 2.2 No comprobado (pendiente)

- No he localizado dónde espera el bucle principal (`sub_82165B48`): la llamada indirecta por vtable en `r30+40` (≈ línea 13120) lleva al estado actual del juego, y no la he trazado. Hay 6 funciones con `KeWaitForSingleObject` (`sub_822441B0`, `sub_82223E40`, `sub_8221E2D8`, `sub_8221E1F8`, `sub_8224CD88`, `sub_82222F88`) y `sub_8224CD88`/`sub_8221E1F8`/`sub_8221E2D8` tienen un único llamador cada una. No sé cuál está en la cadena del bucle.
- **Lectores de tiempo del guest** (7 sitios): `KeQuerySystemTime` en `generated\dbz3_recomp.21.cpp:23443` (`sub_82227940`), `.29.cpp:23338` (`sub_82229230`), `.32.cpp:23083` (`sub_82227E58`), `.39.cpp:23217` (`sub_82227CE8`); `KeQueryPerformanceFrequency` en `.13.cpp:25749` (`sub_8224DE88`), `.36.cpp:24383` (`sub_822385E0`), `.6.cpp:22080` (`sub_82221A98`). No sé si alguno alimenta la lógica de combate (un delta de tiempo en gameplay sería un desync). Hay que revisar cada uno.

### 2.3 Respuesta a "¿al bajar FPS va más lento o salta?"

- **Inferido, no comprobado:** el reloj que avanza al juego parece ser el vblank de pared (60 Hz). Si el hilo del guest no puede seguir, el juego pierde pasos o se frena según cómo espere el bucle. La prueba de los 16x sugiere que la velocidad sigue al vblank.
- **Lo que sí implica para el MVP:** no podemos asumir "un frame lógico = un frame renderizado". Hay que usar un contador lógico propio (ver §5).

## 3. Aleatoriedad (RNG del guest)

### 3.1 Comprobado

- **Generador:** LFSR de 32 bits en memoria del guest. Estado en `0x82310670` (base `lis -32207` = `0x82310000` + offset 1648). Paso en `sub_82084A78` (`generated\dbz3_recomp.6.cpp:129-160`, escritura del estado en `:165`).
  - Por cada paso: realimentación = bit 31 XOR bit 4 del estado; el estado se desplaza a la izquierda y entra la realimentación por el bit 0.
  - `sub_82084A78(n)`: si `n >= 0` avanza `n+1` pasos; si `n < 0` no avanza. Devuelve estado-1.
- **Setter/get:** `sub_82084A60(x)` (`generated\dbz3_recomp.13.cpp:153-165`): devuelve el estado anterior y guarda `x`.
- **Llamadas al setter:** solo en `sub_821D34A8` (`generated\dbz3_recomp.15.cpp:18548` y `:18662`). Parece un bloque que guarda el estado, lo usa para cálculo (16 bytes por bloque, rutina tipo cifrado/firma) y lo restaura al final. No es una semilla de arranque ni depende del tiempo.
- **Escrituras directas a 0x82310670:** solo `sub_82084A78` (`dbz3_recomp.6.cpp:165`) y `sub_82084A60` (`dbz3_recomp.13.cpp:163`). Las demás escrituras con `+1648` usan otras bases (r1, r28, r31) y son otras estructuras.
- **Uso:** 229 llamadas a `sub_82084A78` en `generated\` (grep `sub_82084A78(ctx`). Una de ellas está en el cuerpo del bucle principal (`generated\dbz3_recomp.14.cpp:13097`, dentro de `sub_82165B48`). Es un RNG global compartido.
- **Selección "?"** del select: `src\roster_trace.cpp:219-221` (comentario de `sub_8217ADE8`: bitmap de personajes disponibles → LFSR → slot). Confirmado también en `docs\01_estructura\HISTORICO_AGENTS.md:3806-3809`.
- **Búsqueda en docs y src:** no hay semilla de tiempo en el código del juego (ni `srand`, ni `rand` ni `time` en `src\`). En el SDK no hay rand del guest; el generador es el LFSR del juego.

### 3.2 Inferido

- Como el estado nunca se reescribe desde el código del juego (solo lo guarda y restaura el bloque de cifrado), **el estado al arrancar es el valor inicial de los datos del XEX** (`.data`). Si el binario es igual en ambos PCs, todas las partidas parten del mismo estado.
- Pero **el menú previo también consume RNG** (selección "?", cualquier otra aleatoriedad de menú). Si los dos PCs navegan distinto antes del duelo, el número de llamadas difiere y el estado diverge. Por eso hay que fijar la semilla al empezar el duelo.

### 3.3 Cómo fijar la semilla al empezar un duelo

- Llamar a `sub_82084A60(semilla)` en el mismo punto lógico del duelo en ambos PCs. La semilla la decide el host en el handshake (un `u32`).
- Hay que localizar la función de inicio de combate (no hecho en esta fase). Patrón: `REX_HOOK_RAW` como en `src\roster_trace.cpp:223` (hook a `sub_8217ADE8`).
- Hook de escritura: basta con escribir `0x82310670`, pero llamar a `sub_82084A60` es más limpio.
- **Duda abierta:** si el menú de selección también usa el RNG antes del duelo, la semilla debe fijarse tras la selección de personajes, no al entrar en el menú. Esto requiere ver el flujo de estados.

### 3.4 Riesgos de determinismo que quedan

- Lectores de tiempo del guest (§2.2): si alimentan lógica de combate, hay desync seguro.
- Llamadas al RNG desde rutas que dependen de FPS (efectos, partículas, cámara): cuentas distintas = desync. No comprobado.
- Punto flotante: misma build = mismo resultado en x86-64 con el mismo compilador. Riesgo bajo; comprobar con el hash de desync.

## 4. Desync check

- No hay código de hash en `src\`. La referencia (Burst Limit) compara posición, acción y ki de ambos luchadores por frame (`docs\...` estudio 01, §3.5). Eso exige conocer direcciones de RAM del guest. Pendiente: ya hay mapas de RAM en `docs\03_formatos\` (no revisados en esta fase).

## 5. Lista de tareas para MVP IP directa (UDP, puerto fijo), solo Duelo

| # | Qué | Dónde | Estimación (inferida) |
|---|---|---|---|
| 1 | Transporte UDP en host (Winsock), handshake con hash de build y semilla, puerto fijo, redundancia de N últimos ticks, ACK | nuevo módulo en `src\` (no en el guest) | 4-6 días |
| 2 | Contador lógico de ticks: hook de entrada a `sub_820902F8` (una vez por iteración del bucle, `dbz3_recomp.14.cpp:13077`), no `GetState` (1-2 llamadas por iteración) | `src\` con `REX_HOOK_RAW` (patrón de `src\roster_trace.cpp`) | 2-3 días |
| 3 | Inyección por usuario en `InputSystem::GetState` (`input_system.cpp:232`): usuario 0 = local con retardo D; usuario 1 = remoto; caché por tick para llamadas repetidas; forzar "conectado" en usuario 1 | **SDK**: `rexglue-sdk-0.10\src\input\input_system.cpp`. Revisar cómo se aplican los parches del SDK (existe `github\patches\rexglue-sdk\`) | 3-5 días |
| 4 | Espera (hold) si falta input remoto, fuera del mutex, con timeout y contador de esperas en el log | mismo módulo que 3 | 2-3 días |
| 5 | Semilla al inicio del duelo: localizar función de inicio de combate y hook que llame a `sub_82084A60` | `src\` con hook | 2-4 días (depende de localizar el punto) |
| 6 | Hash de desync por tick (posición y acción de ambos luchadores; direcciones de RAM) + aviso visible | `src\` + mapas de RAM | 2-3 días |
| 7 | Modo "Online (IP)" en el launcher: campos IP y puerto, cvars con el patrón de `src\launcher\settings.cpp` | `src\launcher\` | 2-3 días |
| 8 | Documentación de reenvío de puerto y regla de firewall UDP | `docs\` | 0.5 días |
| 9 | Pruebas: dos PCs o una máquina con dos instancias y latencia simulada; comprobar que las dos pantallas coinciden (hash) | manual | 3-5 días |

Total inferido: **4-6 semanas** de trabajo para una persona, más si §2.2 o §3.3 salen mal. El estimado 2-3 semanas del estudio 01 se queda corto si el reloj del combate depende de tiempo real.

## 6. Pendiente de verificar (antes de F1)

1. Dónde espera el bucle principal (qué función bloquea por vblank o por GPU). Trazar la vtable en `r30+40` de `sub_82165B48`.
2. Si los 7 lectores de tiempo del guest (§2.2) alimentan lógica de combate.
3. Valor inicial de `0x82310670` en el XEX (no necesario si la semilla se fija en el duelo, pero sirve para el log).
4. Flujo de menús: en qué punto exacto se fija la semilla (antes o después de la selección de personajes).
5. Si el desync check se puede hacer con las direcciones de RAM ya documentadas.
6. Que el input remoto basta para los menús: si los dos PCs reciben el mismo input en los mismos ticks, las selecciones de menú (incluida la "?") coinciden. Hay que confirmar que no hay otra fuente de entrada (por ejemplo, un temporizador de UI que avance con tiempo real).

## 7. Resumen de evidencias (archivo:línea)

- Entrada: `rexglue-sdk-0.10\src\input\input_system.cpp:232, 244, 308, 317, 335, 339-341, 342-344`; `rexglue-sdk-0.10\src\kernel\xam\xam_input.cpp:95-116`; `rexglue-sdk-0.10\src\input\device_assignment.cpp:16-52`; `rexglue-sdk-0.10\src\input\state_merge.cpp:31-44`.
- Guest: `generated\dbz3_recomp.14.cpp:12915, 13077`; `generated\dbz3_recomp.20.cpp:666-700, 23163`; `generated\dbz3_recomp.18.cpp:22668-22676`; `generated\dbz3_recomp.1.cpp:662-697`; `generated\dbz3_recomp.6.cpp:688-746`.
- `dbz3_input.req`: `rexglue-sdk-0.10\src\input\mnk\mnk_input_driver.cpp:40, 156-226, 345-367`.
- Vblank: `rexglue-sdk-0.10\src\graphics\graphics_system.cpp:165-203, 362-375`; `rexglue-sdk-0.10\src\kernel\xboxkrnl\xboxkrnl_video.cpp:315-325`; `generated\dbz3_recomp.38.cpp:24844`.
- RNG: `generated\dbz3_recomp.6.cpp:129-165`; `generated\dbz3_recomp.13.cpp:153-165`; `generated\dbz3_recomp.15.cpp:18548, 18662`; `src\roster_trace.cpp:219-223`; `docs\01_estructura\HISTORICO_AGENTS.md:3806-3809`.
- Red: `generated\` (import de NetDll solo WSAStartup/WSACleanup); `rexglue-sdk-0.10\src\kernel\xam\apps\xgi_app.cpp:213-221` (stub).
- Velocidad: `docs\HOJA_DE_RUTA_COMUNIDAD.md:98-105`; `docs\ANALISIS_ESCALADO_RENDIMIENTO_2026-09-14.md:66-68`.
