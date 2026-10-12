#!/usr/bin/env python3
"""campana.py - Campanas de Dragon Universe con mods (Mod Kit).

En Budokai 3 cada linea de dialogo o de narracion de la historia es una IMAGEN por idioma
(#AZT en data_usi y data_eng/spn/fra/ger/ita, una textura por bocadillo). Las escenas son
contenedores #LST de data_cmn y cada combate un #BPC (el rival es el u32 de +0x10). El juego
elige la escena de cada "codigo de historia" con su propia tabla (du_codigos.json); el port
deja que un mod la cambie (campana.toml, seccion [codigos]). Ver docs/11_campanas/CAMPANAS.md.

  python campana.py ver 359                 # PNG de los textos de data_usi 359 (todos los idiomas)
  python campana.py escenas 5500            # escenas del bloque de historia 5500 y sus textos
  python campana.py montar mi_campana.toml  # crea el mod

mi_campana.toml:

  [campana]
  nombre = "La saga de Freezer"
  mod = "campana_freezer"            # carpeta en mods/

  [[texto]]                          # una entrada de textos de data_usi
  entrada = 359
  estilo = "dialogo"                 # "dialogo" (oscuro) o "narracion" (blanco con borde)
  en = ["Sorry... but you left yourself wide open.", "Why... you..."]
  es = ["Lo siento... pero has bajado la guardia.", "Tu..."]

  [[combate]]
  bpl = 708                          # #BPC de data_cmn
  rival = 27                         # ID del rival (27 = Freezer)

  [codigos]                          # escena del juego -> contenedor #LST (en tiempo de juego)
  5500 = 2138

  [rueda]                            # personajes de mas en la rueda de Dragon Universe
  personajes = [27]                  # (27 = Freezer: su casilla tiene historia propia en los datos)

  [[escena]]                         # eventos que se anaden a una entrada de un contenedor #LST
  contenedor = 1725                  # (p. ej. el paso de la intro al mapa que le falta a una historia)
  entrada = 22
  anadir = [[22, 0, 0], [44, 0, 5930]]   # [evento, p1, p2]
"""
import argparse
import io
import json
import os
import struct
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import numpy as np  # noqa: E402
from PIL import Image, ImageDraw  # noqa: E402

import capsulas
import png2pic  # noqa: E402
import roster_build as rb  # noqa: E402

try:
    import tomllib  # noqa: E402
except ImportError:  # Python < 3.11
    import tomli as tomllib  # type: ignore  # noqa: E402

LANG_AFS = {"en": ("data_usi.afs", "data_eng.afs"), "es": ("data_spn.afs",), "fr": ("data_fra.afs",),
            "de": ("data_ger.afs",), "it": ("data_ita.afs",)}
RIVAL_SUITS = {}   # [traje_rival] del campana.toml: palabra del combate -> traje del rival
START_FORMS = {}   # [forma_jugador] del campana.toml: palabra del combate -> forma inicial
STORY_CODES = {}   # [codigos_historia]: codigo -> contenedor, solo mientras se juega una historia de [rueda]
MAP_ANIMS = {}     # [animacion_mapa]: palabra -> ID cuyo vuelo del mapa se usa (Mecha Freezer: 38 Cooler)
MAP_SUITS = {}     # [traje_mapa]: palabra del combate -> traje en el mapa (Freezer 2 = en su nave)
MAP_MODELS = {}    # [modelo_mapa] del campana.toml: palabra del combate -> modelo con el que vuelas hacia el
TOML_DIR = [HERE]   # carpeta del .toml que se monta (las paginas .png van relativas a ella)


# ---------------------------------------------------------------- texto -> imagen
# Estilos medidos sobre los nativos (data_usi 359/1023 dialogo y mapa, 653/661 narracion, 1913 final).
# Cada historia puede cambiarlos con un bloque [historia.estilos.<nombre>] (mismas claves); `fuente`
# puede ser un .ttf junto al .toml o el nombre de una fuente de Windows.
STYLES = {
    "dialogo":   {"fuente": "arialbd.ttf", "tamano": 17, "interlineado": 22, "ancho": 205,
                  "color": [8, 8, 8], "borde": 0, "color_borde": [0, 0, 0]},
    "narracion": {"fuente": "verdanab.ttf", "tamano": 19, "interlineado": 41, "ancho": 290,
                  "color": [0, 40, 33], "borde": 2, "color_borde": [239, 235, 239]},
}
STYLES["final"] = dict(STYLES["narracion"])


