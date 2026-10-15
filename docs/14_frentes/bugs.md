# Frente: ronda de bugs (2026-10-15)

Enjambre de 5 subagentes Haiku (solo lectura): logs de las pruebas de hoy, revision de `main.cpp` + `select_ext.cpp`,
de `src/net/*` + `online_panel.*`, de `campaigns.cpp` + `native_ui.cpp` + `campana.py`, y pendientes de
`docs/14_frentes/*.md` + cvars sin usar. Cada hallazgo se comprobo en el codigo antes de tocar nada; abajo, lo
arreglado, lo descartado (falsos positivos) y las propuestas para otros frentes.

Ficheros tocados: `src/campaigns.cpp`, `src/ingame/online_panel.cpp`, `src/ingame/online_panel.h`,
`src/net/netplay.cpp`, `src/main.cpp`, `src/mods.cpp` (sin dueno, pedido del coordinador), `mod center hd/campana.py`,
`tools/pruebas.py`.

## 1. Arreglado

| # | Fallo (lo que veia el jugador) | Causa | Arreglo |
|---|---|---|---|
| B1 | Tras un combate de una historia de mod con `[forma_jugador]`/`[traje_rival]` (la de Freezer los usa en casi todos), el siguiente Duelo, Tag Team, online o combate del HUB empezaba con esa forma para el jugador 1 y ese traje para el rival, fuera cual fuera el personaje | `campaigns.cpp`: `g_last_battle` (ultimo combate de historia) no se borraba nunca y `tagteam_slots.cpp` `BeforeBattleSetup` lo aplica en cualquier combate (los Duelos no pasan por `sub_8215E9C8`: 0 "se lanza la escena" en los logs de `batalla`) | `ForgetStoryBattle()` al llegar al menu principal (pantalla 5, `OnlineTick`) |
| B2 | Despues de elegir un equipo en el Tag Team de 1P, CREAR PARTIDA y UNIRSE del online fallaban hasta reiniciar el juego con "F4: Tag Team necesita prueba de aborto seguro/relevo identificado" | `tagteam_slots.cpp` guarda el ultimo equipo (`g_mine`) para el online y nunca lo borra; `netplay.cpp` `Host`/`Join` rechazaban la sesion con equipo porque el Tag Team online no esta autorizado | Sin relevo online autorizado el equipo no viaja: 1 contra 1 (`local_team_` vacio, log "no viaja al online"). Los dos lados quedan iguales: `TakeStart` pone los equipos de la sesion |
| B3 | Corte de conexion o desincronia en pleno combate online: ningun aviso, y al acabar el combate tampoco salia la pantalla con el motivo | `online_panel.cpp`: el aviso miraba `desynced()` con el enlace en `kPlaying`, que no se da nunca (`Fail` lo pasa a error y `ResetTransport` borra la marca); la pantalla solo se abria en el cuadro exacto del cambio y fuera de combate | Marca `lost_notice_`: aviso rojo durante el combate y la pantalla ONLINE con el motivo al acabarlo |
| B4 | Tirones en los menus mientras se espera al rival (anfitrion sin IP publica) | `RoomCode()` hacia gethostname + DNS en cada fotograma con `io_mu_` cogido, y el hilo del juego toma `io_mu_` en cada vuelta (`TakeStart`) | `TakeStart` sale sin candado si no hay arranque pendiente; `RoomCode` recuerda la IP local 5 s y la busca sin candado |
| B5 | Mensajes del online solo en espanol (y con jerga: "F4/v3", "plazo real de barrera agotado"...) | textos sin `T::T` en `netplay.cpp` | 9 mensajes traducidos y en lenguaje de jugador |
| B6 | Ventana de "el juego se cerro inesperadamente", la de "no se encontro el ejecutable" y el selector de .iso solo en espanol | `main.cpp` sin `i18n::T` | Traducidos con el idioma del jugador |
| B7 | Tras elegir la .iso en el aviso de arranque, el juego se reiniciaba sin sus argumentos | `CreateProcessW(self, nullptr, ...)` | Se pasa `GetCommandLineW()` |
| B8 | (raro) Cierre por `std::terminate` en una segunda transicion de modulo | `transition_thread_` reasignado estando "joinable" | `detach()` antes de reasignar |
| B9 | (raro) Cache de shaders inicial cortada si el juego se cerraba durante la copia (y no se volvia a copiar) | `copy_file` directo al fichero final | `.tmp` + renombrar |
| B10 | `campana.py`: `[[escena]] anadir` con eventos del mapa (44 = 5 palabras) corrompia el contenedor; el ejemplo de la cabecera (`[44, 0, 5930]`) era justo ese caso | `lst_add` contaba todos los eventos de 12 bytes | `lst_add` mide los eventos con `MAP_EVENT_WORDS` y los empaqueta con `pack_event`; `validar()` rechaza eventos con un numero de valores que no es el del juego; ejemplo corregido; autocomprobacion ampliada |
| B11 | `campana.py`: una historia sin frases en ingles (`dialogo_en`) reventaba al montar | `text_bank(None)` | Cae en las frases en espanol |
| B12 | `campana.py montar`: TOML que falta o mal escrito = traceback; `mod = "..\\x"` escribia fuera de `mods/` | sin control | Mensaje claro; el nombre del mod tiene que ser solo una carpeta |
| B13 | `campana.py` dejaba en `%TEMP%` (disco C) las entradas descomprimidas de cada montaje | `tempfile.mkdtemp()` sin limpiar | `work_dir()`: carpeta de esta ejecucion, se limpia al salir |
| B14 | `pruebas.py`: un `std::terminate` no contaba como error; los respaldos automaticos crecian sin limite (88 hoy) | - | `ERRORES` incluye "std::terminate called"; se quedan los 5 mas recientes por copia, el resto se MUEVE a `D:\_PARA_BORRAR_CLAUDE\pruebas_respaldos` |
| B15 | (pedido del coordinador, visto por el frente HUB) El recuadro ONLINE del Duelo salia sobre el negro al entrar y al salir del Duelo; desde el HUB, la pantalla ONLINE aparecia de golpe con la pantalla aun en negro | `online_panel.cpp` `MenuFade()` contaba 1,5 s desde el cambio del byte de pantalla, que llega con la pantalla aun negra (y el negro dura segun lo que cargue); la salida solo miraba A/B, no Y | Fundido real del juego: float US en `0x8243A278` (0 visible, 1 negro), encontrado comparando la memoria con el brillo de capturas (r = -0,95, `E:\DBZ3HD_trabajo\bugs\analiza_fundido.py`); las filas propias (ONLINE y TAG TEAM) usan `1 - fundido`. Y tambien cuenta como salida del Duelo (con ONLINE elegida, B/Y solo la sueltan: medido). HUB > Online: la pantalla ONLINE espera a que el Duelo se vea y entra fundida en 0,3 s. EU (sin la direccion): la cuenta de tiempo, con 0,9 s de espera en el Duelo |
| B16 | (QoL) Sin datos del juego, el aviso de arranque era tecnico (yae3_xenon.xex, Games on Demand...) y con "No" el programa se cerraba sin dar otra salida | `main.cpp` `OnPreSetup`; el launcher no puede abrirse sin el ejecutable (Runtime::Setup lo necesita) | Texto de jugador (propuesta de QoL) y tres botones: Si = elegir la imagen del disco, No = elegir la carpeta del juego extraido, Cancelar = salir. Lo elegido se guarda y el juego se reinicia; si no vale, lo dice y vuelve a preguntar (antes cerraba). Lo tecnico, al log |
| B17 | (QoL) Errores al instalar un mod .zip en espanol y sin decir que hacer ("ruta del zip invalida"...) | `mods.cpp` `InstallModFromZip`/`PublishMod` | En el idioma del jugador y con la solucion (descargar otra vez, cerrar el juego o el Explorador, mover el juego fuera de Archivos de programa...) |
| B18 | (QoL) `pruebas.py`: dos pruebas seguidas en la misma copia podian leer el log de la anterior | `texto_log` filtraba por fecha (t0 - 5 s) | Foto de los logs de la copia al lanzar; se lee el que es nuevo o ha cambiado desde entonces |

