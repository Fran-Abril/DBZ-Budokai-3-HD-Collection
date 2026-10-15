# Frente: Mod Kit (2026-10-15)

Archivos tocados (solo los del frente): `mod center hd/modkit_gui.py` y `tools/make_modpacks.py`.
Nada publicado, nada subido, nada escrito en la carpeta de mods real (las pruebas usan carpetas temporales).

## Qué cambia para el jugador

1. **Importar personaje → tarjeta «5. Personajes de la visión (beta)»**. Misma forma que «4. Ports oficiales»:
   lista de `ports_vision.toml` (`importar.py oficiales --recetas ports_vision.toml`), botón Crear/Rehacer por
   personaje y «Crear todos» (`importar.py oficial --recetas ports_vision.toml <mod>|--todos --mods <mods>`).
   Avisa de que son propuestas, no oficiales.
2. **Historias → barra «Modos de reto»** encima del editor: torre / supervivencia / kumite + número de
   combates (2-10) + «Crear y montar». Paso 1/2 `modos_reto.py <modo> --pisos|--oleadas|--jefes N --salida
   campanas/retos/reto_<modo>.toml`; paso 2/2 `campana.py --mods <mods del Kit> montar <toml>`.
3. **Montar campaña (también el botón «Montar mod» del editor incrustado)**: ahora monta en la carpeta de mods
   que usa el Mod Kit (`campana.py --mods … [--us …] montar`); antes iba a la carpeta por defecto. Al terminar:
   deja el mod activo, dice dónde jugarlo («JUGAR > Dragon Universe > Freezer»), botón «Abrir carpeta del mod» y,
   si hay otras campañas activas con el mismo personaje de la rueda (se pisan la casilla), lo avisa con un botón
   «Dejar solo esta activa» (solo pone el marcador `.disabled`; se deshace en «Mis mods»).
4. **Progreso por paso**: la barra de abajo enseña la última línea de la herramienta mientras trabaja
   («Trabajando: Modo de reto (2/2): montar… · <línea>»), para todas las herramientas.
5. **Sin avisos falsos de «ya hay una tarea»**: las consultas de fondo (listas de juegos, ports) esperan su
   turno en vez de sacar una ventana emergente.
6. Ports oficiales/visión: «Crear» vs «Rehacer» se decide mirando TU carpeta de mods (antes miraba la de por
   defecto de `importar.py`) y los errores salen en sencillo en la tarjeta.

## Empaquetado (`tools/make_modpacks.py`)

- `KIT_ENTRIES` + `campana`, `editor_historias`, `modos_reto` (modos_reto se lanza como proceso: ningún import
  llegaba a él).
- `KIT_DATA` + `ports_vision.toml`.
- `KIT_CAMPAIGNS` admite globs: + `retos/*.toml`.
- Guardia nueva `no_third_party_art`: si alguna imagen (png/jpg/webp/avif/…) llega a `campanas/` del Kit, la
  build se para. Probado: forzando `imagenes/*` la guardia salta.
- `--dry-run` nuevo: prepara el Kit en una carpeta temporal y lista `mod center hd/` (sin Python portátil, sin
  zip, sin tocar `release_packs/`). Resultado: 117 ficheros, incluye modos_reto.py, ports_vision.toml y los 3
  `campanas/retos/*.toml`; ninguna imagen de campañas.

## Pruebas

- `python modkit_gui.py --selftest`: **RESULT: OK (37.6 s)**. Nuevas comprobaciones: comando de la tarjeta de
  visión, comando de modos de reto, `campana.py --mods <mods> … montar`, y `campaign_rivals` con una carpeta
  temporal (activa/desactivada/otro personaje/sin campaña). Una pasada anterior dio «Historias: el editor no se
  construye» mientras el frente de campañas reescribía `campana.py` (01:15-01:16); repetida después: OK.
- `python tools/make_modpacks.py --help` y `--dry-run`: OK.
- Prueba real en temporal (lo mismo que hace «Crear y montar»): `modos_reto.py torre --pisos 3 --salida <tmp>`
  y `campana.py --mods <tmp>/mods montar <tmp>/reto_torre.toml` → código 0, 74 entradas, `campana.toml` con
  `[rueda] personajes = 27`. (A las 01:15 falló con `KeyError: 'next'` en `campana.nueva_entrada` mientras ese
  frente editaba; a las 01:18 ya va.)
- Capturas (ventana real, sin guardar ajustes): barra «Modos de reto» en Historias y la tarjeta 5 con los 6
  personajes de `ports_vision.toml` leídos por `importar.py oficiales --recetas`.

## Propuestas para otros frentes (no tocado por mí)

- **importar.py (ports)**: `oficiales` debería aceptar `--mods` (hoy calcula «instalado» con
  `rb.default_mods()`); el Mod Kit ya lo calcula solo. La cabecera de `ports_vision.toml` dice que `oficial`
  no lee ese fichero: ya no es verdad desde `--recetas`.
- **campana.py (campañas)**: (a) vigilar el `KeyError: 'next'` de arriba (`nueva_entrada` sin
  `reservar_cmn`), aunque ya no se reproduce; (b) el `campana.toml` del mod escribe `personajes = 27, 28` sin corchetes: con más de un
  personaje no es TOML válido (con uno sale un número suelto; el Mod Kit acepta ambos). Si el lector del
  runtime lo admite, escribir `[27, 28]`.
- **editor_historias.py (campañas)**: la «Ventana aparte» monta en la carpeta de mods por defecto. Propuesta:
  `--mods/--us` en su línea de comandos que pase a `campana.py`; el Mod Kit ya los mandaría desde
  `open_story_editor` (un cambio de una línea cuando exista).
- **modos_reto.py (modos)**: `--selftest` exige que existan las imágenes de Freezer (`comprobar`): en el Kit no
  van (arte de terceros), así que el selftest fallaría allí; mejor aviso que error. Los tres retos y la historia
  de Freezer comparten casilla y `textos_desde = 5620`: el Mod Kit avisa y ofrece dejar solo uno activo.
