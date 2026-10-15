# Frente: campañas propias de Dragon Universe (2026-10-15)

Dueño: `mod center hd/campana.py`, `editor_historias.py`, `campanas/*` (salvo `retos/`), `src/campaigns.cpp`.
Ronda 2 (también): `src/pasarela*.cpp`, `docs/16_pasarela_modos/`.

## 0. Ronda 2 (2026-10-15): fichas 04, 17, 20, 21, 02 C y Leyenda de Mr. Satán

### Qué ve el jugador (antes → ahora)

- **Entrada «Pasarela» del HUB** (página 2; antes en gris «Muy pronto»): menú con el marco y la letra del juego y cinco
  modos. Lo que necesita una copia o un mod que falta sale en gris y el bocadillo dice qué hace falta. Doc:
  `docs/16_pasarela_modos/04_misiones_y_tablero.md`.
  1. **Saga de Vegeta en Namek (Budokai 1)** (ficha 04): los 4 combates de la historia de Vegeta de B1 leídos de la ISO del
     jugador (Zarbon transformado, Recoome, Freezer en 1.ª forma y en forma final), en Namek, con el ki inicial de B1 y
     Zarbon **importado de B1** con el Mod Kit (su cara del marcador, su modelo y su forma transformada).
  2. **Tablero de la saga Saiyan (Budokai 2)** (ficha 17): 6 casillas propias unidas por caminos; LB/RB (o la cruceta) eligen
     camino y A mueve; Saibaman y Nappa en sus casillas, semillas que llenan la vida, Vegeta de jefe (solo tras vencer a los
     dos); la vida se arrastra; **perder = fracaso**: vuelta a Kame House y casillas de nuevo; turnos y avance guardados.
  3. **¡Fusión fallida! (Gotenks gordo)** (ficha 20): misión nueva con charlas (Piccolo, Súper Bu) y dos combates de Gotenks
     gordo (port de B2): contra Majin Bu en la ciudad y contra Súper Bu en la sala del espíritu y del tiempo.
  4. **Las absorciones de Súper Bu** (ficha 21): Bu vence a Piccolo, Goten y Gohan; tras cada victoria lo absorbe y antes del
     combate siguiente se elige con qué absorción luchar. Cada absorción es un **traje nuevo de Súper Bu** (modelo del juego:
     `mod center hd/campanas/trajes_bu/`), así que también se puede elegir en la selección normal; el traje 1 sigue siendo
     el Bu de siempre.
  5. **Retos de Infinite World** (ficha 02 C, tarea del coordinador): «Training No.1» (30 ataques básicos en 180 s, con
     marcador «Golpes N/30» bajo el reloj; al llegar a 30 el combate acaba a favor) y «Endure Gohan's attacks!» (60 s sin
     atacar con 1000 de vida o más, el COM en Novato), con el título y el texto de la misión **de la copia de IW del jugador**.
- **Leyenda de Mr. Satán** (antes: sin dificultad, sin puntos, sin las reglas que B1 tiene en código):
  - Título con el letrero de B1 y sus 3 dificultades (imágenes de la ISO: en español «Novel / Avanzado / Experto»); la
    dificultad es el **nivel de la IA del COM** (el mismo byte que el juego usa para el Torneo).
  - **Puntos como en B1** (vida que sobra + segundos × 20, la mitad para la leyenda) y **pantalla de resultado del original**
    tras cada retador («Retador 1.º», «¡Toma! 940 Pts.», «Total 1890 Pts.», las caras de los 11: vencidos en color, el resto
    en silueta); total y récord al final.
  - Reglas en código: Androide 16 con armadura (la mitad del daño no cuenta), Mr. Satán herido contra Trunks (el daño que
    no viene del ki se queda en un 25 %), ki lento contra Piccolo (gana la mitad), y el ki inicial del rival de la tabla.
- **En todos los combates de la pasarela los luchadores llevan sus cápsulas** (la lista Normal de cada personaje, como el
  COM del Torneo): antes el conector los dejaba sin ninguna y nadie lanzaba técnicas ni se transformaba. Ahora el COM usa
  técnicas (registro «gasta 2000 de ki de golpe») y se transforma (Freezer pasa a su «Tercera forma» en la saga).

### Puntuación de las fichas (0-1, con la prueba)

