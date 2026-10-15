# 12 Conectores: cómo cada modo configura y arranca un combate (US)

Solo lectura. No se ha editado ningún fichero del proyecto ni se ha ejecutado el juego.

Citas: `generated\dbz3_recomp.N.cpp:LÍNEA` (código recompilado US). Convención: **[C]** comprobado en la instrucción recompilada o en `src`; **[I]** inferido por uso o patrón; **[?]** sin confirmar.

Herramientas de apoyo (solo en el scratchpad de la sesión, no en el proyecto): `flow_mode\flow.py` (análisis lineal de constantes y punteros simbólicos sobre los 44 ficheros `dbz3_recomp.*.cpp`: 16.648 stores, 38.909 llamadas) y `flow_mode\showfn.py` (volcado de una función). El análisis ignora ramas: un hallazgo de escritura es una pista que se ha verificado a mano en el código.

Documentos relacionados: `01_pantallas.md` (tabla de pantallas), `docs\08_online\13_duelo_online.md` (bloque CFG), `docs\10_tag_team\11_carga_luchador.md`.

---

## 1. Resumen

1. **Hay dos bloques.** La *petición* del duelo (CFG, 384 B en `0x8245EFC0`, que escribe el menú) y el *combate* `D = *(u32*)0x823D6CF4` (el "mgr"). `D` se crea copiando CFG: `sub_820FE668` hace un `memset` de 35.584 B sobre `D` y un `memcpy` de 384 B de CFG a `D+1680` [C].
2. **`mgr+2018` (modo) no tiene escritor con offset fijo.** Lo copia el `memcpy` de `sub_820FE668`: CFG+338 pasa a D+2018. Ningún `sth/stw ...,2018(rX)` existe en el código [C].
3. **El único escritor de CFG+338 es el reset `sub_82179408`, que pone el modo 5.** Ese reset se ejecuta al pedir la pantalla 23 (nodo `sub_821D7280` → `sub_821D3D80` → `sub_82173B98`) [C]. Que la 23 sea "entrar en el menú de Duelo" es inferencia [I] (`01_pantallas.md` la deja sin nombre).
4. **Las tres filas del Duelo (1P vs COM, 1P vs 2P, COM vs COM) usan el mismo modo 5.** Lo que cambia son los **pads** de cada jugador: CFG+42 (P1) y CFG+122 (P2), donde -1 = COM [C]. La asignación fila → pad tiene un conflicto con el comentario del proyecto (§8).
5. **Los otros modos no usan CFG del menú.** Arrancan con `sub_820FFA80(kind)`: copia un preset de 384 B de `0x82330E50 + kind*384` a una plantilla en pila y llama a `sub_820FE668(plantilla, kind)` [C]. El contenido de los presets no está en el código generado (el XEX está cifrado) [?].
6. **La petición de pelea es `REQ_CUR = 25`.** Se encola con `sub_821D2778(25, 0)` y `sub_821D27C8(10)` (como `sub_82177300`). El nodo `sub_821D7400` la consume: llama a `sub_820FE668(CFG, 0)` y luego pide la pantalla 43 [C].
7. **Receta mínima:** escribir CFG (IDs, pads, cuenta, modo, escenario, tiempo) y encolar la 25. El punto de inserción ya existe: el gancho de entrada de `sub_820FE668` (`src\roster_ext.cpp:1118`) recibe CFG como `sel` y llama a `BeforeBattleSetup` antes de copiar (`src\tagteam_slots.cpp`) [C].

---

## 2. Direcciones

