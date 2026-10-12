# Estudio 08: metodología de equipos de Budokai Tenkaichi 3 y referencia de Dragon Ball FighterZ

Solo lectura. No se ha tocado ningún fichero del proyecto. No se ha ejecutado ningún código descargado.

## Fuentes y alcance

- Repos clonados con `--depth 1` en `SP\bt3\BT3-Decompiled` y `SP\bt3\Tenkaichi3Decomp`.
- **Los dos repos son el mismo juego** (BT3, PS2). `Tenkaichi3Decomp` es el port PC que usa `BT3-Decompiled` como base. El código de equipos es casi idéntico (mismas funciones, mismas líneas en `btl_char_member.c` y `btl_char_mgr.c`; cambia la organización de los ficheros de acciones). Por eso las referencias van a BT3; las equivalentes en TK3 están al final.
- ROM: el directorio `E:\Emuladores\Retrobat\roms\ps2\tenkaichi 3` contiene un único fichero, `DragonBall Z - Budokai Tenkaichi 3 (USA) (En,Ja).iso` (2880.9 MB). No se ha extraído nada.

Leyenda: **[C]** comprobado en el código (archivo:línea). **[W]** comprobado en una web abierta. **[I]** inferido. **[N]** no verificado (solo aparece en un resumen de búsqueda o no se pudo abrir la fuente).

## Resumen

- BT3 tiene un luchador activo por lado. El equipo es una lista de hasta 5 miembros dentro del mismo bloque de luchador.
- El estado de cada miembro (vida, ki, blast, forma) se guarda en su propio registro. Cambiar de miembro solo cambia un índice: el que sale conserva su vida y el que entra recupera la suya.
- La precarga es parcial: al empezar, cada miembro carga un fichero de datos ligero (4 KB). El modelo, la animación y la voz se cargan solo del que entra, uno por lado.
- Cambio voluntario: gauge de cambio que se llena con el tiempo y se gasta al cambiar, más L3+R3. La salida dura unos 15 frames, la espera a la carga tiene tope de 46 frames y la entrada son unos 9 frames.
- Relevo por KO: si quedan vivos, tras 30 frames el siguiente entra volando (2500 km/h). Si no quedan, no hay relevo.
- BT3 no tiene ataques de asistencia. Lo más cercano es la fusión con un compañero, que no es una asistencia.
- DBFZ: 3 luchadores, cambio con botón (con o sin hacia delante), asistencia con botón, asistencias que se refrescan al caer un luchador. No encontré cifras de frames ni la invulnerabilidad al entrar en fuentes accesibles.
- Recomendación: copiar la forma del estado por miembro, la ventana de carga y la secuencia de salida/entrada de BT3. No copiar el código (es PS2 y otra arquitectura). Evitar la pausa de carga de BT3, que hace el combate no determinista.

## 1. Estado por miembro (BT3)

- Bloque del luchador: 0x1600 bytes. Índice activo en `+0x994`, número de miembros en `+0x998`, gauge de cambio en `+0x99C`, candidato a cambio en `+0x9A0`, slots en `+0x9A4` (5 slots de 0xA4 bytes). [C] `include/battle/btl_char_member.h:12, 97-107`
- Cada miembro (0xA4 bytes): personaje, traje, presente, niveles de bonus, 128 bits de habilidad, nivel de CPU, tipo de IA, y un gauge de 0x38 bytes en `+0x40`. El gauge tiene: vida (10000 = una barra), vida máxima, ki (0..100000), blast (100000 por stock), maxPower (0..30000), variante del modelo y banderas. [C] `include/battle/btl_char_member.h:40-68`
- Cambiar de miembro solo escribe el índice (`BtlMember_SetActiveIndex`). Los gauges de los demás se quedan como estaban. [C] `src/battle/btl_char_member.c:273`
- Al entrar un miembro: `switchGauge = 0`. Ki y maxPower se llenan solo si el miembro tiene la habilidad 0x43. [C] `src/battle/btl_char_mgr.c:404-420`
- La regeneración por habilidad (`BtlMember_AddHealth`) actúa solo sobre el miembro activo. Los de banquillo no regeneran. [C] `src/battle/btl_char_member.c:515-516` (comentario "Gives health to the active member")
- Al KO: ki, blast y maxPower a 0, y el aturdimiento a 0. [C] `src/battle/btl_char_member.c:~500-505`
- Carga en memoria, al empezar el combate: por cada lado y cada miembro, dos buffers de 4 KB (`BTL_MEMBER_BUF_SIZE = 0x1000`) y se pide el fichero de datos del miembro. [C] `src/battle/battle_load.c:520-537`, `include/battle/battle_work.h:129`
- Carga en memoria, al cambiar: el trabajo de cambio pide el modelo, dos ficheros de animación y el banco de voz del que entra. Hay un solo modelo residente por lado. [C] `src/battle/battle_load.c:655-684` (creación del trabajo), `:366-400` (carga)
- Cola de peticiones: anillo de 7 peticiones; el cargador sirve una a la vez. [C] `src/battle/btl_change.c:7-21`

