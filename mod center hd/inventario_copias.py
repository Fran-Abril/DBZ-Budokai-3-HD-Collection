#!/usr/bin/env python3
# -*- coding: utf-8 -*-
r"""inventario_copias.py - Que copias de otros juegos tiene el jugador y que huecos de cada modo pueden llenar (S1-F09).

Solo lectura: no extrae ni copia nada. Mira las mismas rutas que ya usan las herramientas del proyecto
(ps2_games con .iso/.ruta, DBZ3_T3_ISO, DBZ3_XENO_CPK/Steam, piezas UT convertidas, Heroes) y, por cada copia
encontrada, comprueba los ficheros clave y lista los huecos que cubre (tabla de studies 1\ENTREGAS\F09_assets\INFORME.md).

  python inventario_copias.py            tabla legible
  python inventario_copias.py --json     lo mismo en JSON (para el launcher o una prueba)
  python inventario_copias.py prueba     autocomprobacion sin copias (tabla de huecos bien formada)
"""
import glob
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path[:0] = [HERE, os.path.join(ROOT, "awo_tools")]

# copia -> [(fichero clave dentro de la copia o None, hueco que llena, estado en el port)]
# estado: "usado" = ya lo lee el port; "barato" = paquete corto; "medio"/"caro" = ver INFORME F09
HUECOS = {
    "b1": [("/USR/DATA_EN.AFS", "Satan: Ring de Cell entero (grupo 11, entradas 167-169)", "medio"),
           ("/USR/DATA_EN.AFS", "Satan: subtitulos de la ronda 1 (imagenes, sin localizar)", "caro"),
           ("/USR/ADX_JP.AFS", "Satan/B1: voces originales de las charlas", "usado"),
           ("/USR/DATA_EN.AFS", "Escenarios B1 como casilla nueva (Paramo...)", "usado")],
    "b2": [("/USR/DATA_CMN.AFS", "Tablero B2 y nave: piezas CMN 402, escenarios 7", "usado"),
           ("/USR/MOVIE_EU.SFD", "Tablero B2: video del Mundo Dragon", "usado"),
           ("/USR/ADX_US.AFS", "Tablero B2: sonidos por evento que faltan (F01.6)", "medio")],
    "iw": [("/USR/DATA_CMN.AFS", "Campana IW: campos, HUD de minijuegos, animaciones", "usado"),
           ("/USR/FD01A.SFD", "Campana IW: 22 videos FDxx + ENDING", "usado"),
           ("/USR/DATA_CMN.AFS", "Cinematicas propias (Satan ronda 1, sagas del Heroe) con animaciones de IW", "medio")],
    "sb1": [("/PSP_GAME/USRDIR/data_btl_cmn.afs", "Ports de personajes de Shin Budokai", "usado")],
    "sb2": [("/PSP_GAME/USRDIR/data_sys_voice_us.afs", "Voces de historia (Another Road) para escenas nuevas", "medio")],
    "t3": [("/DATA/PZS3EU2.AFS", "Voces de combate de personajes que B3 no tiene (t3:CLAVE)", "usado"),
           ("/DATA/PZS3EU2.AFS", "Musica nueva para modos propios (20 pistas ADX)", "barato"),
           ("/DATA/PZS3EU1.AFS", "Nombres de tecnicas en 9 idiomas para ports (lista de cada personaje)", "barato"),
           ("/DATA/PZS3EU1.AFS", "Retratos/iconos de 161 personajes (texturas GS tras BPE)", "medio"),
           ("/DATA/PZS3EU1.AFS", "Modelos/animaciones (pmdl + VIF + BPE)", "caro")],
    "ut": [(None, "Heroe: peinados UT (9 piezas)", "usado")],
    "xeno": [(None, "Heroe: ropa por piezas (HUM 65 Bust / 60 Pants / 50 Boots / 33 Rist)", "medio"),
             (None, "Heroe: 1 prenda (HUM_500_Bust, ropa = 1)", "usado")],
    "sdbh": [(None, "Ports de modelos HD de Heroes y sus voces", "usado")],
}

NOMBRES = {"b1": "Budokai 1 (PS2)", "b2": "Budokai 2 (PS2)", "iw": "Infinite World (PS2)", "sb1": "Shin Budokai (PSP)",
           "sb2": "Shin Budokai Another Road (PSP)", "t3": "Tenkaichi 3 (PS2)", "ut": "Ultimate Tenkaichi (piezas)",
           "xeno": "Xenoverse (data.cpk)", "sdbh": "Heroes World Mission"}


