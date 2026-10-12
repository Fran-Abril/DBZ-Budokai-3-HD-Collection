# Estudio 19: cara de la barra de vida en combate (ruta real)

Análisis estático, sin tocar fuentes. Etiquetas: [C] comprobado en el código recompilado, [I] inferido, [N] no comprobado.

Corrige lo dicho en `17_cara_hud.md` y `14_recursos_por_id.md` §5: `sub_821B9F90` es la cara del **menú de selección**, no la de combate (medido: no se llama en combate).

## Resumen

- La cara de combate se carga **una vez al empezar el combate**, por jugador de la lista de combate. [C]
- Cadena: `sub_820DBD88` (despacho por ID) -> `sub_820E6D70` (carga de caras). [C]
- El ID que decide la cara es el del **registro de combate** `B + 3888 + 80·i` (u16 en `+16`, traje en `+18`), no `X+7420`. Por eso cambiar `X+7420` no cambia la cara. [C lectura / I causa]
- Tabla de caras: `0x82324468` (filas de 16 B, termina en ID -1). Cara = `fila+8`, emparejada por ID y bit de traje. [C]
- La textura por forma va dentro del mismo archivo (`*_HUD.amt`, una textura por forma). El cambio de forma no recarga archivo si el dibujo usa el índice de forma. [I]
- Dónde se dibuja la cara en pantalla: **no encontrado** [N].

## Punteros y direcciones

| Dato | Dirección / cálculo | Estado |
|---|---|---|
| `B` = estado de combate | `*(0x82324064)` (`lis -32206` + `16484`) | [C] |
| Registro de combate i | `B + 3888 + 80·i` (`sub_820DB710`) | [C] |
| ID del registro i | u16 en `B + 3904 + 80·i` (`lha 16`) | [C] |
| Traje del registro i | u16 en `B + 3906 + 80·i` (`lha 18`) | [C] |
| ID del personaje cargado (despacho) | u16 en `B + 4208` (`sth` en `sub_820DBD88`) | [C] |
| Nº de registros | `*(cfg + 35096)`, `cfg = *(0x823D6CF4)` | [C] |
| Tabla de caras | `0x82324468`, filas de 16 B: `+0` ID (u32), `+4` máscara de trajes, `+8` fid de cara (data_cmn), `+12` índice | [C] (formato coincide con `CAPSULAS_B3.md:45`) |
| **Handle de cara del registro i (escritura)** | `0x823CE000 + 4·i` (`stw r3,0(r25)` tras `sub_82083968`) | [C] |
| Handle de cara (liberación) | `0x823CE010 + 4·i` (`sub_820E7058`) | [C] **discrepancia de 16 B** (ver abajo) |
| Puntero auxiliar por registro | `B + 4428 + 4·i` (`stw r3,0(r27)`) | [C] escritura / [N] significado |

Discrepancia: `sub_820E6D70` escribe el handle en `0x823CE000+4i`, pero `sub_820E7058` lo libera desde `0x823CE010+4i`. Si la lectura es correcta, la liberación no libera el handle que cargó la función y deja los demás sin liberar (posible fuga o cara equivocada al recargar). Comprobar en juego antes de reutilizar la liberación. [C código / N efecto]

## Flujo de carga de la cara de combate

1. `sub_820DBD88` (`36.cpp:5133`): llamada por tabla de funciones (no hay `bl` directo; entrada indirecta). Pone a cero el bloque de combate (4496 B), copia ID y datos del personaje a `B+4208/4210/4212`, despacha por ID con la tabla `0x8201DB94 + 4·ID` (`bctrl`), y al final llama `sub_820E6D70`. Devuelve en `r3` el último handle de cara. [C]
2. `sub_820E6D70` (`28.cpp:5514`): recorre `i < *(cfg+35096)`:
   - `r3 = sub_820DB710(i)` = registro de combate `i`.
   - Busca la fila de `0x82324468` cuyo ID = `reg+16` y cuyo bit `(reg+18)` está en la máscara de trajes `+4`.
   - Si `fila+8 != -1`: `h = sub_82083968(fila+8, 0, 1)` (sin el flag `0x10000` que usa la selección). [C]
   - Guarda `h` en `0x823CE000+4i` y devuelve el último `h` no nulo. [C]
   - Lee la fila de sub_821B62D0 (registro `0x82373A00[índice+1]`, ficha de habilidades): dato de fichas, no de la cara. [C]
