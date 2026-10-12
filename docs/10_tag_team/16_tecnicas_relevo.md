# Estudio 16: técnicas, auras y cápsulas al cambiar de luchador en batalla

Fecha: 2026-10-10. Solo lectura de `generated\dbz3_recomp.N.cpp`, `out\analysis\guest_image\dbz3_us_image.bin` (imagen de memoria del juego, 0x82000000 + offset, big-endian) y `src\tagteam.cpp`. No se ha editado código ni ejecutado el juego.
Complementa los docs 11, 12 y 14.

Convención: `N.cpp:L` = `generated\dbz3_recomp.N.cpp:L`.
- **[C]** = comprobado en el código recompilado o en la imagen de memoria.
- **[I]** = inferido por uso, nombre u orden.
- **[N]** = no confirmado.

Bloques:
- `cfg` = `*(u32*)0x823D6CF4` [C]. `X_k` = `cfg + 2096 + 8240·k` (k = 0..1 en batalla de 2) [C].
- `sel` = `cfg + 1680`. Registro de selección del jugador k: `rec_k = sel + 16 + 80·k`; ID en `sel + 32 + 80·k` [C].
- `G2` = `*(0x82333994)` (bloque de técnicas). Por jugador: `G2 + 1152·k`. Dentro, dos ranuras de 544 B: ranura A en `+80`, ranura B en `+624` (624 − 80 = 544) [C].
- `G` = `*(0x82322AEC)` (bloque de auras). Aura del jugador k en `G + 420·k` [C].

---

## 0. Resumen corto

1. **Las funciones de cápsulas (sub_82113818, sub_82113870, sub_82112260) no cargan nada de disco.** Solo leen el registro de selección y la tabla de personajes y rellenan campos de `X`. Se pueden volver a llamar para un solo `X` después de actualizar `rec` (X+7412) y el registro de 372 B (X+7416). [C/I]
2. **Las técnicas (BSP) tienen dos ranuras por jugador** (G2+80 y G2+624). La ranura A la rellena siempre el arranque de combate desde el ID de la selección. La ranura B la rellena un "trabajo" de tipo callback (`sub_82128B08` o `sub_82128B50`) con payload {ID, k}. **Quién encola ese trabajo: [N].** El mod actual no toca ninguna de las dos ranuras (src\tagteam.cpp, FinishSwap). Esa es la causa probable del fallo de técnicas. [C/I]
3. **El aura no está en X.** `sub_820B8F78(k, ID)` carga el archivo y guarda el handle en `G + 420·k`. El procesado del árbol lo hace `sub_820B8820(k)` (aura) con el mismo esquema que las técnicas. El arranque entero pasa por `sub_820CD5D8` → `sub_820BA578`. [C]
4. Hay además un bloque de estado de auras (`*(0x82322848)`, no `0x823229C8` como se pedía) con contador de jugadores en +258 y flag en +256. [C]

---

## 1. Qué hace `sub_82109BA0` y de quién es r3 (21.cpp:7374) [C]

- Entrada: `r3 = sel` (no X). Dentro: `r31 = r3 + 36`, así que el ID se lee en `r31 − 4 = sel + 32` [C].
- Por cada jugador k (bucle de 4, `r27 = 4`, pero solo k < número activo lo usa el llamador):
  - `X+7420 = ID` (`stw r11,0(r30)`, `21.cpp:7412`). Excepciones: ID 78, 79, 84 → 35 (`21.cpp:7511`).
  - `X+7412 = rec_k` (`stw r10,-8(r30)`) [C].
  - `X+7416 = char372 + 372·ID` (`stw r11,-4(r30)`, base 0x82329CF0) [C].
  - `X+7424 = rec_k+18` (u16 en palabra) [C].
  - Llamadas por jugador, con **r3 = X_k** (`mr r3,r29`, `r29 = X_k`) [C]:
    - `sub_82113818(X)` — `21.cpp:7529` → define en `26.cpp:7964`.
    - `sub_82113870(X)` — `21.cpp:7534` → define en `18.cpp:8041`.
    - `sub_82112260(X)` — `21.cpp:7539` → define en `28.cpp:8056`.
  - Después de los 4 jugadores: `sub_82101240` (`21.cpp:7552`, una sola vez).

