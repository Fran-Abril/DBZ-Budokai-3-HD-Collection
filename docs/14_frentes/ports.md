# Frente: ports de modelos, movesets y técnicas (2026-10-15)

Copias de prueba: `D:\DBZ3HD\aura_test` (combates) y `D:\DBZ3HD\ports_test` (traje de época). Nunca el juego ni la
partida del usuario. Herramientas de este turno en `E:\DBZ3HD_trabajo\ports\` (ver §6).

## Turno 2 (tarde/noche): lo que ve el jugador ahora

| Antes | Ahora |
|---|---|
| Vegeta GT (ID 26 en las copias de prueba) cerraba el juego al salir del combate | Sale al menú sin cerrarse (exe nuevo, guarda en `src/ports_guard.cpp`) |
| Tenshinhan B2: piel casi blanca, sin sombra, y manchas negras | Piel y sombras como el Tenshinhan del juego (todas las rampas de B2 convertidas) |
| Fat Gotenks y Ghost Gotenks: mismo sombreado plano de B2 | Mismo arreglo automático (son modelos de B2) |
| Goku SDBH «no aparecía» | Aparece y pelea en sus 5 formas, con su pose de victoria en cada una |
| Yamcha y Tenshinhan «solo llegaban a la entrada» | Combate entero: golpes, modo hiper, embestida y pose de victoria |
| Vegeta saga Saiyan solo como personaje aparte | También como 5.º traje de Vegeta (`traje.toml`), sin quitar el personaje aparte |
| Dos modelos de B2 sin nombre (XYHS, XYMH) | Son Yamhan y su fusión fallida (delgada). Gokule no existe en B2 |

### 1. Cierre de Vegeta GT al acabar el combate (causa raíz y arreglo)
- **Causa.** Al empezar el combate el juego guarda en una ranura por luchador los recursos de su fichero de técnicas
  (BSP): los hijos de `[0]`, de `[1]` y los de la raíz desde el 2 (`sub_82126490`). La ranura tiene sitio para
  **64** (dos listas de 64: punteros a los recursos y modelos creados). Los BSP del juego llegan a 52; el de Vegeta GT
  de Infinite World tiene **65** (15 + 19 + 31; es el único de los 50 BSP de IW que pasa de 64). El recurso 65 (un
  `#CWV` del fichero) cae en la primera casilla de la lista de modelos. En combate no se nota (las búsquedas lo
  encuentran), pero al salir `sub_821266A8` libera esa lista con `sub_82089C10` y libera un bloque del fichero que no
  es memoria propia: lectura de `0x10` y cierre. Encaja con el registro del frente de modos: `lr 0x82089C34`,
  `r31 = 0x474F7720` = el `#CWV` en `+0xA1700` del BSP (el último `#AZT` de la carga está en `+0xA1520`).
- **Arreglo.** `src/ports_guard.cpp` (fichero nuevo de este frente, añadido a `CMakeLists.txt`): gancho de
  `sub_821266A8` (nadie más lo engancha) que, antes de liberar, pone a 0 las casillas desbordadas (eran datos del
  fichero, no modelos). Avisa en el log: `dbz3 ports: tecnicas del lado N con 65 recursos (el juego admite 64)`.
  `roster_build.py` avisa al montar cualquier BSP de más de 64 (`bsp_resources`), y la auditoría
  (`auditar_importar.py`) recoge ese aviso sola.
- **Probado** con el exe compilado (`tools/compilar.py`, OK): `pruebas.py correr aura_test batalla --par ids=44,26,6`
  → PASS, el log trae el aviso y `hub: fin del combate nativo ... al menu principal` sin errores
  (`D:\DBZ3HD\_pruebas\20261015-033819_aura_test_batalla`). La reproducción rápida del frente de modos
  (`batalla --par ids=26,0,2`, Vegeta GT de P1) también pasa: aviso del lado 0 y vuelta al menú
  (`...20261015-042108_aura_test_batalla`). Antes: cierre en el mismo punto (pruebas del frente de
  modos 015122 y 015446). Vegeta GT en Super Saiyan 4 contra Vegeta, con KO y pose: ver §3.

