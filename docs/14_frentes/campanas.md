# Frente: campañas propias de Dragon Universe (2026-10-15)

Dueño: `mod center hd/campana.py`, `editor_historias.py`, `campanas/*` (salvo `retos/`), `src/campaigns.cpp`.

## 1. Una casilla por campaña: varias campañas a la vez

**Límite del juego (medido por el frente DU, `docs/14_dragon_universe/01_casillas_y_partida.md`):** la partida tiene
38 bloques de 4624 B (casillas 0-37) que acaban justo donde empiezan la máscara de historias, el zeni y el byte de
casilla. No hay casilla 38+ sin mover toda la partida. La casilla de una campaña es la de su protagonista
(casilla = ID; 22-25 = ID-5; 26+ = ID-6). Libres (sin historia del juego): 1, 5, 6, 8, 9, 14-16, 18-33, 35-37.

**Por qué no convivían:** además de los bloques por casilla (5000 + 100·saga + casilla, 54xx combates, 58xx charlas),
cada campaña cambiaba **en su sitio** contenedores que comparten todas las historias: introducciones/cierres de saga
(códigos 100-106 → 1721-1727), finales (código 1 → 1728), «I found something!» (2100), y elegía entradas de
`data_cmn` fijas (imágenes 4510+, eventos 4600). Dos mods escribían el mismo contenedor y solo uno ganaba.

**Hecho:**
- `campana.py`: los contenedores compartidos (detectados con `du_codigos.json`: un código fuera de 5000-6199 lleva a
  ellos) ya no se tocan; su copia va a un bloque propio de 64 entradas de `data_cmn` (6000-8191, por crc32 del nombre
  del mod, estable en `campana.toml [contenedores]`) y se sirve por `[codigos_historia]`. Imágenes y eventos del mapa
  sin número fijo van al mismo bloque.
- `campaigns.cpp`: `[codigos_historia]` es **por mod**: solo se aplica si la casilla en juego (0x824A58A0) es la de
  un ID de la `[rueda]` de ese mod. El registro dice ahora `(..., casilla N)` en cada código.
- `campana.toml` generado: `[rueda] personajes = [..]` (TOML válido) y `casilla = N`.
- Validación al montar (y en el editor): la casilla del código tiene que ser la de un ID de `[rueda]`; casilla < 38;
  aviso si sustituye una historia del juego; **error si otra campaña de la carpeta de mods usa ya esa casilla**; aviso
  si otro mod trae la misma entrada de `data_cmn` con otros datos.
- Lo que el montaje ya no escribe en `data_cmn` (las versiones en su sitio de 1723-1725/1728, imágenes viejas) se
  mueve a `mods_respaldo/<mod>_cmn_<fecha>/`, nunca se borra.

## 2. Editor

- Vista previa del diálogo con **la imagen real del texto** (`campana.render`, misma fuente/ancho que va al juego): la
  primera frase del capítulo que usa ese retrato.
- `check()` avisa de la casilla ocupada por otra campaña.
- `--mods` / `--us` en la línea de órdenes: se pasan a `campana.py montar` (ventana aparte).
- Ya existían: validación completa, botón «Montar mod», vista previa de estilos.

## 3. Formas del rival y narración con varias imágenes

- `forma_rival` ya existía (BPC +0x2c/+0x58 en los 8 registros). Ejemplo: Trunks SSJ en `historia_freezer_prueba_tierra.toml`.
- **Nuevo** `imagenes` / `cierre_imagenes` en un capítulo que abre saga:
  `cierre_imagenes = [{ imagen = "x.png", paginas = 2 }, { imagen = 1549 }]`. Cada imagen es una escena con sus
  páginas (y voces); se encadenan con op 20 en entradas nuevas (38+) de la copia del contenedor, como 101 -> 102 del
  juego. `imagen` admite un número (imagen de `data_cmn` del juego: 1411, 1536, 1549). Con varias imágenes no vale
  mezclar `paginas` del juego.

