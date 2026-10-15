# 01 — Casillas de DU y datos de partida

Estado 2026-10-15. Leído en el código recompilado (US). Citas `G/` = `generated/`. [?] = sin medir en el juego.

## 1. Estructura de DU en memoria

Hay dos copias con la misma forma: la viva `0x8247A8E0` y la de la partida `0x824BA110` (`src/campaigns.cpp:413`).
Desplazamientos desde la base (`B`):

| Campo | Dirección viva | Qué es | Fuente |
|---|---|---|---|
| `B+208`, 128 B | `0x8247A9B0` | 512 marcas de historia de 2 bits (16 por u32). Las lee el evento 48. Se guardan. | `G/dbz3_recomp.24.cpp:13497`, `G/dbz3_recomp.5.cpp:18835` |
| `B+336 + 4624·k` | `0x8247AA30 + 4624·k` | **Bloque de la casilla k** (k = 0..37, casilla del select). 38 × 4624 = 0x2AE60 B; acaban justo en `0x824A5890`. | `G/dbz3_recomp.19.cpp:12038-12052` |
| `B+0x2AFB0`, u64 | `0x824A5890` | Bit k = la casilla k tiene historia empezada. Se pone en "Nueva historia" (`sub_821D28C0`), se mira al elegir (`sub_821D2930`) y se quita en `sub_8215BED0` (`sub_821D28F8`) [?: fin de la historia]. | `G/dbz3_recomp.37.cpp:18038`, `.33.cpp:18607`, `.13.cpp:18658` |
| `B+0x2AFB8`, u32 | `0x824A5898` | **Zeni**. Tope 990000 (`0xF1B30`). Lo suma el evento 18. | `G/dbz3_recomp.4.cpp:12411-12448` |
| `B+0x2AFC0`, u8 | `0x824A58A0` | Casilla de DU en juego. | `G/dbz3_recomp.15.cpp:17738` |
| `B+0x2AFE9`, 2048 B | `0x824A58C9` | Inventario de cápsulas (un byte de cantidad por ID). | `G/dbz3_recomp.24.cpp:12570-12590` |

**Consecuencia clave:** no hay sitio para una casilla 38 o mayor: la tabla de bloques termina donde empiezan la máscara u64, el zeni y el byte de casilla.

## 2. Bloque de una casilla (4624 B)

Desplazamientos desde el inicio del bloque. Lo inicializa `sub_8215C728(casilla, bloque)` ("Nueva historia", `G/dbz3_recomp.3.cpp:12254`).

| Desp. | Tipo | Qué es | Fuente |
|---|---|---|---|
| +0..+127 | 128 B | Cabecera; "Nueva historia" la pone a 0 [? uso]. | `.3.cpp:12271-12277` |
| +128..+4223 | 64 × 64 B | Marcas del mapa (ver `03_mapa_premios_codigos.md` §2). | `G/dbz3_recomp.18.cpp:17940-18022` |
| +4544 | 16 B | Posición en el mapa (vector) con +4560; `sub_82156B60` la restaura al volver de una escena con código < 2000. | `.4.cpp:12339-12357` |
| +4560 | u32 | Acompaña a la posición (zona o ángulo) [?]. | ídem |
| +4564 | u32 | **Palabra de escena guardada** (siguiente escena). "Nueva historia" pone aquí la intro: `sub_82156DC8(casilla)`. | `.3.cpp:12280-12287` |
| +4568 | u32 | Segunda palabra guardada (la lee el port) [? quién la escribe]. | `src/campaigns.cpp:345` |
| +4572 | u32 | 0 al empezar [?]. | `.3.cpp:12291` |
| +4576 | u8 | **Planeta del mapa**: 0 la Tierra, 1 Namek (si la intro es 102), 2 si la casilla es 34 (Broly). | `.3.cpp:12296-12323` |
| +4577 | u8 | Casilla (el ID del personaje sale de aquí con `sub_82156E88`). | `.3.cpp:12340`, `.27.cpp:17705-17713` |
| +4578 / +4579 | u8 | Modelo / traje en el mapa (0 al empezar). | `.3.cpp:12328-12332` |
| +4580 | u8 | 0 al empezar [?]. | `.3.cpp:12334` |
| +4581 | u8 | Evento 39 lo pone a 1: el botón de vista del mapa pasa de 2 a 3 modos [? radar]. | `.24.cpp:12545`, `.6.cpp:18239` |
| +4582 | u8 | Evento 40 lo pone a 1; la escena 102.7 (Vegeta llega a Namek) lo pone a 0. Elige los recursos del mapa 3853+2·v y 3854+2·v: **visor del scouter** [? por el nombre PAL `SCOUTER_VIEW_*`]. | `.24.cpp:12565`, `.19.cpp:12094-12114`, `.27.cpp:17853-17880` |
| +4583 | u8 | **Nivel** (tope 99). | `.0.cpp:11976`, `.4.cpp:12381-12394` |
| +4584..+4590 | 7 × u8 | Estadísticas repartidas al subir de nivel. | `.0.cpp:11974-11987`, `.5.cpp:12274-12294` |
| +4592..+4605 | 7 × s16 | Cápsulas equipadas (lista Custom de la casilla). "Nueva historia" les quita los 4 bits altos. | `.3.cpp:12344-12366`, `.24.cpp:12837` |
| +4608 | u32 | Experiencia. | `.0.cpp:11992` |
| +4612 | u8 | 7 al empezar [?]. | `.3.cpp:12375` |