| Dirección | Nombre | Qué es | Evidencia |
|---|---|---|---|
| `0x823D6CF4` | `DUEL_PTR` (mgr) | Puntero a `D`. Si es 0, `sub_820FE668` usa `0x823CE1E0` (objeto estático) y lo guarda | `5.cpp:6930-6941` [C] |
| `0x82456480` | `M` | Base de CFG: `CFG = M + 35648` | `9.cpp:13553` (`addi ... 25728` + `addis 1`) [C] |
| `0x8245EFC0` | `CFG` | Petición del duelo, 384 B. Es el `r3` que pasa `sub_821D7400` | `22.cpp:19030-19034` [C] |
| `0x8247A990` | `REQ_CUR` | Pantalla pedida ahora (-1 = libre) | `docs\08_online\13_duelo_online.md` §0 [C] |
| `0x8247A98C` | `REQ_NEXT` | Pantalla en cola si `REQ_CUR` está ocupada | `16.cpp:18355` [C] |
| `0x8247A988` | `REQ_ARG` | Argumento de la pantalla (`r4`) | `16.cpp:18360` [C] |
| `0x8247A994` | `REQ_FLAG` | Bandera de la cola (`sub_821D27C8`) | `29.cpp:18689` [C] |
| `0x824BA110` | `UI_STATE` | Estado de menú. `+26` y `+27` = `0x824BA12A/B` | `5.cpp:7297-7308` [C] |
| `0x82330E50` | `PRESET` | Presets de 384 B por tipo de combate: `+kind*384` | `43.cpp:7366-7373` [C] |
| `0x82323760` | `ESCENARIOS` | 17 registros de 92 B; `id_mapa` en `+16` | `stage_ext.h`; `36.cpp:3778-3793` [C] |
| `0x8241EA68` | `TABLA_17` | Tabla de 17 B que usa `sub_820F3C98` | `6.cpp:6336` (`0x82420000-5528`) [C] |
| `0x8241EDA8` | `TABLA_ESTADO` | Puntero a función según `mgr+35128` | `30.cpp:7002-7011` [C] |
| `0x823C3D80` | `ESC_ACTUAL` | Índice de escenario cargado | `36.cpp:3803-3804` [C] |

Correcciones a documentos previos: `13_duelo_online.md` §2.3 da `0x8201EA68` para la tabla de 17 B, y el handback del 14-10 da `0x8201EDA8` para la tabla de estados. Los valores correctos son `0x8241EA68` y `0x8241EDA8` [C, aritmética sobre el código]. El gancho de `sub_820FE668` está en `src\roster_ext.cpp:1118` (el documento 13 cita la línea 1069, que ya no es el gancho).

---

## 3. Bloque de petición CFG y su copia en D

`D+k = CFG + (k - 1680)`. Jugador `k` (k = 0, 1): registro en `CFG + 16 + 80k`.

| Offset en CFG | Offset en D | Tipo | Significado | Quién lo escribe | Estado |
|---|---|---|---|---|---|
| `+16 + 80k` | `+1696 + 80k` | 80 B | Registro del jugador k | `sub_82179408` (reset); `BeforeBattleSetup` | [C] |
| `+32 + 80k` (rec+16) | `+1712 + 80k` | u16 | **ID de personaje** (P1 `+32`, P2 `+112`) | selección; `BeforeBattleSetup` (`tagteam_slots.cpp`) | [C] |
| rec+18 | | u16 | Traje | `BeforeBattleSetup` | [C] |
| rec+20 / rec+22 | | u16 | Forma mínima / máxima | `BeforeBattleSetup` | [C] |
| rec+26 (CFG+42, CFG+122) | | u16 | **Pad**: 0 = mando 1, 1 = mando 2, -1 = COM. Se copia a `X_k+5028` | `sub_82173370` (`11.cpp:13657`, `13661`, `13730`, `13732`) y `sub_82171968` (`38.cpp:13529-13563`) | [C] / asignación por fila [?] |
| rec+52 | | u16 | Nº de cápsulas (máx. 7) | `sub_82171968`, `BeforeBattleSetup` | [C] |
| rec+54 | | 7 × u16 | Cápsulas | igual | [C] |
| `+336` | `+2016` | u16 | Nº de luchadores. 0 = 2 | no lo escribe el menú; `sub_820FC9B0` lo pone a 2 si es 0 (`9.cpp:6678-6693`) | [C] |
| `+338` | `+2018` | u16 | **Modo** (ver §5) | `sub_82179408` (`9.cpp:13594`, valor 5) | [C] |
| `+340` | `+2020` | s16 | **Índice de escenario** (0..16) | `sub_82177A88` (`10.cpp:13667`), `sub_82177E00` (`17.cpp:13962`) | [C] |
| `+342` | `+2022` | u16 | Resultado de la tabla (`sub_820F3C98(+340)`) en modos 1, 3 y 5; `D+35574` en modo 4; `UI_STATE+26/27` en modo 2 | `sub_820FE668` (`5.cpp:7042-7050`) | [C] (no escribir a mano) |
| `+344` | `+2024` | u16 | Se copia a `D+35573` (u8) | `sub_820FC9B0` (`9.cpp:6712-6726`) | [C] / nombre [?] (¿nivel de COM?) |
| `+348` | `+2028` | u32 | **Tiempo**: segundos × 60 si es > 0 (`5.cpp:7055-7056`). En el reset, la opción de tiempo de la UI (u16 en `0x824E619C`) × 60, o -1 si es 0 | `sub_82179408` (`9.cpp:13611`) | [C]; unidad [I] |
| `+352` | `+2032` | u32 | -1 por defecto. `sub_820FC9B0` tiene una rama especial cuando vale -1 (`9.cpp:6891-6899`) | `sub_82179408` (`9.cpp:13613`); `sub_82177B90` | [C] / significado [?] |
| `+388` | `+2068` | u16 | Si es ≥ 1000, `sub_82177B90` llama a `sub_82177A88` (elección de escenario) | leído en `15.cpp:13394-13401` | [C] / significado [?] |
| `+35574` (D) | | u8 | Argumento `r4` de `sub_820FE668` (copia de `kind` en `sub_820FFA80`) | `sub_820FE668` (`5.cpp:6986`) | [C] |

