# 12. Mapa de implementación: online P2P por IP directa (DBZ Budokai 3 HD)

Fecha: 2026-10-10. Solo lectura: no se ha editado ningún archivo del proyecto, ni compilado, ni ejecutado el juego.
Rutas relativas a la raíz del proyecto.
Etiquetas: **[V]** verificado leyendo el código · **[I]** inferido · **[P]** pendiente de comprobar.
Código de muestra en §6: esqueleto no compilado.

---

## 0. Qué cambia respecto a 01 y 01b

1. **Se puede hacer sin tocar el SDK.** El guest llama a XAM mediante thunks. Un `REX_HOOK_RAW` sobre el thunk de estado `sub_8221C178` intercepta la lectura de los usuarios 0 y 1 (mismo patrón que `src/hooks.cpp:25-66`). Es la opción recomendada frente a parchear `InputSystem::GetState`. [V]
2. **El SDK se mantiene como overlay.** `github/patches/rexglue-sdk/` es el SDK completo sobre v0.10 (`github/patches/README.md`, sección "2026-10-04 v1.4.0"). Hoy `input_system.cpp` es idéntico en SDK y overlay (comprobado con `cmp`). Cualquier cambio en el SDK hay que copiarlo también al overlay o se pierde con el siguiente "copiar overlay". [V]
3. **El tick lógico es una iteración del bucle principal**, con punto de muestreo en `sub_820902F8`, que tiene una única llamada (`generated\dbz3_recomp.14.cpp:13077`). [V]
4. **`mods\_roster\build.hash` no sirve para comparar PCs**: su digest incluye la fecha de modificación de cada archivo (§4). [V]
5. **Carpeta de mods inconsistente**: `ModsRoot()` no sube niveles, pero el runtime y el constructor sí (§4). [V]
6. **`ws2_32` sí está enlazado** (`CMakeLists.txt:115`, corregido 2026-10-11; el texto original decía lo contrario). Ver §5. [V]
7. **"P2 presente" = `XamInputGetState(1)` devuelve 0** (`generated\dbz3_recomp.6.cpp:746-773`). Basta con que el usuario remoto devuelva éxito. Corrijo mi lectura previa: `sub_8221C188` es la ruta de vibración, no de presencia (§1.1 y §2.5). [V]
8. **El guest no usa keystrokes**: no hay ninguna llamada a `XamInputGetKeystroke` en `generated\`. Solo hay que sincronizar `GetState`. [V]
9. **La semilla RNG puede no hacer falta.** Si la sesión empieza en la misma iteración en ambos PCs (el bloqueo del tick 0 lo garantiza), el LFSR llega igual sin fijar semilla. El paso de F0 §3.3 queda como refuerzo. [I]

---

## 1. Entrada: dónde se lee el mando (punto 1)

### 1.1 Cadena actual, de guest a dispositivos [V]

| Paso | Archivo:línea | Qué hace |
|---|---|---|
| Bucle principal | `generated\dbz3_recomp.14.cpp:12915` (`sub_82165B48`) | Cuerpo en `loc_82165C28` (`:13051`) |
| Sondeo de 4 usuarios | `generated\dbz3_recomp.20.cpp:666-701` (`sub_820902F8`, bucle 1) | `sub_8208FC40(u)`; si no es 0, `sub_8208FD58(u)` |
| Vibración por usuario | `generated\dbz3_recomp.20.cpp:702-727` (bucle 2) | `sub_8221C188(u, 0x8238B6C0+4u)` |
| Poll de un usuario | `generated\dbz3_recomp.6.cpp:688` (`sub_8208FC40`) | Llama al thunk en `:746` |
| Thunk de estado | `generated\dbz3_recomp.18.cpp:22668-22677` (`sub_8221C178`) | `mr r5,r4; li r4,1; __imp__XamInputGetState` |
| Thunk de vibración | `generated\dbz3_recomp.12.cpp:21984-22030` (`sub_8221C188`) | `XamInputGetCapabilities`, luego `XamInputSetState` |
| Export XAM | `rexglue-sdk-0.10\src\kernel\xam\xam_input.cpp:95-116` | `XamInputGetState_entry`: user 0xFF/ANY → 0 (`:108-112`); `is->GetState(...)` (`:115`) |
| Punto central del SDK | `rexglue-sdk-0.10\src\input\input_system.cpp:232-266` | `InputSystem::GetState` |
| Estado fusionado | `input_system.cpp:308-361` | `GetStateForUI`: `DevicesForUser` (`:317`), `GetDeviceState` por dispositivo (`:327`), `MergeInto` (`:335`), zona muerta (`:345-356`) |
| Fusión | `rexglue-sdk-0.10\src\input\state_merge.cpp:31-44` | Botones OR, gatillos máximo, sticks mayor magnitud |

Llamadas al thunk de estado: solo desde `sub_8208FC40` (`dbz3_recomp.6.cpp:746`) y `sub_8208FD58` (`dbz3_recomp.1.cpp:697`). La única llamada a `__imp__XamInputGetState` está en el thunk (`dbz3_recomp.18.cpp:22675`). [V]

Llamadas al thunk de vibración: solo desde `sub_820902F8` (`dbz3_recomp.20.cpp:717`). [V]

### 1.2 Firmas y estructuras [V]

- `X_RESULT InputSystem::GetState(uint32_t user_index, X_INPUT_STATE* out_state)`: `rexglue-sdk-0.10\include\rex\input\input_system.h:50`.
- `X_RESULT GetCapabilities(uint32_t user_index, uint32_t flags, X_INPUT_CAPABILITIES*)`: `input_system.h:49`. `GetStateForUI`: `:55`.
- `X_INPUT_GAMEPAD` (12 bytes): `rexglue-sdk-0.10\include\rex\input\input.h:61-70` → `be<u16> buttons; u8 left_trigger; u8 right_trigger; be<i16> thumb_lx, thumb_ly, thumb_rx, thumb_ry`.
- `X_INPUT_STATE` (16 bytes): `input.h:72-76` → `be<u32> packet_number` (offset 0) + gamepad (offset 4).
- Interfaz de driver: `rexglue-sdk-0.10\include\rex\input\input_driver.h:30-66`. Métodos: `EnumerateDevices(std::vector<DeviceInfo>&)` (`:38`), `GetDeviceState(DeviceId, X_INPUT_STATE*)` (`:40`), `GetDeviceCapabilities` (`:41`), `SetDeviceVibration` (`:43`), `GetDeviceKeystroke` (`:44`).
- `DeviceInfo { id, ordinal, name, guid, synthetic }`: `rexglue-sdk-0.10\include\rex\input\device.h:25-31`. `kMaxGuestUsers = 4` (`device.h:18`).

### 1.3 Cómo se marca un usuario como "conectado" [V]

- Un usuario está conectado si `DevicesForUser(u)` no está vacío y al menos un dispositivo devuelve `X_ERROR_SUCCESS` en `GetDeviceState` (`input_system.cpp:317-341`). Si no hay ninguno, devuelve `X_ERROR_DEVICE_NOT_CONNECTED` (`:339-341`).
- Asignación por ordinal: `rexglue-sdk-0.10\src\input\device_assignment.cpp:16-38`. Los dispositivos sintéticos (teclado/ratón y NOP) van siempre al usuario 0 (`:24-25`). Los mandos físicos van al usuario = ordinal (`:26-27`).
- Ordinales: `input_system.cpp:127-152` (el primer mando libre).
- Driver NOP: sintético, siempre presente (`rexglue-sdk-0.10\src\input\nop\nop_input_driver.cpp:39-45`, añadido en `input_system.cpp:477-478`). Por tanto el usuario 0 siempre está conectado, aunque no haya mando.
- Usuario 1 solo está conectado con un mando físico de ordinal 1. Esto confirma F0 §1.2.
- Backend por defecto: `input_system.cpp:30` (`input_backend` = "sdl"). El launcher lo define en `src\launcher\settings.cpp:229-233` (`dbz3_input_backend`).
- Drivers: XInput `xinput_input_driver.cpp:128-151` (`synthetic=false`, `:148`); MnK `mnk_input_driver.cpp:310-320` (sintético) y `:345-363` (sin foco devuelve ceros, `:357-362`); SDL `sdl_input_driver.cpp:166-186` y `:213-222` (con `is_active()`).

### 1.4 (a) Sustituir el estado del usuario 1 por el recibido por red

**Opción A, recomendada (sin tocar el SDK).** Hook del thunk de estado:

- `REX_HOOK_RAW(sub_8221C178)` en un archivo nuevo de `src\net\`. Entrada: `ctx.r3` = usuario, `ctx.r4` = dirección guest del `X_INPUT_STATE` de destino. Según `sub_8208FC40` (`dbz3_recomp.6.cpp:688-746`), esa dirección es `0x8238B680 + 16*usuario`. [V en cálculo; tabla deducida]
- Si el usuario es 0 o 1 y la sesión está activa: escribir el estado en guest (big-endian con `REX_STORE_U16/U8/U32`) y devolver `ctx.r3 = 0` (éxito).
- Si no: reproducir el original: `ctx.r5 = ctx.r4; ctx.r4 = 1; __imp__XamInputGetState(ctx, base);` (mismo código que `dbz3_recomp.18.cpp:22670-22675`).
- Ventaja: el hilo del guest no entra en `mutex_` de `InputSystem` mientras espera al rival. Ver §1.6.

**Opción B (toca el SDK).** En `input_system.cpp`, insertar tras el bloqueo de diálogo (`:236-241`) y antes de `GetStateForUI` (`:244`) un gancho que, si la sesión está activa y `user_index == 1`, rellene `out_state` y devuelva `X_ERROR_SUCCESS`. El problema es `GetStateForUI` (`:244`), que devuelve "no conectado" para el usuario 1 sin mando. Coste: parche del SDK más copia al overlay (§0.2).

### 1.5 (b) Retrasar el estado del usuario 0 N frames

No se hace en `GetState`: se llama 1-2 veces por iteración y no hay reloj de frame. La receta es la de §2:

1. En el tick T se muestrea el mando local **en bruto** (ver §1.6) y se guarda en `hist[T]`.
2. Se envía `hist[T]` al rival marcado para el tick `T + D` (D = retardo, 2-8).
3. Al aplicar el tick T, el usuario 0 recibe `hist[T - D]`, y el usuario 1 recibe `remote[T]`.
4. Los `D` primeros ticks usan estado neutro.

Para muestrear el mando local con la lógica del SDK (combo del menú rápido `input_system.cpp:246-263`, bloqueo de diálogo `:236-241`) basta con llamar a `InputSystem::GetState(0, &st)`. Se obtiene el objeto así: `REX_KERNEL_STATE()->emulator()->input_system()` (`xam_input.cpp:37-38`; la función de `:37` es local, no exportada).

### 1.6 Precauciones de la entrada [V]

- No bloquear dentro de `GetState`/`GetStateForUI`: el mutex es recursivo y compartido con la UI (`input_system.cpp:95`, `:243`, `:310`). Esperar al rival fuera de esas funciones.
- `packet_number` (`input.h:73`): poner el número de tick en cada estado.
- Con SDL, un mando sin foco de ventana devuelve estado neutro (`sdl_input_driver.cpp:216-220`; MnK en `mnk_input_driver.cpp:357-362`). Al perder el foco, el rival verá al jugador quieto (§8).
- `dbz3_input.req` (`mnk_input_driver.cpp:156-226`) es solo para pruebas automáticas y no sirve para la red.

---

## 2. Reloj de frames y bloqueo (punto 2)

### 2.1 Bucle principal [V]

- `sub_82165B48` (`generated\dbz3_recomp.14.cpp:12915`). Lo llama `xstart` (`dbz3_recomp.20.cpp:23163`).
- Cuerpo en `loc_82165C28` (`:13051`). Orden de llamadas relevantes:
  - `sub_820902F8` = sondeo de mandos (`:13077`). Única llamada del código generado (`grep`), por tanto **1 vez por iteración**.
  - `sub_82084A78(32)` = avanza el LFSR 33 pasos (`:13093-13097`). Cada iteración consume RNG. El número de iteraciones forma parte del estado determinista.
  - Llamada indirecta por vtable `[r30+40]` (`:13113-13122`), que lleva al estado del juego [P: no trazada].
- Vuelta del bucle: `beq loc_82165C28` (`:13134`), tras `bl sub_822BD7D8` (`:13125-13127`).
  - `sub_822BD7D8` es un stub `li r3,0; blr` (`generated\dbz3_recomp.29.cpp:30346-30352`). Por tanto, según el código generado, la condición de salida nunca se cumple (`:13135-13188` no es alcanzable por esa vía) [V en código, P en ejecución].
- Stub vacío: `sub_82308398` = `blr` (`generated\dbz3_recomp.42.cpp:36734-36737`), llamado dos veces por iteración (`:13083`, `:13100`). No espera nada.

### 2.2 Dónde se frena el juego a 60 Hz [P]

- No he localizado el punto que bloquea cada iteración. Candidatos: la llamada por vtable (`:13120`), `sub_82095628` (`:13059`), `sub_820A32D0` (`:13054`), `sub_820927E0`/`sub_821DA2B0`/`sub_820921E0` (`:13085-13092`).
- Lado SDK: el worker "GPU VSync" calcula el intervalo desde el modo de vídeo y lo fuerza a ≥ 60 Hz (`rexglue-sdk-0.10\src\graphics\graphics_system.cpp:165-203`; clamp descrito en `github\patches\README.md` sección 13). `MarkVblank` (`:362-375`) y `DispatchInterruptCallback` (`:340`).
- Swap del guest: `src\graphics\command_processor.cpp:990` (`IssueSwap`) y `src\graphics\d3d12\command_processor.cpp:2775`.
- No existe `VdSwap` en el SDK (búsqueda vacía).
- Mi opinión: no hace falta saber dónde está el freno. Para el lockstep basta con contar iteraciones y bloquear antes del sondeo.

### 2.3 Hook propuesto para el reloj y el bloqueo [V en estructura]

- `REX_HOOK_RAW(sub_820902F8)` que reimplementa el cuerpo (`dbz3_recomp.20.cpp:666-733`, mismo enfoque que `hooks.cpp:53-66`):
  1. Si la sesión está activa: `Session::BeginTick(...)` (muestrea, envía, **espera al rival**, rellena `applied_[0..1]`). Puede bloquear.
  2. Bucle 1 (`:680-701`): para `u = 0..3`, `sub_8208FC40(u)`; si el resultado es distinto de 0, `sub_8208FD58(u)`.
  3. Bucle 2 (`:710-727`): para `u = 0..3`, `sub_8221C188(u, 0x8238B6C0 + 4*u)`.
- Bloquear dentro de ese hook detiene la iteración completa, porque el cuerpo es secuencial. La ventana sigue viva porque el hilo de UI es otro.
- Riesgo [P]: F0 §2.1 indica que el callback de vblank (`sub_82230E48`) usa un spinlock del guest. Si el hilo principal lo tuviera tomado al bloquear, habría interbloqueo. No comprobado.
- Timeout propuesto: sin paquete durante 3 s, avisar "esperando al rival"; a los 10 s, cortar la sesión con mensaje.

### 2.4 Tick y semilla [V/I]

- Tick 0 = primera iteración del juego tras el handshake. El contador de tick empieza en 0 cuando el launcher llama a `Session::Start()`.
- Cada iteración avanza un tick. Así el RNG (`sub_82084A78`, 33 pasos por iteración) queda sincronizado si las dos máquinas parten del mismo estado inicial.
- Si la sesión se inicia antes de lanzar el módulo (§3.4), la semilla de F0 §3.3 (`sub_82084A60`, según F0 `dbz3_recomp.13.cpp:153-165`) es un refuerzo. Como el bloqueo del tick 0 impide que una máquina avance sola, no debería hacer falta. [I]
- Lectores de tiempo del guest (`KeQuerySystemTime`, `KeQueryPerformanceFrequency`; F0 §2.2): sin comprobar. Si alimentan lógica de juego, hay desincronía segura. [P]

### 2.5 Input que no pasa por GetState [V]

- Keystrokes: ninguna llamada a `XamInputGetKeystroke` en `generated\`. No hay que sincronizar nada más.
- Vibración: `sub_8221C188` solo actúa si `caps.subtype == 2` y los flags cumplen (`dbz3_recomp.12.cpp:21989-21997`); después llama a `XamInputSetState` (`:22030`). No afecta a la sincronía. El rival no necesita vibrar.

---

## 3. Launcher (punto 3)

### 3.1 Estructura [V]

- Clase: `LauncherDialog : rex::ui::ImGuiDialog` (`src\launcher\launcher_state.h:40-98`). Declaraciones de pestañas: `DrawVideoTab`…`DrawDevTab` (`launcher_state.h:60-71`).
- Dibujo de cada frame: `LauncherDialog::OnDraw` (`launcher_state.cpp:637`).
- Barra de pestañas: `ImGui::BeginTabBar("##launcher_tabs")` (`launcher_state.cpp:1128`). Cada pestaña usa el lambda `begin_tab(tab_label(ICON, "nombre", "id"))` seguido de `DrawXTab(); ImGui::EndTabItem();` (`:1131-1176`).
- Pestañas actuales (10): Video, Escalado, Audio, Controles (`:1143-1146`), Mods (`:1149`), Mods nativos, Cambio de modelo, Texturas, Personajes nuevos (`:1167`), Avanzado (`:1173`).
- **Trampa:** el salto con Ctrl+Tab usa `% 10` fijo (`launcher_state.cpp:646`). Al añadir una pestaña hay que cambiarlo a `% 11`.
- Ejemplo de pestaña con sección: `LauncherDialog::DrawInputTab()` (`launcher_state.cpp:2014`) usa `BeginChild`, `PushSectionHeader(...)`, combos y `ImGui::InputText` (`:171` en el helper de keybinds).
- Kit visual (`src\launcher\ui_kit.h`): `ICON_GLOBE` ya definido (`:35`); `BeginCard(id, icon, title, subtitle, height)`/`EndCard()` (`:91-93`); `SectionTitle` (`:96`); `RowLabel(label, help)` (`:101`); `Tip` (`:105`); `Pill(icon, text, color)` (`:108`); `IconButton` (`:111`); `Hint` (`:114`). Colores: `kOk`, `kWarn`, `kError` (`:54-56`).

### 3.2 Botón JUGAR y arranque [V]

- Botón: `ImGui::Button(play_label, ImVec2(320, 62))` (`launcher_state.cpp:1346`), bloqueado por `assets_ready` (`:1341-1345` aprox.).
- Disparo (`:1414-1449`):
  - `SaveUserSettings()` (`:1415`), `ApplyUserSettingsToSdk()` (`:1416`), `ApplyRuntimeSettingsToSdk(true)` (`:1417`), `ApplyWindowSizeToSdk()` (`:1418`).
  - Si hay personajes nuevos, `mod_pipeline_.BuildRoster()` (`:1422-1427`).
  - `Close()` (`:1445`) y `on_play_()` (`:1446-1447`).
- Callback del launcher en `src\main.cpp:335-364`: `SaveUserSettings` (`:341`), `RelocateGameData` (`:350`), tamaño y pantalla completa (`:356-362`), y `ReXApp::LaunchModule()` (`:363`).
- Override de `LaunchModule`: `src\main.cpp:711-787`. Dev: `dbz3_skip_launcher` (`:712`, declarado en `:70`).
- `OnPostLaunchModule` marca `launched_ = true` (`src\main.cpp:400-404`).

### 3.3 Cómo se pasan opciones al juego [V]

- **Mismo proceso.** El launcher es un diálogo ImGui dentro del propio exe. No hay argv ni ficheros `.req` para esto. El paso de opciones es por cvars en memoria.
- Cvars del launcher: `REXCVAR_DEFINE_*` en `src\launcher\settings.cpp` (patrón en `:43-60`, `:229-233`). Ejemplo: `REXCVAR_DEFINE_STRING(dbz3_input_backend, ..., "DBZ3/Input", "...").allowed({...}).lifecycle(...)`.
- Persistencia: `SaveUserSettings()` (`settings.cpp:627-640`) llama a `rex::cvar::SaveConfig(path)`, que guarda las cvars modificadas. Las cvars nuevas se guardan solas.
- Ruta de `dbz3_user.toml`: `UserSettingsPath()` (`settings.cpp:529-540`). Junto al exe si se puede escribir; si no, `UserDataRoot()`.
- Reenvío a cvars del SDK: `rex::cvar::SetFlagByName(...)` (`launcher_state.cpp:1276`; `settings.cpp:628-629`).
- `dbz3_input.req` (`mnk_input_driver.cpp:156-226`): solo pruebas. No sirve.

### 3.4 Propuesta: botón y panel "Online" [I]

> **Estado 2026-10-11: NO hecha.** No hay pestaña Online en el launcher. El online vive en el propio juego: fila ONLINE del menú de Duelo (`src/ingame/online_panel.cpp`). Lo de abajo queda como propuesta descartada salvo que se pida.

**Pestaña nueva** (no botón extra), con `ICON_GLOBE`, insertada tras "Controles" (`launcher_state.cpp:1146`). Subir `% 10` a `% 11` en `:646`.

Contenido:
- **Crear partida:** IP local (`getaddrinfo`/`gethostname`, pendiente de implementar) + puerto, botón "Copiar". Botón "Esperar rival" que llama a `Session::Open({kHost, ...})`.
- **Unirse:** campo IP + puerto (`ImGui::InputText`), botón "Conectar" (`Session::Open({kGuest, ip, port, ...})`).
- **Estado:** `Pill` con el estado del enlace (esperando / conectado / mods distintos / perdido).
- **Mods:** `Pill` verde "iguales" o rojo "distintos" comparando el hash (§4).
- **Retardo:** combo 2-8 (solo el host lo elige; viaja en `hello`).
- **Empezar partida:** habilitado cuando hay enlace y mods iguales. Llama a la misma secuencia que JUGAR (`Close()`, `on_play_()`, §3.2).
- **IP pública:** no consultar servicios externos sin permiso del usuario. Mostrar un campo manual y una nota.

**Cvars nuevas** (en `settings.cpp`, junto a `:229`): `dbz3_online_ip` (string), `dbz3_online_port` (int32, 1024-65535, propuesta 7777), `dbz3_online_delay` (int32, 2-8), `dbz3_online_name` (string).

**Paso de estado al juego:** el launcher y el juego están en el mismo proceso. `Session` es un singleton global en `src\net\`. El launcher lo configura antes de `on_play_()`. No hace falta argv.

---

## 4. Mods: comparar listas entre dos PCs (punto 4)

### 4.1 Dónde viven los mods [V]

- `ModsRoot()` = `GetExecutableFolder() / "mods"` (`src\mods.cpp:28-32`). **No sube niveles.**
- `AfsModsRoot()` del runtime (`rexglue-sdk-0.10\src\filesystem\afs.cpp:343-354`) sube hasta 4 niveles buscando `mods/`.
- `ModsOutDir()` del launcher (`src\launcher\mod_pipeline.cpp:67-80`) también sube hasta 4 niveles.
- **Discrepancia:** en la release, el exe vive en `dbz3_avx2/` o `dbz3_legacy/` (`github\patches\README.md` sección 4). Con esas rutas, `ListMods()` (que usa `ModsRoot`) no vería la misma carpeta que el juego. [V en código; no reproducido]
- Acción: usar la misma resolución que el runtime (`AfsModsRoot`) para listar y hashear.

### 4.2 Lista de mods activos [V]

- `ListMods()` (`src\mods.cpp:281-306`): cada subcarpeta es un mod. `enabled = !FolderIsDisabled(...)` (`:303`), que mira el marcador `.disabled` (`:49`).
- Perfiles: `profiles.txt` (`src\mods.cpp:725`), `ApplyProfile` (`:828`).
- `ModInfo` (`src\mods.h:27-44`): `name`, `enabled`, `file_count`, `problems`.

### 4.3 build.hash: por qué no sirve [V]

- `mod center hd\roster_build.py:122` define `OUT_MOD = "_roster"`.
- Digest: `inputs_hash(srcs + trajes)` (`roster_build.py:1564`), que hashea:
  - `VERSION`, `roster_db.json`, `roster_build.py`, `model_render.py`, `voces.py`, `gritos.py`, `capsulas.py` (`:1042-1049`);
  - por cada fuente, `relpath|size|int(st_mtime)` (`:1050-1057`). Excluye `_vista` (`:1052-1053`).
- Se escribe en `mods\_roster\build.hash` (`:1565`) y se compara en `:1566`.
- Problemas: (a) `mtime` cambia al copiar la carpeta, así que dos PCs con los mismos mods darían "distintos". (b) Incluye herramientas locales. (c) Solo cubre personajes nuevos (`srcs` + `trajes`), no modelos, texturas ni AFS de otros mods. (d) Solo se escribe al construir (`mod_pipeline.cpp:702-711`).
- Conclusión: no sirve para comparar. Hacer un hash propio.

### 4.4 Propuesta: fingerprint de mods [I]

- Lista ordenada por nombre de los mods con `enabled == true`.
- Para cada mod, archivos ordenados (excluyendo `_vista`): `"<mod>/<ruta relativa>|<tamaño>|<hash del archivo>"`.
- Hash del archivo: `rex::hash_file(path)` (XXH3-128, `rexglue-sdk-0.10\include\rex\hash.h:54`). Hash total: `rex::hash_bytes(acumulado)` (`hash.h:50`). Ya lo usa el SDK para texturas.
- Caché por (ruta, tamaño, mtime), como el banner de `settings.cpp:1003`.
- Existe un MD5 portable en `settings.cpp` (`FileMd5Hex`, `:965-985`), pero está en un espacio de nombres anónimo. No hace falta: XXH3 ya está.
- **Coste:** el pack de texturas HD puede tener decenas de miles de PNG (la memoria del proyecto habla de ~44k). El primer cálculo puede tardar; la caché es obligatoria. [I, sin medir]
- **Abierto:** `_roster` generado, ¿es reproducible byte a byte entre PCs? Si no, excluirlo y hashear sus fuentes (`personaje.toml` y modelos). [P]
- La versión del build va aparte: `DBZ3_VERSION_STR "1.4.3.1"` (`src\version.rc:40`) y `DBZ3_RUNTIME_BUILD` (`rexglue-sdk-0.10\include\rex\dbz3_build.h`).
- Comparación: `hello.mods_hash == mi_hash` → "iguales". Si no, rechazar la conexión y mostrar qué mod difiere (mandar la lista de hashes por mod en un segundo mensaje si hace falta).

---

## 5. Sockets (punto 5)

- **`CMakeLists.txt` raíz**: **corregido 2026-10-11.** En WIN32 enlaza `winhttp version ws2_32 ole32 oleaut32` (`CMakeLists.txt:115`). `ws2_32` ya está (red) y `ole32`/`oleaut32` (UPnP del router). Lo de abajo describe el estado de la fecha original.
- (Original, 2026-10-10:) `CMakeLists.txt` enlazaba `rex::runtime`, `rex::gpu-xenos` y `winhttp version`, sin `ws2_32`.
- Transitivo: `rex::TracyClient` tiene `ws2_32` (`rexglue\lib\cmake\rexglue\rexglueTargets.cmake:204`), pero solo para Tracy. `rexruntime` lo enlaza en PRIVATE (`rexglue-sdk-0.10\src\system\CMakeLists.txt:122`). No se propaga al exe. Hay que enlazarlo explícitamente.
- **Código de red en el proyecto:** ninguno. Búsqueda de `socket|sockaddr|WSA|winsock` en `src\`: vacía.
- **SDK (host):** `rex/net/socket.h` solo expone `socket_close` y `socket_ioctl` (`rexglue-sdk-0.10\include\rex\net\socket.h:11-15`). Sin API de UDP. `XSocket` (`include\rex\system\xsocket.h:70`) es el socket del guest, no sirve en el host.
- **Guest:** solo importa `NetDll_WSAStartup` (`generated\dbz3_recomp.26.cpp:35038`) y `NetDll_WSACleanup` (`dbz3_recomp.31.cpp:34462`). Ninguna llamada a `socket`, `sendto`, `recvfrom` ni `XSession*`. Confirma la corrección de F0 §0.
- **Linux:** el CI de Linux compila el mismo proyecto (`github\patches\README.md`, sección v1.4.0: "es lo que hace el CI de Linux"). El código Winsock debe ir bajo `#ifdef _WIN32`, o usar sockets POSIX equivalentes (`rexglue-sdk-0.10\src\core\socket_posix.cpp` existe en el SDK).
- Firewall: regla UDP de entrada para el puerto (solo documentación, punto 8 de F0 §5).

