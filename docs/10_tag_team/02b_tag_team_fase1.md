# Estudio 02b: modo Tag Team, fase 1 (RE del cambio de forma y de la carga)

Solo lectura. No se ha editado ningún archivo del proyecto ni se ha ejecutado el juego.

Ruta base: la raíz del proyecto
Código recompilado: `generated\dbz3_recomp.N.cpp` (las líneas que se citan son la línea de `DEFINE_REX_FUNC`).
Convención de este documento:
- **cfg** = `*(0x823D6CF4)`, el bloque de configuración de batalla.
- **X_k** = bloque de estado del jugador k = `cfg + 2096 + 8240·k` (k = 0..3). Es el "luchador" del estudio anterior.
- **T_k** = instancia de modelo = `X_k + 1616 + 1664·k'`. Hay al menos dos: `X+1616` (cuerpo principal) y `X+3280` (segunda instancia).
- Marcas: **[C]** = comprobado en el código (instrucción PPC leída). **[I]** = inferido por uso o por nombre. **[N]** = no confirmado.

## Resumen

- **Punto 1 (función de transformación): [C].** El cambio real del modelo es `sub_820F9970` (`dbz3_recomp.22.cpp:6490`). Escribe en la instancia el ID de modelo, el traje y el puntero a la entrada de forma. La entrada de forma puede ser de **otro ID** (`+2`), así que el motor ya sabe cambiar de cuerpo a otro personaje.
- **Vida: [N].** No he encontrado el campo de vida. El ki sí: `X+6100` (f32) [I].
- **Punto 2 (carga): [C] en la parte de batalla.** La carga se hace en `sub_82109BA0` (`dbz3_recomp.21.cpp:7374`), que ya recorre **4 bloques de jugador**. No he medido memoria ni si un jugador extra se pinta.
- **Punto 3 (ID en caliente): [C] para el ID, [I] para el efecto.** El ID `X+7420` solo se escribe en `sub_82109BA0`, al empezar el combate. Las funciones de forma leen en cada frame el puntero `X+7416` (tabla de forma) y `X+7412`. Cambiarlo a mitad de combate no recarga modelos ni voces por sí solo.

## 1. Función de cambio de forma y campos que toca

### 1.1 Cadena de llamadas (cuando se pulsa P+K+G)

| Paso | Función | Evidencia | Qué hace |
|---|---|---|---|
| 1 | `sub_82101060` | `dbz3_recomp.2.cpp:7056` [C] | Valida el comando del BCM. Si `w5 & 0x4` (transformar) llama a `sub_82119108`. Si `w5 & 0x1`, comprueba ki con `w9`. Si `w5 & 0x2`, comprueba cápsula con `w8` (`sub_82115980`). |
| 2 | `sub_82119108` | `dbz3_recomp.15.cpp:8286` [C] | **Calcula** la forma destino hacia arriba. No aplica nada. |
| 3 | `sub_82119258` | `dbz3_recomp.5.cpp:8461` [C] | **Calcula** la forma destino hacia abajo (revertir). |
| 4 | `sub_8211A510` | `dbz3_recomp.10.cpp:8343` [C] | Reconciliador: si destino ≠ actual, llama a `sub_820FB5D0(T, f)`. |
| 5 | `sub_820FB5D0` | `dbz3_recomp.41.cpp:6442` [C] | Crea el objeto de acción de transformación. Elige el manejador según el bit 0 del byte `+7` de la forma (`0x820FA940` / `0x820FA7D8` para subir, `0x820FAD98` para bajar) [C en las direcciones; el destino de cada manejador no lo he seguido]. |
| 6 | `sub_8211D9C8` | `dbz3_recomp.10.cpp:8518` [C] | Ejecutor de evento de la animación. Parámetros en `[ev+44]`: `+0` forma, `+4` instancia, `+8` jugador. |
| 7 | `sub_82115270` (instancia 0) o `sub_820F9970` directo (instancia ≠ 0) | `dbz3_recomp.19.cpp:8073` [C] | Aplica la forma al cuerpo principal, recalcula el nivel base de ki (`sub_82100760`) y avisa (`sub_820B86A0`). |
| 8 | `sub_820F9970(T, f)` | `dbz3_recomp.22.cpp:6490` [C] | **Cambio real del modelo.** Ver 1.2. |

