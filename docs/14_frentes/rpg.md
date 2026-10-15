# Frente: RPG (ronda 2, 2026-10-15) — retos de rol del HUB

Solo US (con `g_guest_eu` todo queda apagado, como `modos.cpp`). Copias de prueba: `D:\DBZ3HD\rpg_test` y
`D:\DBZ3HD\rpg_test2` (con `res/ingame_ui` copiado). Escenarios: `E:\DBZ3HD_trabajo\rpg\escenarios\*.toml`.

## Qué ve el jugador
- **HUB > Torre / Supervivencia / Kumite** abren ahora un **selector de retos** con el aspecto del Duelo (fondo,
  Kaio, cinta, marco, franja, letra y botones del juego): arriba el luchador (cara del marcador y flechas;
  izquierda/derecha lo cambian), la lista de retos y el bocadillo con qué es cada uno, el récord, el monedero de
  Zenie y el rango del jugador. A empieza, B vuelve al HUB. El reto clásico sigue el primero de cada lista.
  - Torre: Torre (clásica) · **Torres del Tiempo** · **Abyss**
  - Supervivencia: Supervivencia (clásica) · **Supervivencia con tienda** · **Simulación**
  - Kumite: Kumite (clásico) · **Kumite de jefes** · **Torneo de 4** · **Modo Héroe**
  Un reto empezado sale como "(a medias)" y A lo sigue donde se dejó.
- **Todos los retos encadenan los combates sin pasar por el menú principal** (`hub::ChainBattleEx` del
  coordinador): tras el WINNER! la pantalla entre combates sale sobre negro. Clásicos: "Oleada 2 de 10:
  ¡superado!" y a los 2,5 s el siguiente. Retos de rpg.cpp (tienda, cartas, cuadro, misión del héroe, torres,
  jefes): su pantalla con mando en la espera en negro; A = siguiente combate; **B = guardar y salir** vuelve al
  menú principal (`CancelChain`) con la pantalla "Partida guardada" y después al HUB.
- **Supervivencia (09)**: 10 oleadas; las 3, 6 y 9 con 60 s; las 4, 7 y 10 con un rival que se regenera
  (25/s). Se ve en la pantalla entre oleadas y bajo el marcador durante la pelea. Al completarla, **premio de
  1000 Zenie** (pantalla final "Premio: 1000 Zenie").
- **Simulación (23)**: 3 días (Entrenar +15% daño/+1 fatiga/-300 vida; Explorar con la semilla de la partida:
  cápsula Potential, +500 vida máx., emboscada -500 o cápsula Spirit!!; Descansar +1500 vida y sin fatiga). El
  día 4 solo queda "¡Al combate contra <jefe>!": el jefe (Cell, Broly o Buu niño según la semilla, 4000 de vida,
  99 s) con la vida, la fuerza (+% de daño) y las cápsulas encontradas; la fatiga hace recibir +10% por punto.
  B guarda y sale; la partida se rehace desde semilla + eventos. Premio 500 Zenie.
- **Supervivencia con tienda (24)**: 5 combates con la misma vida. Cada victoria +100 Zenie (+50 si no te
  tocan). Tienda entre combates: Cura +1000 (60 Z, no si la vida está llena), Fuerza +25% y Defensa -25% (80 Z,
  solo para el combate siguiente, una vez). Sin Zenie suficiente o repetida: "No puedes comprarlo ahora". Al
  final, "Zenie sin gastar" queda como récord (no se convierte en nada). Perder acaba la partida (sin revancha).
- **Modo Héroe (25)**: una familia (el cuerpo de Goku), 3 variantes prefabricadas con su traje del juego y su
  perfil (Guerrero +15% daño, Guardián +1000 vida, Viajero -20% daño recibido; izquierda/derecha) y 2 técnicas
  de 3 compatibles (Kamehameha, Genki Dama, Warp Kamehameha; LB/RB), equipadas solo para la misión (la partida
  del juego no se toca). Misión original de 3 combates (Saibaman, Nappa, Vegeta) con su texto en el bocadillo.
