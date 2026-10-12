# Estudio 11: carga del luchador (ID, lista de movimientos y recursos por personaje)

Fecha: 2026-10-10. Solo lectura de `generated\dbz3_recomp.N.cpp`, `src\roster_ext.cpp` y `docs\`. No se ha editado código ni ejecutado el juego.

Convención: las líneas son las del `DEFINE_REX_FUNC` salvo que se indique otra.
- **[C]** = comprobado leyendo la instrucción en el código recompilado (o el comentario de `roster_ext.cpp`, que es una tabla ya medida).
- **[I]** = inferido por el uso, el orden o el nombre. **[N]** = no confirmado.

Bloques usados:
- `cfg` = `*(u32*)0x823D6CF4` [C] (`lis -32195` + `lwz 27892`, p. ej. `dbz3_recomp.33.cpp:7773-7778`).
- `X_k` = `cfg + 2096 + 8240·k` (k = jugador 0..3).
- `sel` = bloque de selección de batalla = `cfg + 1680` [C por `src\roster_ext.cpp:1079-1111`; el código de la copia usa `rec >= cfg+1696`].
- `rec_k` = `sel + 16 + 80·k` = `cfg + 1696 + 80·k`. El ID del personaje está en `rec_k + 16` (= `sel+32+80k`) [C].
- `char372` = tabla de registros de combate por ID, base `0x82329CF0`, 372 bytes por ID [C, `src\roster_ext.cpp:10`].
- `char96` = tabla de datos por ID, base `0x8234ABB8`, 96 bytes por ID [C, `src\roster_ext.cpp:6-12`].

---

## 1. Quién pone el ID en X+7420, X+7416 y X+7412

**Respuesta corta:** la escribe `sub_82109BA0` (`dbz3_recomp.21.cpp:7374`). Lee el ID de `sel` y rellena los tres campos para los 4 jugadores.

Detalle de `sub_82109BA0` [C]:
- Bucle de 4 jugadores: `li r27,4` en `21.cpp:7391`.
- Lee el ID con `lha -4(r31)`, donde `r31 = sel+36+80k`, es decir `sel+32+80k` (`21.cpp:7409`).
- **X+7420 = ID** (`stw 0(r30)`, `21.cpp:7412`). Con `r30 = X_k+7420`.
- Excepción de ID [C]: si el ID es **78, 79 o 84**, se sobrescribe **X+7420 = 35** (`21.cpp:7511`, `stw r28=35`). Los IDs impares 65, 67, 69, 71, 73 no cambian el ID, pero ajustan `rec+20` y `rec+22` (`sel+36`, `sel+38`) a un mínimo de 1 (`21.cpp:7418-7470`) [C]. Qué significan esos dos campos [I]: forma mínima y máxima.
- **X+7412 = rec_k** = `sel+16+80k` (`21.cpp:7522`).
- **X+7416 = char372 + 372·ID** (`21.cpp:7528`).
- Después, por jugador: `sub_82113818` (`21.cpp:7531`), `sub_82113870` (`:7536`), `sub_82112260` (`:7541`). Al final, una vez: `sub_82101240` (`:7554`). Los nombres de estas funciones vienen de `docs\10_tag_team\02b_tag_team_fase1.md`; su papel exacto no se ha seguido aquí [I].

**Quién llama a `sub_82109BA0`:** solo `sub_8210E468` (`dbz3_recomp.7.cpp:7773`) [C].

**Quién llama a `sub_8210E468`:** no hay ninguna llamada directa en `generated\`. Sí está registrada en la tabla de funciones (`dbz3_register.cpp:2553`, `dbz3_init.cpp:2571`). Por eso el motor la llama por dirección (tabla), no por `bl` [I]. Desde qué estado se llama en combate: [N].

`sub_8210E468` [C] (`dbz3_recomp.7.cpp:7749`):
1. Llama a `sub_82109BA0(sel)` (`7773`).
2. Bucle por jugador k: `stw k → X+4944` (`dbz3_recomp.7.cpp:7792`; X+4944 = índice del jugador k [C]). Después llama a `sub_82109148(X_k, rec_k)` (`7799`).

**Origen del ID (de la selección de personajes):** la copia `sel` viene de la pantalla de selección. `sub_820FE668` (`dbz3_recomp.5.cpp:6912`) lee el ID en `sel+32+80k` para cada casilla (`src\roster_ext.cpp:1089-1104`). Sus llamadores: `sub_821D7400`, `sub_82156CE8`, `sub_820FFA80` [C]. Qué función escribe el ID en la pantalla de selección: [N].

**Qué NO escribe la rutina de inicio de combate:** `sub_82110810` no escribe X+7420 ni X+7416 ni X+7412. Solo usa los valores ya puestos por `sub_8210E468`.

---

## 2. Cadena de inicio de combate (`sub_82110810`, `sub_82110640`, `sub_82109638`)

Cadena de llamadas [C]:
- `sub_820FEA98` (`dbz3_recomp.30.cpp:6896`) → `sub_82110810` (`:6963`).
- `sub_82110810` (`dbz3_recomp.2.cpp:7837`) → `sub_8210E4F8` (`:7850`) → `sub_82110640` (`:7853`).

`sub_8210E4F8` (`dbz3_recomp.33.cpp:7761`) [C]:
- Lee el número de luchadores de `cfg + 35096` (`0x8918`; `33.cpp:7779-7786`) [I: es el número de jugadores].
- Bucle 1 (por jugador): `sub_82100EB8(X,-1)` y `sub_82100F30(X,-1)`. Luego `sub_821171B8`.
- Bucle 2 (por jugador): `sub_82112C60(X,1)`, `sub_82112D30(X)`, `sub_82111598(X)`, `sub_82111620(X)`. Al final, `sub_82092A90(32, 0x40000000, 1)` (`:7872`).

`sub_82110640` (`dbz3_recomp.26.cpp:7661`) [C]:
- Bucle 1: por jugador, `sub_82109638(X,0)` (`26.cpp:7704`), luego `sub_82100EB8(X,-1)` y `sub_82100F30(X,-1)`.
- Bucle 2: `sub_82114650(X,1.0)`, `sub_82110CA0(X)` (`26.cpp:7757`), `sub_821109D0(X,0)`, `sub_82110AB8(X,-1)`, `sub_82110B08(X)`, `sub_82138E88(X+2948)`, `sub_82113118(X)`, `sub_820CB0A0(X+1616, ...)`.
- Al final: `sub_821177E8`, `sub_821171B8`.

`sub_82109638` (`dbz3_recomp.25.cpp:7574`) = reset del luchador [C]:
- Lee `rec = X+7412` (`25.cpp:7609`). Escribe 4 floats en `rec+0`, `+4`, `+8`, `+12` (`25.cpp:7650-7670`).
- Lee `X+4944` (`25.cpp:7629`) para elegir un valor base.
- Lee `rec+32` (la línea `lhz 32(r11)` de `25.cpp:7683`, con `r11 = rec`) y guarda el bit bajo en `X+8185` (`25.cpp:7687`).
- **X+6048 = r30** (`25.cpp:7849`). Es la entrada de movimiento actual. Se escribe con el valor de r30, que no se ha seguido hasta su origen [C escritura / [N] valor].
- **X+5444 = X+7908** (`25.cpp:7846-7853`). Este es el puntero a la lista de movimientos (CCM), ver sección 3 [C].
- Llama a `sub_820F7868(X+4960)` (`25.cpp:7856`; función en `dbz3_recomp.19.cpp:6391`) [I: usa el CCM].

---

## 3. Lista de movimientos (CCM), y qué campo la guarda

**Dónde vive el CCM:** dentro del archivo de cámara `#AMB` del personaje. Su hijos son `#ACC`, `#ACL`, `#CCM` y `#SPX` [C, `docs\07_ports\GOHAN_FUTURO_EXPLORACION_2026-10-06\05_studio\informe.md:42`].

