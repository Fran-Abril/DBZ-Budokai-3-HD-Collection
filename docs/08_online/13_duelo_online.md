# Estudio 13: Modo Duelo y base para el online

Solo lectura. No se ha editado nada del proyecto ni se ha ejecutado el juego.
Raíz: la raíz del proyecto
Código: `generated\dbz3_recomp.N.cpp` (US). Las líneas son las del fichero. La función es la de `DEFINE_REX_FUNC` que la contiene.
Convención: **[C]** = comprobado leyendo la instrucción PPC recompilada o el código de `src`/`docs`. **[I]** = inferido por uso, patrón o nombre. **[N]** = no confirmado.
Scripts de apoyo (solo lectura) en este scratchpad: `fn_at.py`, `showfn.py`, `modeflow.py`, `callargs.py`, `storectx.py`.

## Resumen

1. El estado de pantalla es una **cola de peticiones** de tres enteros en `0x8247A98C..0x8247A990`. Pantalla pedida ahora = `0x8247A990` (-1 = libre). [C]
2. El duelo se lanza cuando esa cola vale **25**. El nodo que lo consume es `sub_821D7400` y crea el duelo con `sub_820FE668(0x8245EFC0, 0)`. [C]
3. La configuración del duelo es un bloque de **384 bytes en `0x8245EFC0`**. Jugadores: ID en `+32` (P1) y `+112` (P2), u16. [C]
4. Escenario/modo: `+338` (modo u16), `+340` (parámetro), `+342` (resultado de tabla). El tiempo está en `+348`. El significado exacto del resultado no está confirmado. [C]/[I]
5. **1P vs 2P no está localizado.** Candidatos: `+336` (número de luchadores, 0 = 2) y `+338`. Se propone medirlo con un volcado diferencial del bloque. [N]
6. Arranque directo: escribir el bloque, pedir la pantalla 25 con `sub_821D2778(25, 0)` y dejar que el juego haga el resto. El punto más fiable es después de `sub_820FE668`, donde ya engancha `roster_ext.cpp:1069`. [I]
7. Textos: el menú está en `data_eng.afs` (5 páginas `#ACA`: personajes, stages, modos). Las entradas del menú principal no están localizadas. [C]/[N]
8. Opción recomendada para "Online": panel propio en ImGui sobre el submenú de Duelo, dibujado en `InGameMenu::OnDraw` (`src\ingame\menu.cpp:14`). Añadir una entrada real al menú del juego implica tocar código recompilado y sin nombres. [I]
9. Discrepancia en `src`: `kCostumeCounts` (`roster_ext.cpp:146`) y `kAvailTable` (`roster_trace.cpp:38`) apuntan ambos a `0x8245F150` con significados distintos. [C]
10. Pendiente de medir en juego: volcado diferencial del bloque por opción del submenú y tabla valor-de-cola -> pantalla. Ninguno de los dos se ha hecho.

## 0. Mapa de direcciones

Las direcciones se calculan a partir de `lis` + desplazamiento en el código recompilado. Comprobado con aritmética manual.