- **Torres del Tiempo (26)**: torres de 3 encuentros con tiempo y una regla: Saiyan (suelo que drena el ki),
  Androide (el rival se regenera), Ginyu (+50% daño recibido), Majin (rivales blindados -30%). LB/RB: la de la
  semana (rota cada lunes, semana UTC, fija hasta acabar aunque cambie la semana) o una de las dos fijas.
  Revancha con "Battle Again?". Premio 600 Zenie.
- **Abyss (27)**: 3 pisos (rival por semilla); tras el 1 y el 2, 3 cartas de 5 (Fuerza +25%, Piel de acero
  -25%, Vitalidad +1000, Ki eterno, cápsula Rage!!!), una elección por piso que queda guardada; la oferta sale
  de la semilla (reabrir no la cambia). Perder acaba la partida. Premio 800 Zenie.
- **Kumite de jefes (28)**: Cell (armadura 50%, hueco de 3 s cada 10 s, +50% de daño recibido), Broly (60%,
  cada 8 s, +75%), Buu niño (aguanta 45 s = victoria, armadura 70%). Vida finita (5000/6000/7000). Bajo el
  marcador: "Armadura" o "¡Ahora!" (el hueco). Revancha con "Battle Again?". Premio 1500 Zenie.
- **Torneo de 4 (03)**: cuadro propio con las caras del juego (jugador + 3 bots al azar con la semilla).
  Semifinal real; la otra semifinal se decide por la fuerza (Elo del ranking) y la semilla; final real; pantalla
  de campeón. Premio 600 Zenie.
- **Laboratorio** (propuesta del frente combate): la pantalla del HUB enseña, además del resumen del último combate,
  la **frame data de los últimos 6 golpes** (luchador y código del ataque, arranque en cuadros, daño, ventaja y si fue
  golpe o guardia; medida en el juego como `frame_data.py --medido`) y tiene tres filas: **Practicar contra el
  muñeco** (combate sin tiempo en el prado contra el rival del último Duelo, con el muñeco de `combate.cpp`: no
  ataca), **Muñeco: quieto / en guardia** y la captura por fotograma. Se sale desde la pausa (Reelegir o Menú
  principal) o tras el menú final, y se vuelve a la pantalla del Laboratorio con la tabla. El ajuste del muñeco se
  pone solo para ese combate y se devuelve como estaba (si el juego se cierra a mitad, al abrirlo).
- **Reglas de escenario** (propuesta R1 del frente escenarios): cada combate de un reto puede llevar `clima`
  (el cielo se oscurece, "Tormenta"), `gigante` (el rival a esa escala, 0.5..3) e `intro` (cine de 10 s). En el
  Kumite de jefes, **Broly es el jefe gigante** (1,8) con tormenta y su armadura del 60%; en el catálogo,
  `campanas/retos/nativo_kumite_jefe_gigante.toml` (Cell, Broly y Buu niño gigante, con tormenta e intro). Se
  quitan al acabar el reto (y el combate siguiente pone las suyas). Las pantallas entre combates dicen el nombre
  del escenario del juego ("Siguiente: Broly (Montañas rocosas)") con `dbz3::escenarios::Nombre`.
- **Sonidos**: todas las pantallas de `modos.cpp`/`rpg.cpp` (selector, tienda, cartas, cuadro, entre combates,
  final, Ranking, Laboratorio) suenan con los del menú del juego (`hub::QueueSe`: mover 2, aceptar 3, atrás 5).
  Suenan cuando el menú principal está debajo (en la espera en negro entre combates encadenados, no: lo dice la API).
- **Ranking (01)**: nueva primera página "Tu perfil": rango (Guerrero Z, Super Saiyan, Kaio, Kaio-shin,
  Shenron cada 100 de Elo, con su medalla), Elo y pico, victorias-derrotas y luchador favorito; páginas para los
  retos nuevos con marcas. El selector enseña "Rango: Kaio (1210)". Se escribe
  `user_data/dbz3/online_profile.toml` (para el panel Online).

