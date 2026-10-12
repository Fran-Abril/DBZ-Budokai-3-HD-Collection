# BT3: selección de equipo, barras de relevo en el HUD y animaciones de cambio

Investigación solo de lectura. No se ha ejecutado código ni modificado nada fuera de este archivo.

**Rutas base** (las citas usan rutas relativas a estas raíces):
- BT3 (principal): `scratchpad\bt3\BT3-Decompiled\`
- TK3 (comparación): `scratchpad\bt3\Tenkaichi3Decomp\`

Nota: los nombres de funciones y variables vienen del decompilado, son nombres puestos por el equipo que lo hizo y pueden no ser los originales. Las "líneas" son las del archivo tal como está hoy.

---

## Resumen corto

- **Selección de equipo:** cada lado puede elegir hasta 5 personajes. Se ven 5 "casillas" de equipo y una placa final que se llama `mc_menu_plate` en el código (la que tú llamas END). Cruz elige, Triángulo vuelve atrás o quita. La selección de escenario la hace siempre el mando 1.
- **Barra de vida del relevo:** solo se muestra UN personaje de reserva (el que entraría), no todos. Tiene barra de vida pequeña y un gauge de cambio. Los personajes KO no aparecen.
- **Cambio voluntario:** dura unos 2 segundos. El que sale sube volando a 1000 km/h y desaparece. El que entra aparece y aterriza.
- **Cambio tras KO:** el KO espera unos 31 fotogramas y luego entra el relevo desde el aire a 2500 km/h, aterriza con sacudida de cámara y se queda de pie 30 fotogramas.
- **Repos:** la lógica del cambio y del panel de relevo es idéntica en BT3 y TK3.

---

## 1. Selección de equipo (hasta 5 por lado)

### 1.1 Dónde está el código

- Pantalla de equipo: `src/menu/team_select.c`. Función de entrada por mando: `TeamSel_Input` (1184). Bucle de la pantalla: `TeamSel_Run` (1938). Dibujo: `TeamSel_Draw` (780). Estructuras y constantes: `include/menu/team_select.h` (todo el archivo, incluida la segunda parte).
- Botones (mapa fijo del juego): `include/sys/game_pad.h:24-26`. Cruz = bit `0x200`, Triángulo = bit `0x400`. Direcciones: izquierda 1, derecha 2, abajo 4, arriba 8.
- La pantalla de un jugador contra la CPU, de dos jugadores y CPU contra CPU se elige con `gTsProgress->players` (`team_select.c:1184-1240`, `TEAMSEL_PLAYERS_*` en `include/menu/team_select.h`).

### 1.2 La tira de equipo (slots)

- Cada lado tiene 5 casillas de equipo, con índices internos 0 a 4. En la animación se llaman `mc_team_0` a `mc_team_4` (`team_select.c:396`, `1054`). La "casilla 5" es la placa final.
- La placa final es `mc_menu_plate` (`team_select.c:393`). Es el cursor número 5 (`TS_CUR_MENU`, `include/menu/team_select.h`).
- **Casilla vacía:** el dibujo siempre pinta la placa. El retrato y el icono de coste (DP) solo se ven si hay personaje (`team_select.c:1069-1090`, y `TeamSel_SetTeamTex` pone el retrato a `NULL` para huecos, `208-226`). Es decir: una placa sin retrato ni coste.
- **Posiciones exactas en pantalla:** no están en el código, están dentro del archivo de animación (las "películas" `flash[1]` y `flash[2]`). No he podido sacar píxeles de ahí.
- **Qué lado es cuál:** lado 0 (P1) usa la película 37 (`flash[1]`), lado 1 (P2) usa la película 38 (`flash[2]`) (`team_select.c:577-580`). Los colores de las placas salen de la misma hoja de texturas, con celdas distintas: P1 usa las celdas 0-4 y P2 las 5-9 (`team_select.c:1054-1066`). Que P1 sea azul arriba y P2 rojo abajo lo veo en tus capturas; el código no fija ni el color ni "arriba/abajo".

### 1.3 Mover el cursor

- **Dentro de la tira de equipo** (estado `TEAMSEL_ST_TEAM`, `team_select.c:1734`):
  - Izquierda y derecha mueven el cursor entre casillas (`1734-1760`).
  - Izquierda desde la casilla 0 pasa a la placa END. Derecha desde END vuelve a la casilla 0.
  - Derecha desde la última casilla llena pasa directamente a END. Por eso el cursor no "ve" los huecos vacíos más allá del primero.
- **De la tira a la rejilla de personajes:** Cruz sobre una casilla (`team_select.c:1768-1779`). Pasa a `TEAMSEL_ST_GRID` con el cursor del personaje que había en esa casilla.
- **De la rejilla a la tira:** Triángulo en la rejilla, si ya hay algún miembro (`team_select.c:1448-1459`). Restaura el miembro tal como estaba (`CUR_MEMBER = backup`).
- **Dentro de la rejilla de personajes** (`TEAMSEL_ST_GRID`, `team_select.c:1341`): izquierda y derecha cambian de columna (7 columnas por fila). Arriba y abajo cambian de fila (la rejilla gira como un carrete). Cada cambio hace un sonido.

### 1.4 Cómo se llena el siguiente hueco

El flujo de elegir a un personaje nuevo es (`team_select.c:1341-1733`):

1. **Cruz en la casilla vacía:** pasa a la rejilla.
2. **Cruz sobre un personaje:**
   - Si tiene formas, se abre el carrete de formas (`TEAMSEL_ST_FORM`, `1485`).
   - Si no, pasa a la elección de placa de objetos (`TEAMSEL_ST_PLATE`, `1547`).
   - No deja elegir si el coste de DP se pasa del límite o si el personaje ya está en el equipo (suena un "error", ver 1.9).
3. **Placa de objetos:** izquierda y derecha eligen la placa. Cruz pasa a color/traje (`TEAMSEL_ST_COLOR`, `1632`).
4. **Color/traje, Cruz confirma:** el personaje entra en la casilla. Si era un hueco nuevo (`cur == memberCount`), el contador sube y el cursor salta a la siguiente casilla libre (`team_select.c:1707-1709`).
5. Vuelve a la tira de equipo.

Casos especiales:
- **Casilla "random" (0xA1):** salta a la placa de objetos sin mostrar personaje. El personaje se sortea al confirmar el traje, sin repetir personajes ya relacionados con otros del equipo (`team_select.c:1384`, `1679-1691`).
- **Casilla "lista de personajes guardados" (0xA3):** abre una lista de hasta 14 personajes personalizados (`TEAMSEL_ST_CUSTOM`, `1828`). Si no hay ninguno, suena el error.

### 1.5 Cómo se quita un miembro

Se usa **Triángulo estando en la tira de equipo** (`team_select.c:1797-1826`):

- Si solo hay 1 miembro: se quita y el cursor vuelve a la rejilla (`1798-1806`).
- Si el cursor está en END: se quita el último miembro (`1808`).
- Si el cursor está en el hueco libre: se quita el anterior y el cursor retrocede una casilla (dentro del mismo bloque, `1797-1826`).
- Si el cursor está en un miembro: se quita y los de detrás suben una posición (`TeamSel_RemoveMember`, `team_select.c:94-107`).

### 1.6 El botón END (placa final)

- Se muestra siempre como la sexta casilla (`mc_menu_plate`). El texto que se lee ("END") está en la animación; el código no contiene la palabra. Eso no lo he podido comprobar en el código.
- **Cuándo funciona:** con Cruz solo cierra el equipo si hay al menos un miembro (`team_select.c:1781-1792`). Si no hay miembros, suena el error.
- Al cerrar el equipo, ese lado pasa a estado "hecho" (`TEAMSEL_ST_DONE`, `1908`).
- **Modo 1 jugador contra CPU** (y CPU contra CPU): al cerrar el equipo del lado 0, el mismo mando pasa a elegir el equipo del lado 1 (`team_select.c:1786-1790`). Así el mando 1 elige tu equipo y luego el de la CPU.

### 1.7 P1 y P2, banderas de nombre y otros textos

- **P1 es el lado 0 y P2 el lado 1.** En la pantalla de versus, el lado de P2 se dibuja a la derecha.
- **Banderas de nombre:** `mc_name_text_l` (P1) y `mc_name_text_r` (P2) (`team_select.c:837`). Muestran el nombre del personaje **bajo el cursor**, no el del equipo. Con el cursor en END muestran el primer miembro (`1713-1714`, `1744-1745`, `1760-1761`).
- También hay `mc_form_text_l/_r` (nombre de la forma), `mc_face_mask_l/_r` (retrato) y los totales de DP `mc_dp_total_l/_r`.
- **Texto 1P / 2P / COM:** se elige según el modo (`team_select.c:955-963`). En modo 1 jugador: P1 = "1P" y P2 = "COM". En dos jugadores: P1 = "1P" y P2 = "2P". En CPU contra CPU: los dos "COM".

### 1.8 Qué pasa cuando los dos pulsan END

- **Dos jugadores** (`TEAMSEL_PLAYERS_TWO`): cada lado va a su propio mando. El primero que cierra espera a que el otro cierre también. Mientras espera, Triángulo le permite reabrir su equipo (`team_select.c:1908-1925`).
- Cuando los dos están cerrados, se abre la pantalla de escenario y música (`TEAMSEL_STAGE`, `flags` en `team_select.c:1913-1919`).
- **Escenario y música:** los elige **siempre el mando 1** (`gPad[0]` en `team_select.c:1212-1330`), incluso en dos jugadores.
- Al elegir escenario, la pelea empieza 60 fotogramas después (`timer = 0x3C`, `team_select.c:1264-1270`). Luego hay fundido a negro (`1975-1985`) y se fijan los ajustes de la batalla (`BattleSetup_*`, `team_select.c:2027-2049`).

### 1.9 Sonidos (identificadores numéricos)

El código llama `Snd_PlaySe(grupo, id)`. El primer número es un grupo (1 o 2) y el segundo el sonido. No tengo los nombres de esos sonidos, solo los números:

| Acción | Sonido (grupo, id) | Dónde |
|---|---|---|
| Mover cursor en la rejilla de personajes | (2, 0) | 1341-1400 |
| Cambiar fila de la rejilla (abajo / arriba) | (2, 1) / (2, 2) | 1341-1400 |
| Abrir carrete de formas o lista de guardados | (2, 0x29) | 1412, 1430, 1543 |
| Mover cursor entre casillas, placas o trajes | (1, 0) | 1734-1760, 1547-1610, 1632-1650 |
| Confirmar (elegir, cerrar equipo, elegir escenario o música) | (1, 1) | 1272, 1319, 1397, 1441, 1522, 1592, 1724 |
| Atrás (Triángulo) o quitar miembro | (1, 2) | 1292, 1330, 1481, 1606, 1628, 1730, 1824, 1925 |
| No se puede (DP, repetido, END vacío, lista vacía, música bloqueada) | (1, 7) | 1321, 1414, 1443, 1446, 1502, 1504, 1794, 1865, 1892 |

Música: al confirmar la música se llama `Bgm_Play` (`team_select.c:1316`). Al entrar a la pantalla también suena la música del escenario (`720`).

### 1.10 Qué se recuerda al volver a entrar

- **Se recuerda:** modo de juego, tipo de batalla, límite de DP, escenario, música y la posición del cursor de cada casilla (`team_select.c:638-668`, `TeamSel_Run` 1975-1985 guarda los datos al salir).
- **No se recuerda el equipo.** Al entrar, `memberCount` empieza en 0: el código solo copia las 5 casillas (`0xF0` bytes), no el contador (`team_select.c:638-639`). Además, todos los personajes se borran (`chara = -1`, `693-697`). El cursor sí vuelve a la última posición usada.

### 1.11 Lo que no he podido confirmar en el código

- Posición en píxeles de cada casilla y del "END" (están en la animación).
- Colores exactos arriba/abajo (están en la animación).
- Texto exacto de la placa final (ver 1.6).
- Nombre real de los sonidos (solo tengo los números).

---

## 2. HUD: barras del personaje activo y del relevo

### 2.1 Barras del personaje que está peleando

- Vida: **cada barra completa = 10000 de vida, y se dibuja a 160 píxeles de ancho** (`src/battle/hud_gauge_1.c:81`, `w = rem * 160 / HUD_GAUGE_BAR`).
- Si la vida pasa de una barra, las barras de más se apilan. Hay 6 marcas que indican las barras extra (`src/battle/hud_gauge_2.c:120-145`, nodo 2 del árbol "bars in reserve").
- Color de la barra según cuántas barras quedan (`hud_gauge_1.c:81`): 7 o más = color 5; 2 o más = color 1; 1 = color 4; menos de 1 = color 3.
- Ki: 5 barras de 20000 cada una (`hud_gauge_2.c:24-25`, `316-320`).
- **Esto no es el relevo:** el nodo "bars in reserve" de `hud_gauge_2.c` son las barras extra de vida del personaje activo, no de los banquillo.

### 2.2 Panel del relevo (solo uno, el siguiente)

El panel de equipo (`src/battle/hud_team.c`, función `HudTeam_UpdateRoot` en línea 89) muestra **un solo personaje**: el que entraría si cambias.

- Posición: esquina superior izquierda para P1, en `(12, 66)` (`hud_team.c:89-100`). P2 es el espejo (`docs/systems/hud.md`: "side 1 is mirrored, x' = 511 - x").
- **Cara:** se ve solo si hay 2 o más miembros de reserva (`hud_team.c:151-160`, `HudTeam_UpdateFaceA`). Con 2 o más, aparece una placa detrás (sprites 10 y 11). Con uno solo, la cara sale a la posición base y no hay placa. Es decir, no hay un contador numérico visible.
- **Barra de vida del relevo:** un sprite de 32 x 16 píxeles, con nodo en `(46, 4)` respecto al grupo de la cara (`hud_team.c:488-493`). Se desplaza hasta 30 píxeles según `vida / vida máxima` (`hud_team.c:175-183`, `HudTeam_UpdateHpA`). Es decir, a plena vida mide unos 30 píxeles, frente a los 160 píxeles de una barra del personaje activo: unas cinco veces más corta.
- **Solo muestra vida**, no ki.
- **Gauge de cambio:** nodo en `(41, 12)` respecto a la cara (`hud_team.c:510-512`), con barra de 32 x 16 y marca "lleno" de 48 x 16 desplazada `(-3, -3)`. La barra se desplaza hasta 30 píxeles según `gauge / 100000` (`hud_team.c:212-244`).
- **Visibilidad:** el panel aparece solo si hay al menos un miembro de reserva (`HudTeam_SetReserveCount`, `hud_team.c:348-352`). Con 0 reservas, no se muestra.
- **Posición relativa de la barra de vida y del gauge:** ambos nodos cuelgan de la cara, y sus coordenadas se solapan parcialmente. El aspecto final hay que confirmarlo en captura.

### 2.3 Personajes KO

- **No aparecen en el panel.** El número de reservas es `vivos - 1`, con mínimo 0 (`src/battle/hud.c:486-487`). `BtlMember_CountAlive` solo cuenta miembros presentes con vida mayor que 0 (`src/battle/btl_char_member.c:278-292`).
- **El relevo nunca es un KO:** `BtlMember_GetSwitchTarget` salta a los miembros KO y al activo (`btl_char_member.c:298-304`, `BtlMember_NextSwitchTarget` en `btl_char_member.c:316-327`). Si quedan menos de 2 vivos, el relevo es `-1` y el panel no se muestra (`btl_char_api_2.c:2886-2896`).

### 2.4 Gauge de cambio (switch gauge)

- Valor de 0 a 100000 (`include/battle/btl_char_api_2.h:239`).
- **Cómo se carga:** cada fotograma suma `100000 / (tiempo_del_personaje * 30)` (`src/battle/btl_char_action.c:914`), con tope de 100000. En modos 5 y 6 se suma algo más (`0x457`, `916-917`). "Tiempo del personaje" es el parámetro `BtlParam_GetGauge99CTime`; no sé su unidad exacta, la fórmula sugiere segundos.
- **Cuándo se usa:** el cambio voluntario necesita el gauge lleno (100000) y al menos 2 vivos (`src/battle/btl_act_decide.c:449-462`, `BtlAct_CanSwitch`).
- **Indicador "lleno":** cuando el gauge llega a 100000 aparece la marca "lleno" y parpadea: alfa de 0 a 128 y de vuelta en unos 2 segundos (`hud_team.c:212-244`). Al llegar al máximo, el parpadeo empieza (`hud_team.c:335-341`, `HudTeam_SetSwitchGauge`).
- **No hay un icono específico de "cambio"** en el panel. La señal es el gauge con su marca de lleno. Aparte, el HUD dibuja una fila de iconos de botón (`src/battle/hud.c:455`, función `BtlCtrl_GetSwitchPrompt` en `src/battle/btl_param.c:436`). Esa función lee la fila de técnicas, así que no puedo afirmar que sea el aviso de cambio.

### 2.5 Botón de cambio

- La acción de cambio voluntario es la 107 (`src/battle/btl_act_decide.c:467`, `BtlAct_CheckSwitchInput`). Los comentarios la describen como L3 + R3 (`docs/systems/combat.md:330`). No he verificado en el mapa de botones que L3+R3 coincida con la acción 107.
- Cuando el gauge está lleno, se encola la acción `0xF3` (`btl_act_decide.c:478-481`, `BtlAct_QueueSwitch`).

### 2.6 Cambio tras KO

- El cambio por KO **no necesita gauge lleno** (`BtlActB_TickMemberChange`, `src/battle/btl_act_1.c:2588`). Solo necesita que quede al menos un miembro vivo.

---

## 3. Animaciones de cambio

Los valores son fotogramas de la lógica del combate. La fórmula del gauge usa `* 30`, así que supongo 30 fotogramas por segundo. Eso es una suposición, no un dato confirmado.

Hay un detalle: en todas las fases de cambio (`0xEC` a `0xF8`) se activan las mismas banderas. Ver 3.3.

### 3.1 Cambio voluntario: 0xF3 (sale), 0xF4 (entra), 0xF5 (aterriza)

Archivo: `src/battle/btl_act_change.c`. Entrada del cambio: `BtlAct_SwitchLeave` (1363). Llegada: `BtlAct_SwitchArriveWait` (1596) y `BtlAct_SwitchArriveLand` (1620).

| Fase | Fotogramas | Animación | Qué pasa | Sonido / voz / efecto |
|---|---|---|---|---|
| **0xF3, entrada** | 0 | `0xEE` (caída), luego bucle `0x2F` | Se guarda la posición, cámara en el que sale (`ChrCam_RequestCut(chr, 1, 2)`, 1375). | Voz `0x10` en el fotograma 1 (1379). |
| **0xF3, subida** | 1 a 14 | `0x2F` | Sube a **1000 km/h** (`BtlMove_MoveVertical(-KMH(1000))`, 1399). | Sonido del dash del personaje (`BtlParam_GetDashSound`, al pasar de `0xEE` a `0x2F`). |
| **0xF3, frenado y efecto** | 15 | `0x2F` | Frena. Efecto `0xC` y sonido `0x20`. | `0x20` en el fotograma 15. |
| **0xF3, desaparece** | 16 en adelante | `0x2F` | Bandera `0xB` desde el fotograma 16 (`btl_act_change.c:1407`). El código la llama "oculto". | Ver nota 3.5. |
| **0xF3, espera carga** | hasta 46 | `0x2F` | Espera a que el modelo del nuevo personaje esté cargado. | — |
| **0xF3, cambio de fase** | 46 o más, al cargar | — | Pasa a `0xF4` y marca el modelo listo. | — |
| **0xF4, un fotograma** | 1 | `0` (de pie) | Modelo nuevo, oculto un fotograma. Banderas `0xB` y `0xCC`. | — |
| **0xF5, aterriza** | 1 a 8 | `0x2D` (aterrizaje, se mantiene 8 fotogramas) | Entra en su sitio. | Efecto `0xD`, sonido `0x20`, voz `0xF` tras las 8 frames (1625-1640). |
| **0xF5, estable** | Después | `0x26` / `0` | Al 20 % de la animación de reposo lanza el evento `0x5B`. Cámara `(1, 3)`. | — |
| **0xF5, fin** | Al salir | — | Suma 2 segundos al reloj del escenario (`BtlChar_AddStageTimer(2.0f)`, 1668). | Evento `0x4E`. |

- **Lo que hace el que sale:** sube en vertical a 1000 km/h durante 14 fotogramas, frena, y desaparece hacia el fotograma 16. No hay un movimiento lateral.
- **Lo que hace el que entra:** aparece en la posición guardada al empezar el cambio (la bandera `0xF5` la aplica). No he encontrado en el código una animación de vuelo para el cambio voluntario; solo aterriza con `0x2D`.
- **Duración total aproximada:** unos 46 fotogramas de salida (1,5 s a 30 fps) más unos 10 de llegada, unos 2 segundos, más lo que tarde la carga del modelo.

### 3.2 Cambio tras KO: 0xEB (espera), 0xF6, 0xF7, 0xF8 (vuela), 0xF9

Archivo: `src/battle/btl_act_change.c`. Funciones: `BtlAct_KoSwitchLoad` (1674), `BtlAct_KoSwitchEnterWait` (1710), `BtlAct_KoSwitchFlyIn` (1741), `BtlAct_RoundReset` (1815). La espera tras el KO está en `src/battle/btl_act_2.c:456-470` (`BtlAct_ActionEB`).

| Fase | Fotogramas | Animación | Qué pasa | Sonido / voz / efecto |
|---|---|---|---|---|
| **0xEB, espera** | 31 | `0` (de pie, bandera `0xB`) | Tras el KO, si queda alguien vivo, espera **más de 30 fotogramas** (`BtlActB_TickMemberChange`, `btl_act_1.c:2588`, `frames > 30`). | — |
| **0xF6, cae y carga** | Variable | `0xD6` (o `0xD8` si la bandera `0x11` está activa; el comentario lo llama "en el agua") | El derrotado se queda quieto. Cámara sobre el derrotado (`BtlActSwitch_SetLeaveCut`, `708`). Pide el modelo del relevo y espera a que cargue. | — |
| **0xF7, un fotograma** | 1 | `0` | Guarda el punto de entrada del relevo (`BtlActSwitch_SaveEntryPlacement`, `660`): 40° hacia arriba y lejos del rival, hasta 300 unidades o el techo del escenario. Oculto un fotograma. | — |
| **0xF8, cámara y vuelo** | Hasta tocar el suelo | `0x19` (vuelo) | Cámara de entrada (`BtlActSwitch_SetEnterCut`, 738). Vuela a **2500 km/h** hacia el punto de entrada mirando hacia el campo. | Sonido del dash del personaje al empezar. Efectos `4`, `0xA`, `0x3A`. |
| **0xF8, aterriza** | Al tocar suelo | `0x2D` | Se mantiene el aterrizaje. Efecto `0x35`. | Sonido `0x25` y voz `0xF`. Sacudida de cámara (3.0, 0.7 s). Vibración del mando (1.0, 0.5). |
| **0xF8, de pie** | 30 | `0` | Evento `0x5B` al fotograma 30. Después marca la bandera `0xF9`. | — |
| **0xF9, fin de ronda** | — | — | Suma 2 segundos al reloj del escenario (`BtlAct_RoundReset`, 1815-1826). | Evento `0x4E`. |

- **Lo que hace el que cae:** se queda en el suelo mientras carga el relevo. Solo se oculta (bandera `0xB`) si queda bajo el suelo del escenario o si venía de `0xEB` (`btl_act_change.c:1688`).
- **Lo que hace el relevo:** aparece por encima, en el punto de entrada. Vuela en picado a 2500 km/h, aterriza con sacudida y se queda de pie 30 fotogramas.
- **Duración aproximada:** unos 31 fotogramas de espera, más el tiempo de carga (variable), más un fotograma, más el vuelo, más 30 fotogramas de pie. El tiempo de carga es lo que más varía. El comentario de `btl_act_change.h:287-300` lo menciona: "0xF6 waits on BtlChange_IsLoadedFor (disc load time)" (docs/systems/combat.md:694).

### 3.3 Invulnerabilidad, cámara y sonidos

- **Banderas activas durante todo el cambio** (`BtlActChange_SetFlags`, `src/battle/btl_act_change.c:633-644`). Se aplican en cada fotograma de las fases `0xEC` a `0xF8`:
  - `0x125`: "hit-stop" nivel 2. **El rival se queda congelado** durante la animación.
  - `0x42` a `0x46`: en otras partes del código del proyecto estas banderas significan **invulnerable** (`docs/systems/combat.md:457` y `src/battle/btl_act_2.c:3746`). Eso sugiere que el personaje que cambia no recibe daño durante el cambio.
  - También `0x12A`, `0x128` (no comprueba fin de partida), `0xBB`, `0x1B`, `0x1A`, `0x136`.
- **Cámara:**
  - Cambio voluntario: corte al que sale (`0xF3`, `ChrCam_RequestCut(chr, 1, 2)`) y corte al que llega (`0xF5`, `(1, 3)`).
  - Tras KO: corte al derrotado al empezar (`SetLeaveCut`, 708-736) y corte de entrada del relevo (`SetEnterCut`, 738-755). Si el derrotado está bajo el suelo del escenario o viene de `0xEB`, la cámara va a un punto del escenario, no al personaje (`708-727`).
  - Los parámetros de duración de estos cortes llevan `0x3C` (60). No he verificado en el motor de cámara lo que hace exactamente.
- **Sonidos e identificadores:** `0x20` (salida y aterrizaje voluntario), `0x25` (aterrizaje tras KO), voces `0x10` (salida), `0xF` (aterrizaje). El sonido de dash lo da el personaje (`BtlParam_GetDashSound`). No tengo los nombres de los sonidos.

### 3.4 Diferencias: BT3 y TK3

Comparé las funciones de cambio de `Tenkaichi3Decomp/src/battle/btl_act_h_b.c` (líneas 1242, 1476, 1500, 1554, 1590, 1621) con las de BT3. La lógica es idéntica. Solo cambian nombres de campos (`speed` vs `unk98`, `fallSpeed` vs `unk9C`) y los comentarios de cabecera. Lo mismo pasa con el panel de relevo del HUD (`hud_a_b.c` en TK3 tiene las mismas constantes: 30 px, 100000, 0.3 s, 0.25 s, raíz (12, 66)).

No comparé la pantalla de selección de equipo en TK3; sus nombres de archivo son distintos (`menu_*.c`).

### 3.5 Notas y contradicciones del código

- El comentario de `SwitchLeave` (btl_act_change.c:1359-1362) dice "oculto desde el fotograma 15". El código pone la bandera en el fotograma 16 (`if (*timer >= 16)`, 1407). Uso el código.
- Los comentarios de la cabecera (btl_act_change.h:287) dicen que las fases `0xF4..0xF8` están en `btl_act_i.c`. Hoy están en `btl_act_change.c`, segunda parte.
- La bandera `0xB` se llama "oculto" en los comentarios. En la fase `0xEB` también se activa mientras el derrotado está de pie, lo que sugiere que `0xB` puede significar otra cosa (por ejemplo, sin control). No lo he podido confirmar.

---

## 4. Lo que queda por confirmar en el juego

1. Posición exacta de las 5 casillas, del END y de las barras en pantalla (animación del equipo y HUD).
2. Colores arriba/abajo de P1 y P2 en la tira de equipo.
3. Texto exacto de la placa final.
4. Si L3+R3 es el botón real del cambio.
5. Si el icono de la fila de botones es el aviso de cambio.
6. Tiempo real de la carga del relevo tras KO.
7. Si el personaje que cambia es invulnerable de verdad (las banderas 0x42-0x46 apuntan a ello).

---

## 5. Archivos y líneas (resumen)

BT3 (`scratchpad\bt3\BT3-Decompiled\`):
- `src/menu/team_select.c`: 94 (quitar miembro), 126-226 (chips y retratos), 372 (`TeamSel_ClipGoto`), 428 (`TeamSel_Init`), 638-697 (restaurar y reiniciar), 780 (`TeamSel_Draw`), 1136 (`TeamSel_Update`), 1184 (`TeamSel_Input`), 1212-1330 (escenario y música), 1341 (rejilla), 1485 (formas), 1547 (placas), 1610 (panel de objetos), 1632 (color), 1734 (tira de equipo), 1768-1826 (elegir, END, quitar), 1828 (lista de guardados), 1908 (espera a los dos), 1938 (`TeamSel_Run`), 1975-1985 (fundido y guardado), 2027-2049 (ajustes de batalla).
- `include/sys/game_pad.h:24-26`: bits de botones.
- `include/menu/team_select.h`: `TS_CUR_MENU`, estados, tipos y constantes de modo.
- `src/battle/hud_team.c`: 89 (raíz), 151 (cara), 175-183 (barra de vida del relevo), 212-244 (gauge), 335-341 (marca lleno), 348 (reservas), 354-372 (deslizamiento), 403 (init), 488-493 (barra), 510-512 (gauge).
- `src/battle/hud.c`: 455 (fila de botones), 481-494 (valores del relevo cada fotograma).
- `src/battle/hud_gauge_1.c:81`: barra de vida del activo (160 px por 10000).
- `src/battle/hud_gauge_2.c`: 24-25 (constantes), 120-145 (marcas de barras extra), 316-320 (ki), 823 (nodo 2), 991-1005 (deslizamiento).
- `src/battle/btl_char_member.c`: 278 (vivos), 298 (siguiente relevo), 309 (fijar objetivo), 315-327 (siguiente y anterior).
- `src/battle/btl_char_api_2.c`: 2866 (gauge), 2886 (objetivo), 2899 (vivos), 2909 (vida del objetivo).
- `src/battle/btl_char_action.c:914-921`: carga del gauge.
- `src/battle/btl_act_decide.c`: 449 (`BtlAct_CanSwitch`), 467 (entrada 107), 478 (encola 0xF3).
- `src/battle/btl_act_1.c:2588`: espera tras KO.
- `src/battle/btl_act_2.c:456-470`: acción 0xEB.
- `src/battle/btl_act_change.c`: 633 (banderas del cambio), 660 (punto de entrada), 708 (corte de salida), 738 (corte de entrada), 1363 (0xF3), 1596 (0xF4), 1620 (0xF5), 1674 (0xF6), 1710 (0xF7), 1741 (0xF8), 1815 (0xF9).
- `include/battle/btl_act_change.h:287-300`: resumen de las cadenas de cambio.
- `docs/systems/hud.md`: resumen del HUD (valores, espejo P2).
- `docs/systems/combat.md`: 330 (entrada 107), 457 (invulnerable 0x42-0x46), 693-694 (cadena de cambio), 732 (0xD8).

TK3 (`scratchpad\bt3\Tenkaichi3Decomp\`):
- `src/battle/btl_act_h_b.c`: 1242 (0xF3), 1476 (0xF4), 1500 (0xF5), 1554 (0xF6), 1590 (0xF7), 1621 (0xF8), 1695 (0xF9). Lógica igual a BT3.
- `src/battle/hud_a_b.c`: panel de relevo, mismas constantes que BT3.