**Cómo llega a la memoria:**
1. El ID de archivo de cámara está en `char372 + 0x20` (CAM) [C, `src\roster_ext.cpp:10` y escritura en `:419`].
2. `sub_82109148` (`dbz3_recomp.30.cpp:7533`) lee `char372+32` (`30.cpp:7552`) y llama a `sub_82083968` (carga de archivo por ID). El resultado va a **X+7916** (`30.cpp:7565`) y **X+7928** (`:7567`) [C].
3. `sub_8210FDE0(X)` (`dbz3_recomp.27.cpp:7784`) lee el handle de X+7916 (`27.cpp:7796`) y guarda la raíz del `#AMB` en **X+7896** (`27.cpp:7819`). Luego extrae hijos con `sub_820B0CF8(raíz, i)` (helper en `dbz3_recomp.15.cpp:2547`):
   - i = 0 → **X+7900** (`27.cpp:7840`).
   - i = 2 → **X+7908** (`27.cpp:7852`).
   - i = 3 → **X+7912** (`27.cpp:7864`).
   La tabla de hijos tiene 16 bytes por entrada (`sub_820B0CC8`, `dbz3_recomp.15.cpp` cerca de 2547) [C]. Que i=2 sea `#CCM` y i=3 `#SPX` [I: orden de `docs`; el hijo i=1 `#ACL` no se toca].