- **Respuesta a la pista P1 de `bugs.md` §4** (¿el aura del relevo de `tagteam.cpp`?): no. Es el mismo cierre
  (`lr 0x82089C34`, `r4 = 0x474F7720`) y la causa es la ranura de técnicas de Vegeta GT, no el aura; pasa también
  sin Tag Team y sin modos. No hace falta tocar `tagteam.cpp`.
- Otros ports: de los 50 BSP de Infinite World solo el de Vegeta GT pasa de 64; los de Shin Budokai de las
  auditorías llegan a 52; los de la copia de prueba, a 52.

### 2. Sombreado de los modelos de Budokai 2 (Tenshinhan, Fat Gotenks, Ghost Gotenks)
- **Causa.** En B3 HD el color es `base − rampa` (la rampa lleva el color de la sombra en RGB; piel nativa = rampa
  azul). Los modelos de B2 traen otra convención: la base ya coloreada y la sombra **en el alfa** de la rampa, con el
  RGB a 0 y la **última columna blanca**. En B3 eso daba piel sin sombra (casi blanca con el brillo HD) y, donde la luz
  caía en la última columna, `base − 255` = **negro** (las manchas de la nuca).
- **Arreglo.** `awo_tools/rampas.py` (nuevo): detecta por formato las rampas de B2 y escribe rampas RGB nuevas con la
  sombra de B2, calibradas con el Tenshinhan nativo (luz −10 %, sombra cálida). Si dos materiales con bases distintas
  comparten rampa, cada uno recibe la suya (texturas nuevas al final del `#AZT`, máximo 32 por modelo; los nativos
  llegan a 26). Quita también el bit `0x2000` del tipo de material de B2 (no existe en B3). Lo aplica
  `roster_build.py` al montar (`b3_ramps`, personajes y trajes) y en la vista previa (iconos/caras del Tag Team),
  así que **los mods ya importados se arreglan solos al pulsar JUGAR**. Los modelos de B1, IW, SB y SDBH no cambian
  (comprobado: 0 rampas de B2 en todos los ports de la copia).
- **Probado:** `D:\DBZ3HD\_pruebas\20261015-034613_aura_test_combate_port\zz_comparacion_tsh_b2_vs_nativo.png`
  (los dos Tenshinhan juntos) y la pose de victoria de cerca en `...040057_aura_test_combate_port\020_victoria_7.png`.

### 3. Combate completo de los seis ports nuevos
Método (todo automático, en segundo plano): combate nativo con `DBZ3_TEST_BATTLE` y lectura de la memoria del juego
cada 0,1 s (acción, vida, ki, forma, modo hiper) con capturas por tiempo y por evento.
- «COM»: el port lo maneja el ordenador contra un nativo quieto → golpes, combos, modo hiper, embestida.
- «Victoria»: el port de P1 en la forma pedida (6.º valor de `DBZ3_TEST_BATTLE`, del frente hub) + `DBZ3_TEST_KO=1`
  → pelea, K.O. y su pose de victoria («WINNER!»).

Capturas en `D:\DBZ3HD\_pruebas\<carpeta>` (`hoja.png` = hoja de contacto de cada prueba).

| Port (ID en la copia) | Golpes y combos | Modo hiper / embestida | Formas vistas en combate | K.O. y pose de victoria | Carpetas |
|---|---|---|---|---|---|
| Tenshinhan B2 (47) | sí | sí (3 veces el COM) | base | sí | `20261015-034613` (COM), `-040057` (victoria) |
| Yamcha traje amarillo (49) | sí | sí, con embestida | base | sí | `-024746` (COM), `-040230` (victoria) |
| Fat Gotenks (45) | sí, y un rayo de ki | sí, con embestida | traje 1 | sí | `-040349` (victoria), `-041558` (COM) |
| Ghost Gotenks (46) | sí, ráfagas de ki | sí, con embestida | base | sí | `-043047` (victoria), `-041813` (COM) |
| Vegeta saga Saiyan (48) | sí | embestida | base y Super Saiyan | sí, en las dos | `-040553`, `-040723`, `-041322` (COM) |
| Goku SDBH (50) | sí | sí, con embestida | las 5: base, Kaio-ken, SSJ, SSJ2, SSJ3 | sí, en las 5 | `-034845` (COM), `-035257`, `-040857`, `-035810`, `-041003`, `-041116` |
| Vegeta GT (26, extra) | sí | — | Super Saiyan 4 | sí | `-041218`; salida al menú sin cierre: `-033819_aura_test_batalla` |

