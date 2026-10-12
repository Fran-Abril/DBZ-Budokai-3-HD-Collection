# Estudio 18: recargar técnicas y auras de un jugador en mitad de batalla

Fecha: 2026-10-10. Solo análisis estático de `generated\dbz3_recomp.N.cpp`. No se ha editado código ni ejecutado el juego.
Complementa el doc 16 (`16_tecnicas_relevo.md`). Donde el doc 16 y este no coinciden, manda este (se indica en la sección 9).

Convención: `N.cpp:L` = `generated\dbz3_recomp.N.cpp:L`. Las líneas son las reales del archivo.
- **[C]** = comprobado leyendo el código recompilado.
- **[I]** = inferido por uso, nombre u orden. Hay que probarlo.
- **[N]** = no confirmado; no se encontró en el código.

---

## 0. Resumen

1. **Liberar una técnica de un jugador ya existe y se puede llamar para uno solo:** `sub_821266A8(k, i)` (23.cpp:9391). `k` = 0 o 1 (no X). `i` = 0 (técnica A) o 1 (técnica B). [C]
2. **Procesar una técnica:** `sub_82126490(k, i)` (34.cpp:9132). Convierte el nodo crudo de la ranura en estructura de trabajo y **destruye el nodo crudo**. No mira si la ranura ya estaba procesada: procesar dos veces rompe la memoria. [C]
3. **Aura:** no hay función de liberación por jugador, pero `sub_820B8E40` (22.cpp:2998) hace lo mismo para los dos. Su cuerpo se puede replicar para un jugador (sección 3.3). El proceso sí es por jugador: `sub_820B8820(k)` (4.cpp:3006). [C]
4. **No se ha encontrado un contador de "efectos vivos" por jugador.** Lo más cercano son los punteros de las tablas globales `*(0x82333990)` y `*(0x82333998)` (que el juego rellena al procesar) y el trabajo de técnica `G2+1152k+52`. Recargar sin comprobar esto puede dejar un efecto apuntando a memoria liberada. [C dato / N uso]
5. **Las funciones "de batalla" no sirven para un jugador:** `sub_82127390`, `sub_820B9A18`, `sub_820B9B40`, `sub_82126A10`, `sub_820B8E40`, `sub_820CD640` y `sub_820FE9B0` recorren a los dos jugadores. No se deben llamar a mitad de batalla. [C]
6. Secuencia propuesta para el jugador k: sección 7 (liberar → cargar → esperar → procesar → reenlazar). **No probada.**

---

## 1. Mapa de memoria

