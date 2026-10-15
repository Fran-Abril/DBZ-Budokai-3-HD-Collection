# Frente: Combate (ronda 2, 2026-10-15) — fichas 05, 06, 11, 12 (datos), 15, 16, 18

Código: `src/combate.cpp` (nuevo, solo US; en `CMakeLists.txt` y en el `REMOVE_ITEM` de la build solo EU), filas
nuevas en `src/ingame/quick_settings.cpp` (sección **COMBATE (OPCIONAL)** de F1) y `studies/tools/frame_data.py`.

## Qué ve el jugador

Antes: el combate de siempre, sin opciones. Ahora: en F1 (ajustes rápidos) hay una sección nueva **COMBATE
(OPCIONAL)** con seis filas, todas apagadas de serie, con su explicación abajo:

| Fila | Qué hace | Online |
|---|---|---|
| Aviso de golpe | aro y botón A del juego bajo tu barra cuando un golpe del rival va a llegar (momento del contraataque) | sí (solo se ve) |
| Reglas competitivas v0 | AVISO: patada de Goku 50 → 70 y adelante + puñetazo 35 → 50 | no |
| Definitiva directa | la definitiva sin modo hiper (misma entrada, LT de serie), pagando una vez su ki | no |
| Botón de cancelación | LB o RB = adelante + ki de una vez (deja de hacer su atajo) | no |
| Perder la transformación | un golpe de 50/100/300 o más te devuelve a la forma normal | no |
| Sonda del laboratorio | guarda cada cuadro y golpe en `logs/combate_*.csv` (frame data) | sí (solo mide) |

Con alguna regla encendida, al empezar cada pelea (y al cambiarla) sale 4 s abajo una línea con la letra del
juego («REGLAS V0», «DEFINITIVA DIRECTA», «CANCELACIÓN RB», «PIERDE FORMA») para que nadie lo confunda con el
original. En el online y en la versión EU las reglas no actúan aunque estén encendidas (la simulación tiene que ser
la misma en los dos PCs; las direcciones son de la US). El Tag Team no se toca (la reversión no actúa durante un
relevo).

## Lo medido del motor (RE, US) — base de todo lo demás

`X` = luchador (`cfg + 2096 + 8240·k`, `cfg = *0x823D6CF4`). [C] = leído en el código recompilado, [M] = medido
con la sonda en el juego (2026-10-15).

| Qué | Dónde | Fuente |
|---|---|---|
| Paso de pelea de los dos luchadores (entradas, golpes, acciones) | `sub_821100B0` (una vez por cuadro) | [C][M] |
| Entradas del humano / IA del ordenador | `sub_820F7BB0(I)` si `I+68` (mando) ≥ 0, si no `sub_820F8470(I)`; `I = X+4960` | [C] |
| Golpe recibido | `sub_8210ED48(X)`: atacante `X+6132`, bloque HR `X+6136`, pendiente `X+6140`, props `X+6144` | [C][M] |
| Bloque HR del atacante | `*(*(atacante+2756)+28) + 128·HR` (el `#CSK` cargado; `+2756` = puntero al `#CSK`) | [C][M] |
| Código de ataque en curso | `X+2928` (u32) | [M] (7/7 golpes) |
| Cuadro de impacto del ataque en curso (1.ª línea AP1) | `X+2922` (u16; 0xFFFF sin golpe) | [M] |
| Cuadro de la animación | `X+3160` (f32; 1 al empezar) | [M] |
| Vida / ki | `X+6080` / `X+6100` (f32; Goku 3000 de vida, 3000 de ki al empezar, tope 7000) | [M] |
| Modo hiper | `X+8208` ≠ 0 (pone el bit 0x8000 del estado `X+3250` que piden las definitivas) | [C][M] |
| Estado frente a una entrada del `#CCM` | `sub_820F7758(nodo, X)`: cond2 (`w6`) contra `X+3250` | [C] |
| Contraataque (teletransporte) | `sub_82117BF8` (estado `X+8184 == 2` y ki) → humano `sub_82102DF8`, ordenador `sub_820F8F78` → acción 39 | [C][M] |
| Ventana del contraataque del humano | `sub_82102DF8`: tiempo de G (`I+80`) entre −5 y 10 (≠ 0) y P, K, E sin pulsar | [C] |
| Tiempos de P, K, G, E | `I+72..I+84` (s32: > 0 cuadros pulsado, < 0 cuadros desde que se soltó) | [C][M] |
| Forma según el ki tras un golpe | `sub_8211A510(X)` → `sub_82119258` (baja hasta la normal; para en fusiones/formas fijas) → `sub_820FB5D0` | [C] |
| Mando (registro de 184 B) | `0x8238B780 + 184·r` (`r = *(0x8238B670 + 4·mando)`): `+12` botones, `+14` recién pulsados, `+72+4j` tiempo del botón `j = bit−8`, `+108/+110` direcciones | [C][M] |
| Botón → P/K/G/E | `cfg + (mando+4382)·8 + j` (bits 1 P, 2 K, 4 G, 8 E) | [C] |
| Botones del mando de serie (360) | X = P, Y = K, A = G, B = E, LB = P+G, RB = P+K+G (transformarse), LT = P+K+G+E (hiper) | [M] |
| Lado de la pantalla | `I+48` (x propia) y `I+56` (x del rival) en pantalla: adelante = derecha si `I+56 > I+48` | [M] |