Con `D` ya creado, por cada jugador el juego rellena `X_k = D + 2096 + 8240k`: `X+7412` = registro (`D+1696+80k`), `X+7420` = ID (solo lo escribe `sub_82109BA0`, `21.cpp:7511`), `X+5028` = pad (`sub_82110040`, `3.cpp:7997-8003`), `X+5030` = índice del jugador [C].

---

## 4. Quién llama a `sub_820FE668` (crea D desde una petición)

| Llamador | Línea | `r3` (petición) | `r4` (byte) | Uso |
|---|---|---|---|---|
| `sub_821D7400` | `22.cpp:19037` | CFG `0x8245EFC0` | 0 | Duelo: se consume la pantalla 25 |
| `sub_820FFA80(kind)` | `43.cpp:7710` | plantilla en pila `r1+80` (copia del preset `kind`) | `kind` | Práctica y modos de preset (pantallas 15, 18, 20, 21) |
| `sub_82156CE8` | `38.cpp:11790` | `0x82439F90` (otro objeto, `+16` y `+96` son floats) | 0 | [?] Llamado solo por `sub_821D6B88` (`21.cpp:18971`) |

Qué hace `sub_820FE668(r3 = petición, r4 = k)` [C]:

1. Crea o reutiliza `D` (`5.cpp:6925-6941`).
2. `memset(D, 0, 35584)` (`5.cpp:6953`).
3. `memcpy(D+1680, petición, 384)` (`5.cpp:6964`).
4. Guarda `k` en `D+35574` (`5.cpp:6986`).
5. Lee el modo `D+2018` (`5.cpp:7014`). Modos 1, 3, 5: `sub_820F3C98(D+2020)` → `D+2022` (`7042`). Modo 2: `sub_820F3CD8(UI+26, UI+27)`. Modo 4: `sub_820F3D30(k)`. Modo 5: además `sub_820FE3D8(D+1680)` (`7070`), que limpia las cápsulas prestadas.
6. Tiempo: `D+2028 *= 60` si es > 0 (`7055`).
7. `sub_820FC9B0(0)` (`7076`): fija la cuenta y el estado inicial (§5).
8. Por cada jugador: `sub_821007E8(D+1696+80k)` (`7354`).
9. `sub_821D27F8` (`7368`).

---

## 5. Modo por modo

El modo es CFG+338 (`D+2018`). Valores vistos en el código: 0 a 6.

