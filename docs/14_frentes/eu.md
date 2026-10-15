# Frente EU (ronda 2, 2026-10-15) — fase 1 (mapa US→EU, mecanismo, prueba) y fase 2 (todo el juego en EU)

## FASE 2 — ficheros ya en formato EU (leer antes de añadir ganchos o direcciones)
Estos ficheros ya funcionan con el juego europeo y **no tienen guardas `g_guest_eu`**. Al añadir código en ellos:
- **Gancho nuevo:** `DBZ3_HOOK_EU(sub_X) { … orig(ctx, base); }`, nunca `REX_HOOK_RAW` (en la EU no correría).
  No llames `orig` a una variable dentro del gancho.
- **Llamada o puntero a una función del juego** (también como argumento: `call(DBZ3_CALL_EU(sub_X), …)`):
  `DBZ3_CALL_EU(sub_X)`. El original fuera del gancho: `DBZ3_IMP_EU(sub_X)`.
- **Dirección fija:** `constexpr dbz3::GA kX = 0x82…;` o `dbz3::GA(0x82…)`. Un campo es `kX + n`.
- **Después:** `python tools/eu_map.py` y compilar. Si la función o dirección no tiene pareja EU comprobada, **no
  compila**. `python tools/eu_convertir.py src/X.cpp` hace todo esto solo; también sirve para revisar.

| Fichero | Estado |
|---|---|
| `hub.cpp`, `hub_menu.cpp`, `modos.cpp`, `rpg.cpp`, `transformaciones.cpp`, `tagteam.cpp`, `tagteam_slots.cpp`, `net/netplay_hooks.cpp`, `escenarios.cpp` (+ `escenarios_sel_gen_eu.inc`), `clima.cpp`, `cine.cpp`, `gigantes.cpp`, `combate.cpp`, `campaigns.cpp`, `pasarela.cpp`, `pasarela_misiones.cpp`, `select_ext.cpp`, `ingame/tagteam_panel.cpp` | EU: convertidos, sin guardas |
| `roster_ext.cpp`, `stage_ext.cpp`, `pasarela_b1.cpp`, `ingame/hub_panel.cpp` | ya valían en EU (parejas a mano, mismas direcciones o sin direcciones del juego) |
| `ports_guard.cpp` | EU: convertido tras cerrar ports (3 ganchos con su pareja EU, sin guardas). Vegeta GT con la presentación completa en la EU y el arreglo de sus 65 recursos actuando: PASS (`20261015-182306_eu_test_eu_eu_combate_exe_oficial`, roster rehecho en las copias EU) |
| `net/netplay.cpp`, `ingame/online_panel.cpp` | EU: convertidos, sin guardas. Online EU contra EU: PASS con reglas iguales (`20261015-183458_online_eu_reglas`: conexión, sala con rango, 130 comprobaciones del RNG iguales, retardo AUTO arriba y abajo, 52 usos de reglas iguales) y con Tag Team (`20261015-185002_online_eu_equipo`: el equipo no viaja, 1 contra 1, como en US). **US contra EU** (`20261015-185428_online_eu_mezcla`): los dos ven «the other PC has another game region (US/EU)» y no empieza ningún combate. Arreglo: el filtro del saludo (admisión y `ValidPacket`) solo aceptaba la región 0, así que un PC europeo se quedaba en «calling the host». Ahora acepta 0 o 1 y la diferencia la avisa `Handle`. Ojo: tras el arreglo, 1 de 3 intentos EU se quedó en «calling the host» (`184247`); al repetirlo, PASS. Con el mismo PC muy cargado; en US, esta prueba dio PASS las dos veces. Vigilar. Prueba: `E:\DBZ3HD_trabajo\eu\online_eu.py [--reglas …|--equipo|--mezcla|--us]` |

## FASE 2 — resultados (2026-10-15 tarde)
- **Qué ve el jugador con el juego europeo:** HUB, retos y modos (Supervivencia con 10 combates encadenados, Torneo de 4
  encadenado),
  Tag Team nativo con relevo, escenarios nuevos (selector y HUB), clima, intro de cine, jefe gigante, cápsula única
  de transformaciones, Dragon Universe con la historia de Freezer de los mods y la Leyenda de Mr. Satán.