## Fichas

Pruebas: `python tools/pruebas.py correr combate_test <escenario>.toml` con los escenarios de
`E:\DBZ3HD_trabajo\combate\esc\` (copias `D:\DBZ3HD\combate_test` y `combate_test2`); cada carpeta de resultado en
`D:\DBZ3HD\_pruebas\` lleva capturas, `dbz3.log` y los CSV de la sonda (`combate_sonda.csv`, `combate_golpes.csv`).
Muñeco de laboratorio (no sale en F1; lo usa el Laboratorio de `modos.cpp`): `dbz3_combate_prueba_muneco =
quieto|guardia` o `dbz3::combate::SetMuneco(0|1|2)` (`src/combate.h`, sin tocar el ajuste guardado). El rival pasa a
ser un **jugador con un mando 2 virtual** que no se mueve y, en guardia, mantiene G: el juego hace el resto como con
una persona. `dbz3_combate_prueba_ki = N`, `DBZ3_COMBATE_LADOS=2` (el teclado lleva el lado 2 y el muñeco es el 1).

### Arreglo del muñeco en guardia (propuesta 6 de rpg)

- Fallo: el mismo golpe unas veces se bloqueaba y otras no (`20261015-114728_rpg_test_lab_guardia` sí,
  `...115516_...` no). Causa medida con la sonda (`runA_p1`, `muneco_guardia_l2` de 13:19): al muñeco se le quitaba
  la IA de ataques (`sub_820F8470`) y se le ponía la acción 1, pero **otra rutina del ordenador lo seguía moviendo**
  (andar 2, pasos 5/6), **la guardia se caía sola a los 4–7 cuadros** (nadie mantenía G) y de pie en reposo el
  contexto `X+6048` es nulo, así que a veces no se podía volver a poner. El resultado dependía del cuadro exacto del
  golpe; no del lado.
- Arreglo: el muñeco es un jugador con el mando 2 virtual (`sub_8208FC40` lo da por conectado; `sub_8208FD58` rellena
  su registro: sin palancas y con el botón que da G mantenido). Guardia nativa (acción 1 → 26 al bloquear), también
  al levantarse; sin IA ni contraataques. Fuera de la pelea el mando 2 vuelve a ser el de verdad.
- Pruebas (todas con el bloqueo comprobado en el log: `muneco (lado N, guardia): golpe X -> guardia`):
  - lado 1 (`muneco_guardia_l1`, teclado en el lado 2): **5 de 5 PASS seguidas, 35 de 35 golpes bloqueados**
    (13:33–14:12);
  - lado 2, la prueba de rpg (`rpg_lab_guardia` = su `lab_guardia.toml` + la comprobación del bloqueo, entrando por
    HUB > Laboratorio): **5 de 5 PASS seguidas, 20 de 20 bloqueados** (13:57–14:23). Hay que apartar antes su
    `hub_modos.toml` (`limpiar_estado.sh` de rpg): si no, la prueba cambia el muñeco guardado a «quieto» en cada
    vuelta y falla una de cada dos (no es el muñeco);
  - `runA_p1` (14:34): quieto → golpe siempre (50 / 35), guardia → bloqueo siempre (25 / 17), cambio en mitad de la
    pelea y reglas v0 (70 / 50): la tabla de la ficha 12 sale igual.
  - Batería `pruebas.py todo`: 5/5 PASS (14:37).

### 05 Ruleset competitivo v0 — 1

- Interruptor **Reglas competitivas v0** (F1 > COMBATE, apagado; el texto de ayuda avisa de qué cambia y que no va
  online). Reglas estáticas en `kReglasV0`: Goku (ID 0), patada `0x210` (bloque HR 16) 50 → 70 y adelante +
  puñetazo `0x21B` (HR 30) 35 → 50. Solo se tocan bloques HR de un único golpe (`frame_data.py 0 --usos 16 30`).
- El bloque HR se cambia **solo durante** `sub_8210ED48` y se deja **byte a byte** como estaba (copia de 128 B y
  `memcmp` después; ningún aviso «no restaurado» en los logs). Con la regla apagada no se toca nada.
- Prueba `runA_p1` (`20261015-093452_combate_test_runA_p1`): clásico patada 50 ×6 y adelante+puñetazo 35; con v0
  patada 71/70 y 50; apagada otra vez 50. Captura `03_reglas_v0` con el rótulo «REGLAS V0» abajo.
- Salida reproducible: `frame_data.py 0 --codigos 210 21b --medido <carpeta>` da la misma tabla cada vez.
- Queda: contador de combo, decaimiento del aturdimiento y ganancia de ki por separado (lo dice la ficha).

### 06 Z-Counter: sonda de ventanas y telegrafía — 1

- **Consumidor de la ventana identificado** (RE): el golpe pasa por `sub_8210ED48` → `sub_82117BF8` (estado
  `X+8184 == 2` y ki suficiente) → para un jugador `sub_82102DF8`: concede el contraataque si el tiempo de G
  (`I+80`) está entre −5 y 10 (≠ 0), P/K/E no están pulsados y la última entrada del búfer es de tipo 1 con
  |valor| ≤ 15; el ordenador decide con `sub_820F8F78`. Si lo concede: acción 39 (teletransporte) y cobro de ki.
- **Sonda** (`dbz3_combate_sonda` o `DBZ3_COMBATE_SONDA=1`): cada comprobación queda en `combate_golpes.csv` como
  `ventana` (tiempo de G, P/K/E, entrada del búfer, código y cuadro del atacante, resultado). Medido en `runE`
  (`20261015-102458_combate_test2_runE`): G a 2, −23 y −12 → no; **G a 3 con entrada 1/−3 → sí**, y el contraataque
  sale (código `0xFC`, 150 de daño al atacante).
- **Telegrafía sin cambiar nada** (**Aviso de golpe**, F1 > COMBATE, apagado): el juego guarda en el atacante el
  cuadro de impacto de su golpe (`X+2922`) y el cuadro de su animación (`X+3160`); cuando faltan 10 cuadros o menos
  (lo que permite la ventana), bajo la barra de vida del que lo va a recibir (si es un jugador) sale el **botón A
  del propio juego** (la guardia) con un aro amarillo que se cierra hasta el golpe. Pruebas `runD_aviso_cancel` y
  `runE`: log `06 aviso al lado N: golpe 210 en 10 cuadros` (12 avisos) y capturas `04_aviso`, `05_aviso`, `12_aviso`,
  `13_aviso` de `runE` con el aro (para la foto, `DBZ3_COMBATE_AVISO_PRUEBA=1500` lo deja 1,5 s en pantalla).
- Queda (siguiente paso de la ficha): la variante asistida (ventana más ancha) y el esquive por dirección; online
  tendría que ir con la misma regla en los dos PCs.

### 11 Referencia Budokai 4 (lector de frame data) — 1

- `studies/tools/frame_data.py <id>`: lee del `data_cmn.afs` el `#CSK` (bin anm) y el `#CCM` (bin cam) y saca, por
  código, la **entrada** (P/K/G/E, cadenas «P, P, K»), los **cuadros activos** (líneas AP1), el **daño por
  situación** (normal, contra, malabar, espalda, suelo, guardia, aturdido), aturdimiento, empuje y marcas
  (A cargable, ! inesquivable, [0x04] = línea sin daño medido: no es armadura) — lo mismo que la wiki de Budokai 4 / Budokai Reader, para B3 HD.
  `--usos` dice qué códigos comparten un bloque HR; `--medido` junta lo medido en el juego; `--prueba` sin juego.
