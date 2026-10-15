# Pasarela de modos (otros juegos de la saga en el HUB)

Estudio y prototipo del frente "Pasarela de modos" (2026-10-15). Objetivo: traer al HUB modos de Budokai 1,
Budokai 2 e Infinite World. Cada modo depende de que el jugador tenga **su copia** de ese juego (ISO en
`ps2_games`, junto a `dbz3.exe`). Lo del juego de origen (textos, imágenes, tablas) se lee **en tiempo de
ejecución** de esa copia, sin extraerla, y nunca va en la versión publicada ni en el repositorio.

## Resumen

| Modo | Juego | Viable | Coste | Estado | Doc |
|---|---|---|---|---|---|
| La leyenda de Mr. Satán (11 retadores con charlas) | B1 | **Sí** | hecho (prototipo) | jugable desde el HUB (entrada `satan`) | [01](01_leyenda_mr_satan.md) |
| Reglas especiales de cada retador (vida que no se recupera, 60 s, formas, 5 Cell Jr. de 1000) | B1 | Sí | hecho | con `RequestBattleEx` | [01](01_leyenda_mr_satan.md) §3 y §5 |
| Reglas en código (A16, Mr. Satán herido, ki lento, ki del rival), dificultad y puntos con el resultado de B1 | B1 | Sí | hecho (ronda 2) | `VigilarCombate` + piezas 666/668 de la ISO | [01](01_leyenda_mr_satan.md) §5 |
| Saga de la historia de B1 (Vegeta en Namek, con Zarbon importado) | B1 | Sí | hecho (ronda 2) | entrada **Pasarela** del HUB | [04](04_misiones_y_tablero.md) §3 |
| Tablero propio de 6 casillas (estilo B2) | B2 | Sí | hecho (ronda 2) | entrada **Pasarela** | [04](04_misiones_y_tablero.md) |
| Misión con Gotenks gordo; absorciones de Súper Bu (trajes) | B2 | Sí | hecho (ronda 2) | entrada **Pasarela** | [04](04_misiones_y_tablero.md) §4 |
| Retos de IW (Training No.1, Endure Gohan's attacks!) | IW | Sí | hecho (ronda 2) | entrada **Pasarela**; textos de la copia | [04](04_misiones_y_tablero.md) §5 |
| Torneo Mundial (Novato / Experto / Avanzado) | B2 | Sí | bajo | B3 ya lo trae nativo; lista de rivales con `modos.cpp` | [02](02_budokai2.md) §3 |
| Dragon World (tablero por saga) | B2 | Parcial | alto | datos localizados (#YDW), sin motor de tablero | [02](02_budokai2.md) §2 |
| Misiones del Dragón (combates con condición) | IW | Parcial | medio | tablas #DMD localizadas, condiciones sin descifrar | [03](03_infinite_world.md) §2 |
| Minijuegos de IW (vuelo, disparo, escenas) | IW | No (hoy) | muy alto | son motores aparte, no combates | [03](03_infinite_world.md) §3 |

**Lo común a los tres juegos (comprobado):** los textos de los modos son **imágenes** (texturas #AMT de PS2,
una por frase o bloque) en los AFS de cada idioma, no cadenas de texto. Por eso la pasarela muestra esas
imágenes tal cual (el mismo enfoque de "textos como imágenes" que las campañas de Dragon Universe). B1 trae 5
idiomas (EN, FR, DE, ES, IT), B2 también, IW (USA) solo inglés.

## Piezas

| Pieza | Dónde | Qué hace |
|---|---|---|
| Motor del modo | `src/pasarela.cpp` | pasos (presentación, charla, tarjeta del combate, combate, final), panel con piezas del juego, lectura del resultado sin ganchos, avance guardado; piezas comunes (`VigilarCombate`, reglas, cápsulas, trajes, personajes de mods) |
| Saga, tablero, misiones, retos de IW | `src/pasarela_misiones.cpp` | entrada **Pasarela** del HUB ([04](04_misiones_y_tablero.md)) |
| Trajes de Bu absorbido | `mod center hd/campanas/trajes_bu/` | 3 `traje.toml` con modelos del juego (`juego:N`) |
| Lector de la ISO de B1 y de IW | `src/pasarela_b1.cpp`, `src/pasarela.h` | ISO 9660 + AFS + #AMT (PS2) + tablas del ELF (leyenda e historia), textos de misión de IW (ISO o carpeta), en memoria |
| Herramienta reproducible | `mod center hd/pasarela/pasarela.py` | `info`, `extraer` (para revisar), `prueba` (autocomprobación); misma lógica que el lector del juego |
| Pruebas en el juego | `pruebas/*.toml` | escenarios de `tools/pruebas.py` |
| Parche del HUB (aplicado) | `parche_hub_satan.diff` | entrada en gris con explicación sin la ISO |

Estado del frente, pruebas y propuestas: `docs/14_frentes/pasarela.md`.
