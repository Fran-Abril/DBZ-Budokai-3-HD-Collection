# Frente: QoL y facilidad de uso (2026-10-15)

Objetivo (punto 6): que un jugador no técnico entienda y use el launcher y el Mod Kit sin ayuda. Recorrido como
jugador nuevo: primer arranque (sin ajustes, sin partida, sin mods), instalar desde Nova's Mods, modos nuevos,
reparar, mando, idiomas (es/en/it/de/fr). Todo compilado con `tools/compilar.py` y probado con capturas en
`D:\DBZ3HD\launcher_test` (copia aislada). Nada publicado ni subido. Capturas: `handback_evidencias/qol_2026-10-15/`.

## Qué cambia para el jugador (antes → después)

1. **Primer arranque**: antes salía en inglés aunque tu Windows fuera español (también el aviso de "no
   encuentro el juego", que sale antes del launcher) y sin ninguna guía. Ahora sale en el **idioma de Windows**
   (es/it/de/fr; el resto, inglés) y se abre sola una ventana de **Primeros pasos**: 1) tu copia del juego (dice
   si la encuentra), 2) idioma (desplegable ahí mismo), 3) mando o teclado, 4) extras (Nova's Mods, Mods, pestaña
   Juego). Botón **Primeros pasos** fijo en la cabecera para volver a verla.
2. **Idioma**: el desplegable mostraba "Spanish/German/French..." en inglés; ahora cada idioma con su nombre
   (Español, Deutsch, Français, Italiano). La ayuda dice cuándo cambia el juego (al pulsar JUGAR).