**Confirmado: r3 = X.** `r29 = r30 − 7420` y `r30 = cfg + 9516` al inicio, así que `r29 = cfg + 2096 = X_0`.

### 1.1 Qué construye cada una

**sub_82113818(X)** (`26.cpp:7964`). Hoja (sin llamadas). Lee `char372 = X+7416`. Para las 8 formas f: lee el ID de forma `char372+214+20f` y el índice `char372+216+20f`. Con `(ID·12 + índice)·8` indexa la tabla de personajes `char96` (base 0x8234ABB8, 96 B por ID) y guarda un byte (s8) en **`X+7428+f`** [C].
- Cálculo: `ctx.r9 = ID*12 + idx`, `rlwinm 3` (×8), lectura u16 en `0x8234ABC8 + …`. Eso es `char96[ID]` entrada `idx`.
- Qué significa el byte: [I] valor por forma de ki/aura o similar (no se ha identificado el campo de char96 exacto).

**sub_82113870(X)** (`18.cpp:8041`). Hoja. Escribe **`X+7436+f`** (traje/variante por forma), que `sub_82100A10` lee como traje [C].
- Valor base: `rec_k+18`. Luego se pone a 0 si el ID de forma no es 91 y el valor supera `char96[ID].byte@+4`, o si el byte de la forma es 8. [C]

**sub_82112260(X)** (`28.cpp:8056`). Llama a `sub_822F61C0(X+8125, 0, 31)` (memset, [I]). Hace:
- `X+8156 = 0`, `X+8160 = 0`, `X+8172 = 0` (u16) [C].
- `X+8116 = 1.0f` (float) [C]. `X+8120 = (float)(rec+76)` (i16 de la selección) [C]. `X+8124 = 0` [C].
- Lista de cápsulas: `rec+52` = número de cápsulas, `rec+54 + 2i` = ID de cápsula i (u16). Por cada cápsula > 0 busca su entrada en el catálogo #SKC: `0x8237560C + 40·ID` [C]. El byte `+21` de esa entrada es la categoría `t`; cuenta `X+8125+t` (u8 +1) [C].
- Para categorías concretas (t = 30, 11, 7, 6, 5…) escribe flags en `X+8131, +8132, +8136, +8155` [C]. Significado: [I] (probablemente tipos de cápsula con efecto de ki/ultimate).
- `X+8156+f` = 1 si el campo u16 `char372+212+20f` coincide con alguna cápsula de la lista (f = 0..7) [C].

**¿Seguro volver a llamarlas para un solo X?** [I]: sí, con dos condiciones:
- Hay que actualizar antes `rec+52`, `rec+54..` y `X+7416` (si no, lee datos viejos).
- No deben estar leyéndose en ese momento. En la práctica, el mod ya lo hace en `FinishSwap` (src\tagteam.cpp:219-221) después de escribir `sel+16` y copiar las cápsulas desde `char96+80` [C].

### 1.2 Qué pasa con la lista de cápsulas en el mod

`FinishSwap` escribe `sel+52` y `sel+54..67` desde `char96[ID]+80` y `+82` (src\tagteam.cpp:205-210) [C]. Es el mismo dato que hace `sub_821007E8` (20.cpp:6982) en la selección. Por eso las cápsulas **sí** se refrescan. Lo que no se refresca son las técnicas y auras (puntos 2 y 3).

---

## 2. Técnicas (BSP): quién carga qué [C salvo indicado]

### 2.1 Los dos cargadores de técnicas en el arranque

