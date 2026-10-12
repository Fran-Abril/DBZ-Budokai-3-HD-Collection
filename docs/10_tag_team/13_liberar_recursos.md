# Estudio 13: liberación de recursos del luchador (para cambiar de personaje)

Fecha: 2026-10-10. Solo lectura de `generated\dbz3_recomp.N.cpp`, `src\tagteam.cpp` y `docs\10_tag_team\`. No se ha editado código ni ejecutado el juego.
Complementa `11_carga_luchador.md`.

Convención:
- `X` = bloque del luchador (8240 B). `cuerpo1` = `X+1616`, `cuerpo2` = `X+3280`.
- Líneas = `dbz3_recomp.N.cpp:línea` de la función indicada (salvo que se diga otra cosa).
- **[C]** = comprobado en el código. **[I]** = inferido por el uso o el nombre. **[N]** = no confirmado.

---

## 0. Resumen corto

1. Cada recurso tiene **un** liberador claro. El modelo (`X+7608+4f`) se libera en `sub_82100D60`, la animación (`X+7692+28f+20`) en `sub_82100DE8`, la cámara (`X+7916`) y el idioma (`X+7924`) en `sub_821092C8`. [C]
2. Antes de borrar un modelo, `sub_82100D60` **desengancha los dos cuerpos** que lo usan. Es la única protección que se ve. [C]
3. El borrado de modelos (`sub_82133D00`) no tiene contador: el modelo no se comparte. [C] / [I] sobre que no sea caché.
4. La liberación completa del luchador ocurre en el **cierre de combate** `sub_820FE9B0`. Pero la llamada a `sub_8210E5C0` pasa un `r3` que no veo de dónde sale. Esto puede dejar cámara e idioma sin liberar. [N] (ver sección 2).
5. **Reintento:** no lo he identificado. Los inicios de combate (`sub_820FEA98`) no vuelven a cargar cámara ni ID; eso lo hace `sub_8210E468`, cuyo llamador directo no existe en el código. [N]
6. El cambio de forma en combate (`sub_8210E100` -> `sub_8210E690` / `sub_82115270`) ya reemplaza modelos de otro ID, porque una entrada de forma apunta a un modelo de otro personaje. Es el camino más cercano a "cambiar el modelo en caliente". [C] / [I]

---

## 1. Quién libera cada recurso

### 1.1 Modelo por forma `f` (`X+7608+4f`)

| Paso | Función | Dónde | Estado |
|---|---|---|---|
| Carga | `sub_82100A10` -> `sub_82133B88` (crea el modelo) | `23.cpp:7273`; `33.cpp:9983` | [C] |
| Desenganche | `sub_820F9970(cuerpo, -1)` y `sub_820F9F90(cuerpo)` si `cuerpo+1324 == modelo` | `4.cpp:7172-7179` (cuerpo1), `4.cpp:7195-7202` (cuerpo2) | [C] |
| Borrado | `sub_82133D00(modelo)` | `4.cpp:7206-7208` | [C] |
| Puesta a cero del hueco | `X+7608+4f = 0` | `4.cpp:7211-7212` | [C] |

- Dirección del hueco: `r30 = 4*(f+1902) = 7608+4f`. [C] (`4.cpp:7148-7155`).
- Los dos cuerpos se comparan con `X+2940` (= cuerpo1+1324) y `X+4604` (= cuerpo2+1324). [C] (`4.cpp:7160-7188`).
- Si el hueco no es el modelo que tienen enganchado, no desengancha nada y solo lo borra. [C]
- `sub_82133D00` libera en este orden: `sub_821364B8(modelo+28)`, `sub_82136EE0(modelo+4)`, `sub_82134860(modelo+0)` y por último `sub_82089C10(modelo)`. [C] (`20.cpp:9963`).
- `sub_82133B88` (creación) guarda sus sub-objetos en `+0`, `+4` y `+28`. [C] (`33.cpp:9983`).
- Que no haya caché de modelos: `sub_82133B88` construye un objeto nuevo cada vez y no hay contador. [I]

### 1.2 Animación por forma `f` (`X+7692+28f+20`)

- Carga: `sub_82100B78` llama a `sub_82083968` (carga por ID de archivo) y guarda el resultado en `entrada+20`. [C] (`10.cpp:6961`).
- Liberación: `sub_82100DE8(X, f)`, `30.cpp:7024`.
  - Si `entrada+0 != 0` -> `sub_82083A68(entrada+20)` (decremento de contador). `30.cpp:7046-7059`. [C]
  - Si `entrada+0 == 0` y `entrada+20 != 0` -> `sub_82083D08(entrada+20, 0)` (descarga directa). `30.cpp:7083-7085`. [C]
  - Después pone a cero `entrada+0..+20`. [C]
- El significado de `entrada+0` (si es la marca de "cargado por contador") no lo he confirmado: [N].

### 1.3 Cámara `#AMB` (`X+7916`) y su árbol (`X+7896`, `X+7900`, `X+7908`, `X+7912`)

