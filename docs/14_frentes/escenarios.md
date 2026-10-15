# Frente: Escenarios (escenarios de otros juegos como casillas nuevas)

Fecha: 2026-10-15. Solo US (como hub.cpp). Estado vivo: este documento.

# Ronda 2 (2026-10-15): fichas 10, 19, 22 y extra E1

Todo **apagado por defecto** (opción de `dbz3_user.toml` o regla de reto), solo US, **fuera en online** (solo se
vería en un PC). Ningún combate de serie cambia si no se enciende. Pruebas en `D:\DBZ3HD\escenarios_test{,2,3}`.

## Puntuación (entregable mínimo probado en el juego)
| Ficha | Qué ve el jugador ahora | Prueba (capturas en `D:\DBZ3HD\_pruebas\`) | Nota |
|---|---|---|---|
| 10 clima | Con `dbz3_clima = true`: a los 20 s de pelea el cielo y el escenario se van oscureciendo (40 s, suave) hasta una noche; los luchadores siguen con su luz; el fondo lejano se apaga con una niebla azul oscura. Usa el oscurecimiento del propio juego (el de entrar en modo hiper), que sigue funcionando igual. | `clima_namek` (prueba rápida 3 s + 8 s): `01_antes`, `02_medio`, `03_oscuro` (ver §R5) | **1** |
| 19 gigantes | Con `dbz3_jefe_gigante = 1.8`: el rival COM sale 1,8 veces más grande (modelo, huesos y sombra, pies en el suelo) y la cámara abre el campo de visión para que se le vea entero. Los golpes llegan en los dos sentidos y se le puede tumbar. | `gigante` (vida de los dos baja: golpes de ida y vuelta), `gigante_ko` (Goku tumba al jefe gigante, combate acabado) (ver §R5) | **1** (contacto, cámara y victoria; sin ataques nuevos de jefe: ver pendiente) |
| 22 cine | Con `dbz3_cine_intro = true` (o `Pedir()` desde un reto): al empezar la pelea, 10 s de cine con bandas negras: plano del escenario con su nombre, Goku de cerca con su nombre, "VS Vegeta" de cerca; vuelve a la cámara del juego fundiéndose. Los dos esperan en guardia. **A** la salta; terminada o saltada, la pelea sigue igual. | `cine_intro` (fin natural, la pelea sigue), `cine_salto` (A a los 2 s, misma pelea hasta el ring out) (ver §R5) | **1** |
| E1 nombres | `dbz3::escenarios::Nombre(registro[, idioma])`: "Planeta Namek", "Ring de Cell"... en los 6 idiomas del juego (las altas con su `titulo`). Lo usa ya la intro de cine (rótulo del escenario). | `nombres_idioma` (francés en el log) + log de cada arranque en español; rótulos comprobados contra la textura del juego (data_spn 2043) | **1** |

`stage_ext.cpp` revalidado (ficha 29 lo pedía): ya no tiene ganchos temporales de diagnóstico (solo aplica
`stage.toml` y llama a `escenarios::CargarDeMods`); compila y las pruebas `sin_mods` y `hub_alta` siguen en PASS.

## R1. API para otros frentes (retos, HUB, campañas)
```cpp
#include "escenarios.h"   // dbz3::escenarios
std::string Nombre(int registro);              // en el idioma del jugador (dbz3::settings::Language())
std::string Nombre(int registro, int idioma);  // 1 EN, 2 JP (= inglés: el juego no trae texto japonés y la letra es latina), 3 DE, 4 FR, 5 ES, 6 IT
int Total();                                   // 17 de serie + altas de mods (registros 17+)
#include "clima.h"        // dbz3::clima::Activar(true) durante un reto (false al salir)
#include "gigantes.h"     // dbz3::gigantes::Activar(lado, escala) (0 = quitar; 0.5..3); lado 1 = jugador 2
#include "cine.h"         // dbz3::cine::Pedir()  -> intro de 10 s en el combate siguiente (una vez)
```
Opciones (`dbz3_user.toml`, todas apagadas): `dbz3_clima = true`, `dbz3_jefe_gigante = 1.8` (rival COM),
`dbz3_cine_intro = true`. Pruebas: `DBZ3_CLIMA=1|rapido`, `DBZ3_GIGANTE=<escala>` (jugador 2), `DBZ3_CINE=1`.
Sondas: `DBZ3_SONDA_CLIMA=1` (niebla/luz de los 17 registros y valores cada 2 s), `DBZ3_SONDA_GIGANTE=1` (vida y
cámara cada 2 s), `DBZ3_SONDA_VOLCADO=f1,f2,...` (vuelca 0x82300000-0x82500000 en esos fotogramas de pelea, para
medir comparando volcados).

## R2. Medidas nuevas (US)
- **Cámara de la pelea**: `mgr = *0x823D6CF4` (0x823CE1E0, estático). `mgr+0x60` matriz de vista 4×4 de mano
  derecha (filas `der.x arr.x atr.x 0 / … / -der·ojo -arr·ojo -atr·ojo 1`), copia en 0x824E6720 (la usa el recorte de
  objetos); `mgr+0x190` ojo, `+0x1A0` objetivo, `+0x1B0` arriba, `+0x1C4` fov vertical (0,785). Escribir ojo,
  objetivo y matriz al empezar el dibujo (`sub_820C2058`) mueve la cámara de todo el fotograma. **El juego parte de
  su ojo anterior**: alejarlo en cada fotograma lo manda al infinito (medido: 1e16 a los 2 s) → para volver, fundirse.
  En la presentación (estados 1-2) manda la cámara del guion del escenario (ojo `+0x150`, objetivo `+0x160`, fov
  `+0x184`) y la matriz que dibuja no es ninguna de esas (probado escribiendo las tres): ahí no se puede mover.
- **Luchador**: `X = mgr+2096+8240·lado` empieza con su **matriz de mundo** (filas 0-2 giro, fila 3 = posición, X+48);
  el juego la rehace cada fotograma. Escalar sus filas 0-2 **solo mientras se dibuja** (`sub_82109D90`, dibujo del
  luchador; se restaura después) agranda todo el modelo desde los pies; la lógica no la ve cambiada. `X+5028` mando
  (-1 COM; un mando vacío = en guardia quieto). En la presentación están ocultos (`X+3261 = 1`) hasta su entrada.
- **Oscurecimiento del escenario** (el del modo hiper, cielo incluido, luchadores no): `mgr+35180` cuánto (el juego
  sube a 0,75 y baja a 0) y `mgr+35184` su fundido 0..1. Hallado con volcados antes/durante/después del modo hiper
  (rival quieto con `X+5028 = 1`). El efecto común 300 (`sub_82127B20` caso 0) es el que lo pide.
- **Niebla**: `sub_820C2058` borra y pone la niebla del escenario (`G+76 & 0x20`): `sub_82085650` (activa,
  0x8238941C), `sub_82085670` (color 0x82389424), `sub_82085688` (inicio/fin 0x823106B4/B8). Tabla por registro:
  `0x82323760 + i·92 + 0x1C + variante·32` → `[0]` luz (0x82322B30, 80 B) y `[1]` niebla (0x823232A0, 20 B). Namek:
  color 0x0057D7FF, 0..3800. La niebla no llega al cielo.
- **No sirve** para oscurecer: el color de los materiales del #AWO del escenario (no se usa al dibujar; probado con
  los 57 materiales de Namek). `G+0x20` = #ZDD cargado; en memoria la cabecera del #AWO empieza en su marca (en el
  fichero, +0x40).
- Namek B (registro 14) es el Namek que explota (cielo rojo oscuro); otra entrada del AFS, no un estado del mismo mapa.

## R3. Ficheros
`src/clima.cpp/.h` (nuevo; gancho `sub_820C2058`, del que cuelgan también cine y gigantes), `src/cine.cpp/.h`
(nuevo), `src/gigantes.cpp/.h` (nuevo; gancho `sub_82109D90`; no estaba en la lista de RONDA2: es un fichero propio
nuevo, nadie más lo engancha), `src/escenarios.cpp/.h` (E1: 6 idiomas, `Nombre(registro)`, log de nombres al
arrancar; avisa a clima al pedir escenario), `CMakeLists.txt` (los 3 .cpp en `DBZ3_SOURCES` y en el `REMOVE_ITEM`
solo-EU). `stage_ext.cpp` sin cambios. Pruebas nuevas en `escenarios_pruebas/`: `clima_namek`, `gigante`,
`gigante_ko`, `cine_intro`, `cine_salto`, `nombres_idioma`, `cine_gigante` (R6b). Las de medida (ya sin sonda en el código) movidas a
`_respaldo/escenarios_pruebas_medida_2026-10-15/`.

## R4. Pendiente
- 10: un solo disparador (tiempo). Ráfagas de ki o daño como disparador, lluvia/relámpagos (efectos #AME) y cambiar
  la textura del cielo: sin hacer. Fuera del combate (pose final) el cielo sigue oscuro hasta el combate siguiente.
- 19: es escala de un luchador normal: sin ataques propios de jefe (pisotón, ráfaga de área) ni inmunidad a
  agarres (los agarres se ven algo desplazados). El alcance y el cuerpo de golpe ya escalan (WL-19). En la pose final
  la cámara vuelve a su campo de visión normal.
- 22: la intro va al empezar la pelea, no sobre la presentación del juego (allí la cámara no se deja mover). Planos
  fijos por código (sin editor de línea de tiempo ni voces); el tiempo del combate (si lo hay) corre durante la intro.

## R5. Pruebas (copias `escenarios_test`, `escenarios_test2`, `escenarios_test3`)
`python tools/pruebas.py correr <copia> docs/14_frentes/escenarios_pruebas/<prueba>.toml`. Resultados: §R6.

## R6. Resultados (build `python tools/compilar.py` OK; todas en PASS)
| Prueba | Carpeta (`D:\DBZ3HD\_pruebas\`) | Lo que demuestra |
|---|---|---|
| `clima_namek` | `20261015-122154_escenarios_test_clima_namek` | log `oscuro 0.02 → 0.17 → 0.38 → 0.53 → 0.55`, niebla `0057D7FF → 0012142C`, 3800 → 570; capturas: cielo verde → oscuro, luchadores con su luz |
| `gigante` | `20261015-122154_escenarios_test2_gigante` | Vegeta ×1,8, campo de visión 0,785 → 1,100; vida 3000/3000 → 2759/2903 (golpes de ida y vuelta); agarre y modo hiper del gigante en captura |
| `gigante_ko` | `20261015-122154_escenarios_test3_gigante_ko` | `combate acabado: … gana el lado 1, motivo 0` (KO) y "WINNER!" de Goku |
| `cine_intro` | `20261015-122552_escenarios_test_cine_intro` | `empieza la intro ('Planeta Namek', Goku contra Vegeta)` → `terminada en el fotograma 601`; 3 planos con rótulos y "A Saltar"; la pelea sigue |
| `cine_salto` | `20261015-122552_escenarios_test2_cine_salto` | A a los 2 s: `saltada en el fotograma 170`; Goku no ataca con esa A; mismo combate hasta el final (`motivo 4`) |
| `nombres_idioma` | `20261015-122552_escenarios_test3_nombres_idioma` | `nombres (idioma 4): 0 Niveau Champ. du Monde, … 7 Planète Namek, 8 Anneau de Cell…` (y en español en todas las demás) |
| `hub_alta`, `sin_mods` | `20261015-123051_escenarios_test_hub_alta`, `…test2_sin_mods` | sin regresiones: altas de mods (HUB con presentación y ring out) y selector de siempre |
| `python tools/pruebas.py todo` | `20261015-125253_todo.txt` y, tras el arreglo de R6b, `20261015-143036_todo.txt` | 5/5 PASS: tag_swap, campana, du_continuar, batalla, menu (online y Tag Team sin cambios) |

## R6b. Gigante + intro a la vez (aviso del hub, 2026-10-15 tarde)
- **Antes**: con "Intro de cine" y "Jefe gigante" encendidos, el rival salía normal durante la intro y crecía al
  acabar. Causa: la intro le cambiaba el mando al COM (para dejarlo en guardia) y el gigante solo crece si el lado es
  el COM. Además la intro escribía `X+5028` como 32 bits (hub midió que es de 16: `X+5030` va aparte), así que en
  realidad los dos luchadores quedaban con el mando 1.
- **Ahora**: `cine::Mando(lado)` da el mando de antes de la intro y `gigantes::Escala` lo usa (se conserva el arreglo
  de hub: lectura de 16 bits). La intro solo toca al COM (16 bits, mando 1 conectado: un mando vacío saca el aviso de
  "mando desconectado") y lo devuelve a su IA al acabar. Los planos de la intro encuadran con la escala de cada lado
  (`gigantes::EscalaDe`): "VS Vegeta" muestra al gigante entero.
- Presentación del juego: el gigante ya sale gigante (el dibujo escala en todos los estados), pero sus cámaras son
  las del guion y no se pueden abrir: a veces se le corta la cabeza.
- Prueba `cine_gigante` (opciones `dbz3_jefe_gigante = 1.8` y `dbz3_cine_intro = true`, presentación completa):
  `D:\DBZ3HD\_pruebas\20261015-141729_escenarios_test_cine_gigante` (presentación con el gigante, 3 planos de la
  intro con el gigante, pelea con campo de visión 1,100). Con ella, `cine_intro` y `gigante` otra vez en PASS.
- Límite: durante la intro el COM obedece al mando 1, así que si el jugador mueve el mando antes de saltarla, los dos
  se mueven igual (sin tocar nada, los dos esperan en guardia).

## WL. Casos de `studies/11_vision_futura/INTEGRACION_SEGURA.md` (2026-10-15 tarde)

### WL-22 (intro original, fin natural y skip; misma escena siguiente, limpieza y sin recompensa doble) — **PASS**
Reto de 1 combate con `intro = true` y `premio = 777` (`escenarios_pruebas/wl22_kumite.toml`, copiado a
`user_data/dbz3/modos/kumite.toml` de la copia; `DBZ3_TEST_MODO=kumite,0,0`, `DBZ3_TEST_KO=1`).
| Prueba | Carpeta (`D:\DBZ3HD\_pruebas\`) | Resultado |
|---|---|---|
| `wl22_reto_intro` (intro entera) | `20261015-164941_escenarios_test_wl22_reto_intro` | `intro terminada en el fotograma 601` → KO → `kumite terminado: victoria (1 de 1)` → `premio de 777 Zenie (monedero 777)` **una vez** |
| `wl22_reto_salto` (A a los 2 s) | `20261015-164941_escenarios_test2_wl22_reto_salto` | `intro saltada en el fotograma 187` → KO → misma escena (`kumite terminado: victoria (1 de 1)`) → `premio de 777` **una vez** |
Limpieza: al acabar o saltar, la cámara vuelve a la del juego (fundido o en seco), el COM recupera su IA (`X+5028`
de 16 bits, restaurado) y `Pedir()` vale para un solo combate (`NuevoCombate()` en cada carga de escenario). La intro
no toca resultados ni premios: el premio lo da `modos.cpp` al acabar el reto, igual con y sin intro.

### WL-19 (jefe grande: esquive/colisión/cámara y victoria finita; malla grande sola no acredita) — **PASS**
- **RE (medido)**. El golpe lo deciden `sub_8211B738(V)` (cuerpo a cuerpo) y `sub_82112588(V)` (efectos pegados):
  cada **nodo de golpe** del atacante `A = [V+4948]` (lista `[A+2752]`, siguiente `+44`; `+0` posición de mundo,
  `+16` la anterior, `[+32]` su línea de golpe: `+8` banderas, `+12` radio u8) contra cada **punto de cuerpo** de V
  (`V+1616 + 16·i`, 22 puntos que saca `sub_820F9548` de los huesos de la lógica) con su radio
  `[[V+2996] + 24 + 4·i]`: entra si `distancia² ≤ (radio_i + radio_golpe · [[A+2996]+0])²`. Los nodos salen de los
  huesos de la lógica **sin escalar** (`sub_82112AB0`): de ahí el tope (~22) de un luchador normal.
- **Hecho** (`src/gigantes.cpp`, solo para un gigante, US y EU con parejas verificadas): puntos de cuerpo escalados
  desde sus pies (`sub_820F9548`) y, **solo durante esas dos comprobaciones**, sus nodos de golpe alejados de sus pies
  (horizontal ×escala; vertical ×escala del golpeado, para que su puño no pase por encima de un rival normal), su
  radio de golpe ×escala y, si es el golpeado, sus radios de cuerpo ×escala. Después todo vuelve a como estaba; un
  combate sin gigante no pasa por nada de esto. (Probado y quitado: escalar los huesos de la lógica; el puño iba a
  23 de altura y no le daba a un rival normal.)
- **Medido** (los dos clavados en su sitio con `DBZ3_GIGANTE_DISTANCIA`; pruebas en `E:\DBZ3HD_trabajo\escenarios\alc_*.toml`;
  8 puñetazos, golpes que entran):
  | Quién pega a quién | 12 | 18 | 21 | 24 | 30 | 36 | 40 |
  |---|---|---|---|---|---|---|---|
  | normal → normal | 7 | 7 | 3 | **0** | 0 | | |
  | gigante ×2 → normal | 7 | 7 | 4 | | **7** | **3** | |
  | gigante ×2 → gigante ×2 | | | | | | | **3** |
  | normal → gigante ×2 quieto | | | | **7** | | | |
  Carpetas: `20261015-181710_*_alc_g{21,30,36}`, `20261015-182053_*_alc_{n24,gg40,victima_g24}` y las de normal
  `20261015-161710_*_alc_n{12,18,24}`, `…173533…`/`…174311…` (n21, n30).
- **Juego** (`20261015-182349_*`): `gigante` (Vegeta COM ×1,8 contra Goku): 8 golpes a Goku y 4 al gigante en 12 s;
  `gigante_ko` (KO, "WINNER!"), `cine_gigante` (gigante en presentación, intro y pelea). Cámara: campo de visión e
  intro. Esquive: el gigante es el COM del juego (esquiva y se cubre como siempre).
- Límite: sin ataques propios de jefe (pisotón, ondas); el puño de un gigante contra un rival normal pega a la altura
  del rival aunque se vea más arriba.
- Compilación OK; `python tools/pruebas.py todo` 5/5 PASS (`20261015-183417_todo.txt`).
- Nota: entre las 17:15 y las 17:36 faltaron los datos del juego (`us\`, aviso del coordinador); las pruebas de ese
  intervalo (pantalla de carga) se repitieron después.

## R7. Propuestas para otros frentes
- **rpg/modos (`modos.h`, `Reglas`)**: tres reglas de reto con lo de este frente, p. ej. en el TOML del reto
  `clima = true`, `gigante = 1.8`, `intro = true`; al empezar cada combate del reto:
  ```diff
  +#include "cine.h"
  +#include "clima.h"
  +#include "gigantes.h"
  +  dbz3::clima::Activar(r.clima);
  +  dbz3::gigantes::Activar(1, r.gigante);     // 0 = sin gigante
  +  if (r.intro) dbz3::cine::Pedir();
  ```
  y al acabar el reto `Activar(false)` / `Activar(1, 0)`. Un "jefe gigante" de verdad = `gigante` + la armadura de
  jefe que ya tiene `Reglas`.
- **HUB / retos / torneo**: mostrar el escenario por su nombre: `dbz3::escenarios::Nombre(c.escenario)` en vez de
  "escenario N" (hoy solo aparece el número en los logs de `hub.cpp`, `hub_menu.cpp`, `modos.cpp`, `pasarela.cpp`).
- **hub (F1 / launcher)**: tres interruptores en "Mods nativos": `dbz3_clima`, `dbz3_jefe_gigante` (deslizador
  1,0-2,5) y `dbz3_cine_intro` (textos: "Clima vivo: el cielo se oscurece durante el combate", "Jefe gigante",
  "Intro de cine al empezar el combate").
- **campañas**: `dbz3::cine::Pedir()` antes del primer combate de una campaña (intro propia con el nombre del
  escenario y de los luchadores).

# Ronda 1

## Qué ve el jugador
- **Antes**: el escenario convertido de Budokai 2 cerraba el juego al empezar el combate, y solo podía entrar
  sustituyendo uno de serie.
- **Ahora**: cada mod de escenario añade una **casilla nueva** en "Selección de fase" (Duelo), con su bola, su
  vista redonda, su vista grande y su rótulo con la letra del juego, entre los 12 de serie y el "?". Se pelea
  en él con presentación, cámaras, suelo y ring out. Los 17 de serie siguen igual y en su sitio; sin mods el
  selector es el de siempre. También se lanza desde el HUB (`dbz3::hub::RequestBattle(id1, id2, 17+, ...)`).
- Probados en el juego: **Templo de Kami** y **Montañas nevadas** (Budokai 2) y **Páramo al atardecer**
  (Budokai 1). Capturas: `E:\DBZ3HD_trabajo\escenarios\capturas\informe_2026-10-15\`.

## Ficheros del frente
- `tools/escenarios_convertir.py` (nuevo): conversor B2/B1 PS2 → bin de escenario HD + mod opcional;
  `comprobar` reproduce los 13 escenarios nativos B3 PS2 → HD (grafo campo a campo, cámaras y script idénticos).
- `tools/gen_escenarios_sel.py` (nuevo) → `src/escenarios_sel_gen.inc` (generado): copias de 12 funciones del
  selector (mn_StageSel) con datos ampliados (656 → 1024 B, carrusel de 24) y tablas por casilla propias.
- `src/escenarios.cpp/.h` (nuevos): altas (registros 17+ desde `stage.toml` `[[escenario]]`), selector con
  casillas nuevas, `Nombre()`/`Total()` y sonda de colisión (`DBZ3_SONDA_ESCENARIO=1`).
- `src/stage_ext.cpp`: quitados los ganchos TEMPORALES de diagnóstico; llama a `escenarios::CargarDeMods`.
  Copia previa en `_respaldo/stage_ext.cpp.antes_escenarios_2026-10-15`.
- `CMakeLists.txt`: `src/escenarios.cpp` tras `src/hub.cpp` y en el REMOVE_ITEM solo-EU.
- `docs/14_frentes/escenarios_pruebas/*.toml`: pruebas de `tools/pruebas.py` (ver §6).

## 1. Causa raíz del cierre (B2 "Archipiélago", AFS 5000, lr 0x820CB134)
`sub_820CB0A0` es la consulta de altura del suelo: baja por el árbol st50 (BSP) del objeto y lee la lista de
registros de colisión rec30 de la hoja. Cada rec30/rec20 apunta **dentro del bloque de colisión** que cuelga
de la lista de un hueso (4.ª palabra) del modelo del escenario (cabeceras f4/f3, nodos, vértices y triángulos).
`amo2awo` (conversor de personajes) no conoce ese bloque y lo tiraba; el conversor viejo copiaba los rec como
si fueran del grafo (o los ponía a 0) → puntero NULL al caminar.
- **Medido con sonda** (gancho a `sub_820CBC90`, el reubicador): versión vieja "0 registros dentro de su
  modelo, 2 fuera" y cierre; versión nueva "183 dentro, 0 fuera" (Templo de Kami), 332 (Montañas), 409 (Páramo).
- **Arreglo**: `conv_colision` convierte el bloque (idéntico byte a byte en los 20 modelos con colisión de los
  13 pares nativos) y el grafo se copia con el esquema de punteros del reubicador (`Grafo`), traduciendo los
  punteros internos. `comprobar`: 13/13 idénticos.
- Segundo cierre (al lanzar desde el HUB/Duelo, con presentación): el script #SPX del donante pide las pistas
  0..2 del primer #AMC del objeto 0; B2 trae 1-7 pistas repartidas en 2 #AMC → se unen (`amc_pistas`).

## 2. Altas (registros 17+)
La tabla de 17 registros (0x82323760) está pegada a la de temas: no se puede alargar. Un alta usa el hueco del
registro **anfitrión 16 (R&R BASE)** solo mientras está cargada: `sub_820C26E0` (carga, obj+340) y
`sub_820C19B0` (id_SE) traducen 17+ → 16 con los datos del alta, y cualquier registro de serie devuelve el
original. CFG+340 conserva el 17+, así "Luchar otra vez" vuelve a cargarla. `stage.toml`:
```toml
[[escenario]]
nombre = "MONTANAS NEVADA"     # registro (<= 15)
titulo = "Montañas nevadas"    # para pantallas propias (Nombre())
entrada = 5007                 # data_cmn.afs, >= 5000 (B2: 5000+grupo, B1: 5020+grupo)
menu = 5107                    # imágenes de la casilla (entrada + 100)
id_se = 45                     # sonido/ambiente del donante
tema = 16                      # luz y cielo genéricos
```
**Nombres** (extra E1, ampliado en la ronda 2 a los 6 idiomas y `Nombre(registro)`): `dbz3::escenarios::Nombre(registro, idioma)` da el nombre oficial de los 17 de serie en
inglés/alemán/francés/español/italiano (copiado de los rótulos del selector, data_<idioma> 2043) y el `titulo`
de las altas; `Total()` = 17 + altas. Las variantes (ISLAND B, TOWN B, NAMEC B, KAIOU B, EARTH) llevan el
nombre de su mapa.

## 3. Selector de escenarios con casillas nuevas
- Datos del selector ampliados (carrusel de 24, lista propia) en copias generadas de sus funciones; la lista
  es: casillas de serie desbloqueadas, altas y el "?" al final (el azar elige entre todas menos el "?").
- Cada alta trae su #AZT (entrada+100) con 4 texturas: círculo, vista, rótulo y bola de color (copiada del
  juego del jugador). Se carga antes que las del juego y se registra; mientras se dibuja una casilla nueva se
  pone su #AZT en D+4 (`ConBanco`) y luego vuelve el del juego (también como banco global).
- Cierre resuelto con el alta de B1 (medido): con el #AZT del alta como banco global, un sprite del juego pide
  su textura 37 → leía basura detrás del índice (con las de B2 salía un 0 por suerte). Arreglo: índice del
  #AZT relleno hasta 64 huecos (los sobrantes a una entrada vacía) y el banco del juego restaurado tras cada
  llamada envuelta.
- Imágenes de la casilla: foto en perspectiva del escenario sin GPU (`foto_escenario`), o `--foto captura.png`.

## 4. Herramienta
```
python tools/escenarios_convertir.py lista [--juego b1]
python tools/escenarios_convertir.py mod 7 --nombre "Montanas nevadas" --rotulo "Montañas nevadas" --mods <juego>\mods
python tools/escenarios_convertir.py mod 3 --juego b1 --nombre "Paramo al atardecer" --rotulo "Páramo al atardecer"
python tools/escenarios_convertir.py comprobar
```
Lee la ISO del jugador de `ps2_games/` y su `us/data_cmn.afs` (script, efectos y cámaras de presentación del
escenario donante 44); escribe solo en la carpeta de mods. **Nada de otro juego va en el repo ni en la release.**
Diferencias de B1 resueltas: objeto de 0xD4 B (sin cámaras: van las del donante), entradas de animación de 4 B
(`[bloque]` → `[1, bloque, parámetros, 0]`), sin parámetros (los de siempre).
`scenarios converter/b2stages.py` y `zdd_*.py` quedan superados (sus hipótesis de reubicación/#ACE no eran la
causa).

## 5. Viabilidad por juego
| Juego | Escenarios | Estado |
|---|---|---|
| Budokai 2 (PS2) | 9 grupos | Templo de Kami (g2) y Montañas nevadas (g7): selector + combate + presentación OK. g3-g6 entraron en combate en pruebas anteriores. g1 (BUDO) y g8 (TOWN) cierran al dibujar el modelo (`sub_8208E518`, lr 0x8208E5AC): pendiente. g0 sin probar. |
| Budokai 1 (PS2) | 13 grupos | 11 se convierten; Páramo al atardecer (g3) OK en Duelo y HUB. g9 y g11: cabecera de colisión distinta (no convierte). El resto, sin probar en el juego. |
| Infinite World (PS2) | 13 | Mismo formato y mismos 13 escenarios que B3: no aporta nada nuevo. |
| Shin Budokai (PSP) | BB*.AMB | Solo mallas y texturas (#AMO/#AMT/#AMM): sin grafo, colisión ni cámaras. Haría falta fabricarlos: no viable a corto plazo. |

## 6. Pruebas (copia `D:\DBZ3HD\escenarios_test`)
`python tools/pruebas.py correr escenarios_test docs/14_frentes/escenarios_pruebas/<prueba>.toml`
- Con los 3 mods (orden: 17 Montañas, 18 Páramo, 19 Templo): `final_tres` (selector con 3 casillas, combate en
  Montañas) PASS, `final_paramo` (combate en Páramo) PASS, `hub_alta` (HUB → alta 18 con presentación completa
  y ring out) PASS.
- Con 2 mods (17 Montañas, 18 Templo): `selector_combate` PASS, `selector_nieve` PASS, `revancha_nativo`
  (Luchar otra vez en el alta, reelegir y pelear en uno de serie: Llanuras) PASS.
- Solo el de B1: `selector_b1` PASS. Sin mods: `sin_mods` (selector de siempre) PASS.
- Build: `python tools/compilar.py` OK. Conversor: `comprobar` OK (13/13).

## 7. Pendiente
- B2 g1/g8 (cierre al dibujar el modelo) y B1 g9/g11 (colisión); probar el resto de grupos en el juego.
- Sonido y luz: usan los del donante (id_se 45, tema 16). Presentación de B1 con cámaras del donante (encuadres
  raros a veces). En Páramo, unas columnas del fondo salen blancas (transparencia/niebla de B1 sin convertir).
- Vista previa automática mejorable en algunos escenarios de B1 (se puede pasar `--foto`).
- R&R BASE (anfitrión) tras un alta: el código lo devuelve siempre; no se pudo elegir en el juego porque está
  bloqueado en la partida de prueba (sí se probó Llanuras tras un alta).
- Extra E2 (oscurecer poco a poco un escenario): hecho en la ronda 2 (ficha 10, `clima.cpp`).

## 8. Propuestas para otros frentes
- **campaigns.cpp** (`sub_82096FF0`): su control lee `REX_LOAD_U32(res)` aunque `res` sea basura (no mapeada)
  → cierre en vez de "sin efecto". Propuesta:
  ```diff
  -    const bool ok = entry && (!res || (res >= 0x10000 && REX_LOAD_U32(res) == kFxVtable &&
  +    const bool mapeado = res >= 0x40000000u && res < 0x80000000u;   // montones del juego
  +    const bool ok = entry && (!res || (mapeado && REX_LOAD_U32(res) == kFxVtable &&
  ```
- **HUB / modos / retos**: nombres de escenario con `#include "escenarios.h"` y
  `dbz3::escenarios::Nombre(stage, dbz3::settings::Language())` en vez de "escenario N"; `Total()` para admitir altas.
- **Mod Kit**: tarjeta "Escenarios de Budokai 1/2" que llame a `tools/escenarios_convertir.py mod ...` (lista los
  grupos con `lista`, el jugador pone el nombre).
- **Pruebas**: en esta máquina el arranque a veces tarda 40 s en "SDL input driver initialized" (menú a los
  55 s en vez de 18 s); no es del juego. A veces `pantallas` lee 0 con el título en pantalla: repetir.
