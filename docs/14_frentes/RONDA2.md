# Ronda 2 (2026-10-15): llegar al 100 % de lo pactado

Lee antes `REGLAS.md` (siguen todas). Esto añade lo nuevo de esta ronda.

## Objetivo del usuario
- 100 % de `studies/11_vision_futura` (29 fichas; cada una cuenta 1 solo si su **entregable mínimo** funciona en el
  juego y está probado con capturas). La matriz de `vision_futura.md` §1 está **desfasada**: el frente de modos
  terminó después (marcador, Ranking, Laboratorio, torneo.toml, modificadores `tiempo`/`regen`/`objetivo_tiempo`,
  ver `modos.md`). Comprueba el estado real en el código antes de empezar una ficha.
- Más transformaciones y, como en Infinite World, **todas las formas en una única técnica** (ficha 14).
- Fallos que reportan jugadores del Mod Kit (frente modkit).
- Icono propio del guardado del HUB (fuente del usuario, arte con personaje: solo pack opcional; en la release, uno
  CC0 hecho por código).
- Todo nativo, sin sustituir nada del juego, US y EU.

## Enjambres Haiku (petición del usuario)
Para leer y buscar (fichas, docs, código, logs, capturas) lanza **subagentes con `model: "haiku"`**, varios a la
vez y con preguntas concretas ("¿qué función escribe X?", "resume la ficha 19 en 5 líneas"). El razonamiento, las
decisiones, las ediciones, el build y las pruebas los haces tú. No pases a Haiku tareas de edición.

## Frentes y dueños de ficheros (toca solo lo tuyo; para lo demás, propuesta en tu `.md`)
| Frente | Fichas / tareas | Ficheros propios |
|---|---|---|
| modkit | fallos de jugadores (vista previa, SDBH Xeno Janemba, texturas de cristal F5), K1/K2 | `mod center hd/roster_build.py`, `model_render.py`, `modkit_gui.py`, `retos_predefinidos.py`, `awo_tools/sdbh_model.py`, `tools/make_modpacks.py` |
| transformaciones | 14 + más formas por personaje | `src/transformaciones.cpp` (nuevo), secciones de cápsulas de `src/roster_ext.cpp`, `src/form_selection.h` |
| combate | 05, 06, 11, 12 (datos), 15, 16, 18 | `src/combate.cpp` (nuevo), `studies/tools/*` |
| rpg | 23, 24, 25, 27 y lo que falte de 01, 03, 09, 26, 28 | `src/modos.cpp`, `src/rpg.cpp` (nuevo), `mod center hd/modos_reto.py`, `campanas/retos/` |
| escenarios | 10, 19, 22, E1 (nombres de escenarios) | `src/escenarios.cpp`, `src/stage_ext.cpp`, `src/clima.cpp`/`src/cine.cpp` (nuevos) |
| campanas | 04, 17, 20 (misión), 21, Leyenda de Mr. Satán (reglas B1, dificultades, puntos) | `src/campaigns.cpp`, `src/pasarela*.cpp`, `mod center hd/campanas/` (salvo `retos/`), `docs/16_pasarela_modos/` |
| online | 02 (y 03 online si el mínimo lo pide) | `src/net/*`, `src/ingame/online_panel.*` |
| hub | icono/aviso de guardado, entradas del HUB para los modos nuevos, piezas CC0 que falten | `src/hub_menu.cpp`, `src/ingame/hub_panel.cpp`, `res/ingame_ui/`, `assets_propios/` |
| eu | HUB, modos, pasarela, escenarios y `ports_guard` en la versión EU | fase 1: solo mapa de direcciones US→EU (fichero nuevo); fase 2 la coordina el coordinador |
| ports (ya en marcha) | verificación de ports; después 07, 08, 13, 20 (Gokule/Yamhan) | `src/ports_guard.cpp`, `mod center hd/ports_*.toml`, `docs/07_ports/` |

`src/hub.cpp` (conector de combate) y `CMakeLists.txt` son del coordinador: pide lo que necesites (p. ej.
`ChainBattle`) con un diff corto en tu `.md`. Para añadir tu `.cpp` nuevo a `CMakeLists.txt`, añádelo tú en
`DBZ3_SOURCES` y en el `REMOVE_ITEM` de la build solo EU (una línea cada uno, nada más).

## Entrega de cada frente
- Código compilado (`python tools/compilar.py`) y probado en tu copia (`python tools/pruebas.py`), con capturas en
  `D:\DBZ3HD\_pruebas\`. Batería `python tools/pruebas.py todo` en PASS antes de dar tu frente por cerrado.
- `docs/14_frentes/<frente>.md` actualizado: qué ve el jugador, cómo se probó, puntuación de cada ficha tuya
  (0-1) con la prueba, lo que queda.
- Informe final en español, breve, sin jerga.