- Carga: `sub_82109148` -> `sub_82083968(char372+32)`, guarda en `X+7916` y `X+7928`. `30.cpp:7533-7567`. [C]
- Los hijos (`X+7896` raíz, `X+7900`, `X+7908`, `X+7912`) se extraen en `sub_8210FDE0`. `27.cpp:7784`. [C]
- Liberación: `sub_821092C8(X)`, `38.cpp:7441`.
  - Lee la raíz `X+7896` (`38.cpp:7453-7456`).
  - Si `X+7916 != 0`: `sub_82083A68(X+7916)` (`38.cpp:7471`) o `sub_82083D08(X+7916, 0)` (`38.cpp:7493`), según la comparación de la raíz (la condición exacta no la he confirmado: [I]). Después pone a cero `X+7896..X+7916`. [C]
  - Los hijos `X+7900/7908/7912` no se liberan por separado: van con la raíz. [I]

### 1.4 Idioma (`X+7924`, `X+7920`)

- Carga: `sub_82109148`. `30.cpp:7585-7587`. [C]
- Liberación en `sub_821092C8`: si `X+7924 != 0`, `sub_82083A68(X+7924)` (`38.cpp:7508`) y pone `X+7920 = 0`; si no, `sub_82083D08(X+7924, 0)` (`38.cpp:7522`). Después pone `X+7924 = 0`. `38.cpp:7505-7522`. [C]
- `X+7920` lo pone a cero en la rama del contador. Su significado no lo he confirmado: [N].

### 1.5 Las 8 formas en bloque (`sub_821092C8`)

- Después de la cámara y el idioma, `sub_821092C8` llama a `sub_82100D60(X, f)` y a `sub_82100DE8(X, f)` para `f = 0..7`, sin saltar la forma actual. `38.cpp:7533` y `38.cpp:7540`. [C]
- La búsqueda de `sub_82100D60` solo deja borrar el hueco si está lleno. Así que no hay riesgo con los huecos vacíos. [C]

### 1.6 Liberación parcial de formas (sin cámara)

- `sub_82100EB8(X, keep)` libera formas `f` si no son: la actual (`X+2980`), el mínimo (`rec+20`), el máximo (`rec+22`) o `keep`. `28.cpp:7057-7119`. [C]
  - Sale sin hacer nada si `X+7420 == 42`. `28.cpp:7075-7078`. [C]
- `sub_82100F30(X, keep)` hace lo mismo con las animaciones (empieza en `f=1`). `3.cpp:7159-7221`. [C]
- Llamadores:
  - `sub_8210E4F8` (inicio de combate, todos los jugadores): `33.cpp:7800` y `7807`. [C]
  - `sub_82110640` (reinicio del luchador): `26.cpp:7711` y `7718`, justo después de `sub_82109638` (`26.cpp:7704`). [C]
  - `sub_82109380` (cambio de forma en combate): `4.cpp:7729` y `7736`. Llamadores: `sub_82111458`, `sub_8211CCD0`, `sub_820FBCF8`, `sub_820FBB38`. [C]
  - `sub_8210E100` (cambio de forma por frame): `29.cpp:7811` y `7818`, luego carga con `sub_82100A10` (`7827`) y `sub_82100B78` (`7834`). [C]