---

## 6. Esqueleto mínimo (C++, no compilado)

### 6.1 Ficheros nuevos

- `src\net\netplay.h` / `src\net\netplay.cpp`: `Session` (sesión, socket, buffers por tick, protocolo).
- `src\net\netplay_hooks.cpp`: `REX_HOOK_RAW(sub_820902F8)`, `REX_HOOK_RAW(sub_8221C178)`.
- `src\net\mods_fingerprint.h` / `.cpp`: `ModsFingerprint()`.
- `src\launcher\launcher_state.cpp`: `DrawOnlineTab()` y la pestaña nueva.
- `CMakeLists.txt`: añadir los ficheros en `DBZ3_SOURCES` (`:14-31`) y `ws2_32` en `:103`.

### 6.2 Protocolo (UDP, little-endian, empaquetado)

```cpp
// src/net/netplay.h  (ESQUELETO)
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

namespace dbz3::net {

enum class Role : uint8_t { kNone, kHost, kGuest };
enum class MsgType : uint8_t { kHello = 1, kHelloAck = 2, kInput = 3, kPing = 4, kPong = 5, kBye = 6 };
inline constexpr uint32_t kMagic = 0x44425A33u;  // bytes 'D','B','Z','3' (por revisar al implementar)
inline constexpr uint8_t kProto = 1;

#pragma pack(push, 1)
struct PadFrame {          // 16 bytes, un tick de mando
  uint32_t tick;
  uint16_t buttons;
  uint8_t lt, rt;
  int16_t lx, ly, rx, ry;
};
struct Header {            // 8 bytes
  uint32_t magic;
  MsgType type;
  uint8_t proto;
  uint16_t reserved;
};
// hello : Header + build[16] + mods_hash[32] + delay(u8) + role(u8)
// input : Header + tick_apply(u32) + count(u8) + PadFrame[count] (count 1..8, redundancia)
// ping/pong : Header + t_send(u64)
#pragma pack(pop)
static_assert(sizeof(PadFrame) == 16);

struct Config {
  Role role = Role::kNone;
  std::string ip;               // solo guest
  uint16_t port = 7777;         // propuesta, configurable
  uint8_t delay = 3;            // 2-8, lo elige el host
  std::string mods_hash;        // ModsFingerprint()
};

enum class Link : uint8_t { kIdle, kWaiting, kConnected, kModsMismatch, kLost };

class Session {
 public:
  static Session& Get();
  // Hilo de UI (launcher). No bloquea el juego.
  bool Open(const Config& cfg, std::string* error);
  void Close();
  Link link() const;
  std::string LocalAddress() const;   // IP local y puerto, para mostrar

  // Hilo del guest (hooks). Solo con la sesión iniciada.
  void Start();                       // el tick 0 es la primera iteración del juego
  bool active() const { return running_.load(); }
  // Muestrea el mando local en bruto, envía, espera al rival (puede bloquear).
  void BeginTick(const PadFrame& local_raw);
  PadFrame Applied(uint32_t user) const;   // user 0 = local con retardo, 1 = rival

  uint32_t tick() const { return tick_.load(); }

 private:
  bool Pump();                        // recv + procesa; sin locks durante la espera
  std::atomic<bool> running_{false};
  std::atomic<uint32_t> tick_{0};
  mutable std::mutex mu_;
  std::array<PadFrame, 256> hist_{};  // indexado por tick % 256
  std::array<PadFrame, 256> remote_{};
  PadFrame applied_[2]{};
  uintptr_t sock_ = 0;                // SOCKET (Windows)
};

}  // namespace dbz3::net
```