- Comprobado en el juego: el cuadro de impacto leído coincide con el medido (P 8, patada 11, adelante+puñetazo 9) y
  el daño también (30 / 50 / 35). Goku completo: `studies/_data/frame_data/0_0.json` (211 códigos con golpe).

### 12 Laboratorio avanzado (datos) — 1

Dos ataques de Goku medidos en golpe y en guardia, desde los dos lados (`runA_p1` lado 1, `runB_p2` lado 2), sin
superposición. Cuadros a 60 por segundo; «ventaja» = cuándo vuelve el defensor a una acción neutra (0/1/2) menos
cuándo vuelve el atacante (positiva = favorece al atacante). «Guardia» = el defensor en la acción 1 al contacto
(el juego le quita la mitad).

| Ataque | Lado | Resultado | Arranque | Daño | Ventaja |
|---|---|---|---|---|---|
| Patada `0x210` (Y) | 1 y 2 | golpe | 11 | 50 | +2 |
| Patada `0x210` | 1 y 2 | guardia | 11 | 25 | +5 |
| Adelante + puñetazo `0x21B` | 1 y 2 | golpe | 9 | 35 | −10 |
| Adelante + puñetazo `0x21B` | 1 y 2 | guardia | 9 | 17 | −11 |

Los dos lados dan lo mismo cuadro a cuadro. Medido con la sonda (no a mano). Falta (siguiente paso de la ficha):
superposición de una esfera y respuesta del muñeco.