| Qué | Dirección / fórmula | Fuente |
|---|---|---|
| cfg | `*(u32*)0x823D6CF4` | [C] (dato del doc 16) |
| X_k (registro del jugador) | `cfg + 2096 + 8240·k` | [C] |
| k dentro de X | `*(u32*)(X_k + 4944)` = k (0 o 1). Lo leen `sub_82126F30` (18.cpp:9176), `sub_82126FD0` (15.cpp:9133), `sub_82128040` (29.cpp:9267), `sub_82126F80` (11.cpp:9288) | [C] |
| ID del jugador | `u16 sel + 32 + 80·k` (sel = cfg+1680) | [C] (doc 16) |
| G2 (bloque de técnicas) | `*(u32*)0x82333994` | [C] |
| Bloque del jugador k | `G2 + 1152·k` | [C] |
| Ranura A (técnica normal) | `G2 + 1152·k + 80` | [C] |
| Ranura B (técnica especial) | `G2 + 1152·k + 624` (= ranura A con i=1: `544·i + 80`) | [C] (`sub_82126490` 34.cpp:9139-9141) |
| Bandera "B procesada" | `G2 + 1152·k + 644` (= ranura B +20) | [C] (15.cpp:9248, 29.cpp:9298) |
| Bandera "A procesada" | `G2 + 1152·k + 100` (= ranura A +20) | [C] (34.cpp:9426) |
| Trabajo de técnica (cancelable) | `G2 + 1152·k + 52` | [I] (22.cpp:9155 + 13.cpp:9282 `sub_82092A20`) |
| Estado de técnica por jugador | `u16 0x823D76EE + 1152·k` (1 = B cargada) | [C] (1.cpp:8862-8905) |
| Puntero de estado "S1" | `*(u32*)0x82333990` (struct; usa offsets 4k y 4k+8) | [C] (0.cpp:9141) |
| Puntero de estado "S2" | `*(u32*)0x82333998` (struct; usa offsets 32+4k y 40+4k; flags u16 en +128) | [C] (1.cpp:9049; 11.cpp:9531-9613) |
| Global de técnicas | `G2 + 2460` (520 B), `G2 + 2464` (árbol), `G2 + 2330` (u16) | [C] |
| Aura G | `*(u32*)0x82322AEC` | [C] |
| Aura del jugador k | `G + 420·k` | [C] |
| Global de auras | `G + 968` (nodo de 12 B), `G + 972` (árbol), `G + 976` (u16 = 1 cuando se arrancan efectos de aura) | [C] (10.cpp:3065; 15.cpp:3336) |
| Estado de auras | `*(u32*)0x82322848`: `+256` (u16), `+258` (u16 = nº de jugadores, fijo 2) | [C] (10.cpp:3085, 3120) |
| Tabla aura por ID | `0x82322950 + 4·ID` | [C] (21.cpp:2840) |
| Tabla técnica A por ID | `0x82333208 + 4·ID` | [C] (1.cpp:8848) |
| Tabla técnica B por ID (2ª) | `0x82333404 + 4·ID` | [C] (7.cpp:9220) |
| Objetos de efecto de aura (estáticos, 192 B) | `0x82320000 + {4560, 4944, 5328, 5712} + 192·k` (según i=0..3) | [C] (33.cpp:3044-3099) |

---

## 2. Técnicas: qué construye `sub_82126490`

### 2.1 Entrada

`sub_82126490(k, i)` (34.cpp:9132). `r3 = k`, `r4 = i`. Ranura `S = G2 + 1152k + 544i + 80`.
- Lee `node = *(S+0)` (34.cpp:9133-9163). **El valor de entrada es el nodo crudo** que dejó el cargador (`sub_82083968`, 27.cpp:102).
- Si `node == 0`, sale sin hacer nada (34.cpp:9163).

### 2.2 Qué deja en la ranura (salida)

Tras la llamada, la ranura S queda así:

| Campo | Valor | Línea | Estado |
|---|---|---|---|
| `S+0` | `S_raíz` = `node+192` (árbol de la técnica) | 9177-9180 | [C] |
| `S+4` | `T` = `S_raíz+208` (objeto con nombre) | 9181-9186 | [C] |
| `S+8` | 0 | 9193 | [C] |
| `S+12` | `n1` (nº de hijos del grupo 1) | 9268 | [C] |
| `S+16` | `n2` (nº de hijos del grupo 2) | 9318-9326 | [C] |
| `S+20` | 1 = "procesada" | 9426 | [C] |
| `S+32+4i` | puntero a un **registro hijo** del árbol T (no es copia) | 9237-9238 (`stw r26,-256(r27)`) | [C] |
| `S+288+4i` | lista construida con `sub_8208DEE0`, sólo si el hijo tiene tipo 1 | 9252-9254 | [C] |

Detalles:
- `sub_820800A8(T)` (3.cpp:3): "resolver por nombre una vez". Si el primer byte de T es `'!'` (33) no hace nada. Si no, busca el nombre en T+1 en una tabla, llama a la función encontrada y pone `'!'`. Por eso puede llamarse varias veces. [I, por el código]
- `sub_82126430(hijo, B)` (30.cpp:9200): si `hijo+8 >= 0`, resuelve B con `sub_820800A8`. Si no, compara 4 caracteres y, si coinciden, llama a `sub_821546F8`. No libera nada. [C]
- `sub_8208DEE0(hijo, 0)` (11.cpp:611): crea una lista de punteros (asignación con `sub_82089F58`, 11.cpp:621). La lista es **propiedad de la ranura**. [C]

### 2.3 Qué pasa con el nodo crudo

