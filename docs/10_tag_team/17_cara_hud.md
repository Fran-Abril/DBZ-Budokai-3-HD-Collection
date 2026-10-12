# Estudio 17: cara de la barra de vida en batalla (HUD) y cambio de luchador

Análisis estático, sin tocar fuentes. Etiquetas: [C] comprobado en el código recompilado, [I] inferido, [N] no comprobado.

## Resumen corto

- La cara sale de `sub_821B9F90` (`28.cpp:17500`). Mira el **slot** (casilla 0..38), no el ID de `X+7420`. Cambiar `X+7420` no cambia la cara. [C]
- Cadena de datos: `slot` -> `0x82020618[slot]` (ID) -> `0x82373D68[ID]` (archivo de cara) -> `sub_82083968` (carga). [C]
- `sub_821B9F90` tiene 5 llamadores directos y ninguno es código de forma (`sub_8210E100`, `sub_82109BA0`, `sub_820F9970`). [C]
- Que `sub_821B9F90` sea de verdad la cara de batalla **no está probado**. Lo que escribe (`*0x8247971C + 460*k + 68`) coincide con el panel del editor de cápsulas de la pantalla de selección (ver `docs/03_formatos/CAPSULAS_B3.md:58`). [C el dato, I el uso]
- Recomendación: antes de escribir código, poner un hook de entrada en `sub_821B9F90` durante una pelea y registrar r3, r4, r5, r6 y quién llama. [N hasta hacerlo]

## 1. Quién carga la cara y con qué argumentos

### Firma de `sub_821B9F90` (`28.cpp:17500`) [C]

| Registro | Significado | Cómo se usa |
|---|---|---|
| r3 | valor "crudo" del jugador `r3`; `k = r3 >> 1` (`srawi`) | `k` elige el panel. Se guarda crudo en `P_k+16`. |
| r4 | índice de jugador (guardado en `P_k+24`) | Solo se guarda. |
| r5 | **slot** (casilla de selección, 0..38) | `ID = u16 @ 0x82020618 + 2*slot` (`lhax`). |
| r6 | puntero a inventario (guardado en `P_k+44`) | Solo se guarda. |

Donde `P_k = *(u32)0x8247971C + 460*k + 68` (panel del jugador k). [C]

### Campos del panel `P_k` que toca la función [C]

- `P_k+0`: handle de la cara (archivo cargado). Se libera antes de cargar uno nuevo (`sub_82083D08`). [C]
- `P_k+4`: segundo handle, liberado con `sub_82083A68`. [C]
- `P_k+16`: r3 crudo. `P_k+24`: r4. `P_k+28`: **ID** del personaje. `P_k+44`: r6. [C]
- `P_k+12` se pone a 0. [C]

### Flujo de `sub_821B9F90` [C]

1. Lee `ID = 0x82020618[2*r5]`.
2. Crea siempre un objeto nuevo con `sub_82092908` y lo guarda en `*(base) + 8*(k+3)`. Esto ocurre en cada llamada, también si el ID no cambia. [C en el código; el efecto sobre memoria vieja es [I]]
3. Rellena los campos del panel (`P_k+12/16/24/28/44`).
4. Compara el ID nuevo con `P_k+28` (antiguo). Si son iguales y `P_k+0 != 0`, no recarga. [C]
5. Si cambió, libera `P_k+0` y `P_k+4`.
6. Si `P_k+0 == 0`, carga: `h = sub_82083968(0x10000 | file, 0, 1)`, con `file = u16 @ 0x82373D68 + 2*ID`. Guarda `h` en `P_k+0`. [C]
7. Caso especial: si r3 crudo es 0 y `*(base)+64 != 0`, el archivo es fijo `2663` (0xA67), sin buscar en la tabla. [C]

### Llamadores directos (lista completa por búsqueda de `sub_821B9F90(ctx, base)`) [C]

| Función | Línea | Argumentos conocidos | Quién la llama |
|---|---|---|---|
| `sub_82141C08` | `0.cpp:10672` | r3=0, r4=0, r5=0 (fijos), r6=r29 | `13.cpp:11421`, desde estado con `r3=[[-6180(r30)]+80]` |
| `sub_8215AD98` | `11.cpp:12244` | r3=0, r4=byte `0x824BA136`, r5=byte `0x824A58A0`, r6=`0x824A58C9` | no vista |
| `sub_82182210` | `1.cpp:14042` | r3=r28 (1..3, según byte de `r29+5`), r5=r31, r6=`[r30+120]+1094` | menú / HUD (990 líneas) [I] |
| `sub_82180AA0` | `15.cpp:14172` | r3=r29 (0..2), r5=r31 (índice de bucle, paso 7), r6=r10+1094 | no vista [I] |
| `sub_821C3FD0` | `18.cpp:18471` | r3=0, r4=byte `0x824BA136`, r5=byte de `*(obj+228)+0x2AEF0`, r6=`*(obj+228)+0x2AF19` | `sub_821C2778` -> `sub_8215C728` / `sub_8215D9A8` [I] |