## 3. Qué se guarda en el archivo de partida (`sub_821D2BD8`, `G/dbz3_recomp.5.cpp:18510`)

- **De las 38 casillas**: nivel, 7 estadísticas y experiencia (12 B cada una, archivo +1816, líneas 18801-18833) y las 7 cápsulas (14 B, archivo +2808, líneas 18773-18799). El nivel y las cápsulas de una casilla sin DU ya se guardan solos.
- **Bloque entero de 4624 B solo de 11 casillas**: 0, 2, 3, 4, 7, 10, 11, 12, 13, 17 y 34 (líneas 18844-18960). Es la máscara de la rueda (`0x4_00023C9D`).
- Globales: marcas de 2 bits (B+208), máscara u64 (B+0x2AFB0), zeni, byte de casilla e inventario (líneas 18963-19009).
- El port ya guarda aparte los bloques de las casillas de `[rueda]` (`historias_extra.bin`, `src/campaigns.cpp:429-474`).

## 4. Casilla ↔ personaje

- **Casilla → ID, fórmula fija** (`sub_82156E88`, `G/dbz3_recomp.4.cpp:11891`): casilla < 22 → igual; 22-25 → +5 (Freezer 22 → 27); ≥ 26 → +6. **No usa la tabla `0x82020618`.** La usan el personaje del mapa (`.27.cpp:17713`) y `sub_821C6BB8`, `sub_821CD098`, `sub_821D0548`. `sub_8215BED0` repite la fórmula en línea (`.13.cpp:12191-12210`).
- **ID → casilla**: `u16 0x82020668[ID]` (44 entradas). La rueda la usa para pasar los personajes desbloqueados (máscara por ID) a casillas (`G/dbz3_recomp.12.cpp:13418-13456`).
- Por la fórmula, una casilla 38 daría el ID 44, la primera plaza extra. Pero no hay bloque 38 ni entradas 38 en las tablas por casilla (§6).

## 5. Rueda de DU (`sub_8217D2D0`, `G/dbz3_recomp.12.cpp:13230`)

1. `r28` = personajes desbloqueados (u64 por ID, objeto+56; más objeto+8 con una bandera) (13394-13414).
2. Recorre 44 IDs: si el ID está desbloqueado y `0x82020668[ID] != 0xFFFF`, enciende el bit de su casilla en `r26` (13418-13456).
3. "Permitidas" (+32) = todo, salvo en el modo 1 (DU) o el modo 7 con +169 = 0 [? modo 7], donde vale `0x4_00023C9D` (13459-13486). En los modos 2-4 se suma el bit 38 (aleatorio).
4. `sub_8217AD68(objeto, r26)`: el gancho del port suma los bits de `[rueda]` a las dos máscaras (`src/campaigns.cpp:168-183`).

Una casilla se puede elegir en DU si su bit está en las permitidas y en las desbloqueadas.

## 6. Al confirmar en la rueda (`sub_821CA020`, `G/dbz3_recomp.7.cpp:18076`, y tras confirmar sobrescribir `sub_821C9DC8`, `.32.cpp:17895`)

- La casilla elegida es un u16 en `0x8245F140` (= `0x82456480 + 36032`) (18249-18261).
- Muestra nivel y experiencia para el siguiente nivel con la tabla por casilla `0x82374A20[casilla]` (un puntero a la curva de experiencia, indexada por nivel) (18144-18172).
- Objeto+85 = "Nueva historia" (1) o "Continuar" (0). Si es nueva y el bit de §1 ya está puesto, pide confirmar (18265-18316).
- `sub_821C2388(casilla)`: pone a 0 el estado del mapa `0x82479A60` (3632 B), apunta `+180` al bloque de la casilla y escribe el byte `0x824A58A0` (`.15.cpp:17633-17738`).
- Si es nueva (o "Continuar" sin historia) → `sub_8215C728` (§2).

## 7. Tablas del juego indexadas por casilla (todas de 38 entradas)

| Tabla | Forma | Uso |
|---|---|---|
| `sub_82156DC8` (en la pila) | 38 × código de intro | Escena con la que empieza la historia: `(código << 16) \| casilla` (`G/dbz3_recomp.20.cpp:11868-11964`). Valores en `05`. |
| `0x82374A20` | 38 × puntero | Curva de experiencia por casilla (`.7.cpp:18152-18158`). |
| `0x823742C8` | 38 × 5 × u32 | Archivo de vuelo en el mapa (k = traje si modelo < 3, si no 4) (`.27.cpp:17822-17842`). |
| `0x823745C0` | 38 × u32 | Archivo de animaciones del mapa (`.27.cpp:17729-17745`). |
| `0x82374C00` | 38 × 28 B | Datos del mapa por casilla (4 floats en +12..+24) que lee `sub_8215BED0` (`.13.cpp:12213-12251`) [?]. |
| `0x82020668` / `0x82020618` | u16 | ID ↔ casilla (sección de solo lectura). |