`node+192` se pone a 0 antes de liberar el nodo (34.cpp:9182). Después `sub_82083D08(node, 0)` (31.cpp:118) llama a `sub_820831F0(node)` (16.cpp:94), que **devuelve el nodo a un pool** (no a malloc) y libera `node+212` si el byte `node+1` vale 1 (el nodo es dueño de ese buffer). [C]

Consecuencia: el árbol S y T sobreviven porque ya no cuelgan del nodo. Si S o T apuntaran dentro de `node+212`, quedarían colgando al liberar el nodo. Como el juego hace esto en cada arranque, **[I] no apuntan ahí**. No se ha comprobado.

### 2.4 Quién lo llama

- `sub_82127390` (31.cpp:9131): en el arranque de batalla, para k=0..1 llama a `sub_82126490(k,0)` (9271) y `sub_82126490(k,1)` (9278). Antes procesa el global `G2+2460` (9146-9184). [C]
- `sub_82126F80(X)` (11.cpp:9270): llama a `sub_82126490(k, 1)` (9291) — **técnica B, no A** (corrige al doc 16) — y luego `sub_82125808(k, 1)` (9298) y `sub_8212AEC0(k, 1)` (9305). [C]
- `sub_82126FD0(X)` (15.cpp:9118): otra forma de procesar B, distinta de `sub_82126490`. Pone `S+8 = S+12 = S+16 = 2` (9173-9179) en vez de los conteos reales, y no libera nada antes de cargar. [C]

### 2.5 Liberación

**`sub_821266A8(k, i)`** (23.cpp:9391). Esta es la función que hay que llamar. Hace:
1. Si `S+4` (T) != 0: `sub_82083A68(S)` (33.cpp:103, resta una referencia en `S+188` y destruye al llegar a 0 con `sub_820832D8`), y pone `S+4 = 0` (9429-9431). [C]
2. Si `S+4` == 0 y `S+0` != 0: `sub_82083D08(S, 0)` (9443). [C]
3. Pone `S+0 = 0` (9446). [C]
4. Recorre `S+288 .. S+540` (64 entradas): si no es 0, `sub_82089C10(p)` (heap) y lo pone a 0 (9449-9472). [C]

**No toca:** `S+8`, `S+12`, `S+16`, `S+20` ni `S+32..S+284` (los hijos son punteros prestados, no se liberan). [C]

Quién la llama:
- `sub_82126A10` (27.cpp:9215, battle-end): para k=0..1 e i=0..1 (9313, 9320). [C]
- `sub_82127A60` (30.cpp:9260): llama `sub_821266A8(r3, 1)` (9301) con `r3 = *(cfg + 8240k + 7040)` = `*(X_k + 4944)` = k. Sólo si `u16[0x823D76EE+1152k] == 0` (9290). [C]
- `sub_821270C0` (5.cpp:9325): llama a `sub_821266A8` también. Función envolvente, no analizada en detalle. [C]

Para k, la llamada correcta es `sub_821266A8(k, 0)` y `sub_821266A8(k, 1)`. **El primer argumento es k, no X.** Internamente multiplica por 1152.

### 2.6 Técnica B por la otra ruta (`sub_82128040`)

`sub_82128040(X, ID)` (29.cpp:9251). Lee `k = *(X+4944)`. Si `table_B[ID] != -1` (tabla `0x82333208`), `h = sub_82083968(tabla, 0, 1)` y guarda `G2+1152k+624 = h`, `G2+1152k+644 = 0` (9296-9298). Si es -1, no hace nada: **el valor viejo se queda**. [C]

Nota: `sub_821280A8(X, ID)` (7.cpp:9200) hace lo mismo con la tabla `0x82333404` (9220-9247). [C]

---

## 3. Auras

### 3.1 Carga

`sub_820B8F78(k, ID)` (21.cpp:2820):
- `h = sub_82083968(*(0x82322950 + 4·ID), 0, 1)` si la entrada no es -1 (2840-2859). [C]
- Guarda `h` en `G + 420·k` (2861). [C]
- **Si la entrada es -1 no escribe nada (2852-2866): `G+420k` conserva el valor anterior.** Hay que poner `G+420k = 0` antes. [C]

