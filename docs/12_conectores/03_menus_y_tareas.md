# 03 - Sistema de tareas y menús (principal y Duelo)

Estudio de solo lectura sobre el código recompilado (`generated/dbz3_recomp.*.cpp`) y nuestros paneles (`src/`).
Rutas relativas a la raíz del proyecto. Formato de cita: `archivo:línea`. Lo que no está verificado va marcado con **[?]**.

Convenciones: `lis rX,-32199` + `addi` = dirección 0x8239xxxx; el byte de pantalla está en `0x824BA110` (EU `0x824BA100`).

---

## 0. Veredicto corto

- **Séptima fila del menú principal: viable.** El despacho de filas vive en una sola función (`sub_8218C5D8`). Hoy la fila 7 no hace nada (`bge` a salida). Basta con enganchar esa función y mandar la fila 7 a nuestro código. Lo que falta es la parte visual (texto, posición y que aparezca en la lista) y el gancho EU. Ver sección 2.5.
- **Fila nativa nueva en el Duelo: no viable a corto plazo.** El constructor de la lista del Duelo no está localizado. Nuestro botón ONLINE se dibuja encima (`src/ingame/online_panel.cpp:203-215`), y eso ya funciona.

---

## 1. Sistema de tareas

### 1.1 Lista de tareas y planificador

- **Cabecera de la lista**: `0x8238CD60` (global). Su campo `+40` apunta a la primera tarea. **Sentinela**: `0x8238CDA0`. Fuente: `generated/dbz3_recomp.38.cpp:1012-1027`.
- **Planificador (corre cada frame)**: `sub_820927E0`, `generated/dbz3_recomp.38.cpp:1000-1168`.
  - Recorre la lista: `t = head.+40`, y mientras `t != 0x8238CDA0` hace `t = t.+40` (`:1089-1090`).
  - Por tarea: si `t.+28` no es 0 ni -1, llama a `t.+28` con `r3 = t` (`:1034-1063`, `bctrl` en `:1061-1063`). Es el "estado" de la tarea.
  - Mide el tiempo que tarda cada tarea (`+64`, `+72`, `+76`, `sub_822F6A00` en `:1033`, `:1073`). Tiene código que cambia los enlaces `+36`/`+40` para quitar nodos (`:1095-1160`); la condición exacta de borrado no la he verificado [?].
- **Quién lo llama**: `sub_82165B48`, `generated/dbz3_recomp.14.cpp:13086`. Ese mismo `sub_82165B48` llama después a `sub_821DA2B0` (`:13089`), que es el tick de los menús (filas, dibujo; ver 1.3).
- **Bucle de arranque**: `sub_82165B48` lo llama `xstart` en `generated/dbz3_recomp.20.cpp:23163` (dentro de `xstart`, `:22929-23188`). Que sea un bucle por frame lo infiero; no leí todo `xstart` **[?]**.

### 1.2 Creación de tareas

- **Función creadora**: `sub_82092908`, `generated/dbz3_recomp.19.cpp:902-1052`.
  - Parámetros: `r3` = estado inicial (queda en `+28`), `r4` -> `+32`, `r5` -> `+24`, `r6` -> `+16`, `r7` -> `+20`. Devuelve la tarea en `r3`. Esto se deduce de las escrituras de `:1001-1012`, no de la firma de la función.
  - Toma un nodo de un **pool en anillo de punteros**: índice y máscara en globales `-12968`, `-12972`, `-12828`, base del pool `-12820`, tamaño de elemento `-12832` (`:928-961`).
  - Pone a cero `+0..+12`, `+48..+60` (`:964-972`, `:1005-1012`) y enlaza la tarea en la lista doble `+36`/`+40` (`:1036-1045`).
- **Asignación de memoria del objeto de datos**: `sub_82089F58`, `generated/dbz3_recomp.36.cpp:331` (ej. `sub_8218CB88` la llama con `r3 = 172`).
- **Patrón de la tarea**:

| Campo | Significado | Fuente |
|---|---|---|
| `+28` | estado (puntero a función, llamado cada frame) | `sub_820927E0` (`38.cpp:1034`) |
| `+32` | argumento `r4` (padre o grupo) [?] | `19.cpp:1003-1004` |
| `+36` / `+40` | anterior / siguiente en la lista | `19.cpp:1036-1045` |
| `+48` | objeto de datos (p. ej. el menú de 172 bytes) | `2.cpp:14896-14897` |
| `+52` | fase interna de la tarea (0, 9, ...) [?] | `30.cpp:14840-14843` |
| `+64..+76` | contabilidad de tiempo del planificador | `38.cpp:1076-1081` |

Otros estados: el `+28` de la tarea se cambia desde dentro de la propia función de estado (`stw rY,28(rX)`) o desde otra tarea. Por ejemplo, `sub_821D7400` cambia su propio estado a `sub_821D7280` (ver 4).

### 1.3 Menús como tareas

Los menús son tareas cuyo `+48` es un objeto de datos. Tick de menús: `sub_821DA2B0`, `generated/dbz3_recomp.9.cpp:18716-18871`, que llama a `sub_821D99C0`, `sub_821D9A48`, `sub_821D9AF8`, `sub_821D9D20`, `sub_821D9BC8`, `sub_821D9E68`, `sub_821D9F58`, `sub_821DA030`, `sub_821DA0F8`, `sub_821DA1C0` (`:18796-18850`).

`sub_821D99C0` (`generated/dbz3_recomp.8.cpp:18543-18625`) muestra el patrón de **objeto con tabla virtual**: pide `vtbl[28]` (cuántos elementos), y para cada `i` llama a `vtbl[32](obj,i)` y `vtbl[20](obj,item,0x8001)`. Es el recorrido de los elementos de un menú. El objeto de partida es `[0x82395874 + 48]`. El constructor concreto de las filas **no está localizado [?]**.

---

## 2. Menú principal (pantalla 5)

### 2.1 Cómo se crea

1. `sub_821D3B78` (`generated/dbz3_recomp.21.cpp:18536-18579`) abre el menú principal. Lee dos bytes del bloque de pantalla (`r7 = 0x824BA110`, `[r7+37]` y `[r7+169]`) y hace tail-call a `sub_8218CB88` (`:18576-18577`). Lo llaman 8 archivos distintos (conteo con grep; lista no incluida).
2. `sub_8218CB88` (`generated/dbz3_recomp.2.cpp:14831-14986`):
   - Crea la tarea con `sub_82092908`, estado inicial `0x8218C9F0` (`:14849-14870`).
   - Reserva 172 bytes con `sub_82089F58(172)` y los pone en `tarea+48` (`:14887-14897`).
   - Escribe en el objeto: fila inicial `+150` (`stb`, `:14913`), `+148` (u16, `:14927`), `+151` (`:14931`), `+144` (objeto de UI de `sub_8217ED38`, `:14972-14973`).
3. Estados en orden:
   - `0x8218C9F0` (fase): `generated/dbz3_recomp.30.cpp:14825-15059`. Cambia su `+28` a `0x8218C8E0`.
   - `0x8218C8E0` (entrada con temporizador en `obj+160`, float): `generated/dbz3_recomp.3.cpp:14833-15000`. Al acabar pone `+28 = 0x8218C5D8` (`:14970-14982`).
   - `0x8218C5D8` (**menú**, lo que nos interesa): `generated/dbz3_recomp.10.cpp:14782-15241`.
   - Salida: `0x8218C4C8` (`generated/dbz3_recomp.42.cpp:15062`), puesto en `10.cpp:15002-15004`.

### 2.2 Filas, tope y despacho

La fila del cursor es el byte `obj+150` del menú (`generated/dbz3_recomp.10.cpp:14894-14895`: `lbz r11,150(r31)`).

