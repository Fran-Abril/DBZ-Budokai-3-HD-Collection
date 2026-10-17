# Ronda 3: enjambre (2026-10-15, 23:50) — reglas comunes

Proyecto: port a PC de DBZ Budokai 3 HD (recompilación estática con ReXGlue; ganchos C++ en `src/`, pantallas
propias con ImGui en `src/ingame/` y `src/modos.cpp`/`rpg.cpp`/`pasarela*.cpp`). Contexto: `docs/14_frentes/ROADMAP_RONDA3.md`
(qué hay hecho y qué falta), `docs/14_frentes/FEEDBACK_USUARIO_1.4.6.md`, `AGENTS.md` (busca con Grep, no lo leas entero).
Usuario: no técnico, quiere que TODO se sienta nativo (como los menús de Budokai 3) y que los modos de otros juegos se
PORTEEN (datos y reglas del original leídos de la copia del jugador), no imitaciones.

## Reglas (obligatorias)
1. Toca SOLO los archivos/funciones de tu frente (tabla de abajo). Antes de cada Edit, vuelve a leer la zona: otros
   agentes editan a la vez. Ediciones pequeñas; cada una debe dejar el archivo compilando.
2. Compila SOLO con `python tools/compilar.py` (hace cola; build `out/build/win-amd64-dual`). Si falla en un archivo
   que no es tuyo, espera 90 s y reintenta (hasta 5 veces); no lo "arregles".
