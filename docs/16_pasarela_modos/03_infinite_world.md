# 03 · Infinite World: Misiones del Dragón, minijuegos y otros

Copia extraída en `D:\DBZ3HD\ps2_games\Infinite World (USA)\` (ELF `SLUS_218.42`; `USR\DATA_CMN.AFS`,
`USR\DATA_USA.AFS` (1532 entradas, solo inglés), `ADX_*.AFS`, vídeos `.SFD`). ELF: un segmento, fichero `0x80` →
dirección `0x100000`. Estudio de agentes de búsqueda + comprobación propia; scripts y volcados en
`E:\DBZ3HD_trabajo\pasarela\iw\` (`survey_iw.py <carpeta> <salida>`, ~3 s).

## 1. Modos y textos

Estados del juego (nombres en el ELF): `MainMenu`, `VsSelect`/`VsBattle`, `TrainingSelect`/`TrainingBattle`,
`sysMissionSel`, `sysMission`, `sysMissionMovie`, `sysRoadCharSelect` (¿Dragon Road?), `sysShenron`,
`sysStaffRoll`, `BattleDemo`. No hay Torneo Mundial ni Dragon Arena en IW.

**Textos: imágenes** (comprobado): `DATA_USA.AFS` lleva los textos como #AMT de PS2 (p. ej. los nombres de las
técnicas, una textura por nombre). Las condiciones de victoria tienen etiquetas internas `[MSG]Ready`, `Fight`,
`KO`, `TimeUp`, `RingOut`, `Goal`, `Success00/01`, `Clear00/01`, `Miss`... (0x395168-0x395320), también dibujadas.

## 2. Misiones del Dragón (combates con condición)

**Datos:** `DATA_CMN.AFS` **1092** = #AMB con un hijo **#DMD** (+0x30, 8928 B): cabecera `+8` = 106 registros,
`+0x10` tabla de offsets; registros de **0x50 B**. Segunda tabla igual en **1093** (#DMD en +0x105D30, 149 registros).

Campos de la tabla 1092 (comprobado: estructura; significado, en parte hipótesis):

| Offset | Campo |
|---|---|
| +0x00 | número de fila (comprobado) |
| +0x04 | grupo 0-3 |
| +0x08 / +0x0C | valores de 16 bits distintos por fila (¿tiempo, vida, posición?) |
| +0x10 | ID de personaje de IW del rival (nombres en la tabla del ELF 0x3587F0, 52 x 32 B): Buu, Gotenks Buu, Dabura, Janemba, Super 17... |
| +0x14 / +0x18 | 1 (salvo la fila 105) |
| +0x20 | clave 0x867 + fila (¿mensaje o guion?) |
| +0x24 | hito: 1..7 en las filas 14, 29, 44... 104 (7 bloques de 15 misiones) |

La tabla 1093 (149 filas, claves 2000-2268, rivales como Gohan, Vegeta, Gran Saiyaman) podría ser la del modo
"Road" (sin confirmar). La "Misión 100" no aparece como tal: si se cuenta desde 1, sería la fila 99 de la 1092.
`#MSS` (DATA_CMN 890) guarda umbrales (180, 90, 3600/3000/2500/2000/1400, 30.0, 100.0...): probablemente tiempos o
rangos de puntuación. `#DMK` (1094-1111) con la etiqueta "Time": marcador de las misiones con tiempo.

**Con nuestras piezas:** las misiones que son un combate con condición (ganar, aguantar N s, ganar antes de N s,
ring out) caben en el motor de pasos de `pasarela.cpp` + conector (rival, escenario; tiempo y vida con la
ampliación del conector propuesta en `docs/14_frentes/pasarela.md`). Escenarios de IW → B3 por nombre: la tabla
del ELF de IW da R&R BASE 48, BUDO 50, BUU BODY 52, CELL 54, GOKU 56, KAIOU 58, ISLAND 60, MIND & TIME 62,
NAMEC 64, PLANE 66, ROCK 68, TOWN B 70, TOWN 72 (entradas de su DATA_CMN; los mapas de B3 tienen nombres parecidos).
El rival puede ser un personaje de IW ya importado (`importar.py iw`).

