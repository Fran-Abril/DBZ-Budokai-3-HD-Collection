# Frente: Visión futura (2026-10-15)

Objetivo (punto 5): cumplir y superar `studies/11_vision_futura` y revisar la propuesta de `studies/assets`.
Lectura en paralelo con subagentes Haiku (fichas 01-06, 04/17/20-22, catálogo de assets, assets propios);
el resto, las comprobaciones en código y las ediciones, aquí. Solo ficheros nuevos: no he tocado `src/` ni
ficheros de otros frentes.

**Resultado: ~23 % de la visión futura cumplida** (29 fichas, cada una puntuada 0-1 por su entregable mínimo;
tabla en §1). Hay mucho hecho que **no estaba en las fichas** y las supera (§2). Lo pendiente queda repartido en
tareas pequeñas por frente (§3). De los 222 assets propuestos solo hacen falta de verdad unos 10, y casi todos se
pueden generar por código como CC0 (§4).

## 1. Matriz de estado (comparado con el código y las pruebas de hoy)

Estado: **HECHO** (entregable mínimo en el juego), **A MEDIAS** (parte en el juego o solo base), **OFFLINE**
(solo prototipo/documento), **SIN EMPEZAR**. Las pruebas `D:\DBZ3HD\_pruebas\<fecha>_...` son capturas + log.

