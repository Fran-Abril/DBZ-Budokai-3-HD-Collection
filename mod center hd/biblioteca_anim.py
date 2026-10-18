#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""biblioteca_anim.py - Biblioteca de clips de animacion de los personajes del Budokai 3 HD (solo lectura).

Lista cada clip de cada personaje del roster (id, nombre, codigo BSK, indice de animacion, n de
fotogramas) leyendo us/data_cmn.afs del jugador con los lectores del proyecto (afs_pair, studio_core,
altura). Nunca escribe en us/ ni en mods/.

  python biblioteca_anim.py listar [--texto goku] [--categoria ataque] [--json]
  python biblioteca_anim.py resumen
  python biblioteca_anim.py receta-guardar --destino-personaje 0 --destino-codigo 0x21b \\
         --fuente-personaje 5 --fuente-codigo 0x21b --out receta.toml [--nombre "Goku con golpe de Saiyaman"]
  python biblioteca_anim.py selftest

Mapeo (RE del proyecto, docs/03_formatos/ACM_FORMAT.md y awo_tools/altura.py:code_anim):
  #CSK (moveset) -> cabecera +0x10 = n de codigos, +0x14 = tabla de direcciones (4 B por codigo).
  Cada bloque empieza con [anim u16][pool u16]. pool == 3 -> anim es el indice de un clip del
  #ACM del propio bin (el primer hijo de tipo 3). Otros pools no se resuelven aqui (frames = None).
  #ACM -> +0x10 = n de clips, +0x14 = tabla de 16 B por clip [flags, variante, n_fotogramas, offset].

Categorias: INFERIDAS de los datos del roster (roster_db.json, skills.ataques / transformaciones) y de
rangos documentados. No hay una tabla oficial de clips por categoria: "ki", "golpe recibido" y "dash"
no se pueden inferir de los codigos BSK y no aparecen.
"""
import argparse
import json
import os
import struct
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
_DEF_ROOT = "C:/Users/javie/Desktop/PROYECTOS IA/DBZ Budokai 3 HD Collection"


def _bootstrap():
    """Encuentra la raiz del proyecto (env DBZ3_ROOT, carpeta padre de este fichero si esta dentro de
    'mod center hd', o la ruta por defecto) y deja importables studio_core, afs_pair y altura."""
    root = os.environ.get("DBZ3_ROOT")
    if not root:
        here_mch = HERE if os.path.isdir(os.path.join(HERE, "studio")) else None
        root = os.path.dirname(here_mch) if here_mch else _DEF_ROOT
    mch = os.path.join(root, "mod center hd")
    awo = os.path.join(root, "awo_tools")
    for p in (os.path.join(mch, "studio"), awo, mch):
        if p not in sys.path:
            sys.path.append(p)
    return root, mch


ROOT, MCH = _bootstrap()
import studio_core as sc  # noqa: E402  (amb_kids, code_info, anim_pool, anim_locals, hd_entry, us_dir)

CATEGORIAS = ("reposo", "movimiento", "ataque", "agarre", "transformacion", "otro", "sin codigo")
GROUND_CODES = (2, 15, 16, 17)     # altura.py: codigos de andar por el suelo (pies apoyados)
AGARRE_RANGO = (0x480, 0x4C0)       # SB_VS_B3_MOVESET.md: agarre BASE 0x480 en B3 (techo inferido)
ATAQUE_RANGO = (0x200, 0x500)       # docs: codigos de ataque 0x200-0x4FF (tec_sb.py / spx_graph.py)

_cache = {}


# ------------------------------------------------------------------ roster
def personajes(mch=MCH):
    """[{id, nombre, anm, modelo, ataques, transformaciones}] de roster_db.json (solo los que tienen moveset)."""
    with open(os.path.join(mch, "roster_db.json"), encoding="utf-8") as fh:
        db = json.load(fh)
    out = []
    for e in db["ids"]:
        anm = next((a for a in e.get("anm") or [] if a < 0xFFFFFFFF and a), None)
        if anm is None:
            continue
        sk = e.get("skills") or {}
        out.append(dict(id=e["id"], nombre=e["name"].title(), anm=anm,
                        modelo=(e.get("models") or [None])[0],
                        ataques=set(sk.get("ataques") or []),
                        transformaciones=set(sk.get("transformaciones") or [])))
    return out


# ------------------------------------------------------------------ categorias
def categoria(code, ataques=(), transformaciones=()):
    """Categoria inferida de un codigo BSK (ver la cabecera del modulo)."""
    if code in transformaciones:
        return "transformacion"
    if code in ataques:
        return "ataque"
    if code == 0:
        return "reposo"
    if code in GROUND_CODES:
        return "movimiento"
    if AGARRE_RANGO[0] <= code < AGARRE_RANGO[1]:
        return "agarre"
    if ATAQUE_RANGO[0] <= code < ATAQUE_RANGO[1]:
        return "ataque"
    return "otro"


# ------------------------------------------------------------------ lectura de un personaje
def frames_de(acm, e, anim):
    """Numero de fotogramas del clip `anim` del #ACM, o None si no existe."""
    n, tbl = struct.unpack_from(e + "II", acm, 0x10)
    if anim >= n:
        return None
    _flags, _var, nf, off = struct.unpack_from(e + "4I", acm, tbl + 16 * anim)
    return nf if off else None