### 1.7 Reset del luchador (`sub_82109638`)

- `25.cpp:7574`. Pone a cero la máscara de formas cargadas `X+7604` (`25.cpp:8229`). [C]
- Copia `X+7908` (raíz del CCM) a `X+5444`. `25.cpp:7846-7853`. [C]
- Llama a `sub_820F63A8`, `sub_82115270`, `sub_82111458`, `sub_82112260` (según `docs\10_tag_team\02b_tag_team_fase1.md`). [I]

---

## 2. Cierre de combate y reintento: en qué orden se libera

### 2.1 Cierre de combate: `sub_820FE9B0` (`4.cpp:6983`)

Entrada: lo llaman seis funciones de menú: `sub_821D5C38` (`11.cpp:19185`), `sub_821D6718` (`12.cpp:18402`), `sub_820FEBA8` (`22.cpp:6831`), `sub_821D4478` (`32.cpp:18570`), `sub_821D42D8` (`40.cpp:18771`), `sub_821D43A8` (`43.cpp:18868`). [C]

Solo actúa si `cfg+27892 != 0` (`4.cpp:6990-7000`). [C] Orden de llamadas:

1. `sub_82120C90(4, 0)` (`4.cpp:7013` cerca). [C]
2. `sub_8210E4F8` -> por jugador: `sub_82100EB8(X,-1)` + `sub_82100F30(X,-1)` (libera formas no actuales) y desengancha cuerpos. [C]
3. `sub_820F42F0`, `sub_820FB698`, `sub_820C19D8`, `sub_820DBE58`, `sub_820F3728`, `sub_820D3088`, `sub_82221EE8`, `sub_82123DB0`. Sin identificar: [N].
4. `sub_82092A90(32, 0x80000000, 1)` (cancela peticiones de carga por máscara, según `docs\10_tag_team\12_carga_completa.md`). [I]
5. `sub_8210E5C0` -> `sub_821092C8` (cámara, idioma y 8 formas). `4.cpp:7052`. [C]
6. Resto de limpieza (`sub_820FB6A8`, `sub_820C3048`, `sub_820DB4B0`, `sub_820CD640`, `sub_820F8778`, `sub_82123DD8`, `sub_820FCDA0`, `sub_820FCE08`, `sub_82139AC0`, `sub_820F3E68`, `sub_820F3780`): [N].
7. Al final pone `cfg+27892` a cero (corta el contexto de batalla). [I]

**Problema que he encontrado [N]:** en el paso 5, `sub_8210E5C0` (`3.cpp:7900`) no carga ningún `X` antes de llamar a `sub_821092C8`, y `4.cpp:7046-7052` llama a `sub_82092A90` justo antes. `sub_82092A90` devuelve `r31` en algunos caminos (`11.cpp`). Por eso el `r3` que llega a `sub_821092C8` no es claramente el luchador. Si es así, la cámara y el idioma de algún jugador podrían no liberarse nunca. Hay que comprobarlo con un hook en `sub_821092C8` que registre `r3`.

### 2.2 Inicio o reinicio de combate

- Los tres arranques llaman a `sub_820FEA98` (`30.cpp:6896`): `sub_820FF750` (`11.cpp:7050`), `sub_820FFFC8` (`32.cpp:7657`) y `sub_820FF1B8` (`33.cpp:7021`). [C]
- `sub_820FFFC8` **no** llama a `sub_820FE9B0`. Si un reintento usa solo este camino, no libera nada antes de volver a cargar. [C] / [I]
- `sub_820FEA98` -> `sub_82110810` (`2.cpp:7837`) -> `sub_8210E4F8` + `sub_82110640` (`26.cpp:7661`). [C]
- `sub_82110640` solo libera formas y reinicia la máscara. **No carga cámara ni ID.** [C]
- La cámara, el idioma y el ID los carga `sub_8210E468` (`7.cpp:7749`): `sub_82109BA0` (ID, `21.cpp:7374`) y `sub_82109148` (cámara). [C]
- **No he encontrado quién llama a `sub_8210E468`.** Solo aparece en la tabla de funciones (`dbz3_init.cpp`). Si el reintento no la llama, la cámara queda a cero tras `sub_821092C8`. [N]

