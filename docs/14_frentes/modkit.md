# Frente: Mod Kit (2026-10-15)

## Ronda 2 (2026-10-15, tarde): fallos de jugadores con la 1.4.5.2

Ficheros tocados: `mod center hd/swap_b3.py`, `roster_build.py`, `model_render.py`, `modkit_gui.py`, `texture_b3.py`,
`log_en.py`, `awo_tools/sdbh_model.py` y `awo_tools/afs_pair.py` (este último no es de ningún frente: una función,
`hd()`, ver §1). Nada publicado ni subido; pruebas en `E:\DBZ3HD_trabajo\modkit\` y en la copia `D:\DBZ3HD\modkit_test`.

### 1. Vista previa del Kit: «IndexError: list index out of range»
- **Causa**: las herramientas cogían el primer `us\data_cmn.afs` que hubiera junto al Kit, fuera cual fuera. El
  jugador (traceback de la 1.4.2.x: línea 925) tenía otro `data_cmn.afs` (el de Infinite World tiene 1136 entradas;
  el de PS2 tiene 3990 pero sin LZX; Budokai 1, cortado...). Las plantillas de retrato piden hasta la 3961.
  El mismo fichero malo daba plantillas basura al importar de Heroes (`afs_pair.hd`) y quedaban en la caché de
  `%TEMP%\afs_pair_cache` (la clave no llevaba el fichero de origen).
- **Arreglo de raíz** (`swap_b3.py`, que importan todas las herramientas):
  - `cmn_problem(path)`: es el bueno solo si es AFS, tiene ≥ 3990 entradas y la entrada 0 va en LZX (descarta PS2).
  - `find_us(prefer, roots)`: busca el bueno en la carpeta pedida, la carpeta de datos del launcher
    (`dbz3_game_dir` de `dbz3_user.toml`, con comillas simples o dobles; región `eu` primero si toca), las raíces
    dadas y la del Kit, subiendo 4 niveles; en cada una `us`, `DBZ3\us` (volcado del disco), `assets\us`,
    `assets\DBZ3\us` y sus `eu`. `DEFAULT_AFS` sale de aquí (antes: primer fichero existente, sin mirar nada).
  - `b3_afs(path)`: si la carpeta de un AFS del juego (`data_*`, `lang_*`, `adx_*`) no es la buena, usa el mismo AFS
    de la buena con un aviso; si no hay ninguna, `NoGameData` (un `SystemExit`) con un **mensaje claro en 6 idiomas**
    (es, en, fr, de, it, pt; idioma: `DBZ3_LANG` que pone el Kit, `dbz3_language` del launcher o el de Windows).
  - Pasan por ahí: `roster_build.Afs` (todas las lecturas de campana, colores, importar, png2pic, construir, equipo,
    vista), `roster_build.us_dir()` (construir lo resuelve solo si hay personajes que montar), `swap_b3.py` y
    `texture_b3.py` (`--afs`; `texture_b3` usa ya el `DEFAULT_AFS` de `swap_b3`, antes no miraba `eu`) y
    `afs_pair.hd()` (plantillas de Heroes y Studio; caché con la clave del fichero de origen y, si existe
    `E:\DBZ3HD_trabajo\tmp` o `D:\tmp`, fuera de C:; la vieja `%TEMP%\afs_pair_cache`, 5,3 GB y ya sin uso, sigue en
    C: para que el coordinador la aparte).
- **Vista previa que nunca revienta** (`roster_build.preview`): sin datos del juego hace igual el icono (sin
  accesorios nativos) y el rótulo, y los retratos dan el mensaje claro; cualquier otro fallo deja el detalle en el
  registro y una última línea `ERROR: vista previa: …` (excepción `VistaError`; `nuevo` la sigue tomando como aviso).
  Si el modelo no se puede dibujar, el error dice por qué (antes solo «falta ui/cara.png»).
- **Mod Kit**: «Comprobar instalación» marca en rojo un `data_cmn.afs` que no sirve, con el mismo mensaje; la
  carpeta de datos se busca con `find_us` (también la del launcher y `DBZ3\us`); las herramientas reciben
  `DBZ3_LANG`. `log_en.py`: frases nuevas en inglés.

### 2. Heroes (SDBH): «no genera la imagen 3D»
Xeno Janemba es `model/bcjno` (b00 normal, b01 con la armadura oscura); no estaba en la lista del importador (solo los
24 personajes con equivalente en Shin Budokai; desde §5 ya sale), así que se creaba con `sdbh_model.py convertir` +
«Crear con mis modelos». Probado con las dos formas y **los 38 donantes con modelo** (0-43 menos las plazas vacías): fallaban con
Trunks niño (9) y Omega Shenron (41).
Barrido de los **376 personajes** (forma b00, donante Goku) y después de **todas las formas** (ver §Pruebas):

| Fallo (antes) | Causa | Arreglo |
|---|---|---|
| Cualquier modelo de Heroes con donante Trunks niño u Omega Shenron | su hueso de cuerpo es `XTRS_BODY_1` / `1SD_BODY_1`: sin rampa de ropa → `TypeError` | `sdbh_model`: `BODY(_n)`, y si falta, la rampa más usada |
| 6 personajes (`bcbbr`, `bcmag`, `bcmnb`, `bcmnh`, `bctlh`, `bctls`) | ficheros `_xBBR_BODY.emd` en mayúsculas → «no hay cuerpo» (`StopIteration`) | `load_source` normaliza `_BODY`/`_FACE` |
| 3 personajes (`bcmag`, `bcdcw`, `bcpet`): convierten pero sin icono ni retratos | nada pegado al hueso HEAD → caja vacía (`ValueError`) | `model_render`: si no hay cabeza, lo más alto del modelo (icono y retratos) |
| Cualquier importación con un `data_cmn.afs` ajeno | plantilla basura (§1) | `afs_pair.hd` validado |
| `bcgo2b62` | su cara L42 pide la textura 16 y el `.emb` trae 14 (`IndexError`) | `drop_broken`: fuera las submallas sin textura, con aviso |

Resultado: **las 875 formas de Heroes** (`model/*/bc*bNN`) convierten y dan icono y retratos (antes fallaban 10 con
Goku de donante, y todas con Trunks niño u Omega). Hoja de 10 al azar: `E:\DBZ3HD_trabajo\modkit\sdbh\azar10.png`.

### 3. Texturas de «cristal» (F5)
- **Qué es F5**: en el material HD (+0x38) el tipo es `0x1B5` (normal) o `0x1F5` (bit 0x40). Lo llevan las partes
  brillantes: cúpula, hombreras y gemas de Freezer (todas sus formas), cúpula de Cooler, Cell y Cell Jr., visera del
  casco del Gran Saiyaman. Su textura no es un dibujo con uv: es una **imagen de reflejo** (matcap: el remolino morado
  de Freezer). El vertex shader del juego para esas partes (`2DD4268D7EE12280` del volcado) no usa la uv de la malla:
  `uv = (0.5 + 0.5·nx, 0.5 − 0.5·ny)` con la normal en cámara (sphere map); el resto (rampa, contorno) igual.
- **Heroes**: esas partes son materiales `ToonEnvironment_W` (las únicas submallas con 3 texturas: base, rampa,
  reflejo; 227 en todo SDBH). Antes salían B5 con la base (casi siempre blanca): **cúpula de Freezer blanca**. Ahora
  `sdbh_model` las convierte en F5 con el reflejo como textura (tenido con el color medio de la base bajo sus uv) y
  la rampa de ropa, como los nativos.
- **Ports PS2**: `amo2awo` ya copiaba el tipo de la parte (comprobado: Freezer 184, Cooler 166 y Gran Saiyaman 293 de
  PS2 → F5). Budokai 2 no usa reflejos (su Freezer es todo B5 con dibujos por uv): no hay que ponerlos. PSP
  (`psp_amo`, de otro frente) fuerza B5; no he visto reflejos en PSP.
- **`model_render`**: los materiales F5 se pintan con la normal en cámara (como el juego). Comparado con los retratos
  oficiales: Freezer forma 1 y Cooler ahora sacan el remolino/brillo de la cúpula como el oficial (antes, cúpula lisa o
  raya oscura): `E:\DBZ3HD_trabajo\modkit\f5_frz_cmp.png`, `f5_cmp2.png`; Heroes antes/después:
  `E:\DBZ3HD_trabajo\modkit\sdbh\frz_antes_despues.png`.
- **En el juego** (copia `modkit_test`, Freezer de Heroes importado con `importar.py importar sdbh FRZ`, ID 22, forma
  definitiva, pose de victoria de cerca): la cúpula y las hombreras del importado salen con el mismo reflejo morado que
  el Freezer original (antes, con la herramienta vieja, blancas). Capturas:
  `D:\DBZ3HD\_pruebas\20261015-093329_modkit_test_f5_pose_orig` (original) y `...093517_modkit_test_f5_pose_sdbh`
  (Heroes); comparación de cabezas: `E:\DBZ3HD_trabajo\modkit\pose_cabezas.png`. Escenarios:
  `E:\DBZ3HD_trabajo\modkit\escenarios\f5_pose_*.toml` (`DBZ3_TEST_BATTLE=27,22,0,-1,0,1,3,3` + `DBZ3_TEST_KO=1`).

### 4. K1 y K2 (`vision_futura.md` §3)
Ya estaban hechos: fila «Retos predefinidos» del Kit (desplegable, «Torre de la semana», «Quitar», llama a
`retos_predefinidos.py instalar|semana|quitar --juego`) y `retos_predefinidos` en `KIT_ENTRIES`. El selftest del Kit
lo comprueba (15 retos en el catálogo). Puntuación: K1 = 1, K2 = 1.

### Pruebas (ronda 2)
- `python swap_b3.py --selftest` → OK (IW/cortado/PS2 rechazados, `DBZ3\us`, carpeta del launcher con comillas
  simples y dobles, región eu, redirección, 6 idiomas).
- `python awo_tools/sdbh_model.py --selftest` → OK (F5 y tinte nuevos). `python log_en.py` → OK (307 frases).
- `python modkit_gui.py --selftest` → **RESULT: OK (28.7 s)**.
- `python tools/make_modpacks.py --dry-run` → 134 ficheros; entran `swap_b3`, `roster_build`, `model_render`,
  `texture_b3`, `modkit_gui`, `retos_predefinidos` y, por `kit_modules`, `sdbh_model` y `afs_pair`.
- Kit «de jugador» simulado (`E:\DBZ3HD_trabajo\modkit\fakekit`, con un `us\data_cmn.afs` de 1136 entradas como el
  de IW): con la 1.4.5.2 → `IndexError`; ahora `vista --solo icono` hace el icono (código 0) con el aviso; la vista
  completa, `swap_b3` y `texture_b3` acaban con el mensaje claro (código 1); «Comprobar instalación» lo marca en rojo.
  Con un `--us` malo y el juego bueno en otra carpeta: aviso y usa la buena. Con datos buenos: `texture_b3 extract`
  y `swap_b3` reales en temporal, OK.
- Heroes: las 875 formas (conversión + icono + retratos) y Xeno Janemba con los 38 donantes: todo OK.
- En el juego: `f5_pose_orig` y `f5_pose_sdbh` **PASS** (arriba). Sin cambios en el exe (no hace falta compilar).
- `python tools/pruebas.py todo` → **5/5 PASS** (tag_swap, campana, du_continuar, batalla, menu).

### Puntuación de lo mío (ronda 2)
| Tarea | Nota | Prueba |
|---|---|---|
| 1. Vista previa / `data_cmn.afs` | 1 | selftests + Kit simulado de jugador |
| 2. Heroes sin imagen 3D | 1 | 875 formas + 38 donantes |
| 3. Cristal F5 | 1 | retratos oficiales + capturas en el juego |
| K1 / K2 | 1 / 1 | ya hechos; selftest del Kit |

### 5. Cierre (encargo del coordinador): toda la lista de Heroes y Shin Budokai
**Lista completa de Heroes** (`importar.py`, ahora de este frente):
- `sdbh_list()` recorre todas las carpetas `model/bc*/` con formas: **391 personajes** (antes 24 + los 5 que añadió
  otro frente: Goku Black, Roshi, Beerus, Zamasu, Zamasu fusionado). Los de `SB_CHARS` siguen igual (golpes de SB).
- Nombres (`SDBH_CHARS`): identificados mirando el render de cada código (hojas `E:\DBZ3HD_trabajo\modkit\sdbh\
  hoja_00..09.png`): 156 con nombre (Xeno Janemba, Turles, Hit, Jiren, Towa, Super 17, Hirudegarn, Simios gigantes,
  Dragones oscuros…); las variantes dudosas llevan el código («Janemba (JBH)»); los 235 que no reconozco salen como
  «Heroes ACA» (sobre todo avatares y personajes propios de Heroes). Donante por familia (Janemba → Krillin como el
  Janemba de SB, androides → 16/17/18, Freezer → 27, Vegeta → 7…), si no Goku. `SDBH_IN_B3`: los que ya están en B3
  llevan « SDBH» en el nombre sugerido (Raditz SDBH).
- `entry_kind` solo mira las ISO de PSP para los códigos de `SB_CHARS`: la lista sale en 0,5 s.
- **Kit**: la pestaña «Importar personaje» → Heroes enseña los 391; buscando «Janemba» salen 6 (captura de la ventana
  real, sin guardar ajustes: `E:\DBZ3HD_trabajo\modkit\kit_heroes_janemba.png`). El selftest del Kit comprueba ahora
  la lista (≥ 300 y Xeno Janemba).
- **Xeno Janemba de la lista** (`importar.py importar sdbh JNO --mod xeno_janemba --nombre "Xeno Janemba"`, la misma
  orden que el botón del Kit): 2 formas, vista previa con icono, retratos y rótulo
  (`E:\DBZ3HD_trabajo\modkit\xeno_janemba_vista.png`); montado en la copia como ID 23.
- **En el juego** (copia `modkit_test`, ya con el reflejo a 128 px): entra en combate en sus dos formas, da y recibe
  golpes y gana por K.O. con su pose de victoria («WINNER!») y el menú final: **PASS** en
  `D:\DBZ3HD\_pruebas\20261015-104259_modkit_test_xeno_janemba_ko_f0` y `...104451_..._ko_f1` (hoja:
  `E:\DBZ3HD_trabajo\modkit\xeno_ko.png`; escenarios `E:\DBZ3HD_trabajo\modkit\escenarios\xeno_janemba_ko_f*.toml`).
- Presupuesto: el reflejo nuevo (F5) se guarda a 128 px como máximo; con 256 px el traje 1 de Xeno Janemba subía el
  AZT a 642 KB (los nativos llegan a 610); ahora 593 KB y el bin 929 KB (< 948).

**Shin Budokai (PSP) y los reflejos**: no hay partes F5 que convertir.
- Barrido de las 1965 partes de los 86 modelos de SB1 y SB2: todas tienen el mismo formato de vértice del GE
  (`0x000182`: uv de 16 bits y posición, **sin normales**), así que la PSP no puede generar un reflejo por la normal;
  la cúpula de Freezer y Cooler se pinta con uv normales sobre un trozo del atlas que trae el remolino dibujado
  (`E:\DBZ3HD_trabajo\modkit\psp_frz_ver.png`). Nuestra conversión ya lo copia tal cual (se ve como en SB).
- Solo hay otra bandera de cabecera (`0x140101B` en vez de `0x101B`, 28 partes): capas de Broly, Dabura, Buu y Pikkon,
  partes de Gotenks/Gogeta, la espada de Trunks y el rastreador de Bardock. Parece doble cara o transparencia, no
  reflejo; no lo he tocado (propuesta abajo).

### Queda / propuestas para otros frentes
- **Launcher** (`src/launcher/mod_pipeline.cpp`, `UsDir`): pasa `--us` sin comprobar el fichero; ya no hace falta (las
  herramientas lo validan y redirigen), pero podría usar la misma regla (≥ 3990 entradas, entrada 0 en LZX) para
  avisar en su pantalla.
- **Heroes**: 235 personajes sin nombre conocido («Heroes XXX»); si alguien de la comunidad da la lista de códigos,
  basta con añadirlos a `SDBH_CHARS`.
- **psp_amo.py**: la bandera `0x140101B` de SB (capas, rastreador) podría pasarse a translúcido/doble cara del HD
  (el rastreador de Bardock debería verse translúcido como el nativo).

## Qué cambia para el jugador

1. **Importar personaje → tarjeta «5. Personajes de la visión (beta)»**. Misma forma que «4. Ports oficiales»:
   lista de `ports_vision.toml` (`importar.py oficiales --recetas ports_vision.toml`), botón Crear/Rehacer por
   personaje y «Crear todos» (`importar.py oficial --recetas ports_vision.toml <mod>|--todos --mods <mods>`).
   Avisa de que son propuestas, no oficiales.
2. **Historias → barra «Modos de reto»** encima del editor: torre / supervivencia / kumite + número de
   combates (2-10) + «Crear y montar». Paso 1/2 `modos_reto.py <modo> --pisos|--oleadas|--jefes N --salida
   campanas/retos/reto_<modo>.toml`; paso 2/2 `campana.py --mods <mods del Kit> montar <toml>`.
3. **Montar campaña (también el botón «Montar mod» del editor incrustado)**: ahora monta en la carpeta de mods
   que usa el Mod Kit (`campana.py --mods … [--us …] montar`); antes iba a la carpeta por defecto. Al terminar:
   deja el mod activo, dice dónde jugarlo («JUGAR > Dragon Universe > Freezer»), botón «Abrir carpeta del mod» y,
   si hay otras campañas activas con el mismo personaje de la rueda (se pisan la casilla), lo avisa con un botón
   «Dejar solo esta activa» (solo pone el marcador `.disabled`; se deshace en «Mis mods»).
4. **Progreso por paso**: la barra de abajo enseña la última línea de la herramienta mientras trabaja
   («Trabajando: Modo de reto (2/2): montar… · <línea>»), para todas las herramientas.
5. **Sin avisos falsos de «ya hay una tarea»**: las consultas de fondo (listas de juegos, ports) esperan su
   turno en vez de sacar una ventana emergente.
6. Ports oficiales/visión: «Crear» vs «Rehacer» se decide mirando TU carpeta de mods (antes miraba la de por
   defecto de `importar.py`) y los errores salen en sencillo en la tarjeta.

## Empaquetado (`tools/make_modpacks.py`)

- `KIT_ENTRIES` + `campana`, `editor_historias`, `modos_reto` (modos_reto se lanza como proceso: ningún import
  llegaba a él).
- `KIT_DATA` + `ports_vision.toml`.
- `KIT_CAMPAIGNS` admite globs: + `retos/*.toml`.
- Guardia nueva `no_third_party_art`: si alguna imagen (png/jpg/webp/avif/…) llega a `campanas/` del Kit, la
  build se para. Probado: forzando `imagenes/*` la guardia salta.
- `--dry-run` nuevo: prepara el Kit en una carpeta temporal y lista `mod center hd/` (sin Python portátil, sin
  zip, sin tocar `release_packs/`). Resultado: 117 ficheros, incluye modos_reto.py, ports_vision.toml y los 3
  `campanas/retos/*.toml`; ninguna imagen de campañas.

## Pruebas

- `python modkit_gui.py --selftest`: **RESULT: OK (37.6 s)**. Nuevas comprobaciones: comando de la tarjeta de
  visión, comando de modos de reto, `campana.py --mods <mods> … montar`, y `campaign_rivals` con una carpeta
  temporal (activa/desactivada/otro personaje/sin campaña). Una pasada anterior dio «Historias: el editor no se
  construye» mientras el frente de campañas reescribía `campana.py` (01:15-01:16); repetida después: OK.
- `python tools/make_modpacks.py --help` y `--dry-run`: OK.
- Prueba real en temporal (lo mismo que hace «Crear y montar»): `modos_reto.py torre --pisos 3 --salida <tmp>`
  y `campana.py --mods <tmp>/mods montar <tmp>/reto_torre.toml` → código 0, 74 entradas, `campana.toml` con
  `[rueda] personajes = 27`. (A las 01:15 falló con `KeyError: 'next'` en `campana.nueva_entrada` mientras ese
  frente editaba; a las 01:18 ya va.)
- Capturas (ventana real, sin guardar ajustes): barra «Modos de reto» en Historias y la tarjeta 5 con los 6
  personajes de `ports_vision.toml` leídos por `importar.py oficiales --recetas`.

## Propuestas para otros frentes (no tocado por mí)

- **importar.py (ports)**: `oficiales` debería aceptar `--mods` (hoy calcula «instalado» con
  `rb.default_mods()`); el Mod Kit ya lo calcula solo. La cabecera de `ports_vision.toml` dice que `oficial`
  no lee ese fichero: ya no es verdad desde `--recetas`.
- **campana.py (campañas)**: (a) vigilar el `KeyError: 'next'` de arriba (`nueva_entrada` sin
  `reservar_cmn`), aunque ya no se reproduce; (b) el `campana.toml` del mod escribe `personajes = 27, 28` sin corchetes: con más de un
  personaje no es TOML válido (con uno sale un número suelto; el Mod Kit acepta ambos). Si el lector del
  runtime lo admite, escribir `[27, 28]`.
- **editor_historias.py (campañas)**: la «Ventana aparte» monta en la carpeta de mods por defecto. Propuesta:
  `--mods/--us` en su línea de comandos que pase a `campana.py`; el Mod Kit ya los mandaría desde
  `open_story_editor` (un cambio de una línea cuando exista).
- **modos_reto.py (modos)**: `--selftest` exige que existan las imágenes de Freezer (`comprobar`): en el Kit no
  van (arte de terceros), así que el selftest fallaría allí; mejor aviso que error. Los tres retos y la historia
  de Freezer comparten casilla y `textos_desde = 5620`: el Mod Kit avisa y ofrece dejar solo uno activo.