Técnicas: el COM de los ports lanza ráfagas y rayos de ki (Fat Gotenks, rayo azul en `-041558`, captura `011_com.png`),
entra en modo hiper y hace la embestida. Las **técnicas de cápsula y la definitiva no se pueden pedir en este
combate de prueba**: los dos luchadores salen sin cápsulas equipadas (lista vacía en la memoria) y dirección + ki
desde la prueba solo lanza una ráfaga (`tecnicas.sh`, `...042251`, `...042451`, `...042616`).

**No se puede comprobar con este método** (y lo que queda para la prueba con el usuario):
- **Entrada**: el combate de prueba del hub va directo a «READY?» (no hay presentación de personajes). Propuesta al
  frente hub: un 9.º valor en `DBZ3_TEST_BATTLE` para no saltar la presentación.
- **Transformarse en combate y la definitiva**: las entradas simuladas de 3-4 botones no se registran (P+K+G salió
  como puñetazo) y en estos combates no hay cápsulas equipadas (el selector de forma lo dice: `motivo 8`). Las formas
  se han visto todas empezando en ellas (tabla). El COM no se transformó en ningún combate.
- **Voces**: las copias van en silencio (usuario en el PC). Salen de la tabla del roster: Vegeta GT y los IW con
  voz propia en IW la llevan; Vegeta saga Saiyan, Yamcha, Tenshinhan y Goku SDBH usan la de su personaje del juego
  (el donante es el mismo personaje); Fat y Ghost Gotenks, desde hoy, la de Gotenks de IW (§4b).

### 4. Vegeta saga Saiyan como traje de época
- Comando nuevo: `python importar.py traje vis_iw_vegeta_saiyan --personaje 7 --nombre "Vegeta (Saga Saiyan)"`.
  Crea `mods/traje_vis_iw_vegeta_saiyan/traje.toml` con **un modelo por forma** del Vegeta del juego, siguiendo la
  tabla de formas del juego (nuevo en `roster_db.json`: `form_models` por ID y `form_ids`, sacados de la imagen US):
  base y Super Saiyan = los del port; SSJ2 = el del SSJ (como en el juego); **Majin** = el del SSJ con los huesos
  renombrados `VGT_` → `MVG_` (solo en el `#AWO`); **SSJ4 (ID 92) y la forma 5 (ID 89)** = los modelos del juego
  tal cual (`"juego:418"`, `"juego:96"`, nuevo en `build_trajes`), porque el port no tiene esas formas.
- El personaje aparte (ID 48) sigue igual.
- **Probado** en `ports_test` (Torre con `DBZ3_TEST_MODO=torre,7,4`, montaje completo con el código final): Vegeta
  del juego con su 5.º traje, la armadura de la saga Saiyan, pelea y pierde por ring out con su pose de derrota
  (`D:\DBZ3HD\_pruebas\20261015-043340_ports_test_traje_vegeta\hoja.png`; antes `...042200`). Log del juego:
  `forma 0 -> 4115, forma 1 -> 4116, forma 2 comparte el modelo 1, forma 3 -> id 92 = 418, forma 4 -> 4117,
  forma 5 -> id 89 = 96`. Solo se ha visto la forma base en combate (la prueba no deja elegir traje y forma a la vez).

