# Frente: Launcher (2026-10-15)

Objetivo: launcher más simple, rápido, con aspecto de juego y textos completos en español e inglés.
Todo está compilado y probado con capturas en una copia aislada (`D:\DBZ3HD\launcher_test`). Tu partida
real no se ha tocado. Nada publicado ni subido.

## Qué cambia para el jugador

1. **Nueva pestaña "Juego"** (antes "Mods nativos"): todo lo nuevo en un solo sitio.
   - *Modos de juego nuevos*: HUB, Tag Team, Online y Selector de transformación, cada uno con su
     interruptor, la etiqueta **BETA** y una línea que dice dónde aparece en el juego.
   - *Arranque y pruebas*: **Arranque rápido** (abre el juego sin pasar por el launcher; F4 lo vuelve a
     abrir) y **Mostrar los FPS**.
   - *Tu partida guardada*: copia de seguridad, "Guardar al 100%" y restaurar. Ya no salen las entradas
     sin código ("Vida infinita", "Ki infinito"), que solo confundían.
   - *Aspecto de los personajes*: Brillo HD.
2. **Novedades**: botón con estrella en la cabecera. Se abre sola una vez después de actualizar (no en
   una instalación nueva ni con el juego en marcha). Botón "Todas las notas (web)".
3. **Ficha de cada mod**: pulsa el nombre de un mod y sale su ficha (activar/desactivar, descripción,
   autor, versión, qué cambia, archivos, problemas con botón Arreglar, con qué otro mod se pisa,
   abrir carpeta, editar personaje). Los packs de texturas y las historias ya no salen como "otro".
4. **Español con tildes y eñes** en todo el launcher (y en los paneles del juego que usan los mismos
   textos): "Reparar instalación", "Carpeta extraída", "pestaña", "¿Restablecer...?", "Sí, borrar".
   El informe de "Reparar instalación" ya sale también en inglés.
5. **Pestañas sin cortar** en español: "Cambio de...", "Personajes n..." pasan a "Modelos" y "Personajes".
6. **Más rápido** (ver tabla) y un fallo arreglado: Enter o START con una ventana o un desplegable
   abiertos (p. ej. "¿Restablecer todos los ajustes?") lanzaba el juego.

## Antes / después (medido)

Medidas del propio launcher (`dbz3 launcher perf` en el log) en `D:\DBZ3HD\launcher_test`, RTX 4070 S.
"Con pack" = el pack HD de SSGPrinceVegeta (12.350 imágenes) instalado en la copia.

| Medida | Antes | Después |
|---|---|---|
| Primer frame del launcher, con pack | **3.141 ms** (ventana congelada) | **3-8 ms** |
| Entrar en la pestaña Mods, con pack | 2 frames de **~2.900 ms** | peor frame **18-26 ms**; la lista llega en ~2,9 s sin congelar |
| FPS del launcher en los 5 primeros s, con pack | **22,4** y **0,3** | **~60** |
| Pestaña Juego / Mods nativos, coste por frame | **1,22 ms** (buscaba guardados en cada frame) | **0,37 ms** |
| Comprobación de Direct3D 12 al arrancar | **200-430 ms** | **2-26 ms** (desde el 2.º arranque) |
| Textos IT/DE/FR | ~800 comparaciones por texto y frame | búsqueda directa |

Capturas en `handback_evidencias/launcher_2026-10-15/`: `01-02` antes, `03-08` después
(Juego ES/EN, Novedades ES/EN, ficha de mod, lista de mods).

## Pruebas ejecutadas

- `python tools/compilar.py` → **OK** (la 1.ª vez falló al enlazar por `sub_8218BBC0` duplicado entre
  `hub_menu.cpp` y `tagteam.cpp`, de otro frente; a los 2 min se reintentó y pasó).
- `python tools/pruebas.py correr launcher_test menu` → **PASS** (llega al menú principal en 17,6 s).
- `clang++ -std=c++20 -finput-charset=UTF-8 -o i18n_selftest.exe src/launcher/i18n_selftest.cpp` y
  ejecutarlo → **OK** (20 casos de tildes + tabla IT).
- Capturas del launcher: script del scratchpad `lshot.py` (usa `tools/pruebas.py`: misma copia, espera si
  hay 3 juegos, nunca abre otro exe) con `DBZ3_TEST_TAB`, `DBZ3_TEST_NEWS=1`, `DBZ3_TEST_CARD=<mod>` y
  `REX_DBZ3_LANGUAGE=5`.

## Para los demás frentes: cómo sale tu opción en la pestaña Juego (sin tocar el launcher)

- Registra un cvar **bool** con categoría `DBZ3/Mods` (o cualquier `DBZ3/<nombre>` que no sea Video,
  Audio, Input, Language, Paths, Dev o Launcher). Aparece solo en "Modos de juego nuevos".
- Categoría `DBZ3/Pruebas` → sale en "Arranque y pruebas".
- Un cvar de texto con valores permitidos sale como desplegable.
- Texto: la descripción del cvar con el formato `"Nombre: ayuda en español | Name: English help"`; si
  lleva `(beta)`, sale la etiqueta BETA. Para un texto cuidado, pídelo y se añade a `kFeatureTexts`
  (`launcher_state.cpp`), como ya están HUB, Tag Team, Online y el selector.

## Novedades de la próxima versión

En `src/launcher/novedades.h`: añade un bloque arriba con su número (`"1.4.6"`) y frases de jugador en
ES/EN. Se abre sola una vez a quien actualice. Tag Team, Online y HUB siempre como beta.

## Archivos tocados (todos en `src/launcher/`, salvo evidencias y este documento)

- `launcher_state.cpp/.h`: pestaña Juego, lectura de mods en segundo plano, Novedades, ficha de mod,
  medidor de frames, Enter/START con ventanas abiertas, etiquetas de tipo, pistas de prueba.
- `i18n.cpp`: tildes del español, índice para IT/DE/FR, nombres nuevos de pestañas.
- `settings.cpp`: caché de la prueba de Direct3D 12 (`dbz3_d3d12_ok`), informe de reparación ES/EN,
  la lista de packs solo se reescribe si cambia.
- `ui_kit.h/.cpp`: icono de estrella. Nuevos: `novedades.h`, `i18n_selftest.cpp` (no entra en el build).
- Cvars nuevos: `dbz3_news_seen`, `dbz3_d3d12_ok` (categoría `DBZ3/Launcher`).

## Pendiente / límites conocidos

- Las tildes se ponen con un diccionario de palabras seguras: las ambiguas (si/sí, solo, tu/tú, cómo)
  se quedan sin tilde. Texto nuevo: escríbelo ya con tildes y no se toca.
- `README_EN.md` y `github/README_EN.md` (y algún estudio) aún nombran "Native mods" / "New characters
  tab": actualizar al preparar la release.
- Con el pack HD el juego tarda ~2,4 s antes de mostrar el launcher; eso es del plugin gráfico del SDK
  (índice del pack), no del launcher.
- La copia `D:\DBZ3HD\launcher_test` lleva una copia del pack HD (2,7 GB) para medir; se puede reutilizar.
- La columna izquierda de Juego necesita una pequeña rueda en 1280x720 (los textos de ayuda son largos).
- Los textos nuevos de hoy están en español e inglés; en italiano, alemán y francés salen en inglés
  hasta añadirlos a la tabla de `i18n.cpp`.
