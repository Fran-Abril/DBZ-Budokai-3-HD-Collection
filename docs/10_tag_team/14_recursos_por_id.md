# Estudio 14: recursos por ID (aura, técnica, voces, labios, cara HUD, cápsulas)

Fecha: 2026-10-10. Solo lectura de `generated\dbz3_recomp.N.cpp`. No se ha editado código ni ejecutado el juego.
Complementa el doc 11 (`11_carga_luchador.md`) y el doc 12 (`12_carga_completa.md`).

Convención: `N.cpp:L` = `generated\dbz3_recomp.N.cpp:L` (línea del `DEFINE_REX_FUNC`, salvo que se indique otra).
- **[C]** = comprobado en el código recompilado (registros, offsets, llamadas).
- **[I]** = inferido por uso, nombre u orden.
- **[N]** = no confirmado.

Bloques (igual que doc 11/12):
- `cfg` = `*(u32*)0x823D6CF4` [C]. `X_k` = `cfg + 2096 + 8240·k`.
- `sel` = `cfg + 1680`. Registro de selección de cada luchador: `sel + 16 + 80·k`; el ID está en `sel + 32 + 80·k` [C].
- Tablas por ID (IDs 0..63 y más): aura `0x82322950` (u32), técnica `0x82333208` (u32), voces `0x823280B0` (puntero) + `0x82328298` (número), cara HUD `0x82373D68` (u16), `char96` `0x8234ABB8` (96 B), `char372` `0x82329CF0` (372 B) [C].
- Cómo se buscó: un simulador que sigue los registros (`lis`/`addi`/`mulli`/`lwzx`) sobre el ensamblador que viene en los comentarios `// ...` de cada línea. Encuentra qué funciones leen cada tabla por ID. El grafo de llamadas no ve las llamadas indirectas (tabla de funciones); eso explica los [N].

---

## 0. Respuesta corta

Cambiar solo `X+7420` (el ID del luchador) **no basta** para todos los recursos.

| Recurso | ¿Se busca en cada uso o se carga al inicio? | ¿Basta cambiar X+7420? | Dónde queda guardado |
|---|---|---|---|
| Aura (efecto de aura del ID) | Al inicio (recorre la lista de selección) | **No**, hay que repetir su carga | global `*(0x82322AEC) + 420·k` |
| Aura por forma (aura de `char96.forms[f]`) | Al montar cada forma | Sí, si se rehace esa forma (`sub_82100A10`) | registro de forma `X+7608+4f`, +28 |
| Técnicas (BSP) | Por trabajo en cola (callback `sub_82128B08`) para el caso normal; al inicio solo para el caso especial ID 35 | **No** con certeza: quién encola el trabajo es [N] | global `*(0x82333994) + 1152·k + 624` |
| Voces de combate | Al inicio (cadena de `sub_82110640`) | **No**, depende de `X+2980/2984` y de las formas | canal `cfg + 1572 + 32·k` (handle en +8) |
| Labios (lip-sync) | Al montar cada forma | Sí, si se rehace esa forma | registro de forma `X+7608+4f`, +4 |
| Cara HUD (barra de vida) | Por llamada: el llamador pasa el ID | Depende del llamador [N] | no cacheada por ID [I] |
| Cápsulas (lista y por forma) | Lectura desde punteros cacheados | Depende: `X+7416` se reescribe en `sub_82109BA0` [C] | ver sección 6 [I] |

Lo que sí decide el cambio de forma/ID: `sub_82109BA0` es el único que escribe `X+7420`, `X+7416` y `X+7412` [C, doc 11]. Cualquier cambio de luchador tiene que pasar por ahí (o reescribir esos tres campos) antes de recargar lo demás.

---

## 1. Aura del ID (efecto de aura) [C salvo lo indicado]

a) **Lector:** `sub_820B8F78` (`21.cpp:2820`).
- Lee `aura[ID]` con `lwzx` en `0x82322950 + 4·ID` (`21.cpp:2820`, `rlwinm r10,r4,2` + `lwzx r3,r10,r11`).
- Si el valor es `-1` no hace nada (devuelve 0). Si no, llama a `sub_82083968(fid, 0, 1)` (pedir archivo, asíncrono; doc 12 §4.1).
- Parámetros: `r3` = índice del jugador `k`, `r4` = ID.