**a) `sub_82128338` (1.cpp:8785)**, llamado por `sub_820CD5D8` (3.cpp:4222). Es el arranque de técnicas. Para los 2 jugadores (`r27` de 0 a 16480 = 2·8240):
- `r26 = sel + 32 + 80k` (ID de la selección) [C].
- **Ranura A (G2+80):** `r3 = *(0x82333208 + 4·ID_sel)` (tabla BSP). Si ≠ −1: `sub_82083968(bsp, 0, 1)` → handle; `stw r3, 80(G2+1152k)`, `stw 0, 100(…)` [C]. **Usa el ID de la selección, no X+7420.**
- Una línea de estado: `sthx 0` a `0x823D76EE + 1152k`, y al final `sthx 1` (flag) [C]. Significado: [I], probablemente "técnica del jugador k lista".
- **Ranura B (G2+624):** solo si `X_k+7420 == 35` y el ID de la selección no es 35: `sub_82128040(X_k, ID_sel)` [C]. Es el caso especial de los IDs 78, 79, 84 (que el juego mapea a 35).
- Al inicio, `li r3,520` + `sub_82083968` guarda en `G2+2460` (memoria de 520 B). Esto se repite en cada llamada (posible fuga si se llama de nuevo) [C].

**b) Los dos callbacks de trabajo (ranura B):**
- **`sub_82128B08` (16.cpp:9180).** Lee el payload `job+44` → `{ID en +0, k en +4}`. Toma `X = *(cfg + 35188 + 4·k)` y llama a **`sub_82128040(X, ID)`** (16.cpp:9207). Carga `0x82333208[ID]` en **G2+1152k+624** y pone `+644 = 0` [C]. Devuelve 1.
- **`sub_82128B50` (8.cpp, ~8790-8806).** Mismo payload. Llama a **`sub_821280A8`** (7.cpp:9219), que carga `0x82333404[ID]` (otra tabla, `addi r8,r11,13220`) en **G2+1152k+624** y `+644 = 0` [C].
- Los dos callbacks **están en la imagen de memoria**: son la entrada 4 (0x82128B08) y 5 (0x82128B50) de una tabla de 8 B `{fn, flags}` en **0x8206CC30** (flags 0x40001103). Cada entrada es `(fn, flags)`. [C en imagen]. Los flags tienen un 0x40000000 (clase de trabajo), que es el mismo bit que usa `sub_82092A90(…, 0x40000000, …)` en `sub_8210E4F8` (33.cpp:7870). [I]
- También aparecen en otra tabla de punteros a función en **0x823321A0** (0x82128B08 en 0x823321B4, 0x82128B50 en 0x823321C0) [C en imagen]. Quién la lee: [N].

**Quién encola el trabajo (callback 82128B08 / 82128B50): [N].** No hay ninguna instrucción que cargue `0x82128B08` como constante (`lis`/`addi`). No hay `bl` directo a estos callbacks. Se llega por puntero desde datos. Se buscó:
- En el código recompilado: no hay referencias a `-29944` ni a `35592` ni a `0x82128B08` como constante.
- En la imagen: los únicos bytes `82 12 8B 08` están en 0x8206CC50 y 0x823321B4.
- Quién lee la tabla 0x8206CC30 (`lis -32250` + `addi -13264/-13232`): no se encontró en el código recompilado con el patrón simple [N].

### 2.2 Qué procesa las ranuras (de handle crudo a estructura usable)