## Ficheros
- `src/rpg.cpp` (nuevo; en `DBZ3_SOURCES` y en el `REMOVE_ITEM` de la build solo EU), `src/modos.h` (nuevo: API
  común de `modos.cpp` y `rpg.cpp`), `src/modos.cpp`.
- `mod center hd/modos_reto.py`: `nativo ... --premio N`; la supervivencia por defecto lleva los modificadores
  por oleada y el premio (igual que el juego); selftest lo comprueba.
- `mod center hd/campanas/retos/nativo_supervivencia.toml` regenerado (premio y modificadores) y
  `nativo_kumite_jefe_gigante.toml` nuevo (`--clima --gigante 1.8 --gigante-en 2 --intro`).
- Usa (de otros frentes, sin tocarlos): `hub::ChainBattleEx/HoldChain/CancelChain`, `hub::QueueSe`,
  `escenarios::Nombre`, `clima::Activar`, `gigantes::Activar`, `cine::Pedir` y el ajuste
  `dbz3_combate_prueba_muneco` de `combate.cpp`.

## Motor (modos.cpp, para rpg.cpp)
- `StartPropio(tipo 3-9, nombre, palabra, jugador, traje, vida, vida_max, peleas, ganados, reintentar, premio)`,
  `Continue`, `Abort`, `SetVida`, `SetReglas`, `SetSiguiente`, `CerrarFin`, `ArmaduraActiva`, `PerfilJugador`.
- `Reglas` por combate (se quitan al acabar el combate y el reto): `dano_hecho`/`dano_recibido` (%),
  `armadura` (%) con `ventana` (s; los 3 últimos de cada ciclo sin armadura), `ki_drena` (% del tope por segundo,
  < 0 lo recarga), cápsulas del combate (`ncaps`/`caps`, sumadas a las del jugador o `reemplazar`).
  Se aplican en `sub_820FD678` (ya de modos.cpp) antes de la comprobación del KO: cada bajada de vida se escala;
  un golpe que tumba (vida 0) no se toca. Cápsulas: CFG rec+52/+54 en `sub_821D7400`. Log:
  `dbz3 modos: reglas: golpe al rival 120 -> 60 (50%, con armadura)`.
- Sin revancha (`reintentar = false`): tras perder, el menú final elige solo "Reselect" (`sub_820FF750`).
- Encadenado: `Encadenar()` pide `hub::ChainBattleEx` en `sub_820FEBA8` (antes de soltar el combate: dentro va
  `sub_821D3628`) con `HoldChain(true)`. Clásicos: a los 2,5 s `Lanzar()` hace `HoldChain(false)`. Retos propios:
  `Continue()` (A) lo suelta y `Abort()`/`Terminar()` llaman a `hub::CancelChain()` (vuelta al menú principal).
  La CFG del combate pone rival y escenario del reto (el finalista del Torneo se decide en la espera).

## Datos guardados
- `user_data/dbz3/rpg.toml` (versión 1; `.tmp` + rename y la anterior en `.bak`; si no se lee se aparta a
  `.roto` y se usa el `.bak`; si es de una versión más nueva no se toca y el selector lo avisa): `zeni`
  (monedero del HUB), `tienda_record`, la receta del héroe y `[[partida]]` por reto a medias (semilla, eventos,
  vida, Zenie de la tienda, torre, bots...). La partida del juego (`4E4D0856/00000001`) no se toca.
- `hub_modos.toml` (de modos.cpp): marcas de los tipos 3-9 (`simulacion`, `tienda`, `heroe`, `torres`, `abyss`,
  `jefes`, `torneo4`).