**Tope**: `cmplwi r11,7` + `bge` a salida (`10.cpp:14914-14917`). El juego considera válidas las filas **0 a 6** (7 filas). La fila 7 o superior no hace nada.

**Despacho fila -> pantalla** (`10.cpp:14918-14958`):

| Fila (`+150`) | Valor pasado a `sub_821D2778` | Línea | Nombre (fuente) |
|---|---|---|---|
| 0 | 6 | `:14950-14952` | Dragon Universe (`src/ingame/online_panel.cpp:179-180`) |
| 1 | 14 | `:14945-14947` | Duelo (`src/ingame/online_panel.cpp:167-170`) |
| 2 | 16 | `:14940-14942` | Torneo [?] |
| 3 | 19 | `:14935-14937` | Práctica [?] |
| 4 | 22 (antes llama a `sub_82179328`) | `:14927-14932` | ¿Edición de habilidades o Tienda? [?] |
| 5 | 28 | `:14922-14924` | Opciones [?] |
| 6 | 23 | `:14918-14919` | Dragon Arena (`src/ingame/online_panel.cpp:241-242`) |

Después del despacho: `sub_8218C588(obj)` y `sub_82081A48` (`:14960-14969`). Luego el estado pasa a la salida `0x8218C4C8` (`:15002-15004`).

**Cuándo se ejecuta el despacho**: la rama se toma si el bit `0x4000` de la halfword `+14` de la entrada `[0x8238B670]*184 + 0x8238B78E` está activo (`:14815-14845`). La rama del bit `0x1000` (`:15007-15060`) hace otra cosa [?]. La tabla de 184 bytes y el índice `0x8238B670` (¿jugador o entrada actual?) **no están aclarados [?]**. Tampoco sé con certeza qué botón de XInput corresponde a `0x4000`: nuestro código usa `kA = 0x1000` (`src/ingame/online_panel.cpp:217`).

**Cuántas filas hay en realidad**: el juego tiene un tope de 7 (`cmplwi r11,7`). Nuestro comentario dice que en una partida sin todo desbloqueado hay **menos filas** y que el cursor puede acabar en Torneo o Tienda (`src/ingame/tagteam_panel.cpp:158-160`). Esto encaja con tu caso de 6 opciones: Dragon Arena (fila 6) no aparece sin desbloqueo. **El número de filas visibles no es una constante fija; se calcula al abrir el menú [?]** (probablemente con el mismo bloque de 184 bytes; no lo he confirmado).

### 2.3 Modo pendiente (cómo se pasa de menú a pantalla)

- `sub_821D2778` (`generated/dbz3_recomp.16.cpp:18337-18363`): escribe el **modo pendiente** en `0x8247A990` si está libre (-1). Si no, lo escribe en `0x8247A98C`. El segundo argumento va a `0x8247A988`.
- El menú principal lo llama en `10.cpp:14954-14958` con `r3` = pantalla de la tabla. El mismo global está documentado en `docs/10_tag_team/09_tagteam_ko_menu.md:170`.
- Consumidores: 78 funciones leen `0x8247A990` (conteo con script sobre `generated/`; apéndice 4.1). Tres de ellas manejan los modos de menú: `sub_821D7AE8` (`generated/dbz3_recomp.17.cpp:19008-19203`), `sub_821D7C30` (`27.cpp:18818-18959`) y `sub_821D7990` (`35.cpp:18793-18983`). Las tres comparan con la misma lista `[4,6,14,16,19,22,23,28]`. Cuál de las tres está activa **no lo sé [?]**.
- `sub_821D7400` (`generated/dbz3_recomp.22.cpp:18970-19061`) es una tarea de modo: si hay modo pendiente, cambia su estado a `sub_821D7280` (`:19009-19014`). Con modo 23 llama a `sub_821728A8` (`:19045-19048`). Con modo 25 crea el duelo con el bloque de configuración `0x8245EFC0` (`sub_820FE668`, `:19025-19037`; ver también `docs/08_online/13_duelo_online.md:38`).