| Dirección | Nombre propuesto | Qué es | Evidencia |
|---|---|---|---|
| `0x8247A988` | `REQ_ARG` | Argumento de la petición de pantalla (r4 de `sub_821D2778`) | [C] `16.cpp:18360` |
| `0x8247A98C` | `REQ_NEXT` | Pantalla en cola cuando `REQ_CUR` está ocupada. Una sola plaza, se sobrescribe | [C] `16.cpp:18355` |
| `0x8247A990` | `REQ_CUR` | Pantalla pedida ahora. -1 = libre | [C] `16.cpp:18342-18348`, `22.cpp:19003-19008` |
| `0x8247A994` | `REQ_FLAG` | Bandera: `sub_821D27C8` la pone a 1. Las páginas la ponen a 0 al consumir la cola | [C] `29.cpp:18689`, `10.cpp:18846` |
| `0x824BA110` | `UI_STATE` | Estado de menú y perfil (`kUnlockStruct`) | [C] `src\roster_trace.cpp:40` |
| `0x824BA111` | `UI_PAGE` | Fase/página del menú. Valores 1 a 6 en `sub_821D3628` | [C] `14.cpp:19099-19129` |
| `0x824BA185` / `0x824BA186` | `UI_PEND_A/B` | Bytes pendientes que lee `sub_821D3888` | [C] `37.cpp:18080-18107` |
| `0x824BA12F` / `0x824BA130` | `PICKED_P1/P2` | Último personaje confirmado (P1, P2) | [C] `src\select_ext.cpp:437-441` |
| `0x823D6CF4` | `DUEL_PTR` | Puntero a la configuración de combate `D` | [C] `src\roster_ext.cpp:1064`, `5.cpp:6925-6931` |
| `0x823CE1E0` | `DUEL_DEFAULT` | Valor de `D` si `DUEL_PTR` está vacío (lo crea `sub_820FE668`) | [C] `5.cpp:6937-6941` |
| `0x8245EFC0` | `CFG` | Bloque de selección/configuración del duelo, 384 bytes. Es el `r3` de `sub_820FE668` cuando lo llama `sub_821D7400` | [C] `22.cpp:19001-19037` (lis `-32187`, desplazamientos `25728+65536-29888`) |
| `0x82456480` | `M` | Base de la que cuelga `CFG` (`CFG = M + 35648`). Campos `M+36000` y `M+36036` | [C] `15.cpp:13384-13412` |

Sobre `D`: la copia de `CFG` se hace en `D+1680` y tiene 384 bytes. Por eso `D+k` = `CFG + (k-1680)`.

## 1. Detectar la pantalla

### 1.1 Mecanismo [C]

- **Pedir pantalla**: `sub_821D2778(id r3, arg r4)` (`dbz3_recomp.16.cpp:18337-18362`). Si `REQ_CUR == -1` escribe `REQ_CUR = id`. Si no, escribe `REQ_NEXT = id`. Siempre escribe `REQ_ARG = arg`.
- **Bandera**: `sub_821D27C8(r3)` llama antes a `sub_821652D0` y pone `REQ_FLAG = 1` (`dbz3_recomp.29.cpp:18670-18689`).
- **Consumo**: las páginas hacen de "pop". Copian `REQ_CUR = REQ_NEXT` y ponen `REQ_FLAG = 0` (ejemplo: `dbz3_recomp.10.cpp:18836-18854`). Hay unas 80 funciones de este tipo en `0x821D2778..0x821D7D18`.
- **Escrituras directas** a `REQ_CUR` fuera de la cola: `sub_821D3628` escribe 19, 17 y 8 (`14.cpp:19199-19378`). `sub_821D4780` escribe 6 (`2.cpp:18817`). `sub_821D6DE0` y `sub_821D7D18` escriben -1 (`4.cpp:19129`, `11.cpp:19457`).

Consecuencia: escribir en un solo punto no basta. Hay que **leer** las direcciones cada fotograma.

### 1.2 Valores de `REQ_CUR` conocidos

| Valor | Quién lo pide | Qué pasa | Estado |
|---|---|---|---|
| -1 | `sub_821D6DE0`, `sub_821D7D18` | Libre | [C] |
| **25** | `sub_82177300` (`7.cpp:13501-13504`), llamada por `sub_82177918` (`3.cpp:13691`) y `sub_82177B90` (`15.cpp:13415`) | `sub_821D7400` crea el duelo con `CFG` (ver §3). Es la confirmación del select del duelo | [C] en el camino; [I] en el nombre "select" |
| 23 | `sub_8216C5B8` (27.cpp:12947), `sub_8216DB58` (26.cpp:12951), `sub_8216E878` (0.cpp:13502). Lo consume `sub_821D7400` (22.cpp:19016-19020) y llama a `sub_821728A8` (22.cpp:13258) | Reserva memoria y crea estructuras (no identificado) | [C] en el camino; significado [N] |
| 43 | `sub_821D9590` (14.cpp:19554), llamada por `sub_821D9838(1)` (9.cpp:18683-18692) | Transición tras crear el duelo. Probablemente carga del combate | [I] |
| 5, 6, 8, 9, 10, 11, 12, 14, 15, 17, 18, 19, 20, 21, 24, 26, 27 | Varias funciones de menú, por ejemplo `sub_82147738` (5), `sub_82160A70` (12), `sub_82173370` (24), `sub_82178C10` (27) | Pantallas de menú | [C] en el valor; **nombre de pantalla [N]** |