### 4b. Voces de Fat Gotenks y Ghost Gotenks
- B2 no trae sus voces a mano y el donante (Trunks del futuro) no es su voz. Las recetas de `ports_vision.toml`
  ahora ponen `voces = "iw:GOTENKS (GOTEN)"` (Gotenks de Infinite World, 16 grabaciones; si no está el disco de IW,
  el montaje vuelve solo a las del donante). Puesto también en las dos copias de prueba; comprobado al montar
  `ports_test`: tabla de 50 voces para los IDs 45 y 46 en `roster.toml` (antes, ninguna). No se han oído (juego en
  silencio).

### 4c. Carpetas temporales (propuesta P4 de `bugs.md`)
- `roster_build.work_dir()` (nuevo): las carpetas de trabajo de `roster_build`, `importar` y `colores` van a
  `DBZ3_TMP`, `E:/DBZ3HD_trabajo/tmp` o `D:/tmp` (la primera que exista; si no, el temporal del sistema) y se borran
  al acabar el proceso. Las 31 carpetas `importar_*` que quedaban en `%TEMP%` (225 MB en C:) se movieron a
  `D:\_PARA_BORRAR_CLAUDE\temp_importar_2026-10-15` (sin borrar).

### 5. Yamhan en Budokai 2 (ficha 20)
- Render de los dos modelos sin nombre: `E:\DBZ3HD_trabajo\ports\b2_desconocidos\hoja.png`. Los dos llevan el tercer
  ojo de Tenshinhan, el mechón de Yamcha y el chaleco de fusión: **XYMH = Yamhan** (musculoso; tiene moveset propio
  en B2, `YMH`, entrada 318) y **XYHS = Yamhan de la fusión fallida** (delgado, como Fat Gotenks; sin moveset).
  Gokule (Goku + Mr. Satan) no tiene modelo en B2.
- `importar.py` ya los nombra (y donante Tenshinhan) y `ports_vision.toml` trae sus recetas (`vis_b2_yamhan`,
  `vis_b2_yamhan_fallida`), sin montar.

## Ficheros tocados (todos de este frente)
- `src/ports_guard.cpp` (nuevo) y `CMakeLists.txt` (solo la línea del .cpp).
- `awo_tools/rampas.py` (nuevo).
- `mod center hd/roster_build.py`: `b3_ramps`, `bsp_resources` (aviso de más de 64), vista previa con rampas,
  `rampas.py` en el hash de entradas.
- `mod center hd/importar.py`: comando `traje` (+ `_relabel_awo`), nombres y donante de Yamhan.
- `mod center hd/roster_build.py` (`build_trajes`): modelos `"juego:N"` en `traje.toml` (el del juego tal cual).
- `mod center hd/roster_db.json`: `form_models` por ID y `form_ids` (solo números, de la imagen US; mismo formato).
- `mod center hd/ports_vision.toml`: recetas de Yamhan; voces de Gotenks de IW para Fat y Ghost Gotenks.
- `mod center hd/colores.py`: carpeta de trabajo con `work_dir`.
- `roster_build.py` e `importar.py`: carpetas de trabajo con `work_dir` (se borran al acabar).

## Propuestas para otros frentes
- **Tag Team** (iconos de los IDs ≥ 44 con una letra): ya resuelto por su frente (`tagteam_panel.cpp`, `PortFace`
  lee `mods/<mod>/ui/_vista/hud.png`); todos los ports de la copia tienen esa cara. Con este turno la vista previa
  de los ports de B2 sale con sombra (se regenera sola: clave de caché `rampas2`). Nada más que hacer.
- **QoL / Mod Kit** (`ImporterPage.PRESET_CHECKS`): no hace falta cambiar nada; no hay opciones nuevas (el arreglo de
  rampas es automático al montar). Si se quiere el traje de época en el Kit: un botón «También como traje de…» que
  llame a `importar.py traje <mod> --personaje <ID>` (sale una línea `traje\t<mod>\t<id>\t<nombre>\t<n>`).
