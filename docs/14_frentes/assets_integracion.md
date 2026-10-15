# Frente: assets externos como packs visuales OPCIONALES

Fecha: 2026-10-15. Dueno: `mod center hd/packs_visuales/` (constructor + recetas) y este doc.
Catalogo de los assets: `docs/13_assets_externos/CATALOGO.md` + `indice.json` (12.017 ficheros).

## Regla (no negociable)

El arte de terceros **solo** va en packs que el jugador construye en su PC con
`packs_visuales/construir_pack.py`. Nunca en el release (su `mods/` sale vacia y
`verify_release.ps1` lo comprueba), ni en el zip del Mod Kit (`make_modpacks.py` copia por lista
blanca y no conoce `packs_visuales/`), ni en el exe / `res/ingame_ui` (solo nuestros titulos). El
arte del propio juego se saca en tiempo de ejecucion de los AFS del jugador (`native_ui.cpp`).

Salvaguardas en el constructor (cubiertas por `--prueba`):
- rechaza `--mods` dentro del proyecto salvo `out/` (el juego de pruebas);
- `make_modpacks.kit_modules()` no contiene nada de `packs_visuales/` y su fuente no la nombra;
- `packs_visuales/` no contiene imagenes;
- cada pack lleva `CREDITOS.txt` (fichero de origen y autor sacado del nombre "by X").

## Formato de un pack (lo que deja el constructor en `mods/<pack>/`)

```
mods/<pack>/
  manifest.txt                  name=..., type=pack_visual, version, description
  CREDITOS.txt                  destino <- fuente [autor]
  ui_ingame/<pieza>.png         piezas de las pantallas propias (ver 1 y 2)
  texturas/<hash>_<W>x<H>_PNG.png   pack de texturas HD (ver 5)
  campana/<historia>.toml + imagenes/ + retratos/   fuente de una campana (ver 3)
  personajes/<mod>/ui/{cara,retrato,hud}.png         arte de select/HUD (ver 4)
  .disabled                     solo si el pack no trae nada que cargue el juego (fuente pura)
```

Nada se borra: un pack o mod que ya existia se mueve a `mods_respaldo/<nombre>_<fecha>` (fuera de
`mods/`, porque el cargador de packs de texturas recorre las subcarpetas de `mods/`).

## 1. HUB (fondos, banners, renders) — propuesta para `hub_panel.cpp`

**No hace falta cargador nuevo.** `style::Img(name)` (game_style.cpp) ya busca
`mods/*/ui_ingame/<name>.png` en los mods activos (sin `.disabled`, gana el primero por nombre)
antes que la pieza del juego (`native_ui::Take`) y que `ingame_ui/`. Acepta subcarpetas
(`"caras/11"` -> `ui_ingame/caras/11.png`). Propuesta: el HUB pide sus piezas con prefijo `hub/`:

| `S::Img(...)` | tamano del pack | uso propuesto en hub_panel.cpp | sin pack |
|---|---|---|---|
| `hub/fondo` | 1920x1080 (16:9) | fondo a pantalla completa (llenar, recorte centrado) | `fondo_duelo` del juego o degradado de `Backdrop` |
| `hub/banner` | 1024x256 | franja horizontal bajo la cinta del titulo | nada (solo `Banner` con texto) |
| `hub/render_<ID>` | <=1024x1024, RGBA recortado | render del personaje elegido/destacado (ID del juego) | `caras/<ID>` del juego o nada |
| `hub/icono_<fila>` | <=256x256 | icono de cada fila del HUB (Tag Team, Online, Ranking...) | nada |

Reglas para hub_panel.cpp: (a) usar siempre `S::Img` y comprobar `if (pic)`; (b) escalar por
`pic.w/pic.h` (el tamano del PNG es libre; el constructor solo fija el de serie); (c) nunca
depender de que exista: sin pack, las piezas del juego. Opcional: `ui_ingame/hub/hub.toml` con
metadatos de maquetacion (p. ej. `render_ancla = "abajo_derecha"`); hoy no hace falta, ponerlo
solo si el HUB necesita colocar distinto segun el pack.

Hoy mismo funciona sin tocar C++: `ui_ingame/fondo_duelo.png` (1600x1200, 4:3; `Backdrop` usa el
ancho entero y alto = ancho x 0,75) cambia el fondo de TAG TEAM y ONLINE.

## 2. Tag Team (tira del equipo, fichas de companero)

- **Ya funciona**: `ui_ingame/caras/<ID>.png` (256x128 RGBA, tipo `cara_hud`) cambia la cara que
  pintan `TeamStrip` y `PartnerCard` (`FaceOf(id)` = `S::Img("caras/<ID>")`), y
  `ui_ingame/fondo_duelo.png` / `anfitrion_tagteam_a|b.png` (tipo `anfitrion`, se pinta a x1,33
  desde la esquina medida) el menu de TAG TEAM.
- **Propuesta (tagteam_panel.cpp / game_style.cpp, no es de este frente)**: que `TeamStrip` y
  `PartnerCard` dibujen, si existen, `tagteam/tira` (fondo de la tira, ~512x96) y
  `tagteam/ficha` (marco de la ficha, ~384x96) antes de lo que pintan ahora; y que el relevo
  pueda mostrar `tagteam/relevo_<ID>` (franja 1024x256, del grupo `cutin_franja` del catalogo,
  488 ficheros) durante el parpadeo `switching`. Mismo patron `if (pic)`.