**Cuántos personajes en memoria a la vez:** un modelo completo por lado, más datos ligeros (4 KB) de hasta 5 miembros por lado. [C/I]

## 2. Cambio voluntario (BT3)

Entrada:
- **Gauge de cambio** (`switchGauge`, 0..100000): sube cada frame si el lado puede cambiar, en cantidad `100000 / (tiempo_de_llenado * 30)`. En modos 5 y 6 suma un extra fijo por frame. Tope 100000. [C] `src/battle/btl_char_action.c:913-920`. El tiempo de llenado es `BtlParam_GetGauge99CTime` (en segundos, por el factor 30) [I].
- **Botón L3+R3** (entrada 0x6B): exige gauge lleno, no tener la bandera 0xAF, no tener la bandera 0x10F y al menos 2 miembros vivos. Después elige el candidato y pide la acción 0xF3. [C] `src/battle/btl_act_decide.c:448-476` (comprobación), `:477-481` (cola), `src/battle/btl_char_action.c:441-453` (candidato y datos)
- **Stick derecho** (bits 0x2000000 y 0x4000000): solo cambia el candidato. Exige 3 o más vivos, modo distinto de 1 y no tener la bandera 0x134. No cambia por sí solo. [C] `src/battle/btl_char_action.c:1084-1094`
- **Candidato**: el siguiente miembro vivo distinto del activo. [C] `src/battle/btl_char_member.c:315-325`

Secuencia (la lógica va a 30 cuadros por segundo; ver README de TK3):
1. **Salida, acción 0xF3** [C] `src/battle/btl_act_change.c:1359` (comentario) y función `BtlAct_SwitchLeave`:
   - Animación de caída 0xEE. Luego la 0x2F: sube a 1000 km/h durante 14 frames. Efecto y sonido en el frame 15.
   - Oculto desde el frame 16 (bandera 0xB).
   - Espera hasta el frame 46 **y** a que el modelo nuevo esté cargado (`BtlChange_IsLoadedFor`). Ahí pide la acción 0xF4.
   - Corte de cámara al que sale: `ChrCam_RequestCut(chr, 1, 2)`.
2. **Entrada, acción 0xF4** [C] `btl_act_change.c:1595`: un frame oculto, marca el trabajo como hecho.
3. **Aterrizaje, acción 0xF5** [C] `btl_act_change.c:~1615`: animación 0x2D mantenida 8 frames. Corte de cámara tipo 3. Después devuelve el control.
4. **Invulnerabilidad y bloqueos**:
   - `BtlActChange_SetFlags` activa en cada frame del cambio las banderas 0x42..0x46, 0x125 (congela al rival), 0x12A, 0xBB, 0x3F, 0x128, 0x1B, 0x1A, 0x136. [C] `btl_act_change.c:633-647`
   - Que 0x42..0x46 sean invulnerabilidad: [I] por el comentario de otra acción que las usa así (`btl_act_2.c:3745-3746`).
   - La bandera 0x134 se activa durante el cambio y bloquea nuevos cambios. [C] `btl_char_action.c:1084`
   - Que el luchador ignore la entrada durante el cambio: [I], no seguido en detalle.