**Campo que guarda la lista de movimientos: X+7908** [I]. Evidencia [C]:
- `sub_82109638` copia X+7908 a X+5444 (`25.cpp:7846-7853`) y lo pasa a `sub_820F7868`.
- X+6048 (la entrada de movimiento actual) se escribe desde varias funciones (`25.cpp:7849`, `4.cpp:7927`, `4.cpp:8068`, `42.cpp:7563`). Ninguna de ellas se ha seguido hasta la lista de combos [N].

**Lo que NO es la tabla global `0x8237560C`:** la función `sub_82112088(X, techId)` (`dbz3_recomp.39.cpp:7997`) lee la tabla `*(0x8237560C)`, que es el **catálogo de capsulas `#SKC`** (40 bytes por entrada, 596 entradas) [C, `src\roster_ext.cpp:115-118`]. No es la lista de movimientos del CCM. La cuenta de matches se hace contra la lista de capsulas del propio registro `rec_k`: `u16 n` en `rec+52` y `n` IDs en `rec+54` (`39.cpp:8100-8124`) [C].
- `rec+52/+54` lo rellena `sub_821007E8` con la copia de `char96+80` (`src\roster_ext.cpp:1111-1116`) [C].

---

## 4. Otros recursos por personaje cargados al inicio de combate

| Recurso | Campo X | Cargador (archivo:línea) | Estado |
|---|---|---|---|
| Modelo por forma `f` | `X+7608+4f` (handle en caché) | `sub_82100A10` (`23.cpp:7273`). Lee `char372 + 212 + 20f`: +2 ID de modelo, +4 índice. Traje en `X+7436+f`. Jugador en `X+4944`. Llama a `sub_82133B88` (`33.cpp:9983`) | [C] |
| Formas iniciales (min/max) | (`rec+20`, `rec+22`) | `sub_82109148` llama a `sub_82100A10` con esas formas (`30.cpp:7604-7640`) | [C] |
| Archivo de cámara `#AMB` | `X+7916` | `sub_82109148` (`30.cpp:7552-7565`) con `char372+0x20` | [C] |
| Hijos del `#AMB` (ACC, CCM, SPX) | `X+7900`, `X+7908`, `X+7912` | `sub_8210FDE0` (`27.cpp:7784`) | [C] código, [I] nombres |
| Archivo de idioma (+0x28) | `X+7924` (y X+7928 como último) | `sub_82109148` (`30.cpp:7585-7587`) | [C] |
| Archivo de idioma (+0x24) | no localizado | - | [N] |
| Voces de combate | canal de voz `cfg+1572+32k` (k = `X+4944`) | `sub_82117660` (`dbz3_recomp.20.cpp:8224`). Llama a `sub_820F3980` (`:8322`) | [C] |
| ID de voz | `char96[ID_modelo].forms[idx].id` | mismo cálculo en `20.cpp` (`lhax` sobre `0x8234ABB8+16`) | [C] |
| Tablas de voz | `0x823280B0` (puntero por ID), `0x82328298` (número por ID) | `sub_820F3980` (`dbz3_recomp.30.cpp:6253`) | [C] |
| Labios (lip-sync) | `char96+12` (lista LIPS) y `forms[].lips` | no localizado | [N] |
| Cara del HUD | tabla `0x82373D68` (2 bytes por ID) | `sub_821B6B98` (`5.cpp:17175`), `sub_821B9F90` (`28.cpp:17635`). Sin campo X; se usa el ID | [C] tabla, [I] uso |
| Capsulas equipadas | `X+8156+f` | `sub_82112260` (según `02b`, no re-verificado aquí) | [C según 02b] |
| Aura y BSP de técnica por ID | `0x82322950`, `0x82333208` | no seguido aquí | [I] (tabla de `src\roster_ext.cpp:12-13`) |