- **Pruebas EU** en `D:\DBZ3HD\eu_test`, `eu_test2` y `eu_test3`, todas PASS. Se lanzan con
  `E:\DBZ3HD_trabajo\eu\pruebas_eu.py`, que es `tools/pruebas.py` con la carpeta de juego europea y las direcciones
  EU de pantalla y DU. Resultados en `D:\DBZ3HD\_pruebas\`:

  | Prueba | Carpeta |
  |---|---|
  | `batalla` (combate nativo + ring out) | `20261015-143856_eu_test_batalla` |
  | `supervivencia09` (HUB, 10 encadenados, premio) | `20261015-145706_eu_test_supervivencia09` |
  | `clima_namek`, `cine_intro`, `selector_combate` (casilla nueva Templo de Kami) | `20261015-150628_eu_test*_…` |
  | `gigante`, `hub_alta` (escenario nuevo por el HUB) | `20261015-150949_eu_test_gigante`, `…_eu_test3_hub_alta` |
  | `formas goku` (cápsula única: 5 formas arriba y abajo) | `20261015-151902_eu_test2_formas_goku` |
  | `tag_swap` (Tag Team: rueda, cápsulas, relevo con teletransporte) | `20261015-152544_eu_test_tag_swap` |
  | `campana` (DU, Freezer de mods, con `freezer_cap2`) | `20261015-162336_eu_test2_campana` |
  | `torneo_cadena` (HUB, Torneo de 4 encadenado, campeón) | `20261015-163903_eu_test3_torneo_cadena` |
  | `pasarela_eu` (Leyenda de Mr. Satán: ISO B1, combate, puntos, ronda 2) | `20261015-164943_eu_test_pasarela_eu` |

- **Regresión US:**
  - Batería `python tools/pruebas.py todo`: 5/5 PASS dos veces (15:4x y 17:0x, tras convertir campañas y pasarela).
  - Con el exe común y `REGION=us`, también PASS: `selector_combate`, `supervivencia09`, `formas goku` y
    `pasarela_eu` (`20261015-165915_eu_test2_pasarela_eu`).
- **Fallos encontrados y corregidos al convertir:**
  - `transformaciones.cpp` llamaba a funciones del juego pasadas como argumento (`call(sub_X, …)`) sin su pareja
    EU, y en la EU se cerraba al subir de forma (llamada indirecta a 0x820FA940). `eu_convertir.py` ya las caza.
  - En `select_ext.cpp`, "volver de la rueda" comparaba con una dirección de código US (0x8217CC68): en la EU nunca
    coincidía. El recuerdo del personaje elegido (0x824BA12F) tiene pareja EU 0x824BA11F, así que ya funciona en EU.
  - En `gigantes.cpp`, una variable llamada `orig` chocaba con el gancho. Rompió la compilación común unos minutos;
    se renombró a `mat` y `eu_convertir.py` ahora avisa de ese caso.
- **Resuelto (era el roster viejo de la copia, de las 03:37):** Vegeta GT (port IW, id 26) cerraba el juego al empezar su entrada en la presentación del
  Duelo (modo 1). Pasa igual en **US y EU** con el mismo exe (`…_eu_test_eu_us_combate` 14:31, lr 0x82082470 /
  ctr 0x820F4500). Con `DBZ3_HUB_MODE=5` va bien. Es para el frente ports.
- **Entorno de prueba:** la Leyenda busca la ISO de B1 en `ps2_games` junto a `dbz3.exe`. En `eu_test` y
  `eu_test2` hay un enlace (junction) a `D:\DBZ3HD\ps2_games`, sin copiar nada. La escena `torres.toml` del
  frente rpg es anterior a las torres encadenadas: espera el menú entre pisos y falla también en US. Por eso
  se probó `torneo_cadena`.
- **Para el coordinador:**
  - Añadir `src/escenarios_sel_gen_eu.inc` a `github/.gitignore`, junto a `escenarios_sel_gen.inc`: es código
    generado.
  - `CMakeLists.txt`: solo la condición del generador (también crea la gemela EU si falta).

## Qué ve el jugador (fase 1)
- **Antes (exe compartido de hoy):** con el juego europeo no hay HUB, modos, pasarela, escenarios nuevos, Tag Team,
  campañas ni el arreglo de Vegeta GT: cada fichero se apaga con `dbz3::g_guest_eu`. Control medido: el exe oficial
  en la copia EU no lanza el combate del HUB (`D:\DBZ3HD\_pruebas\20261015-094550_eu_test_eu_eu_combate_exe_oficial`,
  FAIL "el log no contiene 'hub: combate nativo'").
- **Ahora (solo en el exe de prueba del frente; el exe compartido no cambia en la fase 1):** el conector de combate
  del HUB y el arreglo de Vegeta GT funcionan también en la EU. Combate Vegeta GT contra Goku lanzado por el HUB en el
  juego europeo; al acabar, el arreglo actúa y se vuelve al menú principal sin cierre. Mismo resultado en US.

## Cómo engancha hoy el exe dual las funciones EU
- El exe lleva los dos códigos: US en `generated/` (`sub_X`) y EU en `generated_eu/` (`dbz3eu_sub_Y`, prefijo de
  `tools/prefix_eu_codegen.py`). Cada función es un símbolo débil: un gancho lo sustituye.
- `REX_HOOK_RAW(sub_X)` solo sustituye la función US: con el xex europeo el juego ejecuta `dbz3eu_sub_Y` y el gancho
  nunca corre. Una dirección US leída en la EU apunta a otra cosa. Por eso los ficheros se cortan con `g_guest_eu`.
- Lo que ya funcionaba en EU (`roster_ext`, `select_ext`, parte de `campaigns` y `tagteam`) usa `guest_region.h`:
  `DBZ3_HOOK(sub_X, dbz3eu_sub_Y)` (un cuerpo, dos ganchos), `DBZ3_CALL2` y `GuestAddr(us, eu)`, con las parejas
  medidas a mano (`D:\DBZ3HD\work\eu\INFORME.md`). `g_guest_eu` se fija en `roster::ApplyAtLaunch`, antes de que el
  juego ejecute nada.
- **No hay un desplazamiento fijo:** 164 de las 172 funciones usadas cambian de dirección, y en cantidades distintas.
  De los 78 datos, 24 se mueven (−0x10, −8, +8, +0x18…) y 54 se quedan igual.

## Mecanismo nuevo: `src/eu_map.h` + `src/eu_map_gen.h` (generado)
Solo cabeceras: no hace falta tocar `CMakeLists.txt`. En la build solo US todo vuelve a ser lo de antes.

| En el fichero | Antes | Después |
|---|---|---|
| gancho | `REX_HOOK_RAW(sub_X) { … __imp__sub_X(ctx, base); }` | `DBZ3_HOOK_EU(sub_X) { … orig(ctx, base); }` |
| llamada o puntero | `sub_X(ctx, base)` | `DBZ3_CALL_EU(sub_X)(ctx, base)` |
| original fuera de su gancho | `__imp__sub_X(ctx, base)` | `DBZ3_IMP_EU(sub_X)(ctx, base)` |
| dirección constante | `constexpr uint32_t kX = 0x82…;` | `constexpr dbz3::GA kX = 0x82…;` |
| dirección suelta | `REX_LOAD_U32(0x82…)` | `REX_LOAD_U32(dbz3::GA(0x82…))` |
| campo de estructura | `kCfg + 16` | igual (`GA + n` sigue siendo GA: el desplazamiento no cambia) |

- `DBZ3_HOOK_EU(sub_X)` toma la pareja de `DBZ3EU_sub_X` (generado) y usa el `DBZ3_HOOK` de siempre.
- `dbz3::GA` busca la pareja **al compilar** y elige la dirección al usarla, según el xex detectado.
- **Seguro:** una función o dirección sin pareja verificada **no compila** (probado: `GA(0x82000004)` y
  `DBZ3_HOOK_EU(sub_82000004)` dan error). Con eso no puede colarse una dirección EU sin medir.

## El mapa: `python tools/eu_map.py`
- **Salidas:** `src/eu_map_gen.h` (solo parejas verificadas) y `docs/14_frentes/eu_mapa.txt` (todas, con su
  evidencia y los ficheros que las usan). `--comprobar` ejecuta solo la autoprueba. Tarda unos segundos.
- **Entrada:** las imágenes ya volcadas (`D:\DBZ3HD\work\eu\{us,eu}_img.bin`), o `--xex-us/--xex-eu` para rehacerlas.
- **Ficheros revisados:** `hub`, `hub_menu`, `hub_panel`, `modos`, `pasarela*`, `escenarios`, `ports_guard`,
  `tagteam*`, `campaigns`, `select_ext`, `roster_ext` y `stage_ext`. También los nuevos de esta ronda (`clima`,
  `cine`, `gigantes`, `combate`, `rpg`, `transformaciones`) y las funciones de `tools/gen_escenarios_sel.py`.
- **Método:** se compara código con código; nunca se supone un desplazamiento.
  - **Funciones:** cuerpos normalizados (sin direcciones ni saltos), alineados en orden más hash único en las dos
    imágenes. Las únicas diferencias crudas permitidas son direcciones o saltos. Además, cada llamada (`bl`) desde
    un llamador emparejado debe caer en la pareja. Si la función no tiene llamadas, se comprueban los punteros a ella
    que calcula el código o sus tablas de punteros (vtables) con las vecinas emparejadas. Las funciones diminutas
    (menos de 6 instrucciones) siempre necesitan evidencia directa.
  - **Datos:** se recorre a la vez cada función US y su pareja EU, siguiendo los valores constantes de los registros
    (lis/addi/addis/ori/add). Cada sitio que calcula la dirección da un voto con su valor EU. Los datos con contenido
    se comparan byte a byte (los punteros, por su pareja).
  - **Datos que nadie calcula** (campos de estructuras): la base calculada más cercana por debajo y por encima debe
    tener el mismo desplazamiento US→EU.
  - **Código dentro de una función** (direcciones de retorno): misma distancia, y el `bl` previo apunta a la pareja.
- **Cobertura** (09:50; cambia al regenerar, porque los frentes siguen editando): 254 direcciones, **253 verificadas**. La que falta es `0x82300000` en `cine.cpp`, que es el
  principio de un rango de memoria, no una dirección del juego.

| Tipo | Total | Cómo se verifica |
|---|---|---|
| funciones | 172 | 141 por llamadas; 28 por punteros o tablas; 3 solo por alineación (cuerpos largos y únicos en las dos imágenes, bloques de 120–137 funciones iguales seguidas) |
| datos | 78 | 69 por votos del código (p. ej. 0x824BA110 → 0x824BA100 con 590/590 votos); 9 por las bases vecinas (p. ej. la casilla de DU 0x824A58A0 → 0x824A5890; la bandera de pelea 0x823F39A0 → igual) |
| código interno | 4 | misma distancia y `bl` previo coherente (p. ej. 0x821B98E8 → 0x821B86D0, la del roster ya probada) |

- **Comprobaciones cruzadas:**
  - La autoprueba recupera 10/10 parejas ya medidas y probadas en el juego (roster EU, 2026-10-07).
  - Hay 0 choques con las 76 parejas escritas a mano en `roster_ext`, `select_ext`, `campaigns` y `tagteam`.

## Prueba de concepto (fase 1)
- **Ficheros:** `ports_guard.cpp` (frente ports) y el conector de combate de `hub.cpp` (coordinador). Se hicieron en
  **copias**, en `E:\DBZ3HD_trabajo\eu\src\`; `src/` y el exe compartido no se tocaron.
  - Las copias salen de los ficheros actuales con `poc.py`, que usa `tools/eu_convertir.py` y luego quita las guardas.
  - `enlazar.py` compila las dos copias y enlaza un exe propio (`E:\DBZ3HD_trabajo\eu\bin\dbz3.exe`) con los demás
    `.obj` del build compartido, bajo el candado de `compilar.py`.
- **Copia de prueba:** `D:\DBZ3HD\eu_test`, una copia de `aura_test` con el mod de Vegeta GT (id 26) más el
  `default.xex` europeo y `eu\` (ya estaban). La carpeta de juego es la propia copia, con región `eu`; nunca la
  partida real.
- **Prueba:** `E:\DBZ3HD_trabajo\eu\prueba_eu.py eu|us` (usa `tools/pruebas.py`).
  - Lanza un combate nativo con `DBZ3_TEST_BATTLE=26,0,2,20` (Vegeta GT contra Goku COM, 20 s) y espera el menú
    final leyendo el estado del combate en memoria.
  - Después elige "Return to Main Menu" y espera el menú principal. La pantalla se lee en la dirección EU 0x824BA100.
- **Resultados** (todos PASS, sin errores en el log):
  - **EU:** `D:\DBZ3HD\_pruebas\20261015-092506_eu_test_eu_eu_combate`. El log dice `dual-region core detected
    EU/PAL`, `hub: combate nativo: id 26 contra id 0`, `dbz3 ports: tecnicas del lado 0 con 65 recursos (el juego
    admite 64): 1 sin liberar` y `hub: fin del combate nativo (resultado del juego 2)`. Capturas: menú, combate (Vegeta
    GT contra Goku en el escenario europeo), menú final y vuelta al menú principal.
  - **Primera pasada EU** (mecanismo sin la aritmética de GA): `…\20261015-090632_eu_test_eu_eu_combate`.
  - **US (regresión, mismo exe):** `…\20261015-091141_eu_test_eu_us_combate` y `…\20261015-093307_eu_test_eu_us_combate`.
  - **Control con el exe oficial en EU:** FAIL, no hay combate (ver arriba).

### Diff de `src/ports_guard.cpp` (frente ports)
```diff
-// Solo US (la base de las ranuras en la EU no esta medida). roster_build.py avisa de los BSP de mas de 64.
+// US y EU (frente eu: la base y la funcion EU, de src/eu_map_gen.h). roster_build.py avisa de los BSP de mas de 64.
-#include "guest_region.h"
+#include "eu_map.h"
-constexpr uint32_t kSlotsBase = 0x82333994, kCap = 64;
+constexpr dbz3::GA kSlotsBase = 0x82333994;
+constexpr uint32_t kCap = 64;
-REX_HOOK_RAW(sub_821266A8) {
-  if (!dbz3::g_guest_eu) {
+DBZ3_HOOK_EU(sub_821266A8) {
+  {
-  __imp__sub_821266A8(ctx, base);
+  orig(ctx, base);
```

### Diff de `src/hub.cpp` (coordinador; sobre la versión de las 09:24, con `ChainBattleEx`)
```diff
-// Solo US (las direcciones EU no estan medidas).
+// US y EU: cada gancho, llamada y direccion con su pareja EU (src/eu_map.h, frente eu).
-#include "guest_region.h"
+#include "eu_map.h"
-constexpr uint32_t kCfg = 0x8245EFC0, kReqCur = 0x8247A990, kSceneTask = 0x8247A9A0, kGame = 0x824BA110;
+constexpr dbz3::GA kCfg = 0x8245EFC0, kReqCur = 0x8247A990, kSceneTask = 0x8247A9A0, kGame = 0x824BA110;
-  if (dbz3::g_guest_eu || g_phase.load() != 0) return false;        (RequestBattleEx)
+  if (g_phase.load() != 0) return false;
-  if (dbz3::g_guest_eu || g_phase.load() != 3) return false;        (ChainBattleEx)
+  if (g_phase.load() != 3) return false;
-  sub_82179408(ctx, base);  /  sub_8218BA00 / sub_821D2778 / sub_821D27C8
+  DBZ3_CALL_EU(sub_82179408)(ctx, base);  / ... (las cuatro igual)
-  REX_STORE_U32(REX_LOAD_U32(kSceneTask) + 28, 0x821D7400);
+  REX_STORE_U32(REX_LOAD_U32(kSceneTask) + 28, dbz3::GA(0x821D7400));
-      REX_STORE_U32(REX_LOAD_U32(kSceneTask) + 28, 0x821D6A08);
+      REX_STORE_U32(REX_LOAD_U32(kSceneTask) + 28, dbz3::GA(0x821D6A08));
-REX_HOOK_RAW(sub_821D7C30) / sub_821D7D18 / sub_8216BF78 / sub_821D3628
+DBZ3_HOOK_EU(sub_821D7C30) / ... (los cuatro igual; dentro, __imp__sub_X(ctx, base) -> orig(ctx, base))
-  if (static const char* tb = std::getenv("DBZ3_TEST_BATTLE"); tb && !dbz3::g_guest_eu) {
+  if (static const char* tb = std::getenv("DBZ3_TEST_BATTLE"); tb) {
-  if (!dbz3::g_guest_eu && g_phase.load() == 1 && int32_t(REX_LOAD_U32(kReqCur)) == -1) {
+  if (g_phase.load() == 1 && int32_t(REX_LOAD_U32(kReqCur)) == -1) {
-  if (dbz3::hub::g_phase.load() == 3 && REX_LOAD_U8(0x824BA110) == 25) return;
+  if (dbz3::hub::g_phase.load() == 3 && REX_LOAD_U8(dbz3::GA(0x824BA110)) == 25) return;
```
Diffs completos: `E:\DBZ3HD_trabajo\eu\hub_eu.diff` y `ports_guard_eu.diff`. Para rehacerlos sobre los ficheros del
momento: `python E:\DBZ3HD_trabajo\eu\poc.py`.

## Fase 2: qué cambia en cada fichero (plan de la fase 1; el estado actual está arriba)
**Herramienta:** `python tools/eu_convertir.py src/X.cpp` muestra el diff mecánico (`--salida` lo escribe). Cambia
ganchos, llamadas, constantes y direcciones, y avisa de cada guarda `g_guest_eu` y de lo que hay que hacer a mano.
Borradores ya generados en `E:\DBZ3HD_trabajo\eu\fase2\src\` (`.cpp`, `.diff`, `.avisos`): **los 20 compilan** en dual
y en solo US (`clang -fsyntax-only`), con 3 arreglos a mano ya puestos (abajo). Los borradores **conservan** las
guardas: la EU se enciende al quitar cada una y probarla en el juego.

| Fichero (dueño) | Líneas mecánicas | Ganchos | Guardas a revisar | A mano |
|---|---|---|---|---|
| `hub.cpp` (coordinador) | 34 | 4 | 4 | hecho y probado (arriba) |
| `ports_guard.cpp` (ports) | 9 | 1 | 1 | hecho y probado (arriba) |
| `hub_menu.cpp` (hub) | 61 | 5 | 1 (`Enabled()`) | — |
| `modos.cpp` (rpg) | 35 | 6 | 12 | — |
| `pasarela.cpp` (campañas) | 6 | 0 | 5 | depende de `hub.cpp` |
| `escenarios.cpp` (escenarios) | 85 | 10 | 2 | **`escenarios_sel_gen.inc` es código US copiado**: falta su gemelo EU (como `select_wheel_gen_eu.inc`): `gen_escenarios_sel.py` debe generar `escenarios_sel_gen_eu.inc` desde `generated_eu/` (parejas: `import eu_map`), y `ConBanco<Sel_X>` pasa a `DBZ3_CALL2(Sel_X, SelEu_X)` |
| `clima.cpp`, `cine.cpp` (escenarios) | 14 / 10 | 1 / 0 | 2 / 2 | `cine.cpp`: 0x82300000 es un rango, se queda |
| `tagteam.cpp` (coordinador/tag) | 198 | 12 | 29 | `kPoseRec` aparte: `constexpr dbz3::GA kPoseRec = dbz3::GA(0x8238B780) + 184 * 3;` |
| `tagteam_slots.cpp`, `tagteam_panel.cpp` | 6 / 0 | 0 | 0 / 1 | — |
| `campaigns.cpp` (campañas) | 96 | 7 | 9 | 0x824A58A0 (casilla de DU) ya tiene pareja: 0x824A5890 |
| `select_ext.cpp`, `roster_ext.cpp` | 12 / 52 | ya dobles | 2 / 2 | `roster_ext:1051`: `uint32_t(kPortraits)` en la lista `{…}` |
| `stage_ext.cpp` (escenarios) | 5 | 0 | 0 | — |
| `combate.cpp` (combate), `gigantes.cpp`, `rpg.cpp` | 40 / 4 / 0 | 6 / 0 / 0 | 6 / 1 / 1 | `combate.cpp:145`: `uint32_t(kPads + 184 * uint32_t(rec))` en el `?:` |

- **Orden propuesto** (cada paso: compilar, `prueba_eu.py eu` y `us`, y la batería):
  1. `hub.cpp` + `ports_guard.cpp` (ya probados).
  2. `hub_menu.cpp` (entrada del HUB).
  3. `modos.cpp` + `pasarela.cpp`.
  4. `campaigns.cpp`.
  5. `escenarios.cpp` + su `.inc` EU.
  6. `tagteam.cpp` (29 guardas, la más larga).
  7. Los nuevos de la ronda.
- **`tools/pruebas.py` (propuesta):** opción `--region eu`. La carpeta de juego pasa a ser la copia (con `default.xex`
  EU y `eu\`, como `D:\DBZ3HD\eu_test`), `dbz3_region = "eu"` y `PANTALLA = GBASE + 0x824BA100`.
  `E:\DBZ3HD_trabajo\eu\prueba_eu.py` lo hace en ~120 líneas. Hasta que `tagteam.cpp` vaya en EU, sus líneas del log
  (`tag: …`) no salen en la EU: el estado del combate se lee en memoria (`*(*0x823D6CF4 + 35128)`, igual en US y EU).
- **Tras cada cambio de código en esos ficheros:** `python tools/eu_map.py`. Una dirección nueva sin pareja hace fallar
  la compilación hasta medirla.

## Lo que queda / límites
- **Fase 2 entera:** quitar guardas fichero a fichero y probar cada modo en el juego europeo (HUB, retos, Torre,
  Leyenda de Mr. Satán, Dragon Universe, escenarios nuevos, Tag Team).
- **Fuera de este frente:** online (`src/net/*`; US y EU no juegan entre sí).
- **No hace falta en EU:**
  - `hooks.cpp`: en la EU, el fallo de codegen ya lo corrige `tools/fix_eu_bctr.py`.
  - Las constantes 0x82000000 de `campaigns` y `roster_ext`: son comprobaciones de rango.
- **Aviso del launcher** ("solo con la versión US"): `online_panel`, `pasarela.cpp:671`. Hay que cambiarlo cuando cada
  modo funcione en EU.
- **`[[parche]]` crudos de `roster.toml`** (dir = 0x…): siguen siendo direcciones US (ya documentado en el informe del
  roster EU).

## Ficheros del frente
- **Nuevos en el repo:** `src/eu_map.h`, `src/eu_map_gen.h` (generado), `tools/eu_map.py`, `tools/eu_convertir.py`,
  `docs/14_frentes/eu_mapa.txt` (generado) y este `eu.md`. No se tocó ningún fichero de otro frente ni `CMakeLists.txt`.
- **Fuera del repo:** `E:\DBZ3HD_trabajo\eu\` (copias y diffs de la prueba, `enlazar.py`, `prueba_eu.py`, `poc.py`,
  borradores de la fase 2).
- **Copia de prueba:** `D:\DBZ3HD\eu_test` (430 MB).