Ganchos de prueba nuevos (solo con la variable puesta):
- `DBZ3_NET_LOCAL=1` (`netplay.cpp`): el anfitrion no usa UPnP ni STUN. Las pruebas cierran el juego a la fuerza y
  no deben dejar puertos abiertos en el router del usuario.
- `DBZ3_TEST_BATTLE_AFTER_DU="id1,id2,escenario"` (`online_panel.cpp`): combate nativo del HUB al volver de Dragon
  Universe al menu principal (`DBZ3_TEST_BATTLE` salta ya en el primer menu principal).
- `DBZ3_TEST_STORY_WORD=palabra` (`campaigns.cpp`): al llegar al menu principal hace como si se volviera de Dragon
  Universe tras ese combate de historia; `DBZ3_TEST_KEEP_STORY=1` deja el fallo de antes (para verlo).
- Escenarios nuevos de `pruebas.py`: `online_crear`, `historia_y_duelo` (`--par antes=1` reproduce el fallo),
  `duelo_fundido` y `hub_online_fundido` (rafagas de capturas en la entrada y la salida).
- Salir de Dragon Universe a mano tras un combate no es practicable en una prueba: la historia de Freezer encadena
  combate, resultados, subir de nivel, premios, dialogos y el siguiente combate sin pasar por el mapa libre (4
  pasadas de exploracion, `_pruebas61015-0308..0325*_bugs_test_campana_y_duelo`). Por eso `historia_y_duelo`
  usa `DBZ3_TEST_STORY_WORD`.