Notas:
- `sub_821C3FD0` (`18.cpp:18029-18593`) recibe el objeto del combatiente en r3. Llega desde `sub_821C2778` (`17931`), que busca en una tabla de 64 entradas. [C el grafo; I que sea HUD de batalla]
- En `sub_8215AD98` y `sub_821C3FD0`, el slot sale de bytes globales/fijos: `0x824A58A0` (slot) y `0x824BA136` (índice de jugador). [C]
- `0x824BA110` es la partida en memoria (`CAPSULAS_B3.md:50`). `0x824BA136` = su byte +38. [C el dato, I el significado]

### Respuesta corta a la pregunta 1

- Caller candidato para la cara de **P1 en batalla**: `sub_821C3FD0` (r3=0). [I]
- Candidatos para P2 y siguientes: `sub_82182210` y `sub_82180AA0` (r3 de 1 a 3). [I]
- Panel por jugador: `*0x8247971C + 460*k + 68`. [C]
- Slot: byte global `0x824A58A0` (en la ruta de `sub_821C3FD0` y `sub_8215AD98`). [C]
- No hay ninguna escritura de `X+7420` que actualice el slot. [I, cf. `docs/10_tag_team/02b_tag_team_fase1.md:18`]

## 2. Formas (transformaciones)

- `sub_821B9F90` no lo llama ninguna función de forma (`sub_8210E100`, `sub_82109BA0`, `sub_820F9970`, `sub_82100A10`). [C]
- No hay archivo de cara por forma en esta ruta. La cara depende solo del ID (tag) del slot. [C por el código]
- Dos formas con el mismo ID comparten cara. Una forma con otro ID cambiará la cara solo si el llamador pasa un slot cuyo tag sea ese ID. [I]
- `docs/10_tag_team/14_recursos_por_id.md` §7 dice lo mismo: el cambio de forma no recarga cara. [C por grafo]
- Para ver si la forma cambia la cara en la práctica hace falta el hook del punto 1. [N]

## 3. Secuencia mínima para recargar la cara de P_k con un ID nuevo

Hay dos opciones. No se han probado en el juego.

### Opción A (recomendada): recargar solo el archivo de cara, sin crear objeto [I]

```
base    = u32 @ 0x8247971C
P       = base + 460*k + 68
newID   = ID nuevo (0..594)
file    = u16 @ 0x82373D68 + 2*newID
old     = u32 @ P+0 ;  si old != 0: sub_82083D08(old, 0); escribir 0 en P+0
old2    = u32 @ P+4 ;  si old2 != 0: sub_82083A68(old2);  escribir 0 en P+4
h       = sub_82083968(0x10000 | file, 0, 1)
escribir h en P+0
escribir newID en P+28
```

- Ventaja: no crea objetos nuevos ni toca `*(base)+8*(k+3)`. [I]
- Riesgo: si el llamador vuelve a correr con el slot antiguo, ve que el ID no coincide con `P+28` y **recarga la cara antigua**. [I]
- Redibujado: no hay bandera de "redibujar" identificada. Las escrituras anteriores (`P+12` a 0) son lo único que parece tocar el estado. [N]

### Opción B: llamar a `sub_821B9F90` con el slot del ID nuevo [I]

```
slot  = u16 @ 0x82020668 + 2*newID      (ID -> slot, tabla de solo lectura)
sub_821B9F90(r3 = u32[P+16], r4 = u32[P+24], r5 = slot, r6 = u32[P+44])
```

- Comprobar antes que `u16 @ 0x82020618 + 2*slot == newID`. [N]
- Ventaja: usa el camino normal, libera bien el handle antiguo. [C]
- Riesgo: crea un objeto nuevo en `*(base)+8*(k+3)` en cada llamada, sin liberar el anterior. Puede haber fuga o efectos raros. [I]
- Riesgo 2: si el llamador vuelve a correr con el slot antiguo, la cara vuelve atrás. [I]

### Qué no hacer

- No escribir `X+7420` como única acción. Esa escritura no pasa por la cara. [C]
- No escribir en `0x82020618` ni en `0x82020668`: son tablas de solo lectura en el port (`docs/01_estructura/HISTORICO_AGENTS.md:3852`). [C]

## Pendiente (para confirmar)

1. Hook de entrada a `sub_821B9F90` en batalla: registrar r3, r4, r5, r6 y el llamador (LR). [N]
2. Confirmar que `*(obj+228)` en `sub_821C3FD0` es `0x8247A9B0` (así el slot sale de `0x824A58A0`). [I]
3. Confirmar si el renderer de la barra de vida lee `P_k+0` o un objeto distinto (`*(base)+8*(k+3)`). [N]
4. Ver qué llamador corre por frame o por cambio de estado, y si vuelve a leer el slot antiguo. [N]