Nota: `sub_821D7280` (`33.cpp:18994`) también mira `REQ_CUR`. Con 23 llama a `sub_821D3D80`. Con 25 marca varios bytes de estado y llama a `sub_821006F8`. Esto puede ser la preparación posterior al duelo. [I]

### 1.3 Cómo leer la pantalla cada fotograma

Propuesta de lectura, sin modificar nada:

```
REQ_CUR  = u32 @ 0x8247A990   (-1 = libre; 25 = duelo; 23; 43; ...)
REQ_NEXT = u32 @ 0x8247A98C
REQ_ARG  = u32 @ 0x8247A988
REQ_FLAG = u32 @ 0x8247A994
UI_PAGE  = u8  @ 0x824BA111   (1..6 vistos)
DUEL_PTR = u32 @ 0x823D6CF4   (no 0 = hay duelo creado o en curso) [I]
```

- El menú de Duelo (1P vs COM, 1P vs 2P, COM vs COM) **no está mapeado**. Hay que medir qué valores de `REQ_CUR`/`UI_PAGE` aparecen al entrar en cada opción. [N]
- "En combate" y "resultados": no localizados. [N]
- Lugar del hook por fotograma: no hay ninguno localizado con este nombre en `src`. `InGameMenu::OnDraw` (`src\ingame\menu.cpp:14`) se llama cada fotograma a través de ImGui. Leer memoria del invitado desde ahí necesita el mismo cuidado de hilos que usan los demás hooks. [I]

## 2. Modo 1P vs 2P y configuración del select

### 2.1 Bloque `CFG` (384 bytes, `0x8245EFC0`) [C salvo indicación]

| Offset en `CFG` | Tipo | Significado | Evidencia |
|---|---|---|---|
| `+16` | 80 bytes | Registro del jugador 1 | `src\roster_ext.cpp:1058-1062` |
| `+32` | u16 | **ID de personaje P1** | `src\roster_ext.cpp:1072`, `1079` |
| `+96` | 80 bytes | Registro del jugador 2 | misma estructura, `+16+80` |
| `+112` | u16 | **ID de personaje P2** | `src\roster_ext.cpp:1072` (`sel+32+80k`, k=1) |
| `+176`, `+256` | 80 bytes | Registros de jugador 3 y 4 | `roster_ext.cpp:1078` (k<4) |
| `+336` | u16 | **Número de luchadores**. 0 o negativo = 2 | No lo lee `sub_820FE668`. Lo leen `sub_820FE3D8` (`16.cpp:6796-6804`) y `sub_820FC9B0` (`9.cpp:6678-6693`, que lo copia a `D+35096`) [C]/[I] |
| `+338` | u16 | **Modo** (ver §2.3) | `5.cpp:7014-7034` [C] |
| `+340` | s16 | Parámetro de la tabla de escenario (modos 1, 3, 5) | `5.cpp:7039` [C] |
| `+342` | u16 | Resultado de la tabla (en `D+2022`). Ver §2.3 | `5.cpp:7045-7047` [C] |
| `+348` | u32 | Tiempo. Si es mayor que 0 se multiplica por 60 | `5.cpp:7050-7058` [C]; unidad [I] |
| `+352` | u32 | Vale -1 si `+388` < 1000 (uso [N]) | `15.cpp:13384-13412` [C] |
| `+388` | u16 | Comparado con 1000 en `sub_82177B90` (uso [N]) | `15.cpp:13394-13401` [C] |

