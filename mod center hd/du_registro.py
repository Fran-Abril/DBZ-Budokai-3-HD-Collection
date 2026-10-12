"""Saca del codigo recompilado la tabla de Dragon Universe "codigo de historia -> contenedor #LST"
(sub_82156F30: la pila se rellena con li/stw y luego cada rango de codigos apunta a un trozo).
uso: du_registry.py generated/dbz3_recomp.18.cpp salida.json"""
import json
import re
import sys

src = open(sys.argv[1], encoding="utf-8").read()
start = src.index("DEFINE_REX_FUNC(sub_82156F30)")
end = src.index("DEFINE_REX_FUNC(", start + 10)
body = src[start:end].splitlines()

regs, stack = {}, {}
li = re.compile(r"ctx\.r(\d+)\.s64 = (-?\d+);$")
stw = re.compile(r"REX_STORE_U32\(ctx\.r1\.u32 \+ (\d+), ctx\.r(\d+)\.u32\);")
for line in body:
    line = line.strip()
    m = li.match(line)
    if m:
        regs[int(m.group(1))] = int(m.group(2))
        continue
    m = stw.match(line)
    if m and int(m.group(2)) in regs:
        stack[int(m.group(1))] = regs[int(m.group(2))]

# cadena de rangos: "compare(r3, LIMITE)" ... "ctx.r11.s64 = ctx.r1.s64 + BASE" (codigo >= LIMITE)
ranges, limit = [], None
cmp_re = re.compile(r"compare<int32_t>\(ctx\.r3\.s32, (\d+), ctx\.xer\)")
base_re = re.compile(r"ctx\.r11\.s64 = ctx\.r1\.s64 \+ (\d+);")
for line in body:
    m = cmp_re.search(line)
    if m:
        limit = int(m.group(1))
        continue
    m = base_re.search(line)
    if m and limit is not None:
        ranges.append((limit, int(m.group(1))))
        limit = None
ranges.sort()
# el indice dentro del rango es (codigo - LIMITE) (sub r10 = r3 - limite; slwi 2; lwzx)
out = {}
for i, (lo, off) in enumerate(ranges):
    hi = ranges[i + 1][0] if i + 1 < len(ranges) else lo + 400
    for code in range(lo, hi):
        v = stack.get(off + 4 * (code - lo))
        if v is None:
            break
        out[code] = v
json.dump({"rangos": ranges, "codigos": out}, open(sys.argv[2], "w"), indent=1)
print("rangos", ranges)
print("codigos", len(out), "ejemplos", list(out.items())[:12])