3. Liberación: `sub_820E7058` (`35.cpp:5494`), que llama `sub_82083A68` sobre el handle y pone a 0. [C]

## Por qué no cambia con `X+7420`

- `X+7420` (ID de la ficha de combate, `X = cfg+2096+8240·k`) no aparece en la cadena de la cara. [C]
- La cara se lee de `B+3904+80i` solo al empezar el combate. Cambiar `X+7420` no toca `B+3904`. [C] / causa [I]

## Candidatos descartados (no están en la ruta de combate)

| Función | Motivo |
|---|---|
| `sub_821B9F90` (`28.cpp:17500`) | Llamadores solo en selección/editor; medido: no se llama en combate. [C] |
| `sub_821B6B98` (`5.cpp:17141`) | Carga **una sola vez** un handle global en `0x82479724` (solo si era 0) con fid `0x10000|ID`. Si fuera la cara de combate, no cambiaría nunca. Medido: no se llama en combate. [C] |
| Panel `0x8247971C + 460·k` | Todas las funciones que usan el paso 460 están en `0x821B6xxx–0x821B9xxx` (menús). [C] |
| `sub_82128338` / `sub_820CD5D8` (aura, técnica) | Carga aura y técnica por ID de selección; no carga caras. [C] |
| `sub_82100B78` (`10.cpp:7022`) | Carga por tabla `X+7416` (descriptor de forma), no por cara. [C] |

## Tablas por ID usadas en combate (para el mapa de carga)

- `0x82324468` (16 B por fila): cara (`+8`), trajes (`+4`), índice (`+12`). Lectura en `sub_820E6D70`. [C]
- `0x82373A00[índice+1]`: ficha de habilidades (`sub_821B62D0`). [C]
- `0x82322950` (u32, aura) y `0x82333208` (u32, técnica): ver docs 14. [C]
- `0x82373D68` (u16 por ID, nombres de cápsulas y cara del menú): solo lo leen `sub_821B9F90` y `sub_821B6B98`. [C]

## Cómo cambiar la cara del jugador k en combate (propuesta, sin probar)

Opción A (recomendada, mínima): tras escribir el ID nuevo en `B+3904+80k` (y el traje en `+18`):
```
fila  = buscar en 0x82324468 (ID nuevo + bit de traje)
viejo = u32 @ 0x823CE000 + 4k ;  si != 0: sub_82083A68(viejo)
h     = sub_82083968(fila+8, 0, 1)
escribir h en 0x823CE000 + 4k
```
- Riesgo: si el dibujo lee el handle de otra copia (ver [N] abajo), la cara no cambiará. [N]
- Antes de escribir: resolver la discrepancia de 16 B.

Opción B: reejecutar `sub_820E6D70` completo. Carga todas las caras y no libera las anteriores (fuga). Solo para pruebas. [I]

Para forma: si el dibujo toma la textura por índice de forma (ver [N]), basta con cambiar ese índice; no hace falta archivo nuevo. [I]

## Pendiente (para confirmar en juego o con hook)

1. Quién lee el handle de `0x823CE000+4i` (o el valor devuelto por `sub_820DBD88`) para dibujar: [N].
2. Qué índice de textura usa el dibujo para la forma (X+2980 o el registro `i`): [N].
3. Resolver la discrepancia `+0` / `+16` (escritura `sub_820E6D70` frente a liberación `sub_820E7058`): [N].
4. Hook de entrada a `sub_820E6D70` en combate: registrar `B`, `count`, ID y traje de cada registro y el handle devuelto. [N hasta hacerlo]

## Construcción de la textura