| # | Ficha (entregable mínimo) | Estado | Prueba | Nota |
|---|---|---|---|---|
| 01 | Ranking Elo local + pantalla de rango | A MEDIAS | `src/modos.cpp:322-353` (Elo K=32 por personaje desde `user_data/dbz3/combates.csv`, que escribe `tagteam.cpp:869-905` en cada combate); `tools/ranking.py --selftest` OK; log `ranking 1: id 0 elo 1060` en `...024909_vision_test_vf_torre_namek` | 0.4 · falta pantalla y entrada "ranking" del HUB (clave existe, `hub_menu.cpp:57`, sin `RegisterMode`) |
| 02 | Retardo adaptativo / snapshot / RT | SIN EMPEZAR | retardo solo negociado al conectar (`net/netplay.cpp:451,598`); precedente de constante de shader: Brillo HD (`ingame/quick_settings.cpp:192`) | 0.1 · base online sí existe (lockstep) |
| 03 | Torneo local de 4 con bots | A MEDIAS | `modos.cpp:466-509`: el Torneo Mundial **nativo** con los COM de una lista (también IDs 44-63); PASS `...020835_modos_test_torneo_44`, `...021059_modos_test_torneo_22` | 0.4 · solo por `DBZ3_TEST_TORNEO`; el jugador no puede elegir aún |
| 04 | Saga B1 propia / mapas B2 | A MEDIAS | personajes B1/B2/IW importables (`importar.py`, `b1port.py`); motor de campañas (`campaigns.cpp`, Freezer PASS en `campanas.md` §4) | 0.3 · sin saga B1 ni mapas B2 |
| 05 | Ruleset competitivo v0 (2 ataques) | SIN EMPEZAR | solo la herramienta `awo_tools/csk_edit.py --attack --damage` | 0.1 |
| 06 | Z-Counter: sonda de ventanas | A MEDIAS | sonda por tick `DBZ3_LAB=1` (acción X+3244, vida, posición → `logs/lab.csv`, `tagteam.cpp:867-890`) | 0.2 · falta medir 2 ataques |
| 07 | Traje de época con moveset/cápsulas | A MEDIAS | trajes extra por `traje.toml` (`roster_ext.cpp:304-364`, validado); hoy como personajes aparte: Vegeta saga Saiyan (48), Yamcha amarillo (49), Tenshinhan B2 (47) (`ports.md` §3) | 0.35 · moveset por traje = I+D |
| 08 | Goku Black completo (vía SDBH) | A MEDIAS | Goku SDBH (50) montado con 5 formas (`ports_vision.toml`); aura de los ports visible (prerrequisito de la ficha) | 0.25 · Goku Black/Beerus/Moro/Granolah sin fuente |
| 09 | 10 oleadas + 2 modificadores + contador + premio | A MEDIAS | Supervivencia nativa en el HUB (`modos.cpp`): vida arrastrada + recuperación; PASS `...014414_modos_test_superv2`; **Misión 100** (100 oleadas) hoy: PASS `...025519_vision_test_vf_mision100` (§5) | 0.6 · faltan modificadores (regen, tiempo), contador en pantalla y premio |
| 10 | Clima: cielo que se oscurece en 1 escenario | SIN EMPEZAR | nada en `src/` | 0 |
| 11 | Referencia B4 (lector de frame data) | OFFLINE | `studies/tools/lab_trace.py` (fixture sintética) | 0.15 |
| 12 | Lab: 2 ataques medidos, ambos lados | A MEDIAS | captura por tick `DBZ3_LAB=1`; `lab_trace.py`; entrada "laboratorio" del HUB sin registrar | 0.3 · no hay modo de jugador |
| 13 | Roshi jugable | SIN EMPEZAR | sin fuente de modelo (`BASE_ROSTER.md`) | 0 |
| 14 | Cápsula agrupada (vista previa) | OFFLINE | `combat_policy.py`; la cadena de prerrequisitos ya se lee (`form_selection.h:20,39-43`) para el selector de forma | 0.15 |
| 15 | Técnica directa sin Hyper | OFFLINE | `combat_policy.py` | 0.1 |
| 16 | Cancel accesible | OFFLINE | `combat_policy.py`; teclas remapeables existen | 0.1 |
| 17 | Tablero de 6 nodos | OFFLINE | nada salvo combate desde marca de mapa en DU (`campaigns.cpp:278-282`) | 0.1 |
| 18 | Reversión por daño | OFFLINE | `combat_policy.py` | 0.1 |
| 19 | Jefes gigantes | SIN EMPEZAR | sin fuente ni escala | 0 |
| 20 | Fusión de B2 como personaje + misión | A MEDIAS | Fat Gotenks (45) y Ghost Gotenks (46) en el juego de prueba (`ports.md` §3, capturas `...013657_aura_test_tag_lento`) | 0.5 · falta la misión; Gokule/Yamhan sin fuente |
| 21 | Buu absorbido elegible | SIN EMPEZAR | solo los absorbidos que ya trae B3/IW | 0.05 |
| 22 | Intro de 10 s con skip | A MEDIAS | narración con varias imágenes y voces, diálogo en escenario (`campanas.md` §3) | 0.15 · sin línea de tiempo ni skip |
| 23 | 3 días + 1 jefe | OFFLINE | `solo_progression.py` modo `simulation` | 0.15 |
| 24 | Vida persistente + Zeni + tienda | A MEDIAS | vida que se arrastra y recuperación (Supervivencia/Kumite nativos) | 0.25 · sin Zeni ni tienda |
| 25 | Modo héroe | SIN EMPEZAR | — | 0 |
| 26 | 2 torres de 3 combates + rotación semanal | A MEDIAS | Torre nativa (PASS `...014055_modos_test_torre3`); **hoy: 6 torres + rotación semanal local** (§5), Torre de Namek 4/4 `...024909_vision_test_vf_torre_namek` | 0.5 · falta el temporizador y los modificadores |
| 27 | Abyss: 3 pisos y 3 cartas | OFFLINE | `solo_progression.py` modo `abyss` | 0.1 |
| 28 | 3 jefes con vida finita, armor y hueco | A MEDIAS | Kumite nativo (jefes 5000-7000 de vida, vida arrastrada); **hoy: 3 kumites** (§5), Kumite exprés PASS 3/3 `...025313_vision_test_vf_kumite_expres` | 0.45 · faltan armor, objetivo por tiempo y marcador |
| 29 | Plan + 3 prototipos offline | HECHO (documental) | `python -B studies/tools/test_wishlist.py`: **33 pruebas OK** hoy | 0.8 · los lotes W-A.. siguen abiertos |

Suma 6,65 / 29 = **23 %**. Antes de este frente era el 22 % (la ficha 26 sube de 0,3 a 0,5).

`BASE_MODOS_RETO.md` está superado: decía que arrastrar la vida "necesita runtime" y que había un máximo de 10
combates y un solo personaje en la rueda; los modos nativos (`modos.cpp`) arrastran la vida, no usan casilla de DU
y aceptan 100 combates (probado). `BASE_ROSTER.md` sigue vigente salvo que las 6 recetas ya están montadas.
`INTEGRACION_SEGURA.md`: los casos WL-12..WL-28 siguen pendientes en el juego salvo lo marcado arriba.