3. Prueba en juego con `python tools/pruebas.py` (ver su cabecera) en TU copia: primero `prep <copia>`, y si cambias
   piezas de `res/ingame_ui`, cópialas a `D:\DBZ3HD\<copia>\ingame_ui\`. pruebas.py ya limita a 3 juegos abiertos.
   Para un escenario nuevo, añádelo a `ESCENARIOS` de pruebas.py en una función propia `_<frente>_...` (solo añadir).
4. Nunca borres: mueve a una carpeta de respaldo. Nunca quites contenido nativo. Arte de terceros solo en mods
   opcionales locales. Nada del juego original se reparte: se lee de la copia del jugador en marcha (`native_ui.cpp`).
5. No publiques, no hagas commit/push, no toques `out/build/win-amd64-release` (es la carpeta de pruebas del usuario:
   la despliega el coordinador). Temporales en `E:\DBZ3HD_trabajo\<frente>\`.
6. Ganchos nuevos: usa `DBZ3_HOOK_EU`/`DBZ3_CALL_EU` como el código vecino; si el sub_ no está en `src/eu_map_gen.h`,
   genera su pareja con `python tools/eu_map.py` (mira su cabecera).
7. Si tienes la herramienta Agent, usa subagentes con model "haiku" para buscar y leer; el razonamiento y las
   ediciones los haces tú.
8. Plazo: 40 minutos. Al acabar, el código compila y anotas tu resultado en una sección propia al final de este
   archivo (5-10 líneas: hecho, archivos, prueba con ruta de capturas, pendiente). Responde con eso mismo.

## Frentes
| Frente | Archivos propios | Copia de prueba |
|---|---|---|
| A Interfaz | `src/ingame/game_style.*`, `src/ingame/hub_panel.cpp`, dibujo de `src/modos.cpp` (clase ModosPanel), funciones de dibujo de `src/rpg.cpp` (Lista, ListaTienda, CajaZenie, Lineas, Bocadillo, DibujarEntre, selector, ranking) salvo la pantalla de creación del héroe | hub_test |
| B Yamhan fusión | nuevo `src/fusion.cpp` (+ `CMakeLists.txt` si hace falta añadirlo), funciones exportadas mínimas en `src/tagteam.cpp/.h` | tag_test |
| C Bordes nativos | `rexglue-sdk-0.10/src/graphics/**` (caché de texturas), `tools/verify_release.ps1`, `AGENTS.md` §7 (tamaños de DLL) | grafico_test |
| D Leyenda de Satán | `src/pasarela.cpp`, `src/pasarela_b1.cpp`, `src/pasarela.h` | camp_test |
| E Tablero B2 | `src/pasarela_misiones.cpp`: solo el tablero (kNodos, kAristas, DrawTablero y su lógica `tablero_*`) | escenarios_test |
| F Misiones IW | `src/pasarela_misiones.cpp`: solo los retos de IW; nuevo `src/pasarela_iw.cpp` si conviene | modos_test |
| G Héroe beta | nuevo `src/heroe.cpp` (+ CMakeLists), en `src/rpg.cpp` solo los `case M::kHeroe` y la vista kHeroeP; `assets_propios/generar.py` (piezas del héroe) | du_test |

## Resultados

### E Tablero B2 (2026-10-15)
- **Hecho:** decodificado el `#YDW` de B2 (DATA_CMN 387 = saga Saiyan). Tabla 1 = casillas de 52 B (x, y float sobre
  un mapa de 1280x592; 4 vecinas u16, 0 = ninguna; +0x10 = 2 casilla especial); tabla 2 = piezas de 40 B (+0 personaje
  con IDs de B2 = orden de B1, comprobado con los nombres de la tabla de personajes del ELF; +2 bando: 0 aliado,
  2 rival, 4 llega tarde (Goku); +0x14 casilla). Tablero 0: 16 casillas, 22 caminos simétricos, rivales Raditz,
  Nappa y 9 Saibaman. El modo tablero del HUB lee ahora ese tablero **de la ISO del jugador** (`CargarTableroB2`,
  lector ISO/AFS mínimo propio) y lo dibuja con las piezas del juego; combates por `RequestBattleEx` como antes
  (Nappa = jefe, exige Raditz; casilla especial = semilla). Sin ISO de B2 siguen las 6 casillas propias.
  Avance aparte (`tablero_b2_*`).
- **Archivos:** `src/pasarela_misiones.cpp` (zona tablero + 4 includes), `tools/pruebas.py` (escenario `tablero`).
  Decodificador y dibujos: `E:\DBZ3HD_trabajo\tablero_e\` (dec.py con `demo()`, dibujar.py, tablero_387..395.png).
- **Prueba:** `correr escenarios_test tablero` PASS; capturas `D:\DBZ3HD\_pruebas\20261015-235412_escenarios_test_tablero\`
  (tablero real con caras de Raditz/Nappa/Saibaman, movimiento y charla del Saibaman). Compila (`tools/compilar.py`).
- **Nave de Babidi:** probablemente el `#YDW` 391 (24 casillas; rivales Dabura, Vegeta, Freezer, Cell, Bu).
  Ojo: en 388/389 (Namek) los rivales no llevan bando 2, así que el campo bando aún no vale para todos.
- **Pendiente:** tablas 3-8 (guion/eventos/turnos, cartas de movimiento, Shenlong), qué hace `+0x10` de la casilla,
  movimiento de las piezas rivales y los otros 8 tableros (fase 2-3 del plan, requiere desensamblar `dw_*.c`).

### G Héroe beta (hecho, compila)
- **Pantalla "Crea tu héroe (beta)"** (vista kHeroeP, piezas del juego): Raza (Saiyan, Humano, Namekiano, Raza de Freezer,
  Majin, Androide) → Cuerpo (1-4 luchadores jugables por raza), Apariencia (traje del cuerpo), Maestro (Goku, Vegeta,
  Piccolo, Krilin, Freezer, Cell) y "¡Empezar la misión!". Arriba a la derecha la cara del cuerpo y la del maestro; el
  bocadillo dice lo que da el maestro. Selector > Modo Héroe > A la abre (si la partida no ha ganado ningún combate).
- **Maestro**: bonus por las Reglas (vida extra, daño, recibido, ki que se recarga) + su habilidad (cápsula "para todos":
  Rage!, Serious!!, Spirit!...) y **sus técnicas solo si el cuerpo es el suyo** (en el juego cada técnica va con su
  luchador); con otro cuerpo, el héroe conserva las suyas. La misión (Saibaman, Nappa, Vegeta) usa cuerpo, traje y bonus.
- Archivos: `src/heroe.cpp` (catálogo, receta, reglas, textos), `src/rpg.cpp` (casos kHeroe, DibujarHeroe, 2 líneas en
  Leer/Guardar: claves nuevas `heroe_raza/cuerpo/traje/maestro` en rpg.toml; las viejas se siguen escribiendo),
  `tools/pruebas.py` (escenario `heroe`).
- Prueba PASS: `D:\DBZ3HD\_pruebas\20261015-235130_du_test_heroe` (Krilin con maestro Krilin: log `capsulas del jugador
  2 -> 3 (55 54 445)`, `ki -3/s`) y `20261015-235448_du_test_heroe` (`d_maestro_krilin.png`: Piccolo con maestro Goku,
  combate con `dano 110%`, vida 3500, cápsulas `57 59 61 378`).
- Pendiente: trajes fijos (2 por cuerpo, Goku 3; leer la tabla 0x8245F150 para todos), anfitrión propio
  (`anfitrion_heroe.png` no hecho: sale Kaio), técnicas de maestro para cualquier cuerpo (necesita portar movimientos),
  más misiones y bucle de runs con progreso del héroe.

### Frente A (Interfaz) — 23:58
- Hecho: esquema común de las pantallas de modo `S::mode` (`src/ingame/game_style.h/.cpp`): cinta arriba a la izquierda,
  Kaio abajo a la izquierda pegado al borde (`kHostX/Y`), contenido en la columna 390..1090 a su derecha (no le tapa la
  cara), marcos separados `kGap`, letras `kRow/kHead/kSmall`, bocadillo a la derecha del Kaio con el pico hacia él
  (`Bubble`), ayuda centrada (`Help`), esquina de arriba a la derecha para Zenie / selector de página. `MenuText` con
  ancho máximo (encoge) y colores legibles (no elegidas turquesa claro, apagadas gris claro); `Banner` encoge títulos largos.
- Aplicado a: selector de retos, tienda, cartas del Abyss (nombre corto en la lista, texto entero en el bocadillo),
  simulación, cuadro del torneo, entre combates y final (`src/rpg.cpp`); entre/fin/derrota, ranking y laboratorio
  (`src/modos.cpp`, el laboratorio sin Kaio: tabla ancha). Fondo: `hub/fondo` 16:9 en todos (lab y héroe, los suyos).
- Zenie con las cifras naranjas del juego (`native_ui.cpp` pieza `cifras_zenie`, 2053 #01) y rótulo ZENIE a la izquierda.
- Compila (`tools/compilar.py` sin errores). Pruebas PASS: `tienda` `D:\DBZ3HD\_pruebas\20261015-235439_hub_test_tienda`
  (y selector en `...-235312_hub_test_tienda\a_selector.png`), `hub_lab` `...\20261015-235637_hub_test_hub_lab`.
- Pendiente: sin captura de ranking, final, torneo, Abyss y simulación (mismo esquema, no vistos en juego); el cuerpo
  del bocadillo nativo deja una muesca arriba (hueco del pico en la textura, ya estaba); `Bordes dorados` no probado.

### Frente C (Bordes nativos) — 00:02
- Hecho: con "Bordes dorados" apagado (`dbz3_ui_side_borders=false`, de serie) el SDK sube TRANSPARENTES las dos
  tiras de los menus del juego, con o sin pack HD: `borde` interior (128x1024, 0x4B735AD364D513BA) y la greca dorada
  exterior del marco 16:9 (64x1024, 0xB21DA43CB61A3DCD, hallada con el volcado `E:\DBZ3HD_trabajo\grafico_c\dump`).
  Fuera queda el relleno amarillo del marco (10BD011B92384BD7, 128x1024; se deja: sin el, bandas negras). La 2045 #35
  (D63F4A1F) NO es el relleno (probada transparente con un pack: no cambia nada) -> no se toca.
- Codigo: entrada "virtual" de pack (`Dbz3HiddenEntry`/`Dbz3MaybeHiddenSize` en `dbz3_texture_pack.cpp/.h`), usada por
  `GetTexturePackFactor` de D3D12 y Vulkan; al cambiar el cvar en vivo `BeginSubmission` rehace las texturas (igual
  que pack on/off). SDK compilado: `rexgpu-xenos.dll` 6594048 (rexruntime sin cambios); anteriores en
  `rexglue-sdk-0.10/out/_respaldo_canonicas/20261015_antes_bordes/`; tamano actualizado en `verify_release.ps1` y AGENTS §7.
- Prueba nueva `bordes` (`--par bordes=true|false`, via `REX_DBZ3_UI_SIDE_BORDERS`): PASS ambas. Sin tiras:
  `D:\DBZ3HD\_pruebas\20261015-235836_grafico_test_bordes`; con tiras: `...\20261015-235907_grafico_test_bordes`.
- Pendiente: cambio en vivo desde F1 no probado en juego (mismo camino que F2); Vulkan compilado pero no probado.

### F Misiones IW (2026-10-16, 00:00)
- **Hecho:** la tabla de misiones de IW está en el **ELF** (no en `#DMD`: esas son los iconos del mapa). USA fichero
  0x376D50, registros de 0x60 B: **1-96 Misiones del Dragón** + 97-201 Dragon Road. Campos: +0x02 condición (-1
  entrenamiento, 0 "Defeat" KO sin tiempo, 1 "Fight" bajar vida o aguantar 120 s, 2 dejar ganar, 3 "Endure" 60 s sin
  atacar), +0x08 escenario (tabla del ELF 0x34A430 → mapa de B3), +0x0C tiempo, +0x14 jugador, +0x3A rival. Comprobado
  con las 2 conocidas (Training No.1 = Goku vs **Piccolo** 180 s; misión 43 Endure = Goku vs Gohan joven 60 s) y los
  96 títulos de DATA_USA. Tabla completa en `docs/16_pasarela_modos/03_infinite_world.md` §2.
- El modo "Retos de IW" del HUB > Pasarela lee ahora esa tabla **de la copia del jugador** y encadena las 96 en orden
  (ficha con título/texto de IW + reglas; avance guardado): **73 jugables**; no disponibles, con su ficha: entrenamientos
  2-10 (mandos: guardia/esquiva/persecución), 67, 69, 73-77, 85-87, 92-94, 96 (personajes solo de IW). No hay minijuegos
  en la tabla (vuelo/disparo/MSNxxx son tareas aparte del ELF).
- **Archivos:** `src/pasarela_iw.cpp/.h` (lector ELF carpeta/ISO, IWaB3, MapaIW), `src/pasarela_misiones.cpp` (zona IW,
  campo `Paso::iw`, 2 líneas en la comprobación de victoria, DrawInforme usa `texto_propio`), `tools/pruebas.py`
  (`iw_misiones`). Volcado: `E:\DBZ3HD_trabajo\F\dec.py`.
- **Prueba:** compila; `correr modos_test iw_misiones` PASS: `D:\DBZ3HD\_pruebas\20261015-235631_modos_test_iw_misiones`
  (ficha Training No.1 + combate Goku vs Piccolo en la pradera) y `...-235831_...` (`--par desde=2`: ficha "no
  disponibles 2-10"). El registro lista las 96 (`IW mision N: ...`).
- **Pendiente:** vida mínima real de Endure/Fight (ahora 1000), formas de IW → B3 (todas en forma 0), reglas MSN009/
  010/016/019, entrenamientos 2-10, Dragon Road y saltar a una misión (hoy es una cadena, sin lista para elegir).

### D · Leyenda de Mr. Satán (2026-10-16 00:05)
- **Hecho:** título como el de B1 (logo, dibujo grande de Mr. Satán y "Pulsa el botón START" de la ISO, 666 #5,
  parpadeando) y después el letrero de dificultad; fondo azul de B1 a toda la ventana (sin "Bordes dorados"); tarjeta
  del combate tipo pantalla VS sin el bocadillo que repetía los nombres (aviso del original en su franja, "Intento N" si
  se reintenta); resultado sin el logo duplicado (el tablón de B1 ya lo lleva); dibujo de Mr. Satán con su corte bajo el
  bocadillo; total del final alineado con "Pts.".
- **Cinemáticas:** la ISO de B1 **no tiene vídeos** (ni PSS/SFD/IPU/M2V, tampoco dentro de los AFS; el ELF no nombra
  ninguno). La entrada (`SATAN DEMO MAIN`) y el final (`SATAN CLEAR MAIN`) son pantallas del motor con las imágenes
  666/668/669, que ya se enseñan en su sitio. 669 #20/#21 (contraseña y web de 2002) no se usan. Documentado en
  `docs/16_pasarela_modos/01_leyenda_mr_satan.md` §3 y §5; listados en `E:\DBZ3HD_trabajo\D\`.
- **Archivos:** `src/pasarela.cpp`, `src/pasarela_b1.cpp` (lee 666 #5), `src/pasarela.h` (`ResultadoB1::pulsa`),
  `tools/pruebas.py` (escenario `satan`, `--par desde=25` = último combate y final; `DBZ3_TEST_PASARELA` acepta `seg=N`).
- **Prueba:** compila; `correr camp_test satan` PASS: `D:\DBZ3HD\_pruebas\20261015-235711_camp_test_satan` (título,
  dificultad, tarjeta, combate ganado por ring out, resultado, charla 2, tarjeta 2) y `--par desde=25` PASS:
  `D:\DBZ3HD\_pruebas\20261016-000057_camp_test_satan` (Cell, resultado, final). Necesita `DBZ3_PS2_GAMES`
  (la copia no tiene ps2_games; el escenario lo pone).
- **Pendiente:** puntos por golpe/combo/lanzamiento de B1 (0x1AF6F0: hay que leer la función, no es una tabla); voces
  de las charlas; textos propios (título, resultado, final) solo en es/en.

### B Yamhan fusión (2026-10-16, 00:11)
- **Hecho:** Yamcha (13) o Ten Shin Han (12), en combate normal (no Tag Team ni online), con 3 barras de ki o más y sin
  forma a la que subir: **mantener P+K+G (transformarse) 1 s y soltar** → pasa a Yamhan con el cambio de personaje del
  relevo (cargas del juego, misma vida, gasta 3 barras, pose de entrada = carga de ki). Sin vuelta atrás: el que se
  fusiona queda "caído" en el equipo interno (sin SELECT, sin fichas del Tag Team; el KO de Yamhan es el de verdad).
  Yamhan se busca por nombre interno "YAMHAN" en el roster (hoy id 54); sin el mod no hace nada.
- **Archivos:** `src/fusion.cpp` (gesto, ki, prueba; sonda de botones con `DBZ3_FUSION_SONDA=1`); `src/tagteam.cpp`:
  `Fuse()` exportada, `Team::fused` (vida de ahora, ki conservado tras el reinicio del luchador, que lo devolvía a 3
  barras; sin HUD de equipo ni SELECT); `tools/pruebas.py`: escenario `fusion_yamhan`.
- **Prueba (PASS):** `python tools/pruebas.py correr tag_test fusion_yamhan` (`DBZ3_TEST_FUSION=3`, a los 3 s):
  `D:\DBZ3HD\_pruebas\20261016-001028_tag_test_fusion_yamhan`; con el gesto real (`--par gesto=1`, teclas
  P+L+Space = Y+X+A = P+K+G): `D:\DBZ3HD\_pruebas\20261016-000849_tag_test_fusion_yamhan` (ki 3000 → 0, 165 ticks
  de carga, Yamhan peleando, sin cierres). Ojo: `L+Space+Backspace` de formas.py NO es P+K+G (sale E000).
- **Copia tag_test:** sus mods se apartaron a `D:\DBZ3HD\tag_test\mods_respaldo\mods_antes_fusion_20261015` y se
  copiaron los de `out/build/win-amd64-release/mods` (no tenía vis_b2_yamhan).
- **Pendiente:** animación propia de fusión (hoy: teletransporte del relevo + carga de ki); la fallida (id 55) no se
  usa; COM no se fusiona; tras un asalto nuevo el equipo interno se reinicia (sigue Yamhan).

### Integración (coordinador, 2026-10-16 00:20)
- Build único `out/build/win-amd64-dual` sin errores; DLL canónicas nuevas (rexgpu-xenos 6594048, rexruntime 11145216).
- Pruebas: `todo` 5/5 PASS; `tienda`, `fusion_yamhan`, `satan`, `tablero`, `iw_misiones`, `heroe` PASS
  (`D:\DBZ3HD\_pruebas61016-0014*` y `-0016*`). Desplegado en `out/build/win-amd64-release` (antes en
  `_respaldo_ronda3_enjambre_*`). Sin publicar.
- Visto en capturas: el tablero B2 y la pasarela aún usan el fondo del Duelo y no el esquema común del frente A;
  dos casillas del tablero se solapan cerca del inicio.
- 00:20: la pasarela (tablero, IW, misiones) usa ya el fondo del HUB; las casillas se encogen segun la distancia
  minima, pero las dos de la izquierda (muy juntas en el original) aun se tocan: el marco de style::Frame tiene
  esquinas de tamano fijo. Arreglo pendiente: casilla propia sin esquinas para tamanos pequenos.