## Puntuación (0-1) y pruebas (capturas en `D:\DBZ3HD\_pruebas\`)
Todas con `DBZ3_TEST_RINGOUT=1` (gana el jugador a los 5 s) y `DBZ3_HUB_MODE=5` (sin presentación), entrando
por el HUB con el mando (teclas), salvo donde se dice.
| Ficha | Nota | Prueba |
|---|---|---|
| 01 Ranking (perfil + rango) | 1 | `20261015-102109_rpg_test2_galeria`: `ranking_perfil` (Kaio, Elo 1196, 19-1, Goku), `ranking_elo`, páginas; `selector_kumite` ("Rango: Kaio (1196)"). Falta solo enseñarlo en la sala Online (propuesta 5). |
| 03 Torneo de 4 con bots | 1 | `20261015-101252_rpg_test_torneo` y `..._galeria`: `cuadro`, `cuadro_final` ("C-18 ha ganado la otra semifinal"), `campeon`; la final es contra el ganador (log `contra id 18`). |
| 09 Supervivencia | 1 | `20261015-100014_rpg_test2_supervivencia09`: 10 oleadas **encadenadas** (log `combate nativo encadenado`), `entre_2_mods` ("Tiempo 60 s"), `oleada3_tiempo`, `oleada4_regen` ("Regenera 25/s", log `regen 25/s`), contador "Oleada 4/10", log `premio de 1000 Zenie`. |
| 23 Simulación (3 días + jefe) | 1 | `20261015-090944_rpg_test2_simulacion`: `dia1`..`dia3`, guardar y salir, `selector_a_medias`, `dia3_seguida`, `jefe` (vida 3000 tras descansar, fuerza +15%), combate con la cápsula encontrada (log `capsulas del jugador 0 -> 2 (366 0 ...)`: la cuenta salía doble por una macro; arreglado y comprobado en Abyss y Héroe), `final`. |
| 24 Supervivencia con tienda | 1 | Encadenada: `20261015-104834_rpg_test_tienda_cadena` (`tienda_1_negro`: la tienda sobre negro, sin menú; compra; combate 2 `encadenado`; B en la espera en negro -> `guardada_menu` "Partida guardada" -> `vuelta_hub` -> `selector_a_medias` -> combates 3-5 encadenados -> `final`). Antes, por el menú: `20261015-090944_rpg_test_tienda`: `tienda_1` (100 Z), compra de Fuerza (log `compra 1 por 80 (quedan 20)`; el combate 2 sale con `dano 125%`), `tienda_1_doble` ("No puedes comprarlo ahora"), guardar y salir (`guardada`), seguir (`selector_a_medias` -> combate 3), 5/5 y `final` ("Zenie sin gastar: 320"). |
| 25 Modo Héroe | 1 | `20261015-101252_rpg_test2_heroe`: `heroe_guerrero`, `heroe_guardian` (traje 1), `heroe_tecnicas` (Kamehameha + Warp Kamehameha), log `capsulas del jugador 0 -> 2 (6 9)` y `traje 1 vida 4000/4000`, `mision1` (Goku de azul), `entre_1/2` con el texto de la misión, `final`. Límite: las variantes son trajes del juego (no mallas nuevas) y las técnicas se equipan como en las pruebas de técnicas de los ports, sin lanzarlas a mano en la prueba. |
| 26 Torres del Tiempo | 1 | `20261015-090413_rpg_test_torres` (torre de la semana: Majin, armadura 30%, 99 s; `selector_torres`, `piso1_hud`, `entre_1/2`, `final` con premio 600) y `20261015-103629_rpg_test_torre_saiyan_ki` (torre fija Saiyan con RB: log `ki del jugador 3000 -> 2500 / 7000 (-4%/s)`, reloj 60 s). |
| 27 Abyss | 1 | Encadenado: `20261015-104834_rpg_test2_abyss_cadena` (`cartas_1_negro`, `cartas_2_negro`, premio 800). Antes: `20261015-094025_rpg_test_abyss`: `cartas_1` (3 cartas), carta Rage!!! (log `capsulas del jugador 0 -> 1 (380)`), `cartas_2`, Vitalidad (combate 3 con `vida 4000/4000`), `final` y premio 800. |
| 28 Kumite de jefes | 1 | `20261015-102109_rpg_test_jefes` (golpes reales con X): log `golpe al rival 30 -> 15 (50%, con armadura)`, `golpe al rival 15 sin armadura (hueco)`, `golpe al jugador 81 -> 122 (150%)`; `armadura_a..d` ("Jefe 1/3", reloj 120, "Armadura"). Flujo de los 3 jefes: `20261015-103629_rpg_test2_jefes_flujo` (vida 5000/6000/7000, armadura 50/60/70%, `entre_2` "Objetivo: aguanta 45 s", `jefe3_aguanta` con reloj 46, "¡Aguanta!" y "¡Ahora!", premio 1500). |