def clips_de(p, us=None):
    """Filas de un personaje: una por codigo BSK con su clip, mas los clips del pool sin codigo.
    Lee la entrada HD de data_cmn.afs (descomprimida, con cache de afs_pair)."""
    anm = sc.hd_entry(p["anm"], us)
    csk, e = sc.csk_of(anm)
    acm, _ = sc.anim_pool(anm)
    filas, usados = [], set()
    if csk:
        n = struct.unpack_from(e + "I", csk, 0x10)[0]
        for code in range(n):
            info = sc.code_info(csk, e, code)
            if info is None:
                continue
            anim, pool, _marks = info
            fr = frames_de(acm, e, anim) if (acm and pool == 3) else None
            if pool == 3:
                usados.add(anim)
            filas.append(_fila(p, code, anim if pool == 3 else None, pool, fr))
    if acm:
        for anim in range(sc.anim_count(acm, e)):
            if anim not in usados:
                filas.append(_fila(p, None, anim, 3, frames_de(acm, e, anim)))
    return filas


def _fila(p, code, anim, pool, frames):
    cat = "sin codigo" if code is None else categoria(code, p["ataques"], p["transformaciones"])
    return dict(personaje=p["id"], nombre=p["nombre"], fid=p["anm"], codigo=code, anim=anim, pool=pool,
                frames=frames, categoria=cat)


def biblioteca(us=None, refrescar=False):
    """Todas las filas (cache en memoria). `us` = carpeta us/ del juego (por defecto la del proyecto)."""
    key = os.path.abspath(sc.us_dir(us))
    if refrescar or key not in _cache:
        filas = []
        for p in personajes():
            filas += clips_de(p, us)
        _cache[key] = filas
    return _cache[key]


# ------------------------------------------------------------------ busqueda
# Nombres de jugador (es / en) de los IDs de B3: el roster usa los internos ("GOKOU"); asi "goku" o "krilin" encuentran.
NOMBRES = {0: "goku", 2: "gohan nino kid gohan", 3: "gohan adolescente teen gohan", 4: "gohan", 5: "gran saiyaman great",
           6: "goten", 7: "vegeta", 8: "trunks", 9: "trunks nino kid trunks", 10: "krilin krillin", 11: "piccolo",
           12: "ten shin han tien", 13: "yamcha", 14: "mr satan hercule", 15: "videl", 16: "kaio-shin supreme kai",
           18: "raditz", 19: "nappa", 20: "ginyu", 21: "recoome", 27: "freezer frieza", 28: "androide 16 android",
           29: "androide 17 android", 30: "androide 18 android", 32: "dr gero", 33: "cell", 34: "majin bu buu",
           35: "super bu buu", 36: "bu nino kid buu", 37: "dabura", 38: "cooler", 42: "saibaman", 43: "cell jr"}