Hay un bloque de 80 bytes por jugador. Dentro de cada uno, el ID está en `+16` y `+52` es u16 (`roster_ext.cpp:1097`, `c96+80`). Los bytes `+18..+31` y `+54..+67` no están caracterizados. [N]

### 2.2 Cómo se llena `CFG` y se pasa al combate

1. El select escribe los IDs en `CFG+32` y `CFG+112`. La función que escribe **no está localizada** [N]. `src\select_ext.cpp:437-457` guarda la elección en `0x824BA12F`, y eso es otro registro.
2. La confirmación, `sub_82177B90` o `sub_82177918`, llama a `sub_82177300`. Esa función pide la pantalla 25 y llama a `sub_821D27C8(10)`, que pone `REQ_FLAG = 1` (`7.cpp:13501-13511`) [C].
3. `sub_821D7400` (`22.cpp:19021-19037`) ve 25 y llama a `sub_820FE668(CFG, 0)` [C].
4. `sub_820FE668` (`5.cpp:6912-7373`) copia 384 bytes de `CFG` a `D+1680` (`5.cpp:6957-6964`), lee el modo y calcula el resultado de tabla (ver §2.3). Después llama a `sub_820FC9B0(0)` y termina en `sub_821D27F8`.
5. Por cada jugador, `sub_821007E8` hace la inicialización. Está hookeada en `src\roster_ext.cpp:1088-1102`, que ajusta IDs y datos de Goku para IDs cortados. [C]
6. Tras `sub_820FE668`, `sub_821D7400` llama a `sub_821D9838(1)` y se pide la pantalla 43 (`sub_821D9590`). [C]/[I]

Lo que el select escribe en `CFG` y en qué orden no está confirmado [N]. Pero sí sabemos que `CFG` es el objeto que pasa a `sub_820FE668`.

### 2.3 Modo `+338` y resultado `+342` [C código, [I] significado]

Lectura de `sub_820FE668` (`5.cpp:7014-7070`):

| Modo (`+338`) | Qué hace | Evidencia |
|---|---|---|
| 1, 3, 5 | `sub_820F3C98(+340, 0)` → resultado en `+342`. Con modo 5 además `sub_820FE3D8(D+1680)` | `5.cpp:7016-7048`, `7061-7070` |
| 2 | Lee bytes `0x824BA12A` y `0x824BA12B` (= `UI_STATE+26/+27`), llama a `sub_820F3CD8(b1, b2)` → `+342` | `5.cpp:7297-7308` |
| 4 | Lee el byte `r4` que se pasa a `sub_820FE668` (en `D+35574`), llama a `sub_820F3D30` → `+342` | `5.cpp:7285-7294` |
| otro | No cambia `+342` | `5.cpp:7034` |

- `sub_820F3C98`, `sub_820F3CD8` y `sub_820F3D30` calculan un índice y llaman a `sub_820F3550` (`28.cpp:6354`). Este lee un entero de longitud variable de hasta 4 bytes (formato [I]). La base de la tabla de `sub_820F3C98` es `0x8201EA68` (`0x82020000-5528`) [I].
- **Interpretación [I]**: `+342` es un índice de escenario u otro valor de reglas que sale de tabla. No está confirmado. El histórico de agentes menciona un crash al "elegir Stage en Duelo", lo que apoya que existe una elección de escenario (`docs\01_estructura\HISTORICO_AGENTS.md:3229`).
- `sub_820FE3D8` (`16.cpp:6782`) lee `+336` como número de luchadores (0 o menos = 2) y rellena datos por jugador. Hay un tope de ID de 595 (`16.cpp:6878`). [C]/[I]

### 2.4 1P vs 2P: no localizado [N]

> **Estado 2026-10-11:** en la práctica el anfitrión elige 1P vs 2P con el piloto automático (`netplay_hooks.cpp:56-68`, pantallas fijas por tiempo) y funciona en las pruebas. El bloque `CFG+336/338` sigue sin confirmar, así que la causa exacta no está resuelta. El piloto depende de tiempos fijos: si el Duelo tarda más, pulsa en el sitio equivocado.