### 2.3 Otras liberaciones de escena (no son del luchador)

- `sub_821C1818` (`28.cpp:17867`): libera tablas globales y `r30+184` (`28.cpp:17959`). Llamado desde `sub_821D6868` (`19.cpp:19039`). [C] Pertenece a escena, no a un luchador. [I]
- `sub_821C24B8` (`27.cpp:17648`): carga escena (`sub_82133B88` y `sub_82083968`). Llamado desde `sub_821D5260` (`35.cpp:18677`) y otros. [C] / [I]
- `sub_82133D00` también se llama desde `sub_821C6BB8` (`33.cpp:18215`), `sub_821CC358` (`26.cpp:18326`), `sub_821CD098` (`11.cpp:18839`) y `sub_82135D70` (`3.cpp:10422`). [C] Qué objeto es en cada caso: [N].

---

## 3. Objetos de escena por luchador que referencian al modelo

Se crean al cargar la forma (`sub_8210E618` -> `sub_821093C8`, `sub_82115270`) y no se liberan en `sub_82100D60`:

| Objeto | Dónde | Relación con el modelo | Estado |
|---|---|---|---|
| Cuerpo1 `X+1616` | `sub_820F9970` (`22.cpp:6490`) | `+1324` = modelo enganchado; `+1364` = índice; `+1376` = descriptor; `+1644` = marca de desenganche | [C] |
| Cuerpo2 `X+3280` | igual | igual, con `X+4604` | [C] |
| Dueño del cuerpo | `X+1616+1384 = X+3000` | Puntero de vuelta al luchador | [C] (`src\tagteam.cpp:kBodyOwner`) / [I] |
| Descriptor de forma | `X+7444+20f` si la forma está en la máscara `X+7604`; si no, `X+7416+212+20f` | Elige la tabla según la máscara | [C] |
| Punteros de partes `X+1132..X+1176` (12 entradas) | `sub_821093C8` (`16.cpp:7478`) | Se escriben desde los datos del modelo | [I] |
| Tabla por forma `X+1200+512f` | `sub_8208D2C8` (`38.cpp`) | Guarda `X+1132+4f` y bloques de 512 B por forma | [I] |
| Sub-objetos del modelo | `sub_82134120` (`29.cpp`): `sub_821349F0`, `sub_821370B8`, `sub_82134050` | Se preparan al aplicar la forma | [I] |
| Marca "listo" del modelo | `modelo+36 == 0` | Comprobación antes de aplicar | [C] (`src\tagteam.cpp:169`) |
| Sombra, física, efectos adjuntos, aura, BSP | no rastreados | - | [N] |

**Riesgo:** `sub_82100D60` no pone a cero `X+1132..X+1176` ni `X+1200+512f`. Si el modelo se borra y estos punteros siguen apuntando a él, quedan colgando. Hay que comprobar si `sub_820F9F90` o la siguiente aplicación de forma los reinicia. [N]

---

## 4. Cómo el cambio de forma reenlaza el cuerpo a un modelo nuevo

Camino en combate (ya usado por el mod de relevo de `src\tagteam.cpp`):

1. `sub_8210E100` (`29.cpp:7737`, por frame): compara forma actual `X+2980` y destino `X+2984`. Si cambian: `sub_82119108` elige la forma, luego `sub_82100EB8`/`sub_82100F30` (libera), `sub_82100A10` + `sub_82100B78` (carga) para el destino. [C]
2. `sub_8210E690(X, f)` (`2.cpp:7794`): llama a `sub_82100CA0` (`6.cpp`) y a `sub_8210E618`. [C]
3. `sub_8210E618(X, f)` (`19.cpp:7666`): si `X+7608+4f != 0` **y** `modelo+36 == 0`:
   - `sub_82134120(modelo)` (prepara sub-objetos). [C]
   - `sub_821108A0(X, f)` (`7.cpp:7833`): pone bit 0 de `+74` en las entradas de la tabla de segmentos del modelo. [C]
   - `sub_821093C8(X, f, 0)` (`16.cpp:7478`): rehace `X+1132..X+1176` y las tablas por forma. [C]
