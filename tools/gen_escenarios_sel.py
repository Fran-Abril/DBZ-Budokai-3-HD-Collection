#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""gen_escenarios_sel.py - Genera src/escenarios_sel_gen.inc (US): copias de las funciones del SELECTOR DE
ESCENARIOS del Duelo (mn_StageSel) con sitio para casillas nuevas.

Datos del selector (656 B, sub_8219F4F8): +0 sprites #ACA, +4 texturas #AZT, +20..+64 dibujos, +68 carrusel
13 x 12 B [dibujo, angulo, casilla u16], +578 lista de casillas 13 x u16, +604 casilla actual, +609 n.
Ampliado (1024 B): carrusel en +668 (24 entradas, angulo en +672 = 56 x 12) y lista en +956 (24 x u16).
Tablas por casilla del ejecutable (13 entradas) -> tablas propias (escenarios.cpp): 0x82020F94 (texturas de
la bola y del circulo), 0x82020FB0 (sprites de la vista y del nombre), 0x82373384 (casilla -> registro); el
limite de casilla 0..11 pasa a 12 + altas. sub_8219F350 llama a dbz3_esc_instalar al tener los recursos y
sub_8219E828 a dbz3_esc_banco alrededor de la vista grande y el rotulo (texturas de la casilla nueva).

  python tools/gen_escenarios_sel.py      # regenera src/escenarios_sel_gen.inc (y la gemela EU)

EU (frente eu, 2026-10-15): src/escenarios_sel_gen_eu.inc, las mismas copias (SelEu_X) tomadas de generated_eu/ con
las parejas verificadas de src/eu_map_gen.h (tools/eu_map.py): nombre EU de cada funcion, direcciones de las
tablas (lo16) y etiquetas internas (misma distancia en la funcion pareja, que tiene el mismo cuerpo).
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GEN = os.path.join(ROOT, "generated")
OUT = os.path.join(ROOT, "src", "escenarios_sel_gen.inc")
GEN_EU = os.path.join(ROOT, "generated_eu")
OUT_EU = os.path.join(ROOT, "src", "escenarios_sel_gen_eu.inc")

SIZE, E, L, MAXN = 1024, 668, 956, 24
assert (E + 4) % 12 == 0 and E + 12 * MAXN <= L and L + 2 * MAXN <= SIZE
K = (E + 4) // 12            # (k + K) * 12 = angulo de la entrada k
F94, FB0, TRANS = "int32_t(dbz3_esc_f94)", "int32_t(dbz3_esc_fb0)", "int32_t(dbz3_esc_trans)"