Comando P+K+G (dato del estudio anterior, coherente con el código): `w5 = 0x0004` activa el paso 2 mediante la rama `andi. 1031` de `sub_82101060`. [C]

### 1.2 Qué escribe `sub_820F9970(T, f)` en la instancia T

Entrada: `T` (r3) y forma `f` (r4). Si `f < 0`, es un reset. [C]

| Offset en T | Valor | Origen |
|---|---|---|
| `+1364` | forma actual de la instancia | `f` |
| `+1368` | forma | `f` |
| `+1356` | clave de modelo (u16) | `char96[ID_entrada].formas[idx]` (`ID = entrada+2`, `idx = entrada+4`) |
| `+1360` | traje (u8) | `X + 7436 + f` [I: byte por forma] |
| `+1372` | puntero a `char96` del ID | `char96 + 96·ID` |
| `+1376` | puntero a la entrada de forma | `X + 7444 + 20f` si el bit `f` de `X+7604` está activo; si no, `char372 + 212 + 20f` |
| `+1380` | tabla de físicas | `entrada+16`, o una tabla fija |
| `+704`, `+708`, `+712` | datos extra por forma | `X + 7676 + 28f` [I] |

La resolución del modelo usa `sub_821399c0(ID, idx, traje)` [C].

Consecuencia: el ID de la **entrada de forma** (`+2`) decide qué modelo se carga. Ya está usado para Goku SSJ4 (ID 91) en `roster_ext.cpp:298-310`.

### 1.3 Campos del luchador X (offsets desde X_k)

| Offset | Tipo | Significado | Evidencia | Estado |
|---|---|---|---|---|
| `+1616` | objeto | Instancia T0 (cuerpo principal). `+1384` apunta a X. | `sub_820FB5D0` (`41.cpp:6442`), `sub_82100D60` (`4.cpp:7136`) | [C] |
| `+3280` | objeto | Instancia T1 (segunda instancia, `1616 + 1664`) | `sub_82100D60`, `addi r29,r31,3280` | [C] uso / [I] significado |
| `+1384` | ptr | Puntero de vuelta a X desde T | `sub_820F9970`, `sub_820FB5D0` | [C] |
| `+2980` | i32 | **Forma actual** | leído en `sub_82119108`, `sub_82119258`, `sub_82100EB8`, etc. | [C] lectura. **Escritor no localizado**: no aparece como `stw ...,2980(rN)` en todo `generated/`. |
| `+2984` | i32 | **Forma destino** (siguiente) | escrito en `sub_8210E100` (`29.cpp:7737`) con el resultado de `sub_82119108` | [C] |
| `+6100` | f32 | **Ki actual** | `sub_82115518` (`23.cpp:8448`) resta y acota con `+6104`; `sub_82100760` da el nivel base | [I] (uso coherente con un medidor de ki) |
| `+6104` | f32 | Tope de ki | `sub_82115518` (`lfs f30,6104`) | [I] |
| `+6232` | f32 | Temporizador de ki (se pone a `-1104`) | `sub_82115518` | [I] |
| `+6728` | i32 | Forma guardada (para volver atrás) | `sub_8211A510`, `sub_82109638` (`25.cpp:7574`) | [C] escritura / [I] significado |
| `+7412` | ptr | **Registro de selección** del jugador (`cfg+1696+80k`). `+22` s16 = forma mínima; `+24` s16 = forma máxima | `sub_82109BA0` escribe el puntero; `sub_82119108` usa `+24`; `sub_82119258` usa `+22` | [C] puntero / [I] significado de +22 y +24 |
| `+7416` | ptr | **Registro de combate del ID actual** = `char372 + 372·ID`. Tabla de formas en `+212 + 20f`. | `sub_82109BA0` (`21.cpp:7374`) | [C] |
| `+7420` | u32 | **ID de personaje** | copiado de `sel+32+80k` en `sub_82109BA0` | [C] |
| `+7436 + f` | u8[] | Traje por forma | `sub_82101240` (`27.cpp:7028`) | [I] |
| `+7444 + 20f` | 20 B × 8 | Copia local de la entrada de forma | `sub_82100AA8` (`29.cpp:6966`) escribe `+7444` | [C] escritura / [I] uso |
| `+7604` | u16 | Máscara: bit `f` = la forma `f` usa la copia local | `sub_82109638`, `sub_82100AA8` | [C] |
| `+7608 + 4f` | ptr[8] | Recurso cargado por forma (se libera con `sub_82133D00`) | `sub_82100D60` (`4.cpp:7136`) | [I] |
| `+8156 + f` | u8[8] | Cápsula equipada para la forma `f` | `sub_82112260` (`28.cpp:8056`) escribe; `sub_82119108` lee | [C] |
| `+8184` | u8 | 0xFF = sin transformación pendiente | `sub_82115518`, `sub_8211A510` | [I] |
| `+8207` | u8 | Estado de transformación (2 = caso especial) | `sub_8211A510`, `sub_82119258` | [C] lectura / [I] significado |
| `+8208` | u8 | Ki infinito o bloqueo (no determinado) | `sub_82115518`, `sub_82101060` | [I] |

