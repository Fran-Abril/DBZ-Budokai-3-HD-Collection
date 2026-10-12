# Estudio 12: carga completa de un luchador en batalla (orden, funciones, estado)

Fecha: 2026-10-10. Solo lectura de `generated\dbz3_recomp.N.cpp` y `docs\10_tag_team\11_carga_luchador.md`. No se ha editado código ni ejecutado el juego.

Convención: `N.cpp:L` = `generated\dbz3_recomp.N.cpp:L` (línea del `DEFINE_REX_FUNC`, salvo que se indique otra).
- **[C]** = comprobado leyendo el código recompilado (registros, argumentos, orden de llamadas).
- **[I]** = inferido por nombre, uso o forma del código.
- **[N]** = no confirmado.

Bloques (igual que el doc 11):
- `cfg` = `*(u32*)0x823D6CF4` [C].
- `X_k` = `cfg + 2096 + 8240·k` (k = 0..3) [C].
- `sel` = `cfg + 1680`; `rec_k` = `sel + 16 + 80·k` = `cfg + 1696 + 80k`; ID en `rec_k+16` [C, doc 11].
- `char372` = base `0x82329CF0`, 372 B por ID. `char96` = base `0x8234ABB8`, 96 B por ID [C, doc 11].

---

## 0. Herramienta usada

Se construyó un grafo de llamadas de las 11.626 funciones de `generated\` (llamadas `// bl 0x...` por función). Sirve para saber quién llama a quién. Las llamadas indirectas (`bctrl`, tabla de funciones) no aparecen. Esto explica los [N] de la sección 9.

---

## 1. Cadena de inicio de combate (orden de llamadas)

### 1.1 Selección y entradas

| Paso | Función | Dónde | Qué hace | Estado |
|---|---|---|---|---|
| S1 | `sub_820FE668` | `5.cpp:6912` | Lee el ID de cada casilla de la selección (`sel+32+80k`). Llama a `sub_821007E8` (`20.cpp:6982`), que copia `char96+80` a `rec+52/+54` (lista de cápsulas) | [C] (doc 11) |
| S2 | `sub_820FEA98` | `30.cpp:6896` | Orquesta la preparación de batalla. Ver 1.2 | [C] |

### 1.2 Orden dentro de `sub_820FEA98` (`30.cpp:6896`)

Llamadas en este orden [C]:
1. `sub_82120C90` ×2 (`@6914`, `@6921`).
2. `sub_820FC9B0` (`@6926`). Llama a `sub_82092CB8` y `sub_82092DA8` (cola, ver 4.3). Sin carga por luchador.
3. `sub_820FE1E8` (`@6958`).
4. `sub_82110810` (`@6961`). Ver 1.3. **Aquí están las dos fases de reset y reubicación.**
5. `sub_820F6298` (`@6964`, `34.cpp:6336`): llama a `sub_82092A90` (cancelar peticiones, `@6353`) y `sub_820F6250` (`6.cpp:6422`, carga global con `sub_82092908`, constantes `r3=0x8233...`, `r5=21760`; NO por luchador).
6. `sub_820FBA58` (`@6967`), `sub_820C2940` (`@6970`), `sub_820DCDB8` (`@6973`), `sub_820D4128` (`@6976`), `sub_82122480` (`@6979`), `sub_820F4270` (`@6982`), `sub_820F8770` (`@6985`), `sub_821253A0` (`@6988`). Algunos llaman a `sub_82092908` con constantes globales [I, no por luchador].

### 1.3 Dentro de `sub_82110810` (`2.cpp:7837`) [C]

1. `sub_8210E4F8` (`@7848`, `33.cpp:7761`) — fase de preparación con -1 y pool de luchadores. Ver 1.4.
2. `sub_82110640` (`@7851`, `26.cpp:7661`) — reset por jugador y arranque. Ver 1.5.

### 1.4 `sub_8210E4F8` (`33.cpp:7761`) [C]

- Lee el número de luchadores de `cfg+35096` (`0x8918`) (`33.cpp:7779-7786`) [I: número de jugadores activos].
- Bucle 1, por jugador: `sub_82100EB8(X,-1)` (`@7798`), `sub_82100F30(X,-1)` (`@7805`). Después `sub_821171B8` (`@7819`).
- Bucle 2, por jugador: `sub_82112C60(X,1)` (`@7835`), `sub_82112D30(X)` (`@7840`), `sub_82111598(X)` (`@7845`), `sub_82111620(X)` (`@7850`).
- Al final: `sub_82092A90(32, 0x40000000, 1)` (`@7870`), que cancela peticiones de carga por máscara [I, `11.cpp:953`].