## 2. Lo que ya supera la propuesta (no estaba en las fichas)

1. **HUB nativo** en el menú principal (8 entradas, etiquetas con la letra del juego, BETA, combate rápido).
2. **Retos sin campaña**: Torre/Supervivencia/Kumite con el combate del juego, desde el HUB, sin límite de 10
   combates, sin ocupar casilla de DU y editables por TOML sin recompilar (`modos_reto.py nativo`).
3. **Torneo Mundial del propio juego** con participantes elegidos, incluidos personajes nuevos (la ficha 03
   proponía un cuadro hecho por nosotros; el nativo se ve 100 % del juego).
4. **Ranking automático de todos los combates** (no solo online) y su espejo offline con test.
5. **Tag Team nativo**: entrada directa a la rueda, varios miembros elegidos con el menú del juego, fichas del HUD
   propias (CC0) y Tag Team online (beta).
6. **Online** con código de sala, UPnP/STUN, barrera de inicio y sincronización de mods.
7. **Selector de forma inicial** (LT/RT) en la selección.
8. **Campañas** que conviven (una casilla por campaña), narración con varias imágenes, editor con vista previa real.
9. **Ports de la visión**: 6 recetas montadas (Fat/Ghost Gotenks, Vegeta saga Saiyan, Tenshinhan B2, Yamcha
   amarillo, Goku SDBH 5 formas) con cara del HUD encuadrada sola y colas del cinturón.
10. **Packs visuales opcionales** con créditos y guardas para que el arte de terceros nunca vaya a la release.
11. **Pruebas automáticas en el juego** (`tools/pruebas.py`, instantáneas, 3 juegos a la vez).
12. **Hoy**: catálogo de 12 retos, **torre de la semana** sin servidor y **Misión 100** (la ficha 09 pedía 10).

## 3. Tareas pendientes por frente (pequeñas y concretas)

**HUB v2** (`src/hub_menu.cpp`, `src/ingame/hub_panel.cpp`)
- H1. Quitar "Muy pronto" de Torre, Supervivencia y Kumite (ya están registrados). Textos propuestos (`kHelp`,
  `hub_panel.cpp:52-57`): Torre `"Sube piso a piso contra rivales cada vez más fuertes con tu último luchador del
  Duelo." / "Climb floor by floor against ever stronger rivals with your last Duel fighter."`; Supervivencia
  `"Oleadas seguidas: la vida que te queda pasa a la siguiente y recuperas un poco entre medias." / "Back-to-back
  waves: your remaining health carries over and you recover a little in between."`; Kumite `"Jefes con mucha vida,
  uno tras otro. Tu vida también se arrastra." / "Bosses with lots of health, one after another. Your health
  carries over too."`. Actualizar `hub.md` (aún dice que están en gris).
- H2. En el bocadillo de Torre/Supervivencia/Kumite, el nombre del reto puesto (claves `nombre`/`name` del TOML;
  necesita M5).

**Modos** (`src/modos.cpp`, ranking) — falta su `docs/14_frentes/modos.md` (el código lo cita)
- M1. `RegisterMode("ranking")` con una pantalla top 10 (`Ranking()` ya existe) → ficha 01.
- M2. Contador visible "Piso/Oleada/Jefe N de M" durante el combate (`Estado` ya tiene `ganados/total`) → 09, 28.
- M3. Dos modificadores por TOML: `regen` (vida por segundo del rival) y `tiempo` (segundos) → mínimo de la 09.
- M4. Torneo para el jugador: participantes desde `user_data/dbz3/modos/torneo.toml` (hoy solo `DBZ3_TEST_TORNEO`) → 03.
- M5. Leer `nombre`/`name` del TOML del reto (los 12 retos nuevos los traen; hoy se ignoran sin problema).
- M6. Kumite: victoria por objetivo de tiempo (`objetivo_tiempo`), no por vida infinita → 28.
- M7. `RegisterMode("laboratorio")`: activar la captura `LabTick` sin variable de entorno y enseñar el último combate → 12.

**Tag Team**
- T1. Medir un reto con el equipo de Tag Team del jugador (reservas sucesivas de la 09, cooperativo por relevo de la 28).
- T2. Icono del relevo para IDs ≥ 44 (hoy una letra; propuesta del frente de ports).