### 6.3 Hook del tick (sustituye al cuerpo de `sub_820902F8`)

```cpp
// src/net/netplay_hooks.cpp  (ESQUELETO; mismo patrón que src/hooks.cpp)
#include <rex/hook.h>
#include "generated/dbz3_init.h"
#include "net/netplay.h"

REX_EXTERN(sub_8208FC40);
REX_EXTERN(sub_8208FD58);
REX_EXTERN(sub_8221C188);

REX_HOOK_RAW(sub_820902F8) {
  auto& s = dbz3::net::Session::Get();
  if (s.active()) {
    s.BeginTick(/* mando local en bruto: GetState(0) convertido a PadFrame */);  // puede bloquear
  }
  for (uint32_t u = 0; u < 4; ++u) {                 // dbz3_recomp.20.cpp:680-701
    ctx.r3.u64 = u;
    sub_8208FC40(ctx, base);
    if (ctx.r3.s32 != 0) {
      ctx.r3.u64 = u;
      sub_8208FD58(ctx, base);
    }
  }
  for (uint32_t u = 0; u < 4; ++u) {                 // dbz3_recomp.20.cpp:710-727
    ctx.r3.u64 = u;
    ctx.r4.u64 = 0x8238B6C0 + 4 * u;
    sub_8221C188(ctx, base);
  }
}
```