def buscar(filas, texto="", categoria_=None, personaje=None):
    """Filtra por texto libre (nombre, id, codigo en hex, indice de clip, categoria) y/o categoria."""
    t = (texto or "").strip().lower()
    hex_t = t[2:] if t.startswith("0x") else t
    out = []
    for f in filas:
        if categoria_ and f["categoria"] != categoria_:
            continue
        if personaje is not None and f["personaje"] != personaje:
            continue
        if t:
            code_hex = "%x" % f["codigo"] if f["codigo"] is not None else ""
            campos = [f["nombre"].lower(), NOMBRES.get(f["personaje"], ""), str(f["personaje"]), f["categoria"],
                      str(f["anim"]) if f["anim"] is not None else "", code_hex]
            if not any(t == c or (hex_t and hex_t == c) or (len(t) > 2 and t in c) for c in campos):
                continue
        out.append(f)
    return out


def formato_fila(f):
    cod = "%#05x" % f["codigo"] if f["codigo"] is not None else "   -  "
    anim = "%4d" % f["anim"] if f["anim"] is not None else "   -"
    fr = "%5s" % (f["frames"] if f["frames"] is not None else "-")
    banco = "" if f["pool"] in (3, None) else "  (banco %d: no resuelto)" % f["pool"]
    return "%3d  %-16s %s  anim %s  %s fr  %s%s" % (f["personaje"], f["nombre"][:16], cod, anim, fr,
                                                   f["categoria"], banco)


# ------------------------------------------------------------------ recetas
def guardar_receta(path, destino, fuente, nombre="", notas=""):
    """Escribe una receta .toml: 'destino' = accion a reemplazar, 'fuente' = de donde sale la animacion.
    destino/fuente: dict(personaje=id, codigo=int, anim=int|None). Escritura TOML a mano (sin dependencias)."""
    for nom, d in (("destino", destino), ("fuente", fuente)):
        if d.get("codigo") is None:
            raise ValueError("%s sin codigo BSK: solo se puede recetar una accion con codigo" % nom)
    L = ["# Receta de la biblioteca de animaciones (no modifica nada por si sola)", "version = 1",
         'nombre = "%s"' % nombre.replace('"', "'"), 'notas = "%s"' % notas.replace('"', "'"), "",
         "[destino]", "personaje = %d" % destino["personaje"], "codigo = %#x" % destino["codigo"],
         "anim = %d" % destino["anim"] if destino.get("anim") is not None else "# anim = (sin clip)", "",
         "[fuente]", "personaje = %d" % fuente["personaje"], "codigo = %#x" % fuente["codigo"],
         "anim = %d" % fuente["anim"] if fuente.get("anim") is not None else "# anim = (sin clip)", ""]
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(L))
    return path


def leer_receta(path):
    import tomllib  # noqa: PLC0415
    with open(path, "rb") as fh:
        r = tomllib.load(fh)
    if r.get("version") != 1 or "destino" not in r or "fuente" not in r:
        raise ValueError("%s no es una receta v1 (faltan [destino] o [fuente])" % path)
    return r


def validar_receta(receta, filas):
    """Comprueba que destino y fuente existen en la biblioteca. -> lista de problemas (vacia = ok)."""
    problemas = []
    for nom in ("destino", "fuente"):
        d = receta[nom]
        if not any(f["personaje"] == d["personaje"] and f["codigo"] == d["codigo"] for f in filas):
            problemas.append("%s: personaje %s, codigo %#x no existe en el juego" % (nom, d["personaje"], d["codigo"]))
    return problemas


