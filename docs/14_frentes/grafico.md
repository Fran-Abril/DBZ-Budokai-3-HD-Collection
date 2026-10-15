# Frente: Gráfico (ronda 2, 2026-10-15) — ficha 02, parte B (reiluminación)

Copia de pruebas: `D:\DBZ3HD\grafico_test`. Scripts y temporales: `E:\DBZ3HD_trabajo\grafico\` (`gtest.py`,
`sdk_build.py`, `online_relight.py`). Resumen de capturas: **`D:\DBZ3HD\_pruebas\grafico_02B_resumen\`**.

## Qué ve el jugador
- **Antes**: no había ninguna opción de iluminación; la escena solo tenía la luz "pintada" del juego.
- **Ahora**:
  - Launcher, pestaña **Escalado**, sección nueva **"Iluminación (experimental)"**: fila **"Sombras de contacto
    (SSAO)"** con **No / Baja / Alta** (por defecto **No**) y, al lado, la etiqueta naranja **"⚠ Experimental:
    cuesta rendimiento"**. El (i) explica qué hace, que la interfaz no cambia, que es solo Direct3D 12 y qué
    hacer si va a tirones. Textos en español, inglés, italiano, alemán y francés (japonés usa el inglés, como el
    resto del launcher). Captura: `04_launcher_es.png`, `04_launcher_it.png`, `04_launcher_en_it_de_fr_fila.png`.
  - Menú rápido del juego (F1 / Back+Start), sección IMAGEN, última fila **"Sombras de contacto"** (No / Baja /
    Alta) con su ayuda. Cambia al momento y se guarda. Captura: `05_menu_rapido_fila.png`.
  - Encendido: la escena 3D se oscurece un poco en los pliegues de la ropa, bajo los brazos, entre las piernas,
    donde los pies tocan el suelo y en las bases de árboles y rocas. Máximo un 40 % y en "banda" suave, para no
    romper el cel-shading. **La interfaz no cambia**: barras, textos (READY?, FIGHT, daño), diálogos de la
    historia, menús, pausa y mapa de Dragon Universe quedan igual (se aplica antes de que el juego los dibuje).
  - Baja y Alta se ven casi igual; Alta mira el doble de direcciones (menos ruido en bordes) y cuesta más.

## Cómo funciona (runtime, `rexgpu-xenos.dll`)
1. En cada copia de la profundidad de la escena que hace el juego (Budokai 3 hace hasta 3 por fotograma: una por
   sombra de personaje) se guarda la profundidad; la última es la escena entera. Solo si en ese fotograma hubo
   dibujos 3D (la pausa restaura una profundidad vieja: así no se oscurece su imagen congelada una y otra vez).
2. Cuando empieza el post-proceso (primer pase pequeño con la forma de la pantalla: el bloom; se mira en los
   registros del viewport del juego), la siguiente copia de color a pantalla completa es la escena ya compuesta,
   antes del HUD: ahí se multiplica el color por el factor de oclusión, una vez por fotograma.
3. Oclusión: "horizonte" desde la profundidad (normales reconstruidas, 4 direcciones en Baja / 8 en Alta, 4 pasos,
   radio 48 px a 720p que escala con la resolución), desenfoque 4x4 que respeta los bordes, banda suave.
4. **Apagado = coste cero**: la función sale en la primera línea; no se crea ningún recurso ni se manda nada a la
   GPU. El log lo demuestra: la línea `dbz3: perf ...` termina en `relight=N` (pasadas cada 5 s); apagado,
   siempre `relight=0`.

Ficheros: `rexglue-sdk-0.10/src/graphics/d3d12/command_processor.cpp` (+ `.h`), `render_target_cache.cpp`
(+ `.h`, `Dbz3BeginRelight`), `src/graphics/shaders/dbz3_relight.hlsl` + 3 cabeceras en
`bytecode/d3d12_5_1/` (recompilar con `fxc`, órdenes en la cabecera del `.hlsl`), `src/graphics/graphics_system.cpp`
(solo pruebas, ver abajo). Juego: `src/launcher/settings.cpp/.h` (cvar `dbz3_relight` 0-2 + puente a
`dbz3_relight_level`), `launcher_state.cpp` (fila), `i18n.cpp` (6 entradas), `src/ingame/quick_settings.cpp` (fila).
Espejo `github/patches/rexglue-sdk/` actualizado con los ficheros del SDK.

Cvars de desarrollo (DBZ3/Dev, no salen en el launcher): `dbz3_relight_strength` (0.4), `dbz3_relight_radius`
(48), `dbz3_relight_debug` (1 = ver el factor, 2 = ver la profundidad), `dbz3_relight_split` (solo mitad
derecha). Solo para pruebas: `dbz3_vblank_slowdown` (el juego avanza N veces más lento para comparar casi el
mismo fotograma; **solo actúa con `DBZ3_NO_DIALOGS`**, el entorno de `pruebas.py`, así que un valor guardado por
error nunca frena a un jugador). `DBZ3_RELIGHT_TRACE=1` traza un fotograma en el que no se aplicó.

## DLL canónicas (reconstruidas)
- Respaldo de las anteriores: `rexglue-sdk-0.10\out\_respaldo_canonicas\20261015_antes_grafico\`.
- `rexgpu-xenos.dll`: 6.540.288 → **6.589.440 bytes**. `rexruntime.dll`: recompilada, **11.145.216 bytes** (igual).
- Marcadores comprobados (`AfsGetVirtualTable`, `dbz3_perf_logging` en rexruntime; `dbz3_relight_level` en
  rexgpu). `tools/compilar.py` ya las copia a `out/build/win-amd64-dual`.

## Cómo se probó
- Compilado: `python tools\compilar.py` OK; SDK con `cmake --build ...build-win-vulkan-baseline` (OK).
- **Batería `python tools\pruebas.py todo`: 5/5 PASS** con las DLL finales
  (`D:\DBZ3HD\_pruebas\20261015-133027_todo.txt`; también 5/5 con la versión anterior del mismo día).
- **Mismo combate y cámara** (Goku vs Vegeta, Archipiélago, "READY?"): el juego a cámara lenta
  (`dbz3_vblank_slowdown`) y el nivel cambiado en vivo entre capturas →
  `01_apagado_baja_alta.png` (apagado / baja / alta / lo que oscurece), `00_apagado_completo.png`,
  `00_alta_completo.png`, `03_mitad_derecha_alta_izquierda_original.png` (en el mismo fotograma: izquierda original,
  derecha con Alta). Carpeta original: `D:\DBZ3HD\_pruebas\20261015-132036_grafico_test_grafico_shots`.
- **HUD intacto**: `02_vista_del_oscurecimiento_alta.png` (vista de depuración: la escena se sustituye por el
  factor en gris y el HUD y el "READY?" salen enteros encima, es decir, se dibujan después).
- **Apagado idéntico / coste cero**: con el nivel 0 el log da `relight=0` en todas las ventanas (menús, combate,
  historia); con Baja/Alta `relight=300/301` cada 5 s (todos los fotogramas del combate) y `sin_post=0
  sin_resolve=0` (ningún fotograma se queda a medias).
- **FPS** (telemetría `dbz3: perf`, ventanas de 5 s, en combate):
  | Escala interna | Apagado | Baja | Alta |
  |---|---|---|---|
  | 1x (720p) | 60,0 | 60,0 | 60,0 |
  | 3x (4K) | 60,0 | 60,0 | 60,0 |
  `gpu_wait` 0,0 ms en todos; ningún `tiron` nuevo (`D:\DBZ3HD\_pruebas\20261015-125534_grafico_test_grafico_fps`
  y `...131604...`). En este PC la GPU no se nota; en gráficas modestas sí costará: de ahí el aviso.
- **Historia y menús con Alta** (`pruebas.py correr grafico_test campana`): PASS; rueda, mapa de DU, diálogos y
  combate de la historia bien (`06_historia_y_menus_con_alta.png`, `...133141_grafico_test_campana`).
- **Menú F1**: cambiar la fila a "Baja" desde el menú pasó de `relight=0` a `relight=301` en el siguiente
  segundo y quedó guardado (`dbz3_relight = 1`).
- **Online**: prueba de dos PCs con latencia (`online_relight.py`, copia del de online con copias propias),
  anfitrión con **Alta** e invitado **apagado**: **PASS**, 130 comprobaciones del RNG iguales, 0 desincronías
  (`D:\DBZ3HD\_pruebas\20261015-133516_online_lag`, `07_online_*.png`). Es solo imagen: el online no compara ni
  envía este ajuste.

## Puntuación
| Parte | Mínimo de la ficha | Estado | Nota |
|---|---|---|---|
| 02-B Reiluminación | Toggle experimental `dbz3_relight` (SSAO/SSGI) en el launcher, apagado por defecto, con aviso de rendimiento | **HECHO** (SSAO; también en F1) | **1** · pruebas de arriba |

Con la parte A (online, 1) la ficha 02 queda en A 1 + B 1 + C 0 (la C, minijuego de IW, sigue sin empezar).

## Lo que queda / límites
- Solo Direct3D 12 (el modo por defecto en Windows). Con Vulkan (Linux) la opción no hace nada; en GPUs Intel
  antiguas (ruta ROV) o si la escena usa MSAA, tampoco (se apaga sola, sin fallos).
- SSGI (rebote de color) no está: solo oclusión. Sería otra pasada leyendo el color de la escena.
- El campo de visión se supone (~50°); solo afecta a la forma de las normales. No se lee la matriz del juego.
- Probado en Archipiélago, historia de Freezer y online. Escenarios con agua/reflejos o ataques definitivos con
  efectos de pantalla completa no se miraron uno a uno.

## Propuestas para otros frentes / coordinador
1. `vision_futura.md` §1, fila 02: "B HECHO (toggle `dbz3_relight` en launcher y F1, apagado, con aviso)", B = 1.
2. `AGENTS.md` §7: tamaños de referencia de las canónicas → rexgpu-xenos 6.589.440, rexruntime 11.145.216.
3. Notas de la versión: "Nuevo (experimental): Sombras de contacto en Escalado → Iluminación. Apagado por defecto;
   cuesta rendimiento".
4. `tools/sync_github.ps1` para los ficheros de `src/` (los del SDK ya están en `github/patches/`).