Datos fijos de las formas (no son del luchador, son de la tabla de `char372`):
- `char372 + 212 + 20f`: `+0` cápsula, `+2` ID del modelo, `+4` índice en `char96`, `+6` tipo, `+7` bit 0 = rutina "Saiyan", `+9` nivel base de ki. Coincide con `roster_ext.cpp:298-310`, `338-349`, `583-603`. [C]
- `char372 + 208` (`+0xD0`): número de formas. `roster_ext.cpp:338, 583`. [C]
- Moveset por forma: `char372 + 4f` (ANM por forma). `roster_ext.cpp:10, 42` y `docs/07_ports/.../evid_char372_formas.txt`. [C en la tabla] / [I] que el cambio de forma toma el ANM del ID propio (`X+7416`).
- Cámara: `char372 + 0x20` (CAM). `roster_ext.cpp:10-11`. [C tabla] / [N] no seguido en el código de forma.

### 1.4 Vida (HP)

**[N] No localizado.** `docs/09_mods_nativos.md:38` dice: "Vida infinita" y "Ki infinito" aparecen como *Sin código encontrado*. Candidatos que aparecen en las funciones de golpe, sin verificar: `X+6048` y `X+6060` (`sub_8210ED48`, `sub_82116458`, `sub_8210DDF8`). No se deben usar sin una prueba en el juego.

## 2. Carga de recursos antes del combate

### 2.1 Batalla (una vez por combate)

1. `sub_8210E468` (`dbz3_recomp.7.cpp:7749`) [C] → llama a `sub_82109BA0` y a `sub_82109148`.
2. `sub_82109BA0` (`dbz3_recomp.21.cpp:7374`) [C]:
   - Bucle `li r27,4`: **4 iteraciones**, paso `8240` para X y `80` para el registro de selección.
   - Por jugador: lee el ID de `sel+32+80k` y escribe `X+7420`; calcula `X+7416 = char372 + 372·ID`; escribe `X+7412 = sel+16+80k`.
   - Por jugador llama a: `sub_82113818` (`26.cpp:7964`), `sub_82113870` (`18.cpp:8041`), `sub_82112260` (`28.cpp:8056`).
   - Al final, una vez: `sub_82101240` (`27.cpp:7028`): lee el número de formas (`char372+208`) y rellena `X+7436`.