**Ports**
- P1. **Fallo**: Vegeta GT (ID 44) cierra el juego en combate (lectura de 0x10), también sin modos:
  `D:\DBZ3HD\_pruebas\20261015-015446_modos_test_vegeta_gt_sin_modos` y `...015122_modos_test_torre_mods`.
- P2. Goku SDBH (50): ver sus 5 formas en combate (aún sin captura).
- P3. Vegeta saga Saiyan también como **traje de época** de Vegeta en `traje.toml` (07), con su nombre de época.
- P4. Renderizar `XYHS` y `XYMH` de B2 para ver si son Gokule/Yamhan (20).

**Escenarios**
- E1. Tabla índice → nombre de los 17 escenarios (para que los retos digan "Namek" y no "escenario 3").
- E2. Ficha 10 mínima: tinte suave de pantalla tras N segundos en un escenario, apagado por defecto (antes,
  revalidar `stage_ext.cpp` como pide la ficha 29).

**Pasarela B2/IW/B1 y campañas**
- G1. Campaña piloto de 3 combates con un personaje importado de B1 (04).
- G2. Misión original con Fat Gotenks (20), un TOML nuevo de campaña.
- G3. Grafo de 6 nodos en JSON + medir el combate desde marca de mapa (17); intro de 10 s con skip (22).

**QoL launcher / Mod Kit**
- K1. Mod Kit (`modkit_gui.py`): desplegable "Retos predefinidos" + botón "Torre de la semana" + "Quitar" que
  llamen a `retos_predefinidos.py instalar|semana|quitar --juego <juego>`.
- K2. `tools/make_modpacks.py`: añadir `retos_predefinidos` a `KIT_ENTRIES` (los `retos/*.toml` ya entran por el
  glob). La guía de jugador lo da por hecho.

**Bugs / limpieza**
- B1. P1 (Vegeta GT).
- B2. `assets_propios/png/add/` mezcla arte de personajes ("Android 17/Trunks flying") dentro de `png/`, que el
  README declara CC0: sacarlos a un pack opcional antes de publicar `png/` (regla de `REGLAS.md`).
- B3. `studies/assets`: `external_sources.json` da por autorizada la subida a la API de Google y dice
  `comfyui_used: false`, pero `VIABILIDAD_...` lo deja sin decidir y el README dice que se usó ComfyUI. Aclararlo
  con el usuario antes de enviar nada a Google.

## 4. Assets (`studies/assets`, 32 carpetas, 222 ficheros propuestos)

Lo generado hasta hoy (`E:\DBZ3_Assets_Google\00_comunes`): el atlas de iconos `common_icons.png` está casi en
blanco (inservible); `background_main.png` (dojo) es correcto pero de cómic occidental, no del juego. En cambio,
`assets_propios/generar.py` (CC0, por código) ya da etiquetas e insignias que no se distinguen del juego
(`assets_propios/revision/hoja_contacto.png`). Por eso la recomendación es: **todo lo de interfaz, por código;
el arte con personajes, solo en packs opcionales; los modelos y animaciones por IA no sirven** (el juego necesita
modelos con su esqueleto y sus bancos de golpes, que salen de los ports).

| Carpetas | Veredicto |
|---|---|
| 29 Tag Team | **Ya hecho** por código (ficha, relevo listo/espera) y en uso (`res/ingame_ui/tt_*`). Sonidos: los del juego. |
| 09 supervivencia, 26 torres, 28 kumite | **Hacen falta 2-3 piezas** por código: marcador "Piso/Oleada/Jefe N/M" e insignia de jefe. El catálogo de torres de la 26 ya está hecho como datos (§5). Fondos y marcos: los del juego. |
| 01 matchmaking | **Medallas de rango** por código cuando exista la pantalla de Ranking (M1). Marco de lobby: el del juego. |
| 12 lab | Iconos grabar/reproducir/paso por código, cuando exista el Laboratorio (M7). |
| 00 comunes | Sobra: el juego ya da panel, marco, agua, bocadillo y sonidos (`native_ui.cpp`); el dojo, solo como pack opcional. |
| 03 torneo | Sobra: se usa el cuadro del Torneo Mundial del juego. |
| 30 launcher/Kit | Sobra: el launcher usa iconos de la fuente de Windows (`ui_kit.cpp:105-108`). |
| 31 campañas | Sobra para la release: diálogo y mapa son del juego; imágenes de narración, por pack opcional (`pack_campana_freezer.toml`). |
| 07, 08, 13, 19, 20, 21, 25 (personajes) | **Solo packs opcionales** para retratos/fondos; los modelos salen de ports (07/20 ya tienen 5 de ellos). Sin referencia revisada salvo Roshi. |
| 02, 04, 05, 06, 10, 11, 14-18, 22-24, 27 | Sobran hoy: sus mecánicas no existen. Cuando existan, iconos por código. |

