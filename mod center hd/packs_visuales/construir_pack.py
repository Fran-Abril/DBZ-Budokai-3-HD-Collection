#!/usr/bin/env python3
"""construir_pack.py - packs visuales OPCIONALES con arte de terceros (se construyen en casa).

Lee una receta (.toml) que elige imagenes de la carpeta de assets externos (catalogada en
docs/13_assets_externos/indice.json), las recorta/escala al tamano que espera cada parte del
juego y deja una carpeta de mod lista en mods/:

  ui_ingame/<pieza>.png            pantallas propias (HUB, Tag Team, Online): las carga ya
                                   style::Img (mods/*/ui_ingame/ gana a la pieza del juego)
  campana/<historia>.toml + imgs   copia de una campana con sus imagenes cambiadas; --montar
                                   la monta con campana.py (sale el mod de la campana)
  personajes/<mod>/ui/*.png        cara.png / retrato.png / hud.png de un personaje importado
                                   (se copian a mods/<mod>/ui/ con --instalar-ui; roster_build)
  texturas/<hash>_<W>x<H>_PNG.png  pack de texturas HD (auras, ki...): hash = XXH3 del original

El arte de terceros NUNCA va en el release, el zip del Mod Kit ni el exe: esta carpeta solo
lleva codigo y recetas (tools/make_modpacks.py no la copia) y el pack se construye en el PC
de quien tiene los assets.

  python construir_pack.py receta.toml --mods <carpeta mods> [--raiz <assets>] [--juego <us/>]
                           [--montar] [--instalar-ui]
  python construir_pack.py --prueba
"""
import argparse
import datetime
import json
import os
import re
import shutil
import subprocess
import sys
import tomllib

from PIL import Image, ImageChops

HERE = os.path.dirname(os.path.abspath(__file__))
MCH = os.path.dirname(HERE)                       # mod center hd/
ROOT = os.path.dirname(MCH)
INDICE = os.path.join(ROOT, "docs", "13_assets_externos", "indice.json")
IMG_EXT = (".png", ".jpg", ".jpeg", ".webp", ".gif", ".avif", ".bmp", ".dds")

# Tamanos que espera cada destino (medidos en el codigo que los usa; ver assets_integracion.md).
TIPOS = {
    "narracion":      {"modo": "llenar", "tamano": [660, 420], "opaco": True},     # png2pic.build_picture
    "retrato_campana": {"modo": "recortar", "tamano": [1082, 1060]},               # campana.portraits: caja 541x530 (x2)
    "fondo_hub":      {"modo": "llenar", "tamano": [1920, 1080], "opaco": True},
    "fondo_duelo":    {"modo": "llenar", "tamano": [1600, 1200], "opaco": True},   # style::Backdrop (4:3)
    "banner":         {"modo": "llenar", "tamano": [1024, 256]},
    "render":         {"modo": "recortar", "tamano": [1024, 1024]},
    "anfitrion":      {"modo": "recortar", "tamano": [420, 460]},                 # style::Host lo pinta x1.33
    "cara_hud":       {"modo": "ajustar", "tamano": [256, 128]},                  # roster_build hud.png / caras/<ID>
    "cara":           {"modo": "recortar", "tamano": [512, 512]},                 # roster_build ui/cara.png
    "retrato_select": {"modo": "recortar", "tamano": [1024, 1024]},               # roster_build ui/retrato.png
    "textura":        {"modo": "textura"},
}


# ---------------------------------------------------------------- imagen
def cargar(path, fotograma=0):
    im = Image.open(path)
    if getattr(im, "n_frames", 1) > 1:
        im.seek(min(int(fotograma), im.n_frames - 1))
    return im.convert("RGBA")


def llenar(im, w, h, foco=(0.5, 0.5)):
    """Escala hasta cubrir w x h y recorta alrededor de `foco` (0..1 de la imagen)."""
    k = max(w / im.width, h / im.height)
    im = im.resize((max(w, round(im.width * k)), max(h, round(im.height * k))), Image.LANCZOS)
    x = round((im.width - w) * float(foco[0]))
    y = round((im.height - h) * float(foco[1]))
    return im.crop((x, y, x + w, y + h))


