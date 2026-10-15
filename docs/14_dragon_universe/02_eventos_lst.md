# 02 — Eventos de una entrada #LST (lista completa)

Intérprete: `sub_8215D9A8` (`G/dbz3_recomp.24.cpp:12226`), un evento por llamada. Lee tres u32 LE (`op`, `p1`, `p2`) (12288-12360) y salta con la tabla `0x8201FDF0` (ops 0-54; 12362-12383). Si `op > 54`, se trata como un evento de recurso (12364). El contador de eventos está en el objeto de escena +220 y el total en +48 (12261-12271). Los eventos largos leen más palabras y avanzan el puntero (+52).

**Eventos de recurso**: cargan la entrada `p2` (`sub_82083968(p2, 0, 1)`; `0x10000|N` = DATA_USI, `0x20000|N` = voces ADX) y, al terminar la carga, `sub_8215D5B8` (`G/dbz3_recomp.10.cpp:12173`) llama al manejador de la tabla de 21 entradas en la pila (12207-12299). Los ops de recurso que no tienen entrada (25-28, 32, 33, 37, >54) leen basura de la pila: **no usarlos**.

| Op | Palabras | Qué hace | Destino / función | Línea |
|---|---|---|---|---|
| 0, 9 | 3 | Nada | — | 12383 |
| 1, 2 | 3 | Recurso → `sub_821B18F0` / `sub_821B4668` (guarda el recurso en `0x82469010 + (4·p0+p1)·8416`) [? actores 3D de la escena] | `.10.cpp:12207-12215` | |
| 3 | 4 | Recurso con un dato más (+224) → `sub_821B1B88` [?] | | 14056 |
| 4-8 | 3 | Recurso → `sub_821B1CE0`, `1D60`, `1DF8`, `1EF0`, `2078` [? animación, cámara, efectos de la escena] | | |
| 10 | 3 | Recurso (texto `DATA_USI#n`) → `sub_82159028` | `sub_8215A4D8` | |
| 11 / 12 | 3 | Imagen → `sub_82159638` / `sub_82159690` | | |
| 13, 14 | 3 | Voz o música: `sub_8213A160(p1, p2)` | | 14046 |
| 15 | 3 | `0x8243A0F0` = p2 (guion tras el combate) | | 12898 |
| 16 | 3 | Combate: recurso #BPC → `sub_8215D368` | | |
| **17** | 3 | **Niveles de premio**: `0x8243A234` = p2. Al acabar la escena se recorta a 99 − nivel (`.4.cpp:12361-12394`) y sale la pantalla de subir de nivel (`sub_8215EB10` → tarea `sub_8215AE58`, `.32.cpp:12099-12134`). | | 12499 |
| **18** | 3 | **Zeni**: objeto+260 = p2. Al acabar se suma a `0x824A5898` (tope 990000) y suena. | | 12504 |
| **19** | 3 | **Cápsula de regalo**: p2 = ID. La apunta en la lista de premios de la escena (objeto + 4·(68+i)), marca "nueva" si no tenías ninguna y la añade (`sub_82158778`). | | 12603 |
| 20 | 3 | Siguiente escena: `0x8234DD1C` = p2 (palabra) | | 12867 |
| 21 | 3 | Escena de diálogo en el escenario: `0x8234DD28` = p2 | | 12889 |
| 22 | 3 | Borra todas las marcas del mapa (`sub_821C19A0`) | | 13532 |
| 23 | 3 | Borra la marca `p2` y la `p2 \| 0x8000` (`sub_821C2828`) | | 13538 |
| 24 | 9 | Marca del mapa `(24, n, palabra, icono, zona, x, y, z, escala)` → `sub_821C2778` | | 13059 |
| 29 | 7 | Objeto del mapa por nombre (16 B) + estado; lista en `0x82439B38 + 16·n`, cuenta en `0x8243A231` | | 13884 |
| 30 | 7 | Nombre (16 B) + valor → `sub_821C1AE0` [? cambia el estado de un objeto ya puesto] | | 13980 |
| 31 | 6 | Cambia +32/+36/+40 de una marca ya puesta (`sub_821C2958`; lee las palabras 2-5) [?] | | 13618 |
| 34 | 4 | Marca: +44 = valor (`sub_821C29A8`) [?] | | 13710 |
| 35 | 4 | Marca: +48 = valor (`sub_821C29F0`) [?] | | 13760 |
| 36 | 5 | Marca: +52, +56 = valores (`sub_821C2A38`; +56 vale 100 al crearla) [?] | | 13812 |
| 38 | 3 | `sub_82139EA0(p2)`; `sub_8215A258` → objeto+308 [?] | | 12527 |
| 39 | 3 | Bloque +4581 = 1 (3.er modo de la vista del mapa [?]) | | 12545 |
| 40 | 3 | Bloque +4582 = 1 (scouter [?]) | | 12565 |
| 41 | 9 | Pantalla de 7 cápsulas dadas en el evento (`p1`, luego 7 IDs; 12 bits bajos) → tarea `sub_8215AD98` [? elegir una] | | 12722 |
| 42 | 3 | La misma pantalla con las 7 cápsulas equipadas de la casilla (bloque +4592) | | 12804 |
| 43 | 3 | Cápsula al azar del grupo p2 (`sub_82145808`) y la regala como el 19 [?] | | 12660 |
| 44 | 5 | Salida en el mapa `(44, 0, x, z, y)` (objeto +240..+256) | | 12903 |
| **51 / 45 / 46** | 3 | **Personaje del jugador en esta escena**: casilla (+324) / modelo (+328) / traje (+332) = p2. `sub_8215A7A8` los rellena con el bloque +4577/+4578/+4579 al crear la escena (`G/dbz3_recomp.2.cpp:12231-12247`). Sirve para cambiar de forma o de personaje en un combate concreto [? medir en el juego qué los lee en el combate]. | | 13021 / 13026 / 13031 |
| 47 | 3 | Planeta: objeto+320 = p2 (0, 1 o 2; si ≥ 3, 0) | | 13036 |
| 48 | 10 | Marca del mapa condicionada a la marca de historia de 2 bits n.º (palabra 9) ≠ 0 (`0x8247A9B0`) | | 13271 |
| **49** | 4 | **Escribe un u32 en el bloque de la casilla**: `(49, -, p2, p3)`: `bloque + 4·p2 = p3` (índice en palabras; 1156 palabras = 4624 B; p1 no se usa) | | 13551 |
| 50 | 3 | Pantalla de guardar: `0x8234DD24` = `0x40000004` (escena especial 16384.4) | | 12876 |
| 52 | 3 | Escena si pierdes: `0x8234DD20` = p2 | | 13525 |
| 53 | 3 | Cápsula de regalo solo si tienes menos de `p1` | | 12570 |
| 54 | 3 | Vuelve a la posición guardada del mapa (bloque +4544/+4560) | | 13049 |

Notas
- El 19 regala siempre; el 53 solo si tienes menos de `p1` (12590-12600). Al acabar la escena, la pantalla de "cápsula conseguida" sale si hubo premios (`.4.cpp:12397-12408`).
- Las palabras de los eventos de mapa coinciden con `MAP_EVENT_WORDS` de `campana.py:358` (24→9, 29→7, 35→4, 36→5, 44→5, 48→10, 54→3). **Faltan en esa tabla 3 (4), 30 (7), 31 (6), 34 (4), 41 (9) y 49 (4).** Si un mod usa uno de ellos, los eventos siguientes se leen mal.