| Modo | Quién lo fija (archivo:línea) | Petición | Estado inicial de combate (`sub_820FC9B0`) | Qué lo distingue |
|---|---|---|---|---|
| **5**: Duelo (1P vs COM, 1P vs 2P, COM vs COM) | Reset `sub_82179408` (`9.cpp:13594`), desde `sub_82173B98` (`22.cpp:13431`) ← `sub_821D3D80` (`40.cpp:18666`) ← nodo `sub_821D7280` (`33.cpp:19077`) cuando `REQ_CUR == 23` | Pads (CFG+42/+122), IDs, tiempo (+348), escenario (+340) | [?] (rama de `D+2032 == -1`, `9.cpp:6885-6900`) | Pads de cada fila |
| **1**: pedido sin preset | `sub_820FE668` con modo 1 (sin escritor propio en el código) [?] | CFG+340 → `sub_820F3C98` | Con argumento 0 (creación): estado 1 (`9.cpp:6872`). Con argumento 1 (arranque): estado 2 (`r7`, `6877`) [C] | [?] |
| **2** | Escritor no encontrado [?] | Bytes `UI_STATE+26/27` → `sub_820F3CD8` | Estado 2 (`9.cpp:6814`) [C] | Lee los bytes de la UI |
| **3** | Preset 3: `sub_821D6368` con `REQ_CUR == 18` pasa `kind = 3` (`32.cpp:18707-18714`). La plantilla escribe modo 3 en `r1+418` (`43.cpp:7222`) | CFG+340 → `sub_820F3C98` | **Estado 4** (pelea directa, sin presentación) (`9.cpp:6797`) [C] | Cuenta 0 |
| **4** | Preset del `kind` recibido: el byte `r4` (`D+35574`) se usa como índice en `sub_820F3D30` (`5.cpp:7285-7294`) [C] | Byte `r4` = `kind` | **Estado 4** (`9.cpp:6782`) [C] | Índice = `kind` |
| **6** | Preset (`r10 = 6` en `43.cpp:7689`, `7693`) [?] | — | **Estado 4** (`9.cpp:6771`) [C] | — |
| **≥ 7 o 0 (sin rama)** | — | — | Estado 1 (`9.cpp:6760`) para ≥ 7 [C]; 0 sigue `CB54` [?] | — |

Presets y pantallas que los piden (de `sub_820FFA80(kind)`):

| Pantalla (REQ) | Nodo que la consume | `kind` que pasa | Quién pide la pantalla | Nombre de la opción [?] |
|---|---|---|---|---|
| 20 | `sub_821D63F0` (`15.cpp:18802-18804`) | 0 (`15.cpp:18827`, llamada `18837`) | `sub_821AC340` (`18.cpp:16781`) | Práctica (combate) (doc 01 §5.1) |
| 21 | `sub_821D63F0` (`15.cpp:18806`) | Valor de una tabla (`r9+r8`, `15.cpp:18820-18822`) | `sub_8219B1C0` (`32.cpp:15478`) | [?] |
| 18 | `sub_821D6368` (`32.cpp:18707`) | 3 (`32.cpp:18711`, llamada `18714`) | `sub_821A5A30` (`28.cpp:17326`) | [?] (¿modo 3 visto en la sonda?) |
| 15 | `sub_821D62E8` (`28.cpp:19080`) | `r3` heredado, sin asignar en la función (`28.cpp:19085`) | No localizado [?] | Duelo (menú) según doc 01 [?] |
| (sin REQ) | `sub_821D38E8` (`13.cpp:19021`) | 0 | Condiciones de `13.cpp:18741-18999` [?] | [?] |

Los modos del preset salen de la plantilla de `sub_820FFA80`: `r26=3` (`43.cpp:7222`), `r26=2` (`7284`), `r24=1` (`7049`), `r9=2` (`7589`), `r10=6` (`7689`), según ramas que dependen de bytes de la página (`r29+28`, `r29+33`). Sin leer el preset no se sabe qué valor sale para cada `kind` [?].

Torneo mundial, Dragon Universe, Supervivencia y Torre: no están localizados en este estudio [?]. Los combates de historia (Dragon Universe) usan #BPC según `docs\11_campanas\CAMPANAS.md`; su vía de arranque no se ha verificado aquí [?].

---

## 6. Escritores y lectores de `mgr+2018`