### 2.4 Dónde se engancha nuestro código

- Ya hay un gancho de estado: `DBZ3_HOOK(sub_8217CB10, dbz3eu_sub_8217CAC8)` en `src/select_ext.cpp:481`. Hay que buscar el par EU para `sub_8218C5D8`.
- Nuestro piloto espera la pantalla leyendo `0x824BA110` (`src/ingame/menu_pilot.cpp:85`).

### 2.5 Viabilidad de una 7ª fila nativa

Qué habría que hacer:

1. **Despacho**: enganchar `sub_8218C5D8` (o su gemela EU) y, si la fila es 7 y se confirma, llamar a nuestro manejador en vez de `sub_821D2778`. Esto no requiere cambiar la tabla de 0..6. **Viable.**
2. **Visual**: añadir un elemento al objeto de filas (texto de `data_usi #AZT`, posición). El recorrido está en `sub_821D99C0` (vtable 28/32/20), pero **no he encontrado el constructor que crea las filas [?]**. Sin él, la fila nueva no se dibuja aunque el despacho funcione.
3. **Mostrarla cuando faltan otras**: con una partida parcial, las filas ocultas se saltan. Hay que ver cómo lo decide el juego antes de meter la fila 7 **[?]**.
4. **Nuestro código**: `src/ingame/online_panel.cpp:243-260` da la vuelta a 7 filas (`% 7`, línea 250) y usa la fila 6 para Tag Team. Habría que actualizarlo.
5. **EU**: no sé si las direcciones EU de estas funciones coinciden. `kUiSnapEu` (`src/net/netplay.h:105`) indica que el bloque de pantalla cambia.

Veredicto: **viable**, con esfuerzo medio. El punto difícil es el constructor visual.

---

## 3. Menú de Duelo (pantalla 14)

### 3.1 Filas nativas

El juego tiene **4 filas**. El orden en nuestro código:

| Índice del juego | Opción | Fuente |
|---|---|---|
| 0 | 1P VS COM | `src/ingame/tagteam_panel.cpp:516` |
| 1 | 1P VS 2P (sale en gris sin mando 2) | `src/ingame/tagteam_panel.cpp:499`, `src/ingame/online_panel.cpp:289` |
| 2 | COM VS 2P | `src/ingame/tagteam_panel.cpp:154, 516` |
| 3 | COM VS COM | `src/ingame/tagteam_panel.cpp:516`, `src/ingame/online_panel.cpp:203-204` |
| (nuestra) | ONLINE: quinta fila, dibujada encima | `src/ingame/online_panel.cpp:203-215, 286-293` |

- El cursor del Duelo se recuerda entre visitas en `g_duel_row` (`src/ingame/online_panel.cpp:211-213`).
- `PilotToDuel` cuenta filas módulo 4 (`src/ingame/tagteam_panel.cpp:172-174`). Confirma las 4 filas.
- Nuestro piloto entra al Duelo con `Poke(obj+150, 1)` (`tagteam_panel.cpp:163`), es decir, la fila 1 del menú principal.

### 3.2 Cadena de funciones