- **Hub** (`src/hub.cpp`): opción de presentación en `DBZ3_TEST_BATTLE` (ver §3) y, para probar transformaciones y
  definitivas, equipar las cápsulas por defecto del personaje en el combate de prueba.
- **Runtime EU**: `ports_guard.cpp` solo engancha la US (falta la dirección EU de la base de ranuras `0x82333994`).

## Pendiente
- Entrada, transformación en combate, definitiva y voces: prueba manual con el usuario (o con las propuestas de §3).
- Voces propias de B1 (B1 solo tiene gritos; las frases quedan en silencio, `voces = "ninguna"`), `MAX_FORMS = 6`
  y `tecnica_clon.py`: sin tocar en este turno.
- Normales «vía ps2hd»: no hacía falta para Tenshinhan (sus normales ya son las de PS2, igual que el nativo: 115 no
  unitarias en los dos); las manchas negras eran la rampa (§2).
- Pelo de Goku SDBH: algo gris con brillos blancos frente al negro azulado del Goku del juego, y en Kaio-ken todo el
  modelo sale rojizo, también el pelo (así viene el modelo de SDBH). Es de la conversión de SDBH (`sdbh_model.py`).

## §6 Herramientas de prueba (E:\DBZ3HD_trabajo\ports)
- `combate_ports.py COPIA P1 P2 [--humano] [--teclas ...] [--env ...]`: combate con monitor de memoria y capturas.
- `victoria.sh COPIA ID RIVAL FORMA`, `tecnicas.sh`, `lote.sh`: lotes usados en este turno.
- `hoja.py` (hojas de contacto), `mat_dump.py`, `tex_dump.py`, `dibujos.py`, `arbol.py`, `bsp_cuenta.py`,
  `refs.py`, `reescribir.py` (rehace solo las entradas de un port en el `_roster` de una copia, para iterar).

---

## Turno 1 (madrugada del 2026-10-15)

### Arreglado y comprobado en el juego

#### 1. Cara de la barra de vida de Fat Gotenks
- No era la cara del donante (Trunks). La hoja que se carga (data_cmn 4069) ya era de Fat Gotenks.
- El problema era el encuadre. Su cabeza es muy alta, así que con el mentón a la altura de siempre los ojos quedaban fuera de la barra. Solo se veían la boca y la barbilla, en tono lila, y parecía otra cara.
- Arreglo en `roster_build.py`, función nueva `hud_dy`. Si el usuario no pone `hud_ajuste`, la cara se baja lo que el hueso HEAD pase de 28 px sobre el mentón.
  - Las 38 caras oficiales están entre 12 y 29 px, y los ports anteriores entre 12 y 26 px: no cambian.
  - Fat Gotenks está a 45 px: baja 17 px y los ojos quedan a la altura de las oficiales.
- Comprobado: `D:\DBZ3HD\_pruebas\20261015-013657_aura_test_tag_lento\20_combate.png` (P1 con sus ojos en la barra).

#### 2. «Hojas blancas» en la mano de Fat Gotenks
- Eran las dos colas del cinturón (huesos LOBI1-4 y ROBI1-4 de B2). En el modelo de reposo salen en horizontal.
- El moveset del donante (Trunks) no tiene pista para esas colas, y B3 no aplica física a los modelos nuevos, así que se quedaban tiesas junto a la mano.
- Arreglo:
  - `awo_tools/altura.py`, función nueva `hang_tails`: gira 80° la raíz de cada cola en el reposo (lo mismo que ya hacía `sdbh_model` con SDBH).
  - `importar.py` la aplica a los modelos de B1, B2 e IW (log: «cinturon: colas colgadas en N modelos»).
  - Si el moveset sí anima las colas, su pista sustituye a este giro: no hay efecto.
- Comprobado: las colas cuelgan pegadas a la cadera (`20_combate.png`, `21_carga.png`). Queda un hilo fino visible de canto; se puede afinar con 90°.

#### 3. Recetas de `ports_vision.toml`
Todas importadas con `importar.py oficial ... --recetas ports_vision.toml --mods D:/DBZ3HD/aura_test/mods`, y el roster montado sin errores (IDs 45-50).