### 6.4 Hook del estado (usuario 0 y 1)

```cpp
REX_EXTERN(__imp__XamInputGetState);

REX_HOOK_RAW(sub_8221C178) {
  const uint32_t user = ctx.r3.u32;
  const uint32_t out = ctx.r4.u32;            // 0x8238B680 + 16*user
  auto& s = dbz3::net::Session::Get();
  if (user < 2 && s.active()) {
    const auto p = s.Applied(user);
    REX_STORE_U32(out + 0, s.tick());         // packet_number
    REX_STORE_U16(out + 4, p.buttons);
    REX_STORE_U8(out + 6, p.lt);
    REX_STORE_U8(out + 7, p.rt);
    REX_STORE_U16(out + 8,  static_cast<uint16_t>(p.lx));
    REX_STORE_U16(out + 10, static_cast<uint16_t>(p.ly));
    REX_STORE_U16(out + 12, static_cast<uint16_t>(p.rx));
    REX_STORE_U16(out + 14, static_cast<uint16_t>(p.ry));
    ctx.r3.u64 = 0;                           // X_ERROR_SUCCESS
    return;
  }
  ctx.r5.u64 = out;                           // mr r5,r4
  ctx.r4.u64 = 1;                             // li r4,1 (flags GAMEPAD)
  __imp__XamInputGetState(ctx, base);
}
```