`sub_820BA578(sel)` (10.cpp:3043), arranque de auras: 
- `G+968 = sub_82083968(12, 0, 1)` (3065-3077). Sobrescribe el anterior sin liberarlo (fuga pequeña). [C]
- `*(0x82322848)+258 = 2` (3085). [C]
- Para i=0..1: `sub_820B8F78(i, u16 sel+32+80i)` (3095-3115). [C]
- `*(0x82322848)+256 = 0` (3120). [C]

### 3.2 Proceso

`sub_820B8820(k)` (4.cpp:3006). Un solo argumento: k (`r3·420`, 3021). [C]
- `raw = *(G+420k)`. Si 0, sale (3027-3031). [C]
- Igual que `sub_82126490`: `G+420k+0 = raw+192`, `+4 = (raw+192)+208`, `sub_820800A8(T)`, `raw+192 = 0`, `sub_82083D08(raw,0)` (3057, libera el nodo), `+8 = 0`, hijos en `+20..+208`, listas en `+212..+400` (48 entradas), `+12 = n1`, `+16 = n2` (3136, 3221). [C]
- **No pone bandera de "procesada"** (no hay store en `+20` ni equivalente). [C]

Quién lo llama:
- `sub_820B9A18` (23.cpp:3279): primero procesa el global `G+968` (3296-3332, 3334-3408) y luego llama `sub_820B8820(k)` para k < `*(0x82322848)+258` (3421-3437). [C]
- `sub_820BA608` (16.cpp:3176): llama `sub_820B9A18` (3189). Si `*(0x82322848)+256 == 0` llama `sub_820B9B40` (3197-3202). [C]
- `sub_820CD618` (35.cpp:4161): llama `sub_820D9568`, `sub_820BA608` (4177) y `sub_82127A58` (4180). [C]

### 3.3 Liberación

**No hay función por jugador.** `sub_820B8E40` (22.cpp:2998) libera las auras de los dos jugadores (y el global G+968). Su cuerpo para un jugador k:

```
p = G + 420·k
if (*(p+4) != 0) { sub_82083A68(*(p+0)); *(p+4) = 0; }      // 22.cpp:3118-3120
else if (*(p+0) != 0) { sub_82083D08(*(p+0), 0); }           // 22.cpp:3132
*(p+0) = 0                                                   // 22.cpp:3135
for j = 0..47:                                               // 22.cpp:3150, 3138-3161
    if (*(p+212+4j) != 0) sub_82089C10(*(p+212+4j))
    *(p+212+4j) = 0
```
[C] (los números de línea son los del bloque `sub_820B8E40`; el bucle de 48 entradas empieza en `p+208` y avanza de 4 en 4, con `p+212` como primera entrada.)

Quién llama a `sub_820B8E40`: `sub_820CD640` (4.cpp:17748 y 12.cpp:4097). [C]

### 3.4 Efectos de aura (lo que puede apuntar a los datos)

`sub_820B9B40` (15.cpp:3090) crea o reinicia, para cada jugador:
- Cuatro objetos de efecto de 192 B estáticos (`sub_820B8170(k, i)`, 33.cpp:3044).
- Pone en `G+420k+404` un **puntero a un registro hijo** del árbol (3224: `REX_STORE_U32(r25+404, ...)`), y pone `+408` y `+412` (3238-3240). [C]
- Pone `+36 = 0` en los cuatro objetos (3198-3208). [C]
- Al final pone `G+976 = 1` (3336). [C]

Consecuencia: **si se recarga el aura de k mientras un efecto de aura de k está vivo, el efecto apunta a memoria liberada.** Quién lee esos campos (`+36`, `+404`) durante el combate: [N].

**No llamar `sub_820B9B40` a mitad de batalla**: recorre a los dos jugadores.

---

## 4. Técnica en uso: punteros y banderas

### 4.1 Los punteros de estado S1 y S2