Cada ranura pasa por dos etapas:
1. **Cargador** (`sub_82128338` / `sub_82128040` / `sub_821280A8`): deja el handle crudo de `sub_82083968` en `+80` o `+624` [C].
2. **Procesador** (convierte el handle en estructura de trabajo y libera el nodo crudo):
   - Ranura A (+80): `sub_82126490(k, i=0)` (34.cpp:9152). Entra con `r3 = k`, `r4 = i`; la ranura es `G2 + 1152k + 544i + 80`. Copia campos del nodo (`node+192`, `node+208`) a `+0/+4/+8` de la ranura, llama a `sub_82083d08` (liberar nodo crudo) y construye la lista en `+288` con `sub_82126430` y `sub_8208dee0` [C]. Llamadores: `sub_82126F80` (11.cpp:9289) y `sub_82127390` (31.cpp:9269/9276) [C].
   - Ranura B (+624): `sub_82126FD0(X)` (15.cpp:9136 lee 624). Mismo esquema; `X` entra en `r3`; llamador: `sub_82128C28` (13.cpp:9353) [C].
   - Consecuencia [I]: el procesador **consume** el handle. Llamarlo otra vez sobre una ranura ya procesada sería incorrecto. Hay que liberar la estructura antigua antes de cargar la nueva. No se ha identificado la función que la libera [N].

### 2.3 Flags que leen el juego y el mod

- `G2 + 644` ≠ 0 → "pendiente", y `G2 + 624` con nodo `+4 == 3` → "listo" (`sub_82126F30`, 18.cpp:9179). Devuelve 1 en uno de esos casos [C/I].
- `G2 + 2330` (u16): la pone a 1/0 `sub_82129058` (16.cpp:9269) según el payload, y la pone a 0 `sub_82126AF8` (13.cpp:9268) y `sub_82126B78` (22.cpp:9148) junto con la liberación del trabajo en `G2+1152k+52` [C/I].
- `G2 + 2324/2326`: estado general de la batalla (`sub_82127A60`, 30.cpp:9338) [I].

---

## 3. Aura por ID [C salvo indicado]

### 3.1 Carga

- **`sub_820B8F78(k, ID)`** (21.cpp:2820-2847). Lee `aura[ID]` (u32) en **0x82322950 + 4·ID** (tabla de auras). Si ≠ −1: `sub_82083968(aura, 0, 1)` y guarda el handle en **`G + 420·k`** (`stwx r3,r30,r31`, `r30 = 420·k`, `r31 = *(0x82322AEC)`). No libera el handle anterior [C].
- Llamadores:
  - `sub_820BA578(sel)` (10.cpp:3043, bucle en 3105). **Es el arranque de auras.** Hace:
    - `G+968 = sub_82083968(12, 0, 1)` (nodo de 12 B, una vez por llamada → fuga pequeña) [C].
    - `*(0x82322848) + 258 = 2` (contador de jugadores fijo a 2) y al final `+256 = 0` [C].
    - Bucle `i = 0..1`: `sub_820B8F78(i, ID_sel_i)` con `ID_sel_i` = `u16 sel + 32 + 80i` [C].
  - `sub_821C0198` (29.cpp:17703). Camino de otra pantalla. [C]

**Corrección a la pregunta 3:** el estado de auras es `*(0x82322848)` (`+256` / `+258`), **no** `0x823229C8`. La dirección `0x823229C8` no aparece como constante en el código ni en la imagen [C].

### 3.2 Proceso del árbol (después de cargar)

- **`sub_820B8820(k)`** (23.cpp:3425 es la llamada desde `sub_820B9A18`). Igual que la ranura A de técnicas: lee `raw = G + 420·k`, copia campos, llama a `sub_82083d08` (liberar), `sub_820b0cf8/cc8` (hijos) y `sub_82126430`/`sub_8208dee0` [C].
- Cadena de llamadores del procesado:
  - `sub_820CD618` (35.cpp:4175) → `sub_820BA608` → `sub_820B9A18` (23.cpp:3279) → `sub_820B8820`. [C]
  - `sub_820D4128` (28.cpp:4645, llamado por `sub_820FEA98` en 30.cpp:6976) → `sub_820BADF8` (11.cpp:3302) → `sub_820B9B40` y `sub_820B9D08`. [C]