3. **Mods**: la lista de mods quedaba debajo de las instrucciones de texturas (solo asomaba media fila). Ahora la
   lista se ve al entrar; "Crear tu propio pack de texturas (PNG)" está plegado. Texto de arriba sin jerga ("Sobrescriben
   entradas dentro de los .afs..." → qué hacer para activar, instalar o ver la ficha). Perfil "vanilla" → "Original
   (sin mods)" y "Ninguno (a mano)" si has activado mods sueltos. Sin mods: botón **Ir a Nova's Mods**. Errores en
   amarillo (antes todo salía en verde). "1 mods (1 activados)" → "Mods: 1 · activados: 1".
4. **Nova's Mods**: el resultado de instalar salía al final de la lista (había que bajar); ahora arriba, en verde
   si ha ido bien (con **Ver en Mods**) o en amarillo si falla; mientras descarga, el % junto al mod.
5. **Controles**: para cambiar una tecla había que escribir nombres en inglés ("Semicolon,Space"). Ahora cada casilla
   tiene un botón de teclado: lo pulsas, pulsas la tecla y listo (Esc cancela; Enter no lanza el juego mientras
   tanto). "Backend del mando" → "Sistema de mandos" con opciones claras; ayuda en la zona muerta; quitado el aviso
   que mandaba a un "menú de ajustes F4" que era este mismo launcher.
6. **Mando sin teclado**: B (o Esc) cierra todas las ventanas (Novedades, ficha de mod, reparación, confirmaciones,
   Primeros pasos); antes había que buscar el botón. Las ayudas del mando ("A Elegir · B Atrás · LB/RB Pestañas ·
   START Jugar") se cortaban debajo de JUGAR: ahora van en la fila de arriba y caben en los 5 idiomas.
7. **Reparar instalación**: antes una lista técnica ("[OK] rexruntime.dll (11134976 B)..."). Ahora una frase con lo
   que pasa y qué hacer ("Todo en orden...", "Falta una parte del juego: descomprime otra vez...", "Tus ajustes estaban
   dañados: se han apartado...") y la lista plegada en "Detalle técnico".
8. **Avisos con solución**: ajustes dañados (ahora dice "pulsa Reparar instalación"), carpeta que no se puede abrir,
   ISO que no es de Xbox 360 ("no la de PS2"), archivos de dos versiones, fallo al preparar personajes (ofrece jugar
   con los de antes o desactivar el último), herramientas del Kit que faltan (dice qué zip descomprimir).
9. **Vídeo / Escalado / Audio / Avanzado**: "Activo: Personalizado -> 1x, MSAA ON, aniso 5, fsr" → "Ahora: escala
   1x · bordes MSAA sí · filtrado 16x · FSR 1"; "MSAA 2x nativo" → "Bordes suaves (MSAA 2x)"; "API de render del host"
   → "Direct3D 12 es el recomendado"; volumen en % (antes 0.00-1.00); "Nitidez RCAS" → "Nitidez"; la pestaña Avanzado
   empieza diciendo que para jugar no hace falta tocar nada; el resumen de abajo dice "FSR 1" (antes "fsr").
10. **Modelos y Texturas**: sin "bin #AMB", "slot", "swap nativo", "recompila el bin": "Cambiar el modelo de un
    personaje" (Modelo de → Se lo pones a → Crear el cambio; la ruta del data_cmn.afs, plegada en Opciones avanzadas)
    y "Retocar las texturas de un personaje" con 3 pasos numerados; si falta el Kit, dice qué descomprimir.
11. **Guardar ajustes**: no daba ninguna señal; ahora el botón pone "Guardado" 2 s y explica que también se guarda solo.
12. **Italiano, alemán y francés**: 40 textos de hoy (pestaña Juego, Novedades, ficha de mod, informe de reparación)
    salían en inglés; ahora traducidos, más los 100 nuevos de este frente (0 pendientes de 617). Arreglados 136
    textos antiguos escritos sin tildes ni diéresis ("Qualitaet", "Modalita qualita", "Reparer", "fenetre"...) y el
    plural roto en alemán ("266 Dateie" → "266 Dateien") e italiano.
13. **Ajustes más seguros**: `dbz3_user.toml` se escribe primero a `.tmp` y luego se cambia (si el juego se cierra a
    mitad, no se pierden los ajustes; propuesta P2 de `bugs.md`).
14. **Mod Kit**:
    - **Retos predefinidos** (pedido del coordinador, K1/K2 de `vision_futura.md`): en Historias, desplegable con los
      15 retos del catálogo (Torre · Torre de Namek (4 combates)...), botones Poner, Quitar y **★ Torre de la semana**,
      y una línea "Ahora en el juego: Torre: Torre de Namek · Supervivencia: de serie · Kumite: de serie". Usa
      `retos_predefinidos.py instalar|quitar|semana --juego <juego>`; `retos_predefinidos` añadido a `KIT_ENTRIES`
      de `tools/make_modpacks.py` (los `.toml` ya entraban por el glob).
    - **Modos del HUB** (propuesta 5 de `modos.md`): en la barra "Modos de reto", botón **Poner en el HUB**
      (`modos_reto.py nativo <tipo> --rivales <los de serie, N combates> --juego <juego>`) y fila **Torneo
      Mundial** (nombres o números separados por comas → `modos_reto.py torneo --participantes ... --juego`). Lo
      que hubiera en `user_data/dbz3/modos/<tipo>.toml` pasa antes a `_respaldo` (la herramienta lo sobrescribía).
      Probado en `launcher_test`: torre de 3 → el juego la carga y empieza "Floor 1 of 3" (`pruebas.py correr
      launcher_test hub_torre.toml` **PASS**, capturas 26-27); torneo → `participantes = [0, 7, 33, 40]`.
    - Idioma inicial: el del juego (`dbz3_language` del `dbz3_user.toml`) o el de Windows; antes siempre español.
    - Errores con solución: archivo en uso ("cierra el juego..."), disco lleno, archivo que falta; el genérico dice
      dónde está el detalle y que se puede copiar para el Discord (antes "Algo falló (código 1): KeyError...").
    - Pulsar un botón mientras el Kit busca juegos al arrancar ya no saca "Ya hay una tarea en marcha": espera su turno.
    - Ventana de error al abrir y lectura de registro en Diagnóstico con frase y solución.
15. **README_EN.md, github/README_EN.md, README.md y github/README.md**: tabla "Pestañas del launcher" con los nombres
    de hoy (Juego, Modelos, Personajes, Avanzado...), "Native mods" anotado como la pestaña Game, "Dev tab" → Advanced,
    instrucciones del Kit sin instalar Python (ya va dentro), Primeros pasos, A/B en los controles. Sin `docs/07_ports`.

## Medido

| Medida | Antes | Después |
|---|---|---|
| Textos del launcher sin italiano/alemán/francés | 40 (+15 de Novedades y pestaña Juego) | 0 de 617 |
| Textos IT/DE/FR sin tildes / diéresis corregidos | — | 136 (19 IT, 46 DE, 71 FR) |
| Mods: dónde empieza la lista (pantalla 1280x720) | y = 600 px, media fila visible | y = 445 px, lista entera |
| Idioma en el primer arranque (Windows en español) | inglés (también el aviso previo, captura 25) | español (captura 01) |
| Cambiar una tecla | escribir el nombre en inglés | 1 clic + la tecla |
| Ayudas del mando en el pie | "START Jugar" tapado por JUGAR | 4 ayudas visibles (es/de, capturas 07-08) |
| Aviso "Personajes nuevos..." encima de JUGAR | se salía de la ventana | partido al ancho del botón |
| Resultado de Nova's Mods | al final de la lista | arriba, con color y "Ver en Mods" |

## Pruebas

- `python tools/compilar.py` → **OK** (varias veces; la última tras todos los cambios).
- `python tools/pruebas.py correr launcher_test menu` → **PASS** dos veces (menú principal en 25,3 s y, con el
  último build, en 20,8 s: `D:\DBZ3HD\_pruebas\20261015-034859_launcher_test_menu`).
- `clang++ -std=c++20 -finput-charset=UTF-8 src/launcher/i18n_selftest.cpp` y ejecutarlo → **OK**.
- `python "mod center hd/modkit_gui.py" --selftest` → **RESULT: OK** (comprueba los 3 comandos nuevos: semana,
  instalar, quitar, y que la lista tiene 15 retos). `python retos_predefinidos.py --selftest` → OK.
- Prueba real del Kit: "Torre de la semana" sobre `D:\DBZ3HD\launcher_test` → instala `torre.toml` (Torre de
  Namek), la línea de estado lo dice y "Ahora en el juego" se actualiza (`kit_semana_es.png`, `kit_semana_en.png`).
- Auditoría de textos: `scratchpad/qol/i18n_audit.py` (cuenta los `i18n::T` sin entrada en `kTable`),
  `gen_i18n.py` (añade las traducciones de `trad.py`), `fix_accents.py` (tildes, solo palabras inequívocas).
- Capturas con `scratchpad/qol/qshot.py` (usa `tools/pruebas.py`, respeta los 3 juegos) y `qfresh.py` (primer
  arranque de verdad: aparta toml, partida y mods de la copia a `E:\DBZ3HD_trabajo\qol\aparte_*` y los devuelve).
  Pistas nuevas de prueba en el launcher: `DBZ3_TEST_FIRST=1` (abre Primeros pasos) y `DBZ3_TEST_PAD=1` (ayudas de
  mando sin mando).

## Archivos tocados

- `src/launcher/launcher_state.cpp/.h`: todo lo de arriba del launcher (Primeros pasos, asignar tecla, B cierra,
  ayudas del pie, Mods, Nova's Mods, textos de todas las pestañas, cvar `dbz3_first_steps_seen`).
- `src/launcher/settings.cpp`: idioma de Windows en el primer arranque, informe de reparación con resumen, escritura
  atómica del toml.
- `src/launcher/i18n.cpp`: 140 entradas nuevas, 136 corregidas (tildes), plural de "archivo%s".
- `src/launcher/ui_kit.cpp/.h`: icono de teclado (U+E765).
- `mod center hd/modkit_gui.py`, `tools/make_modpacks.py` (1 línea: `retos_predefinidos`).
- `README.md`, `README_EN.md`, `github/README.md`, `github/README_EN.md`.
- Copias de lo de antes: `E:\DBZ3HD_trabajo\qol\respaldo_inicio\`.

## Propuestas para otros frentes (no tocado)

- **`src/main.cpp`** (aviso "no encuentro el ejecutable", sale antes del launcher): ahora sale en el idioma de
  Windows, pero el texto es técnico ("yae3_xenon.xex / DBZ3/yae3_xenon.xex", "Games on Demand, Content/0000...") y
  con "No" el programa se cierra sin llegar al launcher. Propuesta de texto:
  `"No encuentro tu copia de Budokai 3 HD.\n\nPon la ISO del juego (Xbox 360) junto a dbz3.exe, o pulsa Sí y elígela
  ahora. Si lo tienes extraído en una carpeta, copia junto a dbz3.exe la carpeta DBZ3 del disco (o default.xex y
  us/).\n\nCarpeta buscada:\n"` (y lo técnico, al log). Y con "No", abrir el launcher igualmente (tiene el aviso y
  los botones Carpeta extraída / Imagen ISO) en vez de `_Exit(1)`.
- **`tools/pruebas.py`** (`texto_log`): coge el log de la prueba anterior si se lanzan dos seguidas (filtra por mtime);
  `qshot.py` usa la fecha de creación (`getctime >= t0 - 1`).
- **`src/mods.cpp`** (`InstallModFromZip`): los errores que devuelve llegan tal cual al jugador ("Error al instalar el
  mod: ..."); revisar que digan qué hacer. P3 de `bugs.md` (SHA-256 del zip de Nova's Mods) sigue pendiente: hace
  falta el campo en el catálogo (`tools/novas_mods.py`) antes de comprobarlo en el launcher.
- **Novedades de la próxima versión** (`src/launcher/novedades.h`, al publicar): "Primeros pasos al abrir el
  launcher por primera vez, en el idioma de tu Windows." / "Cambia las teclas pulsándolas." / "Con mando, B cierra
  cualquier ventana." / "Reparar instalación te dice qué pasa y qué hacer." / "Mod Kit: retos predefinidos del HUB y
  torre de la semana."
- Quedan textos antiguos en IT/FR con alguna tilde de menos (solo se corrigieron palabras sin ambigüedad).
- Para quien escriba textos nuevos en español: la corrección automática de tildes convierte "esta lista" en
  "está lista" (pasó hoy); escribe el texto ya con sus tildes y evita "esta" + palabra de la lista de verbos de
  `i18n.cpp` (`kVerbo`).