def aplicar_receta(receta, mod_dir):
    """NO IMPLEMENTADO (a proposito). Para aplicarla hace falta, verificado antes:
      1. Sustituir el bloque de animacion del codigo destino en el #CSK del ANM del personaje destino
         (csk_graft / csk_set_anim de capsulas.py, mismo patron que roster_build.csk_copy_codes).
      2. Insertar en el #ACM del destino los datos de pose del clip fuente. Como los esqueletos de
         personajes distintos no coinciden, habria que remapear huesos (altura.py) y verificar que el
         tamano del #ACM cambia en multiplos aceptados por amb_rebuild y por el runtime.
      3. Guardar el ANM en mod_dir/us/data_cmn.afs/<fid>/ (como campana.write_entry / roster_build.write_entry),
         con respaldo previo, sin tocar us/.
    Sin esos tres pasos verificados en juego, esta funcion no escribe nada."""
    raise NotImplementedError("aplicar_receta pendiente: ver el docstring (hace falta remapeo de huesos y "
                              "reemplazo de clip verificados en juego). Receta: %s -> %s" % (
                                  receta.get("fuente"), receta.get("destino")))


# ------------------------------------------------------------------ pose de un fotograma (para la pagina)
def render_fotograma(personaje_id, anim, frame, size=(320, 320), yaw=0.0, us=None, filas=None):
    """RGBA (H, W, 4) uint8 del modelo del personaje en la pose del clip `anim` en `frame`.
    Usa el primer modelo del personaje (traje 0). Pose estatica: sin reproduccion."""
    import math  # noqa: PLC0415
    import numpy as np  # noqa: PLC0415
    import altura  # noqa: PLC0415
    from model_render import Model, render, rot_y  # noqa: PLC0415
    p = next(x for x in personajes() if x["id"] == personaje_id)
    if p["modelo"] is None:
        raise ValueError("el personaje %s no tiene modelo" % personaje_id)
    mb = sc.hd_entry(p["modelo"], us)
    m, skel = Model(mb), altura.skeleton(mb)
    acm, e = sc.anim_pool(sc.hd_entry(p["anm"], us))
    m.local = sc.anim_locals(skel, acm, e, anim, frame)
    P, _ = m.posed(None)
    center = (P.min(0) + P.max(0)) / 2
    ext = float(np.max(P.max(0) - P.min(0))) or 1.0
    W, H = size
    scale = 0.92 * min(W, H) / ext
    return render(m, size=size, view=rot_y(yaw), center=center, scale=scale, ss=2, outline=1.2)


def frames_clip(personaje_id, anim, us=None):
    """Numero de fotogramas de un clip (lectura directa, sin renderizar)."""
    p = next(x for x in personajes() if x["id"] == personaje_id)
    acm, e = sc.anim_pool(sc.hd_entry(p["anm"], us))
    return frames_de(acm, e, anim)