- **S1** (`*(0x82333990)`): `sub_82125808(k, r4)` (0.cpp:9120) escribe `S1[k] = 0` (9141-9143). Si encuentra el nombre (4 caracteres, 9165), escribe `S1[k] = r31 + *(r31+12)` y `S1[k+2] = *(r31+8)` (9179-9187). `r31` es un hijo de la ranura que elige `sub_82126078(k, r4)`. [C]
- **S2** (`*(0x82333998)`): `sub_8212AEC0(k, r4)` (1.cpp:9026) hace lo mismo con `S2[k+8]` y `S2[k+10]` (9051-9095). Los flags de S2 están en `+128` (`sub_8212B048`, 11.cpp:9531). [C]
- **r4 importa:** `sub_82126F80` pone `r4 = 1` antes de llamar a ambas (11.cpp:9284-9302). Así que el reenlace trabaja con la **ranura B**. Hay que pasar `r4 = 1`. [C]
- Limpieza global: `sub_82125908` (40.cpp:9291) pone a 0 `S1[0..3]` y `sub_8212AFE0` (30.cpp:9523) hace memset de 144 B de S2 (incluye los flags). Las llama `sub_82126A10` (battle-end). [C]

Estos punteros son los candidatos a "lo que el efecto en uso mira". **Su lector no se ha encontrado: [N].** Muchas funciones leen `0x82333998`: `sub_8212E990`, `sub_821286C8`, `sub_8212DFE0`, `sub_8212BE88`, `sub_82131F58`, entre otras. No se han revisado.

### 4.2 Selector de ranura

`sub_82126078(k, r4)` (7.cpp:9146) devuelve `G2 + 1152k + 544i + 80`. Con r4 < 0: si `X_k+7420 == 35` usa `i = X_k+2980 − 1`; si no, `i = 0`. [C]

Con `r4 >= 0`, el selector devuelve la ranura `r4` directamente. `sub_82125808` y `sub_8212AEC0` reciben `r4 = 1` desde `sub_82126F80`, así que **el reenlace usa la ranura B**. Con `r4 < 0` (caso que no ocurre en el reenlace del juego) elige A, salvo personajes de tipo 35. [C]

### 4.3 Otras banderas

- `G2+2330` (u16): lo pone a 1 o 0 `sub_82129058` (16.cpp:9257, store en 9277) según el payload `*(payload)`. Lo pone a 0 `sub_82126AF8` (13.cpp:9289) y `sub_82126B78` (22.cpp:9169) cuando el jugador tiene `X+2972 == 42`, junto con la cancelación del trabajo `G2+1152k+52` (`sub_82092A20`, 13.cpp:9282). [C] para las escrituras; [I] para el significado.
- `G2+2326`, `G2+2324`: los pone a 0 `sub_82127A60` (30.cpp:9341-9357) con `sub_820CE398`. [C]

### 4.4 Qué comprobar antes de recargar (propuesta)

Como no hay contador, la comprobación debe ser de estado. Antes de liberar k, exigir (todo [I]):
- `G2+1152k+52 == 0` (no hay trabajo de técnica en curso).
- `G2+2330 == 0`.
- `X_k+2972 != 42`.
- `S1[k]` y `S2[k+8]` no cambian durante N fotogramas.
- Ningún efecto de aura de k activo: no hay forma de comprobarlo todavía. Hay que instrumentar (ver sección 8).

---

## 5. Cola de trabajos y estados (pregunta 3)

Las ranuras se procesan por "trabajos" (un callback con payload). Tabla de callbacks en `dbz3_init.cpp`, tipos en `0x8206CC30` y `0x823321A0` (doc 16).

| Función | Archivo:línea | Qué hace | Payload |
|---|---|---|---|
| `sub_82128B08` | 16.cpp:9180 | Carga B: `sub_82128040(X, ID)` | `+0` = ID, `+4` = k |
| `sub_82128B50` | 8.cpp:8777 | Carga B con tabla 2 (`sub_821280A8`) | igual |
| `sub_82128B98` | 17.cpp:9333 | Sondea F30 y guarda resultado en `job+0`. Devuelve 1 | `+44 → +0` = k |
| `sub_82128C68` | 29.cpp:9312 | Espera: F30 != 0 → devuelve 1; si no, devuelve 3 | `job+40` = k (byte) |
| `sub_82128C28` | 13.cpp:9328 | Procesa B con `sub_82126FD0(X)` | `+44 → +0` = k |
| `sub_82128BE8` | 22.cpp:9208 | Procesa B con `sub_82126F80(X)` → `sub_82126490(k,1)` + reenlace | `+44 → +0` = k |