### 15 Técnica directa sin Hyper — 1

- **Definitiva directa** (F1 > COMBATE, apagado). Las definitivas piden el modo hiper en su condición de estado
  (`cond2 = 0x8001`, bit 0x8000 = `X+8208`). Con la regla, fuera del hiper `sub_820F7758` las deja pasar como si
  lo tuviera; la entrada del modo hiper (la misma P+K+G+E, antes en la lista) cede solo si la cápsula de la
  definitiva está equipada y hay ki para su coste (`#SKC +15 × 100`). El cobro lo hace el juego (cápsula; fuera
  del hiper se paga de verdad): **una vez**, al confirmarse la definitiva. Si no hay ki, P+K+G+E es el modo hiper
  de siempre. Coste real medido (la ficha pedía medirlo): **5000** de ki para la Genki Dama de Goku (cápsula 10);
  las «3 barras» de la ficha eran sintéticas.
- Pruebas `runC2_definitiva` (`20261015-102456_...`) y `runC3_definitiva` (`20261015-103112_...`), Goku contra el
  muñeco con las cápsulas 1, 6, 7, 8, 10 (`DBZ3_TEST_CAPS`):
  - ki 5000 al pulsar LT (P+K+G+E) → log `definitiva directa` → código `0x259` y la escena de la Genki Dama
    (`0x814`, `0x4A0`–`0x4A3`), 600 de daño, ki 5000 → 0 (un solo cobro), vuelve a estar de pie; capturas
    `02/03/04_definitiva` (sin modo hiper: `X+8208 = 0` todo el rato).
  - ki 4999 (una unidad menos) → `modo hiper (como siempre)` (`0x258`).
  - LT pulsado dos veces seguidas durante el arranque → una sola definitiva y un solo cobro.
  - Regla apagada con ki 5000 → modo hiper (`0x358`): el clásico no cambia.