5. **Pausa de carga**: mientras hay una petición activa, el combate queda congelado (`timeStop`). [C] `btl_change.c:57-77`. La nota de errores dice que el combate se congela el tiempo que tarde la carga, y que depende del disco. [C] `BT3-Decompiled/docs/known_bugs.md:60`

**Ventana de carga:** 46 frames, unos 1.5 s a 30 cuadros por segundo [I]. Si el modelo tarda más, BT3 congela el combate.

## 3. Relevo forzado por KO (BT3)

- **Disparador**: el activo llega a vida 0. [C] `btl_char_member.c:~500-509`
- **Condición de espera** (`BtlActB_TickMemberChange`): solo si el luchador está muerto, tiene la bandera 3, no tiene 0xAF y quedan miembros vivos. Espera 30 frames (`++frames > 30`) y entonces pide la acción 0xF6. [C] `src/battle/btl_act_1.c:2588-2607`. Desde la caída en el aire o en un foso: `btl_act_1.c:3446` y `:3550`. Desde la acción 0xEB: `src/battle/btl_act_2.c:456-470`.
- **0xF6, caída en el suelo** [C] `btl_act_change.c:1674`: anima la caída (0xD6, o 0xD8 con la bandera 0x11), para la velocidad, pide la carga del entrante. Cuando el modelo está listo, pasa a 0xF7.
- **0xF7, espera de entrada** [C] `btl_act_change.c:1710`: un frame oculto, guarda el punto de entrada, marca el trabajo como hecho.
- **0xF8, vuelo de entrada** [C] `btl_act_change.c:1741`: vuela con animación 0x19 a 2500 km/h hacia el punto de entrada, aterriza con 0x2D, sacude la cámara (3.0 durante 0.7 s) y vibra el mando (1.0 durante 0.5 s). Después pide reiniciar la ronda (bandera 0xF9). `BtlChars_CheckRoundReset` está en `btl_char_mgr.c:945-946`.
- **Cambio del índice**: en el mismo bloque 0xF3..0xF8. [C] `btl_char_mgr.c:404-420`
- **Sin miembros vivos**: no hay relevo; `BtlMember_CountAlive` devuelve 0 y `TickMemberChange` no pide nada. [C] `btl_act_1.c:2601`. Qué pasa exactamente al final del combate: [I], no seguido.
- **Modo 3 (pool)**: solo para el lado 1. Cuando su último miembro vivo cae, entra el siguiente de una lista de hasta 50 rivales. [C] `btl_char_member.c:106-124` (`BtlColl_NextPoolMember`), `:505-508`; `include/battle/battle_setup.h:40-41` (descripción del modo).

**Duración total aproximada del relevo forzado**: 30 frames de espera + caída + carga + 1 frame + vuelo. [I]

## 4. Asistencias y combinados (BT3)

- **No hay ataques de asistencia de compañero en BT3.** [C] búsqueda de `assist|partner|support` en `src/battle` y `include/battle`: los únicos resultados son de fusión.
- **Lo más parecido es la fusión** (acciones 0xF1 y 0xF2): se carga un objeto "partner" (un miembro de la plantilla) y la forma fusionada. Requiere un miembro vivo cuyo ID esté en la lista de fusión. [C] `src/battle/btl_act_decide.c:319-369`, `src/battle/btl_act_change.c:1232-1290`. No es una asistencia: el compañero no ataca por su cuenta.

## 5. Dragon Ball FighterZ (referencia web)