Los dos callbacks de carga (B08, B50) y los de proceso (C28, BE8) están en la imagen. **Quién los encola: [N].** Igual que dice el doc 16.

### 5.1 `sub_82126F30(X)` (18.cpp:9171): la función de sondeo

Devuelve 1 en dos casos:
1. `G2+1152k+644 != 0` (B ya procesada). [C]
2. `G2+1152k+644 == 0` y el nodo crudo de B (`*(G2+1152k+624)`) tiene `u16 +4 == 3`. [C]

En otro caso devuelve 0. El estado 3 lo pone el cargador de archivos, que no se ha localizado. `sub_82083968` pone `u16 +4 = 0` al crear el nodo (27.cpp:181). [C]

Esto corrige al doc 16 (que decía "G2+644 ≠ 0 = pendiente"): **+644 ≠ 0 significa ya procesada.**

Para la ranura A y para el aura no existe un sondeo equivalente en el código. `sub_82127390` procesa A sin esperar (31.cpp:9271). Por eso el mod debe esperar a `u16 raw+4 == 3`, como hace F30 para B. [I]

---

## 6. Trampas y errores que se pueden cometer

1. **Doble proceso.** `sub_82126490` y `sub_820B8820` no miran si ya están procesadas. Llamarlas dos veces sobre una ranura procesada hace que `S` se trate como nodo crudo: memoria corrupta. [C]
2. **Argumento equivocado.** `sub_821266A8`, `sub_82126490`, `sub_820B8820`, `sub_820B8F78` y `sub_82126078` esperan **k**. `sub_82126F30`, `sub_82126FD0`, `sub_82128040` y `sub_82126F80` esperan **X_k**. [C]
3. **Carga sin limpiar.** `sub_82128040`, `sub_82128338` y `sub_820B8F78` no escriben si el ID es -1. Sin limpieza previa, el valor viejo se procesa de nuevo. [C]
4. **Fuga del pool.** `sub_82083968` devuelve -1 cuando el pool de nodos está lleno (27.cpp:132-134, límite `*(base+25520)`). Las fugas (por ejemplo `G+968` en cada `sub_820BA578`, o `G2+2460` en cada `sub_82128338`) pueden hacer que las cargas fallen con el tiempo. [C]
5. **Árbol compartido.** `sub_82083A68` sólo destruye el árbol si su referencia llega a 0. Si otro objeto lo usa, no se libera; no pasa nada, pero no hay que contar con que la memoria vuelva. [C]
6. **Efectos vivos.** Ver sección 4.4. Sin comprobación, el riesgo de cuelgue o corrupción es real. [I]

---

## 7. Secuencia propuesta para el jugador k (NO PROBADA)

Parámetros: `k` ∈ {0,1}; `X = cfg + 2096 + 8240·k`; `ID_t` = técnica nueva; `ID_a` = aura nueva; `sel` = registro de selección (ya escrito por el mod como en FinishSwap).

**Paso 0. Comprobar** (sección 4.4). Si no se cumple, esperar y reintentar.

**Paso 1. Liberar técnicas de k.**
```
sub_821266A8(k, 0)            // ranura A (23.cpp:9391)
sub_821266A8(k, 1)            // ranura B
u16[0x823D76EE + 1152·k] = 0  // estado: B no cargada (se pondrá a 1 si se carga B)
// Limpiar punteros S1 y S2 de k (ver nota):
S1[k] = 0;  S1[k+2] = 0;      // S1 = *(u32*)0x82333990, offsets 4k y 4k+8
S2[k+8] = 0; S2[k+10] = 0;    // S2 = *(u32*)0x82333998, offsets 32+4k y 40+4k
```
Nota: los punteros S1/S2 de k son los que el juego deja apuntando a registros hijos. Limpiarlos antes de liberar evita que queden colgados. La forma exacta de limpiarlos [I].