**Llamadores:**
- `sub_820BA578` (`10.cpp:3043`, llamada en `10.cpp:3105`). Recorre la lista de selección: ID en `r3+32+80·i` (`lhau r4,80(r29)`), con `r3` = `sel` [I, por el offset: coincide con `sel+32+80i`]. Lo llama `sub_820CD5D8` (`3.cpp:4197`, llamada en `3.cpp:4219`).
- `sub_821C0198` (`29.cpp:17703`, llamada en `29.cpp:17766`), llamado desde `sub_821C0BE0` (`27.cpp:17511`). Este segundo camino también carga modelo (`sub_82133B88`) en `sub_821C24B8` (`27.cpp:17648`) y `sub_821C6BB8` (`33.cpp:17926`). Quién los llama: [N] (llamadas indirectas).

b) **Dónde se guarda:** `G = *(0x82322AEC)` (puntero global, `21.cpp:2820`, `lwz r31,10988(r9)`). El aura del jugador `k` va en `G + 420·k + 0` (`stwx r3,r30,r31`, `r30 = k·420`) [C].
- Aparte, `sub_820BA578` guarda un puntero de 12 B en `G+968` (`10.cpp:3043`) [C].

c) **Caché o búsqueda por uso:** **caché al inicio.** Se recalcula solo cuando `sub_820CD5D8` vuelve a correr sobre la lista de selección [C].
- El ID viene de la lista de selección (`sel+32+80i`), **no** de `X+7420`. Por eso cambiar `X+7420` no cambia el aura. [C]
- Aura por forma (distinta): `sub_82136C58` (`20.cpp:10196`) lee `char96.forms[f].aura` (u16 en `0x8234ABB8 + 96·ID + 16 + 8·f + 4`, es decir, `char96 +20` de la forma) y lo guarda en el registro de forma `+28` (`X+7608+4f`). Se reconstruye al montar la forma (ver §7) [C/I].

d) **Liberación:** no se ha encontrado una función que libere `G+420·k` [N]. Se sobrescribe en cada llamada.

---

## 2. Técnicas de especial (BSP) [C salvo lo indicado]

a) **Lector:** `sub_82128040` (`29.cpp:9251`).
- Lee `bsp[ID]` en `0x82333208 + 4·ID` (`lwzx r3,r7,r8`, `r8 = 0x82333208`) [C].
- Si no es `-1`, llama a `sub_82083968(fid, 0, 1)` y guarda el resultado en `G2 + 1152·k + 624` (`stw r3,624(r31)`), con `G2 = *(0x82333994)` (`lwz r11,14740(r9)`) [C].
- Parámetros: `r3` = `X_k` (usa `X+4944` = `k`), `r4` = ID.

**Llamadores:**
- `sub_82128338` (`1.cpp:8785`, llamada en `1.cpp:8892`). Recorre la lista de selección (ID en `sel+32+80i`, `lha 0(r26)`) y los bloques `X_k` (`r27 += 8240`). La llamada a `sub_82128040` solo se ejecuta si **`X_k+7420 == 35` y el ID de la lista no es 35** [C, `1.cpp:8892` y saltos justo antes]. El doc 11 indica que los IDs 78, 79 y 84 se guardan como `X+7420 = 35`. Así que este camino es un caso especial de esos IDs, no el normal [C/I]. Lo llama `sub_820CD5D8` (`3.cpp:4224`).
- `sub_82128B08` (`16.cpp:9180`, llamada en `16.cpp:9209`). Es un **callback de trabajo**: recibe un registro `r4` con el ID en `+0`, el jugador en `+4` y un puntero en `+44`. Toma el bloque del jugador de la tabla `cfg + 35188 + 4·k` y llama a `sub_82128040` con el ID del registro [C]. Esto es lo que carga la técnica en el caso normal [I]. Quién encola ese trabajo y cuándo: [N] (posible cola de peticiones, ver doc 12 §4.3).