# funcion -> [(regex, reemplazo, apariciones esperadas)]
SUBS = {
    "8219F4F8": [(r"= 656;", "= %d;" % SIZE, 2)],
    "8219C870": [(r"ctx\.r31\.s64 = ctx\.r3\.s64 \+ 68;", "ctx.r31.s64 = ctx.r3.s64 + %d;" % E, 1),
                 (r"ctx\.r30\.s64 = 13;", "ctx.r30.s64 = %d;" % MAXN, 1)],
    "8219CAE8": [(r"\+ 578;", "+ %d;" % L, 1), (r"ctx\.r31\.s64 \+ 76;", "ctx.r31.s64 + %d;" % (E + 8), 1),
                 (r"ctx\.r11\.s64 \+ 6;", "ctx.r11.s64 + %d;" % K, 1)],
    "8219ECC8": [(r"ctx\.r11\.s64 \+ 289;", "ctx.r11.s64 + %d;" % (L // 2), 1),
                 (r"compare<int32_t>\(ctx\.r10\.s32, 11, ctx\.xer\)",
                  "compare<int32_t>(ctx.r10.s32, int32_t(dbz3_esc_limite), ctx.xer)", 1),
                 (r"ctx\.r9\.s64 = ctx\.r9\.s64 \+ 13188;", "ctx.r9.s64 = %s;" % TRANS, 1),
                 (r"ctx\.r10\.s64 = ctx\.r31\.s64 \+ 76;", "ctx.r10.s64 = ctx.r31.s64 + %d;" % (E + 8), 1),
                 (r"ctx\.r10\.s64 = ctx\.r11\.s64 \+ 6;", "ctx.r10.s64 = ctx.r11.s64 + %d;" % K, 1),
                 (r"ctx\.r11\.s64 = ctx\.r11\.s64 \+ 6;", "ctx.r11.s64 = ctx.r11.s64 + %d;" % K, 1),
                 (r"REX_LOAD_U16\(ctx\.r10\.u32 \+ 76\)", "REX_LOAD_U16(ctx.r10.u32 + %d)" % (E + 8), 1)],
    "8219E540": [(r"ctx\.r7\.s64 = ctx\.r9\.s64 \+ 3988;", "ctx.r7.s64 = %s;" % F94, 1),
                 # textura del circulo de la casilla de esa posicion del carrusel (el juego usaba la
                 # posicion: solo coincide si estan todos los escenarios)
                 (r"ctx\.r8\.u64 = __builtin_rotateleft64\(ctx\.r10\.u32 \| \(ctx\.r10\.u64 << 32\), 1\) & 0xFFFFFFFE;",
                  "ctx.r8.u64 = 2 * uint32_t(REX_LOAD_U16(ctx.r3.u32 + %d + 12 * ctx.r10.u32));" % (E + 8), 1),
                 (r"ctx\.r30\.s64 \+ 68;", "ctx.r30.s64 + %d;" % E, 1),
                 (r"ctx\.r30\.u32 \+ 72\)", "ctx.r30.u32 + %d)" % (E + 4), 1),
                 (r"ctx\.r30\.u32 \+ 76\)", "ctx.r30.u32 + %d)" % (E + 8), 2)],
    "8219E6C0": [(r"ctx\.r30\.s64 \+ 68;", "ctx.r30.s64 + %d;" % E, 1),
                 (r"ctx\.r30\.u32 \+ 76\)", "ctx.r30.u32 + %d)" % (E + 8), 1)],
    "8219F2A8": [(r"ctx\.r11\.u32 \+ 76\)", "ctx.r11.u32 + %d)" % (E + 8), 1)],
    "8219F350": [(r"ctx\.r11\.u32 \+ 76\)", "ctx.r11.u32 + %d)" % (E + 8), 1),
                 (r"(\tctx\.r3\.u64 = ctx\.r30\.u64;\n\tctx\.lr = 0x8219F468;)",
                  "\tdbz3_esc_instalar(ctx, base, ctx.r30.u32);\n\\1", 1)],
    "8219D520": [(r"ctx\.r30\.s64 = ctx\.r7\.s64 \+ 3988;", "ctx.r30.s64 = %s;" % F94, 1)],
    "8219D6E8": [(r"ctx\.r27\.s64 = ctx\.r7\.s64 \+ 3988;", "ctx.r27.s64 = %s;" % F94, 1)],
    "8219CC48": [(r"ctx\.r30\.s64 = ctx\.r11\.s64 \+ 4016;", "ctx.r30.s64 = %s;" % FB0, 1)],
    # vista grande (D+24) y rotulo (D+40) de una casilla nueva: con su #AZT puesto mientras se dibujan
    "8219E828": [(r"\nloc_8219E98C:\n", "\nloc_8219E98C:\n\tdbz3_esc_banco(ctx, base, ctx.r31.u32, true);\n", 1),
                 (r"\nloc_8219EB40:\n", "\nloc_8219EB40:\n\tdbz3_esc_banco(ctx, base, ctx.r31.u32, false);\n", 1)],
}


def body(name, gen=GEN, sym="sub_"):
    for f in sorted(os.listdir(gen)):
        if not re.match(r"dbz3(_eu)?_recomp\.\d+\.cpp$", f):
            continue
        txt = open(os.path.join(gen, f), encoding="utf-8").read()
        m = re.search(r"^DEFINE_REX_FUNC\(%s%s\) \{\n(.*?)^\}\n" % (sym, name), txt, re.S | re.M)
        if m:
            return m.group(1)
    raise SystemExit("no encuentro %s%s en %s" % (sym, name, gen))


def mapa_eu():
    """US -> EU de src/eu_map_gen.h: {nombre US de funcion: hex EU} y {direccion: direccion}."""
    s = open(os.path.join(ROOT, "src", "eu_map_gen.h"), encoding="utf-8").read()
    funcs = dict(re.findall(r"#define DBZ3EU_sub_([0-9A-F]{8}) dbz3eu_sub_([0-9A-F]{8})", s))
    pares = {int(a, 16): int(b, 16) for a, b in re.findall(r"\{0x([0-9A-F]{8})u, 0x([0-9A-F]{8})u\}", s)}
    return funcs, pares


def a_eu(name, subs, funcs, pares):
    """Las sustituciones de una funcion con los numeros EU: lo16 de las tablas, lr y etiquetas loc_."""
    us_f, eu_f = int(name, 16), int(funcs[name], 16)
    lo = {}
    for a in (0x82020F94, 0x82020FB0, 0x82373384):
        assert pares[a] >> 16 == a >> 16, hex(a)   # misma parte alta: solo cambia el lo16
        lo[str(a & 0xFFFF)] = str(pares[a] & 0xFFFF)

    def tr(t):
        t = re.sub(r"\b(3988|4016|13188)\b", lambda m: lo[m.group(1)], t)
        t = re.sub(r"0x(8219[0-9A-F]{4})\b", lambda m: "0x%08X" % pares[int(m.group(1), 16)], t)
        return re.sub(r"loc_(8219[0-9A-F]{4})", lambda m: "loc_%08X" % (eu_f + int(m.group(1), 16) - us_f), t)
    return [(tr(rx), tr(rep), n) for rx, rep, n in subs]


def generar(gen, out_path, sym, pref, cabecera, traducir=None):
    out = list(cabecera)
    llamadas = set()
    for name, subs in SUBS.items():
        ename = traducir[0][name] if traducir else name
        b = body(ename, gen, sym).replace("\tREX_FUNC_PROLOGUE();\n", "")
        b = "\n".join(l for l in b.splitlines() if not l.strip().startswith("//")) + "\n"
        for rx, rep, n in (a_eu(name, subs, *traducir) if traducir else subs):
            b, k = re.subn(rx, rep, b)
            if k != n:
                raise SystemExit("%s%s: %r aparece %d veces (esperado %d)" % (sym, ename, rx, k, n))
        llamadas |= set(re.findall(r"\b((?:__imp__)?dbz3eu_\w+)\(ctx, base\)", b))
        out.append("static void %s%s(PPCContext& ctx, uint8_t* base) {\n%s}\n" % (pref, name, b.rstrip("\n")))
    if traducir:   # el EU no incluye su cabecera (choca con la US): declara lo que llama
        out[len(cabecera):len(cabecera)] = ["REX_EXTERN(%s);" % c for c in sorted(llamadas)] + [""]
    open(out_path, "w", encoding="utf-8", newline="\n").write("\n".join(out))
    print("->", out_path)


def main():
    generar(GEN, OUT, "sub_", "Sel_", [
        "// GENERADO por tools/gen_escenarios_sel.py a partir de generated/ (US). NO EDITAR.",
        "// Selector de escenarios con casillas nuevas: datos %d B, carrusel +%d (%d entradas), lista +%d."
        % (SIZE, E, MAXN, L), "// Los ganchos estan en escenarios.cpp.", "",
        "constexpr uint32_t kSelTam = %d, kSelCarrusel = %d, kSelLista = %d, kSelMax = %d;" % (SIZE, E, L, MAXN), ""])
    if os.path.isdir(GEN_EU):
        generar(GEN_EU, OUT_EU, "dbz3eu_sub_", "SelEu_", [
            "// GENERADO por tools/gen_escenarios_sel.py a partir de generated_eu/ (EU/PAL) y src/eu_map_gen.h. NO EDITAR.",
            "// Gemela EU de escenarios_sel_gen.inc (mismos cambios; SelEu_<funcion US>). Solo en el nucleo dual.", ""],
            mapa_eu())


if __name__ == "__main__":
    sys.exit(main())
