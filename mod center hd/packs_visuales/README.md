# Packs visuales opcionales (arte de terceros)

Convierten imagenes de la carpeta de assets externos (catalogo: `docs/13_assets_externos/`) en
una carpeta de mod lista para `mods/`. **Se construyen en el PC de quien tiene los assets**: esta
carpeta solo lleva codigo y recetas; ni el release, ni el zip del Mod Kit, ni el exe llevan arte
de terceros (`tools/make_modpacks.py` no copia esta carpeta; la prueba lo comprueba).

```
python construir_pack.py pack_hub.toml --mods <carpeta mods>
python construir_pack.py pack_campana_freezer.toml --mods <carpeta mods> --montar
python construir_pack.py --prueba
```

Opciones: `--raiz` (carpeta de assets; por defecto la de `indice.json`), `--juego` (carpeta
`us/`, para texturas y `--montar`), `--montar` (monta la campana con `campana.py`),
`--instalar-ui` (copia `personajes/<mod>/ui/*` a `mods/<mod>/ui/`, lo anterior a `ui/_respaldo/`).
Nada se borra: un pack que ya existia va a `mods_respaldo/` (junto a `mods/`).

## Receta

```toml
[pack]
nombre = "pack_x"            # carpeta en mods/
titulo = "..."               # manifest.txt
[campana]                    # opcional: copia esta campana dentro del pack
toml = "campanas/historia_freezer.toml"

[[asset]]
fuente = "ruta/relativa/a/la/raiz.png"     # png/jpg/gif (fotograma = N)
destino = "ui_ingame/hub/fondo.png"        # dentro del pack
tipo = "fondo_hub"                         # tamano y modo por defecto (abajo)
# modo = "llenar" | "ajustar" | "recortar" | "textura";  tamano = [w, h]
# foco = [0.5, 0.4]  (recorte de llenar);  fondo = "otra/imagen.png" o "#102030";
# alinear = "abajo";  escala = 1.0;  opaco = true
# reemplaza = "imagenes/x.avif"   (cadena del toml de la campana que pasa a ser esta imagen)
```

| tipo | modo | tamano | lo usa |
|---|---|---|---|
| narracion | llenar, opaco | 660x420 | campana.py (`imagen`, `cierre_imagen`, `[[imagen]]`) |
| retrato_campana | recortar | <=1082x1060 | campana.py `[[retrato]] png` (caja 541x530) |
| fondo_hub | llenar | 1920x1080 | `ui_ingame/hub/fondo.png` |
| fondo_duelo | llenar | 1600x1200 | `ui_ingame/fondo_duelo.png` (TAG TEAM / ONLINE, ya funciona) |
| banner | llenar | 1024x256 | `ui_ingame/hub/banner.png` |
| render | recortar | <=1024x1024 | `ui_ingame/hub/render_<ID>.png` |
| anfitrion | recortar | <=420x460 | `ui_ingame/anfitrion_*.png` (se pinta x1.33) |
| cara_hud | ajustar | 256x128 | `ui_ingame/caras/<ID>.png`, roster_build `ui/hud.png` |
| cara / retrato_select | recortar | <=512 / <=1024 | roster_build `ui/cara.png` / `ui/retrato.png` |
| textura | textura | original x `factor` | pack HD: `texturas/<hash>_<W>x<H>_PNG.png` |

`textura`: `entrada` (data_cmn por defecto, `afs = ...`), `textura` = indice, lista o `"todas"`,
`factor` (1-4), `efecto = "tenir"` (dibujo del original con el color medio del asset) o
`"sustituir"` (el asset con el alfa del original). En las auras (data_cmn 0-43) se saltan las
texturas compartidas con otras auras salvo `compartidas = true` (casi todas lo son: ver
`docs/14_frentes/assets_integracion.md`).