def recortar(im, w, h):
    """Quita los margenes transparentes y reduce (nunca agranda) hasta caber en w x h."""
    bb = im.getchannel("A").getbbox()
    if bb:
        im = im.crop(bb)
    k = min(1.0, w / im.width, h / im.height)
    if k < 1.0:
        im = im.resize((max(1, round(im.width * k)), max(1, round(im.height * k))), Image.LANCZOS)
    return im


def ajustar(im, w, h, fondo=None, abajo=False, escala=1.0):
    """Encaja entera en w x h sobre `fondo` (imagen w x h o transparente)."""
    im = recortar(im, 1 << 15, 1 << 15)
    k = min(w / im.width, h / im.height) * float(escala)
    im = im.resize((max(1, round(im.width * k)), max(1, round(im.height * k))), Image.LANCZOS)
    canvas = fondo.copy() if fondo is not None else Image.new("RGBA", (w, h), (0, 0, 0, 0))
    layer = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    layer.paste(im, ((w - im.width) // 2, h - im.height if abajo else (h - im.height) // 2))   # recorta si escala > 1
    canvas.alpha_composite(layer)
    return canvas


def color(s):
    s = s.lstrip("#")
    return tuple(int(s[i:i + 2], 16) for i in (0, 2, 4)) + (255,)


def convertir(im, cfg, fondo_img=None):
    """Imagen final de un [[asset]] (sin el tipo textura)."""
    w, h = (int(v) for v in cfg["tamano"])
    modo = cfg["modo"]
    if modo == "llenar":
        out = llenar(im, w, h, cfg.get("foco", (0.5, 0.5)))
        if fondo_img is not None:
            fondo_img.alpha_composite(out)
            out = fondo_img
    elif modo == "ajustar":
        out = ajustar(im, w, h, fondo_img, cfg.get("alinear") == "abajo", cfg.get("escala", 1.0))
    elif modo == "recortar":
        out = recortar(im, w, h)
    else:
        raise SystemExit("modo desconocido: %s" % modo)
    if cfg.get("opaco"):
        base = Image.new("RGBA", out.size, (0, 0, 0, 255))
        base.alpha_composite(out)
        out = base
    return out


def tenir(orig, ref):
    """Mismo dibujo (luz y alfa) que la textura original, con el color medio de `ref`."""
    import numpy as np  # noqa: PLC0415
    a = np.asarray(orig, dtype=np.float32)
    r = np.asarray(ref.convert("RGBA"), dtype=np.float32)
    mask = r[..., 3] > 32
    tint = r[mask][:, :3].mean(axis=0) if mask.any() else np.array([255.0, 255.0, 255.0])
    tint = tint / max(1.0, tint.max())
    lum = a[..., :3].max(axis=2, keepdims=True)          # brillo del original (las auras son casi grises)
    out = a.copy()
    out[..., :3] = np.clip(lum * tint, 0, 255)
    return Image.fromarray(out.astype(np.uint8))


# ---------------------------------------------------------------- texturas del juego
AURAS = range(0, 44)   # data_cmn 0-43: un bin de aura/ki por personaje (texturas_etiquetas.json)


def texturas_entrada(juego, afs, entrada, decodificar=True):
    """[(hash, ancho, alto, imagen RGBA o None)] de las texturas sustituibles de una entrada."""
    sys.path.insert(0, MCH)
    sys.path.insert(0, os.path.join(ROOT, "awo_tools"))
    import io  # noqa: PLC0415

    import afs_pair  # noqa: PLC0415
    import texturas_originales as to  # noqa: PLC0415
    import xxhash  # noqa: PLC0415
    b = afs_pair.decompress(afs_pair.entry(os.path.join(juego, afs), int(entrada)), "%s_%d" % (afs, int(entrada)))
    return [(xxhash.xxh3_64_intdigest(base), w, h,
             Image.open(io.BytesIO(to.to_png(b, off))).convert("RGBA") if decodificar else None)
            for off, w, h, _, base in to.dds_textures(b)]


def texturas_elegidas(juego, a):
    """Texturas de un [[asset]] tipo textura: `textura` = indice, lista o "todas". En las auras
    (data_cmn 0-43) se saltan las que comparte con otras auras (el pack va por hash: cambiarlas
    cambiaria el aura de todos), salvo `compartidas = true`."""
    afs, entrada = a.get("afs", "data_cmn.afs"), int(a["entrada"])
    tx = texturas_entrada(juego, afs, entrada)
    sel = a.get("textura", 0)
    idx = range(len(tx)) if sel == "todas" else [int(i) for i in (sel if isinstance(sel, list) else [sel])]
    bad = [i for i in idx if not 0 <= i < len(tx)]
    if bad:
        raise SystemExit("%s %d: tiene %d texturas, no hay %s" % (afs, entrada, len(tx), bad))
    otras = set()
    if afs == "data_cmn.afs" and entrada in AURAS and not a.get("compartidas"):
        for n in AURAS:
            if n != entrada:
                otras.update(t[0] for t in texturas_entrada(juego, afs, n, False))
    out = [tx[i] for i in idx if tx[i][0] not in otras]
    if len(out) < len(idx):
        print("textura: %d de %d compartidas con otras auras, se dejan como estan" % (len(idx) - len(out), len(idx)))
    return out


# ---------------------------------------------------------------- receta
def autor(path):
    m = re.search(r" by (.+?) - \d+\.\w+$", os.path.basename(path))
    return m.group(1) if m else "desconocido"


def raiz_por_defecto():
    try:
        return json.load(open(INDICE, encoding="utf-8"))["raiz"]
    except (OSError, KeyError, ValueError):
        return ""


def comprobar_destino(mods):
    """El arte de terceros no puede acabar en una carpeta que se publica: dentro del proyecto
    solo se admite out/ (el juego de pruebas; su mods/ no entra en el release)."""
    m = os.path.normcase(os.path.abspath(mods))
    r = os.path.normcase(ROOT)
    if (m == r or m.startswith(r + os.sep)) and not m.startswith(os.path.join(r, "out") + os.sep):
        raise SystemExit("destino no permitido (se publica con el proyecto): %s" % mods)


def respaldar(path, mods):
    """Nunca se borra: lo que ya hubiera va a mods_respaldo/<nombre>_<fecha> (junto a mods/, fuera de ella:
    el cargador de packs de texturas recorre las subcarpetas de mods/)."""
    if not os.path.exists(path):
        return
    dst = os.path.join(os.path.dirname(os.path.abspath(mods)), "mods_respaldo", "%s_%s" % (os.path.basename(path),
                                                    datetime.datetime.now().strftime("%Y%m%d_%H%M%S")))
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.move(path, dst)
    print("respaldo: %s -> %s" % (path, dst))


def construir(receta, mods, raiz=None, juego=None, montar=False, instalar_ui=False):
    cfg = tomllib.load(open(receta, "rb"))
    info = cfg.get("pack", {})
    nombre = info.get("nombre") or os.path.splitext(os.path.basename(receta))[0]
    raiz = raiz or info.get("raiz") or raiz_por_defecto()
    comprobar_destino(mods)
    os.makedirs(mods, exist_ok=True)
    out = os.path.join(mods, nombre)
    respaldar(out, mods)
    os.makedirs(out)
    camp = cfg.get("campana")
    toml_txt, toml_out = None, None
    if camp:
        src = camp["toml"] if os.path.isabs(camp["toml"]) else os.path.join(MCH, camp["toml"])
        toml_txt = open(src, encoding="utf-8").read()
        toml_out = os.path.join(out, "campana", os.path.basename(src))
    creditos, hechos, ui_mods = [], 0, set()
    for a in cfg.get("asset", []):
        t = dict(TIPOS.get(a.get("tipo", ""), {}))
        t.update({k: v for k, v in a.items() if k not in ("tipo",)})
        if "modo" not in t:
            raise SystemExit("asset %s: falta `tipo` o `modo`" % a.get("destino"))
        fuente = a["fuente"] if os.path.isabs(a["fuente"]) else os.path.join(raiz, a["fuente"])
        if not os.path.isfile(fuente):
            raise SystemExit("no encuentro el asset: %s" % fuente)
        im = cargar(fuente, a.get("fotograma", 0))
        salidas = []
        if t["modo"] == "textura":
            f = int(a.get("factor", 4))
            for hh, w, h, orig in texturas_elegidas(juego or default_us(), a):
                orig = orig.resize((w * f, h * f), Image.LANCZOS)
                if a.get("efecto", "tenir") == "tenir":
                    img = tenir(orig, im)
                else:                                   # sustituir: el asset, con la forma del original
                    img = llenar(im, w * f, h * f, a.get("foco", (0.5, 0.5)))
                    if a.get("alfa", "original") == "original":
                        img.putalpha(ImageChops.darker(img.getchannel("A"), orig.getchannel("A")))
                salidas.append((os.path.join(a.get("destino", "texturas"),
                                             "%016X_%dx%d_PNG.png" % (hh, w * f, h * f)), img))
        else:
            fondo = None
            if t.get("fondo"):
                w, h = (int(v) for v in t["tamano"])
                if str(t["fondo"]).startswith("#"):
                    fondo = Image.new("RGBA", (w, h), color(t["fondo"]))
                else:
                    fp = t["fondo"] if os.path.isabs(t["fondo"]) else os.path.join(raiz, t["fondo"])
                    fondo = llenar(cargar(fp), w, h, t.get("foco_fondo", (0.5, 0.5)))
                    creditos.append("%s (fondo) <- %s  [%s]" % (a["destino"], t["fondo"], autor(fp)))
            salidas.append((a["destino"], convertir(im, t, fondo)))
        for rel, img in salidas:
            dst = os.path.join(out, rel)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            img.save(dst)
            creditos.append("%s <- %s  [%s]" % (rel.replace(os.sep, "/"), a["fuente"], autor(fuente)))
            hechos += 1
            m = re.match(r"personajes[/\\]([^/\\]+)[/\\]ui[/\\]", rel)
            if m:
                ui_mods.add(m.group(1))
        if a.get("reemplaza"):
            if toml_txt is None:
                raise SystemExit("asset %s: `reemplaza` necesita [campana] toml" % rel)
            nuevo = os.path.relpath(dst, os.path.dirname(toml_out)).replace(os.sep, "/")
            viejo = '"%s"' % a["reemplaza"]
            if viejo not in toml_txt:
                raise SystemExit("%s no aparece en %s" % (viejo, camp["toml"]))
            toml_txt = toml_txt.replace(viejo, '"%s"' % nuevo)
    if toml_txt is not None:
        os.makedirs(os.path.dirname(toml_out), exist_ok=True)
        open(toml_out, "w", encoding="utf-8").write(
            "# Copia de %s con las imagenes del pack %s (construir_pack.py).\n" % (camp["toml"], nombre) + toml_txt)
    solo_fuente = not any(os.path.isdir(os.path.join(out, d)) for d in ("ui_ingame", "texturas"))
    with open(os.path.join(out, "manifest.txt"), "w", encoding="utf-8") as f:
        f.write("name=%s\ntype=pack_visual\nversion=%s\ndescription=%s\n" % (
            info.get("titulo", nombre), info.get("version", "1.0"),
            info.get("descripcion", "Pack visual opcional con arte de terceros (no se distribuye con el juego).")))
    with open(os.path.join(out, "CREDITOS.txt"), "w", encoding="utf-8") as f:
        f.write("Arte de terceros, solo para uso personal. Este pack se construye en tu PC con\n"
                "construir_pack.py; no lo subas ni lo metas en el juego publicado.\n\n" + "\n".join(creditos) + "\n")
    # [pack] desactivado = true: sale con .disabled aunque traiga piezas (arte de terceros: lo activa el usuario)
    if solo_fuente or info.get("desactivado", False):   # sin nada que cargue el juego: fuente de otros
        open(os.path.join(out, ".disabled"), "w").close()
    print("pack %s: %d imagenes en %s" % (nombre, hechos, out))
    if toml_out and montar:
        cmd = [sys.executable, os.path.join(MCH, "campana.py"), "--mods", mods]
        if juego:
            cmd += ["--us", juego]
        modname = tomllib.loads(toml_txt).get("campana", {}).get("mod") or os.path.splitext(os.path.basename(toml_out))[0]
        respaldar(os.path.join(mods, modname), mods)
        r = subprocess.run(cmd + ["montar", toml_out], capture_output=True, text=True, encoding="utf-8", errors="replace")
        print(r.stdout[-2000:], r.stderr[-2000:])
        if r.returncode != 0:
            raise SystemExit("campana.py montar fallo (%d)" % r.returncode)
    elif toml_out:
        print("campana: monta %s en el Mod Kit (Historias) o con --montar" % toml_out)
    for m in sorted(ui_mods):
        src = os.path.join(out, "personajes", m, "ui")
        if not instalar_ui:
            print("ui de %s: copia %s a mods/%s/ui/ (o usa --instalar-ui) y reconstruye el roster" % (m, src, m))
            continue
        dst = os.path.join(mods, m, "ui")
        if not os.path.isdir(dst):
            raise SystemExit("no existe el mod %s en %s" % (m, mods))
        for fn in os.listdir(src):
            if os.path.exists(os.path.join(dst, fn)):
                bk = os.path.join(dst, "_respaldo")
                os.makedirs(bk, exist_ok=True)
                shutil.move(os.path.join(dst, fn), os.path.join(bk, fn))
            shutil.copyfile(os.path.join(src, fn), os.path.join(dst, fn))
        print("ui de %s instalada (los anteriores en ui/_respaldo); reconstruye el roster" % m)
    return out


def default_us():
    sys.path.insert(0, MCH)
    import roster_build as rb  # noqa: PLC0415
    return rb.default_us()


# ---------------------------------------------------------------- prueba
def prueba():
    import tempfile  # noqa: PLC0415
    tmp = tempfile.mkdtemp(prefix="packs_visuales_")
    # 1) conversiones con imagenes sinteticas (nada de terceros)
    sujeto = Image.new("RGBA", (300, 500), (0, 0, 0, 0))
    sujeto.paste((200, 30, 30, 255), (100, 50, 200, 450))          # dibujo en 100,50-200,450
    paisaje = Image.new("RGBA", (1000, 400), (20, 120, 220, 255))
    assert recortar(sujeto, 9999, 9999).size == (100, 400)
    assert recortar(sujeto, 50, 50).size == (12, 50)
    assert llenar(paisaje, 660, 420).size == (660, 420)
    n = convertir(sujeto, dict(TIPOS["narracion"]))
    assert n.size == (660, 420) and n.getchannel("A").getextrema() == (255, 255)
    c = convertir(sujeto, dict(TIPOS["cara_hud"]))
    assert c.size == (256, 128) and c.getpixel((0, 0))[3] == 0
    t = tenir(Image.new("RGBA", (4, 4), (128, 128, 128, 77)), Image.new("RGBA", (2, 2), (0, 0, 255, 255)))
    assert t.getpixel((0, 0)) == (0, 0, 128, 77), t.getpixel((0, 0))
    # 2) receta completa sobre una raiz falsa: ui_ingame, campana (toml reescrito), personaje
    raiz = os.path.join(tmp, "assets")
    os.makedirs(os.path.join(raiz, "Z"))
    sujeto.save(os.path.join(raiz, "Z", "[2020] Freezer render by Alguien - 123.png"))
    paisaje.save(os.path.join(raiz, "Z", "fondo.png"))
    camp = os.path.join(tmp, "historia.toml")
    open(camp, "w", encoding="utf-8").write('[campana]\nmod = "h"\n[[imagen]]\nentrada = 1\narchivo = "imagenes/x.avif"\n')
    receta = os.path.join(tmp, "receta.toml")
    open(receta, "w", encoding="utf-8").write(
        '[pack]\nnombre = "pack_prueba"\n[campana]\ntoml = %s\n'
        '[[asset]]\nfuente = "Z/fondo.png"\ndestino = "ui_ingame/fondo_duelo.png"\ntipo = "fondo_duelo"\n'
        '[[asset]]\nfuente = "Z/[2020] Freezer render by Alguien - 123.png"\ndestino = "campana/imagenes/a.png"\n'
        'tipo = "narracion"\nmodo = "ajustar"\nfondo = "Z/fondo.png"\nalinear = "abajo"\nreemplaza = "imagenes/x.avif"\n'
        '[[asset]]\nfuente = "Z/[2020] Freezer render by Alguien - 123.png"\ndestino = "personajes/cut_x/ui/cara.png"\ntipo = "cara"\n'
        % json.dumps(camp))
    mods = os.path.join(tmp, "mods")
    out = construir(receta, mods, raiz=raiz)
    assert Image.open(os.path.join(out, "ui_ingame", "fondo_duelo.png")).size == (1600, 1200)
    nar = Image.open(os.path.join(out, "campana", "imagenes", "a.png"))
    assert nar.size == (660, 420) and nar.getpixel((5, 5))[:3] == (20, 120, 220)     # fondo, no negro
    assert Image.open(os.path.join(out, "personajes", "cut_x", "ui", "cara.png")).size == (100, 400)
    txt = open(os.path.join(out, "campana", "historia.toml"), encoding="utf-8").read()
    assert 'archivo = "imagenes/a.png"' in txt and "x.avif" not in txt
    assert "Alguien" in open(os.path.join(out, "CREDITOS.txt"), encoding="utf-8").read()
    assert not os.path.exists(os.path.join(out, ".disabled"))      # trae ui_ingame: mod activo
    out2 = construir(receta, mods, raiz=raiz)                        # repetir: lo anterior va a _respaldo
    assert out2 == out and os.listdir(os.path.join(tmp, "mods_respaldo"))
    # 3) destinos prohibidos: dentro del proyecto solo out/
    for bad in (os.path.join(MCH, "x"), os.path.join(ROOT, "res", "ingame_ui")):
        try:
            comprobar_destino(bad)
            raise AssertionError("destino permitido: " + bad)
        except SystemExit:
            pass
    comprobar_destino(os.path.join(ROOT, "out", "build", "win-amd64-release", "mods"))
    # 4) el Mod Kit no se lleva esta carpeta ni imagenes
    sys.path.insert(0, os.path.join(ROOT, "tools"))
    import make_modpacks  # noqa: PLC0415
    assert not [p for p in make_modpacks.kit_modules() if os.path.normcase(HERE) in os.path.normcase(p)]
    src = open(os.path.join(ROOT, "tools", "make_modpacks.py"), encoding="utf-8").read()
    assert "packs_visuales" not in src
    imgs = [os.path.join(dp, f) for dp, _, fs in os.walk(HERE) for f in fs if f.lower().endswith(IMG_EXT)]
    assert not imgs, "imagenes dentro de packs_visuales: %s" % imgs
    # 5) textura de aura con el juego (si esta): nombre = hash del original, tamano x factor
    us = default_us()
    if os.path.isfile(os.path.join(us, "data_cmn.afs")):
        hh, w, h, orig = texturas_entrada(us, "data_cmn.afs", 23)[3]
        assert hh == 0x861EBAAB04C2605A and (w, h) == (128, 128) and orig.size == (128, 128)
        # la 1 (2F2651D218602351) la comparten 42 auras: se salta; la 13 solo es de Goku
        open(receta, "w", encoding="utf-8").write(
            '[pack]\nnombre = "aura"\n[[asset]]\nfuente = "Z/fondo.png"\ntipo = "textura"\nentrada = 23\ntextura = [1, 13]\nfactor = 2\n')
        out = construir(receta, mods, raiz=raiz, juego=us)
        assert os.listdir(os.path.join(out, "texturas")) == ["25069C5BA2D99015_128x128_PNG.png"]
        sys.path.insert(0, MCH)
        import texture_pack  # noqa: PLC0415
        assert texture_pack.parse_name("25069C5BA2D99015_128x128_PNG")
        print("textura de aura: OK")
    else:
        print("textura de aura: sin juego, omitida")
    print("prueba OK (%s)" % tmp)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("receta", nargs="?")
    p.add_argument("--mods", help="carpeta mods/ donde dejar el pack")
    p.add_argument("--raiz", help="carpeta de los assets (por defecto la de indice.json)")
    p.add_argument("--juego", help="carpeta us/ del juego (texturas y --montar)")
    p.add_argument("--montar", action="store_true", help="monta la campana del pack con campana.py")
    p.add_argument("--instalar-ui", action="store_true", help="copia personajes/<mod>/ui a mods/<mod>/ui")
    p.add_argument("--prueba", action="store_true")
    a = p.parse_args()
    if a.prueba:
        return prueba()
    if not a.receta or not a.mods:
        p.error("hacen falta la receta y --mods")
    construir(a.receta, a.mods, a.raiz, a.juego, a.montar, a.instalar_ui)


if __name__ == "__main__":
    main()