Nota: el `REX_STORE_U16`/`U8` deben confirmarse en `rex/ppc/...` (el código generado los usa: `dbz3_recomp.12.cpp:22012`, `dbz3_recomp.14.cpp:13048`).

### 6.5 Fingerprint de mods

```cpp
// src/net/mods_fingerprint.cpp  (ESQUELETO)
#include <algorithm>
#include <filesystem>
#include <vector>
#include <rex/hash.h>
#include "../mods.h"                      // dbz3::ListMods (src/mods.cpp:281)

std::string ModsFingerprint() {
  auto mods = dbz3::ListMods();           // ya excluye .disabled
  std::sort(mods.begin(), mods.end(), [](const auto& a, const auto& b) { return a.name < b.name; });
  std::string acc;
  for (const auto& m : mods) {
    if (!m.enabled) continue;
    // recorrer dbz3::ModsRoot()/m.name (ojo §4.1: usar la raiz del runtime), ordenar,
    // saltar rutas que contengan "_vista" (roster_build.py:1052-1053):
    //   acc += m.name + "/" + rel + "|" + std::to_string(size) + "|" + rex::hash_file(p) + "\n";
  }
  return rex::hash_bytes(acc);            // 32 hex (XXH3-128)
}
```

### 6.6 Pestaña Online (launcher)

