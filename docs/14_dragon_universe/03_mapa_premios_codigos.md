# 03 — Mapa, premios y numeración de códigos

Citas `G/` = `generated/`. [?] = sin medir en el juego.

## 1. Planetas y archivos del mapa

- El planeta está en el bloque +4576 (0 la Tierra, 1 Namek, 2 el de la casilla 34, Broly). Evento 47 lo cambia (objeto de escena +320, < 3).
- `sub_821BCC58(planeta)` (`G/dbz3_recomp.19.cpp:17453`) carga 8 archivos de data_cmn (US): **0 → 3782-3789**, **1 → 3841-3848**, **2 → 3817-3824**. Con un planeta ≥ 3 no carga ninguno. Además lee un byte por planeta de `0x82373E2C` (17543-17561).
- `sub_821C7D30(planeta)` (`.15.cpp:17827`) carga de DATA_USI: planeta 0 y 2 → `0x10000|1963/1964`, 1 → `1965/1966`; todos → `1967/1968`.
- Los nombres PAL (`out/analysis/data_cmn_pal_afl.txt:3762-3804`) son `CLOUDS_/MOON_/EARTH_HI_*` y `*_NAMEK_HI_*`, pero los números US no coinciden (en esa zona US = PAL + 21 hasta + 45). [?] Falta la lista US para saber qué grupo es cada planeta.
- **Un planeta nuevo** pide un grupo nuevo de 8 archivos y un gancho en `sub_821BCC58` / `sub_821C7D30` para el planeta 3. También hay que ampliar el byte de `0x82373E2C` y saber qué más mira el planeta (`sub_821C0BE0` carga 3735 en el modo 1).

## 2. Marcas del mapa (64 B, bloque +128, 64 marcas)

`sub_821C2778(palabra, icono, zona, &pos, escala)` (`G/dbz3_recomp.18.cpp:17931`): busca la marca con esa palabra o un hueco (palabra −1).

| Desp. | Qué |
|---|---|
| +0 | palabra de la escena (−1 = libre) |
| +4 / +8 | icono / zona |
| +12 | escala (float) |
| +16..+31 | posición (x, y, z, w) |
| +32/+36/+40 | −1; evento 31 [?] |
| +44 / +48 | −1; eventos 34 / 35 [?] |
| +52 / +56 | −1 / 100; evento 36 [?] (el 100 coincide con el "5.º valor" del evento 44, que no es la altura) |

Después crea el objeto del mapa con `sub_821BD710`. "Nueva historia" pone una marca con la palabra de la intro, icono 0, en (0, 0, 0) (`.3.cpp:12396-12413`) [? una marca con icono 0 se lanza sola al llegar].

## 3. Personaje del mapa

`sub_821C24B8` (`.27.cpp:17648`) usa el bloque de la casilla (`0x82479A60+180`):
`ID = sub_82156E88(+4577)`, `sub_82133B88(ID, +4578, +4579, 3, 1)`, animaciones `0x823745C0[+4577]`, vuelo `0x823742C8[(+4577)·5 + k]` (k = traje si el modelo < 3, si no 4), y el visor `3853 + 2·(+4582)`. Es el sitio del gancho `modelo_mapa` / `traje_mapa` del port.

Para un personaje con plaza extra (ID ≥ 44) el ID sale de la casilla, no de una tabla. Hay que cambiar la respuesta de `sub_82156E88` solo cuando la llama `sub_821C24B8` (ver `04`).

## 4. Premios y niveles

- **Niveles**: evento 17 (`0x8243A234` = n). Al acabar la escena `sub_8215ECE0` (`.4.cpp:12311`) recorta n a 99 − nivel; `sub_8215EB10` abre la pantalla de subir de nivel. `sub_821587F8` (`.0.cpp:11926`) copia el resultado al bloque: nivel +4583, 7 estadísticas +4584, experiencia +4608. Curva por casilla en `0x82374A20[casilla]`.
- **Zeni**: evento 18, se suma a `0x824A5898` (tope 990000).
- **Cápsulas**: evento 19 (siempre), 53 (si tienes menos de p1), 43 (al azar de un grupo), 41/42 (pantalla de 7 cápsulas). Inventario en `0x824A58C9`. La pantalla "cápsula conseguida" es `sub_821BA820`.

## 5. Numeración de códigos

| Código | Qué es | Fuente |
|---|---|---|
| 1 / 2 | Finales: 1 → contenedor 1728 (estilo Goku), 2 → 1729 (estilo Broly). Una historia acaba con `(20, 0, 1<<16 \| entrada)`; campana.py reusa una entrada libre (39 por defecto) porque añadir entradas al final del contenedor colgaba. | `du_codigos.json`, `campana.py:736-756` |
| 100, 102, 104, 106 | Intros de saga (Saiyan, Namek, Cell, Buu); 101/103/105 cierres. Entrada = casilla. | `campana.py:759` |
| Intro por casilla | `sub_82156DC8`: 0→100, 1→104, 2→100, 3→104, 4-6→106, 7→100, 8→104, 9→106, 10-13→100, 14→104, 15-17→106, 18-19→100, 20-22→102, 23-27→104, 28-31→106, 32-33→102, 34-35→106, 36→100, 37→104 | `G/dbz3_recomp.20.cpp:11868-11964` |
| 2000-2399 | Escenas y eventos compartidos (2100 = "I found something!"). Con código < 2000 al acabar se restaura la posición del mapa; a partir de 2000, no. | `.4.cpp:12335-12357` |
| 3000 | Vuelta al mapa (`sub_82156B38`). | `src/campaigns.cpp:200` |
| 5000-6199 | 12 bloques de 38 entradas (una por casilla): mapa `5000+100·saga+k`, combates `5400+100·saga+k`. | `CAMPANAS.md:47` |
| 8192+N | Escena pedida por un evento con parámetros (`sub_8215EC68`). | |
| **16384.N** | **Pantallas del sistema**, no #LST: `sub_8215E9C8` las desvía a `sub_82160688` (`.19.cpp:12074`, `.30.cpp:12383`). `0x4000000N` en `0x8234DD24`: N = 4 es "guardar" (evento 50); `sub_821602F0` encadena N → N+1 (`.34.cpp:12548-12554`). [? qué es cada N] | |

Palabras pendientes (`sub_8215A5D0` las pone a −1, `.38.cpp:11929`): `0x8234DD1C` siguiente escena (op 20), `0x8234DD20` si pierdes (op 52), `0x8234DD24` pantalla del sistema (op 50), `0x8234DD28` diálogo en el escenario (op 21). Tras un combate, `sub_82159AD8` (`.32.cpp:11859`) pasa la de ganar y la de perder a eventos del mapa (`sub_821C1970` / `sub_821C1988`).

Lanzar una escena (`sub_8215E9C8`, `.19.cpp:12016`): guarda la posición (bloque +4544) y, si el código es < 2000, llama a `sub_821584A0(palabra)` [? actualiza la palabra guardada +4564].