## 2. Como se probo

Copias `D:\DBZ3HD\bugs_test` y `bugs_test2` (instantanea `freezer_cap2`), exe dual compilado con `tools/compilar.py`.

| Que | Resultado | Carpeta en `D:\DBZ3HD\_pruebas\` |
|---|---|---|
| B1 antes (`historia_y_duelo --par antes=1`) | se reproduce: Goku empieza en Super Saiyan (forma 3) y Vegeta con la armadura de Namek (traje 3) en un combate normal | `20261015-033249_bugs_test_historia_y_duelo` |
| B1 despues (`historia_y_duelo`) | PASS: "se olvida el ultimo combate de historia" y Goku/Vegeta normales. Comparativa: `E:\DBZ3HD_trabajo\bugs\antes_despues_historia.png` | `20261015-033332_bugs_test2_historia_y_duelo` |
| B1 regresion: los combates de la historia siguen con su forma | "tu personaje empieza en la forma 2" y "forma 3 / traje 3" en el combate siguiente | `20261015-032542_bugs_test_campana_y_duelo` |
| B2 (`online_crear`, con equipo de Tag Team) | PASS: "no viaja al online: 1 contra 1" y "partida creada"; pantalla ONLINE esperando rival con su codigo | `20261015-030950_bugs_test2_online_crear`, repetida `..._035334_...` |
| B15 Duelo (`duelo_fundido`) | PASS: en negro no hay recuadro; entra y sale con el fondo del juego; con ONLINE elegida, B la suelta y la fila sigue | `20261015-035130_bugs_test_duelo_fundido` (antes: `20261015-024304_hub_test_transicion` del frente HUB) |
| B15 HUB > Online (`hub_online_fundido`) | PASS: la pantalla ONLINE se abre con el Duelo ya visible y entra fundida | `20261015-035132_bugs_test2_hub_online_fundido` |
| B10-B13 `campana.py` | autocomprobacion OK (ampliada); montar la campana de Freezer da los 264 ficheros identicos byte a byte a los del mod instalado; un TOML que falta da un mensaje | `E:\DBZ3HD_trabajo\bugs\mods_nuevo` |
| Bateria `pruebas.py todo` | 5/5 PASS (tag_swap, campana, du_continuar, batalla, menu) | `20261015-033*` |
| B3-B9 | compilado; sin prueba en el juego (hace falta cortar una partida online entre dos PCs, provocar un cierre o una segunda transicion de modulo) | - |
| B16 sin datos (copia minima en `E:\DBZ3HD_trabajo\bugs\sin_juego_exe`, `DBZ3_NO_DIALOGS=1`) | sale con codigo 1 en 0,7 s y el log trae el aviso nuevo. Las ventanas (Si/No/Cancelar y los selectores) no se han abierto: no se sacan ventanas en la pantalla del usuario en las pruebas | `sin_juego.py` |
| B18 dos `menu` seguidos en `bugs_test2` | cada prueba guarda su propio log (04:05:53 y 04:06:49) | `20261015-040552_bugs_test2_menu`, `..._040616_...` |
| B17 | compilado; sin prueba en el juego (habria que soltar un .zip roto sobre el launcher) | - |

## 3. Descartado (falsos positivos de la revision)

- `select_ext.cpp` `kPortraits` / `kSelectGlobal` "solo US": el mapa EU (`D:\DBZ3HD\work\eu\INFORME.md`) las da
  identicas en las dos imagenes.
- `path::string()` lanza con caracteres fuera de la pagina ANSI (comprobado aparte: si lanza), pero el exe lleva
  `activeCodePage UTF-8` (`src/dbz3.manifest`, embebido en el exe): no aplica. Tampoco el "nombre de mod con tilde".
- `campaigns.cpp` ganchos `REX_HOOK_RAW` "no corren en EU": es a proposito (direcciones del mapa solo medidas en US).
- Cvars sin leer: ninguna (78 definidas, todas se leen; 4 por nombre con `Query`/`GetFlagByName`).
- Hilos online sin `join`: todos van con `detach()`; lecturas de paquetes acotadas por `Reader` y `ValidPacket`.

## 4. Propuestas para otros frentes (no tocado: sus ficheros)

### P1. Cierre a los ~20 s de acabar un combate con un port (frentes modos / ports / Tag Team)  [alta]
Dos pruebas del frente de modos murieron igual (`20261015-015122_modos_test_torre_mods`,
`20261015-015446_modos_test_vegeta_gt_sin_modos`): `read of guest 0x00000010`, `lr=0x82089C34`, r4=0x474F7720. 0x82089C34
es el retorno de `sub_82089C10` -> `sub_82099EB8` (liberar memoria del juego): se libera algo ya liberado o que no
es del monton, ~20 s despues de "combate acabado: id 44 contra id 26". El unico `sub_82089C10` propio es
`tagteam.cpp:392` (`SwapAura`, los 48 efectos del aura del bloque del jugador): mirar si el aura de un port (id >= 44,
"auras visibles") deja punteros en `a + 212..` que el juego vuelve a liberar al cerrar el combate. Reproducir con
esa prueba y `DBZ3_FX_VTDIAG`.
Otra muerte distinta: `20261015-011705_tag_test_tag_nativo` (`read of guest 0x8`, `lr=0x8217AAE8`) justo tras
"tag nativo: atras en la rueda -> TAG TEAM" (frente Tag Team; puede estar ya arreglada, pruebas posteriores pasan).

### P2. `src/launcher/settings.cpp:473` (QoL): `dbz3_user.toml` sin escritura atomica  [media]
`std::ofstream os(path, trunc)` sobre el fichero final; si el juego muere a mitad, ajustes perdidos (la
reparacion lo pone en cuarentena). Diff:
```cpp
-  std::ofstream os(path, std::ios::binary | std::ios::trunc);
-  os << out;
+  auto tmp = path; tmp += ".tmp";
+  { std::ofstream os(tmp, std::ios::binary | std::ios::trunc); os << out; if (!os) return; }
+  std::error_code ec; std::filesystem::rename(tmp, path, ec);
```

### P3. `src/launcher/novas_mods.cpp:270` (QoL): zip de Nova's Mods sin comprobar contenido  [media, seguridad]
Solo se mira "PK" y el tamano. Con el boton INSTALAR LO QUE FALTA del online se instala sin mas. Propuesta: SHA-256
en el catalogo y comprobarlo antes de `InstallModFromZip`.

### P4. `mod center hd/roster_build.py:1073` y `write_entry` (ports): temporales en `%TEMP%` sin limpiar  [baja, disco C]
Igual que B13. Hoy hay 215 carpetas `tmp*` (186 MB) en `%TEMP%`. Mismo arreglo: `atexit.register(shutil.rmtree, d,
ignore_errors=True)` tras cada `mkdtemp`.

### P5. `src/tagteam_slots.cpp:91` (Tag Team): `g_mine` nunca se vacia  [baja tras B2]
Ya no bloquea el online (B2), pero si algun dia se autoriza el Tag Team online, el equipo de un Tag Team de hace
horas viajaria al online sin que el jugador lo haya elegido para eso. Propuesta: vaciarlo en `ClearDynamicTeams()`
o al salir del modo Tag Team.

### P6. Online (mio, sin hacer por no ser barato)
- Sin autenticacion: quien sepa el codigo de sala puede ocupar la plaza del rival (lo admite el comentario de
  `netplay.cpp:396`). Idea: 16 bits aleatorios mas en el codigo de sala, comprobados en el HELLO.
- El hash de los mods del HELLO (`SendHello`, `ModFingerprints`) se calcula en el hilo de la interfaz con `io_mu_`
  cogido; con B4 el juego ya no espera por el, pero la pantalla ONLINE se para mientras dura.
- `Close()` cierra el puerto UPnP en un hilo suelto; si se vuelve a CREAR enseguida, ese cierre puede llegar despues
  de la apertura nueva (codigo "router" con el puerto cerrado). Idea: un mutex en `room.cpp` para UpnpOpen/UpnpClose.