- **Escritor con offset fijo:** ninguno. El análisis lineal no encuentra `st*,2018(rX)` ni `addi/addis` que lleve a D+2018 [C].
- **Escritores indirectos:** el `memcpy` de `sub_820FE668` (`5.cpp:6964`), que copia la petición (CFG o plantilla) a `D+1680` [C].
- **Lectores** (37 funciones). Los relevantes: `sub_820FE668` (creación), `sub_820FC9B0` (estado inicial, `9.cpp:6716`), `sub_82109638` (reset por luchador, `25.cpp`), `sub_820F8AD0` (IA por modo, `17.cpp`), `sub_820FD2B8`, `sub_820FF338`, `sub_820FFFC8` y otras IA de `sub_820F8xxx` [C].
- **Otros escritores del bloque D que importan al arranque:** `mgr+35128` (estado) lo escribe `sub_820FC9B0` (valores 1, 2, 4 según modo) y `sub_820FD0B8` (valor 3, `41.cpp:6625`, sin llamador directo) [C]. `sub_82121240` escribe `mgr+35188` (perdedor) [C].

---

## 7. Función de arranque y estados

1. **Consumo de la petición:** `sub_821D7400` (`22.cpp:18970`). Si `REQ_CUR == 23`, llama a `sub_821728A8` (`22.cpp:19048`). Si `REQ_CUR == 25` (`22.cpp:19021-19024`), llama a `sub_820FE668(CFG, 0)` (`19037`) y luego `sub_821D9838(1)` (`19042`, pantalla 43) [C]. Al final instala en `obj+28` el nodo `sub_821D7280` (`22.cpp:19014`). Ese nodo, con 23, llama a `sub_821D3D80` (reset, `33.cpp:19077`) y con 25 a `sub_821006F8` (`33.cpp:19071`) [C].
2. **Creación de D:** `sub_820FE668` (`5.cpp:6912`), ver §4.
3. **Inicio del combate:** `sub_820FEA98` (`30.cpp:6896`): `sub_820FC9B0(1)` (`6928`), `sub_82110810` (`6963`, que llama a `sub_82110640` y a `sub_82109638` por luchador), `sub_820FE1E8`, `sub_820F6298`, `sub_820FBA58`, `sub_820C2940` (escenario), `sub_820DCDB8`, `sub_820D4128`, `sub_82122480`, `sub_820F4270`, `sub_820F8770`, `sub_821253A0` [C]. Al final llama a la función de la tabla `0x8241EDA8[mgr+35128]` (`30.cpp:7002-7011`) [C].
4. **Cuadro a cuadro:** `sub_820FFFC8` (`32.cpp:7657`), `sub_820FF750` (`11.cpp:7050`) y `sub_820FF1B8` (`33.cpp:7021`) llaman a `sub_820FEA98`. No tienen llamador directo: se llaman desde la tabla de estados [C/I].
5. **Escenario:** `sub_820C26E0` (`36.cpp:3751`) lee `s16` en `obj+340` (`3781`, `3802`), toma `0x82323760 + idx*92 + 16` como `id_mapa` y carga la entrada AFS (`sub_82083968`, `3796`). Guarda el índice en `0x823C3D80` (`3803-3804`). Se llama por tabla (no hay `bl` directo) [C]. Como nadie escribe `mgr+340` y sí CFG+340, `obj` es la copia de CFG en `D+1680` [I].
6. **Estados del combate** (`mgr+35128`): 1-3 presentación/READY, 4 pelea, 5 final [C, `docs\10_tag_team\16_ko_select_2026-10-11.md`].

---

## 8. Conflictos y pendientes