- Queda sin probar: interrumpirla con un golpe, KO/tiempo durante el arranque, en el aire (`w13`, `0x359`), forma no
  válida y revancha.

- **Botón de cancelación** (F1 > COMBATE: No / LB / RB; apagado de serie). Al pulsar ese botón, el registro del
  mando de ese cuadro trae **adelante + el botón que da E** en la configuración del jugador (la entrada que el juego
  ya acepta: el «→+E» de las técnicas y de la cancelación al final de los combos). Adelante sale de la posición en
  pantalla (`I+56 > I+48` = derecha). Una pulsación = una solicitud: mantenerlo no repite; soltarlo rearma. Mientras
  esté elegido, ese botón no hace su atajo de serie (LB = P+G, RB = P+K+G, transformarse); apagarlo lo devuelve.
  No alarga ninguna ventana ni añade cancelaciones que el juego no tenga. No se usa online.
- Prueba `runD_aviso_cancel` (`20261015-095913_combate_test2_runD_aviso_cancel`): RB sin la opción = su atajo
  (P+K+G); con la opción, log `16 boton de cancelacion: derecha + E` y el Kamehameha (`0x34A`, −1000 de ki, cápsula
  6) — **lo mismo** que adelante + B a mano (`0x34A`, −1000); RB mantenido 1,5 s = una sola vez; tras un combo
  P, P, P mete la misma entrada y el juego decide (en el aire no hay cancelación: no sale nada nuevo); con la opción
  en «No», RB vuelve a transformar (`0x2E1`). Captura `12_rb` con el rótulo «CANCELACIÓN RB».

### 18 Reversión por daño — 1

- **Perder la transformación** (F1 > COMBATE: No / 50+ / 100+ / 300+; apagado de serie). Tras un golpe con daño
  **aplicado** ≥ umbral, si el luchador está transformado y no cae KO, se llama a la rutina del juego que ajusta la
  forma al ki (`sub_8211A510`) haciéndole ver ki 0 solo durante la llamada: baja a la forma normal con su cambio de
  modelo (las fusiones y formas fijas no bajan, lo decide el juego). Una vez por golpe; nada queda pendiente para el
  combate siguiente; no actúa durante un relevo del Tag Team ni online.
- Prueba `runE` (`20261015-102458_combate_test2_runE`), Goku Kaioken (forma 1, muñeco del lado 1) y el teclado en
  el lado 2: con umbral **51**, dos patadas de **50** → sigue en Kaioken (justo por debajo); con umbral **50**, una
  patada de 50 → log `18 golpe de 50 (umbral 50): forma 1 -> 1 (siguiente 0)` y la forma 0 15 cuadros después
  (umbral exacto); dos golpes más → no vuelve a revertir. `runB_p2` igual (capturas `02_umbral51_sigue_ssj`, rojo,
  y `03_umbral50_vuelve`, normal).
- Proyectiles y el golpe de la escena de una definitiva (no pasan por `sub_8210ED48`): el paso de pelea ve la
  bajada de vida que el gancho no apuntó y aplica la misma regla. KO, armadura, golpe múltiple, ki bajo y relevo:
  sección WL-18.

## WL — casos de aceptación (`studies/11_vision_futura/INTEGRACION_SEGURA.md`)