**Ronda 2 (2026-10-15):** los textos de cada misión están en `DATA_USA.AFS` desde la entrada **132** (un `#AMB` por
misión: `[#AMT texto, #AMT título]`, en el orden del juego: 132-141 "Training No.1-10", 142 "Defeat Raditz!"...); el
título y el texto dicen la condición ("Perform 30 Basic Attacks... 180 seconds", "Without performing any attacks keep your
health above the specified level... 60 seconds", "Let Hercule win!"). En `#DMD`, +0x08/+0x0C son la posición del icono
en el mapa de misiones. Dos retos jugables desde la entrada **Pasarela** del HUB: [04](04_misiones_y_tablero.md) §5.

**Ronda 3 (2026-10-15, frente F): tabla de misiones DESCIFRADA** — está en el **ELF**, no en `#DMD`
(`#DMD` 1092/1093 son los iconos de los mapas: posición, cara, tipo de casilla). ELF USA: fichero **0x376D50**,
registros de **0x60 B** numerados desde 1 (u16 en +0); **201 registros: 1-96 Misiones del Dragón, 97-201 Dragon Road**
(105, jugador = el elegido; coincide con las 105 filas de #DMD 1092). Comprobado con los 96 títulos de `DATA_USA`
(entrada 131 + número: 11 "Defeat Raditz!" = Goku vs Raditz, 61 "Let Hercule win!" = C18 vs Mr. Satán...).

| Offset (u16) | Campo |
|---|---|
| +0x00 | número de misión |
| +0x02 | condición: -1 entrenamiento (cada uno el suyo: 1 = 30 golpes básicos, 2 = guardia/esquiva/teletransporte 5 veces, 3 = golpe que manda lejos + persecución...), 0 "Defeat" (KO, sin tiempo), 1 "Fight" (bajar su vida o aguantar el tiempo), 2 dejar ganar al rival (ring out), 3 "Endure" (sin atacar, vida por encima del nivel) |
| +0x04/+0x06 | acompañan a la condición (0/1, 1/2/3) |
| +0x08 | escenario: índice de la tabla del ELF 0x34A430 (0x5C B): 0 BUDO, 1 MIND & TIME, 2 ISLAND, 3 TOWN, 4 ROCK, 5 PLANE, 6 GOKU, 7 NAMEC, 8 CELL, 9 KAIOU, 10 BUU BODY, 11 ISLAND B, 12 TOWN B, 13 (infierno de las películas, sin nombre leído), 14 NAMEC B, 15 KAIOU B, 16 R&R BASE |
| +0x0A | ¿música? (en Dragon Road va con el escenario) |
| +0x0C | tiempo: 1 = 60 s, 3 = 180 s, 4 = sin límite, 5 = 120 s |
| +0x0E | 80-400 (¿recompensa/dificultad?) |
| +0x14 / +0x16 | jugador (ID de IW) / su forma o traje de IW |
| +0x26/+0x28 | vida (3000; 500 en la misión 10) |
| +0x3A / +0x3C | rival / su forma; +0x30..0x36 = 0xFFFF (¿compañeros?) |

Reparto de las 96: 10 entrenamientos, 67 "Defeat", 15 "Fight", 3 "Endure" (43, 55, 58), 1 dejar ganar (61). **No
hay minijuegos en la tabla**: vuelo (`# FLIGHT #`), disparo (`# SHOOT #`) y los módulos `MSN009/010/016/019`, `MSNF`...
son tareas aparte del ELF (los números 009/010/016/019 coinciden con misiones: reglas extra sin descifrar).
IDs de IW = B3 hasta el 43; del 44 en adelante (GT, Janemba, Pikkon, Pan, Gran Saiyaman 2, fusiones) solo IW.
Lector: `src/pasarela_iw.cpp` (busca la tabla por su numeración: vale carpeta o ISO); script de volcado
`E:/DBZ3HD_trabajo/F/dec.py`. Jugables en B3: **73 de 96** (entrenamiento 1 + todas las de combate con personajes de
B3); no disponibles: entrenamientos 2-10 (mandos), 67, 69, 73-77, 85-87, 92-94, 96 (personajes solo de IW).

**Ronda 3, frente P1 (2026-10-16): mapa de Dragon Mission PORTEADO.** Corrección: el `#DMD` de **1093** es el mapa
de Dragon Mission (no Dragon Road): las 96 misiones están en él con `+0x18 = 1` y clave `+0x20 = 0x7E1 + misión`
(comprobado registro a registro con la tabla del ELF); `+0x04` = saga (0 Saiyan, 1 Freezer, 2 Cell, 3 Bu, 4 GT,
5 Otra historia), `+0x08/+0x0C` = x, y en el lienzo de 1280x960. Las 18 de `+0x18 = 0` (claves 0x7D0-0x7E1) son los
minijuegos y su `+0x10` es la textura de `DATA_USA 334/5` (Snake Way, planeta de Kaio, Bubbles...). El 1092 (105 + 1,
hitos cada 15) es Dragon Road. Mapa por saga: Tierra azul (334/3 t09-t12) para Saiyan, Cell y Bu; Namek verde
(t22-t25) para Freezer; sepia (t48-t51) para GT (todas las casillas caen en tierra); espacio (t32-t35) para Otra
historia. `DATA_USA 334/6` = frase de cada fila del `#DMD` (149 = 149). Insignias "VS": `+0x10` 30-87 → textura de
334/5 por una tabla del juego (probablemente el `#AMA` 334/4, sin descifrar); en `pasarela_iw.cpp` va emparejada a
ojo (`kInsignia`). Goku del mapa = 334/5 t90; aro de selección t89; "New!" t16. Ficha y resultado con 334/1 (t00 panel,
t03 marco, t13-t17, cifras t19/t22, rangos t24) y 333/0 (globo t18, EARNED! t19, ZENI t07).

**Qué falta:** el nivel de vida de "Endure" y de "Fight", formas de IW → B3, las reglas de los módulos MSN y
de los entrenamientos 2-10 (contar guardias, esquivas, persecuciones: ganchos nuevos), Dragon Road (97-201).

**Viable: parcial. Coste: medio** (descifrado + condiciones). Assets: propios = tarjeta, marco, textos de
condición nuestros; del juego de origen = textos de misión (imágenes de `DATA_USA.AFS`, solo inglés).

## 3. Minijuegos

Tareas propias en el ELF, cada una con su bucle de actualización y dibujo: vuelo (`# FLIGHT #` 0x399D38, con
`ITEM%02d`, `CHECK%02d`, `GOAL`: carrera por puntos de control), disparo (`# SHOOT #` 0x39AD20), módulos de misión
`MSN009`, `MSN010`, `MSN016`, `MSN019`, `MSNF`, `MSNMW`, `MSNC2D`, `MSN CV`, escenas guionizadas `MSN_NB006`,
`NB007`, `NB012`, `NB013`, `NB014`, y vídeos (`[DMSR]`, `ending.sfd`).

**Viable: no por ahora. Coste: muy alto.** No son combates con parámetros: son juegos aparte (física, cámara,
objetivos) que habría que escribir de nuevo encima de B3. Lo que sí cabe: versiones "de combate" de algunos (p. ej.
"aguanta 30 s", "gana antes de 60 s") con el motor de pasos.

## 4. Propuesta de orden

1. Misiones de IW que son combates simples (después de descifrar +0x08/+0x0C y la condición), reutilizando todo el
   motor de la leyenda de Mr. Satán. 2. Textos de misión desde `DATA_USA.AFS` (mismo lector de #AMT).
3. Los minijuegos, solo si se decide hacer motores nuevos.

**Ronda 4, frente R1 (2026-10-16): escenas de la historia DESCIFRADAS** (ELF USA, `E:\DBZ3HD_trabajo\P4\iwdis.py`;
volcados en `E:\DBZ3HD_trabajo\R1\`). Las escenas de antes y despues de las misiones de IW **no son guiones**: son 22
videos (`USR/FD01A..FD20.SFD`, Sofdec MPEG + ADX, 3D renderizado) con los subtitulos en imagenes.
- Nombres de los videos: tabla del ELF `0x4811B0` (0x100 B cada uno; indice 0 FD01A, 1 FD01B, 2 FD02 ... 8 FD08A,
  9 FD08B, 10 FD09 ... 21 FD20). Tarea `# MS MOVIE #` (init `0x338ED0`, el indice llega en `gp-0x7214`).
- Subtitulos: **DATA_USA 77 + video** (tabla del ELF `0x4827B0` = 77..98, leida en `0x338F2C`); un `#AMT` por video,
  una textura por linea en orden (texto blanco con borde). Sin tiempos de linea en el ELF ni en el `#AMT` (el user
  data `<SUDPS_>` del video tampoco los lleva): ponytail.
- Que video va con cada mision: dos tablas del ELF que lee `func 0x2899F0` al acabar (estado sysMissionMovie
  `0x2AF100`): **antes** `0x472830` y **despues** `0x472890`, 16 x {tipo, indice, video} int16, -1 = fin. Tipo 1 =
  mision (indice = numero - 1), 0 = minijuego (indice = `+0x1C` del `#DMD` 1093):

  | Clave | Antes | Despues |
  |---|---|---|
  | minijuego 0 (Raditz se lleva a Gohan) | FD01A | - |
  | mision 11 Goku vs Raditz | FD01B | FD02 |
  | mision 23 Goku vs Vegeta | FD03 | FD04 |
  | misiones 37 / 38 Goku vs Freezer | FD05 / FD07 | FD06 / FD08A |
  | minijuego 10 (huir de Namek en la nave) | - | FD08B |
  | mision 49 Vegeta vs Cell | FD09 | FD10 |
  | mision 54 Gohan vs Cell | FD11 | FD12 |
  | mision 63 Goku vs Majin Vegeta | FD13 | FD14 |
  | mision 65 Vegeta vs Bu | FD15 | FD16 |
  | mision 70 Goku vs Bu nino | FD17 | FD18 |
  | mision 77 Gogeta SS4 vs Omega Shenron | FD19 | FD20 |
- Registro de mision del ELF (0x60 B), corregido: cabecera +0x00..+0x0F; **bloque del jugador +0x14..+0x38 y del rival
  +0x3A..+0x5E**, iguales: id, forma, 3 valores 0-4, 3 IDs (~512-1141), vida inicial (+0x24 / +0x4A; 3000 salvo el
  jugador de la 10 = 500), vida maxima (3000), 0/8, 7000, 5 valores (~38-444, -1 = nada), 0 y nivel (1-11, el mismo en
  los dos). Ninguna mision jugable cambia la vida; el nivel de "Endure" no esta en el registro (texto: "the specified
  level").
- `QteMg_FinalizeResult` (EU `0x32F0F0`, USA `0x32EDC0`; descompilacion Ghidra `E:\DBZ3HD_trabajo\ghidra\salida\`) =
  resultado de los minijuegos de botones (P4): 100/8000/8700/9400/10100/10800 x 0.6/0.8/1.0/1.3/1.6/2.0. No es la
  formula de puntos de las misiones (sigue sin localizar).
