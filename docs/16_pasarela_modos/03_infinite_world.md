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

**Qué falta:** descifrar el tipo de condición, el personaje del jugador, el tiempo y la vida de cada fila (buscar
el código MIPS que lee los registros de 0x50 B: con Capstone, como en 01 §2); las condiciones distintas de
"ganar" necesitan leer el combate (tiempo, ring out: ya se leen estado y ganador) y algunas, ganchos nuevos (p. ej.
"gana con una definitiva").

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