3. Carga por forma (durante el combate): `sub_82109380` (`4.cpp:7708`) → `sub_82100EB8` (`28.cpp:7057`) y `sub_82100F30` (`3.cpp:7159`). Recorren las formas 0..7 y saltan la actual, la mínima, la máxima y la pedida. Para cada una llaman a `sub_82100D60` (`4.cpp:7136`), que libera o aplica la forma. La carga de los datos en sí está en `sub_82100DE8` (no seguido en detalle).
4. Reset de luchador: `sub_82109638` (`25.cpp:7574`), llamado desde `sub_82110640` (`26.cpp`). Hace reset de forma (`sub_820F63A8`), `sub_82115270`, `sub_82111458` y `sub_82112260`.

Fuente de los IDs y de la selección (el lado C++):
- `src\roster_ext.cpp:1088-1102` (`sub_821007E8`): reescribe `rec+16` (ID de combate, `cfg+1696+80k+16`) en la configuración de batalla antes de la inicialización. [C]
- `src\roster_ext.cpp:1069-1086` (`sub_820FE668`): lee el ID de `sel+32+80k` para cada casilla. [C]
- `src\roster_ext.cpp:1374-1404` (`sub_820FE3D8`): `players = sel+336` (s16), con tope **4**. [C]

### 2.2 Tercer o cuarto personaje

- La estructura **ya tiene 4 bloques de jugador** (`X_0..X_3`, `sub_82109BA0`). [C]
- Un modelo de otro ID ya se puede cargar dentro de una forma: `src\roster_ext.cpp:302-362` (`AppendCostume`, `ApplyExtraCostume`) escribe el modelo de otro ID en la entrada de forma. [C]
- **No comprobado**: si el modo normal activa 2 o 4 bloques (`sel+336`), si el dibujo y las colisiones recorren los bloques k ≥ 2, y cuánta memoria cuesta cargar 3 modelos. [N]
- Voces: `0x823280B0` (puntero por ID) y `0x82328298` (número por ID) en `roster_ext.cpp:14-15`. Son tablas por ID, así que cambiar el ID del bloque cambia la lista de voces si el código las lee del ID activo. [I]
- Cara de la barra de vida: `kHudFace = 0x82373D68` (`roster_ext.cpp:103`), una pareja de bytes por ID. [C tabla] / [I] uso por bloque.

## 3. ¿Se puede cambiar el ID en caliente?

- **ID (`X+7420`)**: solo se escribe en `sub_82109BA0` (inicio de combate). No he encontrado otra escritura, aunque las escrituras por puntero calculado no aparecen en la búsqueda. [C con reserva]
- **Puntero de tabla (`X+7416`)** y **registro de selección (`X+7412`)**: los leen en cada frame `sub_82119108`, `sub_82119258`, `sub_82100EB8` y `sub_82100F30`. Si se cambian entre frames, las funciones de forma usan la tabla del nuevo ID. [I]
- **Lo que NO se actualiza solo**: modelo y traje ya cargados en `T`, recursos de cada forma (`X+7608`), cápsulas (`X+8156`) y voz/cara de HUD. Hay que rehacer la carga de `sub_82109BA0` para ese bloque.

## 4. Lo que no he podido confirmar

1. Quién escribe `X+2980` (forma actual). No aparece como `stw ...,2980(rN)`.
2. Quién escribe `X+7420` y `X+7416` aparte de `sub_82109BA0`. Podrían escribirse por puntero calculado.
3. Vida (HP): campo y offset. Sin código de referencia.
4. Significado exacto de `X+6100` como ki (uso coherente, no probado en juego) y de `X+7412+22/+24` como forma mínima y máxima.
5. Si el ANM de una forma sale del ID propio (`X+7416`) o del ID de la entrada de forma. Si sale del propio, un cambio de forma no cambia de moveset entre personajes.
6. Cámara de cada personaje (`char372+0x20`) en el camino de forma.
7. Cuántos bloques de jugador usa el modo normal (`sel+336`) y si los bloques k ≥ 2 se pintan.
8. Coste de memoria y de tiempo de carga de un modelo extra.
9. Tabla `cfg+35188 + 4·p` que `sub_8211D9C8` usa para obtener el puntero `X_p`. [I] por el patrón de lectura; no lo he visto escrito.
10. Ruta de `sub_820FB5D0` → manejadores `0x820FA940` / `0x820FA7D8`: no seguida hasta el `sub_820F9970`.