```cpp
// src/launcher/launcher_state.cpp  (ESQUELETO)
// 1) Tras la pestaña "tab_input" (launcher_state.cpp:1146):
if (begin_tab(tab_label(ICON_GLOBE, i18n::T("Online", "Online"), "tab_online"))) {
  DrawOnlineTab();
  ImGui::EndTabItem();
}
// 2) Cambiar el salto de Ctrl+Tab (launcher_state.cpp:646): % 10  ->  % 11

void LauncherDialog::DrawOnlineTab() {
  auto& net = dbz3::net::Session::Get();
  ui::BeginCard("online_host", ICON_GLOBE, i18n::T("Crear partida", "Host a game"), nullptr);
  ui::RowLabel(i18n::T("Tu direccion", "Your address"));
  ImGui::TextUnformatted(net.LocalAddress().c_str());
  // boton "Esperar rival" -> net.Open({Role::kHost, "", port, delay, ModsFingerprint()}, &err)
  ui::EndCard();

  ui::BeginCard("online_join", ICON_GLOBE, i18n::T("Unirse", "Join"), nullptr);
  ImGui::InputText("##ip", online_ip_buf_, sizeof(online_ip_buf_));  // patron: launcher_state.cpp:171
  // boton "Conectar" -> net.Open({Role::kGuest, online_ip_buf_, port, 0, ModsFingerprint()}, &err)
  ui::EndCard();
  // Estado (Pill), mods (iguales / distintos), boton "Empezar partida" -> misma secuencia que JUGAR
}
```