def render(text, style):
    """Una linea que acaba en .png es una imagen propia (RGBA, relativa al .toml) y se usa tal cual.
    Si no, se dibuja con el estilo de STYLES (texto centrado, una linea cada `interlineado` px)."""
    if text.lower().endswith(".png"):           # pagina hecha a mano: la imagen tal cual
        path = text if os.path.isabs(text) else os.path.join(TOML_DIR[0], text)
        return np.array(Image.open(path).convert("RGBA"))
    st = STYLES.get(style, STYLES["dialogo"])
    font = os.path.join(TOML_DIR[0], st["fuente"])
    if not os.path.exists(font):
        font = os.path.join("C:/Windows/Fonts", st["fuente"])
    f, pitch, stroke = capsulas.load_font(font, int(st["tamano"])), int(st["interlineado"]), int(st["borde"])
    lines = capsulas._wrap(text, f, int(st["ancho"]))
    w = int(max(f.getlength(t) for t in lines)) + 8
    img = Image.new("RGBA", (max(16, w), max(16, pitch * len(lines) + 6)), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    for k, t in enumerate(lines):
        d.text((w // 2, 3 + pitch * k + pitch // 2), t, font=f, anchor="mm", fill=tuple(st["color"]) + (255,),
               stroke_width=stroke, stroke_fill=tuple(st["color_borde"]) + (255,))
    return np.array(img)


def text_bank(lines, style):
    """#AZT de dialogo: texturas 0..n-1 (DXT3, flags 0x21, +0x28 = 0x40 como las nativas)."""
    return capsulas.build_bank(0, {k: render(t, style) for k, t in enumerate(lines)}, flag28=0x40, tall=True)


# ---------------------------------------------------------------- lectura
def afs_path(us, name):
    return os.path.join(us, name)


def read_entry(us, afs, n, work):
    return rb.Afs(afs_path(us, afs), work).entry(n)


def textures(b):
    out = []
    for k in range(len(capsulas.azt_entries(b))):
        img = capsulas.azt_read(b, k)
        out.append(img)
    return out


def cmd_ver(a):
    us, work = a.us or rb.default_us(), tempfile.mkdtemp()
    out = a.salida or os.path.join(HERE, "campanas_textos")
    os.makedirs(out, exist_ok=True)
    for lang, afss in LANG_AFS.items():
        p = afs_path(us, afss[0])
        if not os.path.exists(p):
            continue
        b = read_entry(us, afss[0], a.entrada, work)
        if b[:4] != b"#AZT":
            print("data_usi %d no es un banco de textos (%r)" % (a.entrada, b[:4]))
            return 1
        for k, img in enumerate(textures(b)):
            if img is None:
                continue
            f = os.path.join(out, "%d_%s_%d.png" % (a.entrada, lang, k))
            Image.fromarray(img).save(f)
            print(f)
    return 0


def cmd_escenas(a):
    reg = json.load(open(os.path.join(HERE, "du_codigos.json"), encoding="utf-8"))
    codes = {int(k): v for k, v in reg["codigos"].items()}
    us, work = a.us or rb.default_us(), tempfile.mkdtemp()
    for code in range(a.bloque, a.bloque + 100):
        if code not in codes:
            continue
        cont = codes[code]
        b = read_entry(us, "data_cmn.afs", cont, work)
        texts = set()
        battles = []
        if b[:4] == b"#LST":
            n, toff, doff = struct.unpack_from("<3I", b, 4)
            table = [struct.unpack_from("<2I", b, toff + 8 * i) for i in range(n)]
            for i, (rel, cnt) in enumerate(table):
                nxt = table[i + 1][0] if i + 1 < n else len(b) - doff
                hsize = nxt - rel - 12 * cnt
                hsize = hsize if hsize in (16, 28) else 28
                for k in range(cnt):
                    o = doff + rel + hsize + 12 * k
                    if o + 12 > len(b):
                        break
                    op, p1, p2 = struct.unpack_from("<3I", b, o)
                    if op == 10 and (p2 >> 16) & 0xFF == 1:
                        texts.add(p2 & 0xFFFF)
                    if op == 16:
                        battles.append(p2)
        print("codigo %d -> contenedor %d  textos data_usi %s  combates %s" % (
            code, cont, sorted(texts) or "-", battles or "-"))
    return 0


# ---------------------------------------------------------------- montaje
def word(v):
    """Evento de escena: numero, o "codigo.entrada" (la palabra codigo << 16 | entrada)."""
    if isinstance(v, str) and "." in v:
        code, entry = v.split(".")
        return (int(code) << 16) | int(entry)
    return int(v) & 0xFFFFFFFF


def pack_event(ev):
    """(op, p1, p2, *mas): op 24/48 (marca del mapa: icono, tipo, x, y, z, escala[, 0]), 35, 44
    (posicion de salida) y 29 (objeto del mapa por nombre: texto de hasta 16 letras y estado)
    llevan palabras de mas; un texto se guarda como 16 bytes."""
    op, p1, p2, *more = ev
    out = struct.pack("<3I", op, int(p1) & 0xFFFFFFFF, word(p2) if not isinstance(p2, str) or "." in p2 else 0)
    if isinstance(p2, str) and "." not in p2:      # op 29: p2 es el nombre
        out = struct.pack("<2I", op, int(p1) & 0xFFFFFFFF) + p2.encode("ascii")[:16].ljust(16, b"\0")
    return out + b"".join(struct.pack("<I", int(v) & 0xFFFFFFFF) for v in more)


def lst_replace(b, entry, hdr, events):
    """#LST con la entrada `entry` cambiada entera (las demas se copian tal cual, aunque tengan
    eventos de tamano variable)."""
    n, toff, doff = struct.unpack_from("<3I", b, 4)
    table = [list(struct.unpack_from("<2I", b, toff + 8 * i)) for i in range(n)]
    start = doff + table[entry][0]
    end = doff + table[entry + 1][0] if entry + 1 < n else len(b)
    new = struct.pack("<%dI" % len(hdr), *hdr) + b"".join(pack_event(e) for e in events)
    out = bytearray(b[:start] + new + b[end:])
    delta = len(new) - (end - start)
    table[entry][1] = len(events)
    for i in range(entry + 1, n):
        table[i][0] += delta
    for i, (r, c) in enumerate(table):
        struct.pack_into("<2I", out, toff + 8 * i, r, c)
    return bytes(out)


MAP_EVENT_WORDS = {24: 9, 29: 7, 35: 4, 36: 5, 44: 5, 48: 10, 54: 3}   # u32 por evento (los demas, 3)


def lst_events(b, entry):
    """Eventos de una entrada de un #LST con su tamano real (los del mapa son mas largos), como tuplas."""
    n, toff, doff = struct.unpack_from("<3I", b, 4)
    rel, cnt = struct.unpack_from("<2I", b, toff + 8 * entry)
    end = doff + (struct.unpack_from("<I", b, toff + 8 * (entry + 1))[0] if entry + 1 < n else len(b) - doff)
    for hs in (28, 16):
        p, out = doff + rel + hs, []
        for _ in range(cnt):
            w = MAP_EVENT_WORDS.get(struct.unpack_from("<I", b, p)[0], 3) if p + 4 <= end else 0
            if not w or p + 4 * w > end:
                break
            out.append(struct.unpack_from("<%dI" % w, b, p))
            p += 4 * w
        if len(out) == cnt and p == end:
            return out
    raise ValueError("no se pueden leer los eventos de la entrada %d" % entry)


def lst_build(entries):
    """#LST (LE) a partir de [(cabecera de 4 o 7 u32, [(op, p1, p2)][, datos de detras])]."""
    n = len(entries)
    doff = (0x10 + 8 * n + 15) & ~15
    table, data = [], b""
    for e in entries:
        hdr, events = e[0], e[1]
        table.append((len(data), len(events)))
        data += struct.pack("<%dI" % len(hdr), *hdr)
        data += b"".join(pack_event(ev) for ev in events)
        data += e[2] if len(e) > 2 else b""
    out = b"#LST" + struct.pack("<3I", n, 0x10, doff) + b"".join(struct.pack("<2I", *t) for t in table)
    return out + b"\0" * (doff - len(out)) + data


def lst_entries(b):
    """[(cabecera, eventos)] de un #LST (LE), para anadirle entradas."""
    n, toff, doff = struct.unpack_from("<3I", b, 4)
    table = [struct.unpack_from("<2I", b, toff + 8 * i) for i in range(n)]
    out = []
    for i, (rel, cnt) in enumerate(table):
        nxt = table[i + 1][0] if i + 1 < n else len(b) - doff
        hsize = nxt - rel - 12 * cnt
        hsize = hsize if hsize in (16, 28) else 28
        o = doff + rel
        out.append((list(struct.unpack_from("<%dI" % (hsize // 4), b, o)),
                    [struct.unpack_from("<3I", b, o + hsize + 12 * k) for k in range(cnt)],
                    bytes(b[o + hsize + 12 * cnt:doff + nxt])))
    return out


def story(cfg, us, mod, work):
    """[historia] + [[capitulo]]: historia como las del juego. Cada capitulo es un combate con su
    dialogo en el escenario antes (op 21) y despues (la escena de victoria, que deja en el mapa la
    marca del combate siguiente). Un capitulo con `saga = true` abre saga: al ganar el anterior sale
    el cierre de la saga (`cierre_es`: panel, "guardar?" y mapa de `planeta`, 0 Tierra / 1 Namek) y la
    introduccion de la nueva (`es`: panel, salida `salida` y la marca de su primer combate). La
    primera saga la abre la introduccion del personaje ([[escena]] con la marca de combates.codigo.0).
    El ultimo capitulo sin combate es el final. Devuelve las entradas escritas."""
    h, chapters = cfg["historia"], cfg.get("capitulo", [])
    reg = json.load(open(os.path.join(HERE, "du_codigos.json"), encoding="utf-8"))["codigos"]
    cont = lambda code: int(reg[str(code)])
    code = int(h["codigo"])                    # 5122: saga 1 (Namek), casilla 22
    slot, saga0 = code % 100, (code - 5000) // 100
    txt, dlg = int(h["textos_desde"]), h["dialogos"]
    dtxt = int(dlg["textos_desde"])
    for k, v in h.get("estilos", {}).items():   # estilos de texto propios de esta historia
        STYLES.setdefault(k, dict(STYLES["dialogo"])).update(v)
    s_narr, s_end = int(h.get("guion_narracion", 0xADB)), int(h.get("guion_final", 2790))
    s_fight, s_after = int(h.get("guion_combate", 2866)), int(h.get("guion_tras_combate", 2495))
    panel = int(h.get("fondo", 1411))
    bcode = lambda s: int(h["combates"]["codigo"]) + 100 * (s - saga0)
    dcode = lambda s: int(dlg["codigo"]) + 100 * (s - saga0)
    n = 0

    def text(bank, c, key, style):
        nonlocal n
        for lang, afss in LANG_AFS.items():
            # `idioma` en [historia]: ese idioma en todos (p. ej. "en" mientras falten traducciones)
            lines = c.get(key + h.get("idioma", lang)) or c.get(key + "en")
            for afs in afss:
                if os.path.exists(afs_path(us, afs)):
                    rb.write_entry(mod, afs, bank, text_bank(lines, style), work)
                    n += 1
        return len(c.get(key + "en") or c.get(key + "es"))

    music = int(h.get("musica_narracion", 135269))   # 2.4197, la de las introducciones del juego
    pics = [int(h.get("imagenes_desde", 4510))]   # entradas libres de data_cmn para las imagenes propias

    writer = []

    def voice(v):
        if isinstance(v, int) or (isinstance(v, str) and v.strip().isdigit()):
            return 0x20000 | int(v)
        if isinstance(v, str) and os.path.splitext(v)[1].lower() in (".wav", ".mp3", ".ogg", ".flac"):
            import sb_voces  # noqa: PLC0415
            import voces  # noqa: PLC0415
            if not writer:
                writer.append(voces.VoiceWriter(us, mod))
            path = v if os.path.isabs(v) else os.path.join(TOML_DIR[0], v)
            pcm = sb_voces.ff_pcm(open(path, "rb").read(), sb_voces.ADX_RATE, os.path.splitext(v)[1])
            adx = sb_voces.adx_encode(pcm)
            return 0x20000 | writer[0].add(adx, adx)
        return word(v)

    def narration(bank, c, key):
        """Panel de narracion; `imagen` (o `cierre_imagen`) = un .png/.webp/.jpg propio en lugar del panel."""
        pages = text(bank, c, key, "narracion")
        pic = panel
        if c.get(key + "imagen"):
            path = os.path.join(TOML_DIR[0], c[key + "imagen"])
            pic = pics[0]
            pics[0] += 1
            rb.write_entry(mod, "data_cmn.afs", pic, png2pic.build_picture(Image.open(path)), work)
        # locucion: `voz` (o `cierre_voz`), una por frase como las del juego (14, pagina, 2.N: la
        # entrada N de adx_usa/adx_jpn; la intro de Namek trae 14,1..3 = 2.1231..1233). Vale un
        # numero (entrada ADX del juego), "2.N" o un fichero de audio propio (.wav/.mp3/.ogg)
        voices = [(14, k + 1, voice(v)) for k, v in enumerate(c.get(key + "voz", [])) if str(v).strip() not in ("", "-1")]
        return ([9, 3, pages + 1, 2, 0, 0, s_narr],
                [(10, 0, pic), (10, 1, 0x10000 | bank), (10, 2, 0x10000 | bank), (13, 0, music),
                 (11, 0, 1414), (12, 0, 1415), (11, 1, 1414), (12, 1, 1415)] + voices)

    def talk(bank, c, key):
        """Dialogo en el escenario (cabecera de 16 B): retratos (izquierda, derecha; hablan por
        turnos, empieza el de la izquierda) y una frase por pagina, sin voz."""
        nl = text(bank, c, key, "dialogo")
        a, b = c["retratos"]
        return ([4 + 2 * nl, 5, nl + 1, 5],
                [(10, 0, int(a)), (10, 1, int(b)), (10, 2, 0x1026E), (10, 3, 0x10000 | bank), (10, 4, 0x10000 | bank),
                 (13, 0, 0xFFFFFFFF)] + [ev for i in range(4) for ev in ((11, i, 1556), (12, i, 1557))] +
                [(11, 4, 0x1026D), (12, 4, 0x1026E)])

    # saga y posicion de cada combate
    s, k, place = saga0, {}, []
    for c in chapters:
        if "combate" in c:
            if c.get("saga") and place:
                s += 1
            k[s] = k.get(s, -1) + 1
        place.append((s, k.get(s, 0)))
    bword = lambda i: (bcode(place[i][0]) << 16) | (2 * place[i][1])
    # casilla 0 como las marcas sueltas del juego; el op 24 siempre lleva 9 palabras
    mark = lambda i: [24, 0, bword(i)] + list(chapters[i].get("marca", [1, 2, 5462, 54, -5448, 1000]))
    fights, talks, intros, maps = {}, {}, {}, {}
    for i, c in enumerate(chapters):
        if "combate" not in c:
            continue
        s, kk = place[i]
        if kk == 0:
            # llegada al mapa de la saga: como el juego (102.0 -> 5100.0, 104.0 -> 5200.4), la
            # introduccion pasa por una escena del mapa (una frase de tu personaje, `llegada_es`); sin
            # ella el personaje salia volando a ras del suelo. Es la entrada 0 de las escenas del mapa
            # de la saga: la introduccion de la primera saga ([[escena]]) acaba en (20, 0, "codigo.0")
            bank = dtxt + 120 + i
            text(bank, {"mapa_es": c.get("llegada_es", ["¡Adelante!"]), "mapa_en": c.get("llegada_en", ["Let's go!"])},
                 "mapa_", "dialogo")
            maps.setdefault(s, []).append(([3, 3, 2, 2, 0, 0, 2899],
                                           [(10, 0, int(c.get("fondo_dialogo", h.get("fondo_dialogo", 1536)))),
                                            (10, 1, int(c["retratos"][1])), (10, 2, 0x10000 | bank), (13, 0, 0xFFFFFFFF),
                                            (11, 0, 1556), (12, 0, 1557), (11, 1, 1556), (12, 1, 1557)]))
        if "forma_jugador" in c:                # tu personaje empieza transformado (el juego lo lee)
            START_FORMS[bword(i)] = int(c["forma_jugador"])
        # modelo y traje en el mapa mientras ese combate espera (Freezer: modelo 5 = Mecha, traje 2 =
        # en su nave, el modelo 193); [historia] da el de todos los capitulos que no digan otro
        for key, dst in (("modelo_mapa", MAP_MODELS), ("traje_mapa", MAP_SUITS), ("animacion_mapa", MAP_ANIMS)):
            v = c.get(key, h.get(key))
            if v is not None and v != "":
                dst[bword(i)] = int(v)
                if kk == 0:   # el mapa crea al personaje con la introduccion de la saga pendiente (102.22, 104.22...)
                    dst[((100 + 2 * s) << 16) | slot] = int(v)
                    dst[(code + 100 * (s - saga0)) << 16] = int(v)   # o con su llegada al mapa

        if "traje_rival" in c:                  # traje del rival (0 el normal; Vegeta 3 = armadura de Namek)
            RIVAL_SUITS[bword(i)] = int(c["traje_rival"])
        if "plantilla" in c or "plantilla" in h["combates"]:
            # los #BPC sin usar (654-667) son de un formato anterior (1216 B; el juego lee 1248 B) y
            # dan escenario equivocado y cuelgues: el combate se rehace sobre uno del juego
            # (`plantilla`: su escenario, +0x90) con el rival (+0x10) del viejo y `forma_rival` (+0x3c)
            old = read_entry(us, "data_cmn.afs", int(c["combate"]), work)
            new = bytearray(read_entry(us, "data_cmn.afs", int(c.get("plantilla", h["combates"].get("plantilla", 0))), work))
            rival = int(c.get("rival", struct.unpack_from(">I", old, 0x10)[0]))
            form = int(c.get("forma_rival", 0))
            struct.pack_into(">I", new, 0x10, rival)
            # 8 registros de 0x98 B desde 0x10 (uno por nivel): forma en +0x2c y +0x58 de cada uno y
            # sus capsulas (+0x38 cuenta) fuera: las de la plantilla salian como capsula gris al ganar
            # nivel del rival y experiencia (+0x64, +0x60 de cada registro): el juego elegia el ultimo
            # (nivel 99, 100.000 de experiencia); todos pasan a uno progresivo por capitulo (0..4, o
            # `nivel` en el capitulo)
            nb = sum(1 for x in chapters if "combate" in x)
            lv = int(c.get("nivel", min(4, len([x for x in chapters[:i] if "combate" in x]) * 5 // max(1, nb))))
            exp, level = struct.unpack_from(">2I", new, 0x10 + 0x98 * lv + 0x60)
            for r in range(0x10, 0x10 + 8 * 0x98, 0x98):
                struct.pack_into(">2I", new, r + 0x60, exp, level)
                struct.pack_into(">I", new, r + 0x2c, form)
                struct.pack_into(">I", new, r + 0x58, form)
                struct.pack_into(">I", new, r + 0x38, 0)
            rb.write_entry(mod, "data_cmn.afs", int(c["combate"]), bytes(new), work)
            n += 1
        talks.setdefault(s, []).append(talk(dtxt + i, c, "dialogo_"))
        fights.setdefault(s, []).append(([0, 0, 0, 0, 0, 0, s_fight],
                                         [(16, 0, int(c["combate"])), (15, 0, s_after),
                                          (23, 0, bword(i)),                          # quita su marca
                                          (21, 0, (dcode(s) << 16) | kk),             # dialogo al empezar
                                          (20, 0, bword(i) + 1)]))                    # gana: escena de victoria
        nxt = chapters[i + 1] if i + 1 < len(chapters) else None
        if nxt is None:
            tail = []
        elif "combate" not in nxt:                      # el final (entrada nueva del contenedor de finales)
            # se rehace una entrada de final que no usa nadie (`entrada`, 5 = 3 paginas) con su cabecera
            # (y su guion) original: una entrada nueva al final del contenedor cuelga el juego
            fin = h["final"]
            # con la cabecera (guion) y la musica del final de Goku (`estilo`, 1728.0: logo, texto y
            # creditos); 2 paginas. Con el guion de las entradas 4-5 la pantalla se quedaba en negro
            # `fondo`: "heroe" (el del final de Goku, 1728.0: cielo azul, imagenes 1563/1564) o "villano"
            # (el de Broly, 1729.20: imagenes 1561/1562 y su musica); cabecera (guion) y musica de ese final
            k = int(fin.get("entrada", 39))
            b = read_entry(us, "data_cmn.afs", int(fin["contenedor"]), work)
            villain = fin.get("fondo", "heroe") == "villano"
            src = lst_entries(read_entry(us, "data_cmn.afs", 1729, work))[20] if villain else lst_entries(b)[int(fin.get("estilo", 0))]
            hdr = list(src[0])
            music = [e for e in src[1] if e[0] == 13][0]
            pic = (1561, 1562) if villain else (1563, 1564)
            text(txt + i + 1, nxt, "", "final")
            tail = [(20, 0, (int(fin["codigo"]) << 16) | k)]
            b = lst_replace(b, k, hdr, [(10, 0, 0x10000 | (txt + i + 1)), (10, 1, 0x10000 | (txt + i + 1)),
                                        tuple(music), (11, 0, pic[0]), (12, 0, pic[1]), (11, 1, pic[0]), (12, 1, pic[1])])
            rb.write_entry(mod, "data_cmn.afs", int(fin["contenedor"]), b, work)
            n += 1
        elif nxt.get("saga"):                           # cierre de saga -> guardar -> introduccion
            s2 = place[i + 1][0]
            intro, outro = 100 + 2 * s2, 99 + 2 * s2
            hdr, ev = narration(txt + 200 + i + 1, nxt, "cierre_")
            intros[cont(outro)] = (hdr, ev + [(20, 0, (intro << 16) | slot), (50, 0, 0), (47, 0, int(nxt.get("planeta", 0)))])
            hdr, ev = narration(txt + i + 1, nxt, "")
            # `objetos_mapa_de` = "104.0": los objetos del mapa (op 29, por nombre) de una introduccion del
            # juego, como hace la de Namek a mano. Sin ellos R1 en la Tierra cerraba el juego (2026-10-12)
            objs = []
            if nxt.get("objetos_mapa_de"):
                oc, oe = str(nxt["objetos_mapa_de"]).split(".")
                objs = [e for e in lst_events(read_entry(us, "data_cmn.afs", cont(int(oc)), work), int(oe)) if e[0] == 29]
            intros[cont(intro)] = (hdr, ev + [(22, 0, 0), tuple([44, 0] + list(nxt["salida"]))] + objs +
                                   [(54, 0, 0), tuple(mark(i + 1)),
                                    (20, 0, (code + 100 * (s2 - saga0)) << 16)])   # a su llegada al mapa
            tail = [(22, 0, 0), (20, 0, (outro << 16) | slot)]
        else:                                           # sigue la saga: la marca del combate siguiente
            # y, como el juego (2138.1 -> 5100.5), se vuelve al mapa por una escena del mapa: una frase
            # de tu personaje (`mapa_es`; guion 2899, el de 5000.8); sin ella el dialogo del combate
            # siguiente se cuelga (sub_820AAD30)
            m = nxt.get("marca", [1, 2, 5462, 54, -5448, 1000])
            bank = dtxt + 80 + i
            line = {"mapa_es": c.get("mapa_es", ["Siguiente."]), "mapa_en": c.get("mapa_en", ["Next."])}
            text(bank, line, "mapa_", "dialogo")
            mapk = len(maps.setdefault(s, []))
            maps[s].append(([3, 3, 2, 2, 0, 0, 2899],
                            [(10, 0, int(c.get("fondo_dialogo", h.get("fondo_dialogo", 1536)))), (10, 1, int(c["retratos"][1])),
                             (10, 2, 0x10000 | bank), (13, 0, 0xFFFFFFFF),
                             (11, 0, 1556), (12, 0, 1557), (11, 1, 1556), (12, 1, 1557)]))
            # salida a 500 de la marca (x, z, altura), como el juego: encima la tapa y la toca al aparecer
            tail = [(44, 0, int(m[2]), int(m[4]) + 500, int(h.get("altura_mapa", 400))), (54, 0, 0), tuple(mark(i + 1)),
                    (20, 0, ((code + 100 * (s - saga0)) << 16) | mapk)]
        # la escena de victoria es como 2138.1 del juego: cabecera de 28 B con el guion de despues
        # del combate (op 15 + 1) y sus imagenes en orden fondo, jugador, rival (`fondo_dialogo`:
        # 1536 Namek, 1549 la Tierra); con otro orden el guion libera lo que no es y se cuelga
        nl = text(dtxt + 40 + i, c, "victoria_", "dialogo")
        rival, player = c["retratos"]
        bank = dtxt + 40 + i
        # las frases van rival, jugador (como dialogo_*) y el guion habla primero con la imagen 1:
        # el rival en la 1 y tu personaje en la 2 para que cada frase salga con su cara
        ev = ([(10, 0, int(c.get("fondo_dialogo", h.get("fondo_dialogo", 1536)))), (10, 1, int(rival)), (10, 2, int(player)),
               (10, 3, 0x10000 | bank), (10, 4, 0x10000 | bank),
               # musica de la charla tras ganar (`musica_dialogo`, por capitulo o en [historia]; 2.4205 como el juego)
               (13, 0, int(c.get("musica_dialogo", h.get("musica_dialogo", 0x2106D))))] +
              [e for k in range(4) for e in ((11, k, 1556), (12, k, 1557))] + [(11, 4, 0x10755), (12, 4, 0x10756)])
        if c.get("premio"):                      # capsula de premio al ganar (op 19, como 2138.x)
            ev.append((19, 0, int(c["premio"])))
        fights[s].append(([4 + 2 * nl, 5, nl + 1, 5, 0, 0, s_after + 1], ev + tail))
    for s in fights:
        rb.write_entry(mod, "data_cmn.afs", cont(bcode(s)), lst_build(fights[s]), work)
        rb.write_entry(mod, "data_cmn.afs", cont(dcode(s)), lst_build(talks[s]), work)
        n += 2
    for s in maps:                       # escenas del mapa (vuelta tras ganar)
        rb.write_entry(mod, "data_cmn.afs", cont(code + 100 * (s - saga0)), lst_build(maps[s]), work)
        n += 1
    for cn, (hdr, ev) in intros.items():
        b = lst_replace(read_entry(us, "data_cmn.afs", cn, work), slot, hdr, ev)
        rb.write_entry(mod, "data_cmn.afs", cn, b, work)
        n += 1
    return n


def azt_textures(b):
    """[(entrada, datos, tamano)] de las texturas de un #AZT (indice en +0x14, n en +0x10)."""
    n, idx = struct.unpack_from(">2I", b, 0x10)
    out = []
    for k in range(n):
        o = struct.unpack_from(">I", b, idx + 4 * k)[0]
        out.append((o,) + struct.unpack_from(">2I", b, o + 0x14))
    return out


def portraits(cfg, us, mod, work):
    """[[retrato]] entrada = N: retrato de dialogo (#AZT de 2 texturas 1024x1024 BGRA, como 1463).
    `reparar = true`: los retratos sin terminar del juego (1464, 1465, 1486: Freezer 2.a y 3.a forma,
    Mecha) traen una sola textura y cuelgan el dialogo; se rehacen con la suya en las dos.
    `png = "x.png"`: un retrato propio (RGBA; el arte se encaja en la caja de 541x530, abajo;
    `escala` 1.0 y `desplazamiento` [x, y] en px para colocarlo)."""
    n = 0
    tpl = read_entry(us, "data_cmn.afs", 1463, work)
    for c in cfg.get("retrato", []):
        out = bytearray(tpl)
        if c.get("png"):
            path = c["png"] if os.path.isabs(c["png"]) else os.path.join(TOML_DIR[0], c["png"])
            im = Image.open(path).convert("RGBA")
            k = min(541 / im.width, 530 / im.height) * float(c.get("escala", 1.0))
            im = im.resize((max(1, round(im.width * k)), max(1, round(im.height * k))), Image.LANCZOS)
            canvas = Image.new("RGBA", (1024, 1024), (0, 0, 0, 0))
            dx, dy = c.get("desplazamiento", [0, 0])      # x, y en px (y positivo = mas abajo)
            canvas.paste(im, ((541 - im.width) // 2 + int(dx), 530 - im.height + int(dy)), im)
            canvas = canvas.crop((0, 0, 1024, 1024))
            pix = np.ascontiguousarray(np.array(canvas)[..., [2, 1, 0, 3]]).tobytes()
            for _, do, ds in azt_textures(tpl):
                out[do + 0x80:do + ds] = pix
        else:
            src = read_entry(us, "data_cmn.afs", int(c["entrada"]), work)
            (so, sd, ss), = azt_textures(src)[:1]
            for o, do, ds in azt_textures(tpl):
                if ss != ds:
                    raise SystemExit("retrato %s: textura de %d bytes" % (c["entrada"], ss))
                out[do:do + ds] = src[sd:sd + ss]
                out[o + 0x08:o + 0x14] = src[so + 0x08:so + 0x14]    # caja del arte
        rb.write_entry(mod, "data_cmn.afs", int(c["entrada"]), bytes(out), work)
        n += 1
    return n


def clean_panel(cfg, us, mod, work):
    """[[panel_limpio]] cmn = N, filas = R: la textura N de data_cmn (DXT, sin tiling) con sus R
    primeras filas tapadas por la fila de bloques de debajo (la etiqueta de pruebas que dejaron
    los desarrolladores en el panel verde de la introduccion de Freezer)."""
    import texturas_originales as tex
    n = 0
    for c in cfg.get("panel_limpio", []):
        b = bytearray(read_entry(us, "data_cmn.afs", int(c["cmn"]), work))
        for off, w, hgt, fmt, base in tex.dds_textures(bytes(b))[:1]:
            if fmt not in ("DXT3", "DXT5"):
                raise SystemExit("panel_limpio: %s no es DXT3/DXT5" % fmt)
            row = (w // 4) * 16                       # bytes por fila de bloques de 4x4
            src = (int(c.get("filas", 17)) + 3) // 4  # primera fila de bloques limpia
            data = off + 128
            for by in range(src):
                b[data + by * row:data + (by + 1) * row] = b[data + src * row:data + (src + 1) * row]
        rb.write_entry(mod, "data_cmn.afs", int(c["cmn"]), bytes(b), work)
        n += 1
    return n


def map_events(cfg, us, mod, work):
    """[historia] eventos_mapa = { codigos = [2100], retrato = 1463, desde = 4600 }: los eventos del
    mapa que comparte el juego (2100 = "All right, I found something!", con la cara de Goku 1416)
    con el retrato del protagonista. Se copian a entradas nuevas de data_cmn (desde `desde`) y el
    juego las usa solo mientras se juega una historia de este mod ([codigos_historia]); las del juego
    no cambian para nadie mas."""
    ev = cfg.get("historia", {}).get("eventos_mapa")
    if not ev:
        return 0
    reg = json.load(open(os.path.join(HERE, "du_codigos.json"), encoding="utf-8"))["codigos"]
    old = int(ev.get("retrato_original", 1416))
    n = 0
    for i, code in enumerate(ev.get("codigos", [])):
        b = bytearray(read_entry(us, "data_cmn.afs", int(reg[str(code)]), work))
        hits = 0
        for o in range(0, len(b) - 11, 4):     # evento (10, 1, retrato): la cara de quien habla
            if struct.unpack_from("<3I", b, o) == (10, 1, old):
                struct.pack_into("<I", b, o + 8, int(ev["retrato"]))
                hits += 1
        entry = int(ev.get("desde", 4600)) + i
        rb.write_entry(mod, "data_cmn.afs", entry, bytes(b), work)
        STORY_CODES[int(code)] = entry
        print("eventos del mapa %d: %d retratos cambiados (contenedor nuevo %d)" % (code, hits, entry))
        n += 1
    return n


def lst_add(b, entry, events):
    """#LST (LE) con `events` [(op, p1, p2)] anadidos al final de la entrada `entry`."""
    if b[:4] != b"#LST":
        raise ValueError("no es un contenedor #LST")
    n, toff, doff = struct.unpack_from("<3I", b, 4)
    table = [list(struct.unpack_from("<2I", b, toff + 8 * i)) for i in range(n)]
    if not 0 <= entry < n:
        raise ValueError("el contenedor tiene %d entradas" % n)
    rel, cnt = table[entry]
    nxt = table[entry + 1][0] if entry + 1 < n else len(b) - doff
    # cabecera de 16 o 28 bytes, los eventos y, en las intros de los personajes jugables, datos
    # propios detras (posiciones del mapa): los eventos nuevos van justo tras los eventos
    hsize = nxt - rel - 12 * cnt if nxt - rel - 12 * cnt in (16, 28) else 28
    at = doff + rel + hsize + 12 * cnt
    add = b"".join(struct.pack("<3I", *(int(v) & 0xFFFFFFFF for v in e)) for e in events)
    out = bytearray(b[:at] + add + b[at:])
    table[entry][1] += len(events)
    for i in range(entry + 1, n):
        table[i][0] += len(add)
    for i, (r, c) in enumerate(table):
        struct.pack_into("<2I", out, toff + 8 * i, r, c)
    return bytes(out)


def cmd_montar(a):
    cfg = tomllib.load(open(a.toml, "rb"))
    TOML_DIR[0] = os.path.dirname(os.path.abspath(a.toml))
    info = cfg.get("campana", {})
    mods = a.mods or rb.default_mods()
    mod = os.path.join(mods, info.get("mod") or os.path.splitext(os.path.basename(a.toml))[0])
    us, work = a.us or rb.default_us(), tempfile.mkdtemp()
    os.makedirs(mod, exist_ok=True)
    n = 0
    for t in cfg.get("texto", []):
        style = t.get("estilo", "dialogo")
        for lang, afss in LANG_AFS.items():
            lines = t.get(lang) or t.get("en")
            if not lines:
                continue
            bank = text_bank(lines, style)
            for afs in afss:
                if os.path.exists(afs_path(us, afs)):
                    rb.write_entry(mod, afs, int(t["entrada"]), bank, work)
                    n += 1
    for c in cfg.get("combate", []):
        b = bytearray(read_entry(us, "data_cmn.afs", int(c["bpl"]), work))
        if b[:4] != b"#BPC":
            raise SystemExit("data_cmn %s no es un combate (#BPC)" % c["bpl"])
        struct.pack_into(">I", b, 0x10, int(c["rival"]))
        rb.write_entry(mod, "data_cmn.afs", int(c["bpl"]), bytes(b), work)
        n += 1
    by_cont = {}
    for e in cfg.get("escena", []):
        by_cont.setdefault(int(e["contenedor"]), []).append(e)
    for cont, edits in by_cont.items():
        b = read_entry(us, "data_cmn.afs", cont, work)
        for e in edits:
            if "cambiar" in e:
                b = lst_replace(b, int(e["entrada"]), e["cabecera"], e["cambiar"])
            else:
                b = lst_add(b, int(e["entrada"]), [(op, p1, word(p2)) for op, p1, p2 in e["anadir"]])
        rb.write_entry(mod, "data_cmn.afs", cont, b, work)
        n += 1
    if "historia" in cfg:
        n += story(cfg, us, mod, work)
    n += clean_panel(cfg, us, mod, work)
    for c in cfg.get("imagen", []):      # [[imagen]] entrada = N, archivo = "x.webp": imagen de narracion
        rb.write_entry(mod, "data_cmn.afs", int(c["entrada"]),
                       png2pic.build_picture(Image.open(os.path.join(TOML_DIR[0], c["archivo"]))), work)
        n += 1
    n += portraits(cfg, us, mod, work)
    n += map_events(cfg, us, mod, work)
    rb.flush_entries()
    codes = cfg.get("codigos", {})
    wheel = cfg.get("rueda", {}).get("personajes", [])
    with open(os.path.join(mod, "campana.toml"), "w", encoding="utf-8") as f:
        f.write("# Generado por campana.py: escenas de Dragon Universe que cambia este mod.\n[codigos]\n")
        for k, v in sorted(codes.items(), key=lambda kv: int(kv[0])):
            f.write("%d = %d\n" % (int(k), int(v)))
        if wheel:
            f.write("\n[rueda]\npersonajes = %s\n" % ", ".join(str(int(i)) for i in wheel))
        if START_FORMS:
            f.write("\n[forma_jugador]\n" + "".join("%d = %d\n" % kv for kv in sorted(START_FORMS.items())))
        if RIVAL_SUITS:
            f.write("\n[traje_rival]\n" + "".join("%d = %d\n" % kv for kv in sorted(RIVAL_SUITS.items())))
        if STORY_CODES:
            f.write("\n[codigos_historia]\n" + "".join("%d = %d\n" % kv for kv in sorted(STORY_CODES.items())))
        if MAP_MODELS:
            f.write("\n[modelo_mapa]\n" + "".join("%d = %d\n" % kv for kv in sorted(MAP_MODELS.items())))
        if MAP_ANIMS:
            f.write("\n[animacion_mapa]\n" + "".join("%d = %d\n" % kv for kv in sorted(MAP_ANIMS.items())))
        if MAP_SUITS:
            f.write("\n[traje_mapa]\n" + "".join("%d = %d\n" % kv for kv in sorted(MAP_SUITS.items())))
    with open(os.path.join(mod, "manifest.txt"), "w", encoding="utf-8") as f:
        f.write("name=%s\ntype=campana\ndescription=%s\n" % (info.get("nombre", "Campana"),
                                                          info.get("descripcion", "Campana de Dragon Universe")))
    print("mod %s: %d entradas, %d escenas redirigidas" % (mod, n, len(codes)))
    return 0


def selftest():
    img = render("Lo siento... pero has bajado la guardia.", "dialogo")
    assert img.shape[2] == 4 and img.shape[1] <= 200 and img[..., 3].max() > 0
    bank = text_bank(["Hola", "Adios, amigo mio"], "narracion")
    assert bank[:4] == b"#AZT" and struct.unpack(">I", bank[0x10:0x14])[0] == 2
    assert capsulas.azt_read(bank, 1) is not None
    lst = b"#LST" + struct.pack("<3I", 2, 0x10, 0x20) + struct.pack("<4I", 0, 1, 28, 1)
    lst += struct.pack("<4I", 1, 2, 3, 4) + struct.pack("<3I", 10, 0, 5) + struct.pack("<4I", 5, 6, 7, 8) + struct.pack("<3I", 14, 1, 9)
    out = lst_add(lst, 0, [(22, 0, 0), (44, 0, -5)])
    assert struct.unpack_from("<4I", out, 0x10) == (0, 3, 52, 1), struct.unpack_from("<4I", out, 0x10)
    assert struct.unpack_from("<3I", out, 0x20 + 28 + 12) == (44, 0, 0xFFFFFFFB)
    assert struct.unpack_from("<3I", out, 0x20 + 52 + 16) == (14, 1, 9)
    assert pack_event((29, 0, "SAITYOU", 1)) == struct.pack("<2I", 29, 0) + b"SAITYOU".ljust(16, b"\0") + struct.pack("<I", 1)
    assert len(pack_event((24, 3, "5100.2", 2625, 2, 5462, 54, -5448, 1000))) == 36
    rep = lst_replace(lst, 0, [9, 9, 9, 9, 0, 0, 1], [(22, 0, 0), (44, 0, 5930, -1800, 100, 54, 0, 0)])
    assert struct.unpack_from("<2I", rep, 0x10) == (0, 2) and struct.unpack_from("<I", rep, 0x18)[0] == 28 + 12 + 32
    assert struct.unpack_from("<3I", rep, 0x20 + 72 + 16) == (14, 1, 9)
    built = lst_build([([1, 2, 3, 4, 0, 0, 7], [(16, 0, 654), (20, 0, "5122.3")]), ([1, 2, 3, 4], [(0, 0, 0)])])
    back = lst_entries(built)
    assert back[0][1][1] == (20, 0, (5122 << 16) | 3) and back[1][0] == [1, 2, 3, 4] and back[0][2] == b"", back
    tailed = lst_build([([1, 2, 3, 4, 0, 0, 7], [(16, 0, 1)], b"MAPA" * 3), ([1, 2, 3, 4], [])])
    assert lst_entries(tailed)[0][2] == b"MAPA" * 3 and lst_entries(lst_build(lst_entries(tailed))) == lst_entries(tailed)
    t2 = lst_add(tailed, 0, [(20, 0, 9)])
    assert lst_entries(t2)[0][1] == [(16, 0, 1), (20, 0, 9)] and lst_entries(t2)[0][2] == b"MAPA" * 3
    assert lst_entries(lst_build(back)) == back
    print("campana.py: autocomprobacion OK")


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--us", help="carpeta us/ del juego (por defecto la del proyecto)")
    p.add_argument("--mods", help="carpeta mods/ (por defecto la del juego)")
    sub = p.add_subparsers(dest="cmd")
    v = sub.add_parser("ver")
    v.add_argument("entrada", type=int)
    v.add_argument("--salida")
    e = sub.add_parser("escenas")
    e.add_argument("bloque", type=int)
    m = sub.add_parser("montar")
    m.add_argument("toml")
    sub.add_parser("prueba")
    a = p.parse_args(argv)
    if a.cmd == "ver":
        return cmd_ver(a)
    if a.cmd == "escenas":
        return cmd_escenas(a)
    if a.cmd == "montar":
        return cmd_montar(a)
    selftest()
    return 0


if __name__ == "__main__":
    sys.exit(main())