Carpetas en `D:\DBZ3HD\_pruebas\20261015-<hora>_combate_test*_<escenario>` (escenarios en
`E:\DBZ3HD_trabajo\combate\esc\`, generados por `hacer_wl.py`). Medidas con `frame_data.py --medido <carpeta>`.

### WL-12 — laboratorio (dos ataques: golpe, guardia, al aire)

| Caso | Prueba | Resultado |
|---|---|---|
| Golpe y guardia, los dos lados, «listo» en el cuadro real | `runA_p1` 143359, `runB_p2`, `muneco_guardia_l1/l2` | **PASS**: patada 11 / 50 / +2 (guardia 25 / +5); adelante+puñetazo 9 / 35 / −10 (guardia 17 / −11) |
| Golpe al aire (whiff) | `wl_12b` 165139 | **PASS**: patada `210` lejos → «al aire», 55 cuadros hasta estar libre; sin contacto no hay ventaja (no se inventa) |
| Contacto tardío | `wl_a` 152653, `wl_12b`, `wl_12c` 170457 | **PASS**: el ki blast (`253`, no tiene AP1: es un proyectil) toca a los 17 / 19 / 20 cuadros según la distancia y la ventaja sube (+11 / +13 / +14); los golpes cuerpo a cuerpo tocan en su primer cuadro activo (`tardío` = no). Que el proyectil llegue cuando el atacante ya está libre no se alcanza en este escenario con Goku (su animación dura ~32 cuadros y el vuelo más largo medido son 20) |
| Hitstop | `runA_p1` 143359, `wl_12d` 173552 | **PASS**: no hay parada al tocar (0 cuadros en patada y en adelante+puñetazo). Los 2 cuadros repetidos de adelante+puñetazo (13, 13, 14, 14) salen igual golpeando que al aire (`wl_12d`): son de su animación. La herramienta mide la parada solo en los 3 cuadros tras el contacto |
| Geometría ausente no se inventa | `frame_data.py` | **PASS**: no se dibujan esferas ni cajas; el radio y el hueso de cada línea AP1 salen como datos del fichero. Además, las líneas con la marca 0x04 (que la documentación vieja llamaba «armadura») **no hacen daño medido**: P+K (`23E`) solo golpea con su línea de los cuadros 18–22 (`wl_b2`, `wl_e2`, `wl_e3`: contacto en el cuadro 18); la herramienta ahora las marca `[0x04]` |

### WL-15 — definitiva directa

| Caso | Prueba | Resultado |
|---|---|---|
| Inicio confirmado: un solo cobro | `runC2/C3`, `wl15_a` 152656 | **PASS**: `cobro de 4996 de ki ... 4996 -> 0` una vez por definitiva |
| El log de `runC2` («ki 5001 -> 5000», coste 5000) | `wl15_*` | **Aclarado**: esa línea era el ki al **empezar** (el ki baja ~1 por cuadro hacia su nivel base). El juego cobra después, una sola vez, a los ~5 cuadros (al confirmarse): pide 5000 y quita todo lo que hay (4996 → 0; nunca negativo). Ahora el log separa «empieza», «cobro» y «fin» |
| Interrupción en el arranque | `wl15_e` 161616 | **PASS**: el muñeco golpea antes del cobro → `fin de la definitiva: accion 25 ... cobrado 0` (no se cobra) |
| KO durante el arranque | `wl15_b/f/g/h/i` | **PASS por equivalencia**: no se puede provocar en el arranque: la carrera de la definitiva golpea antes que cualquier patada (5 intentos; vida a 0 sin golpe no tumba). Un KO justo antes deja sin definitiva (`wl15_f/g`: KO, nada empieza) y la regla solo actúa en plena pelea (estado 4). El final por KO va por el mismo camino que el fin de tiempo (estado 5, abajo) |
| Fin del tiempo en el arranque | `wl15_c4` 165531 (combate de 10 s) | **PASS**: la definitiva empieza en el cuadro 713, se cobra en el 718 y el tiempo acaba en el 721 → `fin ... accion 15, estado 5, cobrado 4996` y `combate acabado ... motivo 3`: termina limpio |
| En el aire | `wl15_a` | **PASS**: sale la versión aérea (`0x359`) y se cobra igual una vez (como en el modo hiper, donde el ki lo paga el propio modo); si no toca, se pierde |
| Revancha («¿Luchar otra vez?») | `wl15_d2` 164235 | **PASS**: definitiva que tumba al muñeco (100 de vida) → menú → otra pelea → otra definitiva, otro cobro único y otro KO |
| Regla apagada / una unidad menos | `runC2/C3` | **PASS**: modo hiper de siempre |

### WL-18 — reversión por daño

| Caso | Prueba | Resultado |
|---|---|---|
| Umbral exacto y justo por debajo | `runE` 102458 | **PASS**: 51 → dos patadas de 50 no revierten; 50 → revierte |
| El KO manda | `wl_c` 154242 | **PASS**: `18 golpe de 40 ... no revierte: KO (manda el KO)` |
| Golpe múltiple | `wl_a` 152653 | **PASS**: umbral 60 y la cadena K,K,K (47, 45, 34; total 126) → no revierte: cuenta cada golpe |
| Ki bajo a la vez (una sola reversión) | `wl_b` 154120 (ki 900), `wl_b2` 162043 (ki 0, P+K de 70) | **PASS**: una reversión (`forma 1 -> 0` una vez; el juego no la quita en ese golpe, la quitaría al levantarse); los golpes siguientes: `no revierte: ya en la forma normal` |
| Proyectiles (ki blast) | `wl_a2` 161613 | **ARREGLADO + PASS**: el ki blast no pasa por `sub_8210ED48` (antes no contaba); ahora el paso de pelea ve la bajada de vida que el gancho no apuntó → `18 dano de 47 sin golpe directo (proyectil o escena)` y vuelve a la forma normal. Vale también para el golpe de la escena de una definitiva |
| Armadura | `wl_d/e/e2/e3` | **PASS (no existe en el juego base)**: en todas las pruebas (miles de golpes) nadie siguió atacando tras recibir un golpe con daño; la marca 0x04 no es armadura (el muñeco con P+K en curso se interrumpe: acción 15 → 25). Si un mod la tuviera, la regla no revierte (`no revierte: armadura`, acción 15 antes y después) |
| Relevo de Tag Team sin forma residual | `tag18` 175410 (`E:\DBZ3HD_trabajo\combate	ag18.py`, TAG TEAM real con la instantánea `freezer_cap2`) | **PASS con límite**: con la regla a 10 hay golpes y relevos (`relevo 0 -> 1 ... sale en forma 0, entra en forma 0`) sin errores, y durante un relevo la regla no actúa. Pero en el Tag Team ningún miembro puede estar transformado: el Tag Team quita P+K+G (el relevo es SELECT) y la forma de salida (`DBZ3_START_FORM=1,0`) se rechazó por falta de la cápsula de transformación (`forma: pedida 1 aplicada 0 motivo 7`). Así que no hay forma que perder en un relevo. Si la hubiera, no quedaría nada pendiente: la regla no guarda nada (revierte en el momento) y el Tag Team lee la forma del que sale de `X+2980` al relevar |

Código cambiado en esta vuelta (`src/combate.cpp`): la regla 18 también para proyectiles y escenas (bajada de vida sin golpe directo), motivos en el log (KO, armadura, ya normal, relevo), la definitiva directa solo en plena pelea y con su seguimiento (empieza / cobro / fin, también la aérea), y para pruebas: muñeco «ataca» / «pk», `dbz3_combate_prueba_vida`, `_ki_muneco`, `_forma`, `DBZ3_COMBATE_CAPS` y el log `pelea N, estado S` con la sonda. `frame_data.py --medido`: golpes al aire, proyectiles (bajadas de vida sin golpe), parada (3 cuadros tras el contacto) y contacto tardío. Compilado y batería `pruebas.py todo` **5/5 PASS** (2026-10-15 ~18:10). Las pruebas que fallaron entre las 17:15 y las 17:36 (faltaban los datos del juego) se repitieron.

## Puntuación

| Ficha | Nota | Prueba |
|---|---|---|
| 05 Ruleset competitivo v0 | 1 | `runA_p1` |
| 06 Z-Counter (sonda + telegrafía) | 1 | `runD_aviso_cancel`, `runE` |
| 11 Referencia B4 (lector de frame data) | 1 | `frame_data.py` + `runA_p1` |
| 12 Laboratorio avanzado (datos) | 1 | `runA_p1`, `runB_p2` |
| 15 Técnica directa sin Hyper | 1 | `runC2_definitiva`, `runC3_definitiva` |
| 16 Cancel accesible | 1 | `runD_aviso_cancel` |
| 18 Reversión por daño | 1 | `runE`, `runB_p2` |

Batería `python tools/pruebas.py todo --max 2` con el exe que lleva todo esto: **5/5 PASS** (tag_swap, campana,
du_continuar, batalla, menu; 2026-10-15 10:45–11:00).

## Cómo repetir las pruebas

```
python tools/compilar.py
python tools/pruebas.py prep combate_test
python tools/pruebas.py correr combate_test E:\DBZ3HD_trabajo\combate\esc\runA_p1.toml     (05, 11, 12 lado 1)
python tools/pruebas.py correr combate_test E:\DBZ3HD_trabajo\combate\esc\runB_p2.toml     (12 lado 2, 18)
python tools/pruebas.py correr combate_test E:\DBZ3HD_trabajo\combate\esc\runC3_definitiva.toml   (15)
python tools/pruebas.py correr combate_test E:\DBZ3HD_trabajo\combate\esc\runD_aviso_cancel.toml  (06, 16)
python tools/pruebas.py correr combate_test E:\DBZ3HD_trabajo\combate\esc\runE.toml        (18, 06 ventana)
python studies/tools/frame_data.py 0 --codigos 210 21b --medido D:\DBZ3HD\combate_test\logs
```
Los CSV de la sonda quedan en `D:\DBZ3HD\<copia>\logs\` (copiados a cada carpeta de resultado). Ojo: no lanzar dos
pruebas en la misma copia (dos juegos leerían el mismo `dbz3_input.req`; pasó una vez y se repitió).

## Ganchos de este frente (`src/combate.cpp`, solo US)

`sub_821100B0` (paso de pelea), `sub_8210ED48` (golpe recibido), `sub_82102DF8` (ventana del contraataque, solo
mide), `sub_820F7758` (estado de una entrada del `#CCM`), `sub_8208FD58` (mando de cada usuario; el del muñeco es virtual), `sub_8208FC40`
(mando conectado: el del muñeco siempre). Ninguno estaba enganchado en `src/` (comprobado con Grep).

## Propuestas para otros frentes

- **online** (`src/net/*`): para usar estas reglas en el online, mandar en la sala las 4 opciones que cambian el
  combate (`dbz3_combate_reglas_v0`, `_definitiva_directa`, `_boton_cancel`, `_reversion`) y activarlas solo si son
  iguales en los dos PCs. Hoy `Rules()` las apaga en cuanto hay sesión (`Session::active()`).
- **rpg/modos** (`src/modos.cpp`, pantalla del Laboratorio): enseñar la tabla de `frame_data.py --medido` (arranque,
  daño y ventaja de los últimos golpes) y un interruptor del muñeco (`dbz3_combate_prueba_muneco`) para practicar.
- **coordinador**: `CMakeLists.txt` lleva `src/combate.cpp` en `DBZ3_SOURCES` y en el `REMOVE_ITEM` de la build solo EU.