**Lista priorizada** (generar con `assets_propios/generar.py`, mismo estilo medido):
1. `rapido`: etiqueta en 5 idiomas + insignia cronómetro (hoy hechas a mano, no reproducibles; propuesta 4 de `hub.md`).
2. Marcador de reto: placa "Piso/Oleada/Jefe" + números (para M2).
3. Insignia de jefe para el Kumite (corona/calavera estilizada, sin personajes).
4. Medallas de rango (5 niveles) para el Ranking (M1).
5. `titulo_hub.png` reproducible (receta en `hub.md`) e icono del exe en `src/version.rc` (`assets_propios.md` punto 7).
6. Cuando existan: iconos del Laboratorio y de modificadores (regeneración, tiempo, ki).
7. Opcional, solo pack: fondo de HUB (`hub/fondo`), banner, renders, cut-ins de relevo (ya hay recetas en `mod center hd/packs_visuales/`).

## 5. Hecho en este frente (ficheros nuevos; nada existente modificado)

- `mod center hd/campanas/retos/` — **12 retos nuevos** en el formato que lee el juego (`user_data/dbz3/modos/<tipo>.toml`),
  con nombre y descripción ES/EN (claves que el juego ignora): torres `saiyan`, `namek`, `androides`, `buu`,
  `leyendas`, `guerreros_z`; supervivencias `mision100` (100 oleadas, 7 barras), `saibaman`, `una_barra`; kumites
  `villanos`, `saiyan`, `tres_jefes`. Solo IDs de serie que existen sin mods (sin 22-26 ni 31), escenarios 0-10,
  vida ≤ 7000. Entran solos en el Kit por el glob `retos/*.toml` de `make_modpacks.py`.
- `mod center hd/retos_predefinidos.py` — `lista`, `instalar`, `semana` (torre de la semana ISO, sin azar),
  `quitar`; lo sustituido va a `user_data/dbz3/modos/_respaldo/`. Valida con las mismas reglas que el juego y que
  `modos_reto.comprobar_nativo`.
- `docs/jugador/MODOS_Y_RETOS.md` — guía de jugador ES/EN del HUB y los retos.

### Pruebas
- `python retos_predefinidos.py --selftest` → **OK** (15 retos en el catálogo, 12 nuevos; instalar, repetir,
  respaldo, quitar, rotación semanal: 7 días = misma torre, 7 semanas = 7 torres distintas).
- `python -B studies/tools/test_wishlist.py` → **33 OK**; `python tools/ranking.py --selftest` → OK.
- En el juego (copia `D:\DBZ3HD\vision_test`, retos puestos con `retos_predefinidos.py instalar`):
  - Torre de Namek con Goku: **4/4** (Recoome 2000, Ginyu 2500, Vegeta 3000, Freezer 3600), `torre terminado:
    victoria (4 de 4)`: `D:\DBZ3HD\_pruebas\20261015-024909_vision_test_vf_torre_namek` (el FAIL final de esa
    corrida es de mi guion: pulsaba teclas del menú final que el modo ya no muestra tras ganar).
  - Kumite exprés con Vegeta: **PASS 3/3** (Freezer 5000, Cell 6000, Kid Buu 7000 con sus 7 barras, captura
    `03_jefe_36`): `D:\DBZ3HD\_pruebas\20261015-025313_vision_test_vf_kumite_expres`.
  - Misión 100: **PASS** (carga los 100 combates, encadena 4 oleadas con 7 barras y la vida arrastrada: 6874 → 7000
    con la recuperación): `D:\DBZ3HD\_pruebas\20261015-025519_vision_test_vf_mision100`. Las 100 enteras no se han
    jugado (es un reto extremo, sin equilibrar).
  Escenarios de prueba (scratchpad de la sesión): `vf_torre_namek.toml`, `vf_kumite_expres.toml`, `vf_mision100.toml`.
- Sin compilar: no he tocado código del juego.
