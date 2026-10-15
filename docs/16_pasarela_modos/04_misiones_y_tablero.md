# 04 · Entrada "Pasarela" del HUB: saga de B1, tablero de B2, misiones y retos de IW

Fecha: 2026-10-15 (ronda 2, frente campañas). Código: `src/pasarela_misiones.cpp` (motor y pantalla), piezas comunes
en `src/pasarela.cpp` (`VigilarCombate`, avance, nombres, caras, personajes de mods) y lectores en
`src/pasarela_b1.cpp` (historia de B1, textos de IW). Sin ganchos `REX_HOOK_RAW` propios: combate por el conector
`dbz3::hub::RequestBattleEx` y resultado leído de memoria. Solo US.

## 1. Qué ve el jugador

En el HUB (página 2) la entrada **Pasarela** abre un menú con el marco, la franja y la letra de los menús del juego:

| Modo | Ficha | Necesita | Qué es |
|---|---|---|---|
| Saga de Vegeta en Namek (Budokai 1) | 04 | ISO de B1 + Zarbon importado de B1 | los 4 combates de la historia de Vegeta en Namek de B1 (tabla de la ISO): Zarbon transformado, Recoome, Freezer y Freezer en su forma final, en Namek, con el ki inicial del rival de B1 |
| Tablero de la saga Saiyan (Budokai 2) | 17 | nada | 6 casillas propias en un grafo: Kame House, Llanura (Saibaman), Torre de Karin (semilla), Desfiladero (Nappa), Templo de Kami (semilla), Campo de batalla (Vegeta). LB/RB o la cruceta eligen el camino, A mueve; la vida se arrastra entre combates, las semillas la llenan, Vegeta exige haber vencido antes a los dos; perder un combate = **fracaso** (vuelta a Kame House y casillas de nuevo) |
| ¡Fusión fallida! (Gotenks gordo) | 20 | Gotenks gordo importado de B2 | misión nueva: charla con Piccolo, Gotenks gordo contra Majin Bu en la ciudad y contra Súper Bu en la sala del espíritu y del tiempo |
| Las absorciones de Súper Bu | 21 | los 3 trajes de Bu absorbido | Bu vence a Piccolo, Goten y Gohan; cada victoria lo absorbe y antes del combate siguiente se elige con qué absorción luchar |
| Retos de Infinite World | 02 C | copia de IW (ISO o carpeta) | "Training No.1" (30 ataques básicos en 180 s) y "Endure Gohan's attacks!" (60 s sin atacar, vida ≥ 1000), con el título y el texto de la misión de la copia del jugador |

Si falta algo, la fila sale en gris y el bocadillo dice qué hace falta (no empieza). B vuelve al HUB; el avance de
cada modo se guarda en `user_data/dbz3/pasarela/progreso.toml` (claves `saga_b1`, `tablero_*`, `gotenks`, `buu`,
`buu_absorbidos`, `iw`).

## 2. Cómo funciona

- **Pasos** (`Paso`): charla (cara del juego o del mod + bocadillo), tarjeta del combate ("VS", aviso), combate,
  elección (Bu), informe (IW) y final. El tablero arma los suyos al llegar a una casilla.
- **Combate**: `RequestBattleEx(jugador, rival, escenario, COM, tiempo, vida, vida, forma, forma)`; el resultado y las
  reglas, en `VigilarCombate` (hilo propio, solo memoria): estado `mgr+35128`, ganador `mgr+35136`, vida `X+6080`,
  ki `X+6100`, tiempo `mgr+2028` / `mgr+35144`, nivel de la IA del COM `mgr+35573`.
- **Cápsulas de serie**: el conector deja las listas vacías (nadie lanzaba técnicas); `VigilarCombate` pone a cada
  lado la lista Normal de su personaje (tabla `0x8234ABB8 + 96·ID`, +80/+82, la que el Torneo da al COM) en la CFG
  (`rec+52/+54`) en cuanto el combate es nuestro (el juego la copia el fotograma siguiente). Igual el **traje**
  (`rec+18`). Ponytail: carrera de un fotograma con el hilo del juego; propuesta limpia al dueño de `hub.cpp`.
- **Personajes de mods**: `mods/_roster/roster.toml` + `personaje.toml` de cada mod (`origen_fuente`/`origen_clave`
  de `importar.py`): Zarbon = (b1, 13), Gotenks gordo = (b2, XGXL). El ID cambia en cada instalación (en la copia de
  pruebas son 22 y 23; en `aura_test` Gotenks gordo es el 45).