# ------------------------------------------------------------------ CLI
def _selftest(us=None):
    filas = biblioteca(us)
    ok = []

    def chk(nombre, cond, extra=""):
        ok.append(bool(cond))
        print("[%s] %s %s" % ("OK" if cond else "FALLO", nombre, extra))

    per = sorted({f["personaje"] for f in filas})
    chk("personajes con clips", len(per) >= 30, "(%d)" % len(per))
    chk("filas totales", len(filas) > 500, "(%d)" % len(filas))
    gok = [f for f in filas if f["personaje"] == 0]
    chk("Goku tiene codigos BSK", any(f["codigo"] is not None for f in gok), "(%d filas)" % len(gok))
    chk("frames > 0 en clips con pool 3", all((f["frames"] or 0) > 0 for f in filas if f["pool"] == 3 and f["anim"] is not None),
        "(sin frames: %d)" % sum(1 for f in filas if f["pool"] == 3 and not f["frames"]))
    chk("categoria 0 = reposo", categoria(0) == "reposo")
    chk("busqueda por nombre", buscar(filas, "gokou", personaje=0) == gok and buscar(filas, "gokou"))
    chk("busqueda hex", all(f["codigo"] == 0x21B for f in buscar(filas, "0x21b")) and buscar(filas, "0x21b"))
    chk("filtro categoria", all(f["categoria"] == "ataque" for f in buscar(filas, categoria_="ataque")))
    with tempfile.TemporaryDirectory() as td:
        pth = os.path.join(td, "r.toml")
        d = dict(personaje=0, codigo=0x21B, anim=None)
        f_ = dict(personaje=5, codigo=0x21B, anim=None)
        ref = next((x for x in filas if x["codigo"] is not None and x["anim"] is not None), None)
        if ref:
            d.update(anim=ref["anim"]); d["personaje"] = ref["personaje"]; d["codigo"] = ref["codigo"]
            f_.update(anim=ref["anim"], personaje=ref["personaje"], codigo=ref["codigo"])
        guardar_receta(pth, d, f_, nombre="prueba", notas="selftest")
        r = leer_receta(pth)
        chk("receta ida y vuelta", r["destino"]["personaje"] == d["personaje"] and r["fuente"]["codigo"] == f_["codigo"])
        chk("receta valida contra la biblioteca", validar_receta(r, filas) == [], str(validar_receta(r, filas)))
    try:
        aplicar_receta({"fuente": {}, "destino": {}}, td)
        chk("aplicar_receta no escribe (NotImplementedError)", False)
    except NotImplementedError:
        chk("aplicar_receta no escribe (NotImplementedError)", True)
    print("RESULTADO: %d/%d OK" % (sum(ok), len(ok)))
    return all(ok)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--us", help="carpeta us/ del juego (por defecto la del proyecto)")
    sub = ap.add_subparsers(dest="cmd", required=True)
    l = sub.add_parser("listar")
    l.add_argument("--texto", default="")
    l.add_argument("--categoria", choices=CATEGORIAS)
    l.add_argument("--personaje", type=int)
    l.add_argument("--json", action="store_true")
    sub.add_parser("resumen")
    rg = sub.add_parser("receta-guardar")
    rg.add_argument("--destino-personaje", type=int, required=True)
    rg.add_argument("--destino-codigo", type=lambda s: int(s, 0), required=True)
    rg.add_argument("--fuente-personaje", type=int, required=True)
    rg.add_argument("--fuente-codigo", type=lambda s: int(s, 0), required=True)
    rg.add_argument("--out", required=True)
    rg.add_argument("--nombre", default="")
    sub.add_parser("selftest")
    a = ap.parse_args(argv)
    if a.cmd == "selftest":
        return 0 if _selftest(a.us) else 1
    filas = biblioteca(a.us)
    if a.cmd == "listar":
        res = buscar(filas, a.texto, a.categoria, a.personaje)
        if a.json:
            print(json.dumps(res, ensure_ascii=False, indent=1))
        else:
            for f in res:
                print(formato_fila(f))
            print("%d clips" % len(res))
    elif a.cmd == "resumen":
        for c in CATEGORIAS:
            print("%-16s %d" % (c, sum(1 for f in filas if f["categoria"] == c)))
        print("total %d clips, %d personajes" % (len(filas), len({f["personaje"] for f in filas})))
    elif a.cmd == "receta-guardar":
        d = dict(personaje=a.destino_personaje, codigo=a.destino_codigo)
        s = dict(personaje=a.fuente_personaje, codigo=a.fuente_codigo)
        for x, nom in ((d, "destino"), (s, "fuente")):
            row = next((f for f in filas if f["personaje"] == x["personaje"] and f["codigo"] == x["codigo"]), None)
            if row is None:
                ap.error("%s: no existe personaje %s codigo %#x" % (nom, x["personaje"], x["codigo"]))
            x["anim"] = row["anim"]
        guardar_receta(a.out, d, s, nombre=a.nombre)
        print("receta guardada en", a.out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