## 4. Pruebas (evidencia en `D:\DBZ3HD\_pruebas\`)

- `python campana.py prueba` OK (incluye `lst_append` y la validación de casillas); `python editor_historias.py --selftest` OK.
- Compilado con `tools/compilar.py`: OK (21 s).
- Copia `du_test` con el juego **en español** (`dbz3_language = 5` en su `dbz3_user.toml`).
- `20261015-012312_du_test_du_freezer_nuevo` (PASS, `historia_freezer_prueba_tierra.toml`, RINGOUT): registro
  `codigo 102 -> contenedor 6128 (del mod) (casilla 22)` al empezar (la copia se usa ya en «Nueva historia»),
  combate 5522, cierre `103 -> 6132`, **`103 entrada 38`** (segunda imagen: captura 027 con el fondo de la Tierra
  1549), «¿Guardar partida?» (030) y guardada `104.22`. Captura 003: la intro de Freezer en español.
- `20261015-011823_..._du_freezer_nuevo` (la rueda cayó en Yamcha, casilla 13): `codigo 100 -> 1727` sin redirigir,
  es decir, las copias de Freezer no tocan la historia de otra casilla.
- **Regresión encontrada y arreglada (2026-10-15, 01:40):** `du_continuar` fallaba (sin líneas `partida:`, Freezer
  fuera de la rueda de «Continuar», volvía el «sale Goku al cargar»). Causa: `Load()` de `campaigns.cpp` tomaba
  como cabecera de sección cualquier línea con `[`, y el nuevo `personajes = [27]` dejaba la `[rueda]` vacía.
  Ahora solo es cabecera si la línea EMPIEZA por `[`. Compilado y repetido:
  - `20261015-014007_du_test_du_continuar` (`--instantanea freezer_cap2 --par iter=3`): **PASS**,
    «partida: 1 historias de mods recuperadas», «rueda ... casillas de mas 0000000000400000» (bit 22),
    «personaje del mapa al crear (casilla 22 ...)».
  - `20261015-014137_du_test_du_continuar` (RINGOUT, iter=15): **PASS**; captura 009 = charla generada por
    `campana.py` en español en el juego («¿Otra transformación? No me asustas.», con ¿ y tilde); 012 = Freezer 3.ª
    forma contra Piccolo (formas por capítulo).

## 5. Siguiente paso (P1 del frente DU)

Campaña de un protagonista cuya casilla ya tiene historia (o personaje nuevo ID ≥ 44): casilla anfitriona libre
(36/37) + gancho de `sub_82156E88` (casilla → ID) + celda virtual → casilla al confirmar (`0x8245F140`).
`docs/14_dragon_universe/04_ganchos_propuestos.md` P1. Sin hacer.

Arreglado también (aviso del frente DU): `MAP_EVENT_WORDS` con 3:4, 30:7, 31:6, 34:4, 41:9, 49:4 y error al montar
si un `[[escena]]` usa los eventos 25-28, 32, 33 o 37 (leen pila sin inicializar).

## Propuestas para otros dueños

- **Mod Kit (`modkit_gui.py`, `open_story_editor`)**: pasar `--mods <carpeta del Kit>` (y `--us`) al abrir el editor
  en ventana aparte; en el modo incrustado, el `runner` debe añadir `--mods` a la orden `campana.py montar`.
- **Mod Kit**: el aviso «otra campaña activa usa la misma casilla de Freezer» puede leer `[rueda] casilla` del
  `campana.toml` de cada mod (ya lo escribe `campana.py`) en lugar de suponer la 22.
- **`tools/pruebas.py`**: `_du_freezer_nuevo` usa `up=2` desde Goku; con la partida de la copia en español la rueda
  quedó en Yamcha (13), Freezer estaba a un paso. Mejor buscar la casilla por memoria (`0x8245F140`, u16) hasta 22.