Batería `python tools/pruebas.py todo`: 5/5 PASS a las 10:4x (tras el arreglo de la herramienta del coordinador). Con
todo lo último (13:00): 4/5 y `tag_swap` falló en el menú de cápsulas del Tag Team (otra parte del juego); repetido
solo (`todo --solo tag_swap`): PASS.
`python "mod center hd/modos_reto.py" --selftest`: OK (temporales en `E:\DBZ3HD_trabajo
pg	mp`).

### Jefe gigante y nombres de escenario
- `20261015-125725_rpg_test2_jefe_gigante`: `entre_1_escenario` ("Siguiente: Broly (Montañas rocosas)",
  "Armadura 60% - ¡Gigante!"), `broly_gigante_a/b` (Broly a 1,8 con tormenta, bajo el marcador "Rival gigante -
  Tormenta" y "Armadura"; log `CFG: clima true gigante 1.8 intro false`), `jefe3_normal` (Buu niño de tamaño
  normal: la regla se quitó), `final`.
- `python "mod center hd/modos_reto.py" --selftest`: OK con `clima`, `gigante` (y `--gigante-en`), `intro`, sus topes y
  el reto nuevo del catálogo.

### Laboratorio (frame data y muñeco) y sonidos
- `20261015-113934_rpg_test_lab_practica`: `lab_inicio`, práctica contra el muñeco quieto (log `practica id 0
  contra el muneco id 7 (quieto)`, `muneco en el lado 2`), salida por el menú final (`salir_confirmar`) y
  `lab_tabla_quieto`; log de sonidos `sonido 5/2/3`.
- `20261015-114728_rpg_test_lab_guardia`: muñeco en guardia, salida desde la pausa (`pausa_s3`: Continuar batalla,
  Ver habilidades, Reelegir person., Menú principal) y `lab_tabla_guardia`: patada `210` **11 / 50 / +2 golpe** y
  **11 / 25 / +5 guardia**, lo mismo que mide el frente combate (`combate.md` §12).
- En `20261015-115516_rpg_test_lab_guardia` el muñeco "en guardia" no bloqueó nada (y la tabla lo dice: golpe, 50):
  el bloqueo del muñeco de `combate.cpp` no sale siempre (propuesta 6).
- Encadenados con la pantalla en la espera en negro: tienda y "B = guardar y salir" (`20261015-104834_rpg_test_tienda_cadena`),
  Abyss (`20261015-104834_rpg_test2_abyss_cadena`) y Torneo con el finalista puesto en la espera
  (`20261015-113934_rpg_test2_torneo_cadena`: pedido con el 1, la CFG pone el 35 Super Buu).

## WL (casos de `studies/11_vision_futura/INTEGRACION_SEGURA.md`, última vuelta)
Escenarios `E:\DBZ3HD_trabajo\rpg\escenarios\wl*.toml` (los genera `gen_wl.py`); los que se reanudan corren seguidos
en la misma copia (`E:\DBZ3HD_trabajo\rpg\wl_cadena.sh`), con el juego cerrado entre uno y otro.

