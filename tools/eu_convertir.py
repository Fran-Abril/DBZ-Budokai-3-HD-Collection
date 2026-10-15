#!/usr/bin/env python3
"""eu_convertir.py - pasa un fichero de src/ al mecanismo dual US/EU de src/eu_map.h (frente eu, 2026-10-15).

  python tools/eu_convertir.py src/hub.cpp                 # escribe el diff en pantalla, no toca nada
  python tools/eu_convertir.py src/hub.cpp --salida X.cpp  # escribe la version convertida en X.cpp

Cambios mecanicos (solo fuera de comentarios y cadenas):
  REX_HOOK_RAW(sub_X) {...__imp__sub_X(ctx, base)...}  ->  DBZ3_HOOK_EU(sub_X) {...orig(ctx, base)...}
  __imp__sub_Y(...) fuera de su gancho                 ->  DBZ3_IMP_EU(sub_Y)(...)
  sub_Y como llamada o puntero                          ->  DBZ3_CALL_EU(sub_Y)
  constexpr uint32_t kA = 0x82..., kN = 64;             ->  constexpr dbz3::GA kA = 0x82...;  constexpr uint32_t kN = 64;
  otra direccion 0x82... del mapa                       ->  dbz3::GA(0x82...)
  #include "guest_region.h"                             ->  #include "eu_map.h"  (o se anade)
No toca: las guardas con dbz3::g_guest_eu (avisa de cada una: se revisan a mano), lo ya doble (DBZ3_HOOK,
GuestAddr, dbz3eu_sub_, tablas kXxxEu), REX_EXTERN, ni las direcciones que no estan en src/eu_map_gen.h (avisa:
o se mide su pareja con tools/eu_map.py o ese trozo se queda solo US).
"""
import argparse
import difflib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TROZOS = re.compile(r'"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'|//[^\n]*|/\*.*?\*/', re.S)
LIT = re.compile(r"\b0x(82[0-9A-Fa-f]{6})u?\b")


def pares():
    s = (ROOT / "src" / "eu_map_gen.h").read_text(encoding="utf-8")
    return ({int(a, 16) for a in re.findall(r"\{0x([0-9A-F]{8})u, 0x[0-9A-F]{8}u\}", s)},
            set(re.findall(r"#define DBZ3EU_(sub_[0-9A-F]{8}) ", s)))


def codigo(s, fn):
    """aplica fn solo a los trozos de codigo (fuera de comentarios y cadenas)."""
    out, i = [], 0
    for m in TROZOS.finditer(s):
        out += [fn(s[i:m.start()]), m.group(0)]
        i = m.end()
    return "".join(out + [fn(s[i:])])


def fin_bloque(s, i):
    """indice tras la llave que cierra el bloque que abre la primera '{' desde i (salta comentarios y cadenas)."""
    j, nivel = s.index("{", i), 0
    while j < len(s):
        m = TROZOS.match(s, j)
        if m:
            j = m.end()
            continue
        nivel += {"{": 1, "}": -1}.get(s[j], 0)
        j += 1
        if nivel == 0:
            return j
    return len(s)


def convertir(s, dirs, funcs):
    avisos = []
    # 1) ganchos: el original dentro de su cuerpo pasa a orig
    for m in reversed(list(re.finditer(r"REX_HOOK_RAW\((sub_[0-9A-F]{8})\)", s))):
        x = m.group(1)
        if x not in funcs:
            avisos.append("gancho %s sin pareja EU en el mapa: se queda solo US" % x)
            continue
        e = fin_bloque(s, m.end())
        if re.search(r"\borig\b", codigo(s[m.end():e], lambda t: t)):
            avisos.append("gancho %s: su cuerpo ya usa el nombre 'orig' (choca con el original): renombrarlo" % x)
        cuerpo = codigo(s[m.end():e], lambda t, x=x: re.sub(r"\b__imp__%s\(" % x, "orig(", t))
        s = s[:m.start()] + "DBZ3_HOOK_EU(%s)" % x + cuerpo + s[e:]

    def decl(m):
        # constexpr uint32_t kA = 0x82..., kN = 64;  ->  constexpr dbz3::GA kA = 0x82...;  constexpr uint32_t kN = 64;
        ind, cuerpo = m.group(1), m.group(2)
        if "(" in cuerpo or "{" in cuerpo:
            avisos.append("declaracion compleja, a mano: %s" % m.group(0).strip())
            return m.group(0)
        ga, resto = [], []
        for d in cuerpo.rstrip(";").split(","):
            v = re.fullmatch(r"\s*\w+\s*=\s*0x(82[0-9A-Fa-f]{6})u?\s*", d)
            (ga if v and int(v.group(1), 16) in dirs else resto).append(d.strip())
            if LIT.search(d) and not (v and int(v.group(1), 16) in dirs):
                avisos.append("sin pareja en el mapa, se queda US: %s" % d.strip())
        if not ga:
            return m.group(0)
        out = "%sconstexpr dbz3::GA %s;" % (ind, ", ".join(ga))
        return out + ("\n%sconstexpr uint32_t %s;" % (ind, ", ".join(resto)) if resto else "")

    def lit(m):
        v = int(m.group(m.lastindex), 16)
        if v in dirs:
            return "dbz3::GA(%s)" % m.group(0)
        avisos.append("direccion 0x%08X sin pareja en el mapa: se queda US" % v)
        return m.group(0)

    def trozo(t):
        t = re.sub(r"\b__imp__(sub_[0-9A-F]{8})\b",
                   lambda m: "DBZ3_IMP_EU(%s)" % m.group(1) if m.group(1) in funcs else m.group(0), t)
        # tambien como argumento (call(sub_X, ...)): solo se saltan los nombres dentro de las macros de gancho
        t = re.sub(r"(?<![\w])(?<!DBZ3_HOOK_EU\()(?<!DBZ3_IMP_EU\()(?<!DBZ3_CALL_EU\()(?<!REX_EXTERN\()"
                   r"(?<!REX_HOOK_RAW\()(?<!DBZ3_HOOK\()(sub_[0-9A-F]{8})\b(?!\s*,\s*dbz3eu_)",
                   lambda m: "DBZ3_CALL_EU(%s)" % m.group(1) if m.group(1) in funcs else m.group(0), t)
        # 2) declaraciones constexpr de direcciones (pueden ocupar varias lineas): cambia el tipo
        t = re.sub(r"^([ \t]*)constexpr\s+uint32_t\s+(k\w+\s*=[^;{}]*;)",
                   lambda m: decl(m) if LIT.search(m.group(2)) else m.group(0), t, flags=re.M)
        # 3) el resto de direcciones, salvo las de esas declaraciones
        return re.sub(r"(constexpr dbz3::GA [^;]*;|\bGA\([^()]*\))|" + LIT.pattern, lambda m: m.group(1) or lit(m), t)

    # lo ya doble (GuestAddr(us, eu), tablas kXxxEu, REX_EXTERN) no se toca
    guard = []

    def guarda(m):
        guard.append(m.group(0))
        return "\1%d\1" % (len(guard) - 1)
    s = re.sub(r"GuestAddr\([^()]*\)|REX_EXTERN\([^()]*\)|^.*\bk\w*(Eu|Us)\b.*$", guarda, s, flags=re.M)
    s = codigo(s, trozo)
    s = re.sub("\x01(\\d+)\x01", lambda m: guard[int(m.group(1))], s)
    # 4) constantes derivadas (constexpr uint32_t kB = kA + 16, con kA ya GA) pasan a GA: el desplazamiento no cambia
    while True:
        ga = set(re.findall(r"\b(\w+)\s*=", " ".join(re.findall(r"constexpr\s+dbz3::GA\s+([^;]*);", s))))

        def deriv(m):
            ds = [re.fullmatch(r"\s*\w+\s*=\s*(\w+)\s*([-+][^,]*)?", d) for d in m.group(2).split(",")]
            if all(d and d.group(1) in ga for d in ds):
                return m.group(1) + "constexpr dbz3::GA " + m.group(2) + ";"
            return m.group(0)
        n = re.sub(r"^([ \t]*)constexpr\s+uint32_t\s+([^;{}()]*);", deriv, s, flags=re.M)
        if n == s:
            break
        s = n
    if '#include "eu_map.h"' not in s and re.search(r"dbz3::GA|DBZ3_(HOOK|CALL|IMP)_EU", s):
        if '#include "guest_region.h"' in s:
            s = s.replace('#include "guest_region.h"', '#include "eu_map.h"', 1)
        else:
            ult = list(re.finditer(r'^#include "[^"]+"\n', s, re.M))
            if ult:
                s = s[:ult[-1].end()] + '#include "eu_map.h"\n' + s[ult[-1].end():]
    for m in re.finditer(r"g_guest_eu", s):
        avisos.append("linea %d: guarda g_guest_eu (revisar a mano)" % (s.count("\n", 0, m.start()) + 1))
    for m in re.finditer(r"^(static\s+)?const\s+uint32_t\s+\w+.*dbz3::GA\(", s, re.M):   # globales
        avisos.append("linea %d: constante inicializada con GA (se evalua al arrancar, antes de saber la "
                      "region): usar dbz3::GA como tipo" % (s.count("\n", 0, m.start()) + 1))
    return s, avisos


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("fichero")
    ap.add_argument("--salida")
    a = ap.parse_args()
    p = Path(a.fichero)
    s = p.read_text(encoding="utf-8")
    dirs, funcs = pares()
    n, avisos = convertir(s, dirs, funcs)
    if a.salida:   # mismo fin de linea que el original (hay ficheros con CRLF)
        Path(a.salida).write_text(n, encoding="utf-8", newline="\r\n" if b"\r\n" in p.read_bytes() else "\n")
    else:
        sys.stdout.writelines(difflib.unified_diff(s.splitlines(True), n.splitlines(True), str(p), str(p) + " (EU)"))
    for x in dict.fromkeys(avisos):
        print("aviso: " + x, file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