### 1.5 `sub_82110640` (`26.cpp:7661`) [C]

- Bucle 1, por jugador: `sub_82109638(X,0)` (`@7702`, reset del luchador, sección 3), luego `sub_82100EB8(X,-1)` (`@7709`) y `sub_82100F30(X,-1)` (`@7716`).
- Bucle 2: `sub_82114650(X,1.0)` (`@7752`), `sub_82110CA0(X)` (`@7757`), `sub_821109D0(X,0)` (`@7764`), `sub_82110AB8(X,-1)` (`@7771`), `sub_82110B08(X)` (`@7776`), `sub_82138E88(X+2948)` (`@7781`), `sub_82113118(X)` (`@7786`), `sub_820CB0A0(X+1616, ...)` (`@7808`).
- Al final: `sub_821177E8` (`@7833`), `sub_821171B8` (`@7836`).
- Cuatro llamadas a `sub_82092908` con constantes globales (`@7859`, `@7878`, `@7897`, `@7918`; `r3` = `0x822...` fijo, `r4=0`, `r5` = 20480, 20736, 21248, 13568) [C]. No dependen del jugador: son recursos comunes [I].
- `sub_82308398(2)` (`@7923`) [N: no está en `generated\`; llamada al runtime].

### 1.6 Cuándo se fija el ID y se cargan los recursos por luchador: `sub_8210E468`

`sub_8210E468` (`7.cpp:7749`) [C]:
1. `sub_82109BA0(sel)` (`@7771`): fija el ID y las tablas (sección 2). Una sola vez para los 4 jugadores.
2. Bucle `k = 0..3`:
   - `X_k+4944 = k` (`stw r29`).
   - Si `k < count` (`count = cfg+35096`): llama `sub_82109148(X_k, rec_k)` (`@7797`) y guarda el último valor no nulo en `r27`.
   - Si `k >= count`: `X_k+4944 = -1` (jugador no activo) y no carga nada.

**Quién llama a `sub_8210E468`:** no aparece en `generated\` (solo en la tabla de funciones: `dbz3_register.cpp:2553`, `dbz3_init.cpp:2571`). Se llama por dirección desde el código del 360 (fuera del recompilado) [I].
**Dato importante [C]:** ni `sub_820FEA98`, ni `sub_820FE668`, ni sus descendientes hasta profundidad 4, llaman a `sub_8210E468` ni a `sub_82109148`. Por tanto, **la carga de ID y recursos por luchador ocurre antes de `sub_820FEA98`, por un camino distinto que aún no está localizado** [N].

---

## 2. Qué escribe `sub_82109BA0` (`21.cpp:7374`) [C, doc 11]

Llamada una vez por `sub_8210E468`. Bucle de 4 jugadores:
- **X+7420 = ID** leído de `sel+32+80k` (`21.cpp:7412`). Excepciones: ID 78, 79, 84 → X+7420 = 35 (`21.cpp:7511`); IDs impares 65, 67, 69, 71, 73 → ajustan `rec+20` y `rec+22` a mínimo 1 (`21.cpp:7418-7470`).
- **X+7412 = rec_k** (`21.cpp:7522`).
- **X+7416 = char372 + 372·ID** (`21.cpp:7528`).
- Por jugador: `sub_82113818` (`@7529`, `26.cpp:7964`, hoja sin llamadas), `sub_82113870` (`@7534`, `18.cpp:8041`, hoja), `sub_82112260` (`@7539`, `28.cpp:8056`; llama a `sub_822F61C0`, sin cargas) [C].
- Una vez: `sub_82101240` (`@7552`, `27.cpp:7028`) [C].

---

## 3. Por jugador: `sub_82109148(X, rec)` (`30.cpp:7533`) [C]

Esta es la función que carga los archivos del personaje. Orden exacto del código:

| Paso | Llamada | Argumentos | Resultado guardado | Ver |
|---|---|---|---|---|
| L1 | `sub_82083968` (`@7561`) | `r3 = char372[+32]` (cámara `#AMB`), `r4=0`, `r5=1`. Solo si `!= -1` | `X+7916` y `X+7928` (handle) | 4.1 |
| L2 | `sub_82083968` (`@7581`) | `r3 = char372[+40] | 0x30000` (idioma; `+40` = 0x28), `r4=0`, `r5=1`. Solo si `!= -1` | `X+7924` y `X+7928` | 4.1 |
| L3 | `sub_82100B78(X, 0)` (`@7593`) | forma 0 | entrada de anm de forma 0 | 3.1 |
| L4 | `sub_82100A10(X, rec+22)` (`@7602`) | forma máxima | modelo en `X+7608+4f` | 3.2 |
| L5 | `sub_82100B78(X, rec+22)` (`@7609`) | | anm | 3.1 |
| L6 | `sub_82100A10(X, rec+20)` (`@7618`) | forma mínima | modelo | 3.2 |
| L7 | `sub_82100B78(X, rec+20)` (`@7625`) | | anm | 3.1 |
| L8 | si `X+7420 == 42`: `sub_82100A10(X,1)` (`@7638`) y `sub_82100B78(X,1)` (`@7645`) | forma 1 | modelo y anm extra | 3.1 / 3.2 |

Notas:
- El idioma usa solo `+40` (0x28). El `+36` (0x24) no se lee aquí [C]. Esto corrige el doc 11, que lo marcaba como "no localizado".
- `rec+20` y `rec+22` son formas `i16` (`lha`) [C].
- `X+7928` se sobrescribe en cada carga; no es un campo fiable por sí solo [C].

### 3.1 `sub_82100B78(X, f)` (`10.cpp:6961`) = anm de la forma `f` [C]

- Entrada: `E = X + 7672 + 28·f` (`(f+274)·28`).
- Condición de carga: `E+0 == 0`, `E+20 == 0`, `E+24 == 0` (no cargada aún).
- Archivo: `id = *(char372 + 4·f)` (`10.cpp:6961`, `lwzx` con `r10 = 4f`). Si `id != -1`: `sub_82083968(id, 0, 1)` (`@7020`) → `E+20` = handle; `X+7928` = handle.
- Los 8 IDs de archivo por forma están en `char372+0..+0x1C` [C, doc 11].

### 3.2 `sub_82100A10(X, f)` (`23.cpp:7273`) = modelo de la forma `f` [C]

- Entrada: `X+7608+4f`. Si ya tiene valor, no hace nada.
- Datos de la forma: registro `R = char372 + 212 + 20·f`:
  - `R+2` (i16) = **ID de modelo** (`lha 214`).
  - `R+4` (i16) = **índice** (`lha 216`).
  - Traje: byte `X+7436+f` (`lbz`).
  - Jugador: `X+4944`.
- Llama a `sub_82133B88(modelo, índice, traje, jugador, 0, ?, ?, R)` (`@7322`, `33.cpp:9983`).
- Resultado: handle → `X+7608+4f`; `X+7928` = `handle+32`.

### 3.3 `sub_82133B88` (`33.cpp:9983`) = objeto de modelo [C]

- Reserva 40 B (`sub_82089F58`, `@10020`) y los pone a cero.
- Campos: `+8` = puntero a `char96[modelo]` (96·modelo + base `0x8234ABB8`); `+12` modelo; `+16` índice; `+20` traje; `+24` jugador; `+36` = 0.
- `sub_82134640(modelo, índice, traje, jugador)` (`@10062`, `4.cpp:10054`) → `+0`. Dentro hay `sub_82083968` (`@10293`) con `r4=0`, `r5=1`: carga un archivo de modelo a partir de `char96` [I: archivo sacado de `char96`].
- `sub_82136D28(...)` (`@10075`, `21.cpp:9964`) → `+4`. Llama a `sub_82133DA0`, `sub_82133DE0` (`26.cpp:9809`, que llama a `sub_82083968` en `@9848`) [I].
- `+32` = `(obj+0)→+112` si `obj+4` no es nulo y `(obj+4)→+148` es 0 [C, lectura de las comparaciones]; en los demás casos queda en 0 o en ese mismo valor según la ruta (condición exacta sin aclarar del todo [I]).
- Si el flag `r7 != 0`: `sub_82136C58` (`@10120`, `20.cpp:10196`) → `+28`.

### 3.4 Quién más llama a los cargadores de luchador

- `sub_82100B78` también lo llaman `sub_82100C10` (`15.cpp:6972`), `sub_8211CBA8` (`13.cpp:8589`) y **`sub_8210E100`** (`29.cpp:7832`) [C].
- `sub_82100A10` también lo llama **`sub_8210E100`** (`29.cpp:7825`) [C].
- `sub_8210E100` (`29.cpp:7737`) corre cada frame (`dbz3_register.cpp:2550`) y hace cambio de forma (lee `X+2980` y `X+2984`). **Ya hay una ruta de recarga en caliente de modelo y anm por forma.** Es la base más cercana para un cambio de luchador [C].

---

## 4. Archivos y colas: cómo se piden y cómo se espera

### 4.1 `sub_82083968` (`27.cpp:102`) = pedir un archivo (asíncrono) [C]

Argumentos: `r3` = ID de archivo, `r4` = flag (`r4 != 0` se guarda en el nodo), `r5` = prioridad/flag. Devuelve el handle del nodo o `-1` si no hay huecos.

- ID = `(banco << 16) | índice`. El banco (`id >> 16`) elige una entrada de 380 B en una tabla global (`lwz 25568`). Por ejemplo, el idioma usa banco `3` (`0x30000`, `oris r3,r11,3`).
- Se copia una cadena de 185 B desde la entrada de banco (`entrada+194`) al nodo (`nodo+6`) con `sub_822F6260`.
- Pool: contador `25532`, máximo `25520`, tabla de huecos `25528`. Cola enlazada: cabeza `25536`, último `25540`.
- Campos del nodo:
  - `+0` (u8) = 0; `+1` (u8) = `r4 != 0`; `+2` (u8) = `25420`.
  - `+6` = cadena de ruta; `+192` = valor derivado de `r5` (flag de prioridad [I]); `+196` = ID; `+204`, `+208`, `+216` = 0 al crear (`+208` lo rellenaría después el hilo de carga [I]); `+220` = enlace de la cola.
  - `+212` = `r4` (flag).
- Retorna el nodo (`r3 = r31`). El contenido llega después, de forma asíncrona.

Otras funciones de la misma familia (uso no confirmado):
- `sub_82083BC8` (`43.cpp:113`): otra inserción en cola (mismos globales). Llamada desde `sub_82084210` (`8.cpp:350`) y `sub_821D6DE0` [I].
- `sub_82083D08` (`31.cpp:118`): desenlaza y libera un nodo (`25536`). Llamada por `sub_8210FDE0` (`27.cpp:7829`, `7904`) y `sub_82083ED8` (`33.cpp:212`) [I].
- `sub_82083ED8` (`33.cpp:120`): recorre la cola. Muchos llamadores [I].

### 4.2 Árbol de cámara: `sub_8210FDE0(X)` (`27.cpp:7784`) [C]

- Lee `h = X+7916` (handle de `#AMB`). Lee `h+192` y, desde ahí, `+208`. Guarda ese valor en `X+7896` (`27.cpp:7796-7819`). Que sea la raíz del `#AMB` es [I] por el uso posterior de `sub_820B0CF8`.
- Hijos con `sub_820B0CF8(raíz, i)` (`15.cpp:2547`): `i=0` → `X+7900`; `i=2` → `X+7908`; `i=3` → `X+7912`. Tabla de 16 B por entrada (`sub_820B0CC8`) [C, doc 11].
- Después pone `h+192 = 0` (transferencia de propiedad) [C].
- Llamador: solo `sub_82110040` (`3.cpp:7989`), que no tiene llamadores en `generated\` [N].

### 4.3 Cola de peticiones de la pantalla (`sub_82092908`, `sub_82092A90`)

- `sub_82092908(slot, 0, recurso, 32, flags)` (`19.cpp:902`) [C]: toma un nodo libre de una lista de 32 (`r6=32`), lo rellena (`+16,+20,+24,+28,+32` con `r6, r7, r5, r3, r4`) e inserta en una lista ordenada por `r5`. Devuelve el nodo en `r3`. Es una cola de recursos genérica (`0x82...` en `r3`), no la misma que `sub_82083968` [I].
- Todas las llamadas de `sub_82110640`, `sub_820FBA58`, `sub_820F6250` pasan **constantes globales** en `r3` (no el puntero `X`). Por tanto, **no cargan nada por luchador** [C].
- `sub_82092A90(a, mask, modo)` (`11.cpp:953`) [I]: recorre la lista y para cada nodo que casa con la máscara llama a `sub_82092A20` (`11.cpp:...`, pone `+28 = -1`). Es una cancelación/liberación. Llamada en `sub_8210E4F8` (`33.cpp:7870`) y `sub_820F6298`.
- `sub_82092CB8` y `sub_82092DA8` (`34.cpp:949`, `26.cpp:881`): hojas, sin llamadas. Usadas por `sub_82109638`, `sub_82114650`, `sub_820FC9B0`. Función exacta [N].

### 4.4 Cómo se espera a que termine la carga (polling)

- **[N]** No se ha encontrado ningún bucle en la cadena de batalla que compruebe un estado de un nodo de `sub_82083968` (`+0`, `+192`, `+208`) antes de seguir. La comprobación más cercana es `sub_8210FDE0`, que lee `+208` sin esperar.
- Posibles lugares: el hilo de carga del runtime (fuera de `generated\`), o `sub_82308398` (`26.cpp:7923`, sin cuerpo en `generated\`).
- Prueba necesaria: buscar en el runtime la escritura de `nodo+208` o `nodo+0` por el hilo de E/S.

---

## 5. Voces y otros recursos por luchador

| Recurso | Cargador | Cadena | Estado |
|---|---|---|---|
| Voces de combate | `sub_82117660` (`20.cpp:8224`) | llamado por `sub_820FB0D0` (`7.cpp:7375`), que llama `sub_821114F8` (`6.cpp:7978`), que llama `sub_82109638` (`25.cpp:7833`) | [C] la cadena. Canal de voz `cfg+1572+32k` |
| Voz por archivo | `sub_820F3980` (`30.cpp:6253`) | llama `sub_82081FE8` (`@6327`) y `sub_82081BF0` (`@6342`), no `sub_82083968` | [C] llamadas; [I] que cargue archivos de voz |
| Tabla de voces | `0x823280B0` (puntero por ID), `0x82328298` (número) | `sub_820F3980` | [C] (doc 11) |
| Labios (lip-sync) | `char96+12` y `forms[].lips` | no seguido | [N] |
| Cara HUD | tabla `0x82373D68` (2 B por ID) | `sub_821B6B98`, `sub_821B9F90` | [C] tabla, [I] uso |
| Capsulas de forma | `X+8156+f` | `sub_82112260` | [I] (doc 11, no reverificado) |

---

## 6. Reset por jugador: `sub_82109638` (`25.cpp:7574`) [C]

Se llama una vez por jugador desde `sub_82110640` bucle 1 (`26.cpp:7702`). Por el grafo, llama a 30+ funciones. Lo verificado:
- Lee `rec = X+7412` (`@7609`). Escribe 4 floats en `rec+0`, `+4`, `+8`, `+12` (`@7650-7670`).
- Lee `X+4944` (`@7629`) para elegir un valor base.
- Lee `rec+32` (bit bajo) y lo guarda en `X+8185` (`@7687`).
- `X+6048 = r30` (`@7849`). Valor de `r30` no seguido [N].
- `X+5444 = X+7908` (`@7846-7853`): puntero a la lista de movimientos (CCM) del `#AMB` [C, doc 11].
- Llama a `sub_820F7868(X+4960)` (`@7854`) [I: usa el CCM].
- Llama a la cadena de voces (`sub_821114F8` `@7833`, sección 5) y a `sub_82112260` (`@8309`), `sub_82112C60` (`@8316`), `sub_82112D30` (`@8326`), `sub_82112CE0` (`@8331`), `sub_82111A98` (`@8348`), `sub_82111128` (`@8353`), `sub_82113518` (`@8360`, `@8377`) [C, llamadas].

---

## 7. Estado y orden (resumen de la máquina)

Orden lineal confirmado, desde la selección hasta el fin de preparación:

1. Selección: `sel` ya tiene los IDs (`sub_820FE668`, `5.cpp:6912`). Cápsulas por ID: `sub_821007E8` (`20.cpp:6982`).
2. Preparación global: `sub_820FEA98` (`30.cpp:6896`) → `sub_820FC9B0` → `sub_820FE1E8` → `sub_82110810` → `sub_820F6298` (cancela y carga global) → resto.
3. Dentro de `sub_82110810`: `sub_8210E4F8` (preparación) y luego `sub_82110640` (reset por jugador).
4. Por jugador en `sub_82110640`: `sub_82109638` (reset + voces + capsulas + CCM).
5. Fin: `sub_821177E8`, `sub_821171B8`, `sub_82308398`.

**Antes del paso 2, pero no dentro de `sub_820FEA98`:** `sub_8210E468` (`7.cpp:7749`) fija ID, X+7416, X+7412 (`sub_82109BA0`) y carga cámara, idioma, anm y modelos (`sub_82109148`). Es el único punto que hace las cargas por luchador, y no se llama desde la cadena de 2 [C/N, sección 1.6].

Espera: no localizada [N] (sección 4.4).

---

## 8. Qué habría que reproducir para cambiar de luchador en batalla (no implementado, solo análisis)

Lo mínimo que el juego ya hace por luchador, en el orden en que habría que repetirlo para un jugador `k`:
1. `X_k+7420` (ID), `X_k+7416` (char372), `X_k+7412` (rec). Hoy solo `sub_82109BA0` los escribe (sección 2).
2. `sub_82109148(X_k, rec_k)`: cámara `#AMB` (`X+7916`), idioma (`X+7924`), anm por forma (`X+7672+28f`), modelos por forma (`X+7608+4f`).
3. `sub_8210FDE0(X_k)`: hijos del `#AMB` (`X+7900/7908/7912`).
4. `sub_82109638(X_k, 0)`: reset, voces, capsulas, CCM (`X+5444`).
5. Limpieza de peticiones pendientes del jugador (`sub_82092A90`).

**Punto de partida más cercano:** `sub_8210E100` (cambio de forma por frame, `29.cpp:7737`) ya llama a `sub_82100EB8`, `sub_82100F30`, `sub_82100A10` y `sub_82100B78`. Es el modelo de recarga en caliente que ya existe [C].

Riesgos [I]: `sub_82109BA0` y `sub_82110640` no se han revisado en su totalidad. Puede haber más escrituras de X+7420 o X+7412 y más estado (por ejemplo, `X+7908` en el CCM y `X+5444`) que hay que reiniciar.

---

## 9. Lo que no se ha podido confirmar

1. Quién llama a `sub_8210E468` y dónde está el camino de carga por luchador (no es `sub_820FEA98`). Probablemente una llamada indirecta desde el código del 360 [N].
2. Quién llama a `sub_8210FDE0` (solo `sub_82110040`, sin llamadores en `generated\`) [N].
3. Cómo se espera a que los archivos pedidos con `sub_82083968` estén listos (campo de nodo y lugar del hilo de E/S) [N].
4. Qué es `sub_82308398` (`26.cpp:7923`, llamada con `r3=2`) [N].
5. Función exacta de `sub_82092908` y `sub_82092A90` (cola genérica o de luchador) [I].
6. Lectura de estadísticas de `char372` (offsets distintos de 0..0x1C, 212..) durante el reset: no inventariado [N].
7. Valor de `X+6048` (r30 en `sub_82109638`) [N].
8. Lista de labios (lip-sync) y el archivo de idioma `+0x24` (no se lee en `sub_82109148`) [N].
9. Si `sub_82100B78` y `sub_82100A10` con forma 1 (ID 42) cargan algo distinto del caso general [I].

---

## 10. Correcciones al doc 11

- **Corregido [C]:** `sub_82109148` lee el idioma solo en `+40` (0x28). El `+36` (0x24) no aparece. Ver sección 3.
- **Ampliado [C]:** el anm por forma está en `char372 + 4·f` (f = 0..7) y su handle va a `X+7672+28f+20`. Esto da el lugar donde guardar el handle de anm, que el doc 11 no tenía.
- **Ampliado [C]:** el modelo de forma se pide con `char372 + 212 + 20·f`, con ID en `+2` y índice en `+4`. Confirma el doc 11.
- **Ampliado [C]:** las voces se cargan desde `sub_82109638` (vía `sub_821114F8` y `sub_820FB0D0`), no desde `sub_82109148`.
- **Corregido [C]:** `sub_82083968` es una petición asíncrona (encola y devuelve un nodo). El doc 11 lo trataba como carga directa.