4. `sub_82115270(X, f)` (`19.cpp:8073`): llama a `sub_820F9970(cuerpo1, f)` (`19.cpp:8106`), recalcula el nivel base de ki (`sub_82100760`), y avisa (`sub_820B86A0`). [C]
5. `sub_820F9970(cuerpo, f)` con `f >= 0` (`22.cpp:6490`): escribe `+1324`, `+1364`, `+1376`, `+1380`, `+1372` y llama a `sub_821399C0` (enganche real). [C]

Llamadores de la cadena de transformación (ya en uso): `sub_820FA7D8` (`29.cpp:6619`), `sub_820FA940` (`13.cpp:6663`), `sub_820FADD8` (`42.cpp:6755`), `sub_8211CC28` (`20.cpp:8539`), `sub_8211D9C8` (`10.cpp:8518`), `sub_82118FF0` (`19.cpp:8265`). [C]

Las llamadas que sí liberan modelos ya existentes en el cambio de forma son `sub_82100EB8`/`sub_82100F30` (sin la forma actual, mínima ni máxima) y `sub_82100E70` (`17.cpp:6995`, que llama a `sub_82100D60` y `sub_82100DE8` para una forma dada, llamado desde `sub_8211CC88`, `16.cpp:8480`). [C]

---

## 5. Lo que no se ha podido confirmar

1. Quién llama a `sub_8210E468` (carga de ID, cámara y animaciones de inicio).
2. Por qué `sub_8210E5C0` recibe un `r3` no identificado (sección 2.1).
3. Qué hace `sub_82083D08` por dentro (solo se ve el reparto por tipo) y `sub_820832D8` (la liberación real de `sub_82083A68`).
4. Si `sub_82083968` usa caché (el contador `+188` lo decrementa `sub_82083A68`, pero no he visto dónde se incrementa).
5. Aura, BSP, efectos, sombra y física: no rastreados.
6. Qué modo lanza cada arranque de combate (`sub_820FF750`, `sub_820FFFC8`, `sub_820FF1B8`) y cuál es el reintento.

---

## 6. Secuencia propuesta para cambiar un luchador (no probada) [I]

Si se quiere sustituir el modelo de un jugador en combate, con el orden que sugiere el código:

1. Desenganchar ambos cuerpos: `sub_820F9970(X+1616, -1)` y `sub_820F9970(X+3280, -1)`.
2. Liberar modelos y animaciones de todas las formas: `sub_82100D60(X, f)` y `sub_82100DE8(X, f)` para `f = 0..7`.
3. Liberar cámara e idioma: `sub_82083A68` o `sub_82083D08` sobre `X+7916` y `X+7924` (con el mismo criterio que `sub_821092C8`), y poner a cero `X+7896..X+7916` y `X+7920..X+7924`. **Antes** de cargar otra cámara, porque `sub_82109148` sobrescribe `X+7916` sin liberar lo anterior.
4. Poner a cero los punteros `X+1132..X+1176` y las tablas `X+1200+512f`. [I]
5. Reset: `sub_82109638(X, 0)`.
6. Cargar el nuevo ID y recursos: `sub_82109BA0` y `sub_82109148` (cámara, idioma, formas). Hay que reproducir lo que hace `sub_8210E468`.
7. Aplicar la forma 0: `sub_8210E690(X, 0)` y `sub_82115270(X, 0)`, con la espera de `modelo+36 == 0`.

Advertencia: el paso 6 depende de la pregunta 1 de la sección 5. Sin saber quién llama a `sub_8210E468` no sé si la carga de cámara y ID se puede repetir en combate sin efectos secundarios.
