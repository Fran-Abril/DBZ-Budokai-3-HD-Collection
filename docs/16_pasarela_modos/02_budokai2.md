# 02 · Budokai 2: Dragon World, Torneo Mundial y otros

ISO europea (`SLES_518.39`, 5 idiomas). Ficheros: `/USR/DATA_CMN.AFS` (1198 entradas), `/USR/DATA_UK|FR|SP|IT|DU.AFS`
(1275 cada uno; DU = alemán, sin confirmar), `/USR/ADX_US.AFS` (voces). ELF: un segmento, fichero `0x80` →
dirección `0x100000`. Estudio: agentes de búsqueda + comprobación propia (2026-10-15); scripts y volcados en
`E:\DBZ3HD_trabajo\pasarela\b2\` (`survey_b2.py <iso>` en ~35 s) y `E:\DBZ3HD_trabajo\pasarela\re\otros.py`.

## 1. Textos: imágenes, como en B1 (comprobado)

No hay cadenas de texto de los modos en el ELF ni en los AFS (ni ASCII ni UTF-16). Los AFS de idioma difieren
en 915 de 1275 entradas, casi todas #AMT. Decodificadas las primeras 400 de `DATA_UK.AFS` (las que cambian
respecto a `DATA_FR.AFS`): son las **frases del Dragon World** (llegada de los saiyan, Fuerza Ginyu, Freezer, Dr. Gero,
Cell...) una por textura, texto gris claro sobre transparente. Los nombres de los menús son también imágenes
(etiquetas en el ELF: `FG_NOVICE` 0x2A2278, `FG_ADEPT` 0x2A2288, `FG_ADVANCED` 0x2A2298, `FG_BOARD` 0x2A26B0,
`FG_FUKIDASI` (bocadillo) ~0x2A21E8, `D_TURN` 0x2A2C58, `D_CLEAR` 0x2A2C50, `DBALL` 0x2A20F0).

Módulos (nombres de fichero en el ELF): `dw_main.c` 0x2A1FA8, `dw_view.c`, `dw_point.c`, `dw_piece.c`,
`dw_shenglong.c`, `dw_result.c` (Dragon World), `ess_shop.c` (tienda de habilidades), `skill_cmd.c`, `select.c`,
`sel_stage.c`, `sel_model.c`, `mn_option.c`, `btl_cmd.c`, `btb_main.c`.

## 2. Dragon World (tablero por saga)

**Qué es:** la historia de B2 es un juego de tablero: cada saga es un mapa de casillas; el jugador mueve sus
piezas (los guerreros Z) con cartas de movimiento, los enemigos se mueven por el tablero, al chocar hay combate,
hay casillas de evento, cápsulas, bolas de dragón y un límite de turnos; al final, Shenlong.

**Datos:** 9 tableros en `DATA_CMN.AFS` **387-395** (magia `#YDW`; el ELF los lista en 0x277CD0: u32 387..395).
Cabecera: `+0x08` tamaño 0x70; `+0x20..+0x5C` 8 pares (cantidad, offset) a tablas de punteros a registros
(comprobado en la 387: la tabla 3, 687 punteros, acaba justo en su primer registro; la 5, 142 x 16 B, acaba donde
empieza la 6). Cantidades por tablero en `out\ydw_boards.txt`. Significado (hipótesis sin confirmar):
T5 = casillas (id, tipo 1-5, parámetro), T7 = puntos con coordenadas de pantalla (x, y en 1280 de ancho),
T4 = piezas, T3/T6 = rutas o guion. Junto a ellos, 9 grupos de 8 entradas (3-74) con #MAD/#MAS/#MDD: probablemente
el mapa 3D de cada saga.

**Qué se puede reproducir con nuestras piezas:**
- Combates al chocar con un enemigo: conector `RequestBattle` (ya probado aquí con la leyenda de Mr. Satán).
- Charlas: imágenes de texto del AFS del idioma (mismo lector que B1: AFS + #AMT de PS2).
- Pantalla del tablero: panel propio (ImGui con piezas del juego) con casillas, piezas y turnos.

**Qué falta:** descifrar las tablas #YDW (qué casilla es qué, conexiones, piezas y su movimiento, eventos,
bolas, turnos) contra el código de `dw_*.c` (desensamblar MIPS; con Capstone, ver el método de 01 §2); el motor
del tablero (reglas de movimiento con cartas, IA de los enemigos, eventos, Shenlong); el mapa 3D no se puede
mostrar (es de PS2): el tablero iría en 2D sobre una imagen.

**Viable: parcial. Coste: alto** (descifrar formato + motor de reglas + pantalla). Propuesta intermedia (idea del
coordinador, "tablero de 6 casillas"): un tablero pequeño **propio** (6 nodos en un TOML, 2D) con el mismo motor
de pasos de `pasarela.cpp` (charla → tarjeta → combate), usando como fondo/caras las piezas de B3 y, si el jugador
tiene la ISO de B2, sus frases. Coste medio, sin depender de descifrar #YDW.

**Assets:** propios = tablero 2D, casillas, piezas (caras del marcador de B3), textos de reglas; del juego de
origen (ISO) = frases del Dragon World (imágenes, 5 idiomas), quizá las cartas e iconos de casilla (#AMT de los
AFS de idioma/común, sin localizar).

## 3. Torneo Mundial y variantes

**Qué es:** torneo por eliminación en tres niveles (Novato, Experto, Avanzado) con premio en dinero.

**Datos:** no hay tabla de reglas localizada. El código que carga `FG_NOVICE/ADEPT/ADVANCED` está en
0x242000-0x242170 (llama al cargador de sprites 0x1484A0 y compara un byte global cerca de 0x694269 con 16); las
rondas y premios estarán cerca (pendiente de desensamblar).

**Con nuestras piezas:** B3 **ya trae su Torneo Mundial nativo** con los mismos niveles (partida+26: 0 Novato,
1 Experto, 2 Avanzado, 3 Juegos de Cell; `docs/12_conectores/12_torneo_como_motor.md`). Los participantes se
pueden fijar con `dbz3::modos::SetTorneo` (`src/modos.cpp`), incluidos personajes importados de B2.

**Viable: sí. Coste: bajo** (una entrada que abre el torneo nativo con la lista de luchadores de B2; reglas de
dinero y premios, las de B3). Assets: todos de B3; nada del juego de origen.

## 4. Otros modos de B2 (breve)

| Modo | Estado en B3 | Pasarela |
|---|---|---|
| Duelo, Entrenamiento | nativos en B3 | nada que traer |
| Tienda de habilidades (`ess_shop.c`, "# MONEY #", "# SKILL #") | B3 tiene su tienda de cápsulas nativa | nada que traer |
| Fusiones como personaje | ports de B2 (Gotenks gordo/fantasma, `docs/14_frentes/ports.md`) | una misión propia con ellos (idea G2 del coordinador) cabe en el motor de pasos |
| Nave de Babidi | no hay cadena ni módulo propio en el ELF (BABIDI solo como personaje) | sin datos |