| Ficha | Nota | Prueba (capturas en `D:\DBZ3HD\_pruebas\`) |
|---|---|---|
| 04 Fusión trilogía (mínimo: saga de B1 de 3+ combates con un personaje importado de B1) | **1** | `saga_b1` PASS `20261015-111749_campanas_test_saga_b1`: charla de Zarbon B1 con su cara (02), tarjeta (03), Vegeta contra Zarbon transformado en Namek (04, ki del rival 4000), Recoome (05), Freezer 1.ª forma que se transforma (06), Freezer final (07), final (08). Ojo: el modelo transformado de Zarbon (mod `port_b1_zarbon` de `presets_test`) se ve casi blanco: lo revisa el frente ports. Mapas de B2: fuera del mínimo pedido. |
| 17 Tablero B2 (grafo de 6 nodos jugable) | **1** | `tablero` PASS `20261015-131009_campanas_test_tablero` (tablero, Saibaman, Nappa con la vida que quedó, semilla, Vegeta, «¡Completado!») y `tablero_fracaso` PASS `20261015-141824_...` (Saibaman con su «Ácido», «¡Fracaso!», tablero reiniciado) |
| 20 Fusiones hipotéticas (misión con Gotenks gordo) | **1** | `mision_gotenks` PASS `20261015-112617_campanas_test_mision_gotenks` (charlas, Gotenks gordo contra Majin Bu en la ciudad y contra Súper Bu en la sala del tiempo, final) |
| 21 Absorciones de Súper Bu (Bu absorbido elegible) | **1** | `mision_buu` PASS `20261015-093838_...` y `20261015-121049_...` (Bu normal, Bu-Piccolo, Bu-Gotenks y Bu-Gohan en combate); técnica: `bu_tecnica` PASS `20261015-131811_campanas_test_bu_tecnica` (y `20261015-120702_...`): capturas 02-03 = Bu (Piccolo absorbido) del COM lanza «Ataque Bola Malévola» (2000 de ki), y en el registro absorbe («pasa a la forma 3»); 05 = combate siguiente con el Bu de siempre (traje 1) |
| 02 C Minijuego de IW como escena de B3 | **1** | `iw_retos` PASS `20261015-144358_campanas_test_iw_retos`: informe con el título y el texto de IW (01, 04), marcador «Golpes N/30» (02b), «reto: 30 golpes, cumplido» (03) y el de aguantar con su regla (60 s sin atacar: «ataco false»; Goku se cubrió y acabó con 525 < 1000 → «reto fallado» y pantalla de reintento, 08) |
| Leyenda de Mr. Satán (reglas, dificultades, puntos) | **1** | `leyenda_dificultad_puntos` PASS `20261015-113245_...` (letrero con la dificultad, IA del COM 1 → 2, «puntos del combate 940», resultado de B1 con «Retador 1.º/2.º» y caras); reglas en código: `leyenda_reglas_codigo` (A16: 834 de daño anulado), `leyenda_regla_trunks` (714), `leyenda_regla_piccolo` (5447 de ki perdido), todas PASS |

### Pruebas (copia `D:\DBZ3HD\campanas_test`, juego en español)

Escenarios en `docs/16_pasarela_modos/pruebas/` (`python tools/pruebas.py correr campanas_test <escenario>.toml`). La
copia es `pasarela_test` + `mods` con Zarbon de B1 (`port_b1_zarbon`), Gotenks gordo (`vis_b2_fat_gotenks`) y los 3 trajes
de Bu, roster con `roster_build.py construir --mods D:/DBZ3HD/campanas_test/mods` (allí Zarbon = 22 y Gotenks gordo = 23)
y `ps2_games` = unión a `D:\DBZ3HD\ps2_games` (B1 europea; IW USA extraído).

| Escenario | Resultado | Carpeta (`D:\DBZ3HD\_pruebas\`) | Qué se ve |
|---|---|---|---|
| `saga_b1` | PASS | `20261015-111749_campanas_test_saga_b1` | 4 combates de B1 en Namek, Zarbon B1 forma 1, Freezer 0 y 3, ki 4000-6000 |
| `mision_gotenks` | PASS | `20261015-112617_campanas_test_mision_gotenks` | charlas y los 2 combates de Gotenks gordo |
| `tablero` | PASS | `20261015-131009_campanas_test_tablero` | 5 turnos: Saibaman, Nappa con la vida de antes (`vida -2581`), semilla, Vegeta, final |
| `tablero_fracaso` | PASS | `20261015-120134_campanas_test_tablero_fracaso` | derrota contra los Saibaman → «¡Fracaso!» y tablero a cero |
| `mision_buu` | PASS | `20261015-093838_...` y `20261015-121049_campanas_test_mision_buu` | Bu normal → Piccolo → Gotenks → Gohan absorbidos, cada uno en su combate |
| `bu_tecnica` | PASS | `20261015-131811_campanas_test_bu_tecnica` | Bu (Piccolo absorbido) del COM con sus cápsulas: «Ataque Bola Malévola» (02-03), absorción (forma 3) y luego el Bu de siempre (05) |
| `pasarela_menu` / `pasarela_menu_sin_isos` | PASS | `20261015-122530_...`, `20261015-123131_...` | menú de 5 modos; sin copias, la saga y los retos de IW en gris con el motivo |
| `iw_retos` | PASS | `20261015-144358_campanas_test_iw_retos` | informes con las imágenes de IW, «Golpes N/30», «30 golpes, cumplido»; aguantar: 60 s sin atacar, vida 525 → fallado y reintento |
| `leyenda_dificultad_puntos` | PASS | `20261015-113245_campanas_test_leyenda_dificultad_puntos` | letrero de dificultad, COM 1 → 2, «puntos del combate 940», resultado de B1 |
| Leyenda de antes (`leyenda_inicio`, `_reglas`, `_formas`, `_final`, `_hub_gris`, `_sin_iso`) | PASS | `20261015-123254`, `-123553`, `-124020`, `-124600`, `-130004`, `-130145` | nada se rompió (vida que no se recupera, Cell Jr., formas, final con total y récord) |
| `leyenda_reglas_codigo` (A16) | PASS | `20261015-133819_campanas_test_leyenda_reglas_codigo` | combate entero sin ring out: «reglas: 834 de daño anulado (armadura 0.50)», ki del rival 4000 |
| `leyenda_regla_trunks` | PASS | `20261015-134932_campanas_test_leyenda_regla_trunks` | «714 de daño anulado (solo ki true)» |
| `leyenda_regla_piccolo` | PASS | `20261015-135219_campanas_test_leyenda_regla_piccolo` | «5447 de ki perdido (ki lento 0.50)» |
| `pasarela_hub` | PASS | `20261015-141337_campanas_test_pasarela_hub` | como un jugador: fila HUB → RB (página 2) → **Pasarela** → menú → tablero → B → menú → B → HUB |
| `tablero_fracaso` (otra vez) | PASS | `20261015-141824_campanas_test_tablero_fracaso` | «¡Fracaso!» y tablero reiniciado (captura 03) |
| `leyenda_derrota` | PASS | `20261015-142137_campanas_test_leyenda_derrota` | derrota, reintento y otro combate (la leyenda de antes, con la ronda 2) |
| `leyenda_hub` | PASS | `20261015-143629_campanas_test_leyenda_hub` | la leyenda desde el HUB como un jugador (con la entrada Pasarela al lado) |
| `iw_retos` (última, con el marcador solo durante la pelea) | PASS | `20261015-150008_campanas_test_iw_retos` | el menú del final ya sin «Golpes 30/30» encima (03) |

Batería común `python tools/pruebas.py todo` con este exe: 4/5 a la primera (`tag_swap` no llegó al menú en 120 s con el PC
cargado) y `tag_swap` PASS al repetirla (`20261015-150004_smoke_a_tag_swap`): 5/5. Compilado con `tools/compilar.py` sin
errores ni avisos en `src/pasarela*.cpp`. Los combates de la pasarela ya no dejan «UNHANDLED» en los registros.

### WL (casos de `studies/11_vision_futura/INTEGRACION_SEGURA.md`)

Copia `campanas_test`, juego en español. Secuencia: `bateria.sh hash mision_gotenks hash mision_buu sin_gotenks
wl20_sin_mod con_gotenks hash` (`E:/DBZ3HD_trabajo/campanas/bateria.sh`; `sin_gotenks` mueve el mod a
`mods_respaldo/` y monta el roster, `con_gotenks` lo devuelve; nada se borra).

| Caso | Qué se comprueba | Resultado | Prueba (`D:\DBZ3HD\_pruebas\`) |
|---|---|---|---|
| WL-20 paquete faltante | Sin el mod de Gotenks gordo (roster montado sin él: 4 personajes) la fila «¡Fusión fallida!» sale en gris con «Importa a Gotenks gordo de Budokai 2 con el Mod Kit...»; A no la empieza (sigue el menú) y B vuelve al HUB; nada cae. Al devolver el mod y montar, vuelve a estar (Gotenks gordo otra vez ID 23) y su avance (`gotenks` en `progreso.toml`) sigue ahí | **PASS** | `20261015-153708_campanas_test_wl20_sin_mod` (01 gris, 02 tras A, 03 vuelta); registro «modo gotenks listo false (Importa a Gotenks gordo...)» |
| WL-20 selección / equipo personal intactos | Huella de la partida (`data.bin`), de `forma_inicial.txt`, `capsulas_custom.txt` y `_roster/roster.toml` antes y después de jugar la misión entera: iguales (`data.bin` 9a334f9be7b65e08, sin tocar desde las 09:08; los dos .txt no se crean). La misión solo escribe `user_data/dbz3/pasarela/progreso.toml` | **PASS** | `20261015-152512_campanas_test_mision_gotenks` + salida de `hash` (15:25 y 15:28) |
| WL-20 Yamhan | La misión no usa Yamhan (Gokule/Yamhan son del frente ports, sin misión) | n/a | — |
| WL-21 modelos | Cada traje (0, 4, 3, 2) sale con su modelo del juego: Bu normal, Bu-Piccolo, Bu-Gotenks, Bu-Gohan | **PASS** | `20261015-152830_campanas_test_mision_buu` (03, 05, 07, 08); registro «traje del jugador 4/3/2» |
| WL-21 códigos y técnicas | El traje usa el moveset de Súper Bu: Bu (Piccolo absorbido) lanza «Ataque Bola Malévola» (2000 de ki) y su definitiva «Absorción» (pasa a la forma 3, la absorción de serie) | **PASS** | `20261015-131811_campanas_test_bu_tecnica` (02-03) |
| WL-21 cápsulas | Cada variante lleva la lista Normal de Súper Bu (Ill Flash 124), la misma que el Bu normal | **PASS** | `...152830...` registro «capsulas del lado 1: 4 124 0 0 0» (ojo abajo) |
| WL-21 HUD (cara / nombre) | La barra enseña la cara de Súper Bu (ojos y boca) en las 4 variantes; en el fichero de caras del juego (461, 8 formas) las absorciones solo cambian el cuello y el pelo de abajo, que la barra no enseña: coherente con el modelo. La barra de B3 no lleva nombre; en la selección todos los trajes dicen «Súper Bu» (como cualquier traje del juego) y en las pantallas de la misión sale el nombre de la variante | **PASS** (con nota) | `20261015-152515_campanas_wl21_hud` (01 caras en combate, 02 las 8 caras del juego) |

Notas:
- **HUD exacto:** el juego pinta la cara de la forma 0 para todos los trajes (fila de `0x82324468`: ID 35, todos los
  trajes, fichero 461). Para la cara exacta de cada absorción (la expresión cambia un poco) haría falta una fila por traje
  con su propio fichero de caras: propuesta 7 abajo.
- **Cápsulas de Súper Bu:** en memoria su lista Normal tiene ahora 4 huecos con solo una cápsula (124) y tres a 0; en el
  ejecutable es 1 (124). Lo cambia alguien al arrancar (¿cápsula única de la ficha 14?): que lo mire transformaciones.
- **EU:** he tocado `src/pasarela.cpp` y `src/pasarela_misiones.cpp` después de los borradores del frente eu (registro de la
  prueba de menú, registro de cápsulas, marcador solo en la pelea, retos de IW). `tools/eu_map.py`: 0 choques. Hay que
  regenerar sus borradores de esos dos ficheros.

### Cómo está hecho (para el coordinador)

- `src/pasarela_misiones.cpp` (nuevo, en `DBZ3_SOURCES` y en el `REMOVE_ITEM` solo-EU): entrada `pasarela` del HUB
  (`RegisterMode` + `RegisterGate`), motor de pasos, tablero, elección, informes de IW, marcador en combate.
- `src/pasarela.cpp`: piezas comunes en `pasarela.h` (`VigilarCombate` con las reglas, cápsulas, traje, nivel del COM,
  ki, retos y registro de técnicas/formas; avance por claves; personajes de mods por su origen; caras de mods; trajes extra)
  y la leyenda (dificultad, puntos, resultado, reglas en código).
- `src/pasarela_b1.cpp`: tabla de la historia de B1 (los 43 registros antes de la del modo), piezas 666/668 de la ISO y
  lector de los textos de misión de IW (ISO o carpeta).
- Sin ganchos `REX_HOOK_RAW`; todo por el conector `RequestBattleEx` y memoria del combate. Solo US.
- `mod center hd/campanas/trajes_bu/bu_absorbido_{piccolo,gotenks,gohan}/traje.toml`: trajes de Súper Bu con modelos del
  juego (`juego:116/107/104`), sin arte.

### Lo que queda

- Mapas de B2 portados (parte de la ficha 04 fuera del mínimo pedido) y más sagas de B1 (el motor ya lee las 43).
- Leyenda: los puntos por golpe de B1 (HIT, COMBO FIN...: necesitan ganchos de combate), voces de las charlas, textos
  propios en francés/alemán/italiano (hoy en inglés; los del original sí van en su idioma).
- Absorción «de verdad» elegida antes del combate (forma inicial de Bu absorbido): el juego se cae al cargar; hoy son trajes.
- Retos de IW: el nivel exacto de vida de «Endure» y las condiciones del resto de misiones (no están en `#DMD`).

### Propuestas para otros dueños (no tocados)

1. **`src/hub.cpp` (coordinador)**: `RequestBattleEx`/`ChainBattleEx` dejan **las cápsulas vacías** (`rec+52 = 0` tras
   `sub_82179408`): en la Torre, Supervivencia, Kumite y Combate rápido nadie lanza técnicas ni se transforma. Propuesta
   (lo que hace ya `VigilarCombate` en la pasarela, sin carrera):
   ```cpp
   // en Launch, tras escribir ID/traje/mando de cada lado:
   if (!REX_LOAD_U16(rec + 52) && q.id[k] < 105) {
     const uint32_t tab = 0x8234ABB8u + 96u * uint32_t(q.id[k]), n = std::min<uint32_t>(REX_LOAD_U16(tab + 80), 7);
     REX_STORE_U16(rec + 52, uint16_t(n));
     for (uint32_t c = 0; c < 7; ++c) REX_STORE_U16(rec + 54 + 2 * c, uint16_t(c < n ? REX_LOAD_U16(tab + 82 + 2 * c) : 0));
   }
   ```
   y un parámetro de **traje** (`suit1`, `suit2`) en `RequestBattleEx`: hoy la pasarela escribe `rec+18` desde su hilo
   en el fotograma entre el conector y la copia de la CFG (funciona, pero es una carrera).
2. **`src/ingame/hub_panel.cpp` (hub)**: ayuda de la entrada `pasarela`: hoy dice «Trae luchadores de Budokai 1, Budokai 2 e
   Infinite World»; propuesta: «Modos de Budokai 1, 2 e Infinite World: la saga de Namek, un tablero, misiones y retos.» /
   «Budokai 1, 2 and Infinite World modes: the Namek saga, a board, missions and challenges.»
3. **`src/roster_ext.cpp` `ApplyExtraCostume`**: un traje extra de un personaje con formas que usan el modelo de **otro**
   personaje (Súper Bu: formas 4-7 con los modelos de Piccolo, Gotenks, Gohan y Bu fantasma) escribe ese traje también en
   la tabla del otro (con el índice del traje de Bu: el 3.º traje de Piccolo). Los trajes de Bu lo esquivan dando el modelo
   de serie de Piccolo en ese hueco, pero depende del orden de las carpetas. Propuesta: saltar las formas de tipo 8 (de otro
   personaje) salvo que el traje las pida.
4. **Mod Kit (`make_modpacks.py` / Kit)**: ofrecer los 3 trajes de `mod center hd/campanas/trajes_bu/` como mods
   instalables (sin arte: solo números de modelos del juego) para que la misión de Bu esté disponible.
5. **Frente ports**: el modelo transformado de Zarbon B1 (`presets_test/mods/port_b1_zarbon`) sale casi blanco en combate
   (`...111749_campanas_test_saga_b1/04_combate_zarbon_transformado.png`).
6. **Índices**: `docs/README.md` y `docs/04_herramientas/TOOLS.md`: añadir `docs/16_pasarela_modos/04_misiones_y_tablero.md`.
7. **Cara del HUD por traje (`roster_build.py` + `roster_ext.cpp`)**: `traje.toml` con `hud_forma = k` → `build_trajes` copia el
   fichero de caras del personaje (`IDS[id]["hud_face"]`, Súper Bu 461) poniendo la textura de la forma `k` en la 0 y
   escribe `traje_hud = <fid>`; `ApplyExtraCostume` añade una fila a `0x82324468` antes de la del juego: `{ID, 1 << traje,
   fid, el mismo índice}` (como hace ya `SkillTable` con los personajes nuevos). Los trajes de Bu pondrían 1, 2 y 3.

## 1. Una casilla por campaña: varias campañas a la vez

**Límite del juego (medido por el frente DU, `docs/14_dragon_universe/01_casillas_y_partida.md`):** la partida tiene
38 bloques de 4624 B (casillas 0-37) que acaban justo donde empiezan la máscara de historias, el zeni y el byte de
casilla. No hay casilla 38+ sin mover toda la partida. La casilla de una campaña es la de su protagonista
(casilla = ID; 22-25 = ID-5; 26+ = ID-6). Libres (sin historia del juego): 1, 5, 6, 8, 9, 14-16, 18-33, 35-37.

**Por qué no convivían:** además de los bloques por casilla (5000 + 100·saga + casilla, 54xx combates, 58xx charlas),
cada campaña cambiaba **en su sitio** contenedores que comparten todas las historias: introducciones/cierres de saga
(códigos 100-106 → 1721-1727), finales (código 1 → 1728), «I found something!» (2100), y elegía entradas de
`data_cmn` fijas (imágenes 4510+, eventos 4600). Dos mods escribían el mismo contenedor y solo uno ganaba.

**Hecho:**
- `campana.py`: los contenedores compartidos (detectados con `du_codigos.json`: un código fuera de 5000-6199 lleva a
  ellos) ya no se tocan; su copia va a un bloque propio de 64 entradas de `data_cmn` (6000-8191, por crc32 del nombre
  del mod, estable en `campana.toml [contenedores]`) y se sirve por `[codigos_historia]`. Imágenes y eventos del mapa
  sin número fijo van al mismo bloque.
- `campaigns.cpp`: `[codigos_historia]` es **por mod**: solo se aplica si la casilla en juego (0x824A58A0) es la de
  un ID de la `[rueda]` de ese mod. El registro dice ahora `(..., casilla N)` en cada código.
- `campana.toml` generado: `[rueda] personajes = [..]` (TOML válido) y `casilla = N`.
- Validación al montar (y en el editor): la casilla del código tiene que ser la de un ID de `[rueda]`; casilla < 38;
  aviso si sustituye una historia del juego; **error si otra campaña de la carpeta de mods usa ya esa casilla**; aviso
  si otro mod trae la misma entrada de `data_cmn` con otros datos.
- Lo que el montaje ya no escribe en `data_cmn` (las versiones en su sitio de 1723-1725/1728, imágenes viejas) se
  mueve a `mods_respaldo/<mod>_cmn_<fecha>/`, nunca se borra.

## 2. Editor

- Vista previa del diálogo con **la imagen real del texto** (`campana.render`, misma fuente/ancho que va al juego): la
  primera frase del capítulo que usa ese retrato.
- `check()` avisa de la casilla ocupada por otra campaña.
- `--mods` / `--us` en la línea de órdenes: se pasan a `campana.py montar` (ventana aparte).
- Ya existían: validación completa, botón «Montar mod», vista previa de estilos.

## 3. Formas del rival y narración con varias imágenes

- `forma_rival` ya existía (BPC +0x2c/+0x58 en los 8 registros). Ejemplo: Trunks SSJ en `historia_freezer_prueba_tierra.toml`.
- **Nuevo** `imagenes` / `cierre_imagenes` en un capítulo que abre saga:
  `cierre_imagenes = [{ imagen = "x.png", paginas = 2 }, { imagen = 1549 }]`. Cada imagen es una escena con sus
  páginas (y voces); se encadenan con op 20 en entradas nuevas (38+) de la copia del contenedor, como 101 -> 102 del
  juego. `imagen` admite un número (imagen de `data_cmn` del juego: 1411, 1536, 1549). Con varias imágenes no vale
  mezclar `paginas` del juego.

## 4. Pruebas (evidencia en `D:\DBZ3HD\_pruebas\`)

- `python campana.py prueba` OK (incluye `lst_append` y la validación de casillas); `python editor_historias.py --selftest` OK.
- Compilado con `tools/compilar.py`: OK (21 s).
- Copia `du_test` con el juego **en español** (`dbz3_language = 5` en su `dbz3_user.toml`).
- `20261015-012312_du_test_du_freezer_nuevo` (PASS, `historia_freezer_prueba_tierra.toml`, RINGOUT): registro
  `codigo 102 -> contenedor 6128 (del mod) (casilla 22)` al empezar (la copia se usa ya en «Nueva historia»),
  combate 5522, cierre `103 -> 6132`, **`103 entrada 38`** (segunda imagen: captura 027 con el fondo de la Tierra
  1549), «¿Guardar partida?» (030) y guardada `104.22`. Captura 003: la intro de Freezer en español.
- `20261015-011823_..._du_freezer_nuevo` (la rueda cayó en Yamcha, casilla 13): `codigo 100 -> 1727` sin redirigir,
  es decir, las copias de Freezer no tocan la historia de otra casilla.
- **Regresión encontrada y arreglada (2026-10-15, 01:40):** `du_continuar` fallaba (sin líneas `partida:`, Freezer
  fuera de la rueda de «Continuar», volvía el «sale Goku al cargar»). Causa: `Load()` de `campaigns.cpp` tomaba
  como cabecera de sección cualquier línea con `[`, y el nuevo `personajes = [27]` dejaba la `[rueda]` vacía.
  Ahora solo es cabecera si la línea EMPIEZA por `[`. Compilado y repetido:
  - `20261015-014007_du_test_du_continuar` (`--instantanea freezer_cap2 --par iter=3`): **PASS**,
    «partida: 1 historias de mods recuperadas», «rueda ... casillas de mas 0000000000400000» (bit 22),
    «personaje del mapa al crear (casilla 22 ...)».
  - `20261015-014137_du_test_du_continuar` (RINGOUT, iter=15): **PASS**; captura 009 = charla generada por
    `campana.py` en español en el juego («¿Otra transformación? No me asustas.», con ¿ y tilde); 012 = Freezer 3.ª
    forma contra Piccolo (formas por capítulo).

## 5. Siguiente paso (P1 del frente DU)

Campaña de un protagonista cuya casilla ya tiene historia (o personaje nuevo ID ≥ 44): casilla anfitriona libre
(36/37) + gancho de `sub_82156E88` (casilla → ID) + celda virtual → casilla al confirmar (`0x8245F140`).
`docs/14_dragon_universe/04_ganchos_propuestos.md` P1. Sin hacer.

Arreglado también (aviso del frente DU): `MAP_EVENT_WORDS` con 3:4, 30:7, 31:6, 34:4, 41:9, 49:4 y error al montar
si un `[[escena]]` usa los eventos 25-28, 32, 33 o 37 (leen pila sin inicializar).

## Propuestas para otros dueños

- **Mod Kit (`modkit_gui.py`, `open_story_editor`)**: pasar `--mods <carpeta del Kit>` (y `--us`) al abrir el editor
  en ventana aparte; en el modo incrustado, el `runner` debe añadir `--mods` a la orden `campana.py montar`.
- **Mod Kit**: el aviso «otra campaña activa usa la misma casilla de Freezer» puede leer `[rueda] casilla` del
  `campana.toml` de cada mod (ya lo escribe `campana.py`) en lugar de suponer la 22.
- **`tools/pruebas.py`**: `_du_freezer_nuevo` usa `up=2` desde Goku; con la partida de la copia en español la rueda
  quedó en Yamcha (13), Freezer estaba a un paso. Mejor buscar la casilla por memoria (`0x8245F140`, u16) hasta 22.