c) **Caché o búsqueda por uso:** la técnica se guarda por jugador y se usa desde `G2+1152·k+624`. El caso normal se carga por trabajo en cola (`sub_82128B08`), no en el arranque directo [C/I]. Cambiar `X+7420` no la recarga por sí mismo; el trabajo en cola usa el ID del registro, no `X+7420` [C/I].

d) **Liberación:** no encontrada [N].

---

## 3. Voces de combate [C salvo lo indicado]

a) **Lector de voces:** `sub_820F3980` (`30.cpp:6253`).
- Recibe `r4` = ID, `r5` = índice de situación de voz, `r3` = canal de voz, `r6` = -1.
- Comprueba `r4` entre 0 y 121 y `r5 < count[ID]` (`0x82328298 + 4·ID`) [C].
- Lee la lista de voces del ID: `voces[ID]` = puntero en `0x823280B0 + 4·ID`, y `voces[ID][r5]` = índice ADX (`lwzx`) [C].
- Si el índice no es `-1`, carga el banco ADX con `sub_82081FE8(fid | 0x20000, -3)` y `sub_82081BF0(handle, 1)` (`30.cpp:6253`, `@6327`, `@6342`) [C].
- Guarda el manejador en `canal+8` y un volumen en `canal+24` (`stfs`), leído de un float en `cfg+35144` [C].

b) **Dónde se guarda:** canal de voz `cfg + 1476 + 32·n`. Para el jugador `k` se usa `n = k+3`, es decir `cfg + 1572 + 32·k` (`18.cpp:8671`, `r31 = X+4944+3`) [C, igual que doc 12 §5].

c) **Caché o búsqueda por uso:**
- La lista se consulta por ID en cada llamada a `sub_820F3980` (lo que cambia por ID es la tabla, no hay caché de la lista) [C].
- El **manejador** del banco se guarda en el canal y se carga al llamar. Quién llama y con qué ID, en cada evento de voz: [N] en la mayoría de llamadores (sus llamadores son indirectos).
- Camino de inicio confirmado: `sub_82110640` (`26.cpp:7661`) → `sub_82109638` (`25.cpp:7574`) → `sub_821114F8` (`6.cpp:7955`) → `sub_820FB0D0` (`7.cpp:6760`) → `sub_82117660` (`20.cpp:8224`) → `sub_820F3980` (`20.cpp:8322`) [C].
- En `sub_82117660` el ID de voz no sale de `X+7420`: sale de `X+7416` (puntero a `char372`), de `char372 +212+20·f` (ID de la forma en +214, índice en +216) y de `char96.forms[].id` (+16) [C]. Usa `X+2980` (cuántos miembros/formas) y `X+2984` (miembro actual) [C].
- Consecuencia [I]: para que la voz cambie, hay que repetir la cadena de inicio (o `sub_82117660`) con el nuevo ID y los nuevos punteros de forma.

d) **Liberación:** no encontrada [N].

Otros llamadores de `sub_820F3980` (todos con llamadas indirectas arriba): `sub_8211EED0` (`18.cpp:8671`), `sub_8211EF60` (`10.cpp:8659`), `sub_820F3AC0` (`11.cpp:6283`), `sub_821209D8` (`35.cpp:8705`), `sub_82120A90` (`37.cpp:8812`) [C]. Su raíz: [N].

---

## 4. Labios (lip-sync / LIPS) [C salvo lo indicado]

a) **Lector:** `sub_82136D28` (`21.cpp:9964`).
- Lee el byte de labios de la forma: `char96 + 16 + 8·f + 3` (`lbzx` con `+19`) [C]. Si vale `-1` no hay labios.
- Combina ese byte con el byte `char96 +6` del personaje, multiplicado por `r5` (modelo/traje) [I]. El resultado es el índice dentro de la lista LIPS (`char96 +12`, 8 B: fid, caché).
- La lista LIPS (`char96 +12`) no se ha seguido hasta el archivo [N].

b) **Dónde se guarda:** en el registro de forma de 40 B (`X+7608+4f`), en `+4` (`stw r3,4(r31)` en `sub_82133B88`, `33.cpp:9983`) [C].