| Caso | Resultado | Prueba |
|---|---|---|
| WL-23 Simulación: un día por acción; buffs y cápsulas no pasan al juego normal | **PASS** | `20261015-153624_rpg_test_wl23`: dos pulsaciones seguidas = un día (log `pulsacion repetida ignorada`; se pide 0,5 s entre elecciones); el jefe con +45 % y fatiga 3 (log `golpe al rival 50 -> 72 (145%)`, `golpe al jugador 78 -> 101 (130%)`); después, práctica del Laboratorio con el mismo Goku: sin una sola línea `reglas:` ni cápsulas del reto, patada sin el +45 % (`lab_tabla`). |
| WL-24 Tienda: Cura, saldo y vida tras cerrar y reanudar, sin premio ni compra repetidos | **PASS** | `20261015-153624_rpg_test2_wl24a`: victoria +100, Cura (log `compra 0 por 60 (quedan 40 Zenie, vida 3000/3000)`) y el juego se cierra en la tienda. `20261015-154931_rpg_test2_wl24b`: al reabrir, "(a medias)", la tienda vuelve **antes** del combate con `Vida 3000 / 3000 - 40 Zenie` (`tienda_reanudada`), y tras el combate 2: 140 Zenie (40 + 100, no se repite el premio del 1). **Arreglado**: antes, reanudar saltaba directo al combate siguiente (sin tienda). |
| WL-25 Héroe: técnicas de verdad, modelo en movimiento, Custom aislado | **PASS** | `20261015-164757_rpg_test2_wl25c` (el héroe lo lleva el ordenador con ki lleno, `DBZ3_COMBATE_LADOS=2` + `DBZ3_TEST_KI_LLENO=1`): en combate solo lleva sus técnicas (log `en combate: lado 1 con ... (6 9 ...)`, sin el Kaioken ni la Genki Dama de la partida), lanza la técnica (log `tecnica: lado 1 codigo 24A ki 7000 -> 6000 (forma 0 -> 0)`, `tecnica_2`: cámara de la técnica, 180 de daño) y `mov_1..3` (traje 1 en movimiento, sin fallos de modelo: son trajes del juego). `20261015-163841_rpg_test2_wl25b`: después, Goku vuelve a pelear con su Custom de la partida (`1 6 10`). La partida del juego no cambia: `data.bin` igual antes y después (sha1 `1fb04dbb…`). |
| WL-26 Torres: rotación semanal con otra fecha; la partida fija su torre; el reloj no toca rankings | **PASS** | `20261015-154901_rpg_test_wl26a` (semana simulada 2964: torre de la semana **Ginyu**, piso 1 y cerrar), `20261015-155045_rpg_test_wl26b` (semana 2965: la partida sigue en **Ginyu**, "(a medias)" y reanuda) y `20261015-155607_rpg_test2_wl26c` (semana 2965 sin partida: **Majin**). **Arreglado**: la torre de la semana salía casi siempre la misma (el hash daba Majin de la 2963 a la 2968); ahora alterna cada semana. No hay ranking remoto: los récords son locales (`hub_modos.toml`) y el Elo sale de `combates.csv`, sin fechas. |
| WL-27 Abyss: reanudar conserva ofertas, semilla y llave | **PASS** | `20261015-155109_rpg_test2_wl27a` (piso 1 y cerrar con las cartas en pantalla: Piel de acero, Ki eterno, Vitalidad) y `20261015-155243_rpg_test2_wl27b` (al reabrir, **las mismas tres cartas**; se elige Ki eterno → `rpg.toml` `eventos = '3'`, misma `semilla`, y el combate 2 sale con `ki -5/s`). Mismo arreglo de reanudar que WL-24. |

Batería `python tools/pruebas.py todo` con todo esto: **5/5 PASS** (16:5x).