---

## 5. ¿Existe una rutina que cambie de personaje a mitad de combate?

**No se ha encontrado.** [C]
- El único escritor de X+7420, X+7416 y X+7412 es `sub_82109BA0` (`21.cpp:7412, 7522, 7528`).
- Solo la llama `sub_8210E468` (`7.cpp:7773`). Esa función no tiene llamadas directas (solo la tabla de funciones). Se desconoce quién la invoca en combate [N].
- `sub_82109638` (reset) solo la llama `sub_82110640` (`26.cpp:7704`). `sub_82110640` solo la llama `sub_82110810` (`2.cpp:7853`). `sub_82110810` solo la llama `sub_820FEA98` (`30.cpp:6963`).
- `sub_820FEA98` la llaman `sub_820FF750` (`11.cpp:7050`), `sub_820FFFC8` (`32.cpp:7657`) y `sub_820FF1B8` (`33.cpp:7021`). Qué modo es cada una: [N].
- `sub_8210E4F8` también la llama `sub_820FE9B0` (`4.cpp:7013`). Esta última la llaman 6 funciones (`sub_821D5C38`, `sub_821D6718`, `sub_820FEBA8`, `sub_821D4478`, `sub_821D42D8`, `sub_821D43A8`). Esa ruta **no** vuelve a escribir el ID [C].

**Cambio de forma:** `sub_8210E100` (`dbz3_recomp.29.cpp:7737`) corre cada frame (registrada en `dbz3_register.cpp:2550`) [I]. Lee la forma actual X+2980 y la siguiente X+2984 (`29.cpp:7788`, escribe X+2984 en `:7801`) y llama a `sub_82100A10` (`:7827`). Una entrada de forma puede apuntar al modelo de **otro ID** (`+2` de la entrada de forma, usado en `sub_820F9970`, `22.cpp:6490`) [C, según `02b`].

**Conclusión:** para cambiar de personaje a mitad de combate habría que volver a ejecutar `sub_82109BA0` para ese jugador (ID, X+7416, X+7412), rehacer el reset (`sub_82109638`) y recargar los recursos de la sección 4. No hay una rutina existente que lo haga [I].

---

## 6. Correcciones a `02b_tag_team_fase1.md`

- Confirmado [C]: X+7420 solo se escribe en `sub_82109BA0`. La escritura es **indirecta** (con base `X+7420`, `r30`), por eso no aparece como `stw ...,7420(...)` en la búsqueda literal.
- Añadido [C]: X+7420 = 35 para los IDs 78, 79 y 84 (`21.cpp:7511`). El doc anterior no lo tenía.
- Añadido [C]: X+7908 = hijo 2 del `#AMB`, copiado a X+5444 en el reset (`25.cpp:7853`).
- Corregido [C]: la tabla `0x8237560C` que usa `sub_82112088` es el **catálogo de capsulas**, no la lista de movimientos.

---

## 7. Lo que no se ha podido confirmar

1. Qué función escribe el ID en la pantalla de selección (origen de `sel`).
2. Qué modo lanza cada llamador de `sub_820FEA98` y `sub_820FE9B0`.
3. Quién llama a `sub_8210E468` y `sub_8210E100` (solo por tabla de funciones).
4. Que X+7908 sea `#CCM` y X+7912 sea `#SPX` (inferido del orden de `docs`).
5. Lista de labios (lip-sync) y el archivo de idioma +0x24.
6. Qué hace `sub_820F7868` con X+5444.
7. Si el cambio de forma toma el moveset (ANM) del ID propio (`X+7416`) o del ID de la forma.