1. **Pantalla 25.** `01_pantallas.md` §5.1 asigna el nodo `0x821D6D60` a la 25 y dice que `sub_821D7400` es el nodo de la 24. El código de `sub_821D7400` compara `REQ_CUR` con 23 y 25 (`22.cpp:19017-19024`) y crea el duelo con 25 [C]. Hay que revisar esa tabla.
2. **Filas del Duelo.** `sub_82173370` (`11.cpp:13587-13614`) pone: fila 0 → P1=0, P2=-1 (1P vs COM); fila 1 → P1=0, P2=1 solo si se cumple una comprobación de 4 valores == 3 (mando 2 conectado, `11.cpp:13619-13639`) [I]; fila 2 → P1=-1, P2=-1 (COM vs COM); fila ≥ 3 → sin cambio. El comentario de `src\ingame\tagteam_panel.cpp:4` y la llamada `PilotToDuel(row==0?0:row==1?1:3)` (`:516`) dicen: 1P VS COM, 1P VS 2P, COM VS 2P, COM VS COM (fila 3). **Contradicción sin resolver.** Medir el byte `*(obj+48)+53` en cada fila.
3. **Modos 3 y 4.** El lead observa modo 3 y 4 con la sonda `DBZ3_ACT_PROBE`. Por el código, el Duelo arranca en 5 (reset). Si la sonda ve 3 o 4 en el Duelo, hay un escritor no visto. Si ve 5, los 3 y 4 vienen de presets (Práctica y otros). Confirmar con la sonda.
4. **Escenario:** `stage_ext.h` dice `obj+340`. El análisis indica que `obj` es la copia `D+1680` (ver §7.5) [I].
5. **Cuenta de luchadores:** nada en el código escribe CFG+336 desde el menú, así que el Duelo es siempre de 2 [C].
6. **Presets:** contenido de `0x82330E50 + k*384` no leído (XEX2 cifrado, `01_pantallas.md` §9). Pendiente: volcado en ejecución.
7. **Significado** de CFG+344, CFG+352 y CFG+388 [?].

---

## 9. Receta mínima para arrancar un combate sin menús (propuesta)

Pasos:

1. **Condiciones:** `REQ_CUR == -1`, la cola libre, y ningún combate activo (`mgr+35128` en 0 o 5, o `mgr` a 0) [I].
2. **Rellenar CFG** (384 B en `0x8245EFC0`), partiendo de una copia del reset (plantilla de `sub_82179408`):
   - cuenta CFG+336 = 0 (2 luchadores);
   - modo CFG+338 = 5 (Duelo normal);
   - escenario CFG+340 = índice 0..16 de `0x82323760`;
   - pads CFG+42 = 0 (mando 1) y CFG+122 = -1 (COM) para 1P vs COM;
   - tiempo CFG+348 = segundos × 60, o -1 (sin límite) [I];
   - por jugador: ID (rec+16), traje (rec+18), forma mín/máx (rec+20/+22), nº de cápsulas (rec+52) y cápsulas (rec+54).
3. **Encolar la pantalla 25:** `sub_821D2778(25, 0)` y `sub_821D27C8(10)`, igual que `sub_82177300` (`7.cpp:13504`, `13511`). El nodo `sub_821D7400` crea D y pide la 43. Alternativa más directa: `sub_820FE668(CFG, 0)` seguido de `sub_821D9838(1)` (lo que hace el nodo), solo si `REQ_CUR == -1` [I].
4. **Lo demás lo hace el juego:** `sub_820FEA98` prepara a los luchadores (`sub_82110810` → `sub_82109638`) y el estado avanza.
5. **Punto de inserción sin tocar el menú:** el gancho de `sub_820FE668` (`src\roster_ext.cpp:1118`) recibe CFG como `sel`. `BeforeBattleSetup` ya escribe IDs, trajes, formas y cápsulas ahí antes de la copia [C].

Riesgos:

- `sub_820FE668` borra `D` (35.584 B). Si hay un combate en curso, se rompe. Hacerlo solo entre combates [C].
- Un pad -1 (COM) en P1 y P2 a la vez sin contar con la fila de COM vs COM puede no coincidir con la fila que espera el resto del código [?].
- IDs ≥ 595 los borra `sub_820FE3D8` en modo 5 (`src\roster_ext.cpp:1551`) [C].
- Escenario fuera de 0..16 o sin entrada en `0x82323760`: carga AFS inválida [I].
- Modo 3, 4 o 6 entran en pelea directa (estado 4) y usan índices distintos (`kind`, UI+26/27): no son intercambiables con el 5 [C].
- Tiempo: valores no probados. La unidad (segundos) es [I].
- Online: un combate local con CFG distinto puede desincronizarse. No comprobado [?].

Medir antes de implementar (solo lectura, sin tocar el juego):

1. Volcar CFG (384 B) al entrar en `sub_820FE668` para cada fila del Duelo y comparar pads, modo y tiempo.
2. Volcar 384 B de cada preset `0x82330E50 + k*384` en ejecución.
3. Registrar `REQ_CUR`, `REQ_ARG` y `mgr+2018` al pasar por cada opción de menú (la sonda `DBZ3_ACT_PROBE` ya lee mgr+2018 y mgr+35128).