c) **Caché o búsqueda por uso:** **caché por forma**, al montar la forma:
- `sub_82100A10` (`23.cpp:7273`) → `sub_82133B88` (`33.cpp:9983`) → `sub_82136D28` (`21.cpp:9964`) [C].
- `sub_82100A10` lo llama `sub_82109148` (`30.cpp:7533`, tres llamadas) y `sub_8210E100` (`29.cpp:7737`) [C]. Esto es lo que recarga los labios al cambiar de forma o de luchador por ese camino.
- Un cambio de `X+7420` sin repetir `sub_82100A10` deja los labios viejos [I].

d) **Liberación:** `sub_82100EB8` (`28.cpp:7057`) y `sub_82100F30` (`3.cpp:7159`) recorren las 8 formas y llaman a `sub_82100D60` (`4.cpp:7136`) o `sub_82100DE8` (`30.cpp:7024`) para las que no se usan [C]. Que esas dos liberen el registro de 40 B: [I, por el nombre y el orden].

---

## 5. Cara de la barra de vida (HUD) [C salvo lo indicado]

a) **Lector:** `sub_821B9F90` (`28.cpp:17500`).
- Lee `cara[ID]` (u16) en `0x82373D68 + 2·ID` (`lhax`, `rlwinm r9,r5,1`) [C]. El ID es `r5`.
- También la lee `sub_821B6B98` (`5.cpp:17141`, `lhax` en la misma tabla) [C].

b) **Dónde se guarda:** en una estructura de cara por casilla de 460 B (`mulli r11,r30,460`) y en el puntero de HUD `*(0x8232 -26852)` [C/I]. No se ha seguido el campo final.

c) **Caché o búsqueda por uso:** **por llamada.** La cara no se cachea por ID: cada vez que se llama `sub_821B9F90`, se busca `cara[ID]` con el ID que pasa el llamador [C].
- Llamadores (todos con el ID en `r5`): `sub_82141C08` (`0.cpp:10573`, `r5 = r30`), `sub_82182210` (`1.cpp:13698`, `r5 = r31` en `1.cpp:14034`), `sub_8215AD98` (`11.cpp:12172`), `sub_82180AA0` (`15.cpp:13916`), `sub_821C3FD0` (`18.cpp:18029`, `r5` de un byte de la estructura) [C].
- `sub_82182210` es una función grande de menú/HUD (990 líneas) [I]. Si la barra de vida se refresca en el cambio de luchador, depende de cuál de estos llamadores se use [N].

d) **Liberación:** no encontrada [N].

---

## 6. Cápsulas y habilidades [I salvo lo indicado]

- **Lista por defecto del personaje:** `char96 +80` (u16 n + 7 u16). La lee `sub_82182210` (`1.cpp:13698`) [C].
- **Cápsula de cada forma:** `char372 +212 + 20·f` (ID de la cápsula/forma en +214, índice en +216). Se lee desde `X+7416` (puntero a `char372`), que escribe `sub_82109BA0` (`21.cpp:7374`) [C, `sub_82100A10` y `sub_82117660` usan estos campos].
- **Lista de cápsulas por forma en el estado del combate:** `X+8156+f`, escrita por `sub_82112260` (`28.cpp:8056`) [I, doc 12].
- **Nombres de cápsula:** tabla `0x82373D68` (u16 por ID), la misma que la cara HUD [C por el acceso; el uso como nombre es [I]].
- **Catálogo #SKC:** punteros `0x82375608` (cabecera) y `0x8237560C` (registros, +0x20) [C, doc roster].
- **Cacheado o por uso:** la lista se fija al montar cada forma. Su lectura posterior usa `X+7416`, así que cambiar `X+7416` cambia lo que se lee, pero la lista de `X+8156+f` no se recalcula sola [I].

---

## 7. Qué recarga el cambio de forma (camino de `sub_8210E100`)

`sub_8210E100` (`29.cpp:7737`) es el camino de cambio en caliente. Llama a `sub_82103978` (`33.cpp:7146`), `sub_82119108` (`15.cpp:8286`), `sub_82100EB8`, `sub_82100F30` y `sub_82100A10` [C]. Desde ahí, el grafo llega a:
- `sub_82100A10` → `sub_82133B88` → `sub_82134640` (modelo), `sub_82136D28` (labios), `sub_82136C58` (aura de forma) [C].
- Además `sub_82100B78` (`10.cpp:6961`): carga el ANM de la forma en `X+7672+28·f+20` [C, doc 12 §3.1].

