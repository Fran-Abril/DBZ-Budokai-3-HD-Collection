# Feedback del usuario sobre la 1.4.6 (2026-10-15) y mapa de correcciones

Build de prueba del usuario: `out/build/win-amd64-release/dbz3.exe` (1.4.6). La release sigue SIN publicar.

## Activado para el test (hecho)
- `ps2_games` junto al exe (enlace a `D:\DBZ3HD\ps2_games`: ISOs de B1, B2, IW, SB1, SB2): Leyenda de Mr. Satán,
  saga B1, retos de IW y tablero dejan de salir en gris. (Ojo: es un enlace; nunca moverlo con robocopy sin /XJ.)
- Mods de la ronda instalados desde los zips publicados en Nova's Mods: Goku Black, Roshi, Yamhan x2, 3 trajes de Bu,
  ports 1.1 e **Historia de Freezer 1.3** (la 1.2 anterior, apartada en `mods_respaldo/antes_1.4.6`).
- Packs visuales opcionales (solo local, arte de terceros): `pack_hub` (fondo, vuelos, icono SSJ4) y
  `pack_campana_freezer`.

## Mapa de correcciones (por prioridad)
| # | Fallo / petición | Causa probable | Qué hacer |
|---|---|---|---|
| 1 | Cierre en Tag Team: Janemba + Súper Bu contra Goku + Freezer (`logs/dbz3_1034.log`) | Relevo con id 44 en forma 2 y técnicas de IW; mismo patrón que Pan (anillo de efectos `sub_82152890`) o forma/absorción de Bu en el relevo | Medido en el log: tras el relevo a id 44 (con técnicas en uso) el juego llama a `sub_821388D0` (lee una tabla de efectos/recursos: cuenta en +0 y punteros) con **0x54** en vez de un puntero: un desplazamiento del fichero del port sin reubicar (lectura de guest 0x54, `lr 0x82138910`). No es el anillo de efectos de Pan. Siguiente: localizar qué fichero del port (técnicas/efectos de IW) carga el relevo y reubicar ese campo en la conversión, más una guarda que ignore punteros < 0x1000 |
| 2 | Historia de Freezer sin diálogo | Mod 1.2 (para el exe viejo) con el exe 1.4.6 | Ya instalada la 1.3; repetir. Si sigue, revisar textos de campaña con el exe nuevo |
| 3 | Al salir de los modos nuevos, Goku habla como al volver al menú principal | La vuelta pasa por el menú principal nativo (voz/bocadillo de su fila) | Volver directo al HUB sin la voz de bienvenida (o silenciarla esa vez) |
| 4 | No se usan los assets de la carpeta del usuario | El arte de terceros solo va en packs opcionales y no estaban instalados | Instalados en local; ampliar `pack_hub` con más piezas (fondos por modo, renders por personaje, cut-ins) |
| 5 | Modos de IW / B1 y Leyenda de Mr. Satán sin activar | Faltaba `ps2_games` junto al exe | Hecho (arriba); el launcher debería dejar elegir esa carpeta y avisar |
| 6 | Tablero de B2 decepcionante: se esperaba el port real | Hoy es un grafo de 6 casillas propio (mínimo de la ficha 17) | Diseñar el port real: mapa y fichas de B2 leídos de la ISO, tiradas, objetos/cápsulas en casillas, eventos y jefes, varias sagas |

- Roshi y Goku Black no salían: el roster de la carpeta no estaba montado con los mods nuevos. **Hecho**: `_roster` con 29 personajes.

Siguiente paso: el usuario prueba con todo activado y amplía este documento; luego una ronda por prioridad.
Segunda tanda de feedback (2026-10-15 noche): roadmap para debatir en `ROADMAP_RONDA3.md`.

## Estado de los puntos 1-3 (2026-10-15 noche, sin publicar; build `out/build/win-amd64-dual`)
1. **Cierre en Tag Team: ARREGLADO (causa medida).** No era un fichero del port: el `r28 = 0x8234A470` del cierre es
   el registro de modelo de **Freezer (id 27) con el traje 7** (indice 6), fuera de su lista de 3 trajes (volcado de
   char96 en la copia de prueba: `id 27 forma 0 traje 6 -> registro 8234A470, +4 = 0x54`). El relevo de Tag Team
   (`StartSwap`, `src/tagteam.cpp`) daba al que entra el traje del que sale ("mismo traje"): Goku con un traje extra
   (Armadura/SDBH, 6-7) -> Freezer. Ahora se recorta como hace el juego (`sub_82113870`): si el modelo de esa forma no
   tiene ese traje, el 0 (`SuitFor`). Guarda extra en `src/ports_guard.cpp`: `sub_821388D0` con un "puntero" < 0x1000
   carga el modelo sin cadenas de fisica y lo anota en el log (`cadenas de fisica no validas`). Pareja EU regenerada.
   Prueba: relevos con el traje del que sale forzado a 4 (fuera de rango del compañero), 3 relevos por combate, 3
   combates sin cierre (`D:\DBZ3HD\_pruebas\20261015-2058*`, `-2101*`, `-2103*`, `-2106*`). No se pudo elegir por
   teclado justo Goku traje 7 + Freezer en la rueda de Tag Team (los ports rodean a Goku): falta que el usuario lo repita.
2. **Historia de Freezer: BIEN con la 1.3.** DU > nueva historia > Freezer: intro con 3 viñetas y el bocadillo de
   Freezer en el mapa ("Las Bolas de Dragón de Namek serán mías"), escena 102 -> contenedor 6128 del mod. Capturas:
   `D:\DBZ3HD\_pruebas\20261015-211154_feedback_test_freezer_inicio` (002-007). El fallo era la 1.2. Sin cambios de código.
3. **Voz de Goku al volver al HUB: ARREGLADO.** Gancho de la entrada del menu (`sub_8218C8E0`, `src/hub_menu.cpp`):
   si se vuelve de un modo lanzado desde el HUB, el saludo se calla en el mismo cuadro; en el arranque normal sigue.
   Prueba HUB > Online > fuera: log `vuelta al HUB sin el saludo del menu principal` solo en la vuelta, y el HUB
   reabierto (`D:\DBZ3HD\_pruebas\20261015-211427_feedback_test_hub_vuelta`).
- `python tools/pruebas.py todo`: 5/5 PASS (21:17).

## Punto 6: plan del tablero real de Budokai 2 (estudio Haiku, solo lectura)
- En la ISO de B2 hay **9 tableros** `#YDW` (`/USR/DATA_CMN.AFS`, entradas 387-395, 30-122 KB), cada uno con 8 tablas
  (hipótesis: T5 casillas, T7 coordenadas de pantalla, T4 piezas, T6 eventos, T8 tipos). El ejecutable trae los módulos
  `dw_main/dw_view/dw_point/dw_piece/dw_shenglong/dw_result` y `ess_shop`. Los textos del tablero son imágenes en los
  AFS de idioma. Copia de análisis local: `E:\DBZ3HD_trabajo\b2_tablero\ydw\` (no se reparte).
- Fases: **0** (1-2 días) decodificar casillas/coordenadas/piezas de los 9 tableros y dibujarlas sobre una captura →
  decidir; **1** saga completa con el motor de pasos actual; **2** tablero real leído de la ISO del jugador (casillas
  especiales, enemigos, eventos; requiere desensamblar `dw_*.c` de MIPS); **3** reglas completas (movimientos por
  turno, cápsulas, bolas de dragón y Shenlong). Riesgo principal: coste de RE de las reglas (semanas).