**Paso 2. Liberar aura de k.** Replicar el cuerpo de `sub_820B8E40` para k (sección 3.3). Después poner `G+420k+0 = 0`.

**Paso 3. Cargar lo nuevo.**
```
// Aura
if (*(u32*)(0x82322950 + 4·ID_a) != -1)
    G[420k] = sub_82083968(*(u32*)(0x82322950 + 4·ID_a), 0, 1)     // sub_820B8F78(k, ID_a)
// Técnica A
h = *(u32*)(0x82333208 + 4·ID_t)
if (h != -1) { slotA = sub_82083968(h, 0, 1);  *(G2+1152k+80) = slotA;  *(G2+1152k+100) = 0 }
// Técnica B
if (X.u16[+7420] == 35 && ID_t != 35) {                          // caso del tipo 35
    sub_82128040(X, ID_t)                                       // 29.cpp:9251
    u16[0x823D76EE + 1152·k] = 1
}
```
Para personajes que no son del tipo 35, la técnica B la cargaría un trabajo encolado por el juego (B08 o B50, quién lo encola [N]). Si hace falta, llamar `sub_82128040(X, ID_t)` directamente [I].

**Paso 4. Esperar a que esté lista.**
```
espera hasta:  sub_82126F30(X) == 1   // B (o procesada)
               u16[*(G2+1152k+80)+4] == 3   // A  [I]
               u16[G[420k]+4] == 3          // aura  [I]
```
Con tiempo máximo y reintento. [I]

**Paso 5. Procesar, en este orden.**
```
sub_82126490(k, 0)            // A (34.cpp:9132). Sólo si *(G2+1152k+80) != 0
sub_82126490(k, 1)            // B. Sólo si *(G2+1152k+624) != 0 [I: igual que sub_82126F80]
sub_82125808(k, r4=1)        // reenlaza S1 de k con la ranura B (0.cpp:9120)
sub_8212AEC0(k, r4=1)        // reenlaza S2 de k con la ranura B (1.cpp:9026)
sub_820B8820(k)              // aura k (4.cpp:3006). Sólo si G[420k] != 0
```
`sub_82126FD0(X)` puede usarse en lugar de `sub_82126490(k,1)` para B, pero deja otro formato (`+8/+12/+16 = 2`). Se recomienda la primera [I].

**Paso 6. Verificar.** `sub_82126F30(X) == 1` tras procesar; `G2+1152k+644 == 1`; `G2+1152k+100 == 1`.

**NO llamar a mitad de batalla:** `sub_82127390` (procesa A y B de ambos jugadores y `G2+2460`), `sub_82126A10` y `sub_820CD640` (liberan a ambos), `sub_820B8E40` (libera las auras de ambos), `sub_820B9A18` y `sub_820B9B40` (procesan y reinician a ambos), `sub_820FE9B0` (battle-end completo). [C]

---

## 8. Lo que falta (en orden de importancia)

1. **Lector de los efectos vivos.** Quién lee `S1`, `S2`, los hijos de la ranura y los campos `+36`/`+404` de los objetos de aura durante el combate. Sin esto no se puede garantizar que recargar es seguro. [N]
2. **Quién encola los trabajos B08, B50, C28, BE8, B98 y C68.** Para saber qué camino usa el juego en un personaje normal. [N]
3. **Quién pone el estado 3** en el nodo crudo (el cargador de archivos asíncrono). [N]
4. **Liberación exacta de S1/S2** por jugador. Sólo se conoce la limpieza global. [N]
5. **Qué hace `sub_82092A20`** (cancelar trabajo) y `G2+1152k+52`. [I]
6. **Significado de `X+2972 == 42`** (estado de movimiento especial). [I]
7. **Significado de `G+976`** y de `*(0x82322848)+256`: quién los pone a 1. [N]

