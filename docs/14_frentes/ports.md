# Frente: ports de modelos, movesets y técnicas (2026-10-15)

Copias de prueba: `D:\DBZ3HD\aura_test` (combates) y `D:\DBZ3HD\ports_test` (traje de época). Nunca el juego ni la
partida del usuario. Herramientas de este turno en `E:\DBZ3HD_trabajo\ports\` (ver §6).

## Ronda 3 (2026-10-15 tarde): lo que faltaba para el 100 %

Pruebas en `D:\DBZ3HD\aura_test`, `D:\DBZ3HD\ports_test`, `D:\DBZ3HD\zarbon_test` (solo Zarbon B1) y
`D:\DBZ3HD\voces_b`; capturas en `D:\DBZ3HD\_pruebas\20261015-<hora>_<copia>_<prueba>`. Las carpetas de las rondas 1-2
(anteriores a las 14:00) se movieron a `E:\DBZ3HD_trabajo\ports\pruebas_archivo\` (el disco D se llenó a las 16:28).

### R3.1 Qué ve el jugador ahora
| Antes | Ahora | Prueba |
|---|---|---|
| Fat y Ghost Gotenks «se transformaban» (aura de Super Saiyan del donante sin cambiar de modelo) | Sus cápsulas por defecto y las heredables ya no traen las transformaciones del donante: luchan en su única forma | `143237` (Fat) y `143820` (Ghost): formas vistas [0], 11-12 técnicas, sin transformación |
| Vegeta saga Saiyan como 5.º traje de Vegeta solo cambiaba el modelo | Ese traje juega con los golpes, técnicas (Galick Gun, Final Impact), cápsulas propias (Super Saiyan, 628-631) y aura morada del Vegeta saga Saiyan; en el select sale un rótulo «Traje de época: golpes y cápsulas propios» | `161418` (Torre: SSJ con su cápsula, Galick Gun; `hoja_traje_enlazado.png`), `150808` (rótulo y combate desde el Duelo) |
| Roshi gritaba con la voz de Krilin | Roshi grita con sus voces de Super Dragon Ball Heroes (43 grabaciones) | montaje «gritos propios (wav:gritos)»; en combate `gritos RXADPC en uso` (`155352`) |
| Zarbon B1 salía blanco entero (normal y transformado) | Con sus colores y sombreado | `152348` (transformado), `162736` (normal) |

### R3.2 Cómo se arregló
- **Gotenks** (`roster_build.py`, ports de una forma): con `formas = 1`, las cápsulas de transformación del donante
  salen de `hereda` y de `capsulas`, y `capsulas_forma = [0]`.
- **Traje enlazado (ficha 07)**: `traje.toml` admite `enlazado = "<mod del personaje>"` (lo pone `importar.py traje`;
  `--sin-enlace` lo quita) y el montaje escribe `traje_enlazado = [traje, ID]`. En el juego (`src/ports_guard.cpp`),
  justo antes de montar el combate, el jugador que lleva ese traje pasa a ser ese personaje (traje 0) con sus cápsulas
  por defecto; después la selección vuelve a quedar como estaba (el select recuerda su casilla y su traje; la revancha
  vuelve a pasar por ahí). Dos caminos, los dos cubiertos: HUB/retos (espera de la escena 24: `sub_821D7FE8` →
  `sub_820FE668`) y Duelo del juego (`sub_820FFA80`, que arma la configuración desde la partida). Rótulo: panel
  propio en el select, bajo la ayuda de LT/RT.
- **Roshi**: `awo_tools/cri.py` (nuevo) lee los `.cpk` de SDBH World Mission, sus tablas `@UTF`, los `.acb` (nombre de
  cada sonido → onda) y los `.awb` (AFS2). Las voces de Roshi están en `data_07_win.cpk`: `bckms/CueSheet_BCKMS` y
  `CueSheet_BCKMS_PWUP` (HCA de 16 kHz sin cifrar; los decodifica ffmpeg). Son los sonidos `BKMS_nnn` (43 distintos;
  `KamehameCharge`, `DBZP_...` son efectos). `importar.py` los guarda en `<mod>/gritos/*.wav` y pone
  `gritos = "wav:gritos"`; `gritos.wav_pool` los reparte por duración en el banco de gritos del juego (volumen igualado
  al de las grabaciones de IW). El preset `sdbh.original` ya no fuerza las voces del donante.
- **Zarbon B1** (`awo_tools/rampas.py`, `desde_b1`): los modelos de Budokai 1 no traen rampa de sombreado y su tipo de
  material (0x1BD) no es el de B3 (0x1B5/0x29BD con rampa): el juego los pintaba blancos. Se crea una rampa al estilo
  de B3 por textura (como con B2) y se pone el tipo de los nativos. El montaje lo aplica a todo modelo de B1.

### R3.3 Definitivas de Pan y Janemba, y el aviso del efecto de Goku SDBH
- **Janemba** (Spike Hell, cápsula 598): sale contra Krilin COM (`140309`) y 4 veces contra el muñeco (`161050`), sin
  cierres.
- **Pan** (Reliable Friend, cápsula 612): sale 2 veces contra Videl (`151449`, regla «definitiva directa»,
  `hoja_definitiva_pan.png`) y 4 veces contra el muñeco (`160646`, `161509`). **Una vez el juego se cerró**, a los 16 s
  de su segunda definitiva contra el muñeco (`160646`: lectura de 0x24 en la lista de efectos, `lr 0x821554B0`); al
  repetirlo (`161509`) no pasó. Queda abierto (ver «Pendiente de la ronda 3»).
- **Goku SDBH, efecto 47**: solo aparece cuando su **definitiva acaba estando en Kaio-ken** (forma 1). El Goku del
  juego en la misma situación no da avisos (`142142`: 3 definitivas en Kaio-ken). El banco del efecto no es el aura ni
  la ranura de técnicas del luchador. En pantalla no se ve nada raro (solo falta ese destello). Sin arreglar: sale en uno de cada dos o tres combates (en `164621` no salió) y no se ha localizado qué fichero carga el Goku del juego para ese final; el aviso no cierra el juego.

### R3.4 Voces oídas de verdad (música a 0)
- **Lo que suena es suyo**: los bancos de gritos que monta el juego para cada port, decodificados fuera del juego con
  la misma lógica que usa (`gritos.rx_decode`), traen voz audible: Roshi 51 de 56 huecos (41 s, voces de SDBH), Fat y
  Ghost Gotenks 55 de 55 (45 s, Gotenks de IW), Pan 47 de 55, Janemba 51 de 56, y también Baby, Goku GT, Vegeta GT,
  Future Gohan y Trunks SB2 (`D:\DBZ3HD\ports_test\mods\_roster`). En cada combate de prueba el juego las usa
  (`XmaContext: gritos RXADPC (personaje portado) en uso`).
- **Medida por el registro de audio (`audio pico` cada 5 s, música a 0)**: no basta para decir «este pico es el grito».
  La ventana de 5 s mezcla el grito con golpes, rayos y viento; con el muñeco quieto los dos lados hacen ruido. La
  prueba A/B (mismo combate con las voces del port en silencio: `silencio.py`, `voces_ab.py`) da diferencias del
  tamaño del azar del COM (Roshi: 5 ventanas con pico > 0,9 con voces, 3 sin ellas; Fat Gotenks: 1 y 3). La copia B
  aparte (`voces_b`) no quedó con la música a 0, y la repetición en la misma copia se cortó a las 17:15 (falta de los datos del juego).
- **Conclusión honesta**: el juego carga y reproduce los bancos con voz de cada port (comprobado); que se *oigan* por
  encima del resto no se puede demostrar con este medidor. Hace falta escucharlo (o un medidor por voz en el runtime:
  propuesta, no es de este frente).

### R3.5 Ports desde el HUB con su lista Normal de cápsulas
El conector del HUB ya da a cada luchador sus cápsulas de serie (`dbz3 hub: lado N (id X): n capsulas de serie`).
Probado sin cierres: Torre con Roshi (`153907`, `154759`: 2 combates ganados), Kumite con Janemba (`155018`), Torre con
el traje enlazado de Vegeta (`161418`), Supervivencia con Vegeta saga Saiyan (`163938`: 5 cápsulas de serie, 628-632, se transforma) y todos los combates de prueba de esta ronda (que ya pasan por
ese conector). La excepción es el cierre de Pan de R3.3.

### R3.6 Fichas
| Ficha | Entregable mínimo (texto de la ficha) | Qué hay | Prueba | Nota |
|---|---|---|---|---|
| **07** Trajes de época | «Un personaje con un traje de época: modelo + moveset alternativo + cápsulas + color, jugable y con badge» | Vegeta, 5.º traje «Vegeta (Saga Saiyan)»: modelo de IW por forma, moveset y técnicas de IW, cápsulas propias, aura morada y rótulo en el select. La ficha marcaba el «moveset por traje» como [P] porque faltaba el punto donde el juego elige el moveset: ese punto es el montaje del combate (traje → personaje), y ahí el traje se enlaza a un personaje con todo lo suyo | `161418` (Torre: traje 5 → Vegeta saga Saiyan, Super Saiyan con su cápsula, Galick Gun), `150808` (rótulo en el select y combate desde el Duelo). El rótulo se fotografió con un traje de Goku enlazado a Goku SDBH (solo en la copia de pruebas): en las partidas de prueba Vegeta está bloqueado | **1** |
| **13** Maestro Roshi | «Un traje y una técnica, en Versus/Práctica, movimientos compatibles del donante. Documentar toda sustitución provisional en Mod Kit» | Roshi de SDBH con los golpes y técnicas de Krilin (sustitución documentada en la receta `ports_vision.toml` y aquí) y sus voces de SDBH | `093333` (ronda 2) y la batería WL-13 de abajo | **1**: traje (modelo de SDBH), técnicas de Krilin, en Versus, sustitución documentada; WL-13: los 12 casos PASS y la variante terrestre bloqueada como dice la ficha |

### WL — WL-13 (Maestro Roshi; `studies/11_vision_futura/13_maestro_roshi.md` e `INTEGRACION_SEGURA.md`)
| Caso | Prueba | Resultado |
|---|---|---|
| Contra rival aéreo, Roshi en el lado 1 | Roshi contra Goku, los dos COM (`153522`): Goku vuela y se transforma en Kaio-ken; Roshi le persigue (combos de 23 golpes, 6 técnicas) | PASS |
| Contra rival aéreo, Roshi en el lado 2 | Goku contra Roshi (`154225`): modo hiper y técnicas de los dos | PASS |
| Dos Roshis a la vez (espejo) | Roshi contra Roshi (`154640`): uno en el aire, contraataques, 5 y 7 técnicas | PASS |
| Agarre | En los tres combates los COM agarran y lanzan sin quedarse colgados | PASS |
| K.O. | `155621`: el rival cae y Roshi hace su pose de victoria («WINNER!») | PASS |
| Transformación | Roshi no tiene formas: sin las cápsulas de transformación de Krilin (lista `54 55`) | PASS |
| Relevo de Tag Team sin huecos | Roshi + Krilin contra Goku + Yamcha con relevo cada 10 s (`155816`): Roshi → Krilin → Roshi, ficha del equipo con «Maestro Roshi», sin avisos ni cierres | PASS |
| Revancha | Roshi contra Krilin; al acabar, «Battle Again?» (`173922`): la segunda pelea empieza (estados 7 → 2 → 3 → 4) con los dos | PASS |
| Sin T-pose ni bloqueo por una animación que falte | Ninguna captura de las pruebas la muestra | PASS |
| Cápsulas aisladas del anfitrión | Roshi tiene su lista (`54 55`: las de Krilin sin sus transformaciones); Krilin conserva la suya | PASS |
| Desactivar restaura celda y donante y no toca el guardado | Con la marca `.disabled` el montaje deja 18 personajes (sin Roshi), Krilin pelea con su lista del juego (`165543`), al quitar la marca vuelve (19) y el guardado no cambia (mismo md5 antes y después) | PASS |
| **Variante terrestre** | **Bloqueada**: Roshi usa la locomoción de Krilin (vuela). La ficha la deja bloqueada hasta probar el lanzamiento y la vuelta al suelo; no se ha intentado | Bloqueada (documentado) |

### R3.7 Ficheros tocados en la ronda 3
- Míos: `src/ports_guard.cpp` (traje enlazado en los dos caminos y rótulo),
  `awo_tools/cri.py` (nuevo), `awo_tools/rampas.py` (`es_b1`, `desde_b1`), `mod center hd/importar.py` (`enlazado`,
  `--sin-enlace`, voces de SDBH), `mod center hd/gritos.py` (`wav_pool`, `gritos = "wav:carpeta"`),
  `mod center hd/presets_importar.toml` (`sdbh.original`), `mod center hd/ports_vision.toml` (aura morada de Vegeta
  saga Saiyan), este documento.
- `mod center hd/roster_build.py` (permitido en esta ronda): Gotenks sin transformación del donante,
  `traje_enlazado`, `gritos = "wav:..."`, modelos de B1 en `b3_ramps`.
- **Aviso a campañas**: Zarbon B1 de `presets_test`/`campanas_test` sale bien al volver a montar el roster de esa copia
  (`roster_build.py construir --mods ...`).
- Pruebas nuevas en `E:\DBZ3HD_trabajo\ports\`: `wl13.py` (revancha), `voces_ab.py` (voces A/B), `silencio.py`
  (copia B sin voces y vuelta atrás), `combate_ports.py --cvar` y volcado del banco de un efecto que falta.

### Pendiente de la ronda 3
- Entre las 17:15 y las 17:36 faltaron los datos del juego del proyecto (`us\`, `eu\`; los movió el coordinador y ya
  están): las pruebas de ese rato se repitieron después (revancha, Vegeta GT, batería).
- **Pan, definitiva contra el muñeco**: un cierre en 5 definitivas (`160646`, nodo de efecto nulo en `sub_82155228`,
  `lr 0x821554B0`); no se repitió en `161509` ni en `174501`. Medido: `sub_82152890` saca los nodos de efecto de un
  anillo de 2048 (`0x8242E8B0`, índice `0x823EE804`) sin mirar si siguen en uso; Pan llega a ~1150 por segundo y
  Janemba a ~1200-1990 sin cerrarse (`175025`), así que no es solo cuestión de cantidad. Queda abierto.
- **Goku SDBH, efecto 47** al acabar una definitiva en Kaio-ken (R3.3).
- **Voces**: confirmarlas de oído (R3.4).
- **Batería `python tools/pruebas.py todo`**: 5/5 PASS (repetida a las 17:45, tras reponer los datos).
- **Vegeta GT en la EU** (aviso del frente eu: cierre en su entrada con presentación): su copia `eu_test` tiene un
  roster de las 03:37, anterior a `cam_pad_clips`; la cámara de Vegeta GT allí tiene 24 clips y en las mías 35 (la
  del donante). Con el montaje actual entra bien con la presentación completa (`173557`: estados 1 → 2 → 3 → 4, en
  US). Arreglo para la EU: volver a montar el roster de `eu_test` con el `roster_build.py` actual.
- **Copias**: `aura_test` vuelve a tener su roster con voces (restaurado desde su `mods_respaldo`); `voces_b` es la
  copia B (voces en silencio; los originales en `E:\DBZ3HD_trabajo\ports\ab_respaldo\voces_b`); `ports_test` lleva un
  traje de Goku solo de prueba (`traje_prueba_goku_sdbh`, enlazado a Goku SDBH) para fotografiar el rótulo.
- **`src/ports_guard.cpp` cerrado** para que el frente eu lo pase a la EU: ganchos `sub_821266A8`, `sub_821D7FE8`
  (con su `lr 0x821D7428` y la petición `0x8247A990`) y `sub_820FFA80`; direcciones US: CFG `0x8245EFC0`, partida
  `0x824BA110`, cápsulas del Duelo `0x8245EF92`, `char96 0x8234ABB8`. Todo va con `!dbz3::g_guest_eu`.

## Ronda 2 (2026-10-15 mañana): cierre de la verificación y fichas 07, 08, 13, 20

### R2.1 Cómo se ha probado (sin teclas simuladas)
- `E:\DBZ3HD_trabajo\ports\combate_ports.py` con la presentación nativa del hub (modo 1): captura de las **entradas**
  en el estado 2 de cada combate (los dos luchadores).
- **Técnicas, transformación y definitiva**: `DBZ3_TEST_CAPS` (del hub) equipa las cápsulas del port al jugador 1 y
  la prueba lo pasa a **COM** (hueco de mando `X+5028 = -1`, lo mismo que hace `tagteam.cpp` con el COM; escribir
  `-1` en la CFG del jugador 1 llega tarde: el combate ya la copió) y le rellena el ki. Así el COM del port
  transforma, lanza sus técnicas (el rótulo de la cápsula sale en pantalla) y la definitiva (minijuego «COM ATTACK /
  COM DEFENSE»).
- **Voces**: la prueba lee de la memoria la tabla de voces que el juego carga para cada ID (`0x823280B0`). El
  `audio pico` del log es la mezcla entera (música incluida): no sirve para atribuir una voz.

### R2.2 Fallos encontrados y arreglados (en el montaje)
1. **Yamcha amarillo cerraba el juego al lanzar su definitiva (Spirit Ball) o una técnica**: con su fichero de
   técnicas de IW el juego pedía efectos (12, 13, 14) que no existen y se cerraba (`efecto N que no esta en el banco`,
   `...083523`, `...090253`, `...101408`, `...103244`). Con el fichero del Yamcha del juego no pasa
   (`...110352`: la definitiva dos veces, Kamehameha y Wolf Fang Fist, sin avisos). Sus tres técnicas de IW son las
   mismas que las del Yamcha del juego: receta con `tecnicas = "donante"` (clave nueva en `roster_build.py`: técnicas
   del donante aunque el moveset sea de la fuente).
   Por el camino: los registros de ráfaga de IW (`#CST` 360/361) de Yamcha, Pan y Janemba usan el «banco 6» (el de
   los personajes con aura de Super Saiyan); `bsp_bank6_fix` los copia del primer rayo del personaje (quitó el efecto
   12; no bastó para la definitiva). **Pan y Janemba de IW quedan por probar con su definitiva.**
2. **Vegeta saga Saiyan (y Vegeta GT, Baby, Pan) cerraban el juego al acabar el vuelo de la presentación**: su cámara
   de IW tiene menos clips que la del donante (Vegeta saga Saiyan 30, Vegeta 35) y la entrada pide el 30 → clip nulo
   (`lr 0x820825A8`, `...084624`). Arreglo `cam_pad_clips`: se añaden al final los clips que faltan, del donante.
   Probado: `...091039` (entradas de los dos y combate entero).
3. **Fat y Ghost Gotenks «se transforman» en Super Saiyan sin cambiar de modelo** (cápsula de transformación del
   donante, Trunks): las recetas llevan `formas = 1`, pero **no lo evita** si la cápsula está equipada (`124448`:
   solo cambia el aura). Pendiente: quitar las cápsulas de transformación del donante en los ports de una forma
   (en `roster_build.py`, de modkit; propuesta).
4. **Pelo de Goku SDBH gris claro con brillos**: la textura del pelo venía con alfa 255 en toda la imagen, que en B3
   significa «sin sombrear». `rampas.sombrear_planas` la pasa a la banda sombreada (solo en ports de SDBH; los del
   juego no tienen ninguna así salvo una de Krilin). Ahora sale negro azulado como el Goku del juego
   (`D:\DBZ3HD\_pruebas\20261015-110411_ports_test_combate_port\zz_pelo_despues_antes.png`: izquierda después, derecha antes). También
   afecta a una textura de Goku Black.
- Los arreglos 1, 2 y 4 están en `mod center hd/roster_build.py`, que en esta ronda es del frente **modkit**: son
  pequeños (`bsp_bank6_fix`, `cam_pad_clips`, `tecnicas = "donante"`, `b3_ramps(..., c)`), llamados al montar. Aviso
  a modkit para que los conserve. La lógica de rampas y texturas está en `awo_tools/rampas.py` (mío).

### R2.3 Tabla por personaje (COM con cápsulas; capturas en `D:\DBZ3HD\_pruebas\<carpeta>\hoja_entrada.png` y `hoja_eventos.png`)
«Definitiva» = el log del juego dice `dbz3 combate: ... definitiva ... (capsula N)` y sale el minijuego. «Voz» = la tabla
de voces que carga el juego para ese ID (leída de la memoria); no se han escuchado (juego en silencio).

| Personaje | Entrada | Técnicas | Transformación | Definitiva | Voz | Carpetas (`D:\DBZ3HD\_pruebas\20261015-...`) |
|---|---|---|---|---|---|---|
| Tenshinhan B2 | OK | OK (rayos, 7 gastos de ki) | no tiene | OK (2) | OK: la de Tenshinhan del juego (20) | `083046`, `120214` |
| Yamcha amarillo | OK | OK (Kamehameha, Wolf Fang Fist) | no tiene | OK (1-2), tras el arreglo 1 | OK: la de Yamcha (21) | `083046`, `110352`, `113407` |
| Fat Gotenks | OK | OK (Buster Cannon, Finish Buster, Burning Slash) | solo la del donante (aura, sin cambio de modelo) | OK (6) | OK: Gotenks de IW (16) | `083931`, `122514`, `124448` |
| Ghost Gotenks | OK | OK (mismas, ráfagas) | solo la del donante (aura) | OK (3) | OK: Gotenks de IW (16) | `084242`, `123015`, `125202` |
| Vegeta saga Saiyan | OK (tras el arreglo 2) | OK (Galick Gun, Final Impact) | OK (Super Saiyan) | OK (5) | OK: la de Vegeta (33) | `091039`, `120551` |
| Goku SDBH | OK | OK (Super Dragon Fist) | OK (SSJ3 desde base; las 5 formas vistas antes) | OK (6) | OK: la de Goku (32) | `091039`, `100522`, `115600` |
| Traje de Vegeta (saga Saiyan) | OK (5.º traje) | OK (Big Bang Attack) | OK (SSJ2, SSJ4 del juego) | OK (2) | OK: la de Vegeta | `092044`, `101113` |
| Goku Black (nuevo) | OK | OK (Spirit Bomb) | OK (Rosé, aura rosa) | OK (4) | la de Goku (no hay voz de Black en las fuentes) | `092628`, `113929` |
| Maestro Roshi (nuevo) | OK | OK (Destructo Disc) | la de Krilin (mismo modelo) | OK (2) | falla: la de Krilin (no hay voz de Roshi en ninguna fuente) | `093333`, `121046`, `125903` |
| Yamhan (nuevo) | OK | OK (2 técnicas) | no tiene | OK (1) | la de Tenshinhan | `093805`, `123413` |
| Yamhan, fusión fallida (nuevo) | OK | OK (Neo Ki Blast Cannon) | no tiene | OK (1) | la de Tenshinhan | `095111`, `124019` |

Goku SDBH: 8 avisos de un efecto que no existe (47) al final de un combate (`115600`), sin cierre ni nada visible.


### R2.4 Fichas de la visión
| Ficha | Entregable mínimo (texto de la ficha) | Qué hay en el juego | Prueba | Nota |
|---|---|---|---|---|
| **07** Trajes de época | «Un personaje con un traje de época: modelo + moveset alternativo + cápsulas + color, jugable y con badge» | Vegeta saga Saiyan como 5.º traje de Vegeta (una forma por forma del juego) y también como personaje propio con su moveset, cápsulas y técnicas de IW | `...101113` (traje en la Torre: SSJ2, SSJ4 y Big Bang Attack), `...091039` (personaje) | **0,6**: el traje usa el moveset y las cápsulas de Vegeta; moveset y cápsulas por traje necesitan I+D en el runtime (la ficha lo marca [P]); sin «badge» |
| **08** Goku Black (SDBH) | «Goku Black completo (modelo SDBH + moveset de Goku + Rosé + aura rosa + retrato), jugable» | Goku Black de SDBH World Mission (`bcgkb`: normal y Rosé), golpes de Goku, cápsula «Super Saiyan Rose», aura rosa (18 texturas), icono, retratos y cara del HUD generados del modelo | `...092628` (entrada, Rosé, técnicas, K.O.) | **1**: aura rosa al cargar ki (`113929\zz_aura_rosa_zoom.png`), definitiva 4 veces |
| **13** Maestro Roshi | «Un traje y una técnica, en Versus/Práctica, movimientos compatibles del donante. Documentar toda sustitución provisional» | Roshi de SDBH World Mission (`bckms`, barba = huesos `xKMS_hige`) con los golpes y técnicas de Krilin (Destructo Disc) | `...093333` (entrada, Destructo Disc, K.O. al rival) | **0,9**: sustitución documentada en la receta (`ports_vision.toml`) y aquí; la lista WL-13 completa (agarre, Tag Team, dos Roshis…) sin hacer; voz de Krilin |
| **20** Fusiones de B2 | «Una fusión como personaje independiente y una misión original» | Yamhan y Yamhan de la fusión fallida (B2, sin nombre en el juego), Fat Gotenks y Ghost Gotenks, todos jugables | `...093805`, `...095111` (Yamhan), `...083931`, `...084242` (Gotenks) | **0,5**: la mitad de este frente está hecha; la misión original es del frente de campañas (G2). Gokule no existe en B2 |

Búsquedas (enjambre Haiku + render): Goku Black = `bcgkb` (b00 normal, b01 Rosé), Beerus = `bcbls`, Zamasu =
`bczms`, Zamasu fusionado = `bczmu`, Roshi = `bckms`; Moro y Granolah no aparecen entre los 392 modelos (los
candidatos `bcmoa`, `bcmoc`, `bcgrv`, `bcgrd` son otros personajes, `E:\DBZ3HD_trabajo\ports\sdbh\hoja1.png`).
Roshi no existe en B1, B2, IW ni SB (solo dos dibujos 2D en `modding resources\Budokai 3 Story Art`). El importador
ya lista Goku Black, Roshi, Beerus, Zamasu y Zamasu fusionado (`importar.py lista sdbh`); recetas
`vis_sdbh_goku_black` y `vis_sdbh_roshi` en `ports_vision.toml` (nuevo: `capsulas = [...]` en una receta).


### R2.5 Ficheros tocados en la ronda 2 y avisos a otros frentes
- Míos: `awo_tools/rampas.py` (`sombrear_planas`), `mod center hd/ports_vision.toml` (Goku Black, Roshi, Yamhan,
  Yamcha con técnicas del donante, Gotenks con una forma), `mod center hd/importar.py` (Goku Black, Roshi, Beerus,
  Zamasu y Zamasu fusionado en la lista de SDBH; recetas con `capsulas = [...]`), este documento.
- **Aviso a modkit** (`mod center hd/roster_build.py`, suyo en esta ronda): añadí `bsp_cst_records`,
  `bsp_bank6_fix`, `cam_pad_clips`, `tecnicas = "donante"`, `b3_ramps(..., c)` (SDBH) y la clave de caché
  `rampas3` de la vista previa. Son llamadas al montar; por favor, consérvalas al integrar.
- **Aviso a modkit** (`awo_tools/sdbh_model.py`, suyo): el arreglo del pelo podría ir en la conversión (alfa 0 en las
  texturas de color opacas); hoy lo hace `rampas.sombrear_planas` al montar, solo para mods con `origen_fuente = "sdbh"`.
- **Aviso a campañas** (ficha 20): Yamhan y Fat Gotenks ya son personajes jugables para su misión.
- Batería `python tools/pruebas.py todo`: 4/5 PASS; `du_continuar` falló una vez en el menú de Dragon Universe (no
  llegó a la rueda; las copias smoke no tienen ports) y pasó al repetirlo (`--solo du_continuar`): 5/5.
- Pruebas: `E:\DBZ3HD_trabajo\ports\combate_ports.py` admite `--com1 --killeno` (P1 COM con ki lleno), `--modo`
  (retos de modos.cpp) y captura entradas (estado 2) y el aura al cargar ki.

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

## Voces propias desde WAV (2026-10-16)
- **Uso:** en `personaje.toml`, `voces = "wav:voces"`. Lee `<mod>/voces/NN*.wav`, donde NN es el número de la frase (0-49), por ejemplo `07_victoria.wav` (voces.py, `wav_block`).
- **Formatos:** PCM de 8, 16, 24 o 32 bits, mono o estéreo y cualquier frecuencia. Se pasa a 24 kHz y a ADX (`sb_voces.adx_encode`).
- **Frases sin WAV:** se quedan las del donante.
- **Avisos:** salen los ficheros sin número y los que no se pueden leer.
- **Prueba:** `python "mod center hd/voces.py" --prueba-wav` (ida y vuelta del ADX). Falta probarlo en el juego con un port.
- Los gritos ya admitían `gritos = "wav:gritos"`. El Kit los muestra los dos en las pistas de la ficha.
