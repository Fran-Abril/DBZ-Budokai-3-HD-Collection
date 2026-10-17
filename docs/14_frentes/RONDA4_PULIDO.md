# Ronda 4: pulido de los modos jugables (2026-10-16)

Objetivo del usuario: recrear con total fidelidad (ingeniería inversa + port) la campaña de Infinite World (mapas,
scripts, animaciones), la campaña de Budokai 2 + Nave de Babidi, una Leyenda de Mr. Satán pulida con animaciones y
cinemáticas hechas con los modelos de Budokai 3, una beta del Modo Héroe con movesets cambiables en su apartado, y
pulir los modos añadidos y la posición de todo lo que dibujamos (HUD). Estudiar la ROM de Tenkaichi 3 por si sirve.
Contexto: `PORT_EXACTO.md` (qué hay hecho de cada port y qué falta), `RONDA3_ENJAMBRE.md` (frentes anteriores).

## Reglas
Las 8 de `RONDA3_ENJAMBRE.md` siguen valiendo (tus archivos solamente; releer antes de cada Edit; compilar solo con
`python tools/compilar.py`; probar con `python tools/pruebas.py` en tu copia; nunca borrar; nada de publicar, commit,
push ni tocar `out/build/win-amd64-release`; temporales en `E:\DBZ3HD_trabajo\<frente>\`; subagentes haiku para buscar).
Además:
- **Port, no imitación.** Datos, textos, reglas, gráficos y tiempos del original, leídos en marcha de la copia del
  jugador (`AfsPs2`, `BuscarJuego`, ISOs en `out/build/win-amd64-release/ps2_games`). Si algo no se encuentra, dilo y
  márcalo `ponytail:`; no lo inventes. Si portar exige días, haz la recreación exacta de extremo a extremo y dilo.
- **Personajes en 3D = modelos de B3** (el motor del juego), nunca dibujos sueltos cuando el original es 3D.
- **Disposición común:** todo panel propio usa los tamaños de `S::mode` de `src/ingame/game_style.h` (los afina R6).
- `CMakeLists.txt`: si creas un `.cpp`, añade solo su línea en las dos listas (la normal y la de `REMOVE_ITEM` de EU),
  releyendo justo antes.
- Plazo de esta vuelta: 60 minutos. Al acabar: compila, escenario de prueba PASS con capturas revisadas a ojo, y una
  sección propia al final de este archivo (hecho, archivos, prueba con ruta, pendiente).

## Frentes
| Frente | Archivos propios | Copia |
|---|---|---|
| P4 Minijuegos IW (sigue) | `src/iw_minijuegos.*` | combate_test |
| R1 Campaña IW | `src/pasarela_iw.*`, zona IW de `src/pasarela_misiones.cpp` | modos_test |
| R2 Campaña B2 + Nave de Babidi | `src/pasarela_b2.cpp`, nuevo `src/pasarela_nave.cpp`, zona tablero de `pasarela_misiones.cpp`; en `pasarela.h/.cpp` solo los campos nuevos de `Reglas` (vida que baja, sin bloqueo, ki a 0 del jugador) | escenarios_test |
| R3 Satán + cinemáticas | `src/pasarela.cpp`, `src/pasarela_b1.cpp`, `src/pasarela.h`, nuevo `src/cinematica.*` | camp_test |
| R4 Tenkaichi 3 (estudio) | solo lectura; informe en `E:\DBZ3HD_trabajo\estudio_t3\` | — |
| R5 Héroe beta | `src/heroe.cpp`, casos kHeroe/kHeroeP de `src/rpg.cpp`, piezas del héroe de `assets_propios/generar.py` | du_test |
| R6 Pulido UI/HUD | `src/ingame/game_style.*`, `src/ingame/hub_panel.cpp`, dibujo de `src/modos.cpp` y de `src/rpg.cpp` (salvo héroe) | hub_test |
| R7 Vídeos SFD de IW | nuevos `src/video_sfd.{h,cpp,cmake}`, 3 líneas en `CMakeLists.txt` (las 2 listas + `include`), escenario `_r7_video` de `tools/pruebas.py` | video_test |
| R8 Campos de IW como escenarios de B3 | `mod center hd/campos_iw.py` y `mod center hd/escenarios_convertir.py` (movidos desde `tools/`, que quedan como lanzadores); `CampoIW()` y niebla de las altas en `src/escenarios.{h,cpp}`; `BuildStages` en `src/launcher/mod_pipeline.{h,cpp}`; paso de JUGAR en `src/launcher/launcher_state.{h,cpp}`; gancho de prueba `DBZ3_TEST_ESCENARIOS_AUTO` en `src/main.cpp`; `campos_iw` en `KIT_ENTRIES` de `tools/make_modpacks.py`; `r8_limite` y `--par arranque` de `tools/pruebas.py`; `head` de `attach_rigid` en `awo_tools/amo2awo.py` (las 2 líneas de `src/iw_minijuegos.cpp` las pone P4) | stage_test, stage_test2, stage_test3, stage_clean |
| R9 Ports de IW con fallos (Pan, Goku GT, Vegeta GT, Baby) | `src/ports_guard.cpp`; montaje de ports en `mod center hd/roster_build.py` (solo si el dato está mal) | ports_test |
| R10 Carrera de IW en B3 (minijuegos) | nuevos `src/anim_iw.{h,cpp}`, 2 líneas en `CMakeLists.txt` (las 2 listas); sondas en `E:\DBZ3HD_trabajo\R10\` | anim_test |
| R11 Suelo real de los campos de IW | nuevos `src/suelo_iw.{h,cpp}`, 2 líneas en `CMakeLists.txt` (las 2 listas); prototipos en `E:\DBZ3HD_trabajo\R11\` | suelo_test |
| R12 Minijuegos de VUELO de IW (7 Guru/Vegeta, 17 carrera con Goten) | nuevo `src/iw_mini_vuelo.cpp`, 2 líneas en `CMakeLists.txt` (las 2 listas), escenario `_r12_vuelo` de `tools/pruebas.py`; RE en `E:\DBZ3HD_trabajo\R12\` | vuelo_test |
| R13 Minijuegos de DISPARO de IW (3 ladrillos, 8 Kienzan a Freezer, 9 rayos a Piccolo, 15 Ten contra Cell) | nuevo `src/iw_mini_disparo.cpp`, 2 líneas en `CMakeLists.txt` (las 2 listas), 1 línea en `src/iw_minijuegos.cpp` (`Fotograma` → `FotogramaDisparo`); escenarios `.toml` y RE en `E:\DBZ3HD_trabajo\R13\` | disparo_test |
| R14 Minijuegos de BOTONES de IW (5, 6, 11, 12, 13, 18) | nuevo `src/iw_mini_botones.cpp`, 2 líneas en `CMakeLists.txt` (las 2 listas), escenario `_r14_botones` de `tools/pruebas.py`; RE en `E:\DBZ3HD_trabajo\R14\` | botones_test |
| R15 Modelos propios con el z-buffer del juego | `src/ps2_modelo.{h,cpp}`; SDK: capa ImGui de D3D12 (`src/ui/d3d12/d3d12_immediate_drawer.*`, `d3d12_presenter.*`, `src/ui/shaders/dbz3_ui_depth.*`) y copia de profundidad (`src/graphics/d3d12/command_processor.*`, `render_target_cache.*`), `tools/verify_release.ps1` y AGENTS §7 (tamaños de DLL); RE en `E:\DBZ3HD_trabajo\R15\` | zbuf_test |

## Resultados

### R3 · Leyenda de Mr. Satán + cinemáticas (2026-10-16)
- **Ingeniería inversa (ELF de B1, `E:\DBZ3HD_trabajo\P3\jals.py` nuevo):** el modo **no tiene escenas 3D con modelos**
  ni vídeos. `SATAN DEMO MAIN` (0x22E380) es el prólogo "Mr. Satan's Style": 668 #6 (filosofía) y 6 consejos (668 #8,
  #10-#14) con 13 voces (`0x1369D0`; 1261-1268) sobre un fondo con desenfoque (`-BLUR MAIN-` 0x2307AC, filtro 2D,
  `func_141320`). Las charlas (669) las carga **la pantalla de resultado** (`SATAN RESULT MAIN`, 0x231404: 667/668/669)
  bajo "NEXT BATTLE"; el final (`SATAN CLEAR MAIN`, 0x232FC4) usa las mismas. Lo único 3D del modo son los combates en
  `CHG_CELL RING` (14), que B3 tiene (mapa 50): **no hace falta portar ningún mapa de B1**.
- **Hecho:** `src/cinematica.{h,cpp}` (reproductor reutilizable: `cinematica::Empezar(Escena)`, `Activa`, `Cancelar`,
  `Conectada`): dentro del combate pedido, al empezar la pelea, los dos luchadores (modelos de B3) quietos o con la
  pose de carga de ki del propio personaje (Y+B, como el relevo del Tag Team), cámara por planos (general / luchador 1 /
  luchador 2, fundidos de 0,6 s y vuelta a la del combate con `cine::Camara`), bandas de cine que tapan el marcador,
  subtítulo con retrato (`CaraDe`) + imagen del original (o texto) y voz (`OirVozB1`), A salta. Medidas con `S::mode`.
  Un único punto para esconder el HUD nativo: `OcultarHud()` en `cinematica.cpp` (espera la función de P4).
  En la Leyenda (`pasarela.cpp`): antes del primer combate de cada retador, escena en el ring de B3: plano general,
  Mr. Satán con su charla 669 (par) y las 2 voces JP, el rival cargando ki, y el aviso de la regla (669 impar).
  Con el reproductor conectado, el paso 2D de la charla se salta (el avance guardado no cambia de índices).
- **Archivos:** `src/cinematica.h`, `src/cinematica.cpp` (nuevos; registrados en las dos listas de `CMakeLists.txt`),
  `src/pasarela.cpp` (include, `Escena()`, llamada en `Lanzar`, salto de la charla en `Entrar`).
- **Prueba:** mis archivos compilan sin errores; **el enlace falla en `src/heroe.cpp:296/326` ("Saga" ambiguo, de R5)**,
  así que no hay exe nuevo ni prueba `correr camp_test satan` todavía.
- **Pendiente (para el coordinador):** conectar los dos ganchos, una línea cada uno, como los de P4 (no los toco por la
  nota de coordinación): en `src/clima.cpp`, tras `dbz3::iwmini::Fotograma(base, st);` →
  `dbz3::cinematica::Fotograma(base, st);`; en `src/combate.cpp` (sub_8208FD58), tras
  `dbz3::iwmini::Mando(base, user, true);` → `dbz3::cinematica::Mando(base, user);` (+ `#include "cinematica.h"`).
  Sin ellos el modo funciona igual que antes (`Conectada()` falso: se queda la charla 2D). Después: `correr camp_test
  satan --par desde=1` y mirar la escena. `ponytail:` los dos lados comparten el mando 0 durante la escena (misma
  pose; un mando virtual por lado si hace falta); el cronómetro del combate corre durante la escena (~16 s menos de
  TIME); el prólogo "Mr. Satan's Style" (668 #6, #8, #10-#14 + voces 1262-1268) y una escena final 3D quedan por hacer.

#### R3 segunda vuelta (2026-10-16 02:55)
- **Escena 3D funcionando** (ganchos conectados): planos general / Mr. Satán con su charla 669 y las 2 voces / rival /
  aviso 669 en franja oscura; marcador nativo oculto (`combate::OcultarHud` de P4, llamado desde `OcultarHud()`),
  **reloj parado** durante la escena (mgr+35144 se repone cada cuadro: el combate empieza en 99), tiempos por reloj de
  pared (el juego baja de 60 y las voces van en tiempo real), luchadores separados 30 unidades como la tabla de B1
  (lados en x = ±15; `Escena::distancia`), primeros planos casi de lado (el otro queda fuera). Subtítulo: franja oscura
  por defecto (IW: letra blanca con borde) y bocadillo claro solo para letra oscura (charlas de B1).
- **Prólogo "Mr. Satan's Style"** antes del título (`DrawPrologo`): 668 #6 (voz 1261), #8 (1262) y #10-#14 con la cara
  668 #7 (golpe 1263 + voces 1264-1268), tiempos del demo de B1 (b1.c de Ghidra: FUN_0022eb90 1200 cuadros,
  FUN_0022f140 209, FUN_0022f2b0/f420 139 + 349); A pasa, START salta. Final (`SATAN CLEAR MAIN`, FUN_002333d0): jingle
  1180 y las 11 caras 668 #24 en color bajo el total 668 #18.
- **R6:** franja de la dificultad dentro del letrero, récord más grande, tarjeta VS con caras de 380 centradas y aviso
  abajo. La charla 2D ya no sale con el reproductor conectado (va en la escena 3D).
- **Prueba:** compila. `correr camp_test _r3_cine` PASS: `D:\DBZ3HD\_pruebas\20261016-025250_camp_test__r3_cine`
  (c1 Mr. Satán + charla, c3 Ten Shin Han solo, c4 los dos + aviso, d_pelea reloj 97). Prólogo (`--par desde=0 --par
  prologo=1`) PASS: `...\20261016-024750_camp_test__r3_cine`. `satan` PASS: `...\20261016-024905_camp_test_satan`
  (a2 dificultad, b tarjeta); `satan --par desde=25` PASS: `...\20261016-025947_camp_test_satan` (e_final: total y las
  11 caras). Escenario nuevo `_r3_cine` en `tools/pruebas.py`; `satan` acepta la charla saltada.
- **Pendiente / ponytail:** pose propia del personaje (carga de ki, victoria, burla) en la escena: Y+B en el registro
  del mando NO carga ki (ataca; probado); el relevo del Tag Team lo hace en XInput (`net/netplay_hooks.cpp`,
  sub_8221C178), y la de victoria (sub_821017B8) no tiene llamada estudiada: los luchadores quedan en su guardia de B3
  (la entrada de cada uno ya es la nativa, estado 2). Un mando por lado: sin hacer (un mando sin nadie saca el aviso de
  desconectado; haría falta el virtual del muñeco de `combate.cpp`). Contraseña del final (FUN_00234790 con las tablas
  0x35F938) sin portar (la web ya no existe). Detrás del prólogo B1 pasa su escenario 3D desenfocado (FUN_002304c0);
  aquí, el fondo del modo oscurecido. Desbloqueo: la web dice que se compra a Mr. Popo tras el Torneo en Adept; el
  port no tiene tienda de B1 y el modo sigue abierto en el HUB (sin comprobar en el ELF).

#### R3 tercera vuelta (2026-10-16 03:25)
- **Mando virtual por XInput** (`net/netplay_hooks.cpp`, una función nueva `dbz3::net::InyectarPad(usuario, botones)`
  y su rama en el gancho sub_8221C178, como la pose del relevo del Tag Team): durante la escena el mando del humano
  deja de leerse (no puede mover a Mr. Satán) y el COM, con su IA parada, lee los botones de la escena. Traza en el
  registro: `dbz3 net: mando virtual N (cinematica): botones ...`.
- **Medido (sin pruebas a ciegas):** (1) dar al COM un mando libre propio (el 4, como el Tag Team) saca la pausa
  "2P PAUSE" en cuanto empieza la escena (captura en `...20261016-030647_camp_test__r3_cine\c1_paso.png`); por eso
  el COM comparte el mando del humano y, si comparten, va el botón del lado del plano. (2) Y+B por ese mando al
  empezar la pelea da la acción **15** (X+3244) unos 40 cuadros y vuelve a 0, sin carga de ki visible (la del relevo
  es 20/21): traza `cuadro N paso K: acciones A / B` en `...20261016-031341_camp_test__r3_cine\dbz3.log`. La escena de
  la Leyenda no usa poses hasta saber qué pide la carga (`kCargarKi` queda marcado `ponytail:` en `cinematica.h`).
- **Victoria (RE):** `sub_821017B8` elige al ganador (mgr+35136, modo mgr+2018) y llama a `sub_8211C3B8(X)`, que monta el
  final del luchador (cambia su estado y la cámara, esconde al otro con X+3261). No tiene pareja EU y llamarla en plena
  pelea lo deja en estado de fin: no se usa en la escena. Burla: B3 no tiene botón de burla. Final tras Cell: ya sale
  la pose de victoria nativa de Mr. Satán (estado 5-7) y después la pantalla final con el jingle 1180 y las 11 caras.
- **Prueba:** compila. `_r3_cine` PASS `D:\DBZ3HD\_pruebas\20261016-031810_camp_test__r3_cine` (sin pausa, los dos
  quietos, aviso); `satan` PASS `...\20261016-032039_camp_test_satan`.
- **Pendiente:** qué botones (o qué estado previo) dan la carga de ki 20/21 fuera del relevo (mirar `tagteam.cpp`
  `Free()` y la entrada del luchador X+6048 con la sonda `DBZ3_LAB=1`); un mando propio del COM sin la pausa (mirar
  qué la dispara: quizá el mando nuevo conectado en plena pelea, sub_8208FC40 rama de conexión que borra el registro).

#### R3 cuarta vuelta (2026-10-16 03:45)
- **Sonda** (`DBZ3_CINE_TRAZA=1`; el escenario `_r3_cine` la pone siempre, y `--par ringout=1` deja ver la victoria): estado, acción X+3244 y código X+2928 de
  cada luchador al cambiar. Medido (`D:\DBZ3HD\_pruebas\20261016-032626_camp_test__r3_cine\dbz3.log`): entrada (estado 2)
  acción 15 con código **0x281** (lado 1) y **0x280** (lado 2); victoria (estados 6-8) acción 15 código **0x2A0**;
  guardia acción 0 código 0. La carga de ki 20/21 sale solo en el relevo del Tag Team (sin medir aquí).
- **Poses nativas** (`cinematica::Tick`, hilo del juego con contexto, desde mi rama del gancho XInput de
  `net/netplay_hooks.cpp`): como `sub_82103C48` (acción anterior en X+3246, acción 15 en X+3244, `sub_82103978(X, 0)`,
  código en X+2928, `sub_82114FB8(X)`); se repite cuando el juego vuelve a la 0 y, al pasar a un paso sin pose, vuelve
  la guardia. `Paso::anim[2]` + `kPoseEntrada1/2`, `kPoseVictoria`. Solo US (sin pareja EU de `sub_82114FB8`).
  Leyenda: Mr. Satán dice su línea con **su pose de victoria de B3** y el rival contesta con **la de su entrada**; los
  primeros planos se abren con pose y, con la de victoria (que gira al personaje), la cámara va al otro costado.
- **Prueba:** compila. `_r3_cine` PASS `D:\DBZ3HD\_pruebas\20261016-033232_camp_test__r3_cine` (c2: Mr. Satán señalando y
  riendo, de frente). El encuadre por pose (victoria al otro costado, entrada abierta) es de después de esa captura:
  compila, sin captura nueva. `satan --par golpes=1` PASS `...\20261016-034025_camp_test_satan` (con `DBZ3_SIN_CINE=1`, que
  quita la escena en esa prueba para pelear desde el primer cuadro).
- **Para R1:** `Fin::combo_max` (golpes del combo mayor del jugador, como el "N HITS" de B3) y `Fin::combo_dano_max` (vida
  que quitó ese combo), en **todos** los combates vigilados (`VigilarCombate`; la misma detección que los HIT de B1). En
  la última prueba el jugador no llegó a pegar (0 / 0, correcto); el registro lo escribe en "fin del combate".
- **Pendiente:** carga de ki (medir su código en el relevo y reproducirlo igual); EU (`sub_82114FB8` y `sub_82103978`
  sin pareja); un mando propio del COM sin la pausa "2P PAUSE".

#### R3 quinta vuelta (2026-10-16 04:05)
- **Encuadre:** X+48 es el suelo y el luchador mide ~14; la zona libre entre bandas tiene el centro 48 px por encima del
  de la pantalla. Primeros planos a 28 (con pose 38) apuntando a 6-7 sobre el suelo: el actor entero por encima del
  subtítulo. Las poses ya no se cortan a mitad (al acabar, el juego vuelve solo a la guardia; forzarla dejaba al rival
  corriendo). El rival contesta con **su pose de victoria** (la de entrada, 0x280, lo adelanta y salía cortado).
  Capturas revisadas: `D:\DBZ3HD\_pruebas\20261016-035919_camp_test__r3_cine` (c1 los dos; c2 Mr. Satán señalando y
  riendo, entero; c3 Ten Shin Han con su pose, cabeza dentro; c4 los dos en guardia con el aviso).
- **EU:** parejas medidas con el método de `tools/eu_map.py` sin reescribir el mapa compartido
  (`E:\DBZ3HD_trabajo\P3\eu_par.py`): `sub_82114FB8` → `821150B8` y `sub_82103978` → `82103A78` (alineadas, 0 diferencias
  sin explicar, 67 y 63 llamadas bien). Se llaman con `DBZ3_CALL2(us, eu)`: las poses van en US y en EU. (Sin probar en
  una copia EU.)
- **Carga de ki:** Y+B (teclado P+Backspace) en combate da la acción 15 con código **0x20E** (un paso, no la carga:
  `...\20261016-035120_camp_test__r3_golpes\a_ki.png`); la acción 20 que apuntaba el Tag Team es la del golpe recibido
  (el muñeco: "accion 0 -> 20"). `kPoseKi` no se añade: sin código medido de la carga.
- **combo_max (R1):** escenario `_r3_golpes` (Leyenda contra Yamcha sin escena, rival muñeco quieto, sonda puesta):
  PASS `...\20261016-035120_camp_test__r3_golpes`, registro "combo mayor del jugador 1 golpes" (`Fin::combo_max` > 0; el
  combo de 3 no entró: Mr. Satán falla los otros golpes con esa secuencia de teclas).
- **Pendiente:** el código de la carga de ki (grabar una carga real con la sonda); un combo largo en la prueba.

#### R3 sexta vuelta: carga de ki (2026-10-16 04:15)
- **Sonda** `_r3_cine --par ki=1` (`DBZ3_CINE_KI=1` en `pasarela.cpp`): la escena mantiene 2,5 s cada combinación de
  botones por el mando virtual (XInput) con la sonda de acción/código puesta. La carga de ki del juego es la **acción 21**
  (la que usa `E:\DBZ3HD_trabajo\ports\combate_ports.py` para fotografiar el aura).
- **Resultado:** ninguna de 22 combinaciones mantenidas llega a la 21. Cara (`...\20261016-040459_camp_test__r3_cine`):
  Y+B → 15/0x20E (un paso), B+X → 15/0x245 (golpe), X+A → 13, Y+A / B+X+A / Y+X+A → 1 (guardia), el resto nada. Con
  LB/RB (`...\20261016-041218_camp_test__r3_cine`): LB → 15/0x257 y luego guardia (1) mantenida con todas las demás.
  Los gatillos (LT/RT, bytes aparte en XInput) no se probaron.
- **Conclusión:** con el mando virtual la carga no se alcanza; `kPoseKi` no se añade (el rival contesta con su pose de
  victoria, que sí sale). Siguiente paso si se quiere: leer la tabla de órdenes del juego (pausa > lista de comandos) o
  probar LT/RT escribiendo sus bytes en el gancho XInput.

### R4 · Tenkaichi 3 (estudio, enjambre Haiku, 2026-10-16)
Informes: `E:\DBZ3HD_trabajo\estudio_t3\01_iso_contenedores.md`, `02_modelos_animaciones.md`, `03_escenarios_historia.md`,
`04_audio_2d.md` (ISO del usuario: PAL SLES_549.45, `E:\Games\EmuVR\Games\Sony Playstation 2\`).
- **Otro motor (Spike), nada en común con B3:** 3 AFS (PZS3EU1 1,6 GB: modelos `pmdl`, mapas `Map_00..38`, animaciones
  `CMAn`, labios `LIPS`, textos UTF-16; PZS3EU2 1,2 GB: ~65.000 ADX). Sin `#AMT`, `#AWO`, TIM2 ni DDS; formatos de modelo,
  textura (`GSCF`), animación y mapa sin documentación pública.
- **Útil ya, barato:** voces ADX mono 24 kHz (~4.900 JP y ~4.900 US; las lee `mod center hd/gritos.py`), música ADX y
  textos en 7 idiomas. Lo caro es saber qué clip es qué frase.
- **Caro:** personaje jugable T3→B3 2-4 meses cada uno (visor estático 1-3 semanas); mapas = ingeniería inversa nueva
  sin pares de referencia; Dragon History comprimido/cifrado. Snake Way/Kami's Lookout no confirmados en T3.
- **Decisión:** T3 solo como fuente de voces/textos cuando IW/B2/B1 no los tengan; nada de modelos ni mapas en esta ronda.

### Herramientas de RE (coordinador, 2026-10-16)
- Ghidra 12.1.2 (`C:\Users\javie\Desktop\PROYECTOS IA\Shin Budokai 2\ghidra_12.1.2_PUBLIC`) + extensión PS2
  ghidra-emotionengine-reloaded v2.1.38 (en `%APPDATA%\ghidra\ghidra_12.1.2_PUBLIC\Extensions`).
- Descompilado de IW (EU, SLES_553.47, con nombres puestos por su autor: `DragonMission...`, `MiniGoku...`,
  `Battle_...`) exportado a C: `E:\DBZ3HD_trabajo\ghidra\salida\iw_sles55347.c` (proyecto Ghidra en
  `E:\DBZ3HD_trabajo\ghidra\proyectos\IW`; script `E:\DBZ3HD_trabajo\ghidra\scripts\ExportarC.java`). Ojo: es la versión
  europea; nuestra copia es USA (direcciones distintas: casar por cadenas o bytes).
- REA (`rea` en la terminal, MCP registrado en Claude Code para la próxima sesión): con
  `GHIDRA_INSTALL_DIR` apuntando a ese Ghidra.

### R5 Héroe beta (2026-10-16, 02:32)
- **Movesets cambiables, en marcha y sin archivos:** en la pantalla del héroe se elige **Estilo** (moveset de un
  maestro —Goku, Vegeta, Piccolo, Krilin, Freezer, Cell, con su bonus y su cápsula de habilidad— o de otros 17
  luchadores). En combate el jugador ES el luchador del estilo (movimientos, técnicas, cápsulas, voz) con el
  **modelo del cuerpo**: mientras dura la partida del héroe, `heroe.cpp` (Vestir) apunta las formas del registro de
  combate del estilo (char372 +212+20·f: +2 ID del modelo, +4 su forma) a la forma base del cuerpo; `Sync` lo
  devuelve al salir (log `vuelve a su modelo`). Es el cambio de modelo B3→B3 nativo, pero en memoria y solo ahí.
  Si un rival es el luchador del estilo, sale su alternativo (Vegeta → Raditz…), porque el registro es por ID.
- **Pantalla pulida** (S::mode: columna kX0..kX1 desde kTop, esquina kCornerX0): cara del cuerpo y del estilo, filas
  Raza/Cuerpo/Apariencia/Estilo (marca "maestro"), bocadillo con bonus, técnicas, nivel y la saga siguiente. Trajes
  por cuerpo leídos de la tabla del select (0x8245F150 por casilla; si aún no está en memoria, los fijos).
- **Bucle de runs:** 4 sagas de 3 combates (Saiyan, Namek, Androides, Bu; cada vuelta +500 de vida a los rivales);
  cada combate ganado = 1 de experiencia, nivel = 1 + exp/3 (máx. 20): +100 vida y +2 % daño por nivel. El premio de
  700 Zenie va al monedero del HUB (Premio de siempre). rpg.toml: `heroe_estilo/exp/saga` (se sigue escribiendo
  `heroe_maestro`); partida: `jugador` = estilo, `extra` = 1000 + cuerpo, `tecnicas` = saga (las viejas se leen).
  Pantalla final del héroe: "Héroe <cuerpo>: 3/3  Nivel N".
- **Archivos:** `src/heroe.cpp` (reescrito), `src/rpg.cpp` (declaraciones de heroe, kMision → sagas de heroe.cpp,
  casos kHeroe de PeleasDe/Empezar/selector/entre combates/descripción, DibujarHeroe, 2 líneas en Sincronizar, línea
  del héroe en DibujarFin), `tools/pruebas.py` (escenario `r5_heroe`, params `estilo`, `espera`, `ringout`, `completa`).
  Copias de antes y scripts: `E:\DBZ3HD_trabajo\R5\`.
- **Prueba:** compila (`tools/compilar.py`); `correr du_test r5_heroe` PASS x4, capturas revisadas:
  `D:\DBZ3HD\_pruebas\20261016-022015_du_test_r5_heroe` (Krilin con el moveset de Goku: intro y guardia de Goku),
  `20261016-022222_..._r5_heroe` (Piccolo con el de Vegeta, entre combates, salir = vuelve su modelo),
  `20261016-022412_..._r5_heroe` (saga Saiyan completa, Raditz en lugar de Vegeta, nivel 1→2, +700 Zenie),
  `20261016-022746_..._r5_heroe` (nivel 2: vida 3100, daño 122 %, saga de Namek contra Recoome, final "Nivel 3").
  Los pies quedan en el suelo en las dos parejas probadas.
- **Pendiente:** la cara del marcador es la del estilo (la tabla 0x82373D68 son nombres de cápsulas, no caras; habría
  que cambiarla como Tag Team en B+4376/B+4428); las transformaciones del estilo usan el modelo base del cuerpo; la voz
  y los gritos son los del estilo; cuerpos muy distintos de altura pueden flotar (en marcha habría que escalar las
  pistas WAIST del anm como `altura.py`); `anfitrion_heroe` sigue sin hacer (el anfitrión lo dibuja `modos.cpp`,
  frente R6: una línea); los nombres de técnicas son los de los 6 maestros (en B3 son imágenes; ponytail).

#### R5 segunda vuelta (2026-10-16, 02:55)
- **Cara del marcador = el cuerpo.** El juego busca al empezar la fila del jugador en la tabla de caras 0x82324468
  (16 B: ID, máscara de trajes, fichero de caras, ficha; la misma que usa Tag Team) y carga su hoja y su retrato.
  Durante la partida del héroe, las filas del estilo llevan el fichero de caras del cuerpo (fila del cuerpo para su
  traje); `Quitar` lo devuelve. Sale la cara del cuerpo en la barra (Krilin, Bu, Cell, Goku). Sin tocar la ficha +12
  (es la lista de técnicas de la pausa: sigue siendo la del estilo).
- **Altura: no hace falta corregirla entre luchadores de B3.** Probado con las parejas más dispares: Cell (alto) con el
  estilo de Krilin, Bu el puro (bajo) con el de Piccolo, Krilin con el de Cell, Goku con el de Piccolo: pies en el
  suelo en guardia, READY y golpes. La corrección de `altura.py` es para los ports PS2/B1 (cadera hecha para otras
  piernas). Si algún día hace falta en marcha: anm cargado en guest sin LZX (nodo +192), #AMB → hijo #ACM → tabla de
  animaciones (t+16a: flags, var, nf, off) → puntero de pos de WAIST (`off + 4*(per*j+1)`) → claves de 16 B
  [frame, x, y, z] desde +16: x,z *= s; y = y*s + dy, con s = pierna_cuerpo / pierna_estilo (WAIST a LFOOT1/RFOOT1).
- **Transformaciones:** con otro cuerpo, las formas 1+ del estilo exigen una cápsula que el estilo no puede llevar
  (char372 +212+20·f; Second Form de Freezer, o el Kamehameha de Goku si el estilo es Freezer): el héroe pelea en su
  forma base, sin modelos, caras ni auras a medias. Con su propio cuerpo, todo como siempre.
- **Voz y gritos: se quedan los del estilo** (documentado, no es barato): el banco de gritos (char372 +0x24, lang_*)
  va por índices del moveset; los ports reconstruyen el banco del donante con `gritos.py`. El del cuerpo tiene otro
  orden y 55/56 sonidos: pediría gritos equivocados o uno que no existe. Para la voz del cuerpo haría falta montar en
  marcha un banco con el orden del estilo y los sonidos del cuerpo (gritos.py en C++).
- **Pantalla (lo de R6):** las flechas usan `D::kTop + D::kPad + 52·(i + 0.5)`. El marco de las caras va de
  `kCornerY0` a `kTop - 24` y las caras quedan dentro de `kLine`: cuerpo + estilo y el rótulo "Estilo". **Anfitrión
  propio:** `generar.py` (`anfitrion_heroe()`, llamado tras `fondos_hub()` y copiado por `--al-juego`) recorta la
  primera figura de cuerpo entero de `png/add/Capsule Corp Character.png` (fondo transparente, 158x437, pies abajo) →
  `res/ingame_ui/hub/anfitrion_heroe.png` y la copia du_test. `modos.cpp` ya la pinta.
- **Para R6:** la línea del héroe en `DibujarFin` (rpg.cpp, tras lo de la Tienda) es:
  `if (st.tipo == M::kHeroe) { l[1] = Lang("Héroe ", ...) + M::Nombre(H::CuerpoDe(r.extra, r.jugador)) + ": " +
  Num(st.ganados) + " / " + Num(st.total) + "   " + Lang("Nivel ", ...) + Num(H::Nivel()); }`.
- **Archivos:** `src/heroe.cpp` (FilaCara, Vestir con la cara y las formas, Reglas con el traje), `src/rpg.cpp` (la
  llamada `H::Reglas(r.jugador, r.extra, r.traje)` y su declaración, y la esquina y las flechas de DibujarHeroe),
  `assets_propios/generar.py` (anfitrion_heroe), `res/ingame_ui/hub/anfitrion_heroe.png` (nuevo), `tools/pruebas.py`
  (`r5_heroe --par rafaga=N`). Copias de antes en `E:\DBZ3HD_trabajo\R5\`.
- **Prueba:** compila; `r5_heroe` PASS x5, capturas revisadas a ojo: `D:\DBZ3HD\_pruebas\20261016-023815_du_test_r5_heroe`
  (Krilin + Cell, cara de Krilin), `20261016-024039_..._r5_heroe` (Bu el puro + Piccolo), `20261016-024607_..._r5_heroe`
  (Cell + Krilin: anfitrión nuevo y esquina en `b_estilo.png`, pies en el suelo en `r07.png`), `20261016-024847_..._r5_heroe`
  (Goku + Piccolo: guardia de Piccolo, cara de Goku, READY en `d_pelea_1.png`).
- **Pendiente:** la transformación no se ha probado pulsando el mando: solo consta en el log
  `formas (1+ bloqueadas)`. Kaioken también queda bloqueado aunque solo cambia el aura; se podría dejar pasar las
  formas que solo cambian el aura. La voz del cuerpo, como arriba.

### R1 Campaña de Infinite World (2026-10-16, 02:40)
- **Descifrado (ELF USA con `iwdis.py`; volcados en `E:\DBZ3HD_trabajo\R1\`):** las escenas de antes y después de las
  misiones de IW **no son guiones**: son 22 vídeos 3D renderizados (`USR/FD01A..FD20.SFD`) con los subtítulos en
  imágenes en **DATA_USA 77 + vídeo** (tabla del ELF `0x4827B0`). Qué vídeo va antes o después de cada misión lo dicen
  dos tablas del ELF (`0x472830` antes, `0x472890` después; las lee `0x2899F0` al acabar): minijuego 0 → FD01A; misión
  11 FD01B/FD02; 23 FD03/FD04; 37 FD05/FD06; 38 FD07/FD08A; minijuego 10 → FD08B; 49 FD09/FD10; 54 FD11/FD12; 63
  FD13/FD14; 65 FD15/FD16; 70 FD17/FD18; 77 FD19/FD20. Frases o cambios de condición durante el combate: no localizados (el registro de misión no
  los lleva; los `ScriptCmd_*` de la descompilación son de efectos de combate y súper ataques, no de misiones).
  Tabla completa y registro de misión corregido (bloques jugador/rival de 0x26 B; vida 3000 en todas las jugables) en
  `docs/16_pasarela_modos/03_infinite_world.md` (final).
- **Hecho:** `LeerDragonMission` lee además las dos tablas (las busca por su forma: vale otra versión del ELF) y los
  22 juegos de subtítulos de la copia del jugador. Antes de cada misión con vídeo, **escena en el motor de B3**
  (`cinematica::Empezar`, de R3): los dos luchadores con sus modelos de B3 en el escenario de la misión y debajo cada
  línea del subtítulo original. Después de ganarla (y tras el minijuego 10), la escena de después sobre el mapa de IW
  con bandas de cine y el subtítulo original (A pasa de línea, B la salta); antes del minijuego 0, su escena y luego
  el minijuego. Ficha, resultado y escena con las medidas de `S::mode` (`kHead`, `kRow`, `mode::Help`).
- **Archivos:** `src/pasarela_iw.h/.cpp` (`EscenaIW`, `VideoIW`, `LeerEscenas`, `PantallaDM::Contar/Historia`, índice
  +0x1C de las casillas), zona IW de `src/pasarela_misiones.cpp` (`EscenaHistoriaIW`, `g_iw_despues`, MISSION START,
  cierre del resultado y del minijuego), `tools/pruebas.py` (escenario nuevo `r1_historia`; `iw_mapa` salta la escena
  de después si sale), `docs/16_pasarela_modos/03_infinite_world.md`.
- **Prueba:** compila (`tools/compilar.py`). `correr modos_test r1_historia` PASS:
  `D:\DBZ3HD\_pruebas\20261016-022846_modos_test_r1_historia` (`c_escena_antes.png`: Goku y Raditz de B3 en la
  pradera con "Well, well. You're a little more foolish" de FD01B; `f/g_escena_despues.png`: FD02 sobre el mapa;
  `h_mapa`, `i_menu`). `correr modos_test iw_mapa` PASS: `D:\DBZ3HD\_pruebas\20261016-023237_modos_test_iw_mapa`
  (ficha con S::mode, escena de después saltada, vuelta al menú). `correr modos_test iw_misiones` PASS:
  `D:\DBZ3HD\_pruebas\20261016-023516_modos_test_iw_misiones`.
- **Pendiente (ponytail en el código):** los vídeos no se reproducen (no hay reproductor SFD en el juego; la escena es
  recreación con modelos de B3) y no se sabe quién dice cada línea ni cuánto dura (van en el vídeo: plano general y
  3 s por línea); la escena de antes se corta si el combate acaba antes (en la prueba, `DBZ3_TEST_RINGOUT` a los 5 s);
  el subtítulo de IW es texto blanco con borde y R3 lo pone sobre un panel claro (para R3: banda oscura).
  Fórmula de puntos y Zenie de las misiones: sin localizar (`0x32EDC0`, que parecía serlo, es
  `QteMg_FinalizeResult` de los minijuegos de botones, de P4); tabla real valor→insignia (`#AMA` 334/4), Fighter's
  Road (97-201: 105 combates contra el elegido, 16 con personajes solo de IW), entrenamientos 2-10 y fondo del
  resultado: sin tocar esta vuelta. Nivel de vida de "Endure": no está en el registro.
- **Personajes solo de IW (misiones en gris):** 67 Gotenks (IW 64), 69 Vegito contra Bu con Gohan (74 y 79), 77
  Gogeta SS4 (70), 73 y 75-76 Goku GT (44), 73 Baby Vegeta (45), 74-75 Super 17 (46), 74 y 96 Vegeta GT (54), 85-86
  Janemba (47), 87 Pikkon (48), 92 Gran Saiyaman 2 (55), 93-94 Pan (56). Coste: **barato** (minutos, sin importar
  nada) las fusiones que B3 ya tiene: IW 64 → Gotenks, 74 → Vegito, 79 → Super Bu con Gohan absorbido (forma); falta
  comprobar sus IDs/formas de B3 en `IWaB3`. **Medio** (1-2 h): que `IWaB3` busque en el roster extra (IDs 44-63)
  los ya importados con `importar.py iw` (Janemba funciona; Pan, Goku GT, Vegeta GT y Baby están hechos pero con
  fallos en combate según la memoria). **Alto** (horas por personaje): Super 17, Pikkon, Gran Saiyaman 2 y Gogeta SS4
  sin importar; no se importó ninguno.

### R1 segunda vuelta (2026-10-16, 03:05)
- **Zenie de IW portado** (descompilación Ghidra EU `iw_sles55347.c`; el registro de misión EU está 0x800 después del
  USA): `Char_ComputeScaledStat` (EU `0x2ABB80`) = premio `+0x0E` del registro de la misión × 50/80/100/130/160 según
  la dificultad de la ficha (×200 la sexta; la misión 77, Gogeta contra Omega, ×100 siempre; una cápsula de Zenie lo sube ×1,1-×2)
  y `Char_RoundAndScaleStat` (`0x2ABD90`) = +20/40/60/100 % por el rango 1-4 (no en los entrenamientos 1-10 ni en la
  77), redondeado a la centena. Va al monedero del HUB al ganar (misión 11 en Normal con rango S: 3000 + 60 % = 4800 Zenie; antes 300). Rango y
  SCORE de IW, descifrados pero sin portar: `Battle_ComputeRankAndRecord` (`0x28A5E0`) suma tres notas de
  `Char_ComputeStatGrade` (`0x2AC0D0`) sobre lo que guarda `LoadWaitBattleMainExitEnd` (`0x208EA0`) al acabar:
  +0x10 % de vida propia (nota 1-7 en 25/35/45/55/65/75/85), +0x14 % de vida del rival, segundos, golpes, combos…;
  rango 4 si la suma ≥ 13 (≥ 19 en otro modo), 3 ≥ 11 (16), 2 ≥ 8 (12), 1 ≥ 5 (7). Falta saber el modo
  (`mgr+0x7d6`) de las misiones y contar golpes y combos: queda `ponytail` (rango propio, `PantallaDM::Rango`).
  `QteMg_FinalizeResult` (`0x32F0F0`) es de los minijuegos de botones (P4), no de las misiones.
- **Personajes de IW por el roster extra:** `IWaB3` busca del 44 en adelante el port instalado con el Mod Kit
  (`IdPort("iw", "c:" + nombre del modelo de IW)`, claves de `mod center hd/iw_modelos.json`): Goku GT, Baby, Super
  17, Janemba, Pikkon, Vegeta GT, Gran Saiyaman 2, Pan. **Janemba va de punta a punta** (misión 85, Vegeta contra
  Janemba, id 22: `D:\DBZ3HD\_pruebas\20261016-025156_modos_test_iw_mapa`, superada, 18000 Zenie). **Pan cuelga el
  juego al acabar el combate** (estado 6, acceso a la dirección 1 del invitado:
  `D:\DBZ3HD\_pruebas\20261016-024812_modos_test_iw_mapa`); por eso Goku GT, Baby, Vegeta GT y Pan (el lote GT en
  pausa) siguen en gris con «port de IW con fallos conocidos» (`ponytail` en `MotivoIW`, quitarlo cuando el frente de
  ports los arregle). Sin port, el motivo dice «impórtalo con el Mod Kit». **Fusiones de IW (64 Gotenks, 70 Gogeta
  SS4, 74 Vegito, 79 Bu con Gohan): no hay ID ni forma inicial en B3** (en B3 la fusión es una técnica en pleno
  combate, `fusion.cpp`; las formas 4-7 de Súper Bu son absorciones según `roster_ext.cpp`); siguen en gris (misiones
  67, 69 y 77). Para jugarlas haría falta empezar fusionado (conector + `tag::Fuse` al empezar), fuera de esta vuelta.
- **Arreglos de R6:** ayuda del mapa centrada con `S::mode::Help` (ya no queda bajo el coche de Bulma); en la ficha y
  el resultado, el título de IW recortado a sus píxeles y más grande, fondo del mapa más oscuro (ya no se ven la peana
  "?" ni el rombo azul detrás) y la regla en una franja oscura con `kHead`.
- **Subtítulos en franja oscura:** `cinematica.cpp` elige el fondo por el brillo de los píxeles con alfa > 64 y el
  borde oscuro de la letra de IW lo engañaba (bocadillo claro); en la copia que se le pasa el borde baja a alfa 64 y
  sale en franja oscura, como sobre el vídeo. **Vídeo original (R7):** `src/video_sfd.*` aún no existe y R7 no tiene
  sección; en cuanto esté, `EscenaHistoriaIW` / `PantallaDM::Contar` deben llamar a `video::Reproducir(ruta del FD,
  subtítulos)` y dejar la escena con modelos de B3 solo como respaldo (los dos puntos están marcados en el código).
- **Archivos:** `src/pasarela_iw.h/.cpp` (`IWaB3` con ports, `Recorte`, `Dificultad()`, ficha/resultado/ayuda),
  zona IW de `src/pasarela_misiones.cpp` (Zenie, `g_iw_dificultad`, motivo de los ports, subtítulos), y esta sección.
- **Prueba:** compila. `correr modos_test r1_historia` PASS: `D:\DBZ3HD\_pruebas\20261016-024547_modos_test_r1_historia`
  (`c_escena_antes.png`: subtítulo en franja oscura; `b_ficha`; `h_mapa` con la ayuda centrada) y, con el build
  final, otra vez PASS: `D:\DBZ3HD\_pruebas\20261016-025952_modos_test_r1_historia` (`e_resultado.png`: 4800 Zenie).
  `iw_mapa --par desde=84` PASS (Janemba, arriba); `iw_mapa --par desde=92` PASS con Pan en gris (se juega la 95,
  Videl contra Mr. Satán, sin cuelgue): `D:\DBZ3HD\_pruebas\20261016-025541_modos_test_iw_mapa`.
- **Pendiente:** Fighter's Road (97-201) no se ha hecho jugable: datos listos (`#DMD` 1092: 105 casillas en 4 tramos
  de 30/30/30/15 por `+0x04`, x/y en el mismo lienzo de 1280x960, insignia `+0x10` como en Dragon Mission, hito
  `+0x24` cada 15; registros del ELF 97-201 con condición -1/(0,4), jugador = el elegido, rival y escenario propios;
  textos DATA_USA 228-332), falta su elección de personaje (en IW es la rueda normal del select: `LoadWaitFightersRoadSelectionFunc` EU
  `0x2B1150` llama a `LoadWaitCharacterWheelFunc(2)` con el modo 4), su fondo de mapa (sin localizar) y
  su avance aparte: 1-2 h. Tabla real de insignias (`#AMA` 334/4), entrenamientos 2-10 y fondo del resultado: sin
  tocar. Rango de IW: ver arriba.

### R1 tercera vuelta (2026-10-16, 03:35)
- **Vídeos originales conectados (reproductor de R7):** antes de las misiones 11, 23, 37, 38, 49, 54, 63, 65, 70 y 77
  (la 77 sigue en gris) suena el vídeo FD de IW con sus subtítulos (los tiempos van dentro del SFD) y al acabar o saltarlo (A) empieza el
  combate; después de ganarlas, y antes del minijuego 0 y tras el 10, el suyo sobre el mapa. Nombres de los vídeos
  leídos de la tabla del ELF (`/USR/FD01A.sfd`… cada 0x100 B) y abiertos en la carpeta de IW del jugador
  (`RutaVideoIW`). **Respaldo** solo si el vídeo no se puede abrir (ISO o falta el archivo): la escena con modelos de B3
  antes del combate y los subtítulos sobre el mapa después. Mientras suena, y 0,4 s después, el mapa no lee el mando
  (la A que salta el vídeo no elige nada). Prueba: `D:\DBZ3HD\_pruebas\20261016-031133_modos_test_r1_historia`
  (`c_video_antes.png`: Raditz con "Well, well. You're a little more foolish"; `f_escena_despues.png`: FD02 con
  "HAAHAHAHAHA!"; `h_mapa`: el mapa sin la ficha abierta por la A del salto). Con el build final, otra vez PASS: `D:\DBZ3HD\_pruebas\20261016-032732_modos_test_r1_historia`
  (vídeo antes y después, rango S por `RangoIW` y 4800 Zenie en `e_resultado.png`, vuelta al menú).
- **Fusiones de IW jugables:** en B3 son personajes internos con el mismo número que en IW, los que pone la fusión en
  combate: **Gotenks (64) y Vegito (74)** empiezan el combate directamente (`RequestBattleEx`). Misión 67 (Gotenks
  contra Súper Bu, Sala del Espíritu y del Tiempo) superada: `D:\DBZ3HD\_pruebas\20261016-031641_modos_test_iw_mapa`;
  misión 69 (Vegito contra Bu) superada con el build final: `D:\DBZ3HD\_pruebas\20261016-033008_modos_test_iw_mapa`. **Súper Bu con Gohan absorbido**
  es la forma 3 de Súper Bu (35) en la tabla de transformaciones de B3 (formas 1-3 = modelos 84, 78 y 79, los mismos
  números que IW), pero **empezar en esa forma cuelga el juego al cargar** (`...\20261016-031905_modos_test_iw_mapa`):
  sale Súper Bu de serie (`ponytail` en `IWaB3`). Gogeta SS4 (70) sigue en gris (misión 77).
- **Rango de IW portado (dos de tres notas):** el tipo de notas es el `+0x06` del registro (1 Defeat, 2 Fight,
  3 Endure, 4 Road: emparejado por los valores con el modo de `Char_ComputeStatGrade`) y la vida objetivo el `+0x04`
  (barras ×1000: la vida mínima de "Endure", 1000, sale ahora de ahí). `RangoIW`: Defeat = % de vida propia (25..85 →
  1..7) + tiempo (<60 s → 7 … ≥180 s → 1); Fight = % de vida propia + % del daño pedido al rival; Endure = % del daño
  permitido que no se recibió + % de vida del rival; suma con cortes 19/16/12/7 → C, B, A, S, Z (en el resultado y en el
  plus de Zenie). **La tercera nota** (mayor combo ≥ 5 golpes, su daño ≥ 500, y dos contadores más:
  `mgr+0x8c44/0x8c48/0x8c56/0x8c3c` de IW) **no se puede alimentar**: el contador de combos de la Leyenda está dentro
  de `VigilarCombate` (`pasarela.cpp`, de R3) y `Fin` no lo devuelve. **Pedido a R3:** `Fin::combo_max` (golpes del
  mayor combo del jugador) y `Fin::combo_dano_max` (su daño). Mientras, cuenta su mitad (3 de 7). La cifra SCORE sigue
  siendo la propia (`ponytail`). Misión 67: rango 3 (S) y 28800 Zenie.
- **Fighter's Road: no hecho.** Lo encontrado: la elección es la rueda normal del select (`LoadWaitFightersRoadSelection
  Func`, EU 0x2B1150: modo `5c1491 = 4`, `LoadWaitCharacterWheelFunc(2)`), su fin de combate usa el rango de dos notas
  (`Battle_ComputeRankAndRecord(1)`, cortes 13/11/8/5) y `#DMD` 1092 da sus 105 casillas (4 tramos por `+0x04`, x/y,
  insignia, hito cada 15). El fondo de su mapa no aparece ni en `DATA_USA` 334 ni por nombre en la descompilación (los
  nombres de las funciones del mapa no son fiables: `LoadWaitMapIcons` es un memmove); falta seguir quién dibuja las
  piezas del mapa por saga. Coste: 1-2 h (elección con la rueda nativa como el Tag Team, mapa propio y avance aparte).
  Tabla real de insignias, entrenamientos 2-10 y fondo del resultado: sin tocar.
- **Error mío corregido:** al reescribir `_r1_historia` en `tools/pruebas.py` borré `_r7_video` (R7 lo había puesto
  justo después); restaurado tal cual desde `E:\DBZ3HD_trabajo\R7\edit2.py` + `edit4.py` (comprobado: ningún escenario
  del registro queda sin función).
- **Archivos:** `src/pasarela_iw.h/.cpp` (vídeos, `RutaVideoIW`, `IWaB3` con fusiones, `nivel`/`notas`, `rango` en
  `ResultadoDM`), zona IW de `src/pasarela_misiones.cpp` (vídeo antes del combate, silencio del mando, `RangoIW`,
  vida mínima de "Endure"), `tools/pruebas.py` (`r1_historia` con vídeo o respaldo; `_r7_video` restaurado).
- **Prueba:** compila (`tools/compilar.py`). `correr modos_test r1_historia` PASS y `iw_mapa --par desde=68` PASS (arriba); misión 67
  con Gotenks PASS (arriba). Capturas revisadas a ojo.

### R1 cuarta vuelta (2026-10-16, 03:55)
- **Fighter's Road jugable de punta a punta.** RE del mapa (ELF USA, `iwdis.py`; los nombres de la descompilación no
  sirven aquí): la función del mapa `0x300AB0` carga **DATA_USA 334 en los dos modos** (las piezas del mapa del mundo
  son las mismas) y, con `mgr+0x4AC = 1` (Fighter's Road), el `#DMD` de **DATA_CMN 1092** y **DATA_USA 333** en lugar
  de 1093; DATA_CMN 1112 (sonido) en los dos. **333 es la interfaz del Road**: 333/0 = logo "FIGHTER'S ROAD" (t00),
  fajo de Zenie, "REWARD!", foco, pergamino, cifras, bocadillo, presentador y retratos de los jefes (Broly, Omega,
  Freezer, Vegeta, Goku, Cell, Bu niño); 333/2 = frase de cada una de las 105 casillas; 333/3 = 8 frases de jefe. No
  hay un fondo propio del Road: es el mapa del mundo de 334/3 (ponytail: qué juego de piezas por tramo; se usa el de la
  saga del mismo número).
- **Hecho:** en el mapa de Dragon Mission, **Y** abre Fighter's Road: elección del luchador (izquierda/derecha con la
  cara del marcador, de partida el del último Duelo; A empieza, B vuelve), luego **su mapa** con las 105 casillas del
  `#DMD` 1092 en sus 4 tramos (LB/RB), el logo de 333/0 en lugar del nombre de saga, la frase original de cada combate
  (333/2) en el globo de Bulma, la ficha con el título y el texto originales de la misión 97-201 (DATA_USA 131+n), el
  combate contra el rival de la tabla del ELF con el luchador elegido (K.O.), su **rango de dos notas** (vida + golpes,
  cortes 13/11/8/5), su Zenie (misma fórmula) y **su avance aparte** (`iw_road_hecho`, `iw_road_luchador` en
  progreso.toml); Y o B vuelven a Dragon Mission. La tabla del ELF se lee ahora entera (1-201) y los textos de las 201.
  ponytail: la elección no es la rueda del select del juego (en IW, `LoadWaitCharacterWheelFunc(2)`; aquí haría falta
  el piloto del Tag Team), es la de los retos del HUB.
- **Tercera nota del rango (golpes) con `Fin::combo_max` y `combo_dano_max` de R3:** mayor combo ≥ 5 golpes (+2) y su
  daño ≥ 500 (+2; +1 en Fight); en el Road es la segunda nota. ponytail: los otros dos contadores de esa nota
  (`mgr+0x8c56` y `+0x8c3c` de IW, sin identificar) cuentan su mitad (2); la de Endure (cinco contadores) sigue en 3 de 7.
- **SCORE:** en IW los combates **no calculan una puntuación**: `Battle_ComputeRankAndRecord` (EU 0x28A5E0) solo guarda
  el mejor RANGO de cada misión (bytes desde `0x5C518D`; en el Road desde `0x5C1DEC`) y el Zenie; los puntos
  (`SetScore`, `MiniGame_UpdateScoreDigits`) son de los minijuegos. La cifra SCORE/BEST SCORE de nuestras misiones de
  combate sigue siendo propia (`ponytail`); lo fiel sería enseñar el mejor rango (pendiente: decidir con el usuario).
- **Créditos:** al cerrar el resultado de la misión 96 (la última de Dragon Mission) suena ENDING.SFD con
  `video::ReproducirIW(kFinalIW)` (R7; sin probar en marcha: habría que jugar la 96). Los vídeos de antes/después
  siguen con `RutaVideoIW` (mismo resultado).
- **Sin hacer:** tabla real de insignias (`#AMA` 334/4: objetos de sprites con nodos por fotograma; descifrar su formato
  lleva más de lo que quedaba) y entrenamientos 2-10 (necesitan contar guardias, esquivas y persecuciones en combate).
- **Archivos:** `src/pasarela_iw.h/.cpp` (`casillas_road`, `frases_road`, lector de 1092, `Cas()`, logo del Road, Y,
  tabla 1-201), zona IW de `src/pasarela_misiones.cpp` (`DrawElegirRoad`, `pantalla_road_`, `g_iw_road`, avance y
  rango del Road, `RangoIW` con combos, créditos), `tools/pruebas.py` (escenario nuevo `r1_road`, añadido sin tocar los
  vecinos).
- **Prueba:** compila. `correr modos_test r1_road` PASS: `D:\DBZ3HD\_pruebas\20261016-033900_modos_test_r1_road`
  (`a_eleccion`: Goku; `c_mapa_road`: logo FIGHTER'S ROAD, Yamcha con "A battle with Yamcha! Don't bore me or
  else!", 00/30; `d_ficha`: "Battle with Yamcha!" y su texto original; combate 97 superado, rango 2, 22400 Zenie).
  Otra vez con el build final (el luchador recordado, combate 98): `D:\DBZ3HD\_pruebas\20261016-034928_modos_test_r1_road`.
  Regresión de Dragon Mission: `r1_historia` PASS `D:\DBZ3HD\_pruebas\20261016-040159_modos_test_r1_historia`
  (vídeos antes/después, misión 11, rango S, 4800 Zenie).
- **Ports GT (aviso de R9):** Pan, Goku GT y Baby desbloqueados en `MotivoIW` (el cuelgue de Pan era la guarda de la
  cámara de `ports_guard.cpp`); Vegeta GT (54) sigue en gris porque va a 2-10 fps. Roster de `modos_test`
  reconstruido con el juego cerrado (`roster_build.py construir`, 7 personajes). Misión 93 (Pan contra Gohan) de punta a punta con el build final: `D:\DBZ3HD\_pruebas\20261016-041415_modos_test_iw_mapa` (`c_combate.png`: Pan en el combate; superada, rango 3, 12800 Zenie; vuelta al menú). Un primer intento se quedó sin responder a la captura con la máquina cargada (4 juegos a la vez; `...-040835_...`, sin cuelgue en el registro); el segundo, PASS.

### R1 quinta vuelta (2026-10-16, 04:35)
- **SCORE → mejor RANGO en los combates (como IW):** la ficha enseña "RANK" (334/1 t17, fila 1) con el mejor rango de
  la misión en las letras originales (t24; "–" si no se ha superado) en lugar de BEST SCORE y sus cifras; el resultado
  de un combate enseña TIME y RANK (con el "New!" de 334/5 si mejora el de la misión) y EARNED! … ZENI, sin SCORE ni
  TOTAL SCORE. El mejor rango se guarda como `iw_rango_N` (rango + 1) en progreso.toml (`iw_mejor_N` ya no se escribe).
  Los minijuegos siguen con SCORE, RANK y TOTAL SCORE (ponytail: su TOTAL es el récord de ese minijuego).
  Prueba: `r1_historia` PASS `D:\DBZ3HD\_pruebas\20261016-042132_modos_test_r1_historia` (`b_ficha.png`: RANK –; `e_resultado.png`: TIME 0'39", RANK S con New!, 4800 Zenie).
- **Rueda nativa del select para Fighter's Road: no hecha, por propiedad de archivos.** Lo necesario está en archivos de
  otros frentes: el piloto que lleva al Duelo y a su rueda (`PilotToDuel`, `GameScreen`, `MainMenuObj`… son internos de
  `src/ingame/tagteam_panel.cpp`) y el aviso del personaje confirmado (`select_ext.cpp` llama a
  `ingame::OnSelectPicked`, del Tag Team). **Propuesta al dueño de esos archivos:** exportar en `tagteam_panel.h`
  `void PilotToDuel(int fila)` e `int GameScreen()`, y un `RegistrarEleccion(bool (*)(int jugador, int id))` que
  `OnSelectPicked` llame antes que el Tag Team. Con eso el Road haría: cerrar su pantalla → piloto al Duelo 1P VS COM →
  el jugador elige en la rueda → el aviso da el id → piloto atrás (B) → mapa del Road. Mientras, sigue la elección
  con la cara del marcador (y el del último Duelo de partida).
- **Entrenamientos 2-10: sonda hecha, recuento no.** `DBZ3_R1_TRAZA=1` deja jugar los entrenamientos y registra cada
  cambio de la acción (X+3244) y del código de animación (X+2928 & 0xFFF) de los dos luchadores con el estado del
  combate ("iw traza"); escenario nuevo `r1_traza` (misión 2: guardia mantenida, toques de guardia y guardia con
  direcciones mientras ataca el COM). Primera medida, PASS `D:\DBZ3HD\_pruebas\20261016-042758_modos_test_r1_traza` (volcado en `E:\DBZ3HD_trabajo\R1\traza.txt`):
  jugador con la guardia mantenida = acción **1** (código 002); golpe parado en guardia = **26** (070/071/076); golpe
  recibido = **25** (042-05B); en el suelo / levantarse = 16 (08A), 17 (08D), 35 (02C); guardia + dirección = 7 (007)
  y 8 (008) (paso o carrera); un toque de guardia dio una vez **38** (0C5: candidata a esquiva, sin confirmar); el COM
  atacando = 15 (2xx), neutro 0. Falta provocar la esquiva y el teletransporte a propósito (hay que pulsar en el
  instante del golpe) para cerrar la tabla; con ella, el recuento es un contador por transición en `SondaIW`. Qué pide cada uno (textos de DATA_USA 133-141): 2 guardia,
  esquiva y teletransporte ×5; 3 golpe que manda lejos + persecución ×3; 4 golpe imparable, agarre y ataque cargado ×3;
  5 voltereta ×3; 6 bolas de ki, súper y definitiva; 7 carrera en vuelo ×5; 8 aura ×3; 9 transformarse (Kaioken) y
  ganar; 10 habilidades de objeto y burla ×3. Varias (voltereta, aura, objetos, burla) no existen en B3.
- **Training No.9 jugable** ("Transform into Kaioken… and defeat your opponent", DATA_USA 140): Goku contra Piccolo en
  la Sala del Espíritu y del Tiempo, 180 s; un vigilante (`VigilarFormaIW`) mira la forma del jugador (X+2980, la de
  B3: Goku 1 = Kaioken) y la misión solo cuenta si gana habiéndose transformado. Escenario nuevo `r1_entreno9`
  (P+K+G al empezar; ring out de prueba) PASS `D:\DBZ3HD\_pruebas\20261016-043143_modos_test_r1_entreno9`
  (`b_transformado.png`: Goku cargando el Kaioken; registro "transformado (forma 1)", misión 9 superada).
  Con el build anterior (sin el Kaioken en la prueba) la misma misión no cuenta como superada aunque gane: `D:\DBZ3HD\_pruebas\20261016-043521_modos_test_iw_mapa` (FAIL esperado).
- **Training No.6 jugable** ("Defeat the opponent using only Ki Blasts, Super Attacks and Ultimate Attacks", DATA_USA
  137): Goku contra Yamcha, 180 s, con `Reglas::solo_ki` (los golpes hacen el 25 %). ponytail: en IW esos golpes ni
  salen; aquí no se pueden quitar del mando. `iw_mapa --par desde=5` PASS `D:\DBZ3HD\_pruebas\20261016-044037_modos_test_iw_mapa`
  (misión 6 superada). Siguen en gris 2-5, 7, 8 y 10 (esquiva y teletransporte sin identificar; voltereta, carrera
  en vuelo, aura, objetos y burla no existen en B3).
- **Ports GT (segundo aviso de R9):** lo lento de Vegeta GT era la carga de la máquina (va a 50-60 fps) y Goku GT
  cuelga el juego en su tercera definitiva (y el COM usa cápsulas en las misiones): en `MotivoIW` quedan libres Baby
  (45), Vegeta GT (54) y Pan (56) y **en gris Goku GT (44)** («se cuelga en su definitiva») hasta que R9 lo arregle
  (misiones 73, 75, 76, 94 y 96 lo usan).
- **Tabla real de insignias: no hecha** (el `#AMA` 334/4 son animaciones de sprites con nodos por fotograma; sin el
  formato no se puede leer qué textura usa cada `+0x10` del `#DMD`).
- **Archivos:** `src/pasarela_iw.cpp` (RANK en ficha y resultado), zona IW de `src/pasarela_misiones.cpp`
  (`iw_rango_N`, `SondaIW`, `VigilarFormaIW`, Training No.6 y No.9, puertas de los ports GT, `OwnBattle`), `tools/pruebas.py`
  (escenarios nuevos `r1_traza` y `r1_entreno9`, añadidos sin tocar vecinos).

### R1 sexta vuelta (2026-10-16, 05:15)
- **Fighter's Road con la rueda del select del juego** (como IW: `LoadWaitFightersRoadSelectionFunc` abre la rueda
  normal). Exportaciones mínimas aprobadas por el coordinador, sin cambiar lo que ya hacían:
  `src/ingame/tagteam_panel.cpp/.h`: `PilotarAlDuelo(fila)` (llama al `PilotToDuel` interno), `RegistrarEleccion(aviso)` y
  `AvisarEleccion(jugador, id)`; `src/select_ext.cpp`: al confirmar un personaje en la rueda llama a `AvisarEleccion`
  (también fuera del Tag Team; el bloque del Tag Team sigue igual detrás). `GameScreen()` ya estaba en `online_panel.h`.
- **Flujo:** Y en el mapa de Dragon Mission → la pasarela se oculta (`g_iw_eligiendo`) → el piloto lleva a Duelo > 1P VS
  COM → el jugador elige en la rueda (con formas LT/RT y todo lo del select) → el aviso da el id → un Y cada vez,
  mirando la pantalla tras cada uno, hasta el menú principal (la rueda es parte de la pantalla del Duelo, 14;
  `PressUntilBrowse`/`PressToLeave` seguían pulsando y acababan en el título, `D:\DBZ3HD\_pruebas\20261016-045707_modos_test_r1_road_rueda`)
  → mapa del Road con ese luchador. Si el piloto no llega a la rueda, la elección de respaldo (cara del marcador); si el
  jugador sale sin elegir, vuelve a Dragon Mission.
- **Prueba:** compila. Escenario nuevo `r1_road_rueda` PASS `D:\DBZ3HD\_pruebas\20261016-050051_modos_test_r1_road_rueda`
  (`a_rueda.png`: la rueda nativa "Select Characters" 1P/COM; registro: 5 Y hasta el menú; `c_mapa_road.png`: mapa de
  Fighter's Road con Yamcha y su frase; `d_ficha.png`: "Battle with Yamcha!", RANK –). **Tag Team sin cambios:**
  `correr modos_test tag_swap --instantanea freezer_cap2 --par rafaga=4` PASS `D:\DBZ3HD\_pruebas\20261016-045101_modos_test_tag_swap` (con las
  exportaciones ya compiladas).
- **Sin hacer esta vuelta:** recuento de esquiva/teletransporte (entrenamientos 2-5; la sonda y la tabla de acciones de la
  quinta vuelta siguen listas) y la tabla real de insignias.
- **Archivos:** `src/ingame/tagteam_panel.cpp/.h`, `src/select_ext.cpp` (exportaciones), zona IW de
  `src/pasarela_misiones.cpp` (`EleccionRoadNativa`, panel oculto mientras se elige), `tools/pruebas.py` (escenario nuevo
  `r1_road_rueda`). Copias de antes: `E:\DBZ3HD_trabajo\R1\respaldo\*.antes`.

### R1 séptima vuelta (2026-10-16, 05:50)
- **Esquiva y teletransporte (entrenamientos 2-5):** segunda sonda `r1_traza2` (misión 12, Raditz con el COM en
  Normal; G y G + dirección a ráfagas mientras pega) PASS `D:\DBZ3HD\_pruebas\20261016-051826_modos_test_r1_traza2`
  (volcado `E:\DBZ3HD_trabajo\R1\traza2.txt`). Lectura de las dos sondas: acción **38** = evasión, con la animación
  **0C5** al tocar G solo (sonda 1) y **0BE** con G + dirección (sonda 2): esquiva y teletransporte, como los describe
  IW ("G con el tiempo justo" / "G + izquierda/derecha justo antes del golpe"). Solo salió una vez cada una: **sin
  confirmar con más casos**. Lanzado por el jugador: 20 (09A) → 17 → 18 (02A) → 19 (02B) = salir despedido y
  recuperarse; 23/24 (0A6/0AC) en el suelo. La sonda ahora también arranca tras el vídeo de antes (no lo hacía).
- **Training No.2: recuento hecho, sin probar entero → sigue en gris.** `VigilarEntreno2` cuenta por transición de
  la acción del jugador: guardia = 26 (golpe parado), esquiva = 38/0C5, teletransporte = 38/0BE; con 5 de cada pone la
  vida del rival a 0 (el "Success" de IW acaba el combate) y la misión cuenta (`reto.iw == 4`). Como la esquiva y el
  teletransporte solo se midieron una vez y no dio tiempo a probarlo en marcha (la cola de pruebas iba a 10-15 min),
  queda en gris («esquiva y teletransporte medidos una vez; sin probar entero») salvo con la sonda (`DBZ3_R1_TRAZA`);
  escenario `r1_entreno2` listo para probarlo.
- **Entrenamientos 3, 4, 5, 7, 8 y 10 siguen en gris**: 3 (golpe que manda lejos + persecución) y 4 (golpe imparable,
  agarre, ataque cargado) existen en B3 pero falta medir sus acciones con la sonda; 5 (voltereta), 7 (carrera en vuelo),
  8 (aura) y 10 (objetos y burla) no existen en B3. Jugables: 1, 6 y 9 (y 2, ver arriba).
- **Tabla real de insignias: no hecha.** El `#AMA` de IW (334/4) no es el de B2 que descifró R2 (`ama_tabla.py`, registros
  de 0x40 B con listas de 0x20/0x30 B): su cabecera es `+0x10` 190 objetos (tabla en `+0x14`) y `+0x18` 286
  animaciones (tabla en `+0x1C`); cada objeto es un árbol de nodos de 0x20 B con su animación (el 30, Raditz, usa la 83
  y la 84; el 32, Nappa, la 88 y la 89) y cada animación lleva 4 pistas con offsets que no apuntan a datos legibles
  con la base del archivo. Sondas en `E:\DBZ3HD_trabajo\R1\ama_iw*.py`. Sigue la tabla emparejada a ojo (`kInsignia`),
  que coincide con los rivales en todas las capturas revisadas.
- **Ports de IW (aviso de R9, tercera vuelta):** Goku GT, Baby, Vegeta GT y Pan otra vez en gris con «port de IW:
  definitiva en revisión» (las órdenes de guion de IW pueden colgar su definitiva cuando la lanza el COM, y el COM usa
  cápsulas en las misiones); Janemba sigue (pasó misiones enteras).
- **Regresión final (capturas revisadas a ojo):** `iw_mapa` PASS `D:\DBZ3HD\_pruebas\20261016-052111_modos_test_iw_mapa`
  (el vídeo de después no se saltaba con B: las capturas finales eran del vídeo; arreglado el escenario para saltarlo
  con A), `iw_misiones` PASS `...\20261016-052454_modos_test_iw_misiones` (mapa con el entrenamiento 1 y Piccolo),
  `r1_historia` PASS `...\20261016-053535_modos_test_r1_historia` (vídeos, misión 11, rango S, 4800 Zenie, mapa
  05/30). `r1_road` PASS `D:\DBZ3HD\_pruebas\20261016-055307_modos_test_r1_road`
  (ahora entra por la rueda nativa: sus nombres de captura de la elección son de antes; combate 97 superado, resultado
  RANK A con New! y 22400 Zenie). `r1_road_rueda` PASS `D:\DBZ3HD\_pruebas\20261016-055540_modos_test_r1_road_rueda` (rueda nativa, 5 Y, mapa del Road con Kid Gohan, 01/30). Estas cinco corrieron con el build de las 05:21; lo cambiado después (Training No.2 en gris salvo con la sonda, puertas de los ports, escenario de `iw_mapa`) se compiló a las 05:51: ver la última línea.
  **Con el build final:** `iw_mapa` PASS `D:\DBZ3HD\_pruebas\20261016-060020_modos_test_iw_mapa` (el vídeo de después se
  salta con A; `e_mapa_despues.png` ya es el mapa y `g_menu.png` el menú de la Pasarela).
- **Archivos:** zona IW de `src/pasarela_misiones.cpp` (`VigilarEntreno2`, Training No.2, sonda tras el vídeo, puertas
  de los ports), `tools/pruebas.py` (escenarios nuevos `r1_traza2` y `r1_entreno2`; en `iw_mapa`, A salta el vídeo).

### R2 · Campaña de B2 (Dragon World) + Nave de Babidi (2026-10-16, 02:45)
- **Nave de Babidi (nuevo, `src/pasarela_nave.cpp`):** sexta entrada de la Pasarela del HUB, "Nave de Babidi
  (Budokai 2)" (en gris sin la ISO de B2). Pantalla con `S::mode` (cinta, marco de la lista, bocadillo, ayuda, caja de
  Kili en la esquina): arriba/abajo eligen una de las 4 pruebas, izquierda/derecha el luchador (los de B2 que tiene B3),
  A pide el combate nativo (tarjeta VS de la Pasarela). Reglas leídas del **ELF del jugador** con el descompilado de
  Ghidra (`E:\DBZ3HD_trabajo\ghidra\salida\b2.c`, ELF europeo): pagos `0x265AB0` = I x25 por enemigo (los combates se
  encadenan con la vida que queda hasta caer), II x2 por segundo aguantado con una barra, III x4 por golpe en 99 s, IV x3
  en 99 s. Barra de Kili `0x262DD0`: el Kili llena la barra hasta 10000, cada umbral da su premio y al pasar de 10000 da
  la vuelta. Premios = tabla del ELF `0x278320` (43 filas ID + Kili, buscada por firma; 100 Dabura ... 10000), leída en
  marcha. Progreso `nave_kili`.
- **#AMK descifrado: no es texto.** 139 bloques en DATA_<idioma> 67-140 y 394-495, idénticos en los 5 idiomas: tablas de
  pistas (registro de 16 B: id, nº, flag, offset; cada dato = u16 tiempo creciente, u8 0-5, u8 0/1), probablemente el
  movimiento de boca de las voces. Los textos del tablero de B2 son imágenes #AMT (las que ya se usan, DATA_<idioma>
  146+k). Scripts y volcados: `E:\DBZ3HD_trabajo\R2\amk\`.
- **Condiciones de combate que faltaban (`Reglas`, `pasarela.h/.cpp`, 3 campos):** `vida_baja` (parte de la vida máxima
  por segundo; la pausa no cuenta; nunca baja de 1), `sin_bloqueo` (apaga el botón que solo da G en la configuración del
  mando del jugador y lo devuelve al acabar o al salir; los atajos con G siguen), `ki_cero` (el jugador empieza sin ki).
  El tablero las aplica: condición 2 = ki a 0 de los dos, 3 = vida que baja (1 %/s), 5 = sin bloqueo.
- **Movimiento de los rivales (port del original):** cada pieza tiene un tipo de movimiento (pieza +0x1C, tabla
  `0x378FE0`, elige dirección en `0x276B80`): 0 quieto, 2 ir a una casilla, 3 al azar, 4 perseguir a la pieza contraria
  más cercana, 5 seguir a una pieza, 6 perseguir a la de menor valor +1, 7/8 ir a un objetivo o perseguir. El paso
  (`0x273180`): de las 4 vecinas, en su orden, la primera que deja menos casillas hasta el objetivo (tabla de distancias
  n x n); si en ella hay una pieza aliada, no se mueve. El tipo 4 ya está porteado tal cual (`Distancias` + `PasoHacia`
  en `pasarela_b2.cpp`). Los tipos los ponen los scripts de cada fase (orden `0x293EE0` → `0x276D80`).
- **Objetos (RE, `0x27A980`, +0x42 tipo, +0x43 valor):** zenie = valor x 100 (`0x254620`, ya así); cápsula al azar
  (`0x2835F0`) = una al azar del rango `valor` que pueda usar tu equipo y que no tengas al máximo (tabla de cápsulas
  `0x6942A0`, 32 B); daga, armadura y los demás lanzan su efecto en `0x26FE80(tipo + 6)`. Mapa de módulos: `dw_main.c`,
  `dw_view.c`, `dw_point.c` (casillas, `0x273000`), `dw_piece.c` (piezas de 36 B en `0x69B7A0`, tarea DWC_CTRL
  `0x276600`), `dw_shenglong.c`, `dw_result.c`, `script.c`/`scr_func.c` (motor de scripts), `mn_babi.c` (nave, tareas MNB_*).
- **Archivos:** `src/pasarela_nave.cpp` (nuevo); `CMakeLists.txt` (su línea en las dos listas); `src/pasarela_misiones.cpp`
  (entrada `kNave`/`kNaveF`, `CombateNave`, `DrawNave`, condiciones del tablero, alto de las filas del menú para 6
  modos, reinicio en la prueba); `src/pasarela.h` y `src/pasarela.cpp` (los 3 campos); `src/pasarela_b2.cpp`
  (movimiento del tipo 4, prueba `DBZ3_TEST_B2_CONDICION`); `tools/pruebas.py` (escenario `r2_nave`, `tablero --par
  condicion=N`). Respaldos: `E:\DBZ3HD_trabajo\R2\respaldo\`.
- **Prueba:** compila (`tools/compilar.py`, sin errores). Capturas revisadas a ojo.
  `correr escenarios_test r2_nave` PASS: `D:/DBZ3HD/_pruebas/20261016-023534_escenarios_test_r2_nave` (menú de pruebas
  con "Siguiente premio: Dabura (100 Kili)" del ELF, tarjeta Goku contra Piccolo, combate de 99 s, resultado "+0 Kili":
  la prueba no golpea). `tablero` PASS: `D:/DBZ3HD/_pruebas/20261016-023858_escenarios_test_tablero` (última compilación);
  `tablero --par condicion=3` PASS: `D:/DBZ3HD/_pruebas/20261016-022305_escenarios_test_tablero` (la barra de Goku baja sin recibir
  golpes); `tablero --par condicion=5` PASS: `D:/DBZ3HD/_pruebas/20261016-022930_escenarios_test_tablero` (registro "sin bloqueo (mando
  0: 1 botones de guardia apagados)").
- **Pendiente (`ponytail` en el código):** el tipo de movimiento de cada rival y cuándo mueve (hoy todos los que se
  mueven usan el tipo 4 cada dos turnos), y los eventos y diálogos de cada fase: los ponen los scripts, sin descifrar.
  Pista: las tablas T3-T6 del `#YDW` que no se usaban (fase I: T3 687 registros de 20 B, T4 116 de 28 B, T5 142 de 16 B,
  T6 451 de largo variable; valores 0x12D-0x12F con pinta de órdenes) son casi seguro esos scripts, y su intérprete es
  `script.c`/`scr_func.c`. Las escenas de historia en 3D irán con `cinematica.h` (R3) cuando estén descifradas. Además:
  efecto exacto de daga y armadura (`0x26FE80`) y que cambien el combate (falta un campo de ataque/defensa en `Reglas`,
  fuera de esta vuelta); rivales y escenario de cada prueba de la nave (hoy al azar y fijo); detector de onda devuelta
  (la IV cuenta golpes); los premios no entran aún en el inventario de cápsulas de B3 (los IDs son de B2: falta la
  tabla B2 → B3); gráficos de la nave sin localizar en la ISO (letra y marco de B3); ritmo real de "vida que baja".
- **Personajes en 3D en el tablero:** hoy, retrato sobre peana. Con los modelos de B3 es viable de dos maneras:
  (a) sprites hechos en la copia del jugador con `awo_tools/model_render.py` (el mismo render de la selección,
  ~2-3 h: una pose por personaje, guardada en user_data y dibujada en su casilla); (b) dibujarlos en vivo con el motor
  (render a textura como la vista previa de la selección; RE de esa vista, días). Recomendado: (a).

#### R2 segunda vuelta (2026-10-16, 03:05)
- **Gráficos de la nave, los del original:** el modo carga 3 archivos (tabla del ELF `0x378350`: 0x1046B-0x1046D =
  archivo 1, entradas **1131-1133** de DATA_<idioma>). 1131 = 35 piezas (marco de madera, cinta y título "Nave espac. de
  Babidi", Babidi, bocadillos, "Fase 1-4", flechas, silueta de la nave, panel y bola del Kili, cifras, "ganadas / segs. /
  veces / Kili / Hits", "Selecc. Person.", "Selección Fase", 1P/2P); 1132 = las 14 frases de Babidi (presentación,
  explicación de cada prueba, "Ahora escoge a tu oponente", "escoge el sitio donde quieras luchar"...). La nave se
  dibuja ahora solo con eso, en el lienzo de 640x448 del original: selección de prueba con la frase de cada una,
  luchador (1P), oponente (2P) y sitio, y resultado con cifra + palabra del original y la barra del Kili rellenándose.
  Las caras de los personajes son las de B3. ponytail: la 1133 (disposición) está sin descifrar; posiciones a ojo.
- **Rivales y escenario de las pruebas:** no hay tabla: en el original los elige el jugador (lo dicen las frases de
  Babidi #9 y #12). Ahora se eligen igual (oponente de la lista de B2 que tiene B3; escenario entre 9 de B3, porque los
  de B2 no están). En la prueba I, cada victoria trae al siguiente de la lista.
- **Premios al inventario de cápsulas de B3:** tabla B2 → B3 por nombre (`modding resources discord/research/SkillList.dson`;
  `E:\DBZ3HD_trabajo\R2\capsulas\tabla.md`). 23 con equivalente (4 de ellos probables: 398→395, 214→127, 383→343,
  215→128) y 16 sin él (Evil Tornado, Hell Blow Shoot, Merry Swing, Enjoy Browning, Hate Ray Cannon, Innocence Dance,
  Badness Throw, Hate Dancing, Vice Browning, Vice Break, Vanishing Drop, Assault Rain, Innocence Strike, Crazy Smash,
  Mad Kill Spike, Savage Rush); los personajes 486-489 no se dan (B3 ya los tiene). Al ganar un premio se suma una copia
  al inventario de la partida (0x824BA110 + 0x2AFE9, u8 por ID; hasta 9).
- **Daga y armadura:** dos campos nuevos en `Reglas`, `ataque` (daño extra que hace el jugador) y `defensa` (parte del
  daño recibido que se anula; no anula el gasto de "vida que baja"). El tablero suma el % de cada daga y armadura que
  recoge la ficha y lo aplica a sus combates hasta acabar la fase (ponytail: la duración y el efecto exacto de
  `0x26FE80` sin descifrar).
- **Scripts del Dragon World descifrados (formato e intérprete):** T6 = scripts (u32 cuántas + órdenes de 24 B
  {espera en cuadros, op, 4 datos}), intérprete `0x295F20`, tabla de órdenes `0x379EC0` (88; las que bloquean,
  `0x37A020`). Disparadores: **T3** {pieza, otra, turno, script} (-1, 0, N = al empezar el turno N; pieza, otra = al
  tocarse), **T4** contactos con condiciones de banderas, **T5** {pieza, suceso, script, condiciones}. Banderas en
  `0x699260` (número > 0 = encendida, < 0 = apagada). Órdenes identificadas: 44 = saltar a otro script, 50 = tipo de
  movimiento de una pieza, 51/52 = datos de la pieza, 8 = dar cápsula, 9-16 = consultas de casilla/pieza, 17-31 =
  movimientos y cámara, 38/41/42 = ventanas que esperan (diálogos); 4/5/6 = cargar y sonar un archivo del archivo 0 (dato + 450 / 612 /
  943; por cómo se usa, la voz de la frase; sin confirmar); 7 = esperar N cuadros. Lista: `E:\DBZ3HD_trabajo\R2\scr_ops.txt`;
  volcado por fase: `python E:\DBZ3HD_trabajo\R2\ydw3.py <fase 0-8>`.
- **Porteado ya:** los scripts de cada fase se leen del `#YDW` del jugador; al empezar la fase y en cada turno se
  ejecutan los de T3 (-1, 0, turno) con sus órdenes 44 y 50, y cada rival se mueve con su tipo: 0 quieto, 2 hacia su
  casilla, 3 al azar, 4 perseguir (ej. fase I: Nappa va a las casillas 9, 11, 13 y 15 en los turnos 2, 4...). Los rivales
  sin script siguen como antes (cada dos turnos).
- **Revisión de R6 aplicada:** "Elige al compañero de Goku" más grande y en el color elegido; la tira de caras con margen;
  ayuda del tablero más grande; quitada la franja "Dragon World (Budokai 2)" de la tarjeta (solo sale si hay condición).
  La nave usa `S::mode::Help`.
- **Archivos:** `src/pasarela_nave.cpp`, `src/pasarela_b2.cpp`, `src/pasarela_misiones.cpp` (zona tablero/nave),
  `src/pasarela.h`/`.cpp` (`ataque`, `defensa`), `tools/pruebas.py` (`r2_nave` con los nuevos pasos). Respaldos en
  `E:\DBZ3HD_trabajo\R2\respaldo\`.
- **Prueba:** compila (`tools/compilar.py`, sin errores). `r2_nave` PASS:
  `D:/DBZ3HD/_pruebas/20261016-030109_escenarios_test_r2_nave` (capturas revisadas: selección de prueba con "Fase 1-4" y la
  frase de Babidi de la prueba III, 1P, 2P "Ahora escoge a tu oponente", sitio, tarjeta, combate, resultado con "Hits",
  "Kili" y la barra; registro "graficos de la ISO: 35 piezas, 14 frases"). `tablero` PASS:
  `D:/DBZ3HD/_pruebas/20261016-025913_escenarios_test_tablero` (registro "script 333: Nappa pasa al movimiento 2 (9)" en el
  turno 2, como en el original; tarjeta sin la franja).
- **Pendiente:** el resto de órdenes de los scripts (diálogos con imágenes y retratos, apariciones, combates de evento,
  final con Shenron) y los disparadores T4/T5 (contactos y sucesos): el formato y el intérprete ya están localizados;
  falta el significado de cada orden de diálogo (38/41/42) y de qué entrada sale cada imagen. Escenas 3D con
  `cinematica.h` cuando estén. Nombres de los premios con la letra de B3 (su imagen en la ISO sin localizar). Los
  personajes del tablero siguen como retratos (plan en la primera vuelta).

#### R2 tercera vuelta (2026-10-16, 03:40)
- **Disposición exacta de la Nave (sin posiciones a ojo):** la 1133 no es la disposición: es un sonido (ADPCM de PS2).
  El modo carga en realidad 1131 (piezas), **DATA_CMN 905** y 1132 (frases): la tabla del ELF `0x378350` se lee de 4 en 4
  bytes (0x1046B, 0x389, 0x1046C). La CMN 905 es un `#AMA` de 125 animaciones. Cada una trae su rectángulo, su
  lista de fotogramas {tiempo, escala, giro, x, y, z, alfa} en la pantalla de 640x448 y sus cuadros {pieza, recorte}.
  La nave se dibuja ahora con eso: cada pieza en su sitio de reposo con su escala. Animaciones por pantalla, según las
  funciones del modo: marco 4-7, cinta 13, título 16, "Selección Fase" 122, una sola etiqueta "Fase N" (46/50/54/58: las
  4 van en el mismo sitio, cada una con su línea a su casilla), indicador 62 con la casilla amarilla 66-69, Babidi 37 a
  la derecha con el bocadillo 19 a la izquierda, "Selecc. Person." 119, recuadro 25, 1P/2P 29/35, flechas 40/43. En el
  resultado: panel 77, bola 80 con su aguja 81, franja 78, cifras 82/96/107 con "ganadas/segs./Hits/veces" 92-95 y
  "Kili" 106/117. Volcado: `python E:\DBZ3HD_trabajo\R2\nave2\ama_tabla.py`. ponytail: el estado de reposo, sin las
  animaciones de entrada.
- **Lados de la pantalla 4:3:** fuera del lienzo del original, la madera del marco de B2 (pieza 0) repetida y oscurecida
  sobre negro, como en las remasterizaciones. Ya no se ve el fondo del HUB.
- **Nombres de la tarjeta VS:** bajan `S::mode::kPad` (en `Cara()` de `pasarela_misiones.cpp`: el nombre se dibuja
  aparte). Vale para todos los modos de la Pasarela.
- **Diálogos y sucesos del Dragon World:**
  - Ventana de mensaje: la orden 38 {a, b} abre la ventana del ELF (`0x26F280`/`0x26EFC0`), que dibuja las animaciones
    a y b del `#AMA` CMN 343+5k con las texturas de DATA_<idioma> 146+k. Son 2 de los 16 archivos que carga cada fase;
    tabla del ELF en `0x377B70`, script `E:\DBZ3HD_trabajo\R2\dlg\tabla_tablero.py`.
  - Ya porteada: el tablero lee esas ventanas y las enseña cuando un script las pide. Ejemplo, fase I: el script 301 del
    turno 1 muestra "Fase 1" y "¡Obtén las Bolas de Dragón antes que Nappa!" en sus sitios del original. Las de salida
    (las que reposan fuera de la pantalla) no se ponen.
  - Corrección de la vuelta anterior: las órdenes 4/5/6 no son voz sino efectos `#SPX` de DATA_CMN (dato + 450/612).
- **Archivos:** `src/pasarela_nave.cpp`, `src/pasarela_b2.cpp`, `src/pasarela_misiones.cpp` (`Cara`). Respaldos en
  `E:\DBZ3HD_trabajo\R2\respaldo\` (`*_vuelta2/3`).
- **Prueba:** compila (`tools/compilar.py`, sin errores). Capturas revisadas a ojo.
  `r2_nave` PASS: `D:/DBZ3HD/_pruebas/20261016-032124_escenarios_test_r2_nave` (registro "35 piezas, 14 frases, 125
  animaciones"; selección con "Selección Fase", "Fase 3" con su línea a la casilla amarilla, Babidi a la derecha;
  resultado con la bola y la aguja; lados con la madera de B2). `tablero` PASS:
  `D:/DBZ3HD/_pruebas/20261016-033200_escenarios_test_tablero` (registro "script 301: mensaje 0 y 78": en b_tablero salen
  "Fase 1" y el objetivo en sus sitios del original; "script 333: Nappa pasa al movimiento 2 (9)"). Tarjeta VS con los
  nombres más abajo: `D:/DBZ3HD/_pruebas/20261016-032441_escenarios_test_tablero/f_tarjeta.png`.
- **Pendiente:** las órdenes 41/42 (pantallas que esperan: combates de evento) y el resto de órdenes; los disparadores
  T4/T5 (contactos y sucesos con banderas); el final con Shenron y los deseos (`dw_shenglong.c`, sin descifrar), entre
  ellos el que abre la Nave. Las animaciones de entrada y salida de las ventanas y de la Nave (están en las listas de
  fotogramas; hoy solo el reposo). En la fase I no hay escena 3D en los scripts: `cinematica.h` no hace falta todavía.

#### R2 cuarta vuelta (2026-10-16, 04:30)
- **Scripts del Dragon World: más órdenes y los disparadores.**
  - Órdenes: 1/2/3 encienden, apagan o cambian una bandera (bitfield del ELF `0x699260`). 41/42 son el combate del
    contacto: el tablero ya lo pide al tocarse, así que solo se apuntan en el registro. 58-62 y 63-66 son
    animaciones y movimientos de pieza (estado de dibujo 0x0F-0x13): sin portear.
  - **T4** (`0x27F510`) = contactos {pieza A, pieza B, casilla (0 = cualquiera), script, -, -, n.º de banderas,
    banderas}. Vale si los personajes de A y B son los de las dos fichas que se tocan y en esa casilla. Ya porteado:
    al tocarse se ejecuta el script de la primera fila que vale.
  - **T5** (`0x27F3A0`) = {fila de T4, suceso, script, banderas}: lo que pasa tras ese combate. Ya porteado: 3 = gana el
    jugador y 4 = pierde (sin confirmar); 5 = cualquiera.
  - **T3** con dos piezas = contacto por pieza: también porteado.
  - Las banderas se ponen a 0 al empezar cada fase.
- **Final con Shenron y sus deseos:** `dw_shenglong.c` carga 19 archivos (tabla del ELF `0x37A600`); los deseos son
  imágenes de DATA_<idioma> 540 ("Portal de la nave de Babidi" y los trajes de Bulma), 539 (fusión y Pothala) y 538
  ("Avance" de cada personaje). Al ganar la fase IX con las 7 Bolas sale la lista de deseos con esas imágenes (arriba y
  abajo, A pide). El ELF (`0x29A820`) da el premio de cada deseo con la misma función que los premios de la Nave.
  - **El portal abre la Nave de Babidi**, como en B2. La entrada sigue visible en la Pasarela. Hasta pedir el deseo
    sale en gris con el aviso "Junta las 7 Bolas en el Dragon World y pídele a Shenron el portal de la nave de Babidi".
  - Tras el deseo, las Bolas se dispersan y se vuelve a la fase I.
  - ponytail: los otros deseos no tienen equivalente en B3 todavía. El fondo de Shenron (CMN 321-332, 849/850) aún no
    sale: se ve el tablero oscurecido.
- **Animaciones de entrada (las claves del `#AMA`, a 60 cuadros por segundo):** las ventanas de mensaje del tablero
  entran como en el original (por ejemplo, "Fase 1" baja desde arriba y el objetivo sube desde abajo). Las piezas de
  la Nave entran con su escala y posición de cada clave al cambiar de pantalla.
- **Archivos:** `src/pasarela_b2.cpp`, `src/pasarela_nave.cpp` (`Abierta`, claves), `src/pasarela_misiones.cpp` (aviso de
  la Nave cerrada, la prueba la abre), `tools/pruebas.py` (`r2_fase1`, `r2_deseo`). Respaldos en
  `E:\DBZ3HD_trabajo\R2\respaldo\` (`*_vuelta4*`).
- **Prueba:** compila (`tools/compilar.py`, sin errores). Capturas revisadas a ojo.
  - **Fase I entera** (`r2_fase1`, automático: Goku va al rival más cercano y `DBZ3_TEST_RINGOUT` gana cada combate)
    PASS: `D:/DBZ3HD/_pruebas/20261016-035509_escenarios_test_r2_fase1`. Registro:
    - presentación del script 301 ("Fase 1" y el objetivo entrando);
    - 7 combates y "contacto Goku y Saibaman: suceso 25 (script 61)";
    - "contacto Goku y Nappa: suceso 18 (script 51)", que enciende una bandera y salta al 332: Nappa pasa a
      perseguir, como en B2;
    - "tras el suceso: script 302" tras cada combate;
    - Nappa va a las casillas 9/11/13/15 por turnos;
    - "fase 1 completada" y la presentación de la fase II ("Fase 2", "¡Derrota a Frieza!").
    - La pantalla "¡Fase completada!" solo dura un instante en automático: se ve en el registro, no en la captura.
  - `r2_deseo` PASS: `D:/DBZ3HD/_pruebas/20261016-041002_escenarios_test_r2_deseo` (lista de deseos con las imágenes
    del original; "deseo: portal de la nave de Babidi (Nave abierta)").
  - `r2_nave` PASS: `D:/DBZ3HD/_pruebas/20261016-040509_escenarios_test_r2_nave` (las piezas entran con sus claves; la
    prueba abre la Nave como si se hubiera pedido el deseo).
- **Pendiente:** las animaciones de pieza (órdenes 58-66), la cámara (17-31) y los efectos `#SPX` (4/5/6) de los
  scripts; qué tipo de movimiento tiene cada rival sin script; el fondo y la animación de Shenron; equivalentes en B3
  de los otros deseos.

#### R2 quinta vuelta (2026-10-16, 05:15)
- **Fases II-IX en automático (`r2_fases`, `--par desde=1`):** hechas y comprobadas en el registro **II, III y IV** ("fase 2
  completada (bolas 0x2)", "fase 3 completada", "fase 4 completada (bolas 0x8)"). La **V** quedó en marcha con su jefe
  (Majin Bu) aturdido y perdiendo barras cuando se acabó el tiempo; **VI-IX sin probar**. Cada fase tarda 5-13 min en
  automático (20-30 combates en las grandes), así que las 8 seguidas no caben en una vuelta. Registros guardados:
  `E:/DBZ3HD_trabajo/R2/fases_II_III.log` y `fases_IV_V.log`. Visto en el registro: en la fase IV el script 312 coloca a
  Cell en su casilla (orden 55) cuando le toca aparecer, como en B2. Bolas: la ficha va a buscarlas, pero si el jefe la
  alcanza antes, la fase acaba sin Bola (fase III).
  - Arreglos para que el automático juegue todas las fases: mueve la ficha elegida, sea Goku o no (en la fase III no
    hay Goku). Va primero a la Bola de la fase (la busca si está enterrada) y después al rival más cercano que pueda
    luchar en B3 (los que solo son de B2 se saltan).
  - Órdenes nuevas de los scripts: 55 {pieza, casilla} coloca una pieza (ELF `0x27A370`); 57 {pieza, otra} la pone en la
    casilla de la otra (`0x27A3F0`); 72/73 cambian su ficha o retrato (`0x276B30`/`0x276AD0`), por ejemplo las formas.
    35/36 suman a dos contadores de la pieza (`0x276E10`/`0x276DE0`): sin portear.
- **Shenron (`dw_shenglong.c`, 19 archivos):** su ventana (CMN 850, colocada con el `#AMA` CMN 849: marco en (320, 320) y
  filas a 284/320/356) y sus frases (DATA_<idioma> 537): "Te concederé un deseo." → la lista de deseos en las 3 filas
  del original → "Sí, eso es aceptable." → "Ahora debo irme. Esperaré a que vuelvas a llamarme." → fase I. Shenron en 3D
  es el modelo de B2 (CMN 327/328/330, `#AMB`) con efectos `#SPX` (329/331/332). Por la regla de los personajes, iría
  con un modelo de B3 de Shenron, que no se ha localizado. ponytail: de fondo, el tablero oscurecido.
- **Deseos → B3:** Pothala <Vegetto>(Vegeta) da la cápsula **192** (Potara <Vegito> de Vegeta) al inventario. Sin
  equivalente en B3, se listan en el código: Fusión <Tiencha>, Pothala <Gokután> (Goku y Mr. Satán), Pothala
  <Kibitoshin>, los 5 trajes de Bulma (no es luchadora en B3) y los 29 "Avance" (B3 no tiene esas cápsulas). El portal
  abre la Nave, como antes.
- **Archivos:** `src/pasarela_b2.cpp` (automático, órdenes 55/57/72/73, Shenron, deseos), `tools/pruebas.py`
  (`r2_fases`, pasos nuevos de `r2_deseo`). Respaldos en `E:\DBZ3HD_trabajo\R2\respaldo\` (`*_vuelta5*`).
- **Prueba:** compila (`tools/compilar.py`, sin errores). Capturas revisadas a ojo. `r2_fases --par desde=3` (fases IV-V):
  `fase 4 completada` (registro arriba). `r2_deseo` PASS: `D:/DBZ3HD/_pruebas/20261016-051329_escenarios_test_r2_deseo`
  (ventana de Shenron del original con "Te concederé un deseo.", lista de 3 filas con el portal en el centro, "Sí, eso es
  aceptable.", "Ahora debo irme...", vuelta a la fase I).
- **Pendiente:** animaciones de pieza (58-66) y cámara (17-31) de los scripts (no hubo tiempo); Shenron en 3D;
  revisar a mano los diálogos de cada fase.

#### R2 sexta vuelta (2026-10-16, 06:20)
- **Fases V-IX, una prueba por fase** (`r2_fases --par desde=N --par hasta=N+1`). Dos copias en paralelo
  (`escenarios_test` y `escenarios_test2`) y nunca más de 3 juegos abiertos contando los demás frentes: el script
  `E:\DBZ3HD_trabajo\R2\correr_fases.sh` espera hueco y usa un cerrojo. **Todas llegan a
  "fase completada"** (registro de cada prueba en `D:/DBZ3HD/_pruebas/`):
  - V: `20261016-060134_escenarios_test3_r2_fases` ("fase 5 completada (bolas 0x10)"). Antes se atascaba: Babidi
    (personaje solo de B2, inalcanzable como en el original) tapaba el camino al jefe. Ahora el automático rodea a los
    rivales que solo son de B2.
  - VI: `20261016-053322_escenarios_test2_r2_fases` (0x20). VII: `20261016-055802_escenarios_test_r2_fases`.
  - VIII: `20261016-054752_escenarios_test2_r2_fases` (0x80). IX: `20261016-060544_escenarios_test_r2_fases`.
  - Con II-IV de la vuelta anterior, las 9 fases están comprobadas.
  - También arreglado: el automático busca al jefe por su casilla (la puede compartir con otro rival) y, si no llega,
    va al rival más cercano. Antes, en la fase V los turnos pasaban sin moverse nadie.
- **Nombres:** Dabura (37), Videl (15) y Kaio-shin (16) en la lista de nombres de `pasarela.cpp`; antes salían "#37"...
- **Órdenes de pieza:**
  - 63/64 {pieza, casilla}: la pieza va andando a esa casilla (ELF `0x277540`). Porteado, con el deslizamiento del
    tablero.
  - 58-60: animación del modelo 3D de la pieza (+0x1C). 61/62: la giran hacia un ángulo (+0x24, pasos de
    0x800/0x10000). 65/66: la llevan a un punto del mapa. Descifradas y documentadas, pero sin efecto mientras las piezas
    sean retratos: irán con los modelos.
- **Shenron en 3D:** el modelo de B2 (CMN 327: #AMO hijo 0 con su #AMT hijo 1, 11 325
  triángulos y 12 texturas; el 328 es el sitio, las Bolas y la luz) se pinta con el lector de P4 (`ps2_modelo.h`) detrás de
  la ventana de los deseos, girando despacio, con una cámara fija que lo encuadra por el 5-95 % de sus vértices.
  ponytail: en su postura de reposo, sin su animación ni sus efectos `#SPX`. Alternativa sin probar: la escena nativa de
  Shenron del Dragon Universe de B3.
- **Archivos:** `src/pasarela_b2.cpp`, `src/pasarela.cpp` (los 3 nombres), `tools/pruebas.py` (`--par hasta`).
- **Prueba:** compila (`tools/compilar.py`, sin errores). Capturas revisadas a ojo. Fases: arriba. `r2_deseo` PASS
  `D:/DBZ3HD/_pruebas/20261016-061539_escenarios_test2_r2_deseo`: Shenron en 3D detrás de "Te concederé un deseo." y de
  la lista; registro "11325 triangulos, 12 texturas". Nunca hubo más de 3 juegos abiertos.

#### R2 séptima vuelta (2026-10-16, 06:35)
- **Por qué Shenron salía beige liso (medido, no supuesto):** instrumenté el `#AMO` (CMN 327, hijo 0;
  `E:\DBZ3HD_trabajo\R2\shen\partes.py`). Tiene 15 partes y todas traen UV y una textura válida (0-11) de su `#AMT`
  (hijo 1, 12 texturas, `shen\tex327.png`).
  - Las del cuerpo (textura 9, 2358 vértices) y otras llevan **UV fuera de [0, 1]** (v hasta -4,86): en la PS2 se
    repiten (el GS repite por defecto). `ps2_modelo` creaba las texturas **sin repetir**, así que se veían de un color
    liso en la orilla.
  - El beige claro es la textura 10 (vientre, 249/206/150, de 64x64 con pocas líneas): es del modelo, no un fallo.
  - No hay más `#AMT` en el paquete para él: el 328 y el 330 son la escena (nubes y suelo), las Bolas, la luz y sus
    animaciones. Las texturas sin alfa ya se daban opacas en `ps2_modelo`.
- **Cambio en `src/ps2_modelo.cpp` (frente P4; mínimo, avisado aquí):** `Crear` pide la textura **repetida** (`true` en
  `CreateTexture`), como el GS. Respaldo: `E:\DBZ3HD_trabajo\R2\respaldo\ps2_modelo_antes_r2.cpp`. Regresión de P4:
  `iw_minijuego` (Raditz, 0x7D0) PASS con el cambio:
  `D:/DBZ3HD/_pruebas/20261016-063034_escenarios_test3_iw_minijuego` (los aros y Goku se ven igual que antes). Una
  primera prueba con `--par clave=0` falló porque esa casilla no existe ("minijuego -1"); no tiene que ver con el cambio.
  El arreglo de P4 de las texturas con alfa 0 ya estaba y se mantiene.
- **Animación: probada y apagada.** Shenron se carga animable (`CargarSuelto(..., true)`) con su `#AMM` de 8 huesos
  (CMN 328 hijo 3: 290, 240 y 400 cuadros; la 330/3, de 300, es la de irse). Pero con `Posar` las tres animaciones
  dejan el cuerpo como una columna recta:
  - capturas `D:/DBZ3HD/_pruebas/20261016-062320_*` (la 2), `-062737_*` (la 0) y `-062856_*` (la 1);
  - sin animar, el modelo sale bien (`-062620_*`).
  Los pesos o las pistas de los modelos de B2 no casan con el lector, hecho para IW: hay que mirarlo en `ps2_modelo`
  (P4) antes de encenderla. Mientras, Shenron sale en su postura de reposo; `DBZ3_TEST_SHEN_ANIM=N` (o
  `r2_deseo --par anim=N`) pone la animación N para seguir probando.
- **Encuadre:** del 2 % al 98 % de sus vértices; su alto ocupa ~480 de los 900 del área (de y 20 a 500), por encima
  de la ventana, sin salirse por arriba.
- **Escena:** el cielo oscuro con las nubes del original (CMN 328 hijo 1, textura 0, repetida y oscurecida) por toda la
  ventana, en vez del tablero. ponytail: el modelo del sitio (328 hijo 0, 5 huesos) y los efectos `#SPX` no salen.
- **Prueba:** compila (`tools/compilar.py`, sin errores). `r2_deseo` PASS
  `D:/DBZ3HD/_pruebas/20261016-063143_escenarios_test2_r2_deseo`, revisado a ojo: Shenron con sus escamas, ojos rojos,
  cuernos, bigotes y vientre, enroscado y entero, sobre el cielo de nubes y encima de la ventana "Te concederé un deseo.".
  Nunca más de 3 juegos abiertos.

#### R2 octava vuelta (2026-10-16, 06:55)
- **Por qué la animación de Shenron quedaba en columna (instrumentado, `E:\DBZ3HD_trabajo\R2\shen\anim_dump.py`):**
  - Su esqueleto (CMN 327/0, 8 huesos en cadena) tiene giros de reposo nulos y la forma del cuerpo en las traslaciones
    (p. ej. hueso 2: t = (-5,53, 17,75, -5,78)).
  - Su `#AMM` (CMN 328/3) trae claves de giro pequeñas (0,2-1,1°) y **posiciones 0**: en B2 las claves van **sobre** el eje
    de reposo. En IW, en cambio, `Posar` las pone **en lugar del** de reposo.
  - Al sustituir, toda la cadena caía en el origen (la columna).
  - Además, 85 de sus 10 761 listas de pesos no tienen peso (instrumento en el registro: "85 sin peso"). Con `Posar` esos
    vértices iban al origen: los picos que bajaban hasta la ventana.
- **Cambio en `src/ps2_modelo.{h,cpp}` (frente P4; rama B2, IW sin tocar):**
  - `Anim::relativa` (por defecto `false` = lo de IW). Con `true`, `Posar` compone el giro de reposo con el de la clave y
    suma la posición de la clave a la de reposo; una lista sin peso se queda rígida a su hueso.
  - `LeerAmo(..., colores)` (por defecto `false`) lee el color de cada vértice (`vtype & 2`: RGBA en floats tras la
    normal) y `Escena` lo usa.
  - Respaldos: `E:\DBZ3HD_trabajo\R2\respaldo\ps2_modelo_antes_r2b.*` y `_r2c.*`.
  - Regresión de P4 `iw_minijuego` (0x7D0) PASS: `D:/DBZ3HD/_pruebas/20261016-063713_*` (con `relativa`),
    `-064104_*` (con colores) y `D:/DBZ3HD/_pruebas/20261016-064430_escenarios_test3_iw_minijuego` (versión final).
- **Shenron animado:** la animación 2 (400 cuadros, en bucle) con `relativa`. El encuadre sale del cuadro 0 de la pose
  animada. `r2_deseo` PASS: `D:/DBZ3HD/_pruebas/20261016-064214_escenarios_test2_r2_deseo`. Revisado a ojo: el cuerpo se
  mueve enroscado, sin columna ni picos.
- **Escena del original:** el modelo del sitio (CMN 328 hijo 0, 640 triángulos) en las coordenadas de Shenron, con
  sus colores de vértice: cúpula de cielo azul muy oscuro (0,11/0,16/0,32), paredes de nubes translúcidas (alfa 0,2) y
  el suelo de baldosas que se funde en el borde. Ya no hace falta la textura de nubes repetida (queda debajo, por si
  falta el modelo). ponytail: sin la animación del sitio (328/2) ni la luz (328 hijo 11) ni los `#SPX`.
- **Archivos:** `src/pasarela_b2.cpp`, `src/ps2_modelo.h`, `src/ps2_modelo.cpp`, `tools/pruebas.py` (`r2_deseo --par anim`,
  por defecto 2).

#### R2 novena vuelta (2026-10-16, 07:25)
- **Qué carga el tablero de B2 (ELF `0x2350C0`, tabla `0x377C80`):** además de los 16 archivos de la fase, los fijos
  CMN 335 + 334, 337 + 338 (o 339 + 340 con la bandera 4 de `0x6992DC`) y 336.
  - **337/338 = Goku** (#AMO de 44 huesos y 21 variantes, su #AMT con caras, gi y pelo) y **336 sus 19 animaciones**.
  - **335 = la ficha** (#AMO de 21 huesos y 3 partes: la carta con el retrato, textura 0, y la peana con el dorso de la Z,
    textura 1, que se cambian por las de cada pieza de 342+5k) y **334 sus 26 animaciones**.
  - Los demás personajes no tienen modelo propio en el tablero: en B2 son esas fichas 3D (las mismas fichas de 342+5k
    que ya usábamos en 2D). Instrumento: `E:\DBZ3HD_trabajo\R2\shen\partes335.py` y `partes337.py`.
- **Porteado:** Goku en 3D en su casilla, con su animación de espera y andando durante el paso (orden 63/64 o
  movimiento normal), girado hacia donde va. Cada pieza es la ficha 3D con su retrato y su dorso, animada (desfasada
  por pieza). La cámara es casi de frente, 20 grados desde arriba, con el pie en la casilla del tablero (proyección del
  tablero). Se pintan en el orden de profundidad de las piezas, como antes.
  - Si falta un modelo o una textura, se usa el dibujo 2D de antes.
  - El Saibaman sigue siendo la cabeza que asoma del agujero (sprite del original).
- **Coste:** Goku tiene 2586 triángulos y la ficha 124; se posan en CPU en cada cuadro (las fichas comparten una pose
  por cuadro). No hace falta LOD.
- **Cambios en `src/ps2_modelo.{h,cpp}` (P4; solo en el camino `colores`, que pide B2; IW sin tocar):**
  - `CargarSuelto(..., colores)`.
  - En las partes sin textura, el color de material de la parte (+0x30 RGBA; el pelo de Goku, vtype 0x190, es negro
    (0,0,0) y salía gris).
  - El bit alto de vtype, en B2, es mezcla con el alfa de la textura y no translucidez fija: la carta de la ficha sale
    opaca, con la forma del retrato.
  - Respaldos: `ps2_modelo_antes_r2d.*` y `_r2e.cpp`.
- ponytail: las animaciones elegidas (Goku: espera 5 de 120 cuadros, andar 4 de 36; ficha: 0 de 72) van por su largo,
  sin confirmar con el original. Se cambian con `DBZ3_TEST_GOKU_ANIM="espera,andar"` y `DBZ3_TEST_FICHA_ANIM`. Las
  órdenes 58-62 (animar o girar la pieza) siguen sin conectar a estas animaciones.
- **Prueba:** compila (`tools/compilar.py`, sin errores). Capturas revisadas a ojo.
  - `tablero` PASS: `D:/DBZ3HD/_pruebas/20261016-072101_escenarios_test2_tablero`. Goku en 3D con el pelo negro y los
    pies en su sombra; Piccolo y Nappa como fichas 3D: la carta hexagonal con su retrato flotando sobre la peana con la
    Z; los Saibaman, cabezas en su agujero. Registro: "Goku 3D: 2586 triangulos", "ficha 3D: 124 triangulos".
  - Goku andando y girado tras moverse: `D:/DBZ3HD/_pruebas/20261016-065752_escenarios_test2_tablero/d_movimiento.png`.
  - `r2_fase1` PASS: `D:/DBZ3HD/_pruebas/20261016-071104_escenarios_test_r2_fase1` ("fase 1 completada (bolas 0x1)").
  - Regresión de P4 `iw_minijuego` (0x7D0) PASS: `D:/DBZ3HD/_pruebas/20261016-072331_escenarios_test3_iw_minijuego`.

#### R2 décima vuelta (2026-10-16, 07:50)
- **Toon de P4 en las figuras del tablero:** Goku y las fichas 3D pasan `ps2::Escena::Toon` a `Poner`, con el borde al
  0,9 % de su alto (sus partes traen normales, vtype & 1). Se ven con 3 tonos y su borde negro. `tablero` PASS:
  `D:/DBZ3HD/_pruebas/20261016-074110_escenarios_test2_tablero` (revisado a ojo: Piccolo y Goku con borde).
- **Shenron sin toon (aviso a P4):** con `Toon{}` (borde 0,15), Shenron animado sale roto: triángulos enormes por toda la
  pantalla (`D:/DBZ3HD/_pruebas/20261016-074244_escenarios_test3_r2_deseo`). Goku, también animado y con pesos, sale
  bien. Diferencias de Shenron: 10 761 listas de pesos, 85 de ellas sin peso (rígidas por la rama de B2), y es mucho
  más grande. Hasta que P4 lo mire, Shenron se pinta sin toon. `r2_deseo` PASS:
  `D:/DBZ3HD/_pruebas/20261016-074434_escenarios_test3_r2_deseo`.
- **Sin ediciones simultáneas:** en esta vuelta no he tocado `ps2_modelo.{h,cpp}` (la versión de P4 de las 07:32 se usa
  tal cual); solo `src/pasarela_b2.cpp`. Respaldo: `E:\DBZ3HD_trabajo\R2\respaldo\pasarela_b2_vuelta10.cpp`.

### R6 Pulido UI/HUD (2026-10-16, 02:45)
- **Hecho (esquema común `S::mode`, `game_style.h/.cpp`):** medido en el Duelo: la línea del marco (`Frame`) va 18 px
  dentro de p0..p1 → `kLine` 18, `kPad` 22 (las filas empiezan dentro), `kText` 34 (margen del texto) y `Stripe()`
  (franja elegida sin pisar la línea, como el juego). Antes la franja tapaba la línea del marco por los lados y por
  arriba, y las filas tocaban la línea de abajo. El bocadillo nativo ya no tiene la muesca blanca arriba (el borde de
  arriba se estira desde un tramo limpio 60..330 de la textura). HUD de los modos en combate en un solo bloque
  (`kHudY/kHudH/kHudInfoY/kHudLine`): el marcador "Misión 1/3" va centrado **debajo** del contador nativo de rivales
  (cara + "2/2", a la derecha del reloj) y las barras de vida; las líneas de reglas ("Daño 110%", "¡Aguanta!",
  modificadores) seguidas debajo, misma letra y ancho máximo. `rpg::Hud` recibe la y libre (`modos.h`).
- **Aplicado:** `rpg.cpp` Lista, ListaTienda (botella al alto de la fila), Lineas, selector (marco del luchador de
  84 px con la cara dentro), cuadro del torneo (caras dentro del marco); `modos.cpp` Marco (entre/fin), ranking
  (marco al alto de las filas que hay, columnas dentro, título de página que cabe), laboratorio (resumen, tabla y
  opciones sin solapes, **flechas nativas** en luchador/rival en vez de "< >"), anfitrión del Modo Héroe
  `hub/anfitrion_heroe` si existe (si no, el Kaio); `hub_panel.cpp` HUB con fondo: marco propio que encierra las 8
  franjas (antes las franjas y los iconos pisaban la línea), sin tocar el recuadro de página ni el bocadillo, que
  baja (`kFrameY*`, `kBubbleY*`, `BubbleText` con centro y alto).
- **Archivos:** `src/ingame/game_style.h/.cpp`, `src/ingame/hub_panel.cpp`, `src/modos.cpp` (dibujo), `src/modos.h`
  (firma de `rpg::Hud`), `src/rpg.cpp` (dibujo, no el héroe), `tools/pruebas.py` (escenarios `r6_ranking`,
  `r6_torre`, `r6_hud`). Recortes de revisión en `E:\DBZ3HD_trabajo\R6\`.
- **Prueba (compila con `tools/compilar.py`; capturas miradas a ojo, 1280x720 y 1600x900):** `hub_lab` PASS
  `D:\DBZ3HD\_pruebas\20261016-021834_hub_test_hub_lab` (HUB) y `...-022342_hub_test_hub_lab` (laboratorio con
  flechas); `tienda` PASS `...-021933_hub_test_tienda` (selector y tienda); `r6_ranking` PASS
  `...-022520_hub_test_r6_ranking`; `r6_hud` PASS `...-023903_hub_test_r6_hud` (b_hud: contador nativo 2/2 libre,
  marcador y "Daño 110%" debajo; c_entre: entre combates del héroe) y, con el ejecutable final (`kHudY` 210),
  `...-025054_hub_test_r6_hud` (un rival: bloque bajo el reloj, sin tocar barras ni "HITS") y
  `...-024646_hub_test_r6_hud\zz_fallo.png` (fin del héroe, pantalla de `rpg.cpp`; aquel FAIL era la expresión del
  escenario, ya corregida); `r6_torre` PASS `...-022828_hub_test_r6_torre`.
- **Pendiente (R6):** el marcador sale también durante la presentación del escenario (sin HUD del juego, flota solo):
  falta una señal de "HUD del juego visible"; en el menú principal nativo el recuadro "HUB" pisa la barra de abajo
  del marco y el borde del bocadillo (igual que ONLINE en el Duelo; no hay hueco entre los dos); entre/fin de
  `modos.cpp`, Abyss, simulación y torneo cuadrados por cálculo, sin captura (la Torre encadenada no enseña "entre").

#### Para otros frentes (no editado; capturas y qué cambiar)
- **R1** (`D:\DBZ3HD\_pruebas\20261016-011921_modos_test_iw_mapa`): a_mapa: la ayuda de botones de abajo a la
  derecha queda debajo del coche de Bulma ("Guide" tapado) → centrarla (`S::mode::Help`, y 872) o subirla.
  b_ficha: título "Defeat Raditz!" y la línea de abajo "Mission 11: ..." en turquesa diminuto (~18 px) → letra de la
  ficha de IW o `kHead`; esa línea pisa la peana del "?"; "Normal" encima del rombo azul. d_resultado: mismo título
  diminuto.
- **R2** (`...\20261016-012908_escenarios_test_tablero`): a_preparacion: "Elige al compañero de Goku" pequeño y
  turquesa junto al "Fase 1" amarillo de B2; la tira de caras toca el borde derecho del panel → centrarla con margen.
  f_tarjeta: los nombres "Goku"/"Saibaman" van encima de la línea de abajo de sus marcos → bajarlos `kPad`; la franja
  oscura plana "Dragon World (Budokai 2)" no es del juego (y repite la cinta) → quitarla o `S::Frame`; bocadillo sin
  anfitrión y casi vacío → `S::mode::Bubble` con el Kaio o más pequeño. b_tablero: ayuda de abajo muy pequeña.
- **R3** (`...\20261016-021826_camp_test_satan`): a2_dificultad: la franja de agua de "Novel" se sale por arriba a la
  izquierda del cartel de madera → meterla dentro del cartel. b_tarjeta: todo en el tercio de arriba (caras de
  ~140 px) y el 60 % de la pantalla vacío → caras más grandes centradas o la frase del retador en un bocadillo.
  e_charla: texto de B1 apretado (5 renglones, espaciado desigual, pegado al borde izquierdo) → mismo paso entre
  renglones y margen ≥ 30 px. "Récord: 1180 puntos" en turquesa diminuto bajo el cartel.
- **R5** (`...\20261016-021858_du_test_r5_heroe`): `Lista()` empieza ahora las filas en `y0 + D::kPad` con paso 52;
  las flechas de `DibujarHeroe` usan `y0 + 10 + 54 * (i + 0.5f)` → cambiar a `y0 + D::kPad + 52 * (i + 0.5f)` (si
  no, quedan ~10 px altas). El marco de las caras arriba a la derecha (`a.At(660, 12)..(1060, 148)`) se sale por
  arriba (cortado) y toca el cordón de la cinta → empezar en `D::kCornerY0` (40) y caras dentro de `kLine`. Anfitrión
  propio: `modos.cpp` ya dibuja `hub/anfitrion_heroe` en el Modo Héroe si existe → recortar en `generar.py` la
  primera figura de cuerpo entero (de frente) de `assets_propios/png/add/Capsule Corp Character.png` (fondo
  transparente, pies abajo, ~437 px de alto) a `res/ingame_ui/hub/anfitrion_heroe.png`.
- **P4** (`...\20261016-021719_combate_test_iw_minijuego`): a_salida/b_aros: "TIME 04'57"68" va bajo la barra del
  rival, y en el minijuego (un solo luchador) se ve el HUD del rival (barras, V 0) de nadie → ocultar ese lado o
  poner TIME centrado bajo el reloj (bloque `S::mode::kHudY`).

### P4 Minijuegos IW (ronda 4, 02:45)
- Hecho: marcador de combate de B3 oculto por RE (`dbz3::combate::OcultarHud(bool)` en `src/combate.h`, salta la tarea
  "FIX DRAW" del marcador; vale para R3); TIME en su sitio de IW; aros, monedas, bebidas y la nave de Raditz (meta)
  como modelos 3D de IW con sus texturas (lector de `#AMO` en C++); Raditz ya no se ve; dificultad desde la ficha.
- Archivos: `src/iw_minijuegos.*`, `src/combate.*`, `src/eu_map_gen.h` (regenerado), 1 línea en `pasarela_misiones.cpp`,
  `tools/pruebas.py`. Detalle y estudio del campo de IW (3-4 + 4-6 días) en `PORT_EXACTO.md`, sección P4.
- Prueba PASS: `D:\DBZ3HD\_pruebas\20261016-024357_combate_test_iw_minijuego` y `...-024022_...` (nave).
- Pendiente: portar el campo de IW; Bubbles, Piccolo y Radar.

### R7 Vídeos SFD de IW (ronda 4, 03:08)
- **Hecho: reproductor nativo de los vídeos originales** (`src/video_sfd.{h,cpp}`), leídos en marcha de la copia del
  jugador. Formato medido: MPEG-1 PS; vídeo MPEG-2 512x448 a **30,000 fps** (0xE0); audio ADX mono 48 kHz en **dos
  streams** (0xC0 `_jp.sfa`, 0xC1 `_us.sfa`, nombres en la cabecera Sofdec del 0xBF): suena el inglés. **Los tiempos de
  los subtítulos van dentro de cada SFD** (no en el ELF): bloque `CRITAG/USRINFS TYPE=SUBTITLE` en los paquetes 0xBF del
  principio, por canal `CHDAT/CHNO`, `TUNIT=30`, `TIMEDAT{INTIME, DURTIME, CONTENT = imagen}`; canal 2 = inglés y cuadra
  con las imágenes de DATA_USA 77+vídeo en los 22 vídeos (lo lee IW en `Movie_DrawFrameAndSubtitle`, EU 0x338C10;
  tabla completa por vídeo y canal: `E:\DBZ3HD_trabajo\R7\haiku\fd_subtitle_timings.csv`).
- **Decodificador:** el FFmpeg 4.4 del SDK, como objetivo estático propio `dbz3_ffvideo` (`src/video_sfd.cmake`: config
  propio con mpeg1/2video + mpegvideo + sus DSP + parser, 64 .c) enlazado **solo en dbz3.exe** con la libavutil del SDK.
  **Las DLL canónicas no cambian** (ni tamaños ni `verify_release.ps1`). Demux PS, ADX y YUV→RGBA propios.
- **Reproducción:** pantalla completa por encima de todo (ImGui, fondo negro, 4:3), una textura por fotograma (3 en
  rueda), el vídeo va al reloj del audio (waveOut, como `OirVozB1`, con volumen/silencio del juego en cada trozo de 0,1 s),
  subtítulos centrados abajo a escala de la pantalla de PS2 (si se solapan, se apilan). A o START saltan.
- **API para R1** (`#include "video_sfd.h"`): `dbz3::video::Reproducir(iw / "USR" / "FD01B.SFD", dm.subtitulos[video])`
  (las imágenes de DATA_USA 77+vídeo que ya lee `LeerDragonMission`; los tiempos los saca el reproductor del SFD),
  `video::Activo()` (true hasta que acaba o lo saltan) y `video::Saltar()`. Nombres por índice: tabla del ELF 0x4811B0
  (FD01A, FD01B, FD02..FD07, FD08A, FD08B, FD09..FD20). **Mientras `Activo()`, la pantalla de debajo no debe leer el
  mando**: A/START también le llegan al juego (en la prueba con `saltar=1`, la A que salta entró además en Universo
  Dragón del menú nativo).
- **Prueba:** compila (`tools/compilar.py`, sin errores). `correr video_test _r7_video` PASS:
  `D:\DBZ3HD\_pruebas\20261016-025915_video_test__r7_video` (b/c: Raditz con "Well, well. You're a little more
  foolish" / "than I thought!" a su tiempo; imagen y colores bien, sin rayas; e: vuelta al menú al acabar). `--par
  saltar=1` PASS: `...\20261016-030125_video_test__r7_video`. Medido (FD01B): 664 fotogramas en 22,1 s (velocidad
  real), decodificar+RGBA **1,8 ms/fotograma** (máx. 3,3), CPU del hilo 2,1 s en 22 s (~10 % de un núcleo), subir la
  textura 2,0 ms/fotograma en el hilo de la interfaz (máx. 6,8).
- **Pendiente:** conectarlo en R1 (antes/después de cada misión; lo hace R1); bloquear el mando del juego mientras
  suena (para pantallas nativas: en el gancho de mando de `combate.cpp`); voces en japonés (stream 0xC0) y el resto de
  idiomas de subtítulo (canales 3-6; el alemán tiene más líneas y sus imágenes estarían en otro AFS) sin hacer;
  `ponytail:` margen inferior del subtítulo a ojo (24 px de PS2); la música del juego sigue sonando debajo.

### R6 segunda vuelta (2026-10-16, 03:30)
- **HUD del juego visible (a medias):** `dbz3::combate::HudNativoVisible()` (`combate.cpp/.h`, solo lectura): la
  condición de FIX DRAW (mgr+35253 != 1 y bit alto de 0x823CD870), sin `OcultarHud` y con el objeto del marcador
  (*0x82324064). El HUD de los modos (`modos.cpp`) solo se pinta con ella. Funciona en la pose de entrada (sin HUD
  del juego, sin el nuestro: `D:\DBZ3HD\_pruebas\20261016-032223_hub_test_r6_hud\a_hud.png`) y en combate (b_hud), **pero no en la
  vuelta de cámara del escenario**: ahí las tres condiciones dan "visible" y el juego no pinta su HUD
  (`D:\DBZ3HD\_pruebas\20261016-033323_hub_test_r6_hud\a0_presentacion.png`, ya con el objeto). Pendiente: guardar en los ganchos
  de FIX DRAW (`sub_820DC1A8/sub_820DC230`) la hora de la última llamada y exigir < 100 ms (si la tarea no corre en
  la presentación), o buscar el alfa/fundido del marcador (P4).
- **Pantallas que no se veían:** escenario nuevo `r6_pantallas --par que=entre|derrota|fin|sim|torneo|abyss`.
  entre/derrota/fin las pinta `modos.cpp` en el menú principal con un reto de ejemplo (`DBZ3_TEST_R6_PANTALLA`, sin
  mando ni cambios; el panel se crea solo y el HUB se oculta como con un reto de verdad); sim/torneo/abyss van por
  el selector. Vistas a ojo: fin `...\20261016-031544_hub_test_r6_pantallas\a_fin.png` (bien; en esa captura aún
  salía encima el recuadro HUB, ya corregido: `RetoEnCurso` cuenta la prueba), simulación
  `...\20261016-032642_hub_test_r6_pantallas` (bien). Entre/derrota de la primera tanda salieron sin la pantalla (el
  panel aún no existía en el menú; corregido después).
- **Recuadro "HUB" del menú principal:** entre la línea de abajo del marco (~600) y el bocadillo del juego (650) no
  cabe. Ahora va como ONLINE en el Duelo: colgado de la línea del marco (sus esquinas la pisan) y su línea de abajo
  antes del bocadillo (`kBoxY0/Y1` 596..636; antes 606..650 pisaba el bocadillo). Captura
  `...\20261016-030729_hub_test_r6_pantallas\a_derrota.png` (menú principal con el recuadro nuevo).
- **Piezas nativas:** repaso de `modos.cpp`, `rpg.cpp`, `hub_panel.cpp`: marcos, franjas, flechas, botones,
  bocadillo, caja y cifras de Zenie ya son del juego. Quedan propios: las líneas del cuadro del torneo
  (`DibujarCuadro`; candidato: el cuadro del Torneo Mundial de B3) y las cifras doradas `retos/num_*` del marcador
  (candidato: las cifras naranjas del HUD, las del "2/2" del contador de rivales; hacen falta su entrada de
  `data_cmn` y su XXH3 en `native_ui.cpp kPieces`). `ponytail:` no localizadas en esta vuelta.
- **Archivos:** `src/combate.cpp/.h` (solo `HudNativoVisible`), `src/modos.cpp` (prueba de dibujo, HUD con la señal,
  `RetoEnCurso`), `src/ingame/hub_panel.cpp` (`kBoxY*`), `tools/pruebas.py` (`r6_pantallas`, captura de la
  presentación en `r6_hud`). Compila (`tools/compilar.py`, 03:27). Torneo de 4 visto: `...\20261016-032857_hub_test_r6_pantallas\b_torneo.png`
  (bien; las caras aún rozan la línea izquierda de su marco, falta meterlas ~6 px). Abyss y entre/derrota con el
  panel ya creado: sin capturar en esta vuelta (cola de juegos llena).

### R8 · Campos de IW como escenarios de B3 (2026-10-16, 03:35) — fase 1 a medias, sin prueba en juego en verde
- **Hecho:** `tools/campos_iw.py` (nuevo; corre en la máquina del jugador, lee IW de `ps2_games/` y su `us/data_cmn.afs`,
  escribe un mod de escenario como los de B1/B2: casilla nueva, entrada 5060+n, no sustituye nada). Monta el campo sobre
  el escenario de combate de IW **58 = B3 54 (Llanuras, registro 5)** (los 13 escenarios de combate de IW 48..72 son
  **byte a byte** los #MDD de B3 PS2 44..68, comprobado) y lo convierte con el mismo `mdd_a_zdd` de B1/B2: el objeto 0
  pinta el terreno del campo (903/5 + texturas 903/6) y conserva colisión, cámaras y script del base; cielo y nubes
  (903/1-4) en objetos nuevos. `python tools/campos_iw.py mod 903` tarda ~20 s y la **foto sin GPU sale bien** (campo de
  IW con su camino, mesetas, montañas nevadas y cielo: `E:\DBZ3HD_trabajo\R8\foto903.png`). `info` lista los 6 campos.
- **En el juego aún se cierra al empezar el combate.** Causas encontradas una a una (pruebas HUB `DBZ3_TEST_BATTLE=0,7,17`,
  escenario `E:\DBZ3HD_trabajo\R8\pruebas\r8_campo.toml`, copias `stage_test`/`stage_test2`, carpetas
  `D:\DBZ3HD\_pruebas\20261016-025423_*` … `-032810_*`):
  1. Objetos nuevos que **comparten** la colisión del objeto 0: el juego la reubica en su sitio dos veces (lr 0x820CC170,
     punteros dobles). Arreglado: los objetos nuevos van sin colisión.
  2. Objeto 0 con **menos sub-mallas** que el base: el juego pide la sub-malla 1 por índice y lee fuera (lr 0x820BF6C0,
     `sub_820BF558`). Arreglado: el objeto 0 conserva su número de sub-mallas. Confirmado con dos variantes de control
     (terreno con texturas del base: se cierra; modelos del base con texturas del campo: **PASS**,
     `20261016-031117_stage_test2_r8_campo`).
  3. `tema = 5` (luz de Llanuras): se cierra en lr 0x820BF918 (el alta no lleva los grupos de objetos del registro 5).
     Arreglado: tema 16 genérico, como B1/B2.
  4. **Pendiente (el que queda):** consulta de suelo `sub_820CB0A0` (lr 0x820CB134, lee 0x10). La colisión del base
     apunta a su modelo de suelo, y ese modelo ya no está en ninguna sub-malla (el juego solo prepara los modelos que
     dibuja). Pasa también sin cielo ni nubes (`--solo-terreno`) y sin colisión en los objetos nuevos.
- **Límite de ~255 unidades (fase 2):** no hay ninguna constante ni radio en `src/`, docs ni en el código generado
  buscado (subagente haiku). El fuera del ring lo marca `sub_8211F650` (mgr+35569, X+8184 = 0xFF), llamado por tabla.
  Hipótesis (sin medir): el borde es donde se acaba la colisión del escenario (la del base mide eso); con la colisión
  #WCF del campo (~3000) el límite crecería solo. ponytail: sin verificar.
- **Archivos:** `tools/campos_iw.py` (nuevo). Nada de C++ cambiado (no hacía falta compilar); `src/iw_minijuegos.cpp`
  sin tocar (la línea para elegir el escenario espera a que cargue). Scripts y variantes: `E:\DBZ3HD_trabajo\R8\`
  (`variantes.py` C/D/E de control, `pr.py` lanza `pruebas.py` con un sustituto de `_r7_video` mientras R7 lo dejaba a
  medias). Mods de prueba movidos de las copias a `E:\DBZ3HD_trabajo\R8\respaldo\` (las copias quedan sin el campo).
- **Pendiente (estimación realista):** (a) arreglar el 4: dejar el modelo de colisión del base en su sub-malla y
  ocultarlo al dibujar (gancho en el dibujo de la sub-malla, o escala 0 de su hueso raíz solo al pintar) — 1-2 h con
  prueba; luego la línea en `iw_minijuegos.cpp` (registro del alta por título) y captura del minijuego en el campo —
  30 min. (b) Cielo y nubes como objetos propios sin colisión: comprobar si el juego los acepta una vez resuelto (a)
  (si la consulta de suelo recorre todos los objetos, copiar en hondo árbol st50 y registros) — 1-2 h. (c) Colisión
  real del #WCF 904/0 → bloque de colisión HD + árbol st50 (y así el límite de movimiento) — 4-6 días (estudio de P4).
  (d) Los otros 5 campos salen con el mismo comando cuando (a) funcione (897, 884, 900, 906, 909; huesos 112-675;
  `info` los lista).

### R7 segunda vuelta (ronda 4, 03:38)
- **Mando del juego parado durante el vídeo** sin tocar `combate.cpp`: el reproductor usa el bloqueo de interfaz del
  runtime (`InputSystem::AddUIInputBlocker`/`RemoveUIInputBlocker`): mientras hay vídeo el juego lee un mando neutro
  (todas sus pantallas, nativas o no) y lo que siga pulsado al quitarlo, como la A que salta, queda tapado hasta
  soltarlo. Comprobado: con `saltar=1` el menú nativo se queda en su sitio (antes la A entraba en Universo Dragón).
  Ojo R1: las pantallas propias ImGui leen `GetStateForUI` (no bloqueado): no leer el mando mientras `video::Activo()`
  y esperar a que se suelte la A al acabar.
- **Música y sonido del juego callados durante el vídeo:** `audio_gain` (la ganancia que el driver SDL del SDK aplica a
  toda la mezcla del juego, `sdl_audio_driver.cpp:235`) a 0 al empezar y, al acabar o saltar, vuelta al volumen general
  (el mismo valor que pone `settings::ApplyRuntimeSettingsToSdk`). Log: `mando y sonido del juego parados/devueltos`.
- **Voces japonesas si el juego las tiene puestas:** opción Configuración > Idioma voces de B3 = byte **G + 0x2C0C9**
  (US 0x824E61D9, G = 0x824BA110; EU por `dbz3::GA`): 0 japonés, 1 inglés. RE: lo escribe `sub_8218D280`, que elige en
  su tabla `/usr/adx_jpn.afs` (0x82020E04), `/usr/adx_usa.afs` (0x82020DF0), `/usr/lang_jpn.afs`, `/usr/lang_usa.afs`
  (cadenas junto a `mn_option.cpp`), y lo aplica al arrancar `sub_8218F8B8`. Con 0 suena el stream `_jp.sfa` (0xC0).
  Comprobado en juego: cambiar la opción a Japonés en el menú → `voces japonesas: opcion del juego 0`, `audio japones`.
- **ENDING.SFD por la misma API:** `video::ReproducirIW(video)` (nuevo): 0..21 = FD01A..FD20 (índices de las tablas de
  escenas del ELF, `EscenaIW::video`) con sus subtítulos de DATA_USA 77+vídeo, o `video::kFinalIW` (22) = ENDING.SFD
  (créditos de 4 min 51 s, sin subtítulos, música estéreo SBGMF_02). R1 puede llamar `ReproducirIW(e.video)` antes o
  después de cada misión y `ReproducirIW(video::kFinalIW)` tras la última Dragon Mission.
- **Archivos:** `src/video_sfd.{h,cpp}`, `tools/pruebas.py` (`_r7_video`: `--par video=FD01B|ENDING`, `saltar=1`,
  `voces=jp|en` cambia la opción del juego en Configuración, `jp=1` fuerza japonés). Sin cambios en DLL ni en `combate.cpp`.
- **Pruebas (compila sin errores):** `saltar=1` PASS `D:\DBZ3HD\_pruebas\20261016-031237_video_test__r7_video` (menú
  intacto tras saltar); `video=ENDING saltar=1` PASS `...\20261016-031429_video_test__r7_video` (créditos, estéreo,
  velocidad real); `voces=jp saltar=1` PASS `...\20261016-033013_video_test__r7_video` (0_idioma_elegido: Japonés;
  log `audio japones`). Con la opción de serie el byte vale 1 (`voces inglesas: opcion del juego 1`, en
  `...\20261016-032404_video_test__r7_video`). La opción se guarda: la copia `video_test` se queda en Japonés (la vuelta
  a Inglés con las teclas de la prueba no entró; cambiarla a mano en Configuración si hace falta).
- **Pendiente:** conectar en R1; subtítulos en otros idiomas (canales 3-6 del CRITAG; sus imágenes no están en
  DATA_USA); `ponytail:` la música se corta en seco (sin fundido) al empezar y al acabar.

### R9 Ports de IW con fallos: Pan, Goku GT, Vegeta GT, Baby (2026-10-16, 03:58)
- **Causa del cuelgue de Pan al acabar el combate (medida, no a ciegas):** el registro de R1 (`lr 0x82082470`,
  `ctr 0x820F4500`, `r5 = 0x17`, `r11 = 0x40D4A315`, lectura de 0x1) cae en `sub_82082468`, la función que mueve la
  cámara con un clip del fichero de cámara del luchador (`#ACC`: +0x10 cuántos clips, +0x14 tabla de 16 B: +8 frames,
  +12 claves). La pose de victoria (estado 6) pide el **clip 23** y la cámara de Pan de la copia `modos_test` tiene
  **21**: la entrada 23 cae dentro de los datos del clip 0 y trae frames `0x40D4A316` (los de r11 + 1) y claves en `0x1`.
  El roster de `modos_test` es del **14/10 04:45**, anterior a `cam_pad_clips` de `roster_build.py` (15/10), que rellena
  los clips que faltan con los del donante (Videl: 25). El montaje actual ya está bien (`ports_test`: Pan 25 clips,
  Baby 35, Vegeta GT 35, Goku GT 37, todas ≥ su donante; los ficheros de cámara de los mods son todos big-endian, así
  que el relleno no se salta). Es la misma función que el cierre de la entrada de la ronda 2 (`lr 0x820825A8`).
  Volcado: `E:\DBZ3HD_trabajo\R9\cams.py` y `acc23.py`.
- **Arreglo en marcha (para rosters viejos o mods con menos clips):** `src/ports_guard.cpp`, gancho
  `sub_82082468` (US y EU; pareja añadida a `src/eu_map_gen.h` con `python tools/eu_map.py`, solo esa línea nueva): si
  el clip pedido no existe se usa el último (sin clips, la cámara no se mueve) y avisa `dbz3 ports: clip de camara N y
  la camara (...) tiene M`. En el montaje no hacía falta cambiar nada.
- **Prueba** (compila con `tools/compilar.py`; escenario propio `E:\DBZ3HD_trabajo\R9\gt_batalla.toml`: combate desde
  el menú con `DBZ3_TEST_BATTLE=<id>,4,6`, ring out, captura de la pose de victoria y vuelta al menú, en `ports_test`):
  - **Pan PASS** `D:\DBZ3HD\_pruebas\20261016-034335_ports_test_gt_batalla` (`d_victoria.png`: Pan con «WINNER!»,
    `e_menu.png`) y con `batalla` `...\20261016-031257_ports_test_batalla`.
  - **Pan con la cámara vieja de `modos_test` (A/B de la guarda) PASS** `...\20261016-034928_ports_test_gt_batalla`:
    el aviso sale en el mismo sitio y la misma cámara del cuelgue (`clip de camara 23 ... (44CD0140) tiene 21`),
    pose de victoria y menú. Con el último clip el plano no enfoca a Pan (solo pasa con rosters viejos). La cámara buena
    se devolvió a la copia (md5 `4c979277…`; respaldo en `E:\DBZ3HD_trabajo\R9\respaldo\`).
  - **Goku GT PASS** `...\20261016-033410_ports_test_gt_batalla`; **Baby PASS** `...\20261016-032440_ports_test_gt_batalla`.
  - **Vegeta GT: FAIL por lentitud, no por cierre** (`...\20261016-031844_...` y `...\20261016-033722_...`): su combate
    va a **2-10 fps con `gpu_wait` de 76-620 ms por frame** desde la presentación, mientras los otros juegos abiertos en
    ese momento iban a 60 y Pan/Goku GT/Baby en la misma copia a 50-60. A las 4 min no había llegado al combate.
    El 15/10 17:35 (`20261015-173557_ports_test_combate_port`) ya iba a 28-42 fps. Sin causa localizada. Pista: es el único
    BSP de técnicas con 65 recursos (`E:\DBZ3HD_trabajo\R9\bsp.py`): el recurso 64 (#CWV) cae en la casilla de la
    instancia 0 durante todo el combate (la guarda de la ronda 3 solo la limpia al salir).
    `ponytail:` sin medir; siguiente paso: A/B con `tecnicas = "donante"` en su receta, o una sonda de GPU por draw.
- **Para R1 / coordinador (no edito `pasarela_misiones.cpp`):** el bloqueo está en `src/pasarela_misiones.cpp`
  `MotivoIW`, el `for` con `id == 44 || id == 45 || id == 54 || id == 56` (líneas 260-267 a las 04:00; ids de IW 44 Goku GT, 45 Baby, 54 Vegeta GT, 56 Pan). **Se pueden quitar 44
  (Goku GT), 45 (Baby) y 56 (Pan)**; **54 (Vegeta GT) debe seguir bloqueado** (lentitud). Antes de volver a probar las
  misiones, **montar otra vez el roster de `modos_test`** (es del 14/10; también `oficiales_test` y `presets_test`):
  `python "mod center hd/roster_build.py" construir --mods D:\DBZ3HD\modos_test\mods` con el juego de esa copia cerrado.
  Propuesta (no es mío): que `tools/pruebas.py prep` lance ese `construir` (no hace nada si el roster está al día),
  como hace el launcher al pulsar JUGAR; las copias de prueba no pasan por el launcher y se quedan con rosters viejos.
- **Archivos:** `src/ports_guard.cpp` (gancho nuevo), `src/eu_map_gen.h` y `docs/14_frentes/eu_mapa.txt` (regenerados:
  solo la pareja de `sub_82082468`; copia previa en `E:\DBZ3HD_trabajo\R9\`), esta sección y la fila R9.
- **Pendiente:** lentitud de Vegeta GT (arriba). Cierre ocasional de Pan en su definitiva contra el muñeco (ronda 3,
  1 de 5, `lr 0x821554B0`): no repetido aquí, sigue abierto. El fallo de Goku GT con el modo hiper y la definitiva
  (aviso del usuario del 12/10) no se ha probado: estas pruebas no equipan cápsulas.

### R10 · Carrera de IW en B3 para los minijuegos (2026-10-16 03:55)
- **RE (corrige la premisa de P4):** el estado 9 / código **0xE2** (`MgPly_Enter_State09`, 4,5/cuadro) es el corredor
  de la **IA** (Bubbles en el 1, Piccolo en el 14): `Minigame_MovePlayerByStick` (EU 0x299EE0) solo lo usa para el
  luchador 1 del minijuego 0xE, y el único `#BSK` de IW que define 0xE2 es el de **Bubbles** (DATA_CMN 912, huesos
  `BBR_*`, anim 12 de su propio AMM). El jugador va en el **"scatto"**: estados 0x14/0x15/0x16
  (`Minigame_State14_DashStartEnter`, `Minigame_State15_DashEnter` EU 0x2A0CD0, `State16_Enter`), códigos
  **0x19 → 0x1A → 0x1B** (0x1C/0x1D/0x1E en el aire), velocidad **35** (+0xB4 = 0x420C0000), abre el FOV y gasta la
  barra de energía del HUD (`State15_DashMove`). Esos códigos están en el `#BSK` común de IW (DATA_CMN 204 hijo 3) y
  van al **almacén global** de IW (204 hijo 0: 299 animaciones, **los mismos 46 huesos `GOK_*` en el mismo orden** que
  el global de B3, data_cmn 165 hijo 0, 359): 0x19 = anim 257 (12 cuadros), **0x1A = anim 259 (60 cuadros)**, 0x1B = 261
  (22). B3 no los define y ninguna de sus 359 animaciones casa con ellas (comparadas pista a pista; el andar de IW, 2,
  sí casa con el de B3 al 45/46). La 259 no es un ciclo de piernas: es la **postura de carrera inclinada** con poco
  movimiento (≤ 13°), como el scatto de IW. Los códigos 0x15-0x18 de IW (estados 1-4 "aura", 1,8 y 4,0/cuadro) usan
  animaciones que B3 ya tiene (203/204/207/20).
- **Cadena de B3 (código generado):** `sub_82114FB8(X)` → `sub_820FA378(A = X+0x650)` → `sub_820F95F0`: el código
  A+0x520 (X+2928) se busca en los 3 `#CSK` de A+0x2C0 (+0x10 n, +0x14 lista; lista[código] → bloque
  `[anim s16][almacén s16]`); el `#ACM` del almacén = `[[A+0x538] + 0x46C + 4·almacén]`, ya reubicado en memoria
  (magia **`!ACM`**, punteros absolutos; +0x14 tabla de 16 B `[flags, variante, cuadros, bloque]`). El andar de B3:
  código 3 → anim 2 (24 cuadros) y código 4 → anim 3 (32) del almacén 0. IW: la misma cadena con +0x10 menos
  (`AnimationsStatic`/`Animations`, EU 0x207CE0/0x207E90).
- **Hecho:** `src/anim_iw.{h,cpp}` (nuevos; en las dos listas de `CMakeLists.txt`). Lee la anim 259 de la copia del
  jugador (`AfsPs2` DATA_CMN 204, `HijoAmb` 0), copia sus pistas al juego en big-endian (`GuestAlloc`; como
  `ps2hd.conv_amm`) y, para el luchador, cambia la entrada de la animación que su `#CSK` cargado da a los códigos 3 y 4
  (global o propia; huesos casados por nombre sin prefijo) por la carrera de IW; al acabar repone las 4 palabras de
  cada entrada (si el almacén no se ha recargado). API `dbz3::anim::Correr(x, bool)`, y un gancho propio en
  `sub_82103C48` (entrada a toda acción) que lo hace solo: con `dbz3::iwmini::Activo()` se pone al jugador 1
  (mgr+2096) y en cuanto no lo está se devuelve antes de arrancar la animación. `DBZ3_IW_CARRERA=0` lo apaga.
- **Para P4: no hace falta añadir ninguna línea** en `iw_minijuegos.cpp` (el gancho usa `iwmini::Activo()`). Si
  se quiere el scatto fiel (con botón y gastando la barra), la línea sería `dbz3::anim::Correr(mgr + kX0, true/false);`
  al empezar/acabar el scatto (+ `#include "anim_iw.h"`), quitando antes el automático. Ojo: la velocidad extra de
  4,5/cuadro de `iw_minijuegos.cpp` es la de la IA (estado 9); la del scatto del jugador es 35 (unidades de IW).
- **Prueba:** compila (`python tools/compilar.py`, sin errores). Escenario propio
  `E:\DBZ3HD_trabajo\R10\r10_carrera.toml` (Raditz 0x7D0 sin piloto, W 9 s y 6 capturas seguidas) PASS:
  `D:\DBZ3HD\_pruebas\20261016-034829_anim_test_r10_carrera` (`b_corre1..6.png`: Goku en la postura de carrera de IW,
  piel bien, sin huesos rotos; log `codigo 3 (anim 2 del almacen 0, 24 cuadros) -> carrera de IW (60 cuadros)`, 0
  huesos sin pareja). Antes (sin el cambio): `...\20261016-033157_anim_test_r10_carrera\b_corre2.png` (andar en
  guardia). `correr anim_test iw_minijuego --par auto=0` PASS: `D:\DBZ3HD\_pruebas\20261016-035410_anim_test_iw_minijuego`.
- **Pendiente / ponytail:** solo el bucle 0x1A (sin el arranque 0x19 ni la frenada 0x1B, que son de 12 y 22 cuadros);
  los pasos laterales (códigos 5-8) siguen siendo los de guardia de B3; las líneas AP del andar (pisadas) siguen a sus
  cuadros de B3; la vuelta al andar tras el minijuego no se ha visto en una pelea posterior (la reposición solo actúa
  si la entrada sigue apuntando a la carrera); el scatto con botón y barra de IW es de P4.

### R8 segunda vuelta (2026-10-16, 04:00): el campo de IW carga y se pelea en él
- **Cierre 4 resuelto (plan a), PASS en el juego.** En Llanuras de IW (58), la colisión no está en el suelo visible: va
  en un modelo propio de 2 huesos (sub-malla 0, con 2 partes dibujables: el suelo de ±436 y otra pequeña); el modelo
  grande (sub-malla 1, 58 huesos) es solo dibujo. `campos_iw.py` deja ahora en su sitio cada sub-malla con colisión y le
  **oculta las partes** con el truco de los modders (`FFFFFF00 FFFF0000`, que `amo2awo` omite): el juego la prepara y
  la consulta de suelo la encuentra, pero no se dibuja. Las demás sub-mallas pintan el terreno de IW (mismo número e
  índices). Capturas revisadas a ojo: `D:\DBZ3HD\_pruebas\20261016-033328_stage_test_r8_campo` (PASS con ring out de
  prueba: presentación sobre las mesetas y el camino de IW; `02_combate`: la camioneta del granjero y el cráter de la
  nave de Raditz), `...-033334_stage_test2_r8_campo` (`zz_fallo.png`: Goku y Vegeta pegándose sobre la hierba de IW;
  esa prueba falló solo porque el ring out de prueba no llegó en 60 s, no se cerró) y
  `...-034558_stage_test_r8_campo2` (PASS; `03_combate`, `04_combate`: Vegeta con los pies en la hierba de IW).
  Prueba: `E:\DBZ3HD_trabajo\R8\pruebas\r8_campo2.toml` (`correr stage_test <ruta>`).
- **Cielo y nubes (plan b): sin resolver.** Como objetos propios no se ven (el juego no dibuja objetos nuevos de un
  alta; el registro va sin grupos de objetos). Ahora van como sub-mallas 2 y 3 del objeto 0, con sus texturas detrás de
  las del terreno en el mismo banco (`to_azt(extra=...)` e índices +8/+0xC de cada parte corridos, como el `attach` de
  `amo2awo`). Carga sin cerrarse, pero **el cielo sigue negro** (`...-034558_...\03_combate.png`). Causa probable, sin
  comprobar: son cúpulas vistas desde dentro (radio ~1060 y ~1670) y B3 descarta las caras traseras (la foto sin GPU no
  descarta caras, por eso sale bien), o el tipo del cielo (`vtype 0x81C00836`, bits altos) pide un modo de dibujo de IW.
  Siguiente prueba: girar el orden de los triángulos de esas partes (o marcarlas de doble cara) — 1-2 h.
- **Las 2 líneas para P4** (compilado: `dbz3::escenarios::CampoIW(campo)` en `src/escenarios.{h,cpp}` da el registro del
  alta del campo 884/897/900/903/906/909 instalado con `campos_iw.py` (entrada 5060+n), o -1):
  ```cpp
  #include "escenarios.h"   // junto a los demás includes de src/iw_minijuegos.cpp
  // línea 1109 (Lanzar), en lugar de f->escenario:
  if (!dbz3::hub::RequestBattleEx(f->jugador, f->pivote, dbz3::escenarios::CampoIW(f->campo) >= 0 ? dbz3::escenarios::CampoIW(f->campo) : f->escenario, true, -1, 0, 0, 0, 0)) return false;
  ```
  Ojo P4: con el campo de IW los objetos ya no hay que juntarlos (×0,16, `kRadio`) mientras el campo quepa en la
  colisión; hoy la colisión sigue siendo la caja de Llanuras (ver abajo), así que conviene dejar la escala hasta (c).
- **Límite de movimiento (medida):** colisión de los 13 escenarios leída del #MDD (`E:\DBZ3HD_trabajo\R8\colbounds.py`).
  Llanuras: **caja de x/z −361..361 con paredes hasta y = 300** (79 vértices). Casa del abuelo (registro 6, donde P4
  midió ~255): dos cajas, ±300 y x ±239 / z −279..225: ese ~255 cae entre las dos, coherente con que el tope es la
  pared de la colisión y no una constante (no hay ninguna en el código). **En el juego no lo he podido confirmar:**
  escenario nuevo `r8_limite` de `tools/pruebas.py` (combate del HUB, lee las posiciones X+64/+72 de los dos y
  mgr+35569 cada 50 ms y escribe `limite.txt`): andando en un combate el luchador gira alrededor del rival y no pasó de
  radio 127 en 60 s (`D:\DBZ3HD\_pruebas\20261016-035502_stage_test2_r8_limite`), y no hubo fuera del ring. Para
  medirlo bien: el modo libre de P4 en Llanuras (`iw_minijuego --par clave=0x7D2 --par lejos=1`) con este mismo
  muestreo, o un golpe fuerte contra la pared.
- **Qué hace falta para la colisión del #WCF (plan c), estimación honesta:**
  1. Barato primero (2-4 h): **escalar la caja de colisión del base** al tamaño del campo (vértices, esferas de los
     nodos y distancias de los planos ×8; las normales no cambian con una escala uniforme) y su árbol st50 igual. Da
     suelo plano de ~±2900 con paredes: se corre por todo el campo, sin subir mesetas (se atravesarían).
  2. Port real (4-6 días, como estimó P4): leer el #WCF 904/0 (589 registros de 0x90 B, losetas A1..J10 `JIMEN01-04`
     unidas por nombre a los huesos `FFPLNA02_<casilla>JIMEN` del terreno) y fabricar en formato B3 PS2 lo que hoy
     sale del base: bloque de colisión colgado de la lista de cada hueso (cabeceras f4/f3, nodos
     `[siguiente, vértices, triángulos, esfera]`, vértices 16 B, triángulos 32 B), registros rec20/rec30 del objeto y el
     árbol st50 (0x50 B por nodo). `conv_colision` y `Grafo` ya los pasan a HD; lo nuevo es entender y construir
     triángulos (32 B), rec30 (8 punteros) y st50 (planos/límites) desde cero, con `Coll_GetGroundHeightAtXZ` y los
     `Coll_*` de la decomp de IW (EU) como guía.
- **Los otros 5 campos (paso 5):** generados con el mismo comando en `E:\DBZ3HD_trabajo\R8\campos\` (884 Camino de la
  Serpiente 3,9 MB, 897 Planeta del Kaio 3,0 MB, 900 Namek 4,1 MB, 906 Llanura de Piccolo 5,2 MB, 909 Roquedal 5,8 MB),
  ~20 s cada uno. **Sin probar en el juego** (todos sobre la colisión de Llanuras; el 900 lleva además piezas 7/8 y
  14/20/26 que no se añaden).
- **Archivos:** `tools/campos_iw.py` (sub-malla de colisión con partes ocultas, cielo y nubes en el objeto 0 con banco de
  texturas común, `--quedan`, foto del selector sin el suelo oculto); `src/escenarios.h/.cpp` (`CampoIW`, respaldo en
  `E:\DBZ3HD_trabajo\R8\respaldo\src\`); `tools/pruebas.py` (escenario `r8_limite`, función `_r8_limite`).
  Compila (`tools/compilar.py`: sin errores, código 0). Copias: `stage_test` con el 903 actual, `stage_test2` con el
  903 solo terreno; versiones anteriores en `E:\DBZ3HD_trabajo\R8\respaldo\`. Ya sin el envoltorio `pr.py`.
- **Incidencia:** al añadir `_r8_limite`, `tools/pruebas.py` quedó ~2 min sin cargar (un byte nulo y dos saltos de
  línea dentro de cadenas, por el escape al pegar el texto); arreglado y comprobado (`pruebas.py --help`). Si a otro
  frente le falló una prueba hacia las 03:50 con "source code cannot contain null bytes", era eso: basta repetirla.

### R6 tercera vuelta (2026-10-16, 04:05)
- **HUD del modo con el del juego (resuelto):** los ganchos de FIX DRAW guardan la hora de la última llamada
  (`g_fix_draw_ms`) y `HudNativoVisible()` exige < 100 ms. No bastaba: FIX DRAW **sí** corre en la vuelta de cámara.
  La puerta real está en `sub_820DBCF0` (barras, caras, reloj): `([0x823CD870] & [0x823CD874])` con los bits 31 y 1
  y el puntero `[0x823CC6CC]` puesto; añadida (`ponytail:` 0x823CD874 y 0x823CC6CC por distancia a 0x823CD870, que en
  EU es la misma; si EU falla, su pareja con `tools/eu_map.py`). Prueba `r6_hud` PASS
  `D:\DBZ3HD\_pruebas\20261016-035600_hub_test_r6_hud`: a0_presentacion sin nada; a_hud y b_hud con el HUD del
  juego, el contador nativo de rivales "3/3" y debajo nuestro marcador.
- **Cifras nativas del marcador:** el atlas del marcador del juego está en `data_usi` entrada 0
  (`0x38CE22710151CFA1`, 256x256: 0-9 blancas con perfil, "/", "%", "V" y las caras del contador de rivales). Nueva
  pieza `cifras_hud` en `native_ui.cpp kPieces`; el contador de `modos.cpp` las usa teñidas de naranja (como el
  "2/2" del juego), la barra más baja y alineada por abajo. Sin la pieza, las doradas propias `retos/num_*`.
  Encontrado con el inventario `D:\DBZ3HD\work\tex_inventory.json` y las PNG de `D:\DBZ3HD\work\textures_originals_v2`
  (hojas de contacto en `E:\DBZ3HD_trabajo\R6\cifras\`).
- **Torneo:** "VS" del centro del cuadro = el del Torneo Mundial del juego (pieza `vs_torneo`, data_usi 2047,
  `0xEEC9E52D040DC990`); caras 6 px más adentro. Captura `...\20261016-040107_hub_test_r6_pantallas\b_torneo.png` (caras
  dentro, VS nativo; pequeño dentro del recuadro de 90 px). Las líneas del cuadro siguen siendo propias: en 2047 hay
  rótulos ("1st Match", "Final", "Champion", "VS", números 1-8) pero no hay trazos de cuadro; el juego los dibuja.
- **Pantallas:** entre `...\20261016-034421_hub_test_r6_pantallas\a_entre.png` y derrota
  `...\20261016-034523_hub_test_r6_pantallas\a_derrota.png`: bien (marco con franja en la cabecera, bocadillo, ayuda
  A/B en derrota; el recuadro HUB ya no sale encima). Entre no lleva ayuda de botones (sigue sola, encadenada).
  Abyss `...\20261016-040235_hub_test_r6_pantallas`: b_abyss (presentación del piso 1: sin HUD del modo, bien) y
  c_cartas (piso 2: cabecera, 3 cartas con la franja dentro del marco y el texto de la carta en el bocadillo): bien.
- **Archivos:** `src/combate.cpp` (hora de FIX DRAW y puerta de `HudNativoVisible`), `src/ingame/native_ui.cpp`
  (`cifras_hud`, `vs_torneo`), `src/modos.cpp` (cifras del contador), `src/rpg.cpp` (VS y caras del cuadro). Compila
  (`tools/compilar.py`, 03:56).

### R10 segunda vuelta (2026-10-16 04:20): el scatto de IW con sus 3 fases; andar = el de B3
- **Comportamiento fiel:** la palanca anda (el andar de IW, anim 2, es el mismo que el de B3) y el botón hace el
  scatto. **Se quitó el gancho automático de `sub_82103C48` y la API `Correr`** (nada cambia ya el andar de B3).
- **Cómo se pone (RE de la vuelta 1 + avance de la animación, 0x820FA558):** las 6 animaciones de IW (códigos
  0x19/0x1A/0x1B = anims 257/259/261 de 12/60/22 cuadros; aire 0x1C/0x1D/0x1E = 263/265/267) se **añaden detrás**
  de las 359 del almacén global de B3 (tabla nueva = la vieja + 6 en `GuestAlloc`; `+0x10` y `+0x14` del `!ACM`
  apuntan a ella; **ninguna entrada existente cambia**), y un `#CSK` propio de 6 códigos (bloques de 48 B copiados del
  `#BSK` común de IW 204/3, sin líneas AP, animación = 359..364 del almacén 0) va en un **hueco libre** de los 3
  `#CSK` del luchador (A+0x2C0; el 0 está vacío en Goku). Se arranca como el juego: código en X+2928 y
  `sub_82114FB8(X)` (patrón de `cinematica.cpp`). Cada cuadro A+0x608 avanza y al pasar de A+0x60C vuelve a 0 y cuenta
  vueltas en A+0x618 (bucle de serie): con eso el arranque pasa solo al bucle y la frenada a "apagado" (guardia,
  acción 0, código 0, como `cinematica::Arrancar`; se vacía el hueco del `#CSK`).
- **API (`src/anim_iw.h`):** `dbz3::anim::Scatto(x, Fase, aire = false)` con `kInicio` / `kBucle` / `kFin` /
  `kApagado` (cualquier hilo: solo deja el pedido; repetir la misma fase no hace nada) y `FaseScatto()` (la que se
  ve). El pedido se aplica en el hilo del juego desde un gancho propio en `sub_8208FAB0` (lo llama la lectura del
  mando, `sub_8208FD58`, cada cuadro; nadie más lo engancha). **Seguro:** fuera de un minijuego
  (`!iwmini::Activo()`) o de la pelea (mgr+35128 != 4) se apaga solo, aunque falte el `kApagado`.
- **Líneas para P4 en `src/iw_minijuegos.cpp`** (no lo he tocado):
  - arriba: `#include "anim_iw.h"`
  - al pulsar el botón del scatto (con barra > 0): `dbz3::anim::Scatto(x0, dbz3::anim::kInicio, en_el_aire);`
  - al soltarlo o quedarse sin barra: `dbz3::anim::Scatto(x0, dbz3::anim::kFin, en_el_aire);`
  - al acabar el minijuego (en el `VigilarCombate` de `EmpezarCombate`, junto a `OcultarHud(false)`):
    `dbz3::anim::Scatto(mgr + kX0, dbz3::anim::kApagado);` (el seguro lo hace igual).
  - velocidad: mientras `dbz3::anim::FaseScatto() != dbz3::anim::kApagado`, la del scatto de IW (+0xB4 = **35**,
    `Minigame_State15_DashEnter`, que `Minigame_MovePlayerByStick` multiplica por la palanca y +0xB54, 0,4 de serie)
    × la escala del curso, en vez de los 4,5 de ahora (eso era el estado 9 de la IA); sin scatto, el andar de B3 sin
    extra. **Barra:** `State15_DashMove` (EU 0x2A0A30) gasta 0,5 por cuadro de X+0x185C (0,25 con la bandera de escena
    +0x16BB); FOV de la cámara +0x120: +2,5 por cuadro hasta 50 y luego −0,38 hasta 40. ponytail: la recarga de la
    barra y el botón exacto no los he leído.
- **Prueba:** compila (`tools/compilar.py`, sin errores). `E:\DBZ3HD_trabajo\R10\r10_scatto.toml` (con
  `DBZ3_TEST_SCATTO=1`: en la prueba el scatto sale al andar y frena al soltar, para ver las fases sin botón) PASS:
  `D:\DBZ3HD\_pruebas\20261016-040424_anim_test_r10_scatto` (`c_bucle1..3`: postura del scatto; `e_guardia`: vuelta a
  la guardia; `f_otra_vez`: segundo scatto; mosaico `E:\DBZ3HD_trabajo\R10\scatto_mosaico.png`). Log: fase 1 (0x19)
  → 168 ms → fase 2 (0x1A) → al soltar fase 3 (0x1B) → 350 ms (22 cuadros) → código 0, acción 0. La frenada dura
  demasiado poco para la captura (se ve en el log).
- **Corrección "corre de lado" (aviso de P4, 04:25):** medido en los datos: la raíz es `GOK_WAIST` (`XGOK_BODY` sin
  pistas). Con q = qz·qy·qx (`awo_tools/altura.py`), en la guardia de B3 la columna (+Y de la cadera) está vertical
  y su eje Z mira a −47° / X a +43° (la postura de 3/4 hacia +Z, el frente del luchador); en el bucle del scatto de IW
  la columna se inclina 41° hacia **+X** (rumbo 86°): IW lo corre hacia el +X del modelo, 90° del +Z de B3. Arreglo en
  la conversión (`GirarRaiz` en `anim_iw.cpp`): la pista angular y la de posición de `GOK_WAIST` de las 6 animaciones
  se giran −90° alrededor de la vertical (N = Ry(−90°)·R, reescrito en Euler ZYX), con un peso según la inclinación
  de la columna (0 en la guardia del principio del arranque, 1 en el bucle) para que no haya salto al empezar ni al
  acabar. Comprobado sin juego: la columna del bucle pasa de rumbo 86° a −4°; la guardia no cambia. En el juego
  (mismo escenario, PASS): `D:\DBZ3HD\_pruebas\20261016-042858_anim_test_r10_scatto` (`c_bucle1..3`: Goku corre de
  espaldas a la cámara, hacia donde va, inclinado; mosaico `E:\DBZ3HD_trabajo\R10\scatto_girado_mosaico.png`; antes:
  `scatto_mosaico.png`).
- **Vuelta al andar normal tras un minijuego:** `E:\DBZ3HD_trabajo\R10\r10_despues.toml` (minijuego con la meta
  abierta y el scatto puesto hasta CLEAR, menú, HUB > Supervivencia) PASS:
  `D:\DBZ3HD\_pruebas\20261016-041941_anim_test_r10_despues`. El combate normal (Goku contra Saibaman) arranca bien y
  el log no tiene ninguna línea de `dbz3 anim` después del minijuego (el seguro lo apagó al salir). Pero **las
  capturas de andar llegaron durante la presentación** (`e_anda1/2`: la entrada del Saibaman; `f_atras`: READY), así
  que el andar del combate normal **no está visto a ojo**. Por diseño ya no se cambia ninguna animación de B3 (solo
  se añaden detrás y el hueco del `#CSK` se vacía); falta repetir la prueba esperando a la pelea.
- **Nota de P4 para más adelante:** la gravedad fuerte de Bubbles (minijuego 1) en el original solo cambia las
  animaciones, no las velocidades.
- **Pendiente / ponytail:** al reentrar el juego en andar (cambio de dirección) se repone el código de la fase
  (alguna vuelta a empezar el arranque o la frenada; se ve en el log como varias "fase N" seguidas); la frenada (22
  cuadros) no se ha visto en captura (sí en el log); variantes del aire sin probar; recarga de la barra y botón de IW
  sin leer.

### P4 Minijuegos IW, cuarta vuelta (ronda 4, 04:25)
- **Scatto del jugador** (decomp EU 0x2A0FC0 / 0x2A0CD0 / 0x2A0940, botón en 0x299520): círculo (B del mando), arranque
  12 cuadros quieto, carrera 60 a 35 (en proporción: el andar de IW 4,0 = el de B3), frenada 22; energía −0,5/cuadro,
  +0,02/cuadro, agotado hasta una bebida; las bebidas suman la energía de su #ITM; FOV 35→50→40→35; barra del HUD =
  esa energía; animaciones con `anim::Scatto` de R10. Quitado el extra de 4,5 del jugador. Bubbles y Piccolo a 4,5
  (estado 9 / 0xE2), algo más que andar.
- **Campo de IW de R8** en `EmpezarCombate` (`escenarios::CampoIW`); 903 instalado en `combate_test` y probado.
- "Gravedad fuerte" de Bubbles = solo animaciones (0xFB/0xFE/0xFF/0x101/0x103/0x105), velocidades iguales: para R10.
- **Pendiente para otros:** R10, el scatto sale de lado (90°); R8, cielo negro en el campo 903.
- **Pruebas:** compila; PASS `D:\DBZ3HD\_pruebas\20261016-040841_combate_test_iw_minijuego` (campo 903 + 3 scatto hasta
  agotar), `...-041732_...` (Bubbles: bebidas y CLEAR), `...-034532_...` (Piccolo). Detalle en PORT_EXACTO.md, P4.

### R9 segunda vuelta (2026-10-16, 04:30)
- **Vegeta GT «a 2-10 fps»: no es suyo ni de los 65 recursos (medido).** Gancho de medida temporal en `sub_82126490`
  (tras montar la ranura de técnicas): el recurso 64 es `47583B20` = `#CWV` y queda en la casilla de la instancia 0
  durante todo el combate (confirmado). A/B con la variable `DBZ3_R9_AB=1`, que lo ponía a 0 al montar:
  con él a 0 (`D:\DBZ3HD\_pruebas\20261016-040128_ports_test_vgt_fps`) **y sin tocar nada**
  (`...\20261016-040429_ports_test_vgt_fps`, justo después) el combate va a **50-60 fps** (`gpu_wait` 0-15 ms, lo mismo
  que Pan, Goku GT y Baby), con pose de victoria y vuelta al menú. Las tres pasadas lentas coinciden con el GPU saturado
  por otras copias: en `...\20261016-035513_ports_test_vgt_fps` el **menú** ya iba a 15-25 fps antes del combate, y
  otra copia (`stage_test2`) iba a 17-28 fps en el rato de `...-033722_...`. Con el mismo PC cargado, la prueba de
  cápsulas de Goku GT de las 04:10 fue igual de lenta (2-5 fps, menú 11-17). Hipótesis descartada; el gancho de
  medida se quitó (copia en `E:\DBZ3HD_trabajo\R9\ports_guard_con_medida.cpp`). La instancia 0 con el `#CWV` no molesta
  en combate y la guarda de la ronda 3 la limpia al salir: no hace falta otro arreglo. Para medir fps hace falta que el
  PC no tenga otras copias pesadas a la vez (o comparar con el fps del menú de la misma pasada).
- **`tools/pruebas.py prep` monta el roster de la copia** (dentro de `preparar`, al final): llama a
  `mod center hd/roster_build.py construir --mods <copia>\mods` como el JUGAR del launcher. Si está al día no hace nada
  (1 s: `roster: _roster al dia (17 personajes)`, comprobado con `prep ports_test`). Si falla, avisa y la copia se queda
  con el roster anterior (el montaje se publica entero o nada). La próxima prueba de `modos_test`, `oficiales_test` y
  `presets_test` (rosters del 14/10) los volverá a montar sola: tarda unos minutos la primera vez.
- **Pan, definitiva con cápsulas** (`E:\DBZ3HD_trabajo\ports\combate_ports.py ports_test 25 15 --com1 --killeno
  --cvar dbz3_combate_definitiva_directa=true`): `...\20261016-041521_ports_test_combate_port`. 6 técnicas, la
  definitiva Reliable Friend (cápsula 612) sale una vez, Videl cae y el combate acaba sin cierres (43-60 fps).
  El cierre de 1 de cada 5 contra el muñeco no se ha repetido; no hay más pasadas por falta de tiempo con el PC libre.
- **Goku GT con cápsulas** (7 de serie: 602 603 604 608 609 605 606): la pasada de las 04:10
  (`...\20261016-040827_ports_test_combate_port`) no sirve, porque con el GPU saturado no salió de las entradas
  (estado 2). **Repetida a las 04:26: el juego se cierra** (`...\20261016-042304_ports_test_combate_port`; Goku GT es
  el jugador 1 llevado por el COM, contra Krilin COM). Transforma (forma 0 → 3) y saca modo hiper; la definitiva
  de la cápsula 609 sale tres veces (código 259, 359 y 359). Al empezar la tercera: `lr 0x8209F668`,
  `ctr 0x8211E520`, lectura de `0x95777090`.
  - **Causa medida:** `sub_8209F568` es el intérprete de órdenes de la animación (manejador = tabla [orden >> 12]
    [(orden >> 8) & 0xF]). `sub_8211E520` es la orden que hace `luchador = mgr[8797 + p[1]]` (con los parámetros
    `p = *(orden + 44)`), y luego `sub_82115478(luchador, p[0])` y `sub_82111458`. Aquí `r10 = 0x133A8EB0` =
    (8797 + p[1]) × 4, así que **p[1] = 0x4CE81EF**, cuando solo vale 0 o 1 (el lado). Los parámetros estaban en
    `r9 = 0x44A326F8`.
  - Es una orden de la definitiva del port con un parámetro que no es un lado: una orden de IW que significa otra
    cosa en B3, o un trozo de la animación injertada mal reubicado. La misma definitiva 359 salió bien 13 s antes,
    así que depende de algo más (forma 3 o el momento).
  - **Sin arreglar (sin tiempo para probarlo).** Siguiente paso: un gancho de medida en `sub_8211E520` que registre
    la orden y `p[0..3]` cuando `p[1] > 1` y busque esos bytes en el anm de la forma 3 de Goku GT
    (`imp_iw_goku_gt`), para arreglarlo en `capsulas.py`/`roster_build.py`. Como respaldo, una guarda que salte la
    orden si `p[1] > 1`. Esto es probablemente el «hiper/definitiva con fallos» del aviso del usuario del 12/10.
- **Para R1 / coordinador:** con esto **también se puede quitar el 54 (Vegeta GT)** (y 45 Baby, 56 Pan) del bloqueo de `MotivoIW` en
  `src/pasarela_misiones.cpp` (el `for` con `id == 44 || id == 45 || id == 54 || id == 56`). Los cuatro pasan un
  combate completo y la vuelta al menú. **El 44 (Goku GT) mejor dejarlo bloqueado** hasta arreglar el cierre de su definitiva (arriba): sin cápsulas pasa, pero en las misiones de IW el COM las usa. La primera prueba de misiones de `modos_test` tardará más porque `prep` monta
  su roster.
- **Archivos:** `tools/pruebas.py` (solo el bloque nuevo al final de `preparar`), `src/ports_guard.cpp` (gancho de
  medida puesto y quitado; queda solo la guarda del clip de cámara de la primera vuelta), esta sección. Compila
  (`tools/compilar.py`, 04:20).

### R8 tercera vuelta (2026-10-16, 04:50): cielo de IW, colisión estirada y los 6 campos cargan
- **Cielo y nubes de IW: resuelto, comprobado a ojo.** No eran las caras traseras (con el cielo de doble cara seguía
  negro: `D:\DBZ3HD\_pruebas\20261016-040232_stage_test_r8_cielo`) ni el tipo de vértice (`0x81C00836` también lo usa
  el cielo nativo de B3 en el escenario 52). Causa (RE de `sub_820BF558`, tabla de 32 B por sub-malla del escenario
  cargado): **el juego solo pinta tantas sub-mallas como tiene el escenario base** (2 en Llanuras); el cielo y las
  nubes iban en la 3.ª y 4.ª. Ahora se **pegan al modelo del terreno** como partes rígidas de su hueso raíz (bind
  identidad), con el `attach` de `amo2awo` (el de las pelucas de B2: nuevo parámetro `head` en `attach_rigid`, sin
  cambiar lo de siempre) y sus texturas detrás en el mismo banco. Captura:
  `D:\DBZ3HD\_pruebas\20261016-042330_stage_test_r8_campo2\03_combate.png` (cielo azul con nubes de IW, montañas
  nevadas, la camioneta y el cráter de la nave de Raditz).
- **Colisión, paso barato: hecho y medido.** La caja de Llanuras (solo paredes verticales y suelos planos: normales
  con ny 0 o 1; triángulo = `[normal xyz, 1][?, i1, i2, i3]`) se estira en x/z: vértices, esferas de los nodos (centro,
  radio, radio²) y árbol st50 (centro, radio). Escala por campo en `CAMPOS` (perilla de calibración): hasta el borde
  del terreno (903, 900, 906, 909: ×4,2 → ±1516; 897: ×2,5 → ±900; 884 Camino de la Serpiente: ×10,4 → ±3754).
  Medidas con `r8_limite` en el modo libre de P4 (`--par clave=...`, tecla W):
  | Escenario | Hasta dónde corre | Carpeta (`D:\DBZ3HD\_pruebas\`) |
  |---|---|---|
  | Llanuras de B3 (884 sin instalar), antes | **280** y se para (el pivote, 40 por delante, toca la pared de 361) | `20261016-040316_stage_test2_r8_limite` |
  | Campo 884 con la caja ×8 | **681** en 40 s y sigue corriendo | `20261016-041116_stage_test2_r8_limite` (`a_lejos`: el Camino de la Serpiente) |
  | Campo 903 final (×4,2) | **1224** en diagonal y se para en la pared (borde irregular de la caja) | `20261016-043325_stage_test_r8_limite` |
  Confirmado: **el tope es la pared de la colisión del escenario**, no un radio en el código. Límite del paso barato:
  suelo plano a y = 0. Junto al borde del 903 el terreno sube (mesetas): el luchador pasa por debajo (`a_lejos.png`
  de la última carpeta). Lo arregla la colisión real (#WCF, 4-6 días) o bajar la escala del campo en `CAMPOS`.
- **Escala de objetos para P4: 1,0.** El campo está en las coordenadas de IW (raíz con bind identidad) y en unidades
  de IW (= B3). Con el campo instalado (`CampoIW(f->campo) >= 0`), los objetos del #ITM van **en su posición de IW tal
  cual: sin centrar y sin ×0,0675 ni `kRadio`**, y la salida en el punto 0 del #ITM. La recogida en el plano (x, z) ya
  es la de P4. Lo que pase de ±1476 en los ejes (diagonal ~1180 en el 903) no se alcanza con la caja actual.
- **Instalación en el equipo del jugador (paso 3): no existe ese camino tampoco para B1/B2.** `grep` en `src/launcher`
  y `mod center hd`: nadie llama a `tools/escenarios_convertir.py` (solo lo nombra un comentario de
  `src/escenarios.cpp`). Los escenarios de B1/B2 se crean hoy a mano con la herramienta de línea de comandos en la
  carpeta del proyecto. Además, `tools/` no va en la release ni en el Kit (`tools/make_modpacks.py` empaqueta
  `mod center hd/` y los módulos de `awo_tools` que alcanzan sus `KIT_ENTRIES`), y `escenarios_convertir.py` busca
  `xbcompress` en `mod center/...` (ruta de desarrollo) y los datos en `<proyecto>/us`. El único flujo automático del
  launcher es `ModPipeline::BuildRoster` (`mod center hd/roster_build.py` al pulsar Jugar, si está el Kit). Lo que
  falta, con estimación honesta (2-3 h + prueba en una copia limpia): (1) llevar `escenarios_convertir.py` y
  `campos_iw.py` a `mod center hd/` (o a `KIT_ENTRIES`) con `xbcompress` de `mod center hd/tools` y `--us` / ps2_games
  pasados por el launcher, como a `roster_build`; (2) un paso en `ModPipeline` tras `BuildRoster`: si hay IW en
  ps2_games y falta `mods/Campo IW <n>` (o cambió el sello tamaño + fecha de la ISO), correr `campos_iw.py mod <los 6>
  --mods <juego>\mods`; (3) lo mismo, opcional, para B1/B2. Nada del juego se reparte: se genera en su equipo.
  **No lo he hecho esta vuelta**: toca `mod_pipeline.cpp` y `make_modpacks.py` (launcher y release) y necesita el visto
  bueno del coordinador.
- **Los otros 5 campos en el juego (paso 4): cargan todos, un combate cada uno, revisados a ojo.** 884 (Camino de la
  Serpiente, en el modo libre: `20261016-041116_stage_test2_r8_limite\a_lejos.png`), y en `stage_test3` (altas 17-20):
  897 Planeta del Kaio (`20261016-042409_stage_test3_r8_campo_17`: cielo rosa de IW, el planeta pequeño abajo),
  900 Namek que explota (`...-042827_..._18`: lava y cielo verde oscuro), 906 Llanura de Piccolo
  (`...-042942_..._19`: cielo con nubes, carretera y la casa cápsula), 909 Roquedal (`...-043058_..._20`: desierto
  rocoso con el arco y cielo de atardecer). Mosaico: `E:\DBZ3HD_trabajo\R8\mosaico_4campos.png`. Pruebas:
  `E:\DBZ3HD_trabajo\R8\pruebas\r8_campo_{17..20}.toml`. En el 897 la cámara del combate queda por encima del
  planeta (es una esfera pequeña; el suelo plano está a y = 0): habrá que bajar su escala o ajustar la altura.
- **Generados con la versión final** (cielo pegado, caja por campo, determinista: mismo bin dos veces) en
  `E:\DBZ3HD_trabajo\R8\campos\` (884 3,9 MB, 897 3,0, 900 4,1, 903 5,9, 906 5,2, 909 5,8; ~20 s cada uno).
  Copias: `stage_test` con el 903 final, `stage_test2` con el 884 (caja ×8), `stage_test3` con 897/900/906/909;
  versiones anteriores en `E:\DBZ3HD_trabajo\R8\respaldo\`.
- **Archivos:** `tools/campos_iw.py` (cielo y nubes pegados al terreno, `escalar_colision`/`escalar_st50`, escala por
  campo, título del 900, sin la opción de doble cara que no sirvió); `awo_tools/amo2awo.py` (`attach_rigid(...,
  head=None)`, respaldo en `E:\DBZ3HD_trabajo\R8\respaldo\src\amo2awo.py`); `tools/pruebas.py` (`r8_limite`: modo
  libre con `--par clave`, cámara en `limite.txt`). Compila (sin cambios de C++ desde la segunda vuelta).
- **Pendiente:** instalación automática en el equipo del jugador (arriba, 2-3 h, con visto bueno); colisión real del
  #WCF para las mesetas y la altura (4-6 días); escala del 897; niebla y color de cielo del #MCF (tema genérico 16).

### P4 Minijuegos IW, quinta vuelta (ronda 4, 05:00)
- **Piccolo (14) = el Piccolo de B3** (rival visible, mando del muñeco + una línea en combate.cpp para inyectarle el
  andar): le persigue la IA de IW; el jugador mira hacia su marcha gracias a la RE de **X+6062 / X+6074** (u16, hacia
  dónde mira el luchador). La palanca derecha gira la cámara para verle (y ya no lanza el Kamehameha de B3).
- **Bubbles corre con su animación de IW** (0xE2 → #AMM 912/1 anim 12, R = Rz·Ry·Rx), pieles en CPU; el cuerpo, con
  pesos aproximados (hueso más cercano) hasta leer los del #AMO.
- **API final de R10** puesta (kApagado al acabar). **Campos de R8** en coordenadas y velocidades de IW (6 campos en
  `combate_test`). Pendiente de R8: paredes del campo para el luchador, mitad baja negra en el 903 y el suelo del
  Camino de la Serpiente.
- Compila. Pruebas `D:\DBZ3HD\_pruebas\20261016-044419_*` (Piccolo, PASS), `-044656_*` (903 + scatto, PASS),
  `-044851_*` (Camino de la Serpiente: FAIL de la prueba, esperaba 8 aros y hay 6). Detalle en PORT_EXACTO.md, P4.

### R8 cuarta vuelta (2026-10-16, 05:20): los escenarios se instalan solos al pulsar JUGAR, con la niebla de IW
- **Instalación automática en el equipo del jugador (aprobada por el coordinador): hecha y probada en una copia
  limpia.**
  - Los conversores pasan al Kit: `mod center hd/campos_iw.py` y `mod center hd/escenarios_convertir.py` (movidos;
    `tools/campos_iw.py` y `tools/escenarios_convertir.py` quedan como lanzadores, así los comandos de siempre siguen
    valiendo). `campos_iw` entra en `KIT_ENTRIES` de `tools/make_modpacks.py`; su rastreo de imports ya mete
    `escenarios_convertir`, `swap_b3`, `importar`, `iso`, `model_render`, `texture_b3` y de `awo_tools` `amo2awo`,
    `ps2hd`, `amt_ps2`, `awg_vertex_buffer` (comprobado con `kit_modules()`). Sin rutas de desarrollo: `xbcompress`
    sale de `swap_b3.XBCOMPRESS` (`mod center hd/tools`, que ya viaja en el Kit y en la release; en el proyecto, el
    del XDK), y los datos (`--us`) y `ps2_games` (`--ps2`) los pasa el launcher, como a `roster_build.py`. Solo
    viajan los scripts: los escenarios se generan en el PC del jugador desde sus ISOs.
  - `python "mod center hd/campos_iw.py" auto --mods <juego>\mods --us <datos> --ps2 <ps2_games>`: crea los 6 campos
    de IW y los 3 escenarios de B1/B2 probados en el juego (Templo de Kami y Montañas nevadas de B2, Páramo al
    atardecer de B1; el resto de grupos tiene cierres conocidos o está sin probar, `escenarios.md`) que falten o cuya
    ISO cambió (sello `origen.txt`: nombre, tamaño, fecha y `VERSION` del conversor). Hasta 6 procesos a la vez:
    los 9 en ~2 min la primera vez, ~1 s después. Un escenario que falla no para los demás y sale 0.
  - Launcher: `ModPipeline::BuildStages()` (`src/launcher/mod_pipeline.cpp`): `ps2_games` como el juego
    (`DBZ3_PS2_GAMES` o junto a los ajustes), datos con `UsDir()`; sin datos o sin `ps2_games`, no hace nada y lo
    dice en el registro. En JUGAR (`launcher_state.cpp`): si está el Kit y no es modo ISO, tras montar los personajes
    (si los hay y salieron bien) corre los escenarios y **se juega salga como salga** (sin Python, sin ISO o con
    fallo: solo una línea en el registro, `dbz3: escenarios de PS2: ...`).
  - Gancho de prueba con el mismo paso: `DBZ3_TEST_ESCENARIOS_AUTO=1` con `dbz3_skip_launcher` (`src/main.cpp`,
    antes de arrancar el juego). `tools/pruebas.py`: `--par arranque=S` (espera más al menú).
  - **Prueba en copia limpia** (`prep stage_clean`, sin escenarios; Kit copiado con `kit_modules()`):
    `correr stage_clean r8_limite --par clave=0x7D0 --par arranque=420 --env DBZ3_TEST_ESCENARIOS_AUTO=1` PASS:
    `D:\DBZ3HD\_pruebas\20261016-044955_stage_clean_r8_limite`. El registro dice `creado Campo IW 884 ... 909`,
    `Templo de Kami`, `Montanas nevadas`, `Paramo al atardecer`, las 9 altas 17-25, y
    `iwmini: escenario 20 (campo de IW 903: instalado)`: **el minijuego corre en el campo 903** (`a_lejos.png`).
    Con versión nueva del conversor se rehacen solos: `...-045406_...` y `...-050432_...`.
    Sin ISOs (`DBZ3_PS2_GAMES` a una carpeta vacía): `sin Infinite World ... / sin Budokai 2 ... / sin Budokai ...` y
    el juego arranca y pelea igual: `D:\DBZ3HD\_pruebas\20261016-050431_stage_test2_r8_limite` PASS.
    No probado con el launcher de verdad (las pruebas lo saltan) ni sin Python (camino ya existente de `RunAsync`:
    avisa y el estado sale fallido; JUGAR sigue).
- **Niebla y color del #MCF en lugar del tema 16: hecho.** `#MCF` +0x1C color `0x00BBGGRR`, +0x28/+0x2C inicio y fin.
  `campos_iw.py` escribe `niebla = "RRGGBBAA,inicio,fin"` en el `stage.toml` de cada campo (903 `7DE1FFFF,800,1800`
  azul claro, 897 `FFE69BFF,400,800` amarillo, 900 `B43C00FF,750,1500` rojo, 909 `F0C35AFF,1000,1800` arena...).
  `src/escenarios.cpp` la pone, mientras el alta está cargada, en la entrada de niebla del registro anfitrión
  (tabla `0x823232A0`, 20 B: color, 2 floats, inicio, fin; grupos 0 y 1 del registro) y la devuelve en `Restaurar`.
  Medido en el juego (`r8_limite` lee la niebla puesta): campo 903 `activa 80, color 7DE1FFFF, 800..1800`
  (`D:\DBZ3HD\_pruebas\20261016-050100_stage_clean_r8_limite`, montañas con bruma azul en `a_lejos.png`); Llanuras
  nativo sin niebla (`...-050100_stage_test_r8_limite`). La luz sigue siendo la genérica (tema 16).
- **Campo 897 (Planeta del Kaio):** en combate se ve bien (Vegeta sobre la hierba con el árbol y la mesa:
  `D:\DBZ3HD\_pruebas\20261016-044229_stage_test3_r8_limite\a_lejos.png`; los pies en el suelo, posiciones a y = 0).
  Lo que miraba al cielo eran **las cámaras de la presentación**, que son las de Llanuras (pensadas para su caja de
  ±361 y sus montañas): no es un problema de escala. Arreglarlo pide reescalar las pistas de cámara del #AMC del base
  (formato de sus claves sin descifrar) o cámaras propias: sin hacer.
- **Escala de la caja en el 903:** probado ×3,3 y ×2,5: con suelo plano, el luchador pasa por debajo de las mesetas
  a cualquier distancia donde las haya (`...-045406_...` a 972, `...-050432_...` a 734). Se deja ×4,2 (llega a todo
  el recorrido de P4); la solución es la colisión real del #WCF (4-6 días).
- **Archivos:** `mod center hd/campos_iw.py` (`auto`, niebla, VERSION 4), `mod center hd/escenarios_convertir.py`
  (US/PS2 por parámetro, `xbcompress` de `swap_b3`), `tools/campos_iw.py` y `tools/escenarios_convertir.py`
  (lanzadores), `tools/make_modpacks.py`, `src/launcher/mod_pipeline.{h,cpp}`, `src/launcher/launcher_state.{h,cpp}`,
  `src/main.cpp`, `src/escenarios.cpp` (niebla), `tools/pruebas.py` (`arranque`, niebla en `r8_limite`). Compila
  (`tools/compilar.py`: sin errores, código 0). Respaldos de antes: `E:\DBZ3HD_trabajo\R8\respaldo\v4pass\`.
- **Pendiente:** cámaras de la presentación propias de cada campo; colisión real (#WCF); probar el paso con el
  launcher de verdad y con la release empaquetada (`make_modpacks.py --solo kit` + `make_release.ps1`, sin publicar).

### P4 Minijuegos IW, sexta vuelta (ronda 4, 05:15)
- **Pesos reales del #AMO de IW leídos** (lista por hueso en el registro del eje +0x34; destino = dirección del vértice
  desde el #AMG): Bubbles se anima con su piel de verdad (comprobado fuera del juego); en el juego aún no sale en una
  captura (posible altura del #ITM 888; queda con `suelo_iw::Altura`).
- **Camino de la Serpiente:** la prueba espera los aros que da el #ITM (6 en dificultad 2) y pasa; a ojo, el jugador cae
  bajo el camino (suelo del campo: R11).
- **Cruceta** quitada al luchador de B3 (lanzaba un Kamehameha). **Petición a R10:** `anim::Scatto` para un segundo
  luchador (Piccolo con 0xE2). **R11:** su cabecera ya está, falta el .cpp; la línea de la cámara está anotada.
- Compila. Pruebas `D:\DBZ3HD\_pruebas\20261016-045804_*` (Serpiente PASS), `-050703_*` y `-051203_*` (Bubbles PASS sin
  verle). Detalle en PORT_EXACTO.md, P4.

### R9 tercera vuelta (2026-10-16, 05:25): cierre de la definitiva de Goku GT
- **Causa (medida y localizada).** La definitiva corre en el guion `#SPX` de la cámara del port. Goku GT trae el de
  Infinite World tal cual. El intérprete de B3 (`sub_8209F568`) busca la orden N en `filas[N >> 4][N & 15]` (tabla
  US `0x82332670`) sin mirar cuántas tiene la fila. **IW tiene filas más largas** (tabla de su ELF USA `0x457AC0`):
  la fila 8 de B3 tiene 4 órdenes (0x80-0x83) y la de IW 12, la 3 tiene 5 y 12, la 9 tiene 9 y 12.
  - Las órdenes de IW **0x86 `ScriptCmd_GetEquippedCapsuleLevel`** (nivel 1-3 según las cápsulas 0x49-0x4D del lado
    p[0]) y **0x88 `ScriptCmd_GetCurrentCapsuleType`** (byte 5 del registro de la cápsula actual, 1 si no hay) caen
    en B3 en las 0x92/0x94, `sub_8211E520`. Esta hace `luchador = mgr[8797 + p[1]]` y le cambia el estado. Con un
    solo argumento, p[1] es basura (0x4CE81EF) y el juego se cerraba.
  - Nombres casados por el cuerpo en la descompilación EU: USA `0x23AD00` = EU `0x23AE90`; USA `0x23AF00` = EU
    `0x23B090`.
  - Las mismas órdenes de IW sin equivalente en B3 están en **todos** los ports de IW: 0x35-0x37, 0x86, 0x88, 0x99,
    0x9A, y en Vegeta saga Saiyan además 0x3B y 0x87. Afecta a Goku GT, Pan, Baby, Vegeta GT, Janemba, Vegeta saga
    Saiyan y Yamcha amarillo. Ningún personaje del juego las usa. Auditoría: `E:\DBZ3HD_trabajo\R9\spx3.py`; mapa de
    tablas: `iwtab.py`.
- **Arreglo en marcha** (`src/ports_guard.cpp`; parejas EU añadidas con `tools/eu_map.py`). La orden de IW se
  reconoce por la clase y fila con que entra (r10/4, r9/4 del intérprete) en los manejadores de B3 donde cae:
  `sub_8211E520` (0x86/0x88, y cualquier llamada con p[1] que no es un lado), `sub_8211E148`/`E178`/`E1B0`
  (0x35-0x37), `sub_8211E6A0`/`E6F8` (0x99/0x9A), `sub_8211E578` (0x87) y `sub_8211E2B8` (0x3B). Se saltan sin tocar
  nada y dejan un aviso `dbz3 ports: orden de guion N de Infinite World`.
  - En el montaje no se puede traducir: B3 no tiene esas órdenes ni esas cápsulas, y no hay orden «nada» conocida que
    las sustituya. Por eso va en marcha.
  - Probé a escribir 1 en cmd+0, que es donde IW y B3 dejan el resultado, y no cambió nada. Lo quité.
- **Pruebas** (`E:\DBZ3HD_trabajo\ports\combate_ports.py ports_test 24 10 --com1 --killeno`; 6 pasadas, en
  `D:\DBZ3HD\_pruebas\`):
  - **El cierre de `lr 0x8209F668` no vuelve.** La guarda salta 0x88 justo donde antes se cerraba.
  - **Pero cuando salta 0x88, la definitiva se queda colgada (4 de 4):** la cámara queda fija en el escenario vacío
    hasta el final de la prueba, en estado 4 y sin «fin de la definitiva». Pasadas `20261016-044647`, `-045730`,
    `-050436` y `-051002` (`_ports_test_combate_port`; `026_com.png`/`045_com.png` y `049_com.png`).
  - Las definitivas en las que no se llega a 0x88 acaban bien (`-051002`: la 359 entera, 22 s, saltando 0x9A).
    `-045100`: combate entero y vuelta al menú.
  - Antes de la guarda (`20261016-043252`), las definitivas acababan cuando la pila traía 0 o 1 en p[1]: la orden 0x92
    de B3, por casualidad, devolvía al luchador a su estado. El guion de IW espera algo de su motor que en B3 no llega.
  - En `-051458` (con un intento de dar a p[1] el lado del argumento de IW; quitado: 0x88 no salió en esa pasada) hubo
    **otro cierre**, en la lista de efectos (`lr 0x82155A04`). Es la misma familia que el de la definitiva de Pan
    (`lr 0x821554B0`).
- **Resultado:** el cierre original está arreglado en la causa, pero **la definitiva de Goku GT no está bien**: se
  cuelga cuando el guion pasa por 0x88, y hay un cierre aparte en los efectos. **No se puede desbloquear el 44.**
  Como los otros ports de IW llevan las mismas órdenes, **también corrijo lo dicho para 45, 54 y 56**. Pasan combates
  enteros, pero si el COM lanza su definitiva en una misión pueden caer en el mismo cuelgue. Mejor dejar el `for` de
  `MotivoIW` como está (`src/pasarela_misiones.cpp`, `id == 44 || id == 45 || id == 54 || id == 56`) hasta arreglar
  el guion.
- **Siguiente paso:** traducir el guion de definitivas de IW al de B3 en el montaje, como ya se hace con B1/SB
  («definitiva traducida a la cinemática del donante»). Es decir, usar las ranuras de definitiva del `#SPX` del
  donante con las animaciones de IW, en vez del `#SPX` de IW entero. Por separado, el cierre de la lista de efectos
  (`sub_82152890`: anillo de 2048 nodos sin mirar si siguen en uso).
- **Archivos:** `src/ports_guard.cpp` (guardas de órdenes de IW; las medidas temporales quitadas; copia final en
  `E:\DBZ3HD_trabajo\R9\ports_guard_final.cpp`), `src/eu_map_gen.h` y `docs/14_frentes/eu_mapa.txt` (8 parejas nuevas
  en total), esta sección. Compila (`tools/compilar.py`, 05:21).

### P4 Minijuegos IW, séptima vuelta (ronda 4, 05:35)
- **`src/ps2_modelo.{h,cpp}` (namespace `dbz3::ps2`)**: lector de #AMO/#AMT/#AMM de PS2 con esqueleto y pesos reales,
  `Posar` (pieles en CPU) y pintado con la cámara del combate tapado por los luchadores (`Escena`). En las dos listas
  de CMakeLists. Para R2 (Shenron de B2) y R11: API y ejemplo en la cabecera y en PORT_EXACTO.md, P4.
- `iw_minijuegos.cpp` lo usa; regresión PASS y a ojo (0, Radar 0x7DF, Piccolo 0x7DD:
  `D:\DBZ3HD\_pruebas\20261016-051917_*`, `-052214_*`, `-052621_*`).
- Cámara por encima del suelo del campo (`suelo_iw::Altura`, R11) y Bubbles en el suelo: compila; Piccolo PASS
  (`-052921_*`).

### R10 tercera vuelta (2026-10-16 05:35): corredor de la IA y cansado, en cualquier luchador
- **RE del corredor (0xE2):** el único `#BSK` de IW que define 0xE2 sigue siendo el de **Bubbles** (DATA_CMN 912,
  esqueleto `BBR_*` de mono; está en el registro 11 del ELF, +0x144). He mirado los movesets de minijuego/escena de
  IW (DATA_CMN 912-932: 914 Cell, 915 Goku, 920 Gohan, 924 Krilin, **925 Piccolo**, 928 Trunks, 931 Vegeta...):
  ninguno define 0xE2. Para Piccolo, 925 (`XPIC`, 64 huesos) define 0xDF-0xE1 (sus anims 5-7, pasos cortos) y su
  **código 4 = anim 4, 32 cuadros, piernas 77-80°: el único ciclo de carrera humano de los datos de IW**. Es el que
  usa `Corredor`. ponytail: no he encontrado qué hace IW con 0xE2 en un humano (con el `Animations` del decomp, un
  código que no está en ningún `#CSK` cae al 0); si aparece, se cambia la fuente en `Leer`.
- **Orientación del corredor:** medida su cadera: eje Z a −34°/−57°, como la guardia de B3 (−47°), columna casi
  vertical: **ya está en la convención de B3, sin giro** (el giro de −90° solo hacía falta en el scatto, que IW corre
  hacia +X).
- **Cansado (RE):** `Minigame_State16_Update` (EU 0x2A04A0): con la barra (X+0x185C) a 0 pone +0x1C0 = 1 (el botón
  del scatto ya no responde: `Minigame_ReadPlayerButtons` pide barra > 0 y +0x1C0 == 0), la barra a 1 y, en el suelo,
  `Char_EnterState7Transform`: código **0x05** y luego **0x06** (`Char_UpdateAnimToggle`). Son las animaciones 4 y 5
  del almacén global, **iguales en B3** (casan 44/46 huesos): no hace falta portarlas, se arrancan los códigos de B3.
- **Hecho (`src/anim_iw.{h,cpp}`):** `dbz3::anim::Corredor(x, bool)` y `dbz3::anim::Cansado(x, bool)`, aparte de
  `Scatto`, para cualquier luchador (hasta 2 a la vez; un modo por luchador). Las 7 animaciones de IW (6 del scatto +
  el corredor) van detrás del almacén global de B3 y el `#CSK` propio define 0x19-0x1E y 0xE2. Cuando el juego pisa el
  código (la IA de P4 reentra en andar cada pocos cuadros) se vuelve a poner **siguiendo por el cuadro en que iba**
  (A+0x608), para que el ciclo no se quede en los primeros cuadros (antes: 127 reinicios y pose congelada; ahora 24 y
  la zancada avanza). Prueba: `DBZ3_TEST_CORREDOR=1` (el segundo luchador corre) y `DBZ3_TEST_CANSADO=1` (el jugador,
  parado, cansado).
- **Líneas para P4 en `src/iw_minijuegos.cpp`** (no lo he tocado; `#include "anim_iw.h"` arriba):
  - Piccolo (minijuego 14, segundo luchador `x1 = mgr + kX0 + kXPaso`): al empezar a perseguir
    `dbz3::anim::Corredor(x1, true);` y al alcanzarte o acabar `dbz3::anim::Corredor(x1, false);`.
  - barra del scatto a 0 (en el suelo): `dbz3::anim::Scatto(x0, dbz3::anim::kApagado);` y
    `dbz3::anim::Cansado(x0, true);`; cuando IW deja de estar cansado (la barra se recarga, no leído):
    `dbz3::anim::Cansado(x0, false);`. Con la barra a 0 no se acepta el botón del scatto.
  - el seguro apaga todo fuera del minijuego o de la pelea.
- **Prueba:** compila (`tools/compilar.py`, 05:28; antes falló un rato por `src/suelo_iw.cpp`, de otro frente, y se
  esperó). `correr anim_test iw_minijuego --par clave=0x7DD --par npc=1 --par auto=0 --env DBZ3_TEST_CORREDOR=1
  --env DBZ3_TEST_CANSADO=1` PASS: `D:\DBZ3HD\_pruebas\20261016-052811_anim_test_iw_minijuego`. Revisado a ojo:
  `d_mirar.png`: Piccolo, de espaldas y muy cerca, en plena zancada de carrera (pierna izquierda delante, derecha
  detrás, brazos abiertos), sin huesos rotos. `b_npc.png` / `c_npc.png`: Gohan parado con 0x06 (guardia baja) y 0x05
  (salto con los brazos abiertos). La primera vuelta de esta prueba (sin seguir el cuadro):
  `...\20261016-052152_anim_test_iw_minijuego` (`c_npc.png`: Piccolo de cerca).
- **Pendiente / ponytail:** no he visto en las capturas un ciclo completo de Piccolo (solo una zancada); el aspecto
  de "cansado" en B3 (0x05: saltito con los brazos abiertos; 0x06: guardia baja) no lo he comparado con IW, solo es
  lo que dice el decomp (además, cada personaje puede redefinir 0x05/0x06 en su moveset, en IW igual); la recarga de
  la barra no está leída.

### R8 quinta vuelta (2026-10-16, 05:45): cámaras de presentación de IW y prueba empaquetada (sin publicar)
- **Cámaras de la presentación: las del propio campo de IW (datos originales, sin escalar a mano).** La entrada
  siguiente a cada campo (DATA_CMN 885, 898, 901, 904, 907, 910) es `#AMB [#WCF, #AMC]`, y ese `#AMC` tiene **el mismo
  formato que el de los escenarios de combate**: 3 pistas de tipo 0x1D de 300 cuadros (comprobado contra el de
  Llanuras). `campos_iw.py` pone ese `#AMC` como primera cámara del objeto 0 (la que pide el script `#SPX` de la
  presentación) en lugar del de Llanuras; `VERSION` 5 (el paso de JUGAR los rehace solo). Revisado a ojo, sin planos
  de solo cielo: **903** la casa del abuelo Gohan, la cascada, el cráter de la nave y luego los planos de los
  luchadores (`D:\DBZ3HD\_pruebas\20261016-051407_stage_test_r8_intro`, mosaico `E:\DBZ3HD_trabajo\R8\intro_903.png`);
  **897** el coche del Kaio por el camino, el garaje y la casa (`...-051407_stage_test3_r8_intro`,
  `E:\DBZ3HD_trabajo\R8\intro_897.png`). Los planos de cada luchador (el 4.º y el 5.º) los pone el script de B3 a su
  altura; el combate empieza con "READY?" normal. Prueba: `E:\DBZ3HD_trabajo\R8\pruebas\r8_intro.toml`.
- **Prueba empaquetada, sin publicar nada:** Kit con `make_modpacks.py --solo kit` (con `OUT` desviado a
  `E:\DBZ3HD_trabajo\R8\pkg\release_packs`: `DBZ3HD-prueba-r8-Kit-Modding.zip`, 72,5 MB, Python portátil 3.14.3 con
  numpy 2.5.1 y Pillow 11.3.0 comprobado por el propio script) y release con
  `make_release.ps1 -Version v-prueba-r8 -OutDir E:\DBZ3HD_trabajo\R8\pkg\release-stage` (zip de 65 MB al lado).
  Juego montado como lo haría un jugador (release + Kit descomprimidos) en `E:\DBZ3HD_trabajo\R8\pkg\juego`:
  - Antes de convertir no hay ningún `.afs`, `.iso` ni `geom.bin` en el paquete (`mods/` solo `README.md`,
    `ps2_games/` solo los LEEME): no viaja ningún dato del juego.
  - `python\python.exe "mod center hd\campos_iw.py" auto ...` (el Python del Kit, `xbcompress` de
    `mod center hd\tools` del Kit): los 9 escenarios en 1 min 44 s; el bin del 903 es **idéntico byte a byte** al
    generado en el proyecto.
  - Copia de prueba desde el paquete (`prep stage_pkg --desde E:\DBZ3HD_trabajo\R8\pkg\juego`) con el paso de JUGAR
    (`DBZ3_TEST_ESCENARIOS_AUTO=1`): `mod pipeline: "D:\DBZ3HD\stage_pkg\mod center hd\campos_iw.py" "auto" ...` →
    `done (exit 0)` y los 9 `creado ...` (`D:\DBZ3HD\_pruebas\20261016-052423_stage_pkg_r8_limite`; esa prueba se
    quedó en el título porque la copia del paquete no trae partida, no por el paso).

### R11 · Suelo real de los campos de IW (2026-10-16, 05:40)
- **Hecho: el luchador pisa el terreno de IW y no sube las paredes de las mesetas** (sin esperar al #WCF).
  - **Consulta de suelo = `sub_820CB0A0(f1 = x, f2 = z, r5 = registro opcional) -> f1 = altura`** (comprobado en sus
    llamadores: `sub_820C0B28` pone `vec.y` con el resultado de `(vec.x, vec.z)`; `sub_8211B1F0` la usa con las esferas
    del cuerpo del luchador; la llaman 22 funciones). Gancho `DBZ3_HOOK_EU` (ya tenía pareja EU): llama al original y,
    si está cargado un campo de IW, cambia f1 por la altura del terreno. **El juego pega solo al luchador a esa altura**
    (medido: y = 32,0 en la salida del 903 con terreno 32,0; 0,9 / 1,0; −2,0 / −2,0; arriba de una meseta 151,2 / 151,2).
  - **Campo cargado:** G+0 (0x823A3D80) = 16 (anfitrión) y la entrada del registro 16 = 5060..5065 (las altas de
    `campos_iw.py`, en el orden de `sorted(CAMPOS)`); sin tocar `escenarios.cpp`.
  - **Terreno:** triángulos de los **huesos de suelo** del #AMO del campo (hijo 5; mismo lector que P4 sin el tope de 255
    huesos), en mundo (coordenadas y unidades de IW). Huesos de suelo = los que enlaza el #WCF (`FFPLNACOL_A1JIMEN01` ↔
    `FFPLNA02_A1JIMEN`): JIMEN (903/897/906), MITI y DAI (884), ROCK y RISE (909), `_A1G` (900); fuera árboles, hojas,
    sombras, agua, casas. 903: 28 021 triángulos, rejilla de 20 u (151×151), lectura en su hilo ~2 s, una vez por campo.
    Altura = la mayor de los triángulos que cubren (x, z). Prototipo y mapa de alturas: `E:\DBZ3HD_trabajo\R11\`
    (`terreno.py`, `alturas.py` → `alturas_903.png`: mesetas a 100-215, valle a −160, llano a 0).
  - **IW (decomp EU, subagente haiku):** `Coll_GetGroundHeightAtXZ` (0x1E48F0) = plano del triángulo del árbol BSP del
    #WCF que contiene (x, z), `y = (n·v0 − nx·x − nz·z) / ny`; **el minijuego no tiene paredes**:
    `Minigame_MovePlayerByStick` (0x299EE0) mueve sin consultar la colisión y luego pega al suelo; no hay pendiente
    máxima en el código. Las paredes de aquí son lo que pidió el frente (no del original): perillas `kPaso` 1,
    `kPendiente` 1,2 (~50°) y `kMuestra` 4 en `suelo_iw.cpp`.
  - **Paredes:** gancho `REX_HOOK_RAW(sub_8210E700)` (lógica de los luchadores, cada fotograma; solo US, como
    `anim_iw.cpp`), antes (lo que movió el minijuego en `Fotograma`) y después (lo que movió el juego): si el terreno
    del sitio nuevo sube más de `kPaso + kPendiente·d` sobre el último sitio bueno, o la pendiente en los últimos 4 u
    pasa de esa, vuelve al sitio bueno (desliza por x o por z si uno vale). Se salta al luchador oculto (el pivote de
    P4) y los saltos de más de 60 u (la salida). Trampa medida: hay que comparar con la y del sitio anterior, no con la
    de ahora (el juego ya la ha subido al suelo nuevo dentro del mismo cuadro).
- **Pruebas (capturas miradas), copia `suelo_test` con los 6 campos de `combate_test`:**
  - `D:\DBZ3HD\_pruebas\20261016-051407_suelo_test_iw_minijuego` (`--par clave=0x7D0 --par auto=0`, PASS):
    `a_salida.png` Goku de pie sobre el montículo de la salida (y 32) con la sombra en la hierba; `c_derecha.png`
    corriendo por la hierba y el camino; `d_adelante2.png` delante de las mesetas, pies en el suelo.
  - Sin paredes (primera versión) `...-051706_...` (`lejos=1`): subía la meseta (y 0 → 151 → baja a 100 por encima).
  - `...-053537_suelo_test_iw_minijuego` (`--par lejos=1`, PASS): **se para al pie de la meseta** (log
    `suelo_iw: pared ... frenado en (-155, -601)`, y 21,5 en la falda) y `b_lejos.png`: Goku contra la roca, con su
    sombra en la pared; la cámara fuera de la roca.
  - Versión final otra vez con `auto=0`: `...-053828_suelo_test_iw_minijuego` (PASS, 0 paredes falsas; `c_derecha.png`
    corre por la hierba y el camino con los pies en el suelo).
  - Compila (`tools/compilar.py`: sin errores, código 0).
- **Para P4 (cámara, una línea en `src/iw_minijuegos.cpp`, en `Fotograma` justo antes de `dbz3::cine::Camara(base, ojo,
  obj);`, ~línea 1633), con `#include "suelo_iw.h"`:**
  ```cpp
  float ojo[3] = ...;   // (quitar el const)
  ojo[1] = std::max(ojo[1], dbz3::suelo_iw::Altura(ojo[0], ojo[2], ojo[1]) + 8.0f);   // camara por encima del terreno
  ```
  `Altura(x, z, sin_suelo)` da `sin_suelo` sin campo de IW o fuera del terreno. Con el luchador ya a la altura del
  terreno, `ojo` y `obj` (p.y + kCamAlto) le siguen solos. El pivote (`Poner(base, x1, {.., 0, ..})`) puede quedarse en
  y 0: es invisible y las paredes no lo tocan.
- **Archivos:** `src/suelo_iw.h`, `src/suelo_iw.cpp` (nuevos), `CMakeLists.txt` (2 líneas: lista normal y
  `REMOVE_ITEM` de EU), esta fila y sección. Nada más tocado.
- **Pendiente / ponytail:** (1) solo el terreno es sólido: árboles (MIKI), casas, rocas sueltas y la camioneta se
  atraviesan (el #WCF los trae; su formato en disco sin leer: BSP + triángulos de 0x20 B con normal, material e índices,
  según el decomp). (2) Las paredes solo en US (gancho sin pareja EU; `tools/eu_map.py` no mira este archivo); el suelo
  sí va en las dos. (3) Probado a ojo solo el 903; los demás campos usan la misma regla de nombres (log
  `suelo_iw: campo N: T triangulos` al cargar; si sale "no tiene huesos de suelo", queda el suelo plano de antes).
  (4) El borde exterior sigue siendo la caja estirada de R8. (5) Perillas de pendiente a ojo: el original no limita.
  - Con una partida de prueba copiada de `du_test` (la del paquete no trae ninguna): PASS
    `D:\DBZ3HD\_pruebas\20261016-053601_stage_pkg_r8_limite`: `mod pipeline: done (exit 0)`, `escenarios: al dia`
    (segunda vez: no rehace nada), `iwmini: escenario 20 (campo de IW 903: instalado)`, niebla de IW puesta
    (`7DE1FFFF, 800..1800`). En `a_lejos.png` Goku acaba metido en la pared de una meseta: es el suelo plano, que
    arregla el frente R11 (suelo real), no el paquete.
  - El Python que usa el paso es el del paquete (`ModPipeline` prueba primero `python\python.exe` junto al exe). No
    se subió ni se publicó nada; `out/build/win-amd64-release` sin tocar. Paquetes y juego de prueba en
    `E:\DBZ3HD_trabajo\R8\pkg\` (escenarios generados en la primera prueba: `pkg\mods_generados\`).
- **Archivos:** `mod center hd/campos_iw.py` (cámaras del `#AMC` del campo, `VERSION` 5). Sin cambios de C++ en esta
  vuelta (compila igual que en la cuarta).
- **Pendiente:** el suelo y las paredes reales los hace R11 (consulta `sub_820CB0A0` con las alturas de IW); la caja
  estirada de `CAMPOS` y la sub-malla de colisión oculta del objeto 0 siguen en el bin para él (estructura descrita
  en la segunda y la tercera vuelta). Probar el paso de JUGAR con el launcher de verdad (las pruebas lo saltan con
  `dbz3_skip_launcher`).

### R11 segunda vuelta (2026-10-16, 06:05): fiel a IW (sin paredes), #WCF medido, los 6 campos probados
- **Paredes apagadas de serie** (fidelidad: `Minigame_MovePlayerByStick` de IW mueve sin consultar la colisión y luego
  pega al suelo; no hay pendiente máxima). Ahora el luchador **solo sigue el suelo**, como en IW: sube y baja mesetas
  por sus laderas. El modo con paredes queda en el código, documentado, con `DBZ3_SUELO_PAREDES=1`. Su gancho
  (`REX_HOOK_RAW(sub_8210E700)`) es solo la función US: con el xex EU no se ejecuta nunca. Sin la variable solo
  registra la altura cada 5 s (`suelo_iw: luchador 0 en (...), terreno ...`).
- **Altura frente a IW (#WCF), medido:**
  - **Formato del #WCF (904/0) leído** (`E:\DBZ3HD_trabajo\R11\wcf.py`): cabecera +8 = n<<16 | 0x10 (589 nodos de
    0x90 desde 0x40). Nodo: +0x0C nombre, +0x10 centro, +0x20 matriz 4x4 (fila 3 = traslación **relativa al padre**:
    A1JIMEN01 lleva (−1425, 0, −1425), el padre sin leer). En las hojas, +0x84 & 0xFFFF = cuántos registros de plano hay
    en +0x88. Registro de plano (0x20) = [nx ny nz d en mundo][grupo, banderas (7 = suelo; 6 y 8 en árboles), 0, 0].
    Grupo = [n<<16, 0x10007, n × (v1, v2, v3 punteros a vec4 locales, f)]. 903: 23 208 triángulos (21 779 de suelo).
    La traslación de cada hoja la saqué de sus planos (n·(v + T) + d = 0, mínimos cuadrados), porque el árbol de
    padres está sin leer.
  - **Comparación en el 903** (rejilla de 10 u): el #WCF y la malla de suelo coinciden a menos de 2 u en el **92 %**
    de los puntos (diferencia media 2,7 u). El resto: bordes de meseta (teselado distinto) y hojas que solo tienen
    planos llanos, cuya x/z no sale de los planos.
  - **Capas apiladas en la malla** (`capas.py`, puntos con dos alturas de suelo a más de 5 u): 897 0 %, 906 0,18 %,
    900 0,54 %, 903 0,72 %, 909 6,8 % (arcos y salientes) y **884 88 %** (el camino de la Serpiente es un tubo: cara de
    arriba y cara de abajo). Aquí gana la mayor, es decir, la de arriba del camino. IW coge el primer triángulo del BSP
    que contiene (x, z), y en el #WCF el camino solo trae suelo arriba.
  - **Conclusión: la rejilla NO es exactamente IW.** Es igual en lo llano y en las laderas. Puede diferir en los bordes
    de las mesetas y, en el 909, bajo arcos y salientes (aquí el luchador sube encima). Para ser exacto falta leer el
    árbol de padres del #WCF y usar sus triángulos con bandera 4 en lugar de la malla. Con el formato de arriba se
    estima en 1-2 h, más la prueba. No lo he hecho en esta vuelta. ponytail en `suelo_iw.cpp`.
  - Sin triángulo, IW devuelve 0 (`Coll_GetGroundHeightAtXZ`); aquí queda lo del juego, que es el suelo plano a 0 de
    `campos_iw.py`. Es lo mismo.
- **Pruebas, un minijuego por campo** (`--par auto=0`; con PNJ, `--par npc=1`), carpetas en `D:\DBZ3HD\_pruebas\`,
  todas PASS, capturas miradas:
  | Campo | Clave | Carpeta | Visto |
  |---|---|---|---|
  | 884 Camino de la Serpiente | 0x7D2 | `20261016-054347_suelo_test_iw_minijuego` | `a_salida`: **Goku de pie sobre el camino** (y 35,2, terreno 35,3; antes caía a las nubes). Al salirse del camino andando ("Right") no hay suelo y queda a y 0, entre las nubes, junto al cuerpo de la serpiente (lo mismo que el 0 de IW) |
  | 897 Planeta del Kaio | 0x7D1 (Bubbles) | `20261016-054716_...` | Goku en la hierba con la sombra a los pies (terreno llano a 0, como su malla) |
  | 900 Namek | 0x7D9 | `20261016-054928_...` | salida sobre la roca (y 31,9, terreno 32,0), pies en el suelo; luego por la zona de `W#_PARTS` (fuera del filtro: y 0, su malla va de −5 a 9) |
  | 906 Llanura de Piccolo | 0x7DD (Piccolo) | `20261016-055307_...` | log: Gohan con y = terreno (3,2 / 3,2; 3,3 / 2,7). **La cámara se mete en la ladera de detrás** (`b_npc`: solo hierba): es la línea de cámara que pone P4 |
  | 909 Roquedal | 0x7DF (radar) | `20261016-055808_...` | Goku de pie en el desierto (y 0, terreno 0) con el radar |
  Mosaicos: `E:\DBZ3HD_trabajo\R11\mosaico_884.png`, `mosaico_897_900.png`, `mosaico_906_909.png`. El 903 se probó en
  la primera vuelta (`...-051706_...`, sin paredes: sube la meseta y baja por encima, igual que ahora).
- Compila (`tools/compilar.py`: sin errores, código 0).
- **Archivos:** `src/suelo_iw.cpp` (paredes con `DBZ3_SUELO_PAREDES`, registro cada 5 s, notas del #WCF) y
  `src/suelo_iw.h` (comentario). Prototipos: `E:\DBZ3HD_trabajo\R11\wcf.py` y `capas.py`.
- **Pendiente:** (1) el port del #WCF de arriba (1-2 h). (2) En el 900, mirar si `W#_PARTS` (agua o lava) es suelo en
  su #WCF. (3) La cámara de P4 (su línea).

### P4 Minijuegos IW, octava vuelta (ronda 4, 06:00)
- **R10 puesto:** Piccolo corre con `anim::Corredor` (sin mando inyectado; quitada mi línea de combate.cpp); cansado
  del decomp (State7: 0x05 + 0x06 + 0x07, 92 cuadros sin mando, `Scatto(kApagado)` + `Cansado`), recarga +0,02 por
  cuadro y bebidas; el círculo no vale cansado. Quitado el "agotado hasta una bebida" (no era de IW).
- **Cámara contra paredes:** 12 muestras de `suelo_iw::Altura` entre la cabeza y el ojo; acerca el ojo si el terreno
  corta la vista (más la línea de R11 sobre el suelo). Piccolo: ya no mira la roca de cerca.
- **Pendiente:** la cruceta de la HD sigue lanzando técnicas (Masenko) por otra vía; Bubbles sigue sin pintarse en el
  juego (ni puesto al lado: no es oclusión; salía a 500 de alto por el #ITM, ya arreglado).
- Compila. PASS `D:\DBZ3HD\_pruebas\20261016-053735_*` (Piccolo), `-054338_*` (cansado), `-055821_*` (Bubbles, sin verle).

### P4 Minijuegos IW, novena vuelta (ronda 4, 06:30)
- **Bubbles se ve:** medido con una sonda (posición, proyección y 2654 triángulos entregados, todo bien): sus texturas
  de cuerpo tienen el alfa a 0 (la PS2 las pinta solo RGB). `ps2::Crear` las da opacas (vale para R2 y R11 también).
- **Sin técnicas en los minijuegos:** la flecha del teclado es la palanca derecha, que la HD usa de atajo de técnicas.
  En `src/net/netplay_hooks.cpp` (`sub_8221C178`, usuario 0, sin online), mientras `iwmini::Activo()`, la palanca
  derecha va a la cámara del minijuego (`iwmini::PalancaDerecha`) y no al juego; la cruceta pasa a la palanca.
  Comprobado: flecha derecha sin Masenko, la cámara gira.
- Salto del modo 2 de Bubbles: el decomp no tiene salto (25 es una distancia en el suelo).
- Compila. PASS `D:\DBZ3HD\_pruebas\20261016-060911_*` (Bubbles), `-062413_*` (Piccolo).

### R11 tercera vuelta (2026-10-16, 06:30): port de la colisión #WCF de IW (suelo y paredes)
- **Corrección de la segunda vuelta: IW sí tiene paredes en los minijuegos a pie.** La física del personaje es
  `Char_UpdateMovementPhysics` (decomp EU 0x2A8E20), no `Minigame_MovePlayerByStick`. Hace tres cosas:
  1. `Collision_MoveSphere` (barrido de esfera, máscara 7) del sitio al destino: las **paredes**.
  2. `Collision_Raycast` desde la cabeza (pos.y + altura del personaje) hasta 500 por debajo: el **suelo**.
  3. Si no hay corte, o está a 500 o más, cae (+0x1CC = 1 y fundido, `TransitionFilter`).
  Las dos consultas van contra el árbol del #WCF (`Collision_LoadAndRelocFile` / `Collision_RelocTree`, 0x2E7C80 /
  0x2E73A0). `Coll_GetGroundHeightAtXZ` y su BSP son la colisión de los escenarios de combate, no la de los campos.
  Por eso **las paredes vuelven a estar encendidas de serie**, ahora contra el #WCF; `DBZ3_SUELO_PAREDES=0` las quita.
- **#WCF leído en C++** (`LeerWcf` en `src/suelo_iw.cpp`, como `Collision_RelocTree`):
  - Cabecera: +0x0C = raíz; +8 = n << 16 | 0x10, con n nodos de 0x90 seguidos.
  - Nodo: +0x24 (u8) registros de plano en +0x28; +0x25 (u8) hijos en +0x2C; +0x3C nombre; +0x50 matriz 4x4 de la pieza
    **en mundo**. No hace falta componer con los padres: los planos del registro pasan por los vértices transformados,
    con error < 0,25.
  - Registro de plano (0x20): [nx ny nz d en mundo][grupo, material, 0, 0]. Grupo: [n << 16, ?, n × (3 punteros a
    vec4, f)].
  - Materiales en el 903: 7 = suelo, troncos y vallas; 6 = copas y casa; 8 = agua (MIZU).
  - Triángulos leídos: 903: 23 208; 884: 18 402; 897: 1 771; 900: 23 358; 906: 33 203; 909: 26 479. Los 6 se leen
    (log `suelo_iw: campo N: T triangulos (#WCF)`). La malla de la vuelta 1 queda solo de respaldo si un #WCF no se lee.
- **Suelo, como el rayo de IW:** el plano más alto que no pasa de la cabeza del luchador que pregunta (kCabeza = 20)
  entre los triángulos de máscara 7 que contienen (x, z). Es el primer corte del rayo hacia abajo. Se aplica en la
  consulta del juego `sub_820CB0A0`; la altura de cada luchador se toma de `sub_8210E700`.
  - Bajo los arcos del 909 el luchador se queda debajo; en el tubo del 884 pisa la cara de arriba.
  - Si la consulta no es de un luchador (efectos), se toma el plano más alto.
  - Sin corte, queda lo del juego (suelo plano a 0). IW ahí hace caer al jugador; eso es regla del minijuego (ver P4,
    abajo).
- **Paredes, como `Collision_MoveSphere`:** el paso de cada luchador se barre a 8 u (rodillas) y 20 u (cabeza) contra
  las caras empinadas (|ny| < 0,7) de máscara 7 que miran hacia él, más 4 u de radio. Si corta, desliza a lo largo de la
  pared; si el deslizamiento también corta, se queda en el sitio. ponytail: un segmento a dos alturas en vez de la
  esfera de IW, y radio, altura y respuesta sin leer (perillas `kRadio`, `kRodilla`, `kCabeza`, `kParedNy`). Siguen
  solo en US (`REX_HOOK_RAW(sub_8210E700)`, sin pareja EU).
- **900, `W#_PARTS` (lava/agua):** no están en su #WCF (todo material 7, losetas `FFNMHA..G`). En 400 puntos al azar
  sobre esas piezas, el #WCF trae suelo en todos, a la misma altura o por encima (media +9 u, hasta +69): en IW la lava
  no se pisa, se pisa la roca del #WCF. Queda resuelto con el port.
- **Pruebas** (`--par auto=0`, más `npc=1` en Bubbles y Piccolo; carpetas en `D:\DBZ3HD\_pruebas\`), capturas miradas;
  mosaicos en `E:\DBZ3HD_trabajo\R11\v3_903_884.png` y `v3_900_906_909_897.png`:
  | Campo | Carpeta | Visto |
  |---|---|---|
  | 903 `lejos=1` | `20261016-061140_suelo_test_iw_minijuego` | **se para contra la cara de la meseta** (`b_lejos`: Goku pegado a la roca, pies en la hierba; log `pared ... queda en (-148, -598)`) |
  | 903 | `20261016-061700_...` | baja del montículo de la salida (y 32 → −2 → 0) y corre por el camino; una pared pequeña (valla o tronco) en (47, −474) |
  | 884 | `20261016-061841_...` | de pie en el camino (35,2 / 35,3), roza el borde (19 paredes, desliza) y por un hueco sin pared sale del camino: sin suelo en el #WCF, queda a y 0 entre las nubes (en IW, caída) |
  | 897 (Bubbles) | `20261016-062018_...` | en la hierba con la sombra a los pies (suelo 0) |
  | 900 | `20261016-062155_...` | sale sobre la roca (31,9 / 32,0) y anda por la meseta de roca con los pies en ella (`d_adelante2`); al rozar rocas desliza. En `c_derecha` la cámara queda dentro de una roca (cámara de P4) |
  | 906 (Piccolo) | `20261016-062335_...` | Gohan a la altura del suelo (3,2 / 2,2) y **frenado contra la ladera** en (−259, −130) |
  | 909 | `20261016-062513_...` | Goku en el desierto con el radar, pies en el suelo |
  "Subir una meseta y bajar de ella": con las paredes de IW la cara de la meseta no se sube (como en IW). Bajar de un
  alto sí se ve: el montículo de la salida del 903 y la meseta de roca del 900. Subir por una rampa no lo he probado
  (la prueba no lleva a ninguna).
- Compila (`tools/compilar.py`: sin errores, código 0).
- **Para P4:**
  1. **Cámara como IW** (`Camera_UpdateFollow`: rayo de máscara 1 del objetivo a la cámara y, si corta, la cámara en
     el corte). Nueva función en `suelo_iw.h`:
     `float corte[3]; if (dbz3::suelo_iw::Rayo(obj, ojo, 1, corte)) std::copy(corte, corte + 3, ojo);`
     Sustituye a la línea de `Altura(...) + 8` (esa sigue funcionando).
  2. **Modo Piccolo (14):** `propio` lleva su propia posición y se escribe cada cuadro, así que las paredes la devuelven
     y `propio` sigue avanzando (log de 906: el paso crece hasta saltar). Hay que releer `propio` del luchador
     (`Leer(base, x0)`) antes de sumarle el andar, como en el modo normal.
  3. **Caída de IW** donde no hay suelo (Camino de la Serpiente): `Altura(x, z, centinela) == centinela` quiere decir
     sin suelo. IW pone +0x1CC = 1 con un fundido y repone al jugador (regla del minijuego, sin portar).
- **Archivos:** `src/suelo_iw.cpp` (`LeerWcf`, `Terreno::Suelo` / `Rayo`, paredes por barrido, `InicioRayo`; la malla
  queda de respaldo) y `src/suelo_iw.h` (`Altura`, nuevo `Rayo`). Prototipos: `E:\DBZ3HD_trabajo\R11\wcf2.py`
  (lector y comprobación de los planos). No quedó respaldo de la versión de la segunda vuelta (la copia falló porque
  faltaba la carpeta); su contenido está descrito arriba.
- **Pendiente / ponytail:** la máscara exacta del rayo de suelo (el argumento se pierde en el decomp; se toma la 7 de
  `MiniGame_QueryAnimState`); la esfera real de `Collision_MoveSphere` (radio y respuesta); la altura del personaje
  (char +0xC) en vez de 20; las paredes en EU (pareja de `sub_8210E700` con `tools/eu_map.py`).

### R9 cuarta vuelta (2026-10-16, 06:40): definitivas de IW sobre la cinemática del donante
- **Arreglo en el montaje** (`mod center hd/roster_build.py`, función nueva `iw_guion_donante`, llamada para todo
  personaje con `origen_fuente = "iw"`; `guion = "propio"` lo evita). Se hace como en los ports de B1
  (`b1port.port`):
  - **Guion y cámara del donante.** El `#SPX`, `#ACC` y `#ACL` del port pasan a ser los del donante; su `#CCM` (los
    golpes) se queda.
  - **Códigos de la cinemática.** El guion del donante pide los códigos 0x480-0x4FF (definitiva: ranura 0,
    `sub(cápsula, 0x4A0, …)`; agarre: ranura 20). Los que el moveset de IW ya tiene **se quedan con su animación de
    IW**: los de Goku GT son los mismos códigos y duraciones que los de Goku. Los que faltan se injertan del donante
    con su animación, añadida al `#ACM` propio.
  - **Ranuras que el donante no tiene.** Los golpes con guion en las ranuras 1 y 50 de IW pasan a la 0.
  - **Animaciones de la víctima.** El 2.º y 3.er `#ACM` son los del donante, como sus guiones. Los bloques del moveset
    de IW solo usan los bancos 3 y 0 (comprobado).
  - **Formas que solo cambian golpes** (`#CSK` sin `#ACM`): solo el cambio de ranura.
  - Comprobado en el roster montado: **ningún port de IW lleva ya órdenes de guion de IW**
    (`E:\DBZ3HD_trabajo\R9\spx3.py`).
- **Guardas en marcha que se quedan** (`src/ports_guard.cpp`): las de las órdenes de IW de la tercera vuelta, ahora de
  red. Nueva: `sub_82155990`, el anillo de 128 nodos de efecto, que reutilizaba un nodo aún enlazado en la lista
  ordenada (centinela `0x823EE820`) y causaba el cierre `lr 0x82155A04` de la tercera vuelta. Antes de reutilizarlo
  lo saca de la lista; parejas EU con `tools/eu_map.py`. En las 8 pasadas de esta vuelta no se ha activado, así que
  no está comprobada en el juego. Una guarda de recurso nulo en `sub_820CF580` se probó y se quitó: el cierre de
  Baby no pasa por ahí.
- **Pruebas** (`prep ports_test` monta el roster: 11-16 min la primera vez; luego
  `E:\DBZ3HD_trabajo\ports\combate_ports.py ports_test <id> 10 --com1 --killeno --seg 140`, el port llevado por el COM
  con sus cápsulas y ki lleno contra Krilin; carpetas `D:\DBZ3HD\_pruebas\20261016-<hora>_ports_test_combate_port`):

| Port (id) | Pasada | Definitivas | Resultado |
|---|---|---|---|
| Goku GT (24) | `060105` | 10 (259 y 359; dos enteras de ~25 s, la de 0x88 incluida) | **PASS**: cinemática de Goku con el modelo SSJ4 y el minijuego (`034/037/067_com.png`), pose y vuelta al menú, sin avisos de guardas |
| Pan (25) | `060525` | 2 (una entera de 29 s) | **PASS**: cinemática de Videl con Pan, «COM ATTACK/DEFENSE», 14 golpes (`061/065_com.png`), KO, vuelta al menú |
| Vegeta GT (26) | `061142` | 7 (enteras de 19 y 31 s) | **PASS** sin cierres: Final Flash con choque de rayos (`071/085_com.png`). La prueba acabó a mitad del combate (140 s); la vuelta al menú ya se comprobó en la segunda vuelta |
| Baby (23) | `061539`, `063037` | se cierra 1,5 s después de empezar la definitiva | **FAIL**: `lr 0x820CFF1C`. `sub_820CFEC8` monta un efecto y llama a `sub_8208DDF0(*(efecto+148))` con 0. Pendiente |
| Janemba (22) | `061936` | 6 (cinemática de Krilin, ≤5 s) | **PASS**: pose de victoria y vuelta al menú |
| Vegeta saga Saiyan (50) | `062305` | 5 (una entera de 31 s) | **PASS** sin cierres; la prueba acabó a mitad del combate |
| Yamcha amarillo (51) | `062701` | 5 (una entera de 21 s) | **PASS**: vuelta al menú |

- **`MotivoIW` (`src/pasarela_misiones.cpp`, el `for` con `id == 44 || id == 45 || id == 54 || id == 56`):** se
  pueden quitar **44 (Goku GT), 54 (Vegeta GT) y 56 (Pan)**. **El 45 (Baby) se queda** hasta arreglar su cierre.
  Antes de probar las misiones, `prep` vuelve a montar el roster de `modos_test` (el montaje cambió).
- **Pendiente:**
  - **Baby.** La definitiva de Vegeta (su donante) monta un efecto cuyo campo +148 queda vacío con el BSP de técnicas
    de IW de Baby. Siguiente paso: un gancho de medida en `sub_820CFEC8` (banco/índice que pide y +148) y, según lo
    que salga, `tecnicas = "donante"` solo para la definitiva o injertar ese recurso en su BSP.
  - La guarda de nodos de efecto, sin comprobar en el juego.
  - El nombre de la definitiva en el rótulo de la cinemática puede ser el del donante: la llamada `sub(cápsula, …)`
    lleva la cápsula del donante. No lo he mirado a ojo.
- **Archivos:**
  - `mod center hd/roster_build.py`: `iw_guion_donante`, `IW_CINE` y la llamada al montar.
  - `src/ports_guard.cpp`: guarda de nodos de efecto; copia en `E:\DBZ3HD_trabajo\R9\ports_guard_final.cpp`.
  - `src/eu_map_gen.h` y `docs/14_frentes/eu_mapa.txt`: regenerados.
  - Esta sección.
  - Compila (`tools/compilar.py`, 06:35). Respaldo del roster anterior de `ports_test` en
    `E:\DBZ3HD_trabajo\R9\respaldo_roster\`.

### R11 cuarta vuelta (2026-10-16, 06:45): pareja EU, valores del ELF, prueba de rampa (FALLA)
- **Paredes también en EU.** Añadido `src/suelo_iw.*` a `FUENTES` de `tools/eu_map.py` y vuelto a pasar.
  - Pareja medida: **`8210E700 -> 8210E800`**, "alineada (bloque de 188 iguales seguidas), 180 instr, difs sin
    explicar 0, punteros 1/1" (`docs/14_frentes/eu_mapa.txt`). Autoprueba 10/10; 295 de 296 direcciones verificadas
    (la que falta es de `cine.cpp`).
  - `src/eu_map_gen.h` solo gana esa pareja (diff: 1 `#define`, 2 `REX_EXTERN` y 1 fila; copia de antes en
    `E:\DBZ3HD_trabajo\R11\respaldo\`).
  - El gancho pasa a `DBZ3_HOOK_EU(sub_8210E700)`.
- **Valores del original** (contrapartes US de los envoltorios del decomp EU, desensamblados con `P4\iwdis.py` del
  `SLUS_218.42`):
  - **Rayo de suelo:** `Collision_Raycast` (US `func_2E75C0`) con **máscara 7** (0x296774 `addiu t2, zero, 7`). Ya
    era 7.
  - **Paredes:** `Collision_MoveSphere` (US `func_2E7430`) con **máscara 2** (0x2967E4 `addiu t1, zero, 2`). Era 7:
    cambiado. Los materiales 6 y 7 la cumplen; el 8 (agua) no.
  - **Radio de las paredes = `Char_GetActionSpeedScale(H − 1)`:** H − 1 al andar; ×0,5 en los estados 5, 6, 0x10 y
    0x14-0x16; ×1,2 en 0x12-0x13; ×1,8 cayendo. El barrido hacia abajo (`MiniGame_QueryAnimState`) usa radio 3,0 y
    máscara 7.
  - **H = `*(char + 3000) + 0xC`: SIN LEER.** El puntero no se asigna en ninguna línea legible del decomp (busqué
    `+ 3000`, `0xbb8` y `[0x2ee]`). Sigue `kCabeza = 20` como perilla, y por eso `kRadio = kCabeza − 1 = 19`.
- **Rampa del #WCF buscada** (`E:\DBZ3HD_trabajo\R11\rampa.py`: BFS con pendiente ≤ 45° sobre el suelo del #WCF, celdas
  de 5 u).
  - Desde la salida del 903, **ninguna meseta alta (≥ 95) se alcanza por rampa**. Lo más alto alcanzable es 66
    (hacia (−127, 992)).
  - Rampa elegida: pie en (−98, 362) a y ≈ 5 → (−102, 388) a y 24 → rellano a y ≈ 29 → (−188, 392) a y 59.
  - Prueba nueva sin mando ni cámara: `DBZ3_SUELO_PRUEBA="x,z;x,z;..."` lleva al luchador 0 a la primera marca 6 s
    después de la salida (junto con el pivote: si quedan lejos, el juego los junta, medido 330 u en 2 s) y lo hace
    andar a 0,2 u por cuadro por las demás. Se lanza con:
    `correr suelo_test iw_minijuego --par clave=0x7D0 --par auto=0 --par quieto=1 --env "DBZ3_SUELO_PRUEBA=-98,355;-102,385;-170,372;-190,395"`.
- **Resultado: la rampa FALLA con el radio de 19.** `D:\DBZ3HD\_pruebas\20261016-063713_suelo_test_iw_minijuego`:
  log `pared ... queda en (-97, 357)` cada cuadro; en `b_quieto2.png` Goku se queda al pie de la rampa.
  - Causa: mis paredes son dos segmentos horizontales a 8 y 20 u alargados 19 u. Con ese largo cortan la propia rampa
    y la ladera de delante (caras con |ny| < 0,7). La esfera de IW, en cambio, se desliza hacia arriba por los planos.
  - Las mesetas siguen frenando bien: `...-063519_...` (`lejos=1`) se para en (−137, −591), unas 11 u antes que con
    el radio de 4, como corresponde a una esfera mayor.
  - **No hay prueba de rampa en verde.** Con el radio de 4 de la vuelta 3 no la he probado.
- Compila (`tools/compilar.py`: sin errores, código 0).
- **Archivos:** `src/suelo_iw.cpp` (`DBZ3_HOOK_EU(sub_8210E700)`, `kMascaraPared = 2`, `kRadio = kCabeza − 1`,
  `Prueba`), `src/suelo_iw.h`, `tools/eu_map.py` (una línea en `FUENTES`), `src/eu_map_gen.h` y
  `docs/14_frentes/eu_mapa.txt` (regenerados). Copias de la vuelta 3 y del mapa EU anterior en
  `E:\DBZ3HD_trabajo\R11\respaldo\`.
- **Pendiente (lo que arregla la rampa):** portar `Collision_SweepSphereVsModel` (barrido de la esfera con respuesta de
  deslizamiento) y medir el centro de la esfera (+0x640 del personaje de IW) y H. Hasta entonces hay dos salidas de una
  línea:
  - `kRadio = 4.0f` (lo de la vuelta 3; la rampa sin probar).
  - `DBZ3_SUELO_PAREDES=0`.

### P4 Minijuegos IW, décima vuelta (ronda 4, 06:40)
- **Registro para R12/R13/R14** en `src/iw_minijuegos.h`: `RegistrarModulo(clave, Modulo)`, `Terminar(Resultado)` y
  `HudIW(segundos, barra, cara)`; `Empezar` despacha la clave al módulo y `Activo()` lo cubre. Detalle en
  PORT_EXACTO.md, P4.
- **Para R2:** la regla "alfa 0 en toda la textura → opaca" de `ps2::Crear` puede ser la de sus partes beis planas.
- **R11:** cámara con su `Rayo` (desde la cabeza); caída sin suelo → último sitio con suelo (Serpiente PASS, sobre el
  camino). Piccolo: releer el luchador no vale (B3 lo acerca al rival), se integra la marcha propia + rayo de pared,
  pero el rayo aún no le para en las mesetas (pendiente con R11).
- Compila. PASS `D:\DBZ3HD\_pruebas\20261016-063404_*` (Serpiente), `-063848_*` (Piccolo; cámara contra la pared).

### R11 quinta vuelta (2026-10-16, 07:15): barrido de esfera, H del modelo de B3, `Mover` para P4; la rampa FALLA con el radio de IW
- **1. Provisional `kRadio = 4`: la rampa SÍ sube.** `D:\DBZ3HD\_pruebas\20261016-064329_suelo_test_iw_minijuego`
  (`DBZ3_SUELO_PRUEBA`): y 5 → 16 → 28 → rellano 29 → 44 → 56 → fin del camino en (−190, 395).
  `b_quieto1/2.png`: Goku arriba de la loma de hierba, sobre el llano.
- **2. Barrido de esfera** (`Barrer` en `src/suelo_iw.cpp`, en lugar de los dos segmentos).
  - Esfera de radio r = H − 1 (`Char_GetActionSpeedScale`), apoyada en los pies (centro a pies + r), contra las caras
    del #WCF con máscara 2.
  - En el destino, el contacto más profundo (normal del punto más cercano, Ericson 5.1.5) de una cara que se opone al
    paso quita esa componente del paso, y se vuelve a probar (hasta 4 contactos; si no sale, se queda).
  - Solo cuentan las caras empinadas (|ny| < 0,7); el suelo andable lo lleva el rayo. Lo medí con
    `E:\DBZ3HD_trabajo\R11\barrer.py`, réplica en Python sobre el #WCF:
    - Contando todas las caras, los bordes de los triángulos de suelo dan normales horizontales y paraban al pie de la
      rampa.
    - Con el centro en los pies, las caras empinadas de debajo paraban al bajar una cuesta (el montículo de la salida).
  - El barrido hacia abajo de IW (radio 3, máscara 7, `MiniGame_QueryAnimState`) solo da el estado al minijuego 10 (su
    resultado no mueve al personaje): no hace falta para el suelo.
  - ponytail: la respuesta exacta de `Collision_SweepSphereVsModel` y el centro de IW (+0x640) no están leídos.
- **3. H = el del luchador de B3** (`Alto`): lo alto de la esfera de la cabeza (la 3.ª de las 22 del cuerpo, centro en
  X+1616 + 48, radio en [X+2996] + 36) sobre los pies (X+68). Medido: Goku 15,5-16,8, Gohan niño 10,5-11,6.
  - Con el máximo de las 22 esferas salía 31 con los brazos en alto, por eso solo la cabeza.
  - El rayo de suelo sale de pies + H.
  - El factor por estado (×0,5 / ×1,2 / ×1,8) no se aplica a los pasos del juego (los estados de B3 no son los de IW).
    Sí está en `Mover` (`escala`) para el minijuego.
- **4. `suelo_iw::Mover(desde, hasta, salida, escala = 1)`** (`src/suelo_iw.h`): el mismo barrido. H es la del
  luchador que está en `desde`; `salida` lleva la y del suelo (rayo desde la cabeza). Devuelve false, con
  salida = hasta, si no hay campo de IW cargado. **Línea para P4** (modo Piccolo, `src/iw_minijuegos.cpp` ~1163, en
  lugar del `Rayo(a0, a1, 7, corte)`):
  ```cpp
  const float a0[3] = {propio.x, p.y, propio.z}, a1[3] = {propio.x + dx * andar, p.y, propio.z + dz * andar};
  float s[3];
  dbz3::suelo_iw::Mover(a0, a1, s);
  propio = {s[0], 0, s[2]};
  ```
  En el scatto, `Mover(a0, a1, s, 0.5f)` (IW: estados 0x14-0x16 ×0,5).
- **Pruebas con el barrido final** (carpetas en `D:\DBZ3HD\_pruebas\`; mosaico `E:\DBZ3HD_trabajo\R11\v5_final.png`):
  | Prueba | Carpeta | Resultado |
  |---|---|---|
  | Meseta del 903 (`lejos=1`) | `20261016-070758_suelo_test_iw_minijuego` | **bien**: baja del montículo (32 → −1,7 → 0) y se para contra la cara de la meseta en (−138, −594); `b_lejos.png`, pies en la hierba |
  | Piccolo en el 906 (0x7DD) | `20261016-070945_...` | **ya no entra en la meseta**: `c_npc.png` y `d_mirar.png`, Gohan al pie de la pared, fuera de la roca, con Piccolo detrás. Pero tiembla: el `propio` de P4 y el tirón de B3 hacia el rival se alternan (log: (−223, −94) ↔ (−247, −124)); se arregla con la línea de P4 de arriba |
  | Rampa del 903 | `20261016-070453_...` | **FALLA**: con r = H − 1 = 14,7 se queda al pie, en (−94, 363) (las marcas 2-4 no se alcanzan; `b_quieto2.png`). La rampa es un paso estrecho (celdas de 5 u en `rampa.py`) con caras empinadas a los lados a menos de 15 u; la esfera de radio 4 cabía |
- Compila (`tools/compilar.py`: sin errores, código 0).
- **Archivos:** `src/suelo_iw.cpp` (`Alto`, `Cercano`, `Barrer`, `Paredes` con la esfera, `Mover`, `Prueba` que salta
  la marca que no se alcanza) y `src/suelo_iw.h` (`Mover`). Prototipos en `E:\DBZ3HD_trabajo\R11\`: `barrer.py` y
  `rampa.py`. Copias de la vuelta 4 en `respaldo\`.
- **Pendiente / decisión:** con H de B3 y r = H − 1 la esfera no cabe por esa rampa. Puede que en IW la H del personaje
  (char +0xC) sea menor que la cabeza del modelo de B3, o que la esfera de IW no vaya donde la pongo: los dos datos
  siguen sin leer. Salidas de una línea: un factor sobre r (con r = 4 la rampa subía) o `DBZ3_SUELO_PAREDES=0`. Lo
  exacto: leer H de los datos del personaje de IW (el puntero de +3000) y la respuesta de
  `Collision_SweepSphereVsModel` (decomp EU 0x2E6...).

### R13 · Minijuegos de DISPARO de IW (ronda 4, 2026-10-16, 06:30-07:20)
- **RE (decomp EU `E:\DBZ3HD_trabajo\ghidra\salida\iw_sles55347.c` con subagentes Haiku + ELF USA con `iwdis.py`):**
  las 6 tablas de clases (ELF USA 0x472B10 + 0x60 t) llevan, para el 3, un descriptor del motor de **vuelo** (0x369020-
  0x3694A0, `MgFly_*`, `"DestroyTheBricks"Minigame` EU 0x3691A0); para el 8 el bloque `Viewer_*` de MSN009 (0x348520-
  0x34BDA0); para el 9 el `Viewer_*` de MSN010 (0x3245D0-0x327F90 y 0x32B2A0-0x32E8xx); para el 15 `MgShoot_*` (0x3099E0-
  0x30CEF0). En esta zona las direcciones de código USA y EU casi coinciden; los datos que el juego rellena al empezar
  (BSS 0x5D8xxx USA = EU − 0x800) salen de tablas iniciadas (EU 0x47E010 = USA 0x47D880).
  - **9 (0x7D8) "Fire beams at Piccolo!" (MSN010):** primera persona; apuntar = girar la cámara (0,0165 rad por cuadro,
    0,01 desde el 3.er disparo; inclinación 20-160°); círculo (0x2000) dispara un rayo con una mano y luego con la otra,
    hacia el punto a 60 delante, a 8 por cuadro; falla a más de 80. Piccolo orbita a 60 + 10·f(fase) y ±45°, esquiva de
    lado al azar (2,0; 2,25 con vida < 40 %), quieto con vida < 25 %. Vida {900, 950, 1000, 1050, 1100, 1200} y daño por
    grupo de los 22 huesos (tabla 0x499A60: cabeza 40, tronco 3, brazos 12, piernas 10; radios por hueso). Tiempo
    {180, 160, 140, 120, 110, 90} s: es el **"130"** del aro azul de la captura 83. Rango por los segundos hasta el
    derribo (≤ 30 Z 10000, ≤ 45 S 8000, ≤ 60 A 6000, ≤ 75 B 5000, si no C 3000) × {0,6 … 2,0}, a la centena; sin
    derribo: fallo, 100 puntos. **No hay regla de "no pasarse"**: la frase es texto.
  - **8 (0x7D7) "Attack Frieza with Destructo-Discs!" (MSN009):** cámara 0,015 rad (0,010 cargando o con el disco en
    vuelo), giro ±65,73°, inclinación −25,73..65,73°. Círculo: carga 40 cuadros, otra pulsación: lanza hacia el punto a
    150, a 5 por cuadro; si la mira tocaba un hueso (5 + radio) el disco se guía a él (peso 1 − 0,04 por cuadro).
    Acierto: disco (5) contra hueso; fallo: a más de 150; 3 fallos seguidos: 120 cuadros de pausa. Gana con **7
    aciertos**; pierde si aciertos + discos < 7 o a los 180 s. Discos {12, 12, 11, 10, 9, 8}, esquiva al apuntarle
    B50 {45..75} %, 40 + azar % 80 cuadros. Rango como el 9 pero la banda peor vale 4000.
  - **15 (0x7DE) "Tien! Stop Cell!" (MSN016, `MgShoot_*`):** tiempo 1 (min, tabla +0xB0), energía 1,0; cuadrado
    mantenido carga +0,1 cada 2 s, círculo dispara (32 + 32 cuadros) y gasta 0,1; una espera de 220 cuadros la primera
    vez con ≤ 0,1; energía ≤ 0 = derrota. Peligro = altura de Cell (+0x2914, la y de su posición) / altura de la cámara
    (+0x84): ≥ 0,5 aviso DANGER (si no SAFE), ≥ 1 derrota. Al acabar el tiempo p = y / (3 · altura): rango ≤ 0,0001 Z,
    ≤ 0,1 S, ≤ 0,4 A, ≤ 0,7 B, < 1 C; puntos {3600, 4200, 4800, 5400, 6000} × 0,5 a la centena; derrota 100. Cámara
    (0, 400, 200) mirando a (0, 15, 0), FOV 45°, mira en x −111..74 y z 60..280 a 1,5 por cuadro; manos de Ten =
    `ITEMB00` (DATA_CMN 895); HUD DATA_USA 9/1 (SAFE t10, DANGER t11, escala 50-350 t07, arco t09, cara t22).
    **Falta:** cómo sube Cell y cómo le baja un Kikoho no está en `MgShoot_*` (va por la raíz de sus animaciones
    0x501/0x502/0x4B0-0x4B3 y el efecto 5): sin eso no se porta (no se inventa).
  - **3 (0x7D3) ladrillos (`# SHOOT #`):** es el motor de vuelo (`MgFly_*`): mira 2D con la palanca, blancos que cruzan
    en línea recta, se "fijan" al pasar la mira cerca y el círculo los rompe; 7 disparos; rango por aciertos con
    umbrales de una tabla de datos. Las cifras (número de ladrillos, umbrales, velocidades) están en tablas del ELF sin
    leer y depende del motor de vuelo (R12): no portado.
- **Hecho (`src/iw_mini_disparo.cpp`, registrado para 0x7D8 y 0x7D7):** combate propio (`hub::RequestBattleEx`) en
  Namek de B3; el jugador (Freezer en el 9: las manos de IW 892 tienen su piel lila y uñas oscuras; Krilin en el 8)
  **oculto** (X+3261/X+4925) en el centro, con el mando virtual del muñeco (X+5028 = 1: no anda ni ataca; el mando 0 lo
  lee el minijuego, comprobado); el blanco es el **modelo de B3** (Piccolo 11 / Freezer 27, muñeco quieto) puesto cada
  cuadro en su órbita de IW; cámara en primera persona con `cine::Camara`; impactos contra las **22 esferas de cuerpo**
  del luchador de B3 con los radios de la tabla de IW. Manos = modelos de IW `ITEMB04/ITEMB05` (892, sin centrar: están
  en el espacio de la cámara) con retroceso al disparar. HUD de IW (DATA_USA 9/1): aro azul con los segundos (t05 + cifras
  t04), retícula (t01: anillo, o cruz con un hueso dentro), vida de Piccolo (cara t23 en el aro t12, barra t06 + t03),
  rayo = bola de ki t24, chispa t21; en el 8, estrellas de aciertos (t19 + t20) y discos que quedan. Final: fundido a
  negro con **CLEAR** (334/3 t69, como P4) que tapa el final de B3, combate cerrado como ring out y `Terminar` con TIME,
  rango y puntos de IW.
- **Archivos:** `src/iw_mini_disparo.cpp` (nuevo); `CMakeLists.txt` (2 líneas); `src/iw_minijuegos.cpp` (1 línea al
  principio de `Fotograma`: `FotogramaDisparo(base, estado)`, el gancho de cámara del hilo del juego, que P4 no da a los
  módulos); escenarios `E:\DBZ3HD_trabajo\R13\r13_piccolo.toml`, `r13_piccolo_teclas.toml`, `r13_freezer.toml`.
- **Pruebas (compila; capturas miradas):**
  - 9 con piloto: `D:\DBZ3HD\_pruebas\20261016-071119_disparo_test_r13_piccolo` PASS: derribo a los 28,38 s → Z, 10000
    (`c_impacto.png`: Piccolo de B3, manos de IW, rayo y chispa; `e_fin.png`: CLEAR).
  - 9 con teclado: `D:\DBZ3HD\_pruebas\20261016-065459_disparo_test_r13_piccolo_teclas` y `...-071408_...` PASS (Quote = círculo dispara y
    resta vida; D y W giran la cámara; el luchador de B3 no hace nada).
  - 8 con piloto: `D:\DBZ3HD\_pruebas\20261016-070719_disparo_test_r13_freezer` PASS: 7 aciertos en 8,85 s → Z, 10000
    (`b_lanzado.png`: Freezer de B3, disco, estrellas y discos; `d_fin.png`: CLEAR).
- **ponytail (marcados en el código):** el escenario de IW de los dos (Stage_BeginLoad de un campo del contexto, sin
  identificar) → Namek; B3 junta a los luchadores a más de ~60, así que las distancias del 8 (150, 5) van × 0,4; f(fase)
  de la órbita de Piccolo = seno; duración/velocidad de las esquivas (en IW salen de la animación); sitio y cadencia de
  las manos (animación +0xD0/+0xE0 sin leer); el blanco flota en la pose de salto de B3 (no hay vuelo en B3); efectos de
  rayo y Kienzan dibujados (sprites de IW / elipse), no los #AME de IW ni los de B3; el 8 sin manos (las de Krilin no
  están identificadas); el mando 0 no abre la pausa mientras dura (el jugador lleva el mando virtual).
- **Pendiente:** 15 (subida de Cell: leer la raíz de sus animaciones o una sonda en PCSX2), 3 (tablas del motor de
  vuelo, con R12), las manos de Krilin del 8 y el escenario de IW de cada uno (con R8).

### R14 Minijuegos de BOTONES de IW (2026-10-16, 06:30-07:25)
- **RE (ELF USA `SLUS_218.42` con `E:\DBZ3HD_trabajo\P4\iwdis.py`; decomp EU con nombres; aquí código EU = USA + 0x330,
  datos EU = USA + 0x7B0; scripts `E:\DBZ3HD_trabajo\R14\seqs.py` y `tablas.py`):**
  - Los seis usan la **librería QTE común** (EU 0x2EE6D0-0x2EF290): `QTE_SetSequence(n, tabla, fila)` copia códigos de
    botón; **código i = bit (1 << i) del mando de PS2** (tabla 0x497E90 USA): 8 L2, 9 R2, 10 L1, 11 R1, 12 triángulo,
    13 círculo, 14 equis, 15 cuadrado. Módulos (tablas de clases 0x472B10): 5 = 0x373800-0x37D200, 6 = 0x2926C0-0x295B80,
    11 = 0x311D20-0x316760, 12 = 0x350700-0x3584E0, 13 = 0x32E5D0-0x333B60, 18 = 0x2EAA20-0x2EC6A0 (USA).
  - **Familia A, "dos carriles" (5 NB006, 12 NB013, 13 NB014; mismo código clonado, `QteMg_*` del decomp):** por
    dificultad dos secuencias de 50 (S + d·0x70; carril 1 en +0x32): carril 0 = botones de la derecha, carril 1 =
    gatillos. Separación entre iconos en cuadros: S + 0x2A0 + d·0x190 + carril·200 (50 ints). Los iconos van de izquierda
    a derecha a la velocidad del módulo (px por cuadro en la pantalla de 640; carriles en y 366 y 409), se aceptan con
    x > W/2 + 50, fallan al pasar W/2 + 150 y se quitan en 495. Vida 1000 los dos; acierto en el carril 0 = el rival
    pierde el ataque del jugador, fallo en el carril 1 = el jugador pierde el del rival (`QteMg_ApplyHitDamage`).
    Nota = aciertos / iconos que llegaron × 100 (`SkillEdit_ComputeStatPercent` +0x2DC). Rango (`QteMg_ClassifyResult`,
    umbrales de `s_MSN_NB0xx_Updata`, iguales en los tres): 95-100 → 5, 81-94 → 4, 69-80 → 3, 56-68 → 2, 1-55 → 1, vida
    del jugador a 0 → 0. Puntos (`QteMg_FinalizeResult`, el `0x32EDC0` USA de R1) = tabla[rango] × multiplicador[d]
    (0,6 0,8 1,0 1,3 1,6 2,0) redondeado a la centena; rango 0 → 100.

    | Módulo | S (USA) | Puntos (USA) | Velocidad d0..5 | Ataque jugador / rival d0..5 | Tabla de puntos |
    |---|---|---|---|---|---|
    | 13 NB014 | 0x480530 | 0x481170 | 1,7 2,1 2,6 3,2 3,8 4,4 | 50 35 25 20 15 10 / 15 25 35 45 55 65 | 8000-10800 |
    | 12 NB013 | 0x483060 | 0x483F40 | 2,0 2,5 3,0 3,5 4,0 4,7 | 20 20 15 15 10 10 / 25 35 45 55 65 75 | 10000-13600 |
    | 5 NB006 | 0x484720 | 0x485670 | 2,0 2,5 3,0 3,5 4,0 4,7 | 15 15 10 10 5 5 / 35 45 55 65 75 85 | 9000-11800 |
  - **Familia B, "rondas" (6 NB007 `0x292BB0`, 11 NB012 `0x3124F0` USA):** 10 rondas (la lista 1-10 de la captura 88)
    con orden barajado (`Mg_ShuffleRoundOrder`) de 20 secuencias de 10 por dificultad (tabla 0x472F30 / 0x47DB10 USA +
    d·0xD0); botones por ronda +0xBC y en la última +0xC0 (NB012 d0: 3 y 4, `Mg_SetHitThresholdsByLevel`); un cursor
    (objeto 0x20) recorre la fila y se juzga cada botón a 80 px (`Mg_UpdateQteInput`): juicio 0-2 suma
    +0x108[juicio] / botones de la ronda a la nota de la ronda (100 = perfecta), 3 = fallo; NB012 deja elegir entre 4
    personajes (tabla 0x47FAC0 EU, por defecto el 7). Tablas de puntos 0x473680 (NB007, 5000-6200) y 0x47F2D0
    (NB012, 5500-6700). **Sin portar.**
  - **18 MSN019 (Videl):** no usa `QTE_SetSequence`: N objetos (0x5D91C4, cargados de DATA_CMN 0x5AA-0x5AD = 1450-1453)
    que se juzgan en 4 categorías con la palanca (umbral 0x20) y los botones; nota = suma de puntos por categoría,
    rango por umbrales (0x5D918C...) y premio × multiplicador, redondeado. **Sin portar** (falta leer el formato de
    esos datos).
  - **Piezas de IW** (en la copia del jugador, buscadas por un subagente haiku): botones DATA_USA 0/8 t74 (128×128,
    celdas de 32: cuadrado, triángulo, círculo, equis / L1 R1 L2 R2), READY? 5/1 t08, START! 5/1 t00, HIT! 5/1 t07,
    MISS! 5/1 t02, GOOD!/NORMAL/BAD/GREAT! 4/0 t08-t15 (para la familia B y 18), cifras 3-2-1 5/1 t04-t06.
- **Hecho (familia A completa: 0x7D4, 0x7DB, 0x7DC), `src/iw_mini_botones.cpp`:** se registra en `RegistrarModulo`;
  lee del ELF del jugador las secuencias, separaciones y tablas de puntos (comprueba que son botones válidos), y de
  DATA_USA las piezas de arriba. Combate de B3 con los dos personajes (rival con `SetMuneco(1)`, sin marcador de B3),
  puestos a 11 de distancia; vida de B3 repuesta cada cuadro (la que cuenta es la del minijuego). Lógica de IW a 60
  cuadros por segundo en el hilo de la interfaz (pausa con el juego). **Mando real:** el registro del mando 0 **+12** lleva
  los bits de PS2 (medido: X → 0x8000 cuadrado, Y → 0x1000 triángulo, A → 0x4000 equis, B → 0x2000 círculo, LB → L1,
  RB → R1, LT → L2, RT → R2), así que los botones de IW se comparan tal cual; además el luchador de B3 hace su golpe con
  ese botón (cuadrado puñetazo, triángulo patada...) y el rival lo recibe con sus animaciones de B3. Pantalla: dos
  carriles abajo con los iconos de IW, HIT!/MISS!, READY? y START!, arriba las dos vidas con el aro, la cara y la barra
  del HUD de minijuegos de IW (6/0, en espejo para el rival), CLEAR al superar. Resultado (rango 1-5 → 0 C .. 4 Z,
  puntos, aciertos) a `Terminar`; el combate acaba con mgr+35569 (como P4).
- **Prueba:** compila. Escenario nuevo `correr botones_test _r14_botones --par clave=0x7DC` (auto: pulsa el botón
  correcto **por el mando virtual** `net::InyectarPad`, o sea, por el mismo camino que el jugador; L2/R2 son gatillos y
  se dan por pulsados dentro). PASS, capturas miradas:
  - 13 gravedad (Vegeta/Goku) `D:\DBZ3HD\_pruebas\20261016-065531_botones_test__r14_botones`: 75/75, rival a 0, rango 5,
    10800 puntos (= 10800 × 1,0); `b_iconos`: Vegeta golpea a Goku ("2 HITS!! 200 DMG"), `d_fin`: CLEAR.
  - 5 Gohan/Piccolo `...-070416_...`: 97/97 (el carril 1 de NB006 trae 47 iconos), 11800; `c_iconos`: caras de Gohan
    y Piccolo, Gohan pateando.
  - 12 Cámara del Tiempo (Gohan/Goku) `...-070906_...`: 100/100, 13600.
  - Sonda del mando con el teclado (`--par auto=0 --par teclas=1`): `...-064828_...`.
  - Regresión con el código final (07:30) `...-073011_...`: 75/75, 10800, CLEAR (`d_fin`). La cara t33 de 6/0 sale
    rubia (¿Vegeta SSJ?): revisar qué cara es la de Vegeta normal.
- **Pendiente / ponytail (en el código):** personajes de 12 y 13 y los escenarios sin leer del módulo (van Gohan joven
  contra Goku y Vegeta contra Goku; 5 en el rocoso); el sitio del objetivo (objetos 1 y 2 de la UI del módulo) y la
  pieza propia de las barras (UiFade) sin leer: objetivo pintado entre aceptar y fallar, barras del HUD 6/0; el
  "golpe doble" (carril 2: los dos iconos solapados a la vez, `QteMg_ProcessComboInput`) y el botón especial al azar
  de NB006 (`Mg_SpawnQteButton`) no están; en NB006 la separación -256 se toma como fin del carril; el rival no ataca
  al fallar el carril 1 (haría falta un modo "ataca" público en `combate::SetMuneco`, que ya existe dentro de
  `combate.cpp`: para su dueño); la cámara y las poses de IW (intro 0x4B0, READY 0x1A4, victoria 0x14/0x15) son las de
  B3; el contador "DMG" de B3 se sigue viendo. **Familia B (6 y 11) y 18 sin portar** (RE arriba; siguiente vuelta).

### R12 · Minijuegos de VUELO de IW: 7 (0x7D6, Vegeta) y 17 (0x7E0, carrera con Goten) (2026-10-16, 06:30-07:25)
- **RE (ELF USA con `iwdis.py` + decomp EU; el módulo `# FLIGHT #` vive en EU 0x339620-0x345000, USA = EU − 0x500, su
  estado en `*(gp-0x6FD0)` = G).** Scripts y volcados en `E:\DBZ3HD_trabajo\R12\` (`vuelo_decomp.c`, `msf_dec.py`/`.txt`,
  `names891.py`, `afs891.py`).
  - **Carga** (USA 0x33E040): variante en G+0x4F0 (0 = Vegeta, 1 = Goten), personaje G+0x4F4, dificultad 0..5 G+0x4F8.
    Personajes (tabla 0x482860): v0 **Krilin 10 o Gohan niño 2 contra Vegeta 7** (traje 3); v1 **Gohan 4 o Trunks niño 9
    contra Goten 6**. Cuál de los dos lo decide el tipo de escena (+0x10 == 9 / == 2), sin leer: va el 1 (Gohan niño,
    Trunks niño; `DBZ3_IWVUELO_PJ=0` el otro).
  - **Campo:** v0 **DATA_CMN 883** (`MFNMCA`, Namek de vuelo: nubes, cielo con luna, terreno de 112 piezas en losetas de
    4000 y la **casa del Gran Patriarca** 883/7 `A1_SAITYOU`), v1 **881** (`MFEARA`, Tierra: 348 piezas, 881/7 casa).
    Ninguno es de los que convierte R8 (no llevan `#WCF`: el vuelo no choca). Se usa el campo de R8 más parecido (900,
    Namek de lava; 903, llanura) o, sin instalar, el escenario de B3 (Namek 7, Llanuras 5). Convertir 883/881 = R8.
  - **Datos = DATA_CMN 891** (#AMB): aro `RING_A01`+barrera (3/2), moneda `ITEMA11_COIN` (7/6), objetos `ITEMA02/03/08/10`
    (11, 14, 17, 20), flecha `ITEMA04_ARROW` (1), **#MSF 23 (v0) y 24 (v1)**, 3 `#AMC` de cámara y voces ADX.
  - **#MSF:** cabecera +0x14 hover 16, +0x18/+0x1C/+0x20 empuje de los estados 3/5/6 (10/34/56), +0x24 28, +0x28 10,
    +0x34..0x3C freno 0,98, +0x44.. giro por estado (192, 128, 96, 96, 96 en 1/65536 de vuelta por cuadro), +0x58/+0x5C
    180 (duración de los objetos 2 y 3), +0x60 + 4·(tipo & 0xF) radio de recogida (26 aro, 20 moneda...); +0xA8 seis
    bloques por dificultad: salida {estado, giro, x, z}, meta {flags, tipo, x, z}, L1 puntos de control (tipo 0x100), L2
    monedas (0x201/0x101/1 = 100/500/1000 puntos) y objetos, +0x14 medidor inicial, **+0x18/+0x1C/+0x20 subida del
    medidor (base, extra en el estado 5, extra en el 6)**, +0x24 300 cuadros de congelado, +0x28 120 de reaparición,
    +0x38 rangos en segundos (el 5.º es el límite: 110/105/90/100/90/90 s en v0), +0x4C puntos por rango, +0x60 puntos
    al fallar (100). v0 tiene 19 puntos de control y la meta en la casa; v1, 19-29 todos visibles a la vez.
  - **Vuelo** (`Viewer_StateFlyUpdate0/3/5/6`, `Viewer_ReadControlAxes`): **altura fija 400** (no hay palanca vertical);
    la palanca arriba da velocidad (hover: 16·v|v|; crucero/R1: max(v·0,98, empuje·v²)), los lados giran (giro −=
    h·tasa), **R1 (0x800) = estado 5**, el 6 solo con el objeto 2 activo, el objeto 3 impide R1; zona muerta 64,
    (v ∓ 64)/64. El mundo **da la vuelta** (`Node_WrapPositionInGrid`; 16000 en v0, 24000 en v1, de los datos).
  - **Objetos** (`Viewer_UpdateObjectCollection`): en el plano, con barrido desde el sitio anterior. v0 enseña los puntos
    de control **de uno en uno** (G+0x504), v1 todos; con todos cogidos sale la meta (tipo 0). 2 = velocidad (estado 6),
    3 = sin R1, 8/0x108 = congela el medidor 300 cuadros, 10 = efecto al azar (sin portar).
  - **El rival es un MEDIDOR** (`FlyingMinigameSpritesScripts`, EU 0x33C660): G+0x51C += base + extra del estado 5/6
    cada cuadro; a 1, **Vegeta te atrapa** (v0: base −0,0001..−0,0003 y +0,0008..+0,0038 al acelerar: **Vegeta siente
    el ki**, hay que volar sin R1) o **Goten llega antes** (v1: +0,0001 fijo, ≈ 167 s). v0 pierde también con el límite.
    En IW el rival solo aparece en las escenas (bits G+0x54C de las escenas, modelos G+0x7A0/0x7A4).
  - **Resultado** (`Viewer_StateCheckResult/Success/Fail`): meta con t ≤ límite → rango = 4 − (umbrales de +0x38 que
    supera t), puntos = monedas + puntos del rango; si no, rango −1 y 100 puntos.
- **Hecho:** `src/iw_mini_vuelo.cpp` (nuevo, ~740 líneas), registrado para 0x7D6 y 0x7E0. Lee el #MSF de la copia del
  jugador y hace todo lo de arriba con sus números: vuelo por estados, mundo que da la vuelta, puntos de control
  (secuenciales en v0), monedas y objetos, medidor, límite, rango y puntos → `Terminar`. Los luchadores son **modelos de
  B3** (Gohan niño / Trunks niño y Vegeta / Goten) en la postura de vuelo de IW (`anim::Scatto(x, kBucle, aire)` de R10,
  código 0x1D), puestos donde dice el vuelo cada cuadro y mirando hacia su marcha; cámara de `CameraFocusWhileFlying`
  (52,2 detrás, −11,25°); objetos con sus modelos y texturas de IW (`ps2::Escena`), HUD de IW (`HudIW`: TIME en cuenta
  atrás en v0 y hacia arriba en v1, cara t36/t34 y la barra = 1 − medidor) y al acabar fundido con CLEAR (334/3 t69).
  - **Medido con sonda (`DBZ3_R12_SONDA=1`):** B3 junta a los luchadores a ~150 (con el rival a 553 los movía 200 cada
    uno y el jugador quedaba detrás de la cámara) y la lógica los pega al suelo, pero el dibujo usa lo puesto en
    `Fotograma`: el rival va como mucho a 120 (ponytail: Vegeta a (1 − medidor)·120 por el rastro del jugador; Goten
    por la ruta salida → puntos de control → meta a la fracción del medidor, oculto con X+3261/X+4925 si está lejos).
  - El mundo de IW se junta al campo de R8 (±1476: escala 0,18 / 0,12; los modelos a su tamaño) y la altura es 400 ×
    escala o el terreno + 30 (mirando 60 por delante, suave).
- **Una línea en `src/iw_minijuegos.cpp`** (en `Fotograma`, como la de R13): `{ void PasoVuelo(uint8_t*);
  PasoVuelo(base); }`. Probé antes con un gancho propio en "FIX PREDRAW" (sub_820DC080): corre antes de la cámara del
  juego y la mía no se veía; quitado. Más: 2 líneas en `CMakeLists.txt`, escenario `_r12_vuelo` en `tools/pruebas.py`.
- **Prueba** (compila; `correr vuelo_test _r12_vuelo`, campos 900 y 903 de R8 copiados a `vuelo_test\mods`; capturas
  miradas):
  - 0x7D6 PASS `D:\DBZ3HD\_pruebas\20261016-071648_vuelo_test__r12_vuelo` (versión final): 19 puntos de control en
    orden, META a los 52,7 s con el medidor en 0,43, rango Z, 15200 puntos; `b_vuelo.png`: Gohan niño de B3 volando
    sobre la lava del 900 con un objeto de IW; `d_fin.png`: fundido y CLEAR. Antes: `...-070636_...\c_control.png` (el
    aro de IW de cerca).
  - 0x7E0 PASS `D:\DBZ3HD\_pruebas\20261016-071103_vuelo_test__r12_vuelo`: 29 puntos de control, META a 154 s con
    Goten en 0,925, rango A, 11000 puntos; `b_vuelo.png`: Trunks niño volando hacia el aro hexagonal sobre la llanura.
- **Pendiente / ponytail:** convertir los campos de vuelo 883/881 (R8; con ellos la escala sería 1 y saldría la casa
  real del campo); la flecha `ITEMA04_ARROW` va en el espacio de la cámara (desplazamiento en EU 0x483250 sin leer):
  apagada (`DBZ3_R12_FLECHA=1` la pinta encima del jugador); las animaciones de vuelo propias del minijuego (0x320-0x330,
  bancos DATA_CMN 915-931 por personaje) y la de vuelo de B3 (para R10: hoy es el scatto en el aire de IW); el objeto
  10; las escenas de entrada/final con el rival (guion G+0x720) y las voces ADX de 891; R1 en el teclado de pruebas sin
  confirmar (el piloto lo pulsa por dentro); los botones de ataque no se le quitan al luchador (la palanca derecha y la
  cruceta ya los quita netplay_hooks); EU sin probar (`Empezar` devuelve false con el xex EU).

### P4 Minijuegos IW, undécima vuelta (ronda 4, 07:30)
- `suelo_iw::Mover` (R11) en la marcha del modo Piccolo, en el scatto (escala 0,5) y en el extra del andar: la meseta
  para a Gohan y desliza por ella (PASS `D:\DBZ3HD\_pruebas\20261016-071329_*`).
- Bubbles de cerca: se ve entero y animado (`...-072423_*\a_salida.png`), tamaño de su #AMO (7,3), sin luces.
- Compila (07:2x).

### R11 sexta vuelta (2026-10-16, 07:30): H de IW no encontrado; radio calibrado 4 con la esfera a 8 de los pies
- **Búsqueda de H (30 min, sin resultado).** `*(char+3000)` es un bloque de parámetros por personaje: +0 escala, +8
  una altura, +0xC H, +0x10 radio luchador contra luchador, +0x18 + i por pieza (se lee en `Char_UpdateMovementPhysics`,
  en el empuje entre luchadores y en el combate). Nadie lo escribe de forma visible:
  - En el decomp no hay ninguna asignación a `+ 3000`, `0xbb8`, `[0x2ee]`, `[750]`, ni a +0x13B8 o +0x3488 desde el
    objeto global (personajes en ca70 + 0x800 / + 0x28D0, `Mg_SetupCharacters`).
  - En el ELF USA no hay ningún `sw`, `sd` ni `sq` a 0xBB8, 0xBB0 ni 0x13B8. Los cinco `addiu ..., 0xBB8` son la
    constante 3000, no el campo.
  - Busqué el bloque por su forma en los datos del ELF (+0 = 1,0, H 5-60) y no aparece. Tampoco está en la cabecera del
    `#BSK` de Goku (DATA_CMN 477).
  - Lo más probable: el puntero se reubica al cargar un archivo del personaje (como `Collision_RelocTree`) y no se
    escribe con una instrucción propia. Sin H de IW no hay escala IW → B3. El centro (+0x640) tampoco está medido.
- **Por defecto, calibrado** (ponytail en `suelo_iw.cpp`, con la medida):
  - Radio de las paredes `kRadioPared = 4`.
  - Esfera con su parte baja 8 por encima de los pies (`kBajoEsfera`; centro a pies + 12): lo que queda por debajo (el
    escalón empinado al pie de la rampa) se sube andando.
  - Medido con la réplica `E:\DBZ3HD_trabajo\R11\centro.py`: con el centro a pies + 4 u 8 la rampa no se sube; a
    pies + 12 o + 16 se sube entera y la meseta frena igual.
  - H del modelo de B3 (la cabeza) sigue dando el inicio del rayo de suelo; `Mover` usa el mismo radio por `escala`.
- **Pruebas** (carpetas en `D:\DBZ3HD\_pruebas\`; mosaico `E:\DBZ3HD_trabajo\R11\v6_final.png`), todas PASS:
  | Prueba | Carpeta | Visto |
  |---|---|---|
  | Rampa del 903 | `20261016-072422_suelo_test_iw_minijuego` | **sube entera**: y 5 → 28,5 (rellano) → 43,6 → 55,9 → 59,5 en (−190, 395), 0 paredes. `b_quieto1/2.png`: Goku en lo alto de la loma |
  | Meseta del 903 | `20261016-072646_...` | baja del montículo (32 → 0) y **se para contra la cara** en (−148, −599); `b_lejos.png`: pies en la hierba, sombra en la roca |
  | Piccolo en el 906 | `20261016-072829_...` | Gohan a la altura del suelo (4,5 / 4,8) y fuera de la roca (`c_npc.png`: delante de la pared, con la cámara cerca de ella). Paredes deslizando (24). El temblor por el `propio` de P4 sigue hasta que P4 use `Mover` |
- Compila (`tools/compilar.py`: sin errores, código 0).
- **Archivos:** `src/suelo_iw.cpp` (`kRadioPared`, `kBajoEsfera`) y `src/suelo_iw.h` (comentario de `Mover`).
  Copias de la vuelta 5 en `E:\DBZ3HD_trabajo\R11\respaldo\`.
- **Pendiente:** la H real de IW (seguir el puntero de +3000 en el cargador de personajes: buscar quién reubica los
  archivos del personaje del minijuego) y la respuesta de `Collision_SweepSphereVsModel`. Línea de P4 para `Mover`:
  ver la quinta vuelta.

#### R12 segunda vuelta (2026-10-16, 07:20-08:05)
- **Personaje, leído (no supuesto):** `Viewer_BeginMinigameLoad/Alt` eligen el 1 si el descriptor de la misión (USA
  0x5C05C0, el de `GameEngineLoadWaitEndFunc` 0x28A8D0; 0x174 B) +0x10 vale 2 (v0) o 9 (v1). Ese +0x10 lo escribe
  Dragon Mission en USA 0x2FA558: **tabla 0x47D450 [DM+0x374]** = el personaje que el jugador lleva en el mapa (0 Goku,
  44, 2 Gohan niño, 3, 4 Gohan, 6 Goten, 7 Vegeta, 54, 8 Trunks, 9 Trunks niño, 10 Krilin, 11 Piccolo...). Así que en
  IW **va Krilin (v0) / Gohan (v1) salvo que se lleve a Gohan niño / Trunks niño**. El #DMD no lo dice (su +0x10 en estas
  casillas es 10 y 19: el orden de la insignia). El mapa de R1 no pasa el personaje al módulo: va el 0, lo de IW con
  cualquier otro (`DBZ3_IWVUELO_PJ=1` para probar el otro). **Para R1:** si el mapa sabe qué personaje lleva el jugador,
  basta con pasarlo y comparar con 2 / 9.
- **Campos de vuelo reales (R8):** `CampoIW(883 / 881)` y, si da −1, el 900 / 903 de antes. Con ellos el mundo va en
  coordenadas de IW (escala 1); la vuelta del mundo la sigue haciendo el vuelo (cajas de R8 de ±8000 / ±12000 con
  paredes y sin vuelta). La casa del Gran Patriarca (883/7 + 883/8) y el objeto de 881 (7/8), que R8 deja fuera, van
  con `ps2_modelo` en la meta (ponytail: su sitio propio en el campo, por sus huesos, sin leer), y todos los objetos de
  IW con `ps2::Escena::Toon{}`. Instalados en la copia con `campos_iw.py mod 883|881 --mods D:\DBZ3HD\vuelo_test\mods`.
  - **Medido (`DBZ3_R12_SONDA=1`):** puesto a 400 de alto, la lógica deja al luchador de B3 en el suelo del escenario
    (y = 0: 883/881 no tienen `#WCF`, así que R11 no da suelo) y la cámara del juego lo sigue allí: la vista miraba
    al mar desde arriba y no se le veía (`...-074551_.../a_salida.png`). **ponytail:** en estos dos campos se vuela a
    ras del mar (suelo + 30) en vez de a 400, hasta poder mantener al luchador en el aire; en 900/903 sigue a 400 ×
    escala. Así ya se ve bien: Krilin sobre el mar verde de Namek con las islas y el cielo amarillo
    (`...-080217_.../b_vuelo.png`).
- **Flecha `ITEMA04_ARROW` encendida, leída:** `World_CharacterLoader` (USA 0x33B1E8) la pone con la matriz de la
  cámara (0x40F800) por el vector **(0, −9, −45)** de 0x482AA0: 45 por delante del ojo y 9 por debajo, a su tamaño (24
  triángulos), girada hacia el objetivo (G+0x544: v0 el punto de control visible; v1 el más cercano; la meta al final).
  ponytail: sin la inclinación −G+0x604. Comprobado: `e_botones.png` (flecha blanca abajo en el centro).
- **Botones del luchador de B3 quitados:** una línea más en `src/iw_minijuegos.cpp`, al principio de `Mando` (el camino
  de P4: `combate.cpp` sub_8208FD58 antes y después de leer el mando): `{ void MandoVuelo(uint8_t*, uint32_t, bool);
  MandoVuelo(base, usuario, leido); }`. `MandoVuelo` guarda el mando como lo leyó el juego (el vuelo usa eso: palanca,
  direcciones, R1) y deja al luchador solo START y SELECT, sin palanca ni direcciones; antes de la lectura siguiente le
  devuelve el registro como lo dejó (igual que P4). Comprobado: con L, P, Ñ y ' (golpes, ki, guardia del teclado de
  pruebas) Krilin sigue volando sin atacar. Con solo +12/+14, tres de las cuatro teclas aún le cambiaban el código de
  animación un cuadro: una sonda del registro (`DBZ3_R12_SONDA=1`, 184 B al pulsar) enseñó **otro campo de botones en
  +16/+18** y **un contador por botón en +0x28 + 4·bit** (> 0 cuadros pulsado, < 0 desde que se soltó; el +92 de P4 es
  el del círculo, bit 13). `MandoVuelo` también los guarda, los quita (contador a −1, como P4) y los devuelve: con eso
  ninguna tecla le cambia el código (`...-073630_...`: sin `anim: luchador` al pulsar; `e_botones.png`).
- **Animación de vuelo:** el scatto en el aire de IW de R10 (0x1D) se ve bien (cuerpo tumbado hacia delante, como al
  volar). No lo he cambiado por uno de B3.
- **Para R13, el 3 (ladrillos, `# SHOOT #`), leído del ELF USA:** **no es** mi módulo de vuelo (`# FLIGHT #`, Viewer_*).
  Es otro con el mismo esquema de carga: registro USA 0x369280, cargador USA 0x3692D0 (EU 0x3691A0
  `"DestroyTheBricks"Minigame`), estado `*(0x49CEAC)` EU (0x700 B): +0x4B4 variante (0), +0x4B8 personaje, +0x4BC
  dificultad (descriptor +0xC). Personaje por el mismo descriptor +0x10 (EU 0x369370): 0xB Piccolo → 3, 0xD Yamcha → 2,
  0xC Ten → 1, si no 0 (Goku). gp de USA = **0x4A35F0**. Ficheros: gp−0x78FC = **DATA_CMN 897** (planeta de Kaio, ya lo
  convierte R8), gp−0x78F8 = **890** (#AMB: `ITEMB03_BRICK` 1/0, `ITEMC01` 3/2, **`#MSS` 4** (10748 B), #AMC 5, efectos
  6, ADX 7-13), gp−0x7900 = 915 (banco de animaciones, el mismo que carga el vuelo), DATA_USA 9 (0x10009, HUD). El
  **#MSS** tiene la misma idea que mi #MSF: +0x10 10.0, +0x14 180, 15, 15, 90, 5, 5; +0x2C..+0x40 **6 bloques por
  dificultad** (0x44, 0x738, 0xE2C, 0x1520, 0x1C14, 0x2308). Bloque 0: 60, 0, 18.0, 115.0, 0, **umbrales 7 6 5 4 3**
  (aciertos para Z..C), **puntos 3600 3000 2500 2000 1400 y 100** (fallo), luego 30.0, 100.0, 0, 35.0, 0, 45.0, 0, 60 y
  registros de 7 floats (1.0, 320, 200, 1000, 620, 185, 50) que parecen las trayectorias de los ladrillos. Volcado:
  `E:\DBZ3HD_trabajo\R12\mss890.txt`. Lo que leí del vuelo que también sirve: el mando `Viewer_ReadControlAxes` (zona
  muerta 64, (v ∓ 64)/64; +0 palanca derecha x, +2 izquierda x, +3 izquierda y en 0x4ADED8 + 0xBC·pad) y el cuadro = 1
  (fRam0049C7DC). Las tablas que R13 pedía son las del #MSS, no las mías.
- **Prueba** (compila; capturas miradas): a mano `D:\DBZ3HD\_pruebas\20261016-072605_vuelo_test__r12_vuelo` (Krilin, W
  vuela y D gira; flecha blanca en `e_botones.png`); con los botones del todo quitados
  `...-073630_vuelo_test__r12_vuelo` PASS. Piloto `...-072830_vuelo_test__r12_vuelo` PASS: Krilin contra Vegeta, META a
  52,7 s, rango Z, 15200 puntos (igual que la primera vuelta). En los campos reales: 7 (campo 883, escala 1)
  `...-080217_vuelo_test__r12_vuelo` PASS, Krilin, META 52,7 s, rango Z, 15200; 17 (campo 881)
  `...-080605_vuelo_test__r12_vuelo` PASS (ya a ras del mar), Gohan contra Goten, META 154 s con Goten en 0,925, rango A,
  11000 puntos; `b_vuelo.png`: Gohan volando sobre el mar de la Tierra hacia un aro, con la flecha.
- **API para R13 (`src/iw_mini_vuelo.h`, `namespace dbz3::iwvuelo`; implementado al final de `iw_mini_vuelo.cpp`, solo
  US):**
  - `LeerMsf(bytes, dificultad, Msf&, error)`: el #MSF entero (cabecera de vuelo, bloque de la dificultad, salida, meta,
    listas con su radio de recogida, medidor, rangos y puntos). Lo usa el propio vuelo.
  - `Bloques(guion, en, n = 6)`: los desplazamientos de los bloques por dificultad de un guion de minijuego (#MSF:
    `en` = 0xA8; el #MSS de 890: 0x2C).
  - `EjeIW(v)` (palanca como `Viewer_ReadControlAxes`) y `Envolver(d, periodo)` (mundo que da la vuelta).
  - `CamaraVuelo(pie, yaw, ojo, obj, alto = 8, dist = 52.2, cabeceo = -0x800)`: la cámara de `CameraFocusWhileFlying`
    (avanza con el personaje); se pone con `cine::Camara`.
  - `DelanteDeCamara(base, adelante, abajo, out)`: un punto en el espacio de la cámara (la flecha: 45 y 9).
  - `Proyectar(base, ancho, alto, p, x, y, px_por_unidad)`: mundo → pantalla con la cámara del combate
    (`ps2::CamaraCombate`).
- **Archivos:** `src/iw_mini_vuelo.cpp`, nuevo `src/iw_mini_vuelo.h`; una línea más en `src/iw_minijuegos.cpp`
  (`Mando`); `tools/pruebas.py` (`_r12_vuelo` pulsa los botones de golpe en `auto=0`).
- **Pendiente:** que el mapa pase el personaje (R1); mantener al luchador de B3 en el aire a 400 en 883/881 (o que esos
  campos tengan suelo); el sitio propio de la casa y del objeto de 881; la inclinación de la flecha; las animaciones
  propias del vuelo de IW (0x320-0x330) si se quieren en vez del scatto.

### R9 quinta vuelta (2026-10-16, 07:40): Baby
- **Medida.** El cierre (`lr 0x820CFF1C`) lo da una orden del guion de la definitiva de Vegeta: `sub_82128A38`,
  parámetros p[0] índice, p[1] banco, p[2] lado y p[3] objeto de efecto. Llama a `sub_820CFEC8`, que usa el campo
  +148 del objeto de efecto `0x823A6250 + 8512*p[3]`.
  - En los dos personajes la orden es la misma: **efecto 13 del banco 1, lado 0, objetos 0 y 1**.
  - Con Vegeta del juego el objeto ya está preparado (+148 = `0x4764E160`, `...\20261016-064555_ports_test_combate_port`
    y `-065006`). Con Baby está sin preparar (+148 = 0, `-065331`).
  - El único que escribe ese +148 con esa tabla es `sub_820CFE18`, y no se llamó en ninguno de los dos combates. No he
    localizado qué prepara el objeto.
  - Probé a saltar la orden cuando +148 = 0, y la siguiente orden sobre el mismo objeto (manejador `0x82129A90`,
    `lr 0x82129ADC`, `-070222`) también cerraba el juego. Lo quité (no se arregla orden a orden).
- **Arreglo en el montaje** (`mod center hd/roster_build.py`, `IW_TECNICAS_DONANTE = {"c:super baby vegeta 2"}`).
  Un port de IW de esa lista usa el BSP de técnicas del donante aunque su receta traiga uno propio, como ya hacía
  Yamcha amarillo con `tecnicas = "donante"`. Con el de Vegeta la definitiva se prepara como en el juego. Se aplica a
  los mods ya importados al volver a montar, sin tocar su receta. Comprobado en el roster de `ports_test`: Baby se
  monta sin línea `tecnicas` (usa el BSP del donante) y Goku GT conserva el suyo.
  - Coste: los especiales de IW de Baby usan los recursos del BSP de Vegeta. En las capturas sus técnicas salen sin
    cerrarse, pero puede que con el aspecto de las de Vegeta.
- **Pruebas** (Baby llevado por el COM con sus cápsulas y ki lleno contra Krilin, `combate_ports.py`):

| Pasada | Montaje | Definitivas | Resultado |
|---|---|---|---|
| `20261016-071649` | receta de la copia con `tecnicas = "donante"` | 8, dos enteras de 19-20 s | PASS, sin cierres. Final Flash con Baby (`051_com.png`: Baby en la cinemática; `054_com.png`: rayo y «DEFENSE WIN») |
| `20261016-072134` | igual | 3 | PASS, vuelta al menú |
| `20261016-073111` | **regla del montaje** (receta original repuesta) | 6, enteras de 19 s | PASS, sin cierres, vuelta al menú |

- **Rótulo con el nombre de la definitiva:** no ha salido en ninguna de las capturas, que se hacen cada 3-4 s. No
  puedo confirmar qué nombre muestra; hay que mirarlo jugando. Como el guion es el del donante, la llamada lleva la
  cápsula del donante y el nombre podría ser el suyo (p. ej. «Final Flash» en Baby).
- **`MotivoIW`:** **el 45 (Baby) ya se puede desbloquear.** Antes, cada copia monta de nuevo su roster con `prep`
  porque `roster_build.py` cambió.
- **Archivos:**
  - `mod center hd/roster_build.py`: `IW_TECNICAS_DONANTE` y su uso al leer las técnicas.
  - `src/ports_guard.cpp`: medidas temporales puestas y quitadas; queda como al final de la cuarta vuelta. Copia en
    `E:\DBZ3HD_trabajo\R9\ports_guard_final.cpp`.
  - `src/eu_map_gen.h` y `docs/14_frentes/eu_mapa.txt`: regenerados.
  - Esta sección. Compila (`tools/compilar.py`, 07:11).
  - La receta de Baby de `ports_test` se tocó solo para la prueba y quedó repuesta (respaldo en
    `E:\DBZ3HD_trabajo\R9\respaldo_baby\`).
- **Pendiente:** qué prepara el objeto de efecto de la definitiva. Hace falta para que Baby pueda usar su propio BSP
  de IW: buscar la escritura de +148 por otra vía, por ejemplo una copia de plantilla con watch de escritura.

### P4 Minijuegos IW, duodécima vuelta (ronda 4, 07:45)
- **Toon en `ps2_modelo` (opcional):** normales leídas y animadas; `Escena::Poner(..., Toon)` con 3 tonos (luz de B3)
  y borde negro (caras de detrás empujadas). Todos los modelos de IW de P4 lo usan. **Para R2:** pasar un
  `ps2::Escena::Toon{}` en las figuras del tablero y el Shenron.
- Bubbles con tonos y borde negro (`D:\DBZ3HD\_pruebas\20261016-073329_*`); aro con sombreado, borde apenas visible
  (`-073734_*`). Compila.

### R8 sexta vuelta (2026-10-16, 07:50): campos de vuelo 881 (Tierra) y 883 (Namek)
- **Añadidos a `mod center hd/campos_iw.py`** (tabla `CAMPOS`, paso `auto` de JUGAR; `VERSION` 9: se rehacen solos):
  cielo y nubes de IW pegados al terreno, niebla del #MCF (881 `87FFFFFF,2400,3700`, 883 `F8FC66FF,1300,3000`),
  caja de colisión hasta el borde del terreno (881 ±12000, 883 ±8000; el mundo de IW da la vuelta a 24000/16000: aquí
  no da la vuelta, son paredes). No tienen #WCF (vuelo a altura fija 400): el suelo da igual. Las piezas 7/8 (881: un
  objeto animado con #AMM; 883: **la casa de Guru**) no van en el escenario: las pone el minijuego en su sitio, como la
  nave de Raditz de P4 (R12: 883/7 + 883/8, modelo y texturas).
- **Entradas fijas** (`ENTRADAS`): 884 5060, 897 5061, 900 5062, 903 5063, 906 5064, 909 5065, **881 5066, 883 5067**
  (un campo nuevo ya no mueve las demás). `dbz3::escenarios::CampoIW(campo)` busca ahora el alta por su carpeta de mod
  (`Campo IW <campo>`), sin lista fija en C++: los campos que añada R13 funcionan sin tocar el código del juego.
  Compila (`tools/compilar.py`: sin errores).
- **881 necesita partirse:** su terreno (7,9 MB, 348 huesos) pasa del límite de índices de 16 bits de un `#AWG` de la HD
  (`amo2awo`: 103687 ventanas). `partir()` hace 2 copias con la mitad de las partes cada una (truco de las partes
  ocultas) y van en 2 sub-mallas; por eso 881 y 883 usan el escenario base **IW 52 (= B3 48)**, que tiene 4 sub-mallas
  libres (Llanuras solo 1). Las sub-mallas que sobran pintan el modelo de colisión (oculto) **cada una con su propia
  sub-malla**: repetir la misma en la lista la reubicaba dos veces (cierre al cargar, `lr 0x82080148`, arreglado).
- **Cámaras de presentación:** sin #AMC propio (la entrada siguiente de los campos de vuelo no es #WCF + #AMC). Probadas
  y vistas a ojo: las de Llanuras daban 2 planos de solo cielo en 883; las prestadas de 903/900 metían la cámara bajo
  el agua (881) o en una meseta (883); **las del base IW 52** dan tierra, costa y mar (881) y el paisaje de Namek con
  sus árboles (883). Quedan esas. `CAMARAS = {}` permite prestar el #AMC de otra entrada si hace falta.
- **Pruebas (combate del HUB en el alta, presentación completa, capturas revisadas a ojo):**
  - 881: `D:\DBZ3HD\_pruebas\20261016-073145_stage_test3_r8_intro` PASS (mosaico
    `E:\DBZ3HD_trabajo\R8\intro_881_base52.png`: pradera con mesetas, costa, mar abierto, cielo y nubes de IW; Vegeta y
    Goku sobre el mar a y = 0). Antes del arreglo de la sub-malla repetida: `...-072340_stage_test3_r8_intro` (cierre).
  - 883: `D:\DBZ3HD\_pruebas\20261016-073744_stage_test_r8_intro` PASS (mosaico `intro_883_base52.png`: cielo verde de
    Namek, mesetas, agua y árboles). **ponytail:** los 2 planos de cerca de los luchadores (4.º y 5.º) quedan dentro de
    una meseta (el terreno de 883 está por encima de y = 0 donde salen; sin #WCF no lo arregla R11); en el combate el
    luchador sí sale sobre la hierba (`06_combate`). Con Llanuras de base salían bien esos planos pero no los generales
    (`...-072340_stage_test_r8_intro`).
- **Para R12 (vuelo):** `CampoIW(883)` (Namek, variante 0) y `CampoIW(881)` (Tierra, variante 1) en lugar de 900/903
  (`src/iw_mini_vuelo.cpp`, línea de `EmpezarCombate`: `CampoIW(j->v == 0 ? 883 : 881)`, con 900/903 si devuelven -1).
  Campo en coordenadas de IW (raíz sin mover), paredes en ±8000 / ±12000.
- **Para R13 (disparo):** aún no ha dicho qué campos usan los MSN009/010/016 (su sección: "Stage_BeginLoad de un campo
  del contexto, sin identificar"). Añadir uno = una línea en `CAMPOS` (+ su entrada en `ENTRADAS` y, si su #AMC no
  está en la entrada siguiente, en `CAMARAS`); el juego lo encuentra con `CampoIW(<campo>)` sin compilar nada.
- **Archivos:** `mod center hd/campos_iw.py` (881/883, `ENTRADAS`, `PARTES`/`partir`, `CAMARAS`, sub-mallas sobrantes
  propias, `VERSION` 9), `src/escenarios.{h,cpp}` (`CampoIW` por carpeta). Respaldos en `E:\DBZ3HD_trabajo\R8\respaldo\`
  (`q6*`, `v4pass\`).

#### R13 segunda vuelta (2026-10-16, 07:18-07:58)
- **15 (0x7DE) "Tien! Stop Cell!" hecho de extremo a extremo.** RE de la IA de Cell (decomp EU 0x31C400-0x31DF30 y sus
  tablas leídas del ELF EU `modding resources discord\research\SLES_553.47.47`: entrar 0x4996B0, al acabar la animación
  0x4996E0, cada cuadro 0x499710; las registra `s_PLY_PRE_EXEC`, no `Char_UpdateAllPerFrame`):
  - **Cell no sube por animación sino por velocidad:** estado 3 (`Char_EnterPoseState3Alt`) pone vy = campo +0x10 de
    la dificultad {3,0; 3,0; 3,5; 3,5; 4,0; 4,0} y su tick (0x31CE60) suma vy **cada cuadro sin gravedad** desde la 2.ª
    animación (0x657 → 0x658 → 0x659): llega a Ten (y = 400, el ojo de la cámara, `UiAnim_SetPos(0, 400, 200)` en el
    controlador +0x84) en 100-134 cuadros.
  - **Kikoho:** si la posición de Cell proyectada cae en el rombo (300,147)-(403,250)-(300,353)-(197,250) (EU 0x480A10,
    `Char_IsProjectedInQuad`), estado 4: vy = −0,08 (o −0,08 más si ya bajaba) y cae con −0,06 por cuadro
    (`Char_PoseTickRise`) hasta y = 0,1; estado 5 (aterriza) y, tras altura del golpe / 400 × 60 cuadros, 6 (se levanta);
    luego 5 marcas al azar (x −140..89, z −90..179, tipo 0..9, paso, 32-159 cuadros): si la 1.ª es de tipo 0 vuelve a
    subir; si no, corre entre ellas a 3 por cuadro (campo +0xC; llega a menos de 30), se para (estado 2) y sube.
  - **Corrección de la 1.ª vuelta:** la tabla por dificultad empieza en USA 0x47D860 (no 0x47D880): tiempo {40, 60, 80,
    100, 120, 160} s, energía 1, divisor del rango 1 (p = altura / 400), +0xC = 3, +0x10 = subida, multiplicador {0,6 …
    2,0}. En la espera de energía baja Cell se para (`Store5d94c0Pair(0, 0)`).
  - Port: Ten (12) oculto con la cámara de IW (ojo (x, 400, z), mira trasladada hacia (x, 15, z − 200), x −111..74 y
    z 60..280 a 1,5 por cuadro), Cell = el **Cell de B3** (33) movido por esa IA, con la carrera de IW de R10
    (`anim::Corredor`) mientras corre; manos de Ten = `ITEMB00` (DATA_CMN 895, en el sitio de `CameraCtrl_Create`); el
    Kikoho = las mallas del paquete del minijuego (935/1) desde las manos; chispa al acertar; HUD 9/1: aro con los segundos,
    cara de Ten (t22) y su barra de energía, arco t09 + escala t07 con el triángulo t08 a la altura de Cell, SAFE/DANGER
    (t10/t11 al 50 %) y el rombo (t17). Final por tiempo (rango por p, {3600..6000} × mult) o derrota (energía ≤ 0, Cell
    a 400): CLEAR / fundido como el 8 y el 9.
- **Escenarios (pregunta del coordinador):** no son campos de IW. Cada constructor fija el escenario de combate de IW:
  MSN016 **ISLAND (2)** (`Menu_CreateContext` EU 0x30D11C), MSN010 y MSN009 **NAMEC (7)** (`Viewer_AllocContext`), que
  `Stage_BeginLoad` abre por la tabla 0x44ABC0 (mapas 60 y 64 de DATA_CMN): son los escenarios que IW comparte con B3, así
  que se usan los de B3 (Archipiélago 2 y Namek 7), sin `campos_iw.py`. Los campos 881/882/883 (#MCF sin #WCF) son del
  vuelo (R12: 883 Namek de vuelo, 881 Tierra).
- **Efectos (sin formas de relleno):** el paquete de efectos de los cuatro minijuegos de disparo es **DATA_CMN 935**
  (`Effect_OpenBundleForMode`, USA 0x347C10: modo 15/9/8/3 → 0x3A7): atlas 935/0, mallas `MODEL_L00/L01/L03_EFT` (935/1,
  conos de 25 × 120) y **21 `#AME`** (guiones de partículas, formato sin leer; el 3 usa una copia, 890/6). Proyectiles con
  `ps2_modelo`: en el 9 el rayo rosa de Freezer (842/7 `BEAM` + `CORE`, texturas 842/8) orientado a su marcha; en el 8 el
  Kienzan (851/6 `KIENCORE`, plano y girando; texturas 851/2), también cargándose sobre la mano; en el 15 el cono de 935/1.
  Icono de discos = textura del Kienzan (851/2 t00). Quitadas la elipse, la línea y la bola de relleno. ponytail: los
  `#AME` (animación de UV, partículas) no están portados, así que el cono del Kikoho se ve tenue y los rayos y el disco
  van sin estela; la escala del rayo es a ojo.
- **Manos de Krilin (8):** el MSN009 **no carga manos**: sus carpetas de DATA_CMN son solo 873 (`#AMC`, cámara; USA
  0x34B04C) y 935; el que lanza es el propio luchador de IW en sus poses 8/9/10. Todos los `ITEMB*` de DATA_CMN: 00 Ten
  (895), 02 radar, 03 ladrillo (890), 04/05 Freezer (892), 06 roca, 08 rama. En el 8 no se pintan manos.
- **Alfa de PS2:** las texturas de las manos y los proyectiles traen alfa 0x80 = opaco (escala de PS2) y salían a medias;
  se dobla al cargar en `iw_mini_disparo.cpp`. **Para P4:** debería hacerlo `P::TexturaAmt` / `ps2::Crear` para todas las
  texturas de PS2 (hoy solo se arregla el caso de alfa 0).
- **Para R12 (ladrillos, 3):** su carpeta es **DATA_CMN 890**: 890/1 `ITEMB03_BRICK` (+ texturas 890/0), 890/3 (#AMO
  pequeño), **890/4 `#MSS`** (guion del tiro, hermano del `#MSF` del vuelo; cabecera 10.0, 180, 15, 15, 90, 5, 5, …,
  1848/3628/5408/7188/8968, …), 890/5 `#AMC` (cámara), 890/6 efectos (= 935), 890/7-13 siete entradas `80000024`. Lo
  que necesito de vuestro motor de vuelo: (1) el lector del `#MSS`/`#MSF` (tabla de etapa B: N tramos, umbrales de
  rango +0x14, premios +0x28, cámara +0x40..+0x5C, blancos +0x64 + 0x1C i con velocidad e inicio/fin x, y, escala); (2) el
  avance de cámara/escenario (`MgFly_UpdateStageByCameraDelta`) y la proyección (`MgFly_ProjectToScreen`, radio de
  fijado). Con eso el 3 es: mira 2D, fijar al pasar cerca (12 cuadros), círculo = romper, 7 disparos, rango por aciertos.
- **Archivos:** `src/iw_mini_disparo.cpp` (registra también 0x7DE); escenario nuevo `E:\DBZ3HD_trabajo\R13\r13_cell.toml`
  (`r13_freezer.toml` con capturas del disco en carga y en vuelo).
- **Pruebas (compila 07:57; capturas miradas):**
  - 15 con piloto automático: `D:\DBZ3HD\_pruebas\20261016-074716_disparo_test_r13_cell` PASS, 80 s con Cell contenido
    → S, 5400 (`b_juego`/`c_kikoho`: vista desde arriba por el triángulo de las manos de Ten, Cell abajo, SAFE);
    `...-075053_...` PASS con derrota ("Cell llega a Ten", DANGER, 100 puntos): los dos finales funcionan. El piloto
    falla a veces porque la mira va a 1,5 por cuadro y Cell sube a 3,5.
  - 9: `...-075425_disparo_test_r13_piccolo` PASS (derribo 32,4 s, S, 8000; rayo rosa de IW y chispa); 8:
    `...-073000_disparo_test_r13_freezer` PASS (Kienzan de IW cargándose sobre la mano; en vuelo `Escena` no pinta
    lo que está a menos de ~19 de la cámara, así que se ve poco; 7 aciertos, Z).
- **ponytail (15):** duración de las animaciones de Cell de IW (0x654…0x66C) no leída (20-30 cuadros); mirada de la
  cámara trasladada con el ojo (el controlador guarda el objetivo en +0x90, sin leer quién lo mueve); el Cell de B3 flota
  en su pose de salto; el rombo se mide con la posición de los pies, como IW.

### P4 Minijuegos IW, decimotercera vuelta (ronda 4, 08:10)
- **Shenron con toon arreglado:** con la cámara sin profundidad de R2, el borde iba por `AddTriangleFilled` entre
  triángulos de vértices empaquetados (R15) y salían abanicos; ahora va por el mismo camino que su triángulo. Shenron
  con tonos y borde negro: `D:\DBZ3HD\_pruebas\20261016-080505_*` (prueba con `DBZ3_TEST_SHEN_TOON=1`).
- Normales basura → "sin normal"; vértices no finitos fuera; tope de 65535 vértices por lista (quita bordes primero).
- Aros, monedas y nave de IW sin normales (vtype sin el bit 1): sin borde, como en IW. Plano cercano: quitado el corte
  a ~19 de la cámara (R13). Alfa: `TexturaAmt` ya hace ×2 (R13: no repetirlo; pintar opacas las de personaje).
- **Para R15:** el aro de cerca se pinta encima de Goku estando detrás (`-081020_*`).

#### R13 tercera vuelta (2026-10-16, 07:58-)
- **3 (0x7D3) ladrillos con el Kaio, hecho de extremo a extremo** (con lo que leyó R12 del ELF USA y el #MSS):
  - Datos de **DATA_CMN 890**: ladrillo `ITEMB03_BRICK` (890/1 + texturas 890/0) y el **#MSS 890/4**, leído en marcha
    (bloque de la dificultad en +0x2C + 4·d): N ladrillos {60, 50, 40, 30, 30, 15}, recorrido de 115 por tramo, umbrales
    de aciertos {7, 6, 5, 4, 3} (Z..C), sus puntos (3600..1400 en Very Easy … 18000..7200 en Z; fallo 100), cámara (0,
    30, 100) → (0, 35, 0) a 45° y 60 trayectorias (velocidad 1; inicio x, y, profundidad → fin, en la pantalla de PS2 de
    640 × 448).
  - Reglas (motor `MgFly_*` del 3, EU 0x366480-0x369370): mira 2D con la palanca o la cruceta; el ladrillo se **fija**
    si el recorrido de la mira en ese cuadro pasa cerca y está a ≤ 700 de profundidad; luego la mira va 12 cuadros
    hacia él y se queda encima. Círculo = disparo (**7**): fijado = roto (chispa, siguiente a los 30 + 90 cuadros); si se
    escapa, el siguiente a los 60. Sin tiempo: acaba con los N tramos o los 7 disparos; rango = primer umbral ≤ aciertos.
  - Port: **Goku de B3** en tercera persona (el luchador que se ve; Piccolo/Yamcha/Ten según el personaje que lleva el
    mapa, que R1 no pasa aún) con el rival oculto delante; **campo de IW 897** (planeta del Kaio de R8, con su casa) si
    está instalado, si no Llanuras; la cámara del #MSS; el ladrillo de IW con `ps2_modelo` en su punto (pantalla de PS2 →
    mundo a esa profundidad por la cámara); HUD 9/1: mira (anillo / cruz fijado), estrellas de rotos (t19 + t20), disparos
    que quedan y ladrillos que quedan en el aro azul. Final CLEAR + rango y puntos del #MSS.
  - Prueba: `D:\DBZ3HD\_pruebas\20261016-080157_disparo_test_r13_ladrillos` PASS (piloto: 7 de 7, Z, 9000 en Normal;
    campo 897 de R8 con la casa del Kaio, Goku de espaldas, mira y estrellas) y `...-080952_disparo_test_r13_ladrillos_ver`
    (sin piloto: el ladrillo de IW cruzando el cielo, `a_ladrillo1`/`c_ladrillo3`).
  - ponytail: la velocidad de la mira (tabla A +0x10 del motor de vuelo) no está leída: 6 px por cuadro; el radio de
    fijado (ctx+0x514, fórmula con datos sin leer) = el tamaño del ladrillo en pantalla; el ladrillo se pinta macizo
    (`trans = false`: el bit alto de su vtype lo dejaba casi invisible).

#### R14 segunda vuelta (2026-10-16, 07:35-08:30)
- **Los 6 minijuegos de botones van de punta a punta** (`src/iw_mini_botones.cpp`): familia A (5, 12, 13) + familia B
  "rondas" (11 NB012 y 6 NB007) + 18 MSN019 (Videl). Pruebas PASS con capturas miradas (abajo).
- **Iconos de B3 HD** (la HD cambió los del mando de PS2 por los de la 360): los de la tira LB LT RB RT Y B A X
  (data_usi 2062, los de `style::Glyph`), triángulo → Y, círculo → B, equis → A, cuadrado → X, L1 → LB, R1 → RB,
  L2 → LT, R2 → RT; la cruceta de NB007 con la flecha de los menús (`flechas`) girada. Lo demás de IW: objetivo = aro
  rojo y amarillo con la flecha (DATA_USA 0/1 t10), READY?/START!/HIT!/MISS!/CLEAR!/FAILED! (5/1), GREAT!/GOOD!/
  NORMAL/BAD (4/0 t15/t08/t09/t10) para los juicios de la familia B.
- **"80 DMG.", "2 HITS!!" y "FIRST ATTACK!!" ocultos:** no son del marcador sino tareas propias (nombres de la imagen
  del juego): "[FIX]DMG %dP" + "DMGDRW %dP" (sub_820DF248), "[FIX]CMB %dP" + "CMBDRW %dP" (sub_820DF330) y los avisos
  "[MSG]FATK/CNTR/GBREAK %dP" que pinta "[MSGDRW]ID=%d" (sub_820DEAC8). `OcultarHud` salta ahora también
  `sub_820DDCA0` y `sub_820DDFE8` (DMGDRW/CMBDRW), `sub_820DDC20` y `sub_820DDF68` (el segundo callback de cada [FIX],
  que lo pinta mientras se desvanece: con solo los dos primeros seguía saliendo "48 DMG." del lado 2) y `sub_820DE490`
  (MSGDRW). Ganchos en `src/combate.cpp` junto a FIX DRAW; parejas EU con `tools/eu_map.py`. Vale para todos los que
  usan `OcultarHud` (P4, R3). **Queda:** tras la patada del rival aún se ve a veces "48 DMG." pequeño del lado 2
  (`D:\DBZ3HD\_pruebas\20261016-082331_botones_test__r14_botones\c_iconos.png`); el del jugador y "FIRST ATTACK!!" ya no
  salen. Siguiente paso: registrar qué función lo pinta (candidata, la tarea "[PLY] HIT EFCT"; no se toca a ciegas porque
  también lleva las chispas de los golpes).
- **Carriles:** IW no tiene pieza de carril entre las de minijuegos (0/1, 0/8, 4/0, 5/1, 9/1 miradas); se quedan las
  bandas oscuras (ponytail) con el objetivo de IW encima.
- **Personajes y escenarios leídos del módulo** (bloque de partida que escribe cada `Mg_AllocContexts`: +0x10/+0x12
  personajes, +0x14/+0x16 formas; escenario de su `w_Stage_BeginLoad`; ID de IW = ID de B3 hasta el 43):
  5 NB006 Gohan niño (2) contra Piccolo (11), escenario 4 ROCK; **12 NB013 Vegeta SSJ (7, forma 1) contra Trunks (8)**,
  escenario 1 MIND & TIME (la Cámara del Tiempo de B3); **13 NB014 rival Gohan adolescente (3)**, escenario 1, el jugador
  no lo escribe el módulo (queda el del mapa: Goku); 11 NB012 el del mapa si es 7, 12, 10 o 13, si no Vegeta (7),
  escenario 4 (12 con Krilin o Yamcha); 6 NB007 Goku (0) solo. **Cara de Vegeta:** t33 es Vegeta SSJ (y t34 Trunks SSJ):
  IW solo trae esas, y ahora salen justo en el 12, que es Vegeta SSJ contra Trunks; el 13 lleva Goku (t29) y Gohan (t36).
- **El rival golpea al fallar el carril 1 (aprobado):** `combate::SetMuneco(3)` = nuevo modo público "ataca" (el que ya
  había dentro, patada cada 12 cuadros con el mando virtual); el minijuego lo pone 14 cuadros y vuelve a 1. Además
  `SetMuneco` ya no registra si el modo no cambia. Probado: `muneco ataca` / `quieto` en el registro y la vida del
  jugador baja (prueba de 12 con `--par fallos=1`).
- **Familia B, NB012 (11, captura 88):** 10 rondas con orden barajado (5 de las filas 0-9 y 5 de las 10-19, como
  `Mg_ShuffleRoundOrder`); por ronda N botones (3..9; la última 4..10) de la tabla del ELF del jugador (0x47DB10 USA) con
  sus separaciones (0x47DFF0); un carril en y 395 a 1,5..6,5 px por cuadro; juicio por solape con el objetivo
  (`Mg_JudgeIconOverlap`: dentro 0, < 25 % 1, < 50 % 2, si no 3) → la ronda suma 100/70/50 dividido (entero) por N; lista
  1-10 a la derecha con la nota de cada ronda; rango por la suma (901/801/751/601/301) y puntos de 0x47F2D0.
  Un solo luchador: el rival es el pivote invisible de P4 (Raditz con X+3261/X+4925).
- **Familia B, NB007 (6):** otra mecánica (`MiniGame_*` EU 0x2928C0-0x295C10): la fila de N botones (3..9, la última
  4..10) está quieta y se teclea en orden lo antes posible; el botón malo no avanza; tiempo de cada ronda en la lista;
  rango por la suma de los 10 tiempos (<= 25/35/40/45/55 s) y 0 si las 10 rondas pasan de 7 s; tablas 0x472F30 (USA) con
  la **cruceta** (códigos 4-7) desde la dificultad 1: se lee de la dirección de la palanca en +108 (la cruceta de la HD
  llega como palanca en los minijuegos; +12 no lleva direcciones, sonda de teclado).
- **18 MSN019 (Videl):** todo constantes de `Minigame_InitConfigTable`: 10 filas; N objetos (4 5 7 8 9 10) en huecos de
  una barra centrada (0,5..0,9 del ancho, con azar: `Results_RespawnItem`) con un botón al azar de la máscara de la
  dificultad (F000 / FC00 / FF00); un cursor rebota de punta a punta (2,0..5,2 px por cuadro) y se pulsa el botón de
  cada objeto mientras lo pisa; la fila acaba al cogerlos todos o pasados 7 rebotes; nota por rebotes (<= 2 → 10,
  <= 4 → 5, <= 7 → 3, si no 0; `Results_StateEvaluate`); rango por 60/70/80/90/100 y premio 6500..7700 ×
  multiplicador (redondeo a la centena más cercana, `Results_ComputeTotalsAndRewards`).
- **Pruebas** (`correr botones_test _r14_botones --par clave=...`; auto pulsa por el mando virtual):
  | Clave | Carpeta (`D:\DBZ3HD\_pruebas\`) | Resultado | Visto |
  |---|---|---|---|
  | 0x7DB `fallos=1` | `20261016-074334_botones_test__r14_botones` | 90/100, vida 550, rango 4, 12700 | Vegeta SSJ y Trunks en la Cámara del Tiempo, botones Y/B/A/LB/LT, aro de IW, sin "DMG" |
  | 0x7DA (NB012) | `-074831_...` | 10 rondas (100 ×9, 96), total 996, rango 5, 6700 | Vegeta solo en el escenario rocoso, lista 1-10 |
  | 0x7D5 (NB007) | `-080016_...` | 10 rondas de ~1,1 s, 11,3 s, rango 5, 6200 | Goku, lista de tiempos |
  | 0x7E1 (MSN019) | `-080358_...` y `-080841_...` | 10 filas: 90 (una fila a 0: el piloto perdia un objeto; arreglado) y luego 100, rango 5, 7700 | Videl sola en la llanura, la barra con Y/A y el aro de IW como cursor, lista 1-10 |
  | 0x7DC `fallos=1` | `-081150_...`, código final `-082331_...` | 68/75, vida 755, rango 4, 10100 | Goku contra Gohan en la Cámara del Tiempo, READY?, CLEAR!; "muneco ataca" en el registro |
  | sonda de teclas | `-075845_...` | +12 = bits de PS2 de los 8 botones; palanca y cruceta no llegan a +12 | |
- **Pendiente / ponytail:** la x del objetivo y el ancho de su caja (objeto 0x20 de la UI) sin leer; la lista 1-10 va
  con la letra del menú de B3 (sus cifras de 4/0 sin colocar); la segunda tabla de NB007 en dificultad 3-5 (0/1/2 por
  botón, 0x473410...) sin leer; el escenario de NB007 es la nave de IW (DATA_CMN 880, modelo de IW: va el 0 BUDO) y el de
  MSN019 sin leer (va el 5 PLANE); Videl no sube con el medidor de vuelo (`UpdateProgressGauge` → +0x205C); IW no trae
  cara de Videl en 6/0 (va la de Gohan); el pivote invisible deja ver su polvo; la animación de victoria/derrota de IW y
  el cansancio de NB007 (el "stun" tras un error) no están.
- **Archivos:** `src/iw_mini_botones.cpp`; `src/combate.cpp` y `src/combate.h` (modo 3 "ataca" de `SetMuneco` y los 2
  ganchos de DMG/CMB bajo `OcultarHud`); `src/eu_map_gen.h` y `docs/14_frentes/eu_mapa.txt` regenerados con
  `tools/eu_map.py`; `tools/pruebas.py` (`_r14_botones`: `--par fallos=1`, `--par seg=N`, sonda `teclas=1` con
  direcciones). Compila (`tools/compilar.py`: sin errores).

### R15 · Modelos propios con el z-buffer real del juego (2026-10-16, 08:30)
- **Ingeniería inversa (SDK):** la capa ImGui la pinta `D3D12ImmediateDrawer` dentro de `PaintAndPresentImpl` del
  presentador, sobre la imagen ya escalada. El z-buffer del juego ya se copiaba para FSR 3/DLSS (`Dbz3TemporalOnCopy`:
  el último resolve de profundidad a pantalla completa de la escena, R32 a la resolución interna, que viaja con cada
  imagen como `Dbz3TemporalInputs::depth`), pero solo con `present_effect` fsr/dlss y sin MSAA (el MSAA 2x nativo está
  encendido de serie: sin copia).
- **Hecho (SDK):** cvar `dbz3_ui_depth` (lo pone el juego): copia la profundidad en cada cuadro aunque no haya FSR (sin
  jitter), también con MSAA (muestra 0, `dbz3_depth_copy_ms.cs`). El presentador pasa al dibujo de la interfaz esa
  profundidad y el rectángulo de la imagen. Filtro nuevo `ImmediateTextureFilter::kDbz3LinearDepth` (D3D12): sus
  triángulos van por otro pipeline (`dbz3_ui_depth.vs/.ps`: UV en dos mitades + profundidad en el vértice; el píxel se
  descarta si la escena está más cerca). `dbz3_ui_depth_estado`: 0 sin soporte (Vulkan, SDK viejo), 1 sí, 2 probado en
  el último cuadro. Compatible en los dos sentidos (exe nuevo con DLL vieja: como antes; exe viejo con DLL nueva: igual).
- **Medido (sonda, no a ciegas):** la profundidad de 360 es **normal** (0 cerca, 1 lejos; prueba LESS), no la invertida de
  PS2. Con `DBZ3_ZBUF_DEBUG=1` el shader pinta la profundidad de la escena como color; contra las esferas de Goku
  (distancia 25-31) da 0,964-0,968 y el cielo 0,9997: z = lejos/(lejos−cerca)·(1−cerca/d) con cerca = mgr+0x20 (1) y
  lejos = mgr+0x24 (65535). La primera versión usaba la z de PS2 (mgr+0x40/0x44) y por eso el aro de detrás salía
  encima de Goku (aviso de P4): corregido.
- **Hecho (juego, `ps2_modelo`):** `Camara::Prof`, `CamaraCombate` lee cerca/lejos; `Texturas()` crea las texturas del
  filtro nuevo si se puede (`Crear(im, prof)`; las 2D siguen igual); `Escena::Pintar` empaqueta los vértices, usa una
  textura blanca para los triángulos sin textura y el borde toon, y pide la copia. **Las esferas de los luchadores
  siguen** como respaldo (orden del coordinador). Cámaras sin cerca/lejos (Shenron, tablero de R2): sin prueba, igual que antes.
  Sondas: `DBZ3_ZBUF_TRAZA=1` (registro), `DBZ3_ZBUF_DEBUG=1` (color = profundidad), `DBZ3_SIN_ZBUF=1` (todo como antes),
  `DBZ3_ZBUF_SIN_ESFERAS=1` (solo el z-buffer).
- **Archivos:** SDK: `include/rex/ui/immediate_drawer.h`, `include/rex/ui/d3d12/{d3d12_immediate_drawer,d3d12_presenter}.h`,
  `src/ui/d3d12/{d3d12_immediate_drawer,d3d12_presenter}.cpp`, `src/ui/shaders/dbz3_ui_depth.{vs,ps}.hlsl` (+ bytecode),
  `include/rex/graphics/d3d12/{command_processor,render_target_cache}.h`, `src/graphics/d3d12/{command_processor,
  render_target_cache}.cpp`, `src/graphics/shaders/dbz3_depth_copy_ms.cs.hlsl` (+ bytecode). DLL: rexruntime **11158528**,
  rexgpu-xenos **6596608** (en `verify_release.ps1` y AGENTS §7; anteriores en
  `rexglue-sdk-0.10/out/_respaldo_canonicas/20261016_antes_r15/`). Juego: `src/ps2_modelo.{h,cpp}`; **2 líneas en
  `src/iw_minijuegos.cpp` (de P4)**: `texs` = `ps2::Texturas(m)` y `Cam()` = `ps2::CamaraCombate` (antes era una copia).
  Fuentes de antes en `E:\DBZ3HD_trabajo\R15\fuentes_antes\`.
- **Prueba:** compila (SDK y juego). `iw_minijuego --par auto=0` PASS `D:\DBZ3HD\_pruebas\20261016-081443_zbuf_test_iw_minijuego`
  (estado 2, aros y monedas bien). **Oclusión píxel a píxel por el luchador, solo con el z-buffer**
  (`--par quieto=1 --env DBZ3_ZBUF_SIN_ESFERAS=1`): `...\20261016-081656_zbuf_test_iw_minijuego\a_salida.png` y
  `zoom_pelo.png` (la moneda de detrás queda cortada por las puntas del pelo). Con esferas: `...\20261016-081845_...`.
  Sonda con la escena en color (se ven los árboles del escenario en la profundidad): `...\20261016-080901_...`.
  R2: `r2_deseo` PASS `...\20261016-082104_zbuf_test_r2_deseo` y `tablero` PASS `...\20261016-082222_zbuf_test_tablero`.
  Shenron sale sin el borde negro ni los 3 tonos que tenía la prueba de R2 de las 08:05, pero **igual con
  `DBZ3_SIN_ZBUF=1`** (`...\20261016-082441_...`, comparación en `E:\DBZ3HD_trabajo\R15\shenron_ab.png`): no viene de esta
  vía, sino de cambios posteriores en el toon/borde de `ps2_modelo` o `pasarela_b2` (P4/R2), sin aislar.
- **Pendiente / ponytail:** sin captura todavía de **una roca tapando un aro** (la copia sí lleva el escenario: árboles
  en la sonda; falta una toma con un aro detrás de una roca). Vulkan: sin z-buffer (estado 0, como antes). Intel queda
  fuera como el FSR (motivo sin volver a medir). Ruta ROV del SDK: sin copia. `dbz3_ui_depth` sigue pedido toda la
  sesión tras el primer modelo (una copia por cuadro). Quitar las esferas cuando el coordinador lo dé por bueno.
  `github/patches/` del SDK sin actualizar. Margen de la prueba: 2e-6 en z (≈ 0,002 unidades a 30 de distancia).

#### R12 tercera vuelta (2026-10-16, 08:10-08:35)
- **Altura de IW (400) en los campos de vuelo 883/881:** API nueva de R11 en `src/suelo_iw.{h,cpp}`:
  `suelo_iw::SueloFijo(altura, on)`. Mientras está puesto, el gancho de la consulta de suelo del juego (sub_820CB0A0)
  responde esa altura en todo el escenario. El vuelo lo pone a 400 al empezar en 883/881 y lo quita al acabar
  (`VigilarCombate`). La lógica de B3 deja al luchador "en el suelo" a 400 (sonda: `luchador (0.0, 400.0, -2189.7)`), la
  cámara del juego lo sigue allí y se vuela a la altura de IW. Quitado el "a ras del mar" de la segunda vuelta.
  Comprobado a ojo: Krilin a 400 con las islas de Namek debajo y el mar al fondo
  (`D:\DBZ3HD\_pruebas\20261016-081625_vuelo_test__r12_vuelo\b_vuelo.png`, `c_control.png`). En el 881:
  Gohan a 400 sobre el mar de la Tierra con las islas al fondo (`...-081911_.../b_vuelo.png`); cerca del borde de la
  caja de R8 (±12000, la salida está en 11000, 11000) se ve dónde acaba el campo: IW repite el terreno en losetas.
- **El "HUD" suelto de arriba a la derecha (cara de Goku con flecha roja) era un objeto de IW, no un HUD:** el objeto 8
  / 0x108 del #MSF (congela el medidor del rival) es `ITEMA08_BLV_CHARACTER` (891/17, 10 triángulos), un cartel con la
  textura 891/16: **t0 = cara de Goten en su aro, t1 = la flecha roja, t2 = cara de Vegeta** (volcado
  `E:\DBZ3HD_trabajo\R12\item08.png`). Lleva la cara del rival: la malla apunta a la t0 (Goten) y en el 7 se pinta con la
  t2 (Vegeta). Los objetos `_BLV_` (ITEMA02/03/08/10) se pintan además de cara a la cámara (eran carteles vistos de
  lado). ponytail: el frente de la malla se toma en +z. Comprobado: `...-082340_vuelo_test__r12_vuelo_salida.png` (con
  `DBZ3_R12_SALIDA=0,-5350,32768`, 185 delante del objeto 8: la cara de Vegeta con la flecha roja, de cara a la cámara).
- **Personaje del mapa:** `iwvuelo::PersonajeDelMapa(id_iw)` (en `iw_mini_vuelo.h`) y una línea en la zona de IW de
  `src/pasarela_misiones.cpp` (más su `#include`), junto a la de la dificultad de P4: pasa el `jugador` (ID de IW) de la
  última misión superada (`info_.hecho`), que es lo que más se parece en el port al personaje que se lleva en el mapa
  de IW (DM+0x374 → tabla 0x47D450). El vuelo lo compara con 2 (7) / 9 (17) como IW: Gohan niño / Trunks niño cambian
  de personaje; con cualquier otro van Krilin / Gohan. `DBZ3_IWVUELO_PJ=0/1` lo fuerza en pruebas. ponytail: nuestro
  mapa no tiene un personaje que se lleve; si R1 añade uno, basta con pasar ese.
- **Prueba** (compila; capturas miradas): 7 PASS `...-081625_vuelo_test__r12_vuelo` (Krilin a 400, META 52,7 s, rango
  Z, 15200 puntos). 17 PASS `...-081911_vuelo_test__r12_vuelo` (Gohan a 400, META 154 s, rango A, 11000 puntos).
  Final, con la flecha corregida (su punta va hacia −z de la malla: apuntaba al revés) `...-082613_vuelo_test__r12_vuelo`
  PASS (`a_salida.png`: Krilin a 400 sobre las islas, la flecha hacia los aros).
- **Archivos:** `src/iw_mini_vuelo.cpp`, `src/iw_mini_vuelo.h` (`PersonajeDelMapa`); `src/suelo_iw.{h,cpp}`
  (`SueloFijo` y su rama en el gancho de sub_820CB0A0); `src/pasarela_misiones.cpp` (include + 1 línea). Pruebas:
  `DBZ3_R12_SALIDA=x,z,giro` empieza el vuelo en otro sitio.
- **Visto, sin arreglar:** con giro 0x8000 parado (`...-082340_...`) Krilin mira a la cámara en vez de hacia su
  marcha; en vuelo normal mira bien. Puede que la fórmula de P4 (X+6062 = ángulo − 15759) no valga en todo el círculo o
  que B3 le gire hacia el rival cuando está quieto: falta una sonda.
- **Pendiente:** el sitio propio de la casa y del objeto de 881 (sus huesos); la inclinación de la flecha; el objeto 10
  (efecto al azar); las animaciones de vuelo propias de IW (0x320-0x330).
- **Efectos de IW (`#AME`) portados con un reproductor propio** (`ame::` en `src/iw_mini_disparo.cpp`; RE con un
  subagente Haiku sobre el motor de partículas de IW, decomp EU 0x26F9C0-0x278340, y el censo de los 3316 `#AME` de
  DATA_CMN):
  - **Formato** (`#AME` v2, cabecera 0x10: el mismo que usa B3): nodos con tipo (clase << 16 | subtipo), banderas,
    hijo +0x24 y hermano +0x28 (desplazamientos). Emisores 0x1000x (0 esfera, 1 cono, 2 caja, 3 radial): duración,
    retraso, radio y velocidad (base + azar), tope de vivas, por cuadro, forma. Partículas 0x2000x (0 línea, 1 cuadro
    girado, 2 rayo, 3 sprite, 4 modelo): vida, retraso, tamaño (base + azar) con factor inicial → final, ángulo y giro,
    color inicial → final (bandera 0x10), mezcla (palabra del GS de 0x472480: 0x44 normal, 0x48 suma, 0x42 resta), su
    textura del atlas del paquete y UV fijas (0x2000) o animadas (0x8000). Cada cuadro: edad + 1, posición += velocidad
    / 60.
  - **Qué efecto lanza cada minijuego** (`Effect_DispatchById` EU 0x347DF0 → 21 cabos que fijan el hijo del paquete
    935): Ten: **2 → 935/3** carga, **3 → 935/4** disparo, **5 → 935/6** Kikoho, **4 → 935/5** golpe en Cell,
    **6 → 935/19** aura al cargar; Freezer/Piccolo (MSN010): **8 → 935/7** en la mano y **15 → 935/13** el núcleo del
    rayo (`s_HBCORE_MAIN`), que aquí sigue a cada rayo y deja su estela; ladrillos: **7 → 890/6/22** en la mira.
    El 8 (Kienzan) no usa `Effect_DispatchById` (su disco va por su propio sistema): sigue con el modelo `KIENCORE`.
  - Pintado con la cámara del combate (cuadros de cara a la cámara, líneas y rayos como estelas a lo largo de la
    velocidad) y el atlas del paquete (935/0, 58 texturas; 890/6/0 en el 3). ponytail: ImGui solo mezcla por alfa, así
    que la suma (y también la normal: sus texturas son grises con alfa plano y salían manchas oscuras) se pinta con alfa
    = brillo; la resta no se pinta; sin afectores (nodos 0x3000x: gravedad, remolino...), sin las partículas modelo, y el
    objeto del efecto no se mueve (el Kikoho de Ten se queda en las manos y apenas se ve; en IW el efecto 5 lleva una
    dirección cuyo uso no está leído).
- **Alfa de PS2 (P4):** `TexturaAmt` ya pasa 0x80 → 255 (sección de P4): quitado mi ×2. Lo que queda: las manos (texturas
  de personaje, cuyo alfa elige la fila del toon) se pintan opacas, como hace la PS2. P4 quitó también el corte cercano
  de `Escena`: el Kienzan en vuelo ya se ve.
- **Archivos:** `src/iw_mini_disparo.cpp` (registra también 0x7D3; `ame::`); escenarios `E:\DBZ3HD_trabajo\R13\
  r13_ladrillos.toml` y `r13_ladrillos_ver.toml`; sin cambios fuera de mi archivo.
- **Pruebas (compila 08:30; capturas miradas):** 9 con efectos `D:\DBZ3HD\_pruebas\20261016-082421_disparo_test_r13_piccolo`
  PASS (S, 8000; brillo rojo en las manos y estelas rosas de IW en los rayos); 15 `...-082032_...`, `...-082651_...` (S,
  5400) y `...-083018_...` PASS (con la mezcla corregida ya sin manchas; el Kikoho se ve poco); 3 `...-083258_...` PASS (Z,
  9000); 8 `...-081507_disparo_test_r13_freezer` PASS (Kienzan en vuelo visible tras el cambio de P4).
- **Pendiente:** el movimiento del objeto del efecto (args de dirección y velocidad de `Effect_SpawnAmo`), los afectores
  y la mezcla aditiva real (necesita un estado de mezcla en el pintado de ImGui o pasar a la pasada 3D de R15); las
  manos y el escenario propios de cada variante del 3 (Piccolo, Yamcha, Ten) cuando R1 pase el personaje del mapa.

### R16 pulido (2026-10-16, 08:55-09:15): tirón entre luchadores, sin presentación de B3, letreros y cámara
- **Tirón de raíz (Piccolo, 0x7DD).** Sonda `DBZ3_SONDA_PICC=1` (por cuadro, lo que el juego deja entre dos dibujos):
  con Gohan puesto en (0, 0) y Piccolo en (643, 323) el juego los dejaba en (255, 128) y (389, 195): **tope de distancia
  de B3 = `sub_82114910`** (estados 3/4/5/8; si `sub_82114348` > 150.0 (0x8201B41C) acerca uno, o los dos la mitad
  cada uno, en X+64 y luego `sub_820F9548(X+1616, 1)` + `sub_82110A38(X)`). Gancho `DBZ3_HOOK_EU(sub_82114910)` en
  `iw_minijuegos.cpp`: mientras `iwmini::Activo()` no acerca a nadie, pero sí refresca el cuerpo de los dos (sin eso el
  cuerpo dibujado se quedaba quieto: Gohan fuera de cuadro). Pareja EU por `tools/eu_map.py` (añadido
  `src/iw_minijuegos.cpp` a `FUENTES`; `eu_map_gen.h` solo suma 3 parejas: 82114910, 820F9548, 82110A38). Sonda
  después: X+64 = `propio` en cada cuadro (+1,43 por cuadro andando) y Piccolo sigue a la IA con un cuadro de retraso
  (+1,6 por cuadro), sin saltos.
- **Sin presentación de B3** en todos los minijuegos (también los módulos R12/R13/R14): en `Mando`, con el combate en
  estado 1-2, START recién pulsado (0x0008 del registro +12/+14, se devuelve como estaba en la lectura siguiente): el
  juego salta el vuelo y las entradas por su camino. Medido: estado 2 dura 17 ms (antes 6 s); el 1 aún ~2,4 s (el
  juego no acepta START hasta acabar su fundido).
- **Letreros de IW:** "READY?" (DATA_USA 5/1 t08) durante el READY de B3 y "START!" (t00) el primer segundo de juego,
  en el sitio y escala de R14.
- **Cámara:** mira más cerca y más baja (`kCamMira` 14→6, `kCamMiraAlto` 7→5): el jugador queda entero por encima del
  aro y la barra del HUD (revisión del coordinador, minijuego 0). Con el rayo contra el #WCF cortando, el ojo va 3 u
  antes del corte hacia la cabeza (antes, en el corte: el plano cercano entraba en la roca).
- Compila (`tools/compilar.py`, código 0). Pruebas PASS (`D:\DBZ3HD\_pruebas\`): Piccolo `20261016-090955_combate_test_iw_minijuego`
  (`d_mirar`: Gohan de espaldas, Piccolo detrás, sin temblor; sonda en el log), antes del gancho `-090204_*` y
  `-090715_*`; minijuego 0 `20261016-091142_combate_test_iw_minijuego` (Goku entero sobre el HUD); vuelo
  `20261016-091142_vuelo_test__r12_vuelo` (presentación saltada, vuelo y resultado bien). Mosaicos en
  `E:\DBZ3HD_trabajo\R16\` (buscadores del tope: `buscar*.py`, `const150.py`, `llamadores.py`).
- **Pendiente:** (1) Gohan no se ve en `a_salida`/`b_npc`/`c_npc` del 0x7DD: la cámara queda pegada a la cabeza cuando
  el terreno detrás está cerca (rayo + 3 u); IW mueve esa cámara con `Minigame_UpdateFollowCamera` (EU 0x299A70,
  factor 0,12) y `Camera_UpdateFollow` (0x297DE0): portar su distancia mínima. Ya existía antes de R16 (`-071329_*`).
  (2) Arranque propio de IW sin portar: el decomp da cámaras animadas por datos (`Camera_PlayCameraAnim`, códigos
  0x296, 0x258..0x558 por variante), animación del jugador 0x11D (intro, `MgPly_Enter_State0A` 0x3185A0), 0x51/0x118/
  0x17C/0x49C/0x62C/0x6F4 al empezar (`s_SPACE` 0x29BFF0), cartel del objetivo 140/150 cuadros con el reloj parado
  (0x299900) y al acabar 0x2F/0x30 (anim 0x53/0x11A/0x17E/0x49E/0x62E/0x6F6); falta leer qué variante es cada MSNMW y
  sus animaciones de cámara del AFS. (3) Disparo 9 (Piccolo se sale por arriba, R13) no tocado; con el tope quitado
  puede cambiar su órbita: revisar con `disparo_test`. (4) La sonda `DBZ3_SONDA_PICC` queda (solo con la variable).

#### R16 segunda vuelta (2026-10-16, 09:15-09:30): entrada propia de IW, cámara de IW, disparo 9
- **Decomp (Haiku):** la variante = el índice del minijuego (campo +4 de `LoadWaitMinigamesFlag`, 0/1/2/10/14/16).
  Entrada: `MgPly_Enter_State0A` (0x3185A0) pone 0x11D (cámara animada 0x296) y luego `s_SPACE` (0x29BFF0) la de la
  variante (0x51, 0x118, 0x17C, 0x49C, 0x62C, 0x6F4); `LoadWaitGroundMinigameObjective` (0x299900) con parámetro 0 =
  cartel de 140 cuadros con el reloj parado (+0x8D21). Final: estado 0x2F (`MgResult_ComputeScore(+1)`, anim fija
  0xF0, solo la define el #BSK de Piccolo 925: no portable al jugador) y 0x30 ((-1), por variante 0x53, 0x11A, 0x17E,
  0x49E, 0x62E, 0x6F6). Las de variante salen del #BSK de DATA_CMN 922 (Goku) y 914 (Gohan niño, 0x62C/0x62E);
  0x11D del común (204/3 → almacén global 266). Escaneo con `E:\DBZ3HD_trabajo\R10\scan_bsk.py`.
- **`anim::Reproducir(x, codigo, sujetar)` + `EstadoPose()`** en `src/anim_iw.{h,cpp}` (R10): esas 13 animaciones se
  leen con las del scatto (`kExtras`; la que falte no impide las demás), van al almacén global y al #CSK propio (lista
  hasta 0x6F6), y se ponen una vez; al acabar, a la guardia o sujeta en su penúltimo cuadro.
- **Entrada de IW en `iw_minijuegos.cpp` (Fotograma):** al empezar la pelea 0x11D y después la de la variante, sin mando
  (ni andar ni scatto), reloj parado y NPC quieto hasta acabar las dos y los 140 cuadros del cartel (la frase de la
  fila en una banda; la pieza de UI del cartel no se lee en el decomp). Cámara de la entrada: media vuelta alrededor
  del jugador, de cara a él hasta detrás (ponytail: las cámaras animadas de IW, datos sin leer).
- **Cámara de seguimiento de IW** (`Camera_InitFieldDefault` 0x297140 + `Camera_UpdateFollow` 0x297DE0 +
  `Minigame_UpdateFollowCamera` 0x299A70): ojo 70 detrás y 15 arriba del punto seguido, mira a ese punto + 5; el punto
  sigue al jugador 0.12 por cuadro (0.3 en la variante 2); rayo de T al ojo y el ojo en el corte (aquí 3 u antes).
  **IW no tiene distancia mínima:** a menos de 10 pinta al jugador con alfa 0.1 (no portado).
- **Disparo 9** (`E:\DBZ3HD_trabajo\R13\r13_piccolo.toml`) PASS con el tope quitado: Piccolo entero en pantalla en las
  4 capturas (`D:\DBZ3HD\_pruebas\20261016-092234_disparo_test_r13_piccolo`).
- Pruebas (escenario `E:\DBZ3HD_trabajo\R16\r16_inicio_fin.toml`, 0x7DD con piloto): escenario de B3
  `20261016-092715_modos_test_r16_inicio_fin` (START!, cartel y reloj parado en 06'00"00, Gohan con su entrada; luego
  en juego) y campo de IW `-092715_combate_test_*` (CLEAR al final); `-091938_modos_test_iw_minijuego` (entrada y
  Piccolo detrás). Mosaicos `E:\DBZ3HD_trabajo\R16\mosaico_v2_*.png`, `mosaico_inicio_fin.png`, `mosaico_v3.png`. Compila.
- **Pendiente:** (1) la animación final (0x62E...) con `Reproducir(..., sujetar)` deja la pantalla en azul liso
  mientras dura (HUD incluido; el juego sigue: medido con y sin ella en `-092439_*` / `-092715_*`): queda apagada salvo
  `DBZ3_IWMINI_FINAL=1`; sin ella el final es el de antes (fundido + CLEAR). (2) En el campo 906 la loma tapa a Gohan
  con la cámara de 70 (el rayo de la máscara 1 no la corta): falta el barrido de esfera de IW (radio de la pantalla,
  sin leer) o el alfa 0.1. (3) 0xF0 (éxito) no existe para Goku/Gohan en sus bancos.

### F2
Tablero Dragon World de B2 (`src/pasarela_b2.cpp`), revisado con el descompilado de B2 (`E:\DBZ3HD_trabajo\ghidra\salida\b2.c`).

**Hecho (lo que leíamos mal del #YDW):**
- **Bolas de Dragón** (ELF `0x27BD80`): el juego no pone 1 Bola por fase, pone `min(fase, 6) - Bolas que ya tienes + 1`
  (como mucho 3) en sitios al azar de la tabla de objetos, cada una con un número de Bola que aún no tienes. Así se
  recuperan las que no cogiste. Ahora es igual: las Bolas se guardan por número (1-7), no por fase. Los sitios posibles
  (tipo 0) son los del juego: 1 en la fase I, 5 en las II-VIII y 6 en la IX. La guía de PAndrews pinta más (7, 7, 6,
  ~9, ~7, ~13): en el #YDW no hay más, así que la guía cuenta otra cosa (ponytail: no se ha averiguado qué).
- **Objetos enterrados** (ELF `0x27C090`): cada búsqueda saca **uno**, el primero que queda en esa casilla. Antes salían
  todos de golpe y la casilla quedaba vacía. La fase IX tiene 5 o 6 pilas de zenie enterradas en las casillas 3, 4, 6, 7
  y 8: hay que buscar varias veces. La casilla se pone gris cuando ya no queda nada.
- **Objetos con condición** (ELF `0x27CD40`): la tabla trae banderas (+8 cuántas, +12 cuáles). Antes se descartaban.
  Ahora salen cuando se cumplen: cápsula de la fase III (bandera 307), las 3 cápsulas de la fase VI (mientras la 50 esté
  apagada) y daga, armadura y cápsula de la fase VIII (201-203). Las 3 cebollas condicionadas de la fase IX también.
- **Movimiento inicial de cada pieza** (ELF `0x2791A0`): los bytes +3/+4 de T2 son su tipo de movimiento y su dato,
  el mismo campo que cambia la orden 50. Antes todos perseguían cada dos turnos. Ahora, por ejemplo, Nappa y los
  Saibamen de la fase I esperan quietos (tipo 0), Raditz se mueve (tipo 5), y en la fase III dos rivales van a las
  casillas 10 y 8 (tipo 2).
- **Botones como B2:** círculo = B **pasa**, triángulo = Y **sale**, en el tablero, en la preparación, en el resultado de
  la fase y en Shenron (y en su línea de ayuda). La pantalla "no se pudo leer la ISO" sigue con B para salir.
- **Comprobado y correcto (no cambia):** casillas, caminos, Dende, alfombras de Mr. Popo (fase VII: 7 alfombras y 6
  destinos; la guía cuenta los dos), y casillas con condición (fase V: 2 de vida que baja y 1 de ki: el #YDW no tiene
  un tercer corazón gris). Saibamen de la fase I: el #YDW tiene 9 (rivales, bando 2, en las casillas 3, 5, 6, 8, 9, 11,
  12, 13 y 16); la guía dice 4. Se deja lo del juego. Goku en la fase III: en el #YDW es una pieza sin bando en la
  casilla 17 que activan los scripts (T3/T4 con las piezas 15-17). ponytail: sigue saliendo como compañero elegible,
  falta portar su llegada.
- **Estructura (tarea 2):** superpuse casillas, caminos y adornos sobre los mapas 346+5k (fases I, V, VII y IX,
  `E:\DBZ3HD_trabajo\F2\ov_todo.png`). Caen sobre el terreno, en la escala y el origen del mapa (px de la textura del
  mapa, eje y hacia abajo). No hay error de escala ni de ejes que corregir.

**Archivos:** `src/pasarela_b2.cpp`, `tools/pruebas.py` (escenario `tablero`: pasar con Backspace = B),
`docs/14_frentes/PORT_EXACTO.md` (línea de botones). Respaldo: `E:\DBZ3HD_trabajo\F2\respaldo\`. Scripts de lectura:
`E:\DBZ3HD_trabajo\F2\` (`t8b.py` objetos, `t2.py` piezas, `scr.py` scripts, `ov.py` superposición, `fns.pkl` +
`g.py`/`p.py` para buscar en el descompilado).

**Prueba:** compila (`tools/compilar.py`, sin errores).
- `correr escenarios_test2 tablero`: **PASS** (`D:\DBZ3HD\_pruebas\20261016-101234_escenarios_test2_tablero`). Revisado a
  ojo `d_movimiento.png`: la ayuda dice "A Mover, X Buscar, B Pasar, RB Personaje, Y Salir"; Saibamen en sus agujeros,
  adornos (torre de Karin, naves) sobre el terreno. La primera pasada falló porque el escenario pasaba con P (= Y, que
  ahora sale): corregido el escenario.
- `r2_fases --par desde=8 --par hasta=9` (fase IX): **PASS** (`D:\DBZ3HD\_pruebas\20261016-101058_escenarios_test_r2_fases`).
  El registro muestra 60 objetos leídos, 3 Bolas puestas (nº 3, 5 y 1 en las casillas 33, 24 y 20, porque la copia no
  tenía ninguna) y "¡Una Bola de Dragón!" al buscar en la 33.

**Pendiente:** saber qué cuenta la guía como bola naranja de más; la llegada de Goku en la fase III (scripts); el ritmo
exacto con el que mueve cada tipo de movimiento (hoy, cada turno según su tipo).

### F1 (ronda final, Infinite World, 2026-10-16 09:55-10:45)

**Hecho**
- **Caras del HUD de los minijuegos.** t36 de DATA_USA 6/0 es Yamcha, no Gohan (hoja `E:\DBZ3HD_trabajo\R14\caras_6_0.png`).
  Nueva `iwmini::PintarCara(dl, x, y, lado, id, forma, espejo)` (`src/iw_minijuegos.{h,cpp}`, junto a `HudIW`): usa la
  cara de IW solo si es del mismo personaje y forma (`CaraIW`: Goku 0 t29, Vegeta SSJ t33, Trunks SSJ t34, Krilin t32,
  Piccolo t35, Ten t37, Yamcha t36). Si no, usa la nativa de B3 (`caras/<id>` vía `S::Img`, recortada en círculo).
  `HudIW(segundos, barra, id, forma)` pasa a recibir el ID del personaje, no el índice de textura.
  - Archivos: `iw_minijuegos.cpp` (`d.cara = f->jugador`), `iw_mini_vuelo.cpp` (sin `kCara`: `HudIW(..., j.jugador)`),
    `iw_mini_botones.cpp` (sin las tablas de caras: jugador y rival con su forma, el rival en espejo). Disparo, sin tocar.
- **Ladrillos de Kaio (0x7D3).** Los ladrillos ya se pintaban en su sitio: `Lad3D` y la proyección real dan el mismo
  punto (sonda en el registro). Las capturas de R13 caían entre dos ladrillos. Con el piloto, cada ladrillo vive menos
  de 1 s y a 1000 de distancia mide unos 27 px. Aun así, ahora se pintan con `LadCam`: la inversa de la cámara real del
  combate, en el punto de pantalla de PS2. Así el ladrillo siempre queda bajo la mira, y su centro (no su base) en ese
  punto. Sondas: `iwdisparo: ladrillo pintado` (una por segundo) y la medida del ladrillo y de los tramos al cargar.
- **Goku del mapa de Dragon Mission en 3D.** La entrada no está en las funciones "MiniGoku*" del descompilado: sus
  nombres de Ghidra son falsos (pow/atan2). Está en **DATA_CMN 1093**, el paquete del mapa que lleva el #DMD. Hijos:
  0 #AMT, 1 #AMM (8 animaciones: 100 10 75 52 36 20 50 20 cuadros) y 2 #AMO (huesos `GKS_`, como el tablero de B2).
  **No** es el mismo archivo que B2 CMN 337/338/336: ningún md5 coincide. Hecho `DibGokuMapa` en `pasarela_iw.cpp`, con la
  cámara de R2 (20 grados), el toon de P4, espera/andar y el giro hacia donde anda. Se añadió `material` a
  `ps2::LeerAmo/CargarSuelto` (`ps2_modelo.{h,cpp}`, por defecto apagado).
  **Sale mal:** brazos blancos y cara rota. Las texturas de 1093/0 son casi blancas, el material de sus partes es
  blanco (1,1,1,1) y sus vtype 0x1B5/0x1B4 no traen color de vértice (sonda `dbz3 ps2: parte`). Falta leer de dónde saca
  IW la piel. Por eso queda **apagado**: solo con `DBZ3_IW_GOKU3D=1`. Sin esa variable, el dibujo t90 de siempre, ahora con
  sombra. Pruebas de textura y animación: `DBZ3_TEST_GOKU_IW_TEX="hijo,relativa"` y `DBZ3_TEST_GOKU_IW_ANIM="espera,andar"`.
- **Final de IW (DBZ3_IWMINI_FINAL).** Sigue apagado. Medido con `E:\DBZ3HD_trabajo\F1\f1_final.toml`: en el mismo
  cuadro en que se pide 0x62E, la imagen entera (HUD de ImGui incluido) pasa a azul liso (78,133,204). El juego sigue a
  60 fps, sin errores de GPU, y el combate acaba bien. El bloque del #BSK de 0x62E es igual al de 0x62C, que sí funciona
  (registro nuevo en `anim_iw.cpp`). Diferencias que quedan: `sujetar` y el `Scatto(kApagado)` del mismo cuadro. Sin causa.

**Pruebas (todas PASS, capturas revisadas a ojo)**
- Caras: `D:\DBZ3HD\_pruebas\20261016-100709_combate_test_iw_minijuego` (0x7DD: Gohan niño con su cara nativa),
  `-100709_vuelo_test__r12_vuelo` (0x7D6 con `DBZ3_IWVUELO_PJ=1`: Gohan niño) y `-102646_botones_test__r14_botones`
  (0x7DB: Vegeta SSJ con t33; Trunks forma 0 con la nativa, en espejo).
- Ladrillos: `-102646_disparo_test_f1_ladrillos` (escenario `E:\DBZ3HD_trabajo\F1\f1_ladrillos.toml`, sin piloto, con
  capturas mientras el ladrillo vuela: lejos y cerca); `-101633_disparo_test_r13_ladrillos` (sonda).
- Mapa: `-102646_modos_test_iw_mapa` (sprite por defecto) y `-102029_modos_test_iw_mapa` (3D, con los fallos de color).
- Final: `-102114_combate_test_f1_final` (azul desde `b_final0`).

**Pendiente**
1. La piel del Goku 3D del mapa: leer en IW cómo pinta 1093/2 (otra CLUT, otra textura o el color de luz de VU1).
   Después, quitar la variable y probar con `DBZ3_IW_GOKU3D=1`.
2. La causa del azul del final de IW.
3. `Lad3D` sigue usándose en los efectos y chispas del disparo. Coincide con `LadCam` con la cámara del #MSS; si la
   cámara cambia, deberían usar también el punto pintado.

#### F1 segunda vuelta (10:45-11:00): Goku 3D del mapa, color de piel (sigue apagado)
Probado a petición del coordinador (`colores` y `material` de R2 ya iban puestos). Lo medido:
- **Color de vértice:** las partes son vtype 0x1B5/0x1B4. Hay normal y UV, pero no el bit 2: el modelo no trae color
  por vértice, y el paso de la tira lo confirma (`esperado == paso`).
- **Material:** la cabecera de cada parte tiene +0x20..+0x3C a 1.0 (blanco) y no hay ningún otro color en 0x00-0xA0.
  De 0x40 en adelante van los registros del GS: COLCLAMP 1, CLAMP_1/2 = 5 (clamp, sin repetir) y ALPHA_1/2 = 0x44.
- **UV:** todas dentro de [0, 1], así que no es un problema de repetición. La cara no se rompe por la UV.
- **Texturas (1093/0, 14, PSMT4):** el RGB es correcto (pantalón azul, gi naranja, cara con los ojos). Pero la piel de
  brazos, manos y cara es **blanca** en la textura, y 7 de las 14 tienen alfa 0 en toda la CLUT (en IW irán con TCC=0,
  opacas). Mosaico en `D:\tmp\f1\tex0_rgb.png`.
- Conclusión: el tono de piel no está en el modelo ni en sus texturas. IW debe ponerlo en el pintado (color de luz del
  microprograma de VU1 o una rampa toon propia del mapa), y eso sigue sin leer.
  **No sale bien → se queda el Goku 2D por defecto** (`DBZ3_IW_GOKU3D=1` para verlo en 3D). Captura en 3D:
  `D:\DBZ3HD\_pruebas\20261016-103104_modos_test_iw_mapa`.
- Pendiente: buscar en el ELF de IW el pintado del modelo 1093/2 (quién lee el #AMB del #DMD y con qué color de luz), y
  tratar como opacas (TCC=0) las texturas con alfa 0 cuando se arregle la piel. Las sondas extra se quitaron; queda la
  de las partes (`dbz3 ps2: parte`, solo con `material`). Compila.

### F3 Ayudas y tutoriales optativos (2026-10-16)
- **Hecho.** Módulo nuevo `src/ingame/tutorial.{h,cpp}` (en las dos listas de `CMakeLists.txt`): panel propio con el estilo
  del juego (marco `S::Frame`, bocadillo `S::Speech`, barra `S::mode::Help`, letra del juego) en la columna de `S::mode`;
  A siguiente, B cerrar (vuelve la próxima sesión), Y "No volver a mostrar" (también al acabar los pasos). Lee el mando
  él mismo (DeferUi + `GetStateForUI`) y la pantalla de debajo deja de leerlo con `tutorial::Abierto()`. Lo visto va a
  `UserDataRoot()/ayudas.toml` (lectura con try/catch, escritura .tmp + rename). Catálogo de textos en `tutorial.cpp`
  (inventario §15, comprobados en el código). Las pruebas con `DBZ3_NO_DIALOGS` (todas las de `pruebas.py`) no los ven,
  salvo con `DBZ3_TEST_TUTORIAL=1`, así que no tapan capturas de otros frentes.
- **Ajustes rápidos (F1 > PANTALLA):** "Mostrar tutoriales" (cvar nueva `dbz3_tutoriales`, de serie sí) y "Volver a ver
  tutoriales" (mueve `ayudas.toml` a `user_data/dbz3/respaldo/ayudas_<hora>.toml`, nunca borra).
- **Una línea por modo:** Dragon World y Nave de Babidi (`pasarela_misiones.cpp` DrawTablero/DrawNave), Dragon Mission y
  Fighter's Road (DrawMapaIW), minijuegos IW (`pasarela_iw.cpp`, al pulsar A en un "?": el general de controles la primera
  vez + el de su tipo carrera/vuelo/disparo/botones por clave; tras cerrarlo, A otra vez empieza), Leyenda de Mr. Satán
  (`pasarela.cpp` OnDraw), Modo Héroe y retos (`modos.cpp`, pantalla kRpgP), HUB (`ingame/hub_panel.cpp`; `hub_menu.cpp`
  Input no lee el mando con el tutorial abierto).
- **Tooltips:** HUB Laboratorio (muñeco + datos del último golpe), Pasarela (historias y misiones de B2/IW, ISO) y Satán
  (11 retadores, pide Budokai 1) en 5 idiomas (`hub_menu.cpp` kHelps); Modo Héroe: el bocadillo explica la fila elegida
  (raza, cuerpo, apariencia, estilo, empezar) con `L()` de 5 idiomas (`rpg.cpp` DibujarHeroe). 51 textos nuevos fr/de/it
  en `res/ingame_ui/i18n_modos.toml`.
- **[verificar] resueltos leyendo el código:** "Minijuegos" del HUB no abre nada (nadie registra el modo; sale gris) →
  su texto no se cambia; la Torre sí muestra su regla y la Simulación su "Día N / 3" → sin cambios; `g_auto` de Satán es
  solo de prueba → sin ayuda; la raza solo cambia qué cuerpos/trajes hay (no estadísticas); online "DEF. DIRECTA" =
  definitiva directa (no defensa), CANCELACION LB/RB = botón de cancelar, PIERDE FORMA = el daño quita la forma,
  "Distintas" = reglas distintas en los dos PCs → se juega el combate original; Tag Team: el que sale no recupera vida (se
  guarda y vuelve igual).
- **Prueba:** `docs/14_frentes/escenarios_pruebas/_f3_tutorial.toml` (Dragon World: abre, A pasa al 2/6, Y lo guarda) PASS
  `D:\DBZ3HD\_pruebas\20261016-112717_bugs_test2__f3_tutorial` (capturas revisadas: panel con marco, bocadillo y barra
  A/B/Y; tras Y, el tablero sin panel y sin salir del modo) y `_f3_tutorial_visto.toml` (segunda entrada: "ya visto", no
  sale) PASS `D:\DBZ3HD\_pruebas\20261016-112958_bugs_test2__f3_tutorial_visto`. Compila (`tools/compilar.py`, código 0).
- **Pendiente:** solo se ha probado en juego Dragon World (los demás modos usan la misma llamada); la barra de dificultad
  de Satán no dice arriba/abajo (no hay icono de cruceta en `S::Glyph`; lo cuenta el tutorial); Kili de la Nave y las
  etiquetas del Online sin tooltip propio (archivos de R2 y del online: textos ya resueltos arriba); el tutorial de
  minijuego pide pulsar A otra vez tras cerrarlo.

### F4
Fidelidad a Budokai 2 del Dragon World y la Nave (`src/pasarela_b2.cpp`, `src/pasarela_nave.cpp`), con el descompilado
`E:\DBZ3HD_trabajo\ghidra\salida\b2.c` y el ELF europeo. Volcados y hojas: `E:\DBZ3HD_trabajo\F4\` (`ext.py CMN|SP <n>`
saca una entrada y vuelca su `#AMA`/`#AMT`; `hoja188.png` = todas las piezas del marcador).

**Hallado (RE):**
- `D_TURN`, `D_CLEAR`, `D_RADAR`, `D_RADARBASE`, `D_DBGET` y `DBALL` no son imágenes sueltas: son **nombres de tareas**
  (ELF 0x270210 y 0x26FE80). Todas dibujan con el mismo par: el cargador del tablero (0x2350C0) pone sus 16 archivos en
  0x6993C0; las texturas 0-5 van a 0x698DA0 y los `#AMA` 6-9 a 0x698DC0. Así `uRam00698dc8` = **CMN 402** (`#AMA`, 122
  animaciones) y `uRam00698db0` = **DATA_<idioma> 188** (39 texturas). La 188 trae todo el marcador: radar sano y roto,
  barra de madera, "MUNDO DRAGÓN", las 7 Bolas, el rombo y sus cifras azules, la ventana de mensaje (nube, 4 lados,
  fondo azul), la caja del menú, "Salir", "¿Reiniciar Mundo Dragón?", "¿Salir del modo Mundo Dragón?", "¿Salir de
  partida actual y empezar de nuevo?", "Sí", "No" y las flechas.
- `D_CLEAR` (0x26F280 → 0x26EFC0 → 0x26ED50 → 0x26EB00) es la ventana de los mensajes de la orden 38: 8 piezas (entrar
  0x33+3k, reposo +1, salir +2) y encima los dos textos de la fase. La pieza 0 es "Borrar cond." y la tapa la nube.
- Control del tablero (0x237xxx): triángulo y Start abren el menú (0x2A3C10), L1 esconde o saca la barra y el radar
  (0x2700E0), R1 abre la lista de habilidades (0x28EBC0, módulo `ess_*`), R2 sale a otra tarea (0x237790, sin descifrar).
- Menú (`DWU_MMAIN`, 0x2A35F0): "Salir" y "¿Reiniciar Mundo Dragón?"; cada uno abre su Sí/No (`DWU_YN*`, 0x2A2C90), que
  empieza en "No". "Sí" en reiniciar borra la partida del Mundo Dragón (0x693A1C, 0x9C bytes).
- **Orden de fases:** el Mundo Dragón carga la fase de su partida (0x6992DA, copiado de 0x693A96 y limitado a 0-8). No
  hay pantalla para elegir otra: **B2 obliga a jugarlas en orden.**

**Hecho:**
- Marcador del original (`DibHud`): rombo con el turno (cifras puestas como en 0x26E6F0), barra de madera, "MUNDO
  DRAGÓN", cada Bola en su hueco (la recién cogida entra con las claves de la 7, como D_DBGET, sin letrero) y el radar.
  El radar solo sale tras recoger el Radar del Dragón: brillo y, por cada Bola de la fase, la flecha girada hacia ella
  (a más de 300 px del mapa) o el punto a 1/10 de la distancia (0x26F940). L1 esconde o saca la barra y el radar.
  Sin los archivos (otra ISO), el marcador de antes.
- Ventana de los mensajes de guion con sus 8 piezas y sus claves de entrada (texto "Fase 1" sobre la nube, objetivo en
  el fondo azul).
- Menú del tablero con las imágenes del original: Y o Start abren "Salir / ¿Reiniciar Mundo Dragón?", A abre el Sí/No
  (flecha en la opción), Y vuelve. "Sí" sale del tablero o reinicia el Mundo Dragón (fase I, sin Bolas). Antes, Y salía
  al instante.
- Fases en orden: quitados arriba/abajo de la preparación (y sus flechas). Para volver a la I, el menú.
- Nave: "¡Nuevo!" solo en la primera vuelta de la barra y si el premio da una cápsula de B3 (progreso nuevo
  `nave_vueltas`). Los demás premios salen sin la palabra.
- `tools/pruebas.py`, escenario `tablero`: abre el menú (P = Y), entra al Sí/No y vuelve (capturas `d2_menu`, `d3_si_no`).
- Respaldos: `E:\DBZ3HD_trabajo\F4\respaldo\` (`*_antes_f4.cpp`).

**Prueba:** compila (`tools/compilar.py`, código 0; la primera vez falló el enlace por `PantallaDM::SonidoResultado`
de IW, a medias en otro frente). Capturas revisadas a ojo.
- `tablero` PASS `D:\DBZ3HD\_pruebas\20261016-112657_escenarios_test2_tablero`: `b_tablero` con la ventana del original
  ("Fase 1" en la nube y el objetivo en el fondo azul), rombo con el 1 arriba a la derecha y barra "MUNDO DRAGÓN";
  `d2_menu` (caja con "Salir" y "¿Reiniciar Mundo Dragón?"), `d3_si_no` ("¿Salir del modo Mundo Dragón?", Sí, No y la
  flecha en "No"); combate como antes.
- `r2_deseo` PASS `D:\DBZ3HD\_pruebas\20261016-112901_escenarios_test3_r2_deseo` (vuelve a la preparación de la fase I sin
  las flechas de fase).
- `r2_nave` PASS `D:\DBZ3HD\_pruebas\20261016-112901_escenarios_test_r2_nave` (0 Kili: sin premios, como antes).
- `r2_fase1` PASS `D:\DBZ3HD\_pruebas\20261016-113232_escenarios_test2_r2_fase1` ("fase 1 completada" a los 592 s; pasa
  sola a la fase II, "¡Derrota a Frieza!", sin flechas para elegir fase).

**Pendiente (no hecho, por prioridad):**
- Avisos sin imagen en el original: Dende, Mr. Popo, daga, armadura, zenie, cápsula y "no hay nada" siguen con letra de
  B3. La 188 no trae esos textos; B2 seguramente los muestra con iconos o con la ventana de mensaje, sin descifrar.
- R1 (lista de habilidades, `ess_list`/0x28EBC0) y R2 (0x237790): sin portar. Las habilidades de B2 no son las de B3.
- Radar roto (anims 24-37, "BOOM") y entradas y salidas animadas del marcador y del menú: no hechas (solo el reposo).
- Pantalla de resultado tras el combate (`dw_result.c`) y órdenes de guion (diálogos 41/42, cámara 17-31, piezas 58-62):
  sin tiempo en esta vuelta.
- Nave, Prueba IV (ya hecha, 2026-10-17): **cuenta ondas de ki devueltas**, 3 Kili por onda, como B2. Regla de B2
  (`b2.c`): `FUN_001e2610` pregunta a `FUN_001f7fc0` y, con 5 ("devuelta": guardia pulsada hace menos de 5 cuadros),
  `FUN_001ebf40(luchador, 3)` suma 1 en +0x9bb8; `FUN_001d41e0` lo da como resultado y `FUN_00265ab0` paga x3. En B3 es el
  mismo suceso (`sub_82116FA0` = `FUN_001f7fc0`, mismos 5 y 12 cuadros): `sub_82119B38` devuelve la onda llamando a
  `sub_82122488(1, 26)` desde 0x82119D00 (EU 0x82119E00); `src/combate.cpp` lo cuenta (`OndasDevueltas`) y
  `pasarela_nave.cpp` lo toma en `Resultado`. El rival de B2 en el modo 9 no ataca (`FUN_001d1c40` da -2), vuelve a la
  guardia (`FUN_001d1db0` da 1) y devuelve toda onda (`FUN_001d1980` da 2): un rally. Aquí: `OndasModo` lo pone de muñeco
  en guardia con la habilidad Deflect-Back (X+8125+16) solo mientras dura la prueba IV. Medido: 97 ondas = 291 Kili con
  Deflect-Back y guardia mantenida (resultado "97 veces / 291 Kili", igual que el registro), 23 ondas = 69 Kili a toques
  de guardia sin cápsula. Los nombres de los premios siguen siendo los de la FAQ inglesa: su imagen en la ISO sigue sin
  localizar.

### F5
Fidelidad de Dragon Mission / Fighter's Road con IW (auditoría `E:\DBZ3HD_trabajo\integracion\fidelidad_iw.md`;
descompilado EU `iw_sles55347.c`). Entrada por el HUB sin tocar; textos de IW siguen de la copia USA.

**Hecho**
- **Resultado (D05, D08):** las filas de la hoja 334/1 t17 entran una a una con el ritmo de IW (`Results_StateBeginRoll`:
  100 cuadros antes de la primera; `Results_UpdateTally`: 60 por fila) y luego EARNED! ... ZENI; A salta el conteo y,
  acabado, vuelve al mapa. Orden de t17: TIME, RANK, BRICKS DESTROYED (solo 0x7D3, con los ladrillos), SCORE (con New!)
  y TOTAL SCORE = el total de esta partida (antes, el récord). Combates: TIME y RANK como antes.
- **Zeni de minijuego (D06):** `Results_ComputeTotalsAndRewards` (373040) escribe el premio (tabla del rango x factor de
  la dificultad, a la centena; sin rango, el de fallo) en el bloque de resultados +0x13 = `+0x5A` que suma
  `Battle_ApplyResultRewards` (305942) con o sin rango. Los módulos de minijuego ya calculan ese premio como sus puntos,
  así que Zeni = puntos (antes puntos / 10 y 0 si no se superaba). ponytail: el tope de IW (999900) no se aplica porque
  el monedero es el común del proyecto (`rpg.cpp`, tope 9.999.999).
- **Dificultad (D07):** la de la ficha (0 Very Easy .. 4 Very Hard) llega tal cual al minijuego (= descriptor +0xC del
  MSNMW; t16 tiene una sexta fila, Z, sin condición de apertura localizada). El COM de las misiones sigue con su
  equivalente más cercano de B3 (3 niveles: Very Easy/Easy Novato, Normal Experto, Hard/Very Hard Avanzado).
- **Ficha (D03):** menú de IW con las piezas de 334/1: t11 (MISSION SETTINGS, CONTROLS, DETAILS, SKILL EDIT) y
  MISSION START (cursor por defecto); arriba/abajo cambia y el panel de la izquierda enseña la entrada: DETAILS = título,
  texto y condición; MISSION SETTINGS = DIFFICULTY (t13 + t16) y CHARACTER (t13 fila 1, el luchador fijo de la misión);
  CONTROLS = controles del modo (combate de B3, START pausa) y la condición; SKILL EDIT = aviso de que las misiones usan
  las cápsulas de serie. DIFFICULTY sigue visible bajo el panel; izquierda/derecha la cambia siempre.
- **Barra de ayuda (D14):** "Y" del mapa con etiqueta traducible (A Fighter's Road / A Dragon Mission). El resto ya era T::T.
- **Audio:** `pasarela::OirAdxPs2(juego, afs, entrada, volumen)` / `CallarAdxPs2()` (`pasarela_b1.cpp`, cada sonido por
  su waveOut, se mezclan). `PantallaDM::SonidoResultado` lo llama por fila y al premio (tabla `kSonidos`, hoy -1 = mudo).

**No localizado (marcado, no inventado)**
- Sonidos/voces de las filas: los de `Results_*` son de un minijuego (voces del personaje, tabla 0x494280; slots de
  `Menu_VoiceScreenLoadFiles` en DATA_CMN, no ADX). La música de resultado de misión la pone `s_FIX_MSN_RESULT`
  (EU 0x287040) con `PTR_Soundtracks_0044e3ec[uRam005cb888] | 0x20000` (slot 2 = ADX_CMN por inferencia): el índice es una
  global sin escritor en el descompilado. Para cerrarlo: leer 0x5CB888 en marcha o volcar la tabla del ELF y poner la
  entrada en `kSonidos`. ADX_CMN: 0-26 mono 24 kHz, 27-89 música estéreo.
- SKILL EDIT nativo de B3 desde la ficha: no viable en esta vuelta (habría que abrir la pantalla de Editar habilidades del
  menú principal y volver al mapa).
- Desbloqueos: `Battle_ApplyResultRewards` pone bits 0xE0/0xE1 en el byte de `DragonMissionStoryModeBattlePointCapsules_
  Moderation` y llama a `Capsule_UpdateUnlockFlagsAt0xC4`; qué cápsula es cada bit sigue sin leer, así que no hay
  equivalente de B3. Fighter's Road: el código no tiene condición (`MainMenuFunc` solo mira la opción 0x13; el menú no
  filtra); se deja siempre abierta.

**Archivos:** `src/pasarela_iw.{h,cpp}` (Ficha, Resultado, SonidoResultado, ayuda del mapa, comentario del vídeo),
`src/pasarela_misiones.cpp` (Zeni, ladrillos, dificultad al minijuego, gancho `DBZ3_TEST_IW_RESMINI`),
`src/iw_minijuegos.{h,cpp}` (`Dificultad(ficha)`), `src/pasarela_b1.cpp` + `src/pasarela.h` (`OirAdxPs2`).
Respaldo: `E:\DBZ3HD_trabajo\F5\respaldo\`. Escenario nuevo: `E:\DBZ3HD_trabajo\F5\f5_ficha.toml`.

**Prueba:** compila (`tools/compilar.py`, sin errores). Todas PASS, capturas revisadas a ojo:
- `D:\DBZ3HD\_pruebas\20261016-112657_modos_test_iw_mapa` (ficha con el menú, resultado de combate rodando).
- `D:\DBZ3HD\_pruebas\20261016-113406_modos_test_f5_ficha` (resultado de ladrillos de muestra: a vacío, b 3 filas, c las
  5 + Zeni; ficha en MISSION START, SKILL EDIT, CONTROLS, MISSION SETTINGS con Hard).
- `D:\DBZ3HD\_pruebas\20261016-113421_combate_test_r1_historia`, `-113043_combate_test_r1_road`,
  `-112717_combate_test_iw_minijuego` (auto=0).

**Pendiente:** índice de la música de resultado y sonidos de fila (arriba); cápsulas de historia; SKILL EDIT nativo;
condición de la dificultad Z.


### F6 Leyenda de Mr. Satán igual que en Budokai 1 (2026-10-16, 11:05-11:45)
Revisado con el descompilado de B1 (`E:\DBZ3HD_trabajo\ghidra\salida\b1.c`) y las tablas del ELF europeo
(`E:\DBZ3HD_trabajo\D\SLES_512.33`, leídas con Python).

**Hecho**
- **Orden de B1:** título (música 1259 a los 50 cuadros, FUN_0022db00) → START → dificultad → prólogo "Mr. Satan's
  Style" (antes iba delante del título) → combate 1 (Yamcha) **directo, sin tarjeta VS ni escena** → resultado →
  tablero → (escena 3D) → combate ... → final. La tarjeta VS ya no existe: los combates se piden solos.
- **Tablero de 11 caras** (escena 0x1C, FUN_00231f20-00232a80) antes de cada retador del 2.º al 11.º: caras de
  668 #24 (vencidos en color con el aspa 668 #17, el resto en silueta), la del siguiente en grande (crece de 0,7 a 1)
  con "Challenger Nth" (668 #16, late como en B1). Tras perder se vuelve a este tablero (Reintentar / Salir).
- **Escena 3D** (petición del usuario) donde B1 pone la charla: justo después del tablero, desde el 2.º retador; Yamcha
  no tiene (en B1 su par de charla es -1,-1). Sin el reproductor conectado, la charla 669 y sus voces salen en el tablero.
- **Voces con los cuadros de B1:** `OirVozB1` admite `-ms` ("la siguiente entra a esos ms"). Voz 2 de la charla a
  80 + {500, 700, 700, 650, 500, 750, 830, 700, 890, 730} cuadros de la voz 1 (tabla 0x350B10, `kVoz2`/`MsVoz2`);
  en el tablero 2D la voz 1 va a los 61 cuadros. Prólogo: voz 1261 a los 90 cuadros, consejos con golpe 1263 y su voz a
  los 139; duraciones 1290, 209 y 139 + 349/239/309/229/369 cuadros; música 79 del demo (FUN_0022e8f0).
- **Resultado** (0x1D, FUN_00231720-00231e60): jingle 1178 a los 30 cuadros, "¡Toma!" y "Total", y la **cuenta
  animada** (33 puntos por cuadro, START la acaba; FUN_00231c40); al acabar va solo al tablero.
- **Final** (0x1E): tras Cell ya no sale un resultado aparte; jingle 1180 a los 10 cuadros, la misma cuenta y con START
  el total (668 #18). **Sin récord** (también quitado del letrero de dificultad y del registro).
- Música y jingles van por `OirAdxPs2` (se mezclan con las voces); se cortan al salir y al empezar cada combate.
- **Globos inventados quitados:** título/dificultad, charla, resultado, derrota y final. Fuera también "VS",
  "Retador N de 11", "Intento N" y "¡Mr. Satán ha caído!".
- **Dificultad: NO se cambia.** Las imágenes 666 #7-9 de la ISO española dicen, en este orden, **"Novel", "Avanzado",
  "Experto"** (captura `a2_dificultad.png` abajo): el texto de reserva de `NombreDificultad` ya era el del original.
  La auditoría lo daba como bug sin haber mirado esas imágenes. Solo se corrige el comentario.

**No hecho (ponytail)**
- **Contraseña:** no es FUN_00234790 (esa pinta la cifra del total). La hace FUN_00235310: 2 números al azar,
  dificultad, si el total cuadra con la suma de los 11, y el total → 12 símbolos de la tabla 0x35F890 tras barajar con
  una de las 16 permutaciones de 0x350B70 (0x35F938...), en un recuadro con 669 #20 (FUN_00234d90). Sus símbolos son
  sprites del modo sin localizar (ids 31-111) y era para la web de Bandai, que ya no existe: no se pone.
- **Sonidos de menú** (5, 3, 0xE tic cada 10 cuadros, 0xF fin de cuenta): son del banco de efectos de B1 (FUN_00152690),
  no del ADX; sin localizar.
- Posiciones exactas de las caras del tablero y la animación del tablón (0x16) sin leer (tabla de sprites del modo):
  el tablón del resultado lleva dentro el "Challenger Nth" del vencido, como antes. Largo de la voz 2 en la escena: 4 s
  supuestos. Detrás del prólogo, el fondo oscurecido (B1: su escenario 3D desenfocado).

**Archivos:** `src/pasarela.cpp`, `src/pasarela_b1.cpp` (`-ms` en `OirVozB1`), `tools/pruebas.py` (escenarios `satan` y
`_r3_cine`: sin tarjeta, tablero, escena, final en 2 capturas; `_r3_cine` por defecto `desde=2`, `desde=1` comprueba
Yamcha sin escena). Respaldos: `E:\DBZ3HD_trabajo\F6\respaldo\`.

**Prueba:** compila (`tools/compilar.py`, sin errores).
- `correr camp_test satan`: **PASS** `D:\DBZ3HD\_pruebas\20261016-113134_camp_test_satan`. Revisado a ojo:
  `a2_dificultad` (letrero original), `d_resultado` (cuenta), `e_tablero` (Yamcha con aspa, siluetas, Ten Shin Han
  grande y "Retador 2º"), `f_escena` (escena 3D tras el tablero). Registro: prólogo tras la dificultad, voces con sus
  `-ms`, combate 1 sin `cinematica`. (El resultado de esta pasada aún no tenía el "Challenger Nth" en el tablón.)
- `correr camp_test _r3_cine --par desde=1`: **PASS** `D:\DBZ3HD\_pruebas\20261016-113608_camp_test__r3_cine`
  (`a_yamcha_sin_escena`: pelea directa, sin escena).
- `correr camp_test satan --par desde=25`: **PASS** `D:\DBZ3HD\_pruebas\20261016-113753_camp_test_satan`
  (`e_final_cuenta`: cuenta hecha, sin récord; `f_final_total`: total 668 #18).
- Sin probar en juego: la escena entera de Ten Shin Han con la voz 2 retrasada (`_r3_cine` sin parámetros), y con
  sonido (las pruebas van en silencio).