## 3. Campanas (narracion y retratos)

`campana.py` ya acepta PNG/WebP/JPG/AVIF en `imagen`, `cierre_imagen`, `[[imagen]] archivo` y
`[[retrato]] png`, relativos al `.toml`, y usa el arte del juego si faltan (el Kit no los lleva).
El pack copia el `.toml` de la campana dentro de `mods/<pack>/campana/`, guarda las imagenes ya
recortadas y cambia en la copia las rutas indicadas (`reemplaza`). Con `--montar` llama a
`campana.py --mods <mods> montar <copia>` y sale el mod de la campana (el anterior a
`mods_respaldo/`). El pack en si queda con `.disabled` (es fuente, no carga nada).

Tamanos: narracion 660x420 opaca (`png2pic.build_picture`, caja por defecto; en pantalla 440x280);
retrato RGBA recortado al dibujo (campana.py lo encaja en la caja 541x530 de la textura 1024x1024).
`modo = "ajustar"` + `fondo = <paisaje>` compone un render con alfa sobre un fondo (para que la
narracion no salga sobre negro). Ojo: los `kakarot_episodio` son paginas de libro con texto.

## 4. Select y HUD de personajes importados (roster_build.py)

`roster_build` lee de `mods/<mod>/ui/`: `cara.png` (arte del icono; con `icono_fuente = "imagen"`
o si el modelo no se puede renderizar), `retrato.png` (`retrato_fuente = "imagen"`), `icono.png`
84x84 y `retrato_p1/p2.png` 512x512 terminados, `hud.png` / `hud_<forma>.png` 256x128. El pack los
deja en `personajes/<mod>/ui/` (tipos `cara`, `retrato_select`, `cara_hud`); `--instalar-ui` los
copia al mod (lo que hubiera a `ui/_respaldo/`) y luego se reconstruye el roster. Ojo: un
`icono.png` o `retrato_p1.png` en `ui/` hace que `original_sources` los use por defecto.
Grupos utiles del catalogo: `render_juegos_3d` (estilo cercano al juego), `render_budokai3_retrato`
(referencia: el propio retrato del B3, 416x408).

## 5. Auras y ki por pack de texturas HD

Hallazgo (medido con `texturas_entrada` sobre data_cmn 0-43): las texturas de las auras son
**formas en gris compartidas**: de las 26 distintas de Goku (entrada 23), 25 las usan otras auras
(la 861EBAAB04C2605A, 27 entradas; la 2F2651D218602351, 42). Solo 12 de las 44 auras tienen alguna
textura propia (Broly 10, Cell 11, Freezer 10, Goku 1: la 25069C5BA2D99015...). El color por personaje no esta en la textura
(eso es `colores.py aura_color/ki_color`). Por tanto:
- **Color de un personaje**: `colores.py` (ya existe), no un pack.
- **Forma/definicion de las auras de todos**: pack HD de las texturas compartidas
  (`textura = "todas"`, `compartidas = true`, `efecto = "sustituir"` con el alfa del original, o
  `"tenir"` para un tono global); cambia el aura de todos a la vez.
- Los GIF/MOV de `Aura/` del catalogo valen como referencia de color o fotograma suelto
  (`fotograma = N`), no como hoja de sprites del juego (otro trazado).

## Propuestas para ficheros de otros frentes (no tocados)

1. `hub_panel.cpp`: piezas `hub/*` via `S::Img` (tabla del punto 1).
2. `game_style.cpp` (`TeamStrip`, `PartnerCard`): piezas opcionales `tagteam/tira`, `tagteam/ficha`,
   `tagteam/relevo_<ID>`.
3. `make_modpacks.py`: dejarla como esta (lista blanca). Si algun dia el Kit lleva el
   constructor, copiar solo `construir_pack.py` + `*.toml` + `README.md` y mantener la comprobacion
   de "sin imagenes" de `--prueba`.
4. Launcher (pestana Mods): mostrar `type=pack_visual` con su `CREDITOS.txt` y no avisar de "mod
   roto" por un pack fuente con `.disabled`.
5. Mod Kit: boton "Construir pack visual" que llame a `construir_pack.py` con una receta.

## Pruebas

- `python "mod center hd/packs_visuales/construir_pack.py" --prueba` -> `prueba OK` (conversiones,
  receta completa con campana y personaje, respaldo, destinos prohibidos, exclusion del Kit, aura
  con el juego real: nombre = hash XXH3, compartidas saltadas).
- Packs de ejemplo construidos en `D:\DBZ3HD\_packs_prueba\mods\` (`pack_hub`,
  `pack_campana_freezer` y el `historia_freezer` montado con sus imagenes: la entrada 4520 del mod
  es byte a byte `png2pic.build_picture` de la imagen del pack).
- Sin probar en el juego (no hacia falta el juego para validar formatos; el siguiente paso es
  ver `fondo_duelo` en TAG TEAM con `pack_hub` activo).