| Dato | Estado | Fuente |
|---|---|---|
| 3 luchadores por equipo. Se puede cambiar en cualquier momento. Un ataque (Dragon Rush) puede forzar el cambio del rival. Hay que derrotar a los 3 para perder. | [W] | https://en.wikipedia.org/wiki/Dragon_Ball_FighterZ |
| Cambio ("Z Change"): mantener el botón de asistencia más L1/LB (compañero del medio) o L2/LT (compañero de anclaje). "Quick Z Change": hacia delante + botón de cambio. El cambio "tarda unos instantes" y se puede castigar. | [W] | https://dashfight.com/news/all-the-ways-to-tag-characters-in-dbfz-3454 |
| Cambio tras un golpe que deja al rival en hitstun o blockstun (combo tag). | [W] | https://dashfight.com/news/all-the-ways-to-tag-characters-in-dbfz-3454 |
| Asistencia: pulsar el botón del compañero. Hacia delante + asistencia = cambio. Si el principal recibe un golpe mientras empieza la asistencia, esta se anula. | [W] | https://www.eventhubs.com/news/2018/mar/06/here-are-five-tips-regarding-assists-dragon-ball-fighterz-could-help-improve-your-gameplan |
| Al caer un luchador, las asistencias recuperan su cooldown. | [W] | https://www.eventhubs.com/news/2018/mar/06/here-are-five-tips-regarding-assists-dragon-ball-fighterz-could-help-improve-your-gameplan |
| La asistencia tiene cooldown. Un cambio o una asistencia deja un breve bloqueo en el que no se puede hacer ninguna de las dos cosas. | [N] (solo resumen de búsqueda; la página de Twinfinite dio 403 al abrirla) | https://twinfinite.net/guides/dragon-ball-fighterz-switch-change-characters-how/ |
| Posición tras KO o cambio: una opción "Starting Side" (parche 1.11) centra al luchador. | [N] (solo resumen de búsqueda) | https://en.bandainamcoent.eu/dragon-ball/news/dragon-ball-fighterz-patch-note-111 |
| Revivir un aliado caído con un deseo de Shenron. | [W] | https://en.wikipedia.org/wiki/Dragon_Ball_FighterZ |
| **Invulnerabilidad al entrar**: no encontrada. La página de cambios no la menciona. | [N] | - |
| **Frames o segundos del cambio y de la entrada tras KO**: no hay cifras en las fuentes accesibles. | [N] | - |

Intentos fallidos: Dustloop (`dustloop.com/w/DBFZ/Controls`) devolvió error al leerla. Twinfinite devolvió 403.

**Diferencia de diseño clave:** BT3 es un relevo con carga de modelo y pausa, sin asistencias. DBFZ es un equipo de 3 con asistencias como mecánica central y cooldowns. Para B3 (modo de 2-3 luchadores con relevo) el modelo de BT3 encaja mejor con lo que ya existe; las asistencias de DBFZ serían una capa aparte, de diseño propio.

## 6. Pasos concretos para B3

Dato de partida (del estudio 02b y del usuario; no lo he re-verificado en el binario): bloque de 8240 bytes por jugador (`X_k = cfg + 2096 + 8240·k`), vida float en `X+6080` (llena en 3000), ID en `X+7420`, y el cambio de modelo en `sub_820F9970(T, f)`.