- Ningún escritor de `CFG+336` o `CFG+338` se ha encontrado con el filtro de base usado. Se probó con las bases `0x8245xxxx` habituales y no apareció ninguna escritura u16 en esos offsets.
- Pistas: `sub_820FFA80` (`43.cpp:7016-7710`) construye un `CFG` local en la pila (`r1+80`) con modo **3** (`r1+418`, `43.cpp:7230`) y cuenta **0** (`r1+416`, `43.cpp:7216`). Es otra vía de creación de duelo. Un tercer llamador, `sub_82156CE8` (`38.cpp:11790`), pasa otro objeto cuyos campos `+16` y `+96` son floats. [C]
- Propuesta de medición (solo lectura, sin tocar el juego): hookear `sub_820FE668` (ya hookeado en `roster_ext.cpp:1069`) y volcar `CFG` completo (384 bytes más `+388..+391`) para cada opción del submenú de Duelo. Después comparar. Los campos que cambien son el 1P/2P o el modo COM. [I]

## 3. Arranque directo de un combate 1P vs 2P

### 3.1 Qué se puede hacer [I]

Opción A (la de menor riesgo, propuesta):

1. Preparar un bloque de 384 bytes válido. Hay una plantilla completa en la pila de `sub_820FFA80` (ver §2.4), que es un duelo por defecto con modo 3 y cuenta 0 (`43.cpp:7216-7230`).
2. Poner los IDs: `CFG+32` = ID P1, `CFG+112` = ID P2 (u16).
3. Poner la cuenta `CFG+336` = 2 y el modo `CFG+338` = 1 o 3 (usa la tabla de `sub_820F3C98`). Parámetro `CFG+340` = el que corresponda al escenario deseado. Tiempo `CFG+348` (u32).
4. Pedir la pantalla 25 con `sub_821D2778(25, 0)` (la llamada que hace `sub_82177300`, `7.cpp:13501-13504`). Si `REQ_CUR` está ocupada, la petición va a `REQ_NEXT`. Mirar `REQ_CUR` antes.
5. Poner `REQ_FLAG = 1` con `sub_821D27C8(10)` si se quiere replicar `sub_82177300`.
6. El juego hace el resto con su propio nodo de estado (`sub_821D7400`).

Riesgo: el nodo de estado `sub_821D7400` tiene que estar activo. Está instalado como función de actualización en un objeto (`+28`, `22.cpp:19013-19014`), pero no se ha localizado qué objeto lo ejecuta. Si no está activo, la petición se queda en `REQ_CUR` sin consumirse. [N]

Opción B (punto más tardío y fiable): **después** de `sub_820FE668`. El hook de `src\roster_ext.cpp:1069` ya corre ahí. En ese momento `D` está completo y los dos jugadores tienen su registro inicializado por `sub_821007E8`. Es el punto donde ambos quedan en el mismo estado. [I]

No recomiendo llamar a `sub_820FE668` directamente sin el nodo de estado. Tiene efectos secundarios: crea `D` si no existe, llama a `sub_820FC9B0` y a `sub_821D27F8`. [I]

### 3.2 Cápsulas, traje y tiempo

- **Cápsulas "Custom"**: 7 × s16 por casilla del select, en `0x824BA110+4928` (`docs\03_formatos\CAPSULAS_B3.md:55`). [C en docs]
- **Cápsulas "Normal"**: no localizadas. [N]
- **Traje**: `kCostumeCounts` (`src\roster_ext.cpp:146`) indica trajes elegibles por casilla. El campo del traje elegido en `CFG` no está localizado. [N]
- **Tiempo**: `CFG+348` (u32), multiplicado por 60 cuando es mayor que 0. [C]
- **Dificultad**: no localizada. [N]

## 4. Textos del menú y opción "Online"