- Quién procesa el aura de un jugador concreto en cada momento: [I] `sub_820B9A18` recorre los jugadores. No está confirmado.
- Los flags `+256/+258` que se pidieron en la pregunta aparecen en el bloque `*(0x82322848)`, no en el nodo del aura. Dentro del nodo, `+256/+258` son variables de pila (`sth …,256(r1)`) [C].

### 3.3 Liberación

- No hay una función que libere `G+420·k` directamente [N]. `sub_820B8820` libera el nodo crudo con `sub_82083d08` después de copiarlo [C].

---

## 4. Otras cosas que se construyen por jugador al empezar desde el ID [C/I]

| Recurso | Dónde | Quién | Estado |
|---|---|---|---|
| Costume por forma `X+7436+f` | `sub_82113870` | cápsulas/traje | [C] |
| Valor por forma `X+7428+f` (de `char96[ID].forms[idx]`) | `sub_82113818` | ki/aura (no identificado) | [C fuente] / [I] uso |
| Cápsulas activas, ki (`X+8116` = 1.0, `X+8120`) | `sub_82112260` | cápsulas | [C] / [I] significado |
| Contadores por categoría de cápsula `X+8125+t` y flags `X+8131/8136/8155` | `sub_82112260` | ultimates/especiales [I] | [C] / [I] |
| Cámara `#AMB` hijos `X+7900/7908/7912` | `sub_8210FDE0` (27.cpp:7784) | golpes y cámara de especiales | [C]; el mod ya lo llama en FinishSwap (src\tagteam.cpp:222) |
| Ranura de técnica A `G2+80` | `sub_82128338` (desde ID de la selección) | técnicas | [C] |
| Ranura de técnica B `G2+624` | `sub_82128040` / `sub_821280A8` (vía callback) | técnicas especiales | [C] |
| Aura `G+420k` | `sub_820B8F78` (desde `sel` por `sub_820BA578`) | efecto de aura | [C] |
| Estado de auras `*(0x82322848)+256/+258` | `sub_820BA578` | procesado de auras | [C] |

**Ultimate cinematic:** [N]. No se ha encontrado en este análisis una estructura ligada al ID que se procese en el arranque. El grupo `X+8125..8155` (sub_82112260) es lo más cercano.

---

## 5. Secuencia propuesta para cambiar el luchador k (NO PROBADA, NO IMPLEMENTADA)

Solo análisis. Los pasos están en orden. [I]

**Paso 0. Estado de partida.** `sel+16+80k`, `sel+32+80k` (ID nuevo), `sel+52` y `sel+54..` (cápsulas) ya escritos, igual que en FinishSwap. [C]

**Paso 1. Cápsulas y registro (ya lo hace el mod).** `sub_82113818(X)`, `sub_82113870(X)`, `sub_82112260(X)`. Seguros porque son hojas o solo escriben campos de `X`. [C/I]

**Paso 2. Aura del jugador k.**
- Liberar el handle viejo de `G+420k` (no hay función confirmada; alternativa: dejar la fuga).
- Llamar `sub_820B8F78(k, ID_nuevo)` → `G+420k` = handle crudo. [C]
- Esperar a que el nodo esté listo (flag `+1` del nodo en 0; ver `Loaded` en src\tagteam.cpp:228, que ya usa esa regla para voz/cámara). [I]
- Procesar con `sub_820B8820(k)` (o el camino `sub_820B9A18`). [I, riesgo: no confirmado que `sub_820B8820` acepte solo k]

**Paso 3. Ranura A de técnicas (G2+80).**
- Liberar el procesado viejo (no identificado) [N].
- `G2+1152k+80` = `sub_82083968(0x82333208[ID_nuevo], 0, 1)`, igual que hace `sub_82128338` por jugador. [C]
- Procesar con `sub_82126490(k, 0)`. [C]