- **Formas** (tabla de combate `0x82329CF0 + 372·ID`: +208 cuántas, +212 + 20·f cápsula, modelo, tipo; medidas en el
  juego): Freezer 0-4 (B1 1 = final → 3, B1 2 = 100 % → 4), Goku 4 = SSJ3, Gohan 3 = definitivo, Goten 1 = SSJ.
- **Escenarios** (`0x82323760 + 92·i`, +0x10 mapa): 0 BUDO 46, 1 MIND & TIME 58, 2 ISLAND 56, 3 TOWN 68, 4 ROCK 64,
  5 PLANE 62, 6 GOKU 52, 7 NAMEC 60, 8 CELL 50, 9 KAIOU 54, 10 BUU BODY 48, 11 ISLAND B, 12 TOWN B 66, 13 EARTH 64,
  14 NAMEC B, 15 KAIOU B, 16 R&R BASE 44.

## 3. Saga de B1 (ficha 04)

La tabla de la historia de B1 son los 43 registros de 128 B de antes de la de Mr. Satán (misma estructura; europea:
`0x35CA40`; +0x20 protagonista, +0x28 capítulo, +0x70 modo 0/6/7). Hilo de la saga = el primer registro contra
Zarbon (B1 13) y los siguientes del mismo protagonista con capítulo +1: Vegeta contra Zarbon (forma 1), Recoome,
Freezer (0) y Freezer (1). Charlas: textos propios (las escenas de B1 no se copian). Personajes de B1 que B3 no
tiene (Zarbon 13, Dodoria 14) = el mod importado; los demás, el donante de `importar.B1_DONOR`.

## 4. Absorciones de Súper Bu (ficha 21)

Las absorciones de B3 son formas especiales de Súper Bu (tipo 7, cápsula 123 "Absorción": modelos 84 = Piccolo, 78 =
Gotenks, 79 = Gohan) que solo nacen en combate: **pedidas como forma inicial, el juego se cae al cargar** (medido,
`20261015-091834_campanas_test_forma_captura`). Por eso cada absorción es un **traje extra de Súper Bu** con el
modelo del juego del Bu absorbido (`mod center hd/campanas/trajes_bu/bu_absorbido_*/traje.toml`, `"juego:N"`: el
mod no trae arte). En la selección es un traje más; el primero sigue siendo el Bu de siempre. Formas 1-3 del traje =
las absorciones de serie; 4-7 (modelos de Piccolo, Gotenks, Gohan y Bu fantasma) = su modelo de serie, para no
cambiar a esos personajes (ver propuesta a `roster_ext.cpp` en `docs/14_frentes/campanas.md`).

## 5. Retos de Infinite World (ficha 02 C)

`DATA_USA.AFS` de IW (USA): desde la entrada 132, un `#AMB` por misión `[#AMT texto, #AMT título]` en el orden del
juego (132-141 "Training No.1-10", 142 "Defeat Raditz!", 149 "Fight Nappa!" = 120 s, 174 "Endure Gohan's attacks!",
192 "Let Hercule win!"...; hoja de títulos: `E:/DBZ3HD_trabajo/campanas/iw_titulos.png`). El lector vale para una
carpeta extraída o la ISO. Retos:

- **Training No.1**: "Perform 30 Basic Attacks on your opponent! The time limit is 180 seconds!" → combate de 180 s
  contra Krilin (COM en Novato); cada bajada de la vida del rival es un golpe; al llegar a 30 el combate acaba a
  favor (aviso de ring out `mgr+35569` = 1).
- **Endure Gohan's attacks!**: "Without performing any attacks keep your health above the specified level... 60
  seconds" → 60 s contra Gohan joven; si el jugador quita vida, falla (ring out en contra); se cumple acabando el
  tiempo con 1000 o más, con el COM en Novato (ponytail: el nivel exacto de IW está sin descifrar).

`#DMD` (`DATA_CMN` 1092): +0x08/+0x0C son la posición del icono en el mapa de misiones, +0x10 el rival (IDs de IW =
los de B3 para los comunes); las condiciones no están ahí (siguen sin localizar en el ELF).

## 6. Pruebas

Escenarios en `pruebas/` (copia `D:\DBZ3HD\campanas_test`: `pasarela_test` + `mods` con Zarbon de B1, Gotenks gordo
y los 3 trajes de Bu, roster con `roster_build.py construir --mods`, `ps2_games` = unión a `D:\DBZ3HD\ps2_games`).
Resultados y capturas: `docs/14_frentes/campanas.md`.
