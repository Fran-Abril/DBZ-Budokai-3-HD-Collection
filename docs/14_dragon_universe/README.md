# 14 — Dragon Universe por dentro

Estudio del código recompilado (US) del 2026-10-15, solo lectura. Completa `docs/11_campanas/CAMPANAS.md`. [?] = sin medir en el juego.

| Archivo | Contenido |
|---|---|
| `01_casillas_y_partida.md` | Estructura de DU, bloque de 4624 B campo a campo, qué se guarda, casilla ↔ ID, rueda, confirmación, tablas por casilla |
| `02_eventos_lst.md` | Los 55 eventos del intérprete `sub_8215D9A8`, con sus palabras y líneas |
| `03_mapa_premios_codigos.md` | Planetas y archivos del mapa, marcas (64 B), personaje del mapa, niveles, zeni y cápsulas, numeración de códigos |
| `04_ganchos_propuestos.md` | Bocetos para `src/campaigns.cpp` y `campana.py`: casilla nueva independiente, planeta nuevo, medidas que faltan |

## Los 10 hallazgos más útiles

1. **Ya hay 27 bloques libres.** Las 38 casillas del select tienen bloque (`0x8247AA30 + 4624·k`) y solo 11 tienen historia. Una casilla de DU nueva = una casilla del select sin historia, no una 38. La 38 no cabe: tras el bloque 37 vienen la máscara u64, el zeni y el byte de casilla (`0x824A5890`-`0x824A58A0`).
2. **Casilla → ID es una fórmula fija** (`sub_82156E88`: +5 en 22-25, +6 desde 26), no la tabla `0x82020618`. Solo la usan 4 funciones de DU: un único gancho cambia el personaje del mapa y de los menús de DU.
3. **El bloque, campo a campo**: marcas +128, posición +4544, palabras +4564/+4568, planeta +4576, casilla/modelo/traje +4577-4579, radar +4581 [?], scouter +4582 [?], nivel +4583, 7 estadísticas +4584, 7 cápsulas +4592, experiencia +4608.
4. **El archivo de partida guarda nivel, estadísticas, experiencia y cápsulas de las 38 casillas.** Solo el resto del bloque (marcas, palabras, planeta) se guarda para 11; el port ya guarda aparte las demás.
5. **Lista completa de eventos (0-54).** Faltan en `campana.py` las longitudes de 3, 30, 31, 34, 41 y 49. Los ops 25-28, 32, 33 y 37 leen basura: no usarlos.
6. **Evento 49 = escribir un u32 en el bloque de la casilla** desde la propia historia (por ejemplo, modelo y traje del mapa que quedan guardados, o el planeta).
7. **Eventos 51/45/46 = casilla, modelo y traje del jugador en esa escena**: cambiar de forma o de personaje en un combate sin ganchos [? medir en el combate].
8. **Premios nativos**: 17 niveles (con su pantalla, tope 99), 18 zeni (`0x824A5898`, tope 990000), 19/53/43 cápsulas, 41/42 pantalla de 7 cápsulas.
9. **Planetas 0-2** fijos en `sub_821BCC58` (8 archivos cada uno: 3782, 3841 y 3817 en US) y en `sub_821C7D30`. "Nueva historia" pone Namek si la intro es la 102 y el planeta 2 si la casilla es 34 (Broly).
10. **Los códigos 16384.N son pantallas del sistema**, no #LST (16384.4 = guardar, evento 50). Los finales son los códigos 1 y 2. La intro de cada casilla sale de la tabla de 38 de `sub_82156DC8`.

## Qué hace falta para una casilla nueva (resumen de `04` P1)

Una casilla anfitriona libre (por ejemplo 36 o 37) en `[rueda]` + el guardado aparte (ya existe), más tres ganchos:
1. Cambiar la casilla en `0x8245F140` al confirmar una celda virtual (`sub_821CA020` / `sub_821C9DC8`).
2. Casilla → ID del mod en `sub_82156E88`.
3. Opcional: la intro en `sub_82156DC8`.

También `animacion_mapa` para el vuelo y medir el luchador del combate (+324).