**No llega** (por el grafo estático) a: `sub_820B8F78` (aura por ID), `sub_82128040` (técnica), `sub_820F3980` (voces), `sub_821B9F90` (cara). El aura se alcanza desde `sub_820CD5D8`; la técnica desde `sub_820CD5D8` (solo ID 35) o desde el callback `sub_82128B08`; las voces desde `sub_82110640` [C].

---

## 8. Resumen de la cadena de inicio vs. cambio

Inicio de combate (todo se carga una vez por jugador):
1. `sub_82109BA0(sel)` fija `X+7420/7416/7412` para los 4 jugadores (`21.cpp:7374`), llamado desde `sub_8210E468` (`7.cpp:7749`, llamada en `7.cpp:7773`) [C].
2. `sub_82109148(X_k, rec_k)` monta cámara, idioma, ANM y formas (`30.cpp:7533`) [C].
3. `sub_82110640` (`26.cpp:7661`): reset por jugador, voces, cápsulas, CCM [C, doc 12 §6].
4. `sub_820CD5D8` (`3.cpp:4197`): aura por ID (`sub_820BA578`) y técnica solo para el caso especial ID 35 (`sub_82128338`) [C]. El caso normal de la técnica va por el callback `sub_82128B08` (quién lo encola: [N]).

Cambio de forma/luchador (`sub_8210E100`, `29.cpp:7737`): recarga modelo, labios y aura de forma. No recarga aura por ID, técnica, voces ni cara [C por grafo].

---

## 9. Qué no se ha podido confirmar [N]

1. Quién encola el trabajo que llama a `sub_82128B08` (callback de técnica normal) [N]. Es lo más importante para saber si el cambio mid-batalla ya recarga la técnica.
2. Quién llama a `sub_820CD5D8`, `sub_8210E100`, `sub_8210E468` y las funciones de creación `sub_821C24B8`/`sub_821C6BB8`/`sub_821D5260`/`sub_821D65B8`/`sub_821D74A0`/`sub_821D78F8`. Son llamadas indirectas (tabla de funciones) [N]. Sin saberlo, no se sabe qué camino corre al cambiar de luchador en batalla.
3. Si el cambio mid-batalla (Tag Team u otro) pasa por `sub_820CD5D8` o `sub_82110640` otra vez. Si lo hace, aura, técnica y voces se recargan solas; si no, hay que llamarlas [N].
4. Liberación de `G+420·k` (aura), `G2+1152·k+624` (técnica) y del canal de voz (`cfg+1572+32k`). No se han encontrado funciones de liberación específicas [N].
5. Semántica exacta del índice de labios (`byte +19` + `byte +6`·modelo) y de la lista LIPS (`char96 +12`) [I].
6. Estructura de la cara HUD (460 B por casilla) y su campo de textura final [N].
7. Qué lee `sub_82100A10` del `char372 +212` (la variable `r10` se calcula pero no se pasa en las llamadas vistas) [I].

---

## 10. Cómo reproducir (para quien siga)

Para cambiar el luchador de un jugador `k` en batalla, según este análisis, habría que:
1. Escribir `X_k+7420`, `X_k+7416` (`char372` del ID nuevo) y `X_k+7412` (registro de selección) — ya lo hace `sub_82109BA0`, que va sobre los 4 jugadores [C].
2. Repetir `sub_82109148(X_k, rec_k)` para cámara, idioma, ANM y formas (incluye aura de forma y labios) [C].
3. Repetir la parte de voces de `sub_82110640`/`sub_82109638` para ese jugador (voces) [I].
4. Recalcular aura por ID (`sub_820B8F78`) y técnica (`sub_82128040`) para ese `k` con el ID nuevo, escribiendo en `G+420·k` y `G2+1152·k+624` [I, no probado].
5. Cara HUD: volver a llamar `sub_821B9F90` con el ID nuevo en la casilla correspondiente [I].

Ninguno de los puntos 3 a 5 está probado en juego.