Código de esta vuelta (aviso al frente eu: `modos.cpp`/`rpg.cpp` tocados después de su conversión, sin direcciones nuevas
del juego; solo desplazamientos de luchador ya usados: `X+2928`, `X+2980/2984`, `X+6100/6104`, `X+7412`):
`StartPropio(..., pausado)` (reanudar entre combates), un día por pulsación en la Simulación, torre de la semana por
semana, `DBZ3_TEST_SEMANA`, `DBZ3_TEST_KI_LLENO` (pruebas) y el log `tecnica:`/`en combate:`.

## Lo que queda
- Las pantallas en la espera en negro (entre combates encadenados) no suenan: `QueueSe` solo suena con el menú.
- Textos en 5 idiomas (es, en, fr, de, it); japonés cae a inglés (la letra del menú no tiene esos glifos).
- Torneo de 4: la otra semifinal no se juega (se decide por Elo + semilla); COM contra COM necesitaría `pad[0] = -1`
  en `hub.cpp`.
- Héroe: sin mallas nuevas; Tag Team y online no aplican a estos retos.

## Propuestas para ficheros de otros frentes (no tocados)
1. **HECHO por el coordinador** (`src/hub.cpp`): `CancelChain()` también en la espera en negro (fase 4); los retos
   de rpg.cpp ya encadenan. Diff aplicado:
   ```cpp
   void CancelChain() {
     g_chain = false;
     int cuatro = 4;
     if (g_phase.compare_exchange_strong(cuatro, 0)) g_hold = false;   // sub_821D7D18 sigue: crea el menu principal
   }
   ```
   Con eso, en `modos.cpp` el reto propio usaría `Encadenar()` también y `Abort()` llamaría a `CancelChain()`.
   El marcador "V 1" en los combates encadenados se queda (cuenta los pisos ganados: encaja).
2. **HUB (`src/ingame/hub_panel.cpp`)**: el aviso "Guardando datos del HUB / Guardado con éxito" sale encima del
   principio de cada combate de un reto (modos.cpp guarda el reto a medias al pedir cada combate). Que no se
   dibuje con la pantalla 25 (combate) o que espere a volver al menú. Captura:
   `D:\DBZ3HD\_pruebas\20261015-090413_rpg_test_torres\piso1_hud.png`.
3. **HUB (`hub_panel.cpp`, bocadillo)**: Torre / Supervivencia / Kumite ahora abren el selector; el bocadillo
   podría decir los retos de cada entrada ("Torre, Torres del Tiempo y Abyss") y el monedero (`rpg.toml`, `zeni`).
4. **HUB (`hub_menu.cpp`)**: una cola de sonidos del menú para las pantallas propias, p. ej.
   `void QueueSe(int id)` que `Frame()` reproduzca con `PlaySe` en el hilo del juego (mover 2, aceptar 3, atrás 5).
   Hoy las pantallas de modos/rpg son mudas (la interfaz no puede llamar al juego desde su hilo).
5. **Online (`src/ingame/online_panel.cpp`)**: "reto que muestra el rango" (ficha 01): enseñar en la sala el rango
   del jugador con `dbz3::modos::PerfilJugador(&elo, &tramo, &nombre)` (modos.h) o leyendo
   `user_data/dbz3/online_profile.toml` (`rating`, `tramo` 0-4, `wins`, `losses`); medalla
   `retos/medalla_<tramo+1>_*`.
6. **Combate (`src/combate.cpp`)**: el muñeco "en guardia" (`dbz3_combate_prueba_muneco = guardia`) no siempre se
   pone en guardia (dos prácticas iguales: una bloqueó, otra no; `20261015-114728_...` y `20261015-115516_...`).
   Quizá la guardia solo se pide con la acción 0 y el contexto `X+6048` listo justo en ese cuadro. Y para el
   Laboratorio vendría bien `dbz3::combate::SetMuneco(int)` sin pasar por un ajuste guardable.
   (Propuestas 2 y 4: HECHAS por el frente hub — aviso de guardado fuera de las pantallas y `QueueSe`.)