1. **Medir antes de escribir código.** Con un write-watch sobre `X+6080`, `X+7420` y el resto del bloque de 8240, registrar qué campos cambian durante (a) un cambio de forma con `sub_820F9970` y (b) un combate normal. Resultado: lista de campos "del miembro" frente a campos "de la forma". Esto es lo que BT3 documenta como su tabla de gauges. [I] Se hace por instrumentación, no a ciegas, como pide la nota de memoria del proyecto.
2. **Registro por miembro.** Tres registros como máximo por equipo (BT3 tiene 5 slots, pero para un equipo de 2-3 basta con 3). Cada registro guarda los campos de miembro encontrados en el paso 1: vida, vida máxima, ki, variante, ID, traje y stock. Un índice de activo por lado, como `+0x994` en BT3.
3. **Cambio en la misma casilla.** Pasos: (a) copiar el bloque activo al registro del que sale; (b) copiar el registro del que entra al bloque activo; (c) llamar a `sub_820F9970` para el modelo; (d) restaurar vida y ki del registro si `sub_820F9970` los sobrescribe (comprobar con el write-watch). BT3 hace (c) y (d) en el mismo bloque de cambio de índice (`btl_char_mgr.c:404-420`).
4. **Precarga.** Al empezar el combate, copiar los registros de todo el banquillo a memoria (son pocos bytes; en BT3 son 4 KB por miembro). Cargar modelo, animación y voz solo del que entra. Un modelo por lado.
5. **Gauge de cambio.** Campo nuevo en el bloque (offset libre, a elegir con el paso 1). Sube cada frame hasta llenarse en T segundos y se pone a 0 al cambiar. Condición: gauge lleno, al menos 2 vivos, sin aturdimiento ni KO y sin cambio en curso.
6. **Secuencia voluntaria.** Tiempos de BT3 como punto de partida: salida de 15 frames (elevación), oculto desde el frame 16, espera a que el modelo esté listo con tope de 46 frames, entrada de 1 frame oculto y 8 frames de aterrizaje. **Decisión a tomar:** qué pasa si el modelo tarda más de 46 frames. BT3 congela el combate, lo que no es determinista. Recomiendo no congelar: prolongar la espera con un tope, o precargar el banquillo al empezar (paso 4).
7. **Invulnerabilidad y bloqueo de entrada** durante el cambio. Usar banderas propias de B3, encontradas con RE. Las de BT3 (0x42..0x46, 0x134) no sirven directamente porque son otro formato. Cámara: un corte al salir y otro al entrar.
8. **KO forzado.** Si el activo cae y quedan vivos: 30 frames, animación de caída, carga del siguiente, entrada en vuelo (2500 km/h, animación 0x19 hasta 0x2D), sacudida y vibración, y después reinicio de ronda. Sin vivos: fin del lado, sin relevo.
9. **Asistencias (opcional, diseño propio).** No hay precedente en BT3. Si se quieren: botón de asistencia con cooldown, refresco del cooldown al caer un luchador, y cancelación si el principal recibe un golpe [W, DBFZ].
10. **Validar.** Tras cada paso, compilar y ejecutar los tests del proyecto según `AGENTS.md` §4, y comprobar en juego. Este estudio no ha tocado nada del proyecto.

**Lo que no se copia de BT3:** la pausa de carga (no determinista) y el código en sí (PS2, otra arquitectura). Se copia la forma del estado, la ventana de carga y la secuencia.

## 7. Equivalencias en Tenkaichi3Decomp (port PC)

| Función BT3 | Archivo TK3:línea |
|---|---|
| `BtlAct_SwitchLeave` (0xF3) | `src/battle/btl_act_h_b.c:1242` |
| `BtlAct_SwitchArriveWait` (0xF4) | `src/battle/btl_act_h_b.c:1476` |
| `BtlAct_KoSwitchLoad` (0xF6) | `src/battle/btl_act_h_b.c:1554` |
| `BtlAct_KoSwitchFlyIn` (0xF8) | `src/battle/btl_act_h_b.c:1621` |
| `BtlActB_TickMemberChange` | `src/battle/btl_act_a.c:2589` |
| `BtlAct_CanSwitch` / `BtlAct_CheckSwitchInput` | `src/battle/btl_act_j.c:449` / `:467` |
| `BtlChange_Update` / `BtlChange_RequestChara` | `src/battle/btl_char_ctl_b.c:57` / `:80` |
| `BtlMember_Replace` / `CountAlive` / `NextSwitchTarget` | `src/battle/btl_char_member.c:223` / `:278` / `:315` (mismas líneas que BT3) |
| Cambio de índice activo | `src/battle/btl_char_mgr.c:411` (igual que BT3) |

## Lo no comprobado

- Invulnerabilidad al entrar en DBFZ: no encontrada.
- Frames de DBFZ para el cambio, la entrada y el KO: no encontrados.
- Bloqueo de entrada durante el cambio en BT3: inferido, no seguido.
- Qué pasa al final del combate cuando caen todos: inferido, no seguido.
- Campos de B3 (`X+6080`, `X+7420`, `sub_820F9970`): tomados del estudio 02b y del usuario, no re-verificados.