**Paso 4. Ranura B de técnicas (G2+624).**
- Lo más seguro: `sub_82128040(X_k, ID_nuevo)` (carga) y luego `sub_82126FD0(X_k)` (procesado). [I]
- Si el ID nuevo no es 35, lo que enlazó el juego para el caso normal es el callback `sub_82128B08`/`sub_82128B50`. Hay que decidir cuál: la entrada del juego para el caso normal no está confirmada [N]. Por eso la propuesta usa la carga directa.

**Paso 5. Esperar.** Hasta que `+644 == 0`, el nodo de cada ranura esté en estado 3 (`sub_82126F30`), y el aura esté lista. Mismo criterio que `Loaded` (src\tagteam.cpp:228) con timeout. [I]

**Paso 6. Estado de la batalla.** Poner `G2+2330 = 0` (como `sub_82126AF8`) y, si hace falta, `G2+1152k+52 = 0` [C/I].

---

## 6. Riesgos y lo que falta

1. **Liberación de estructuras procesadas** de técnicas (+80/+624) y auras (G+420k). Sin ella, recargar deja fugas o punteros viejos. [N]
2. **Quién encola los callbacks 82128B08/82128B50** (tipo 4/5 de la tabla 0x8206CC30). Es el punto clave para saber cómo el juego recarga la técnica B en el caso normal. [N]
3. **`sub_820B8820` acepta solo k?** Se ha visto `mulli r10,r3,420` con `r3` como argumento, pero no se ha confirmado el resto de argumentos. [I]
4. **Significado del byte `X+7428+f`** y del `+8116`/`+8120` (ki). [I]
5. **Significado de `G2+2330`, `+2324`, `+2326`** (estado global). [I]
6. **Ultimate cinematic por ID** [N].
7. Tampoco se ha comprobado en juego nada de lo anterior.

---

## 7. Correcciones al doc 14 (14_recursos_por_id.md)

- **Corregido:** la técnica "caso normal" no va solo por callback. La ranura A (G2+80) la carga `sub_82128338` para **todos** los jugadores desde el ID de la selección. El callback solo rellena la ranura B (G2+624). [C]
- **Corregido:** la llamada a `sub_82128040` desde `sub_82128B08` está en 16.cpp:9207 (el doc decía 9209; es la misma llamada, dentro de la función). [C]
- **Ampliado:** existe un segundo callback de técnica, `sub_82128B50`, que usa la tabla 0x82333404 (vía `sub_821280A8`). [C]
- **Ampliado:** las técnicas y auras tienen fase de procesado (`sub_82126490`, `sub_82126FD0`, `sub_820B8820`) que convierte el handle crudo en estructura. [C]
- **Corregido:** el estado del aura no está en `0x823229C8`, sino en `*(0x82322848)` (+256/+258). [C]

---

## 8. Archivos mirados

- `generated\dbz3_recomp.21.cpp` (sub_82109BA0, sub_820B8F78)
- `generated\dbz3_recomp.26.cpp` (sub_82113818), `18.cpp` (sub_82113870), `28.cpp` (sub_82112260)
- `generated\dbz3_recomp.1.cpp` (sub_82128338), `29.cpp` (sub_82128040), `16.cpp` (sub_82128B08), `8.cpp` (sub_82128B50), `7.cpp` (sub_821280A8)
- `generated\dbz3_recomp.34.cpp` (sub_82126490), `15.cpp` (sub_82126FD0), `18.cpp` (sub_82126F30), `13.cpp` (sub_82126AF8), `22.cpp` (sub_82126B78), `30.cpp` (sub_82127A60)
- `generated\dbz3_recomp.10.cpp` (sub_820BA578), `3.cpp` (sub_820CD5D8), `23.cpp` (sub_820B9A18), `28.cpp` (sub_820D4128), `11.cpp` (sub_820BADF8)
- `out\analysis\guest_image\dbz3_us_image.bin` (tablas 0x82333xxx y 0x8206CC30)
- `src\tagteam.cpp` (StartSwap/FinishSwap, líneas 147-230)