def _probar(f, *a):
    try:
        return f(*a)
    except Exception:   # noqa: BLE001  (una copia que no se puede leer cuenta como ausente)
        return None


def localizar():
    import iso   # noqa: PLC0415
    out = {k: _probar(iso.find_game, k) for k in ("b1", "b2", "sb1", "sb2")}
    try:
        import importar  # noqa: PLC0415
        out["iw"] = _probar(importar.iw_dir)
        cpks = _probar(importar.sdbh_cpks) or []
        out["sdbh"] = _probar(importar.sdbh_dir) or (os.path.dirname(cpks[0]) if cpks else None)
    except Exception:   # noqa: BLE001
        out["iw"] = out["sdbh"] = None
    try:
        import t3  # noqa: PLC0415
        out["t3"] = _probar(t3.buscar_iso)
    except Exception:   # noqa: BLE001
        out["t3"] = None
    try:
        import xeno_model  # noqa: PLC0415
        out["xeno"] = _probar(xeno_model.buscar_cpk)
    except Exception:   # noqa: BLE001
        out["xeno"] = None
    ut = [os.path.join(os.environ["DBZ3_CAC_PIEZAS"], "pelo")] if os.environ.get("DBZ3_CAC_PIEZAS") else []
    ut += [os.path.join(ROOT, "cac_piezas", "pelo"), r"E:\DBZ3HD_trabajo\ut\cac\convertidas\pelo"]
    out["ut"] = next((d for d in ut if glob.glob(os.path.join(d, "avatar_hair_*_b3.bin"))), None)
    return out


def ficheros_de(ruta):
    """Conjunto de rutas (MAYUSCULAS, con / inicial) de una ISO o carpeta; vacio si no es ninguna."""
    if not ruta:
        return set()
    if os.path.isdir(ruta):
        s = set()
        for dp, _dn, fns in os.walk(ruta):
            for fn in fns:
                s.add("/" + os.path.relpath(os.path.join(dp, fn), ruta).replace("\\", "/").upper())
        return s
    if ruta.lower().endswith(".iso"):
        import iso  # noqa: PLC0415
        img = _probar(iso.Iso, ruta)
        return {p.upper() for p in img.files()} if img else set()
    return set()


def inventario():
    filas = []
    for k, ruta in localizar().items():
        fs = ficheros_de(ruta)
        for clave, hueco, estado in HUECOS[k]:
            if not ruta:
                ok = False
            elif clave is None or not fs:
                ok = True
            else:   # la region cambia el sufijo (PZS3EU2 / PZS3US2, ADX_US / ADX_JP...): se compara el patron
                pat = re.sub(r"(EU|US)(?=\d?\.)", "..", clave.upper())
                ok = any(re.fullmatch(pat, p) for p in fs)
            filas.append({"copia": k, "nombre": NOMBRES[k], "ruta": ruta or "", "hueco": hueco, "estado": estado,
                          "disponible": bool(ok)})
    return filas


def prueba():
    assert set(HUECOS) == set(NOMBRES)
    for k, v in HUECOS.items():
        for clave, hueco, estado in v:
            assert estado in ("usado", "barato", "medio", "caro"), (k, estado)
            assert clave is None or clave.startswith("/"), (k, clave)
            assert hueco
    assert re.sub(r"(EU|US)(?=\d?\.)", "..", "/DATA/PZS3EU2.AFS") == "/DATA/PZS3..2.AFS"
    print("inventario_copias.py: prueba OK (%d huecos en %d copias)" % (sum(map(len, HUECOS.values())), len(HUECOS)))


def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    if sys.argv[1:] == ["prueba"]:
        prueba()
        return 0
    filas = inventario()
    if "--json" in sys.argv:
        print(json.dumps(filas, ensure_ascii=False, indent=1))
        return 0
    actual = None
    for f in filas:
        if f["copia"] != actual:
            actual = f["copia"]
            print("\n%s: %s" % (f["nombre"], f["ruta"] or "NO ENCONTRADA"))
        print("  [%s] %-7s %s" % ("x" if f["disponible"] else " ", f["estado"], f["hueco"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