**Instrumentación recomendada** (en vez de probar a ciegas, como pide la memoria del proyecto): leer en cada fotograma, para k, `S1[k]`, `S2[k+8]`, `G2+1152k+52`, `G2+2330`, `G[420k+404]` y `G[420k+36]` (si existe), y registrar cuándo cambian. Así se ve si hay efectos vivos sin tocar nada.

---

## 9. Correcciones al doc 16 y al doc 14

- **Corregido:** `sub_82126F80` procesa **B** (`r4 = 1` en 11.cpp:9284), no A. [C]
- **Corregido:** `sub_82126490` está definido en 34.cpp:9132 (el doc decía 9152, que es una línea del cuerpo). [C]
- **Corregido:** `G2+644 ≠ 0` significa "procesada", no "pendiente" (sección 5.1). [C]
- **Corregido:** `sub_820B8820` **sí** acepta sólo k (`r3·420`, 4.cpp:3021). Era la pregunta 3 del doc 16. [C]
- **Resuelto:** la liberación del aura existe: `sub_820B8E40` (22.cpp:2998). El doc 16 decía [N]. [C]
- **Resuelto:** la liberación de técnicas: `sub_821266A8(k, i)` (23.cpp:9391). El doc 16 decía [N]. [C]
- **Nuevo:** `sub_82126F30` es sondeo, no sólo estado. Devuelve 1 si procesada o cargada en estado 3. [C]
- **Nuevo:** el battle-end completo es `sub_820FE9B0` (4.cpp:6983) → `sub_820CD640` (12.cpp:4081) → `sub_82126A10` (27.cpp:9215) y `sub_820B8E40` (22.cpp:2998). [C]

---

## 10. Archivos mirados

- `generated\dbz3_recomp.34.cpp` (sub_82126490)
- `generated\dbz3_recomp.15.cpp` (sub_82126FD0, sub_820B9B40)
- `generated\dbz3_recomp.18.cpp` (sub_82126F30)
- `generated\dbz3_recomp.23.cpp` (sub_821266A8, sub_820B9A18)
- `generated\dbz3_recomp.27.cpp` (sub_82126A10, sub_82083968)
- `generated\dbz3_recomp.30.cpp` (sub_82126430, sub_82127A60, sub_8212AFE0)
- `generated\dbz3_recomp.31.cpp` (sub_82127390, sub_82083D08)
- `generated\dbz3_recomp.33.cpp` (sub_82083A68, sub_820B8170)
- `generated\dbz3_recomp.16.cpp` (sub_820831F0, sub_82128B08, sub_82129058, sub_820BA608)
- `generated\dbz3_recomp.11.cpp` (sub_82126F80, sub_8208DEE0, sub_8212B048)
- `generated\dbz3_recomp.22.cpp` (sub_82126B78, sub_82128BE8, sub_820B8E40)
- `generated\dbz3_recomp.13.cpp` (sub_82126AF8, sub_82128C28)
- `generated\dbz3_recomp.29.cpp` (sub_82128040, sub_82128C68)
- `generated\dbz3_recomp.1.cpp` (sub_82128338, sub_8212AEC0)
- `generated\dbz3_recomp.7.cpp` (sub_821280A8, sub_82126078, sub_820B0CC8)
- `generated\dbz3_recomp.0.cpp` (sub_82125808)
- `generated\dbz3_recomp.4.cpp` (sub_820FE9B0, sub_820B8820)
- `generated\dbz3_recomp.10.cpp` (sub_820BA578)
- `generated\dbz3_recomp.21.cpp` (sub_820B8F78)
- `generated\dbz3_recomp.12.cpp` (sub_820CD640)
- `generated\dbz3_recomp.35.cpp` (sub_820CD618)
- `generated\dbz3_recomp.3.cpp` (sub_820CD5D8, sub_820800A8, sub_82089C10)
- `generated\dbz3_recomp.17.cpp` (sub_82128B98), `8.cpp` (sub_82128B50), `40.cpp` (sub_82125908)
- `generated\dbz3_init.cpp` (tabla de funciones: 0x820CD5D8, 0x820CD618, 0x820FEBA8)
- `docs\10_tag_team\16_tecnicas_relevo.md` (estudio del compañero)