Pasada estática (solo lectura de `generated\`). Lo que sigue corrige o confirma lo anterior.

### Escritura y liberación de los handles [C]

- `sub_820E6D70` (`28.cpp:5514`): primero pone a cero 1600 B desde `0x823CD9C0` (`28.cpp:5542`; esto llega hasta `0x823CE000`, que queda fuera). El handle del registro `i` se escribe en `r25 = 0x823CE000 + 4i` (`28.cpp:5676`, `stw r3,0(r25)`). Si no hay fila o `fila+8 == -1`, el slot **no se toca**: conserva el valor de la batalla anterior.
- `sub_820E7058` (`35.cpp:5455`): libera y pone a cero `0x823CE010 + 4i` (`35.cpp:5493-5496`: `addi -8192` + `addi 16`; libera en `5504-5506`, cero en `5511`).
- **Discrepancia confirmada**: con 2 jugadores libera `0x823CE010` y `0x823CE014`, no los que escribe el combate (`0x823CE000`, `0x823CE004`). Esos dos no se liberan ni se ponen a cero, así que se pierden en cada batalla. [C código] [I efecto: fuga]
- Además, `sub_820E7058` libera `*(B+4428+4i)` con `sub_821B63F8` (`35.cpp:5520`) y pone a cero `B+4376+4i` (`35.cpp:5515`), no `B+4428+4i`. [C]
- Para liberar la cara que escribe `sub_820E6D70`: usar `0x823CE000 + 4k` (Opción A corregida).

### Nodo que devuelve `sub_82083968` [C]

- `27.cpp:102`: toma un registro de un pool de 380 B (tabla en `-32200` + `25568`, `27.cpp:184-189`).
- Guarda `fid` en `+196` (`27.cpp:198`), el argumento `r4` en `+212`, y en `+192` deja `0xFFFFFFFF` si `r5 != 0` (`27.cpp:194-209`; en la llamada `r5=1`). Devuelve el nodo en `r3` (`27.cpp:236-237`).
- Copia el nombre del archivo en `+6` (`27.cpp:192`, `sub_822F6260`). No parsea `#AZT`: el nodo no contiene aún la textura. [C]

### Lector de la cara (dibujo) [N]

- Desplazamientos `-8176` / `-8192` (base `0x823D0000`): solo aparecen en `35.cpp:5493-5494` (la liberación). No hay lector directo de `0x823CE000`. [C]
- El escritor llega por `-9792` (`0x823CD9C0`) + `1600`. Ningún lector localizado con esa base. [N]
- Llamadores de `sub_820DB710` (`4`, `26`, `32`, `35`, `37`): no leen el handle. `sub_820E0618` (`35.cpp:5236`) lee flags del registro (`+26`) y llama `sub_820DF588` (`17.cpp:5127`, flags de movimiento, no HUD). [C]
- Único llamador de `sub_820E6D70`: `sub_820DBD88` (`36.cpp:5238`). Su retorno (último handle, `r23`) no tiene consumidor localizado. [N]

### Forma (Goku, 6 texturas) [N]

- No localizado el índice de forma ni el dibujo que lo usa.

### Conclusión [I]

- El HUD no lee `0x823CE000+4k` en cada frame, o copia el handle a un objeto propio al empezar el combate. Esto encaja con la medida en juego (cambiar el handle a mitad no cambia la cara).
- Por tanto, la "Opción A" de arriba no está validada: reemplazar el handle no bastará.

### Procedimiento mínimo (propuesta, sin probar)

1. Hallar el lector: vigilancia de lectura (read-watch) sobre `0x823CE000..0x823CE01F` durante el combate; anotar el PC lector (`0x8xxxxxxx`) y buscarlo en `generated\`.
2. Con el lector: ver si toma el handle y crea la textura, y dónde guarda el objeto por jugador.
3. Reconstruir la cara de `k`: `h = sub_82083968(fid_nuevo, 0, 1)`; liberar el objeto viejo del HUD (no el handle de `0x823CE010`); escribir el nuevo. Para la forma, cambiar el índice que use el lector.