| Receta | ID | Juego |
|---|---|---|
| Fat Gotenks | 45 | OK: modelo, cara HUD y cinturón (capturas 20-22). |
| Ghost Gotenks | 46 | OK: entra por relevo con SELECT y su cara sale en el HUD (`24_relevo.png`). |
| Vegeta (Saiyan Saga) | 48 | OK: modelo, cara HUD y técnicas de IW (lanza «Final Impact» con aura visible, `24_relevo.png`). |
| Tenshinhan B2 | 47 | Montado, con moveset de B2 portado (14 técnicas tras combo). En el juego carga y hace su entrada (`D:\DBZ3HD\_pruebas\20261015-014328_aura_test_tag_lento\24_relevo.png`). A revisar: manchas negras en la nuca y piel muy blanca. Sin captura del combate: el PC iba a pocos fps. |
| Yamcha amarillo | 49 | Montado, con técnicas de IW (Kamehameha, Wolf Fang Fist, Spirit Ball). En el juego carga y hace su entrada (`...014328...\20_combate.png`); el modelo se ve bien. Sin captura del combate. |
| Goku SDBH | 50 | Montado: 5 formas con las cápsulas de Kaio-ken y SSJ1-3 (golpes de Another Road). En el juego sin ver: iba de compañero y la prueba no llegó al relevo. |

- Las caras HUD generadas de los 7 ports nuevos tienen el encuadre de las oficiales: `scratchpad/ports_front/huds_new.png`.
- Primera prueba con Tenshinhan B2 como rival: no pasó de la presentación del escenario. Coincidió con el PC muy cargado (4-20 fps, varios juegos de otros agentes abiertos), y con Vegeta nativo pasó lo mismo. No era un fallo del port.
- `scratchpad/ports_front/tag_lento.toml` es un escenario con esperas largas para cuando el PC va cargado.

#### 4. Peticiones del frente Mod Kit
- `importar.py oficiales` acepta `--mods`: dice si cada port está instalado en esa carpeta.
- Cabecera de `ports_vision.toml` al día: se usa con `--recetas` igual que `ports_oficiales.toml`.

### Pendiente (no hecho en este turno)
- **Aura «visible por defecto»:** el aura de los ports sale (Vegeta Saiyan con técnica, y ya validado en Goku GT el 14-10). Mi tecla de carga de ki en la prueba (P+Quote) no carga, así que no la he comprobado en Fat Gotenks.
- **Normales y sombras:** `awg_normal_fix.py` solo sirve para la antigua «Vía A». Copia normales de un modelo nativo con el mismo número de vértices, y los ports por `ps2hd` no tienen esa pareja. Para ports hay que recalcular las normales suavizadas por vértice en `ps2hd` (propuesta, sin hacer).
- **Pies en el suelo de los ports GT:** `altura.ground_moves` (GROUND_CODES 2, 15, 16, 17) ya está conectado en `roster_build`. No lo he medido de nuevo en el juego.
- **Voces propias de B1** y **formas recortadas al donante (MAX_FORMS = 6)**: sin tocar.
- **Voces de B2:** el preset `b2.original` usa `voces=donante`.
- **Técnicas (estudio de `docs/15_tecnicas/`):** recibido el aviso del coordinador. `awo_tools/tecnica_clon.py` (clonar o mover técnicas, boceto en 04 §3) queda sin hacer por tiempo.

### Ficheros tocados (todos de este frente)
- `mod center hd/roster_build.py`: `hud_dy`, y `hud_images` la usa.
- `mod center hd/importar.py`: colas del cinturón, y `--mods` en `oficiales`.
- `awo_tools/altura.py`: `hang_tails`.
- `mod center hd/ports_vision.toml`: cabecera.

### Propuestas para otros dueños
- `src/` (iconos del relevo en el HUD de Tag Team): los IDs ≥ 44 muestran una letra (G, V, F) en vez del icono de la rueda. El icono ya existe en el roster, así que se podría usar ese.