### 4.1 Qué está localizado [C]

- Las páginas del menú del select están en `data_eng.afs`: la entrada 0 es un composite con 5 secciones `#ACA` (personajes, stages, modos) (`docs\03_formatos\AUDITORIA_DATA_CMN.md:110-119`).
- `docs\03_formatos\CAPSULAS_B3.md:37` habla de **nombres de personajes y cápsulas dentro de los menús** (`data_usi` 2663 y 2684). No habla de las entradas del menú principal. Este punto del estudio 09 está mal citado. [C]
- La selección de opción se hace en código. `sub_821D3628` (`14.cpp:19087`) recibe el índice de opción en `r3`. Lo llama `sub_820FEBA8` (`22.cpp:6838`) con el valor de `[r11+27872]`. Según la página `UI_PAGE`, el índice pide una pantalla u otra.

### 4.2 Qué no está localizado [N]

- Las entradas del menú principal (Dragon Universe, Duel, World Tournament, Practice, Edit Skills, Options). No se ha encontrado su índice de página `#ACA` ni su texto.
- Formato `#ACA` decodificado: no en este estudio.

### 4.3 Opciones para "Online"

| Opción | Esfuerzo | Riesgo | Comentario |
|---|---|---|---|
| Panel propio en ImGui sobre el submenú de Duelo | Bajo | Bajo | `InGameMenu` ya es un `ImGuiDialog` con F10 (`src\ingame\menu.cpp:14-40`). Se mostraría cuando `REQ_CUR`/`UI_PAGE` indiquen el submenú de Duelo (condición a medir) |
| Entrada nueva dentro del menú del juego | Alto | Alto | Cambiar número de opciones y despacho de `sub_821D3628`, texto `#ACA`, y registrar cualquier función nueva. Ver el aviso de crash por funciones plegadas en `docs\01_estructura\HISTORICO_RELEASES.md:3023-3078` |

Recomendación: panel propio. Es lo que plantea la pregunta 4 y es el camino de menor riesgo. [I]

## 5. Discrepancias y riesgos

1. **Misma dirección, dos nombres**: `src\roster_ext.cpp:146` (`kCostumeCounts`) y `src\roster_trace.cpp:38` (`kAvailTable`) usan `0x8245F150`. Uno de los dos está mal. [C]
2. **Cita de menús en el estudio 09**: `CAPSULAS_B3.md:37` no es la fuente de los menús principales (ver §4.1). [C]
3. **`kSelectGlobal` (`0x8245F190`)**: está a `CFG+464`, fuera del bloque de 384 bytes (`src\select_ext.cpp:120`). [C]
4. **Versión EU**: los valores de `0x8247A990` y `0x823D6CF4` solo están confirmados en US. Los hooks EU usan nombres `dbz3eu_sub_*` (`roster_ext.cpp:1069`), pero no se han comprobado las direcciones EU de esta cola. [N]

## 6. Pendiente de medir (sin tocar el juego)

> **Estado 2026-10-11:** el punto 1 (1P/2P) está resuelto en la práctica con el piloto de `netplay_hooks.cpp`; lo que sigue abierto es confirmar `CFG+336/338`.

1. Al entrar en `sub_820FE668` (ya hookeado en `roster_ext.cpp:1069`), volcar `CFG` completo (384 bytes más `+388..+391`) para cada opción del submenú de Duelo. Comparar las dumps para encontrar 1P/2P y el modo COM. [I]
2. Con cada opción del submenú de Duelo, anotar `REQ_CUR`, `REQ_NEXT`, `REQ_ARG`, `REQ_FLAG` y `UI_PAGE`. Así se obtiene la tabla valor -> pantalla. [I]
3. Confirmar qué objeto ejecuta `sub_821D7400` (el que tiene `+28` = `sub_821D7280`). [N]
4. Identificar el significado del resultado `+342` y de `+352`/`+388`. [N]
5. Confirmar las direcciones EU de la cola y del bloque antes de cualquier hook. [N]