## 5. Plan mínimo de implementación (F1, sin ejecutar)

Objetivo: cambiar al compañero reutilizando la transformación, guardando vida y ki por miembro.

**Paso 0 (medir, antes de tocar nada):**
- Hook de lectura (en `src\roster_ext.cpp`, con `DBZ3_HOOK`) sobre `sub_82109BA0` y `sub_8211D9C8` que registre `X+7420`, `X+2980`, `X+6100` por jugador.
- Localizar el HP con un log: leer candidatos (`X+6048`, `X+6060` y floats cercanos) durante un golpe. Esto lo tiene que hacer el usuario en el juego.

**Paso 1 (estado por miembro, solo datos):**
- Guardar en una estructura C++ propia por bloque k: `id` (`X+7420`), `ki` (`X+6100`), `forma` (`X+2980`, `X+2984`), `HP` (cuando esté localizado).
- No copiar el bloque X completo: tiene punteros internos (`T+1384` → X, `X+7412` → cfg, `X+7416`, `X+7608`). Copiarlo rompe los enlaces.

**Paso 2 (cambio, con el combate parado entre frames):**
1. Escribir `X_k+7420 = ID_nuevo` y `X_k+7416 = char372 + 372·ID_nuevo`.
2. Rehacer la carga del bloque: llamar a `sub_82113818`, `sub_82113870`, `sub_82112260` y `sub_82101240` para ese `k`, igual que en `sub_82109BA0`.
3. Restaurar ki (`X+6100`) y forma (`X+2980`, `X+2984`) del miembro que entra.
4. Aplicar la forma con `sub_820F9970(T, forma)` y las funciones de carga por forma (`sub_82100EB8`, `sub_82100F30`).
5. Voz y cara: revisar que la lista de voces y la cara de HUD usan el ID nuevo (`0x823280B0`, `0x82373D68`).

**Paso 3 (validar):** prueba con un jugador humano y CPU, dos casillas, en el juego. Solo el usuario puede hacerlo, porque hay que mirar la pantalla. Comprobar: sin pérdida de HP al cambiar, ki por miembro, sin cierre al cambiar.

**Riesgo principal:** mezcla de estado entre miembros (HP o ki compartido por error) y recargas a mitad de combo. La fase 1 no ha confirmado el campo de HP, así que el paso 1 no se puede cerrar hasta el paso 0.

**Esfuerzo estimado:** el paso 0 es una sesión de medida. El resto es una a tres sesiones de C++ y prueba, según lo que aparezca en el paso 0.

## 6. Paso 0 hecho en el juego (2026-10-10, Claude, sonda de memoria `scratchpad/hpprobe.py`)
Medido en Práctica, Vegeta GT (id 48) contra Goku (id 0). Fue solo lectura de memoria.
- Bloque X del jugador 1 en el guest: `0x823CEA10`. Jugador 2 = X + 8240.
  - Se localiza buscando `X+7420 = id` y `X+7416 - X2+7416 = 372·(id1-id2)`. **[C]**
- **Vida: float en X+6080**, con copias en X+6092 y X+6096 (barras). **[C]**
  - Llena = 3000.0.
  - Un golpe de 250 la deja en 2750.0.
  - En Práctica se rellena a 3000.
- Otros floats que bajan con el golpe, como X+2756, X+2784 y X+2932 (~2028 → 1626), tienen relación con la posición o la distancia. No son vida.