---

## 7. Build y trampas

- Compilar solo el target `dbz3`. Tras cualquier build, recopiar las DLL canónicas (`CLAUDE.md` del proyecto; `AGENTS.md` §4 y §7). El build sobrescribe `rexruntime.dll` con una versión antigua.
- Si no se toca el SDK, no hace falta tocar sus DLL. Toda la parte nueva vive en el exe `dbz3`.
- Release: el exe sale de `out/build/win-amd64-dual/` (`CLAUDE.md`).
- Si se toca `input_system.cpp` (opción B), actualizar también `github\patches\rexglue-sdk\src\input\input_system.cpp`.
- Ctrl+Tab: `% 10` a `% 11` (§3.1).
- Linux: `#ifdef _WIN32` para Winsock, o alternativa POSIX.

---

## 8. Pendiente de verificar [P]

1. Dónde se frena el bucle a 60 Hz (§2.2). Trazar la vtable `[r30+40]` (`dbz3_recomp.14.cpp:13113-13122`).
2. Cómo termina el juego, dado que la condición de salida del bucle es un stub (§2.1).
3. Lectores de tiempo del guest (F0 §2.2): si alimentan lógica de combate, la sincronía falla.
4. Riesgo de interbloqueo con el spinlock del callback de vblank (§2.3).
5. Si `_roster` es reproducible entre PCs (§4.4).
6. Mapas de RAM para el desync check (`docs\03_formatos\`, no revisados).
7. Menú rápido: abrirlo pone el mando local a cero, pero el juego sigue corriendo. Decidir si pausa a ambos.
8. Foco de ventana: al perderlo, el mando local se envía como neutro. Decidir si el juego pausa.
9. Macros `REX_STORE_U16/U8` y declaración de `__imp__XamInputGetState` para `REX_EXTERN`.
10. Medir el coste del primer hash de mods con el pack de texturas.

---

## 9. Lista de archivos de referencia

- Entrada SDK: `rexglue-sdk-0.10\src\input\input_system.cpp:30, 85-90, 92-181, 232-266, 308-361, 477-481`; `rexglue-sdk-0.10\src\input\state_merge.cpp:31-44`; `rexglue-sdk-0.10\src\input\device_assignment.cpp:16-38`; `rexglue-sdk-0.10\include\rex\input\input_system.h:47-55`; `rexglue-sdk-0.10\include\rex\input\input_driver.h:30-66`; `rexglue-sdk-0.10\include\rex\input\device.h:18-31`; `rexglue-sdk-0.10\include\rex\input\input.h:61-76`.
- Drivers: `rexglue-sdk-0.10\src\input\nop\nop_input_driver.cpp:39-80`; `...\mnk\mnk_input_driver.cpp:310-363`; `...\xinput\xinput_input_driver.cpp:128-151`; `...\sdl\sdl_input_driver.cpp:166-222`.
- XAM: `rexglue-sdk-0.10\src\kernel\xam\xam_input.cpp:37-38, 95-116`.
- Guest: `generated\dbz3_recomp.14.cpp:12915, 13051-13134`; `generated\dbz3_recomp.20.cpp:666-733`; `generated\dbz3_recomp.6.cpp:688-773`; `generated\dbz3_recomp.1.cpp:662-697`; `generated\dbz3_recomp.18.cpp:22668-22677`; `generated\dbz3_recomp.12.cpp:21984-22030`; `generated\dbz3_recomp.29.cpp:30346-30352`; `generated\dbz3_recomp.42.cpp:36734-36737`; `generated\dbz3_recomp.26.cpp:35038`; `generated\dbz3_recomp.31.cpp:34462`.
- Hooks existentes: `src\hooks.cpp:25-66`; `src\roster_trace.cpp:223` (`REX_HOOK_RAW`).
- Launcher: `src\launcher\launcher_state.h:40-98`; `src\launcher\launcher_state.cpp:171, 637, 646, 1128-1176, 1346, 1414-1449, 2014`; `src\launcher\ui_kit.h:35, 91-114`; `src\launcher\settings.cpp:43-60, 229-233, 527-540, 627-640, 965-985, 1003`; `src\main.cpp:70, 301-307, 334-365, 400-404, 711-787`.
- Mods: `src\mods.cpp:28-32, 49, 281-306, 725, 828`; `src\mods.h:27-44`; `src\launcher\mod_pipeline.cpp:32-44, 67-80, 702-732`; `mod center hd\roster_build.py:122, 150-154, 1042-1058, 1564-1566`; `rexglue-sdk-0.10\src\filesystem\afs.cpp:343-354`; `rexglue-sdk-0.10\include\rex\hash.h:48-54`.
- Build y sockets: `CMakeLists.txt:14-31, 95-104`; `rexglue\lib\cmake\rexglue\rexglueTargets.cmake:204`; `rexglue-sdk-0.10\src\system\CMakeLists.txt:122`; `rexglue-sdk-0.10\include\rex\net\socket.h:11-15`; `rexglue-sdk-0.10\src\core\socket_win.cpp`, `socket_posix.cpp`.
- Overlay: `github\patches\rexglue-sdk\src\input\input_system.cpp` (idéntico); `github\patches\README.md` (secciones 4, 13, v1.4.0).
- Vblank y swap: `rexglue-sdk-0.10\src\graphics\graphics_system.cpp:165-203, 340, 362-375`; `rexglue-sdk-0.10\src\graphics\command_processor.cpp:949, 990`; `rexglue-sdk-0.10\src\graphics\d3d12\command_processor.cpp:2775`.
- Versión: `src\version.rc:40` (`DBZ3_VERSION_STR`); `rexglue-sdk-0.10\include\rex\dbz3_build.h`.