- **Lanzador (crea el duelo, modo 25)**: `sub_82177300`, `generated/dbz3_recomp.7.cpp:13465-13523`. Pone modo 25 con `sub_821D2778` (`:13500-13504`) y después llama a `sub_821D27C8` con 10 (`:13507-13511`) [?] sobre su significado.
- **Estado del Duelo (resultado 4 = lanzar)**: `sub_82177918`, `generated/dbz3_recomp.3.cpp:13643-13705`. Llama a `sub_8218A520([menú+12])`; si devuelve 4, llama a `sub_82177300` (`:13672-13691`).
- **Sub-objeto de resultado**: `sub_8218A520`, `generated/dbz3_recomp.42.cpp:14936-14943`. Devuelve la halfword `+14` del objeto `+48` de su argumento (el valor elegido).
- **Otro estado del Duelo**: `sub_82177B90`, `generated/dbz3_recomp.15.cpp:13335-13429`. Compara un valor entero derivado de `+268` con 1000 (`:13396-13398`). Si llega a 1000 o más llama a `sub_82177A88` (`:13399-13401`). Si es menor, escribe -1 en una tabla (`:13404-13412`) y llama a `sub_82177300` (`:13413-13415`) [?] sobre qué dato es.
- **Constructor de la lista del Duelo**: `sub_82177A88`, `generated/dbz3_recomp.10.cpp:13557-13694`. Es el que monta la lista de opciones:
  - Llama a `sub_8218AEA0` (`:13670`) y guarda lo que devuelve en `[menú+12]` (`stw r3,12(r30)`, `:13675`). Ese objeto es el que luego lee `sub_82177918` con `sub_8218A520`.
  - Pone el estado de la tarea a `0x82177918` (`lis -32233` + 31000, `:13677-13679`).
  - Rellena valores de 16 bits en la pila (los `li` 9, 7, 4, 8, 10, 11, 2, 6, 3, 1, 5 de `:13582-13638`) y lee una tabla en `0x8245xxxx`. Que ese objeto sea exactamente la lista de 4 filas y que el índice coincida con el orden de la sección 3.1 **no está confirmado [?]**.
- **Actualización de la UI del Duelo**: `sub_82177608`, `generated/dbz3_recomp.21.cpp:13416-13892` (no analizada a fondo).
- **Estados que apuntan a estas funciones**: `sub_82177A88` escribe `+28 = sub_82177918`; `sub_82177C30` escribe `+28 = sub_82177B90`.

Falta: cómo se asocia la fila elegida (índice 0-3) con el ítem de la tabla y con el bloque `0x8245EFC0`. **[?]**

### 3.3 Viabilidad de una fila nativa nueva

- El tope de 4 filas no está en una constante que haya encontrado. Nuestro código asume 4 (`% 4`, `online_panel.cpp:289`).
- Hay que localizar el constructor de la lista (candidato: `sub_82177A88`) y su contador de elementos. **Sin eso, no es viable de forma segura.**
- **Recomendación**: mantener ONLINE como fila dibujada encima (ya funciona, `online_panel.cpp:203-215`). Una fila nativa exigiría reescribir la lista del juego.

---

## 4. Apéndice: funciones de modo

### 4.1 Familia que lee `0x8247A990` (78 funciones)

Ejemplos con los modos que comparan (de `modefam.py` sobre `generated/`):

| Función | Archivo:líneas | Modos comparados | Estado que asigna |
|---|---|---|---|
| `sub_821D7400` | 22.cpp:18970-19061 | -1, 23, 25 | `sub_821D7280` |
| `sub_821D7280` | 33.cpp:18994-19090 | 23, 25 | `sub_821D70F8` |
| `sub_821D7AE8` | 17.cpp:19008-19203 | 4, 6, 14, 16, 19, 22, 23, 28 | `sub_821D7990` |
| `sub_821D7C30` | 27.cpp:18818-18959 | 4, 6, 14, 16, 19, 22, 23, 28 | `sub_821D7AE8` |
| `sub_821D7990` | 35.cpp:18793-18983 | 4, 6, 14, 16, 19, 22, 23, 28 | `sub_821D78F8`, `sub_821D5590` |
| `sub_821D5C38` | 11.cpp:19069-19198 | 6, 8, 10, 13 | `sub_821D78F8` |
| `sub_821D6110` | 43.cpp:18949-19147 | 6, 8, 10, 13 | `sub_821D5C38` |

Varias funciones escriben `0x821D7C30` como estado de salida (p. ej. `sub_821D4548`, `sub_821D42D8`, `sub_821D43A8`).

### 4.2 Escrituras de `+28` con valor resuelto

Escaneo completo: escribe 1093 asignaciones `+28` en todo `generated/`. Las relevantes para menús:

- `sub_8218C9F0` -> `sub_8218C8E0` (`dbz3_recomp.30.cpp`)
- `sub_8218C8E0` -> `sub_8218C5D8` (`dbz3_recomp.3.cpp`)
- `sub_8218C5D8` -> `sub_8218C4C8` (`dbz3_recomp.10.cpp:15004`)
- `sub_821D7400` -> `sub_821D7280` (`dbz3_recomp.22.cpp:19013`)
- `sub_82177A88` -> `sub_82177918`, `sub_82177C30` -> `sub_82177B90` (`dbz3_recomp.10.cpp`, `dbz3_recomp.34.cpp`)

---

## 5. Nuestro código que depende de esto

| Qué | Dónde | Por qué importa |
|---|---|---|
| Constantes de pantalla 5 y 14 | `src/ingame/tagteam_panel.cpp:61` | Coinciden con el despacho (`sub_821D2778`) |
| Ir al Duelo desde el principal | `src/ingame/tagteam_panel.cpp:155-179` | Escribe la fila 1 en `obj+150`, pulsa A y cuenta filas del Duelo |
| Filas Tag Team en el Duelo | `src/ingame/tagteam_panel.cpp:485-516` | Mapea sus 3 filas a los índices 0, 1 y 3 |
| Cursor del principal (7 filas) | `src/ingame/online_panel.cpp:238-260` | `% 7`, fila 6 = Dragon Arena |
| Cursor del Duelo (4 filas) | `src/ingame/online_panel.cpp:203-215, 286-293` | `% 4`, ONLINE dibujada encima |
| Pantalla del juego | `src/ingame/online_panel.cpp:167-170`, `src/ingame/menu_pilot.cpp:85`, `src/tagteam.cpp:1310-1321` | Lectura del byte `0x824BA110` |

---

## 6. Direcciones útiles

| Dirección | Qué es | Fuente |
|---|---|---|
| `0x8238CD60` | cabecera de la lista de tareas | `38.cpp:1016-1017` |
| `0x8238CDA0` | sentinela de la lista | `38.cpp:1018-1019` |
| `0x82395874` | objeto global cuyo `+48` es el menú en curso [?] | `8.cpp:18543-18559` |
| `0x8238B670` / `0x8238B780` | índice y tabla de 184 bytes del menú [?] | `10.cpp:14804-14840` |
| `0x8247A990` | modo pendiente (y `+4` `0x8247A98C`, `+8` `0x8247A988`) | `16.cpp:18337-18363` |
| `0x824BA110` | pantalla actual (EU `0x824BA100`) | `src/ingame/online_panel.cpp:167` |
| `0x8245EFC0` | bloque de configuración del Duelo | `docs/08_online/13_duelo_online.md:38` |
| `0x8218C5D8` | estado del menú principal | `10.cpp:14782` |
| `0x8218C4C8` | estado de salida del menú principal | `42.cpp:15062` |
| `0x8217CC68` | estado de la rueda de personajes | `src/select_ext.cpp:504` |

---

## 7. Qué no está verificado

- **[?] Constructor de filas visuales** del menú principal y del Duelo (texto `#AZT`, posición, conteo de elementos). Solo tengo el recorrido `sub_821D99C0`.
- **[?] Rama del bit `0x1000`** y significado exacto de los bits de la halfword `+14` (botones o eventos del juego).
- **[?] Nombres** de las filas 2, 3, 4 y 5 del principal. Solo están confirmados Dragon Universe (0), Duelo (1) y Dragon Arena (6).
- **[?] Mapeo** de las 4 filas del Duelo a sus ítems y al bloque `0x8245EFC0`.
- **[?] Planificador**: `sub_820927E0` es el que recorre la lista, pero el bucle exterior de `xstart` no lo he leído entero.
- **[?] Versión EU**: todas las direcciones de este documento son US salvo que se diga lo contrario.
- **[?] Cuál de las tres copias** `sub_821D7AE8` / `sub_821D7C30` / `sub_821D7990` se ejecuta en cada caso.
