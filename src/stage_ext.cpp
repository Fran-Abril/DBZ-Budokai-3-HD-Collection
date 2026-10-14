// dbz3 - Extension de ESCENARIOS (ver stage_ext.h).
//
// Banco de pruebas: redirige un registro de la tabla de escenarios a una entrada
// de data_cmn (propia o nueva) y, opcionalmente, le cambia el nombre y el TEMA
// (para que la funcion de tema encaje con la geometria cargada). Confirma en el
// juego el id_mapa -> AFS antes de reubicar las tablas para el hueco nuevo.

#include "stage_ext.h"

#include "mods.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/system/xmemory.h>

#include "generated/dbz3_init.h"

namespace dbz3::stages {
namespace {

// Tabla de registros de escenario (misma direccion US y EU: .data).
constexpr uint32_t kStageTable = 0x82323760;
constexpr uint32_t kStageRec = 92;
constexpr uint32_t kStageCount = 17;
// Tabla paralela de punteros a funcion de tema (17 + terminador 0).
constexpr uint32_t kThemeTable = 0x82323D80;

struct Rec {
  std::string name;
  uint32_t id_map = 0;
  uint32_t id_se = 0;
  uint32_t variant = 0;
};

class Guest {
 public:
  explicit Guest(rex::memory::Memory* m) : m_(m) {}
  uint8_t* P(uint32_t a) const { return m_->TranslateVirtual<uint8_t*>(a); }
  uint32_t U32(uint32_t a) const {
    const uint8_t* p = P(a);
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
  }
  void Protect(uint32_t a, uint32_t n) {
    if (auto* heap = m_->LookupHeap(a)) {
      const uint32_t lo = a & ~0xFFFu, hi = (a + n + 0xFFF) & ~0xFFFu;
      if (hi > lo)
        heap->Protect(lo, hi - lo, rex::memory::kMemoryProtectRead | rex::memory::kMemoryProtectWrite);
    }
  }
  void W32(uint32_t a, uint32_t v) {
    uint8_t b[4] = {uint8_t(v >> 24), uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v)};
    Protect(a, 4);
    std::memcpy(P(a), b, 4);
  }
  void WBytes(uint32_t a, const void* src, uint32_t n) {
    Protect(a, n);
    std::memcpy(P(a), src, n);
  }

 private:
  rex::memory::Memory* m_;
};

std::string Trim(std::string s) {
  while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r')) s.erase(0, 1);
  while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) s.pop_back();
  return s;
}

std::string Unquote(std::string s) {
  if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\'')))
    return s.substr(1, s.size() - 2);
  return s;
}

}  // namespace

void ApplyAtLaunch(rex::memory::Memory* memory) {
  if (!memory) return;
  Guest g(memory);

  // Firma: el primer registro es "BUDO" en las dos imagenes.
  if (std::memcmp(g.P(kStageTable), "BUDO", 4) != 0) {
    REXLOG_INFO("dbz3 escenarios: tabla de escenarios no reconocida; sin cambios");
    return;
  }

  std::error_code ec;
  for (const auto& info : ListMods(false)) {
    if (!info.enabled) continue;
    const auto file = ModsRoot() / info.name / "stage.toml";
    if (!std::filesystem::is_regular_file(file, ec)) continue;

    std::ifstream in(file);
    std::string line;
    bool in_parche = false;
    long registro = -1, entrada = -1, id_se = -1, tema = -1;
    std::string nombre;
    while (std::getline(in, line)) {
      const auto hash = line.find('#');
      if (hash != std::string::npos) line.resize(hash);
      line = Trim(line);
      if (line.empty()) continue;
      if (line.front() == '[') {
        in_parche = line.find("[parche]") != std::string::npos;
        continue;
      }
      const auto eq = line.find('=');
      if (eq == std::string::npos || !in_parche) continue;
      const std::string k = Trim(line.substr(0, eq));
      const std::string v = Unquote(Trim(line.substr(eq + 1)));
      if (k == "registro") registro = std::strtol(v.c_str(), nullptr, 10);
      else if (k == "entrada") entrada = std::strtol(v.c_str(), nullptr, 10);
      else if (k == "id_se") id_se = std::strtol(v.c_str(), nullptr, 10);
      else if (k == "tema") tema = std::strtol(v.c_str(), nullptr, 10);
      else if (k == "nombre") nombre = v;
    }

    if (registro < 0 || registro >= int(kStageCount)) continue;
    const uint32_t rec = kStageTable + uint32_t(registro) * kStageRec;

    if (entrada > 0) {
      const uint32_t old = g.U32(rec + 0x10);
      g.W32(rec + 0x10, uint32_t(entrada));
      REXLOG_INFO("dbz3 escenarios [{}]: registro {} id_mapa {} -> {}", info.name, registro, old, entrada);
    }
    if (id_se > 0) {
      const uint32_t old = g.U32(rec + 0x14);
      g.W32(rec + 0x14, uint32_t(id_se));
      REXLOG_INFO("dbz3 escenarios [{}]: registro {} id_se {} -> {}", info.name, registro, old, id_se);
    }
    if (!nombre.empty()) {
      char buf[16] = {};
      std::strncpy(buf, nombre.c_str(), 15);
      g.WBytes(rec + 0x00, buf, 16);
      REXLOG_INFO("dbz3 escenarios [{}]: registro {} nombre -> '{}'", info.name, registro, nombre);
    }
    if (tema >= 0 && tema < int(kStageCount)) {
      const uint32_t fn = g.U32(kThemeTable + uint32_t(tema) * 4);
      g.W32(kThemeTable + uint32_t(registro) * 4, fn);
      REXLOG_INFO("dbz3 escenarios [{}]: registro {} tema -> fn[{}] = 0x{:08X}", info.name, registro, tema, fn);
    }
  }
}

}  // namespace dbz3::stages

// ---------------------------------------------------------------------------
// TEMPORAL (diagnostico RE del grafo de escena): despachador de bloques
// sub_820800A8 (registro magic->parser en 0x82310110). Logea cada magic y, si el
// puntero de entrada es NULL, lo anota (eso es el "read of guest 0x0").
// QUITAR cuando termine la investigacion.
// ---------------------------------------------------------------------------
namespace {
constexpr uint32_t kBlockReg = 0x82310110;  // US
int g_dbg_dispatch_logged = 0;
}  // namespace

REX_HOOK_RAW(sub_820800A8) {
  const uint32_t in = ctx.r3.u32;
  const uint32_t lr = uint32_t(ctx.lr);
  // seguro: si el puntero no esta en el heap del guest, no leerlo (crashearia).
  if (in < 0x40000000u || in >= 0x50000000u) {
    if (g_dbg_dispatch_logged < 400) {
      REXLOG_INFO("dbz3 dbg dispatch BAD ptr: in=0x{:08X} lr=0x{:08X} r4=0x{:08X} r5=0x{:08X}",
                  in, lr, ctx.r4.u32, ctx.r5.u32);
      g_dbg_dispatch_logged++;
    }
    ctx.r3.s64 = 0;
    return;
  }
  static bool table_dumped = false;
  if (!table_dumped) {
    table_dumped = true;
    for (int i = 0; i < 16; i++) {
      uint32_t func = REX_LOAD_U32(kBlockReg + i * 8);
      uint32_t magic = REX_LOAD_U32(kBlockReg + 4 + i * 8);
      REXLOG_INFO("dbz3 dbg table[{}]: func=0x{:08X} magic=0x{:08X}", i, func, magic);
    }
  }
  if (in == 0) {
    REXLOG_INFO("dbz3 dbg dispatch NULL ptr: lr=0x{:08X} r4=0x{:08X} r5=0x{:08X}", lr, ctx.r4.u32,
                ctx.r5.u32);
    ctx.r3.s64 = 0;
    return;
  }
  uint32_t handler = 0;
  for (int i = 0;; i++) {
    uint32_t func = REX_LOAD_U32(kBlockReg + i * 8);
    if (func == 0) break;
    const uint32_t magic = kBlockReg + 4 + i * 8;
    uint32_t len = 0;
    while (REX_LOAD_U8(magic + len)) len++;
    bool match = true;
    for (uint32_t j = 0; j < len; j++) {
      if (REX_LOAD_U8(magic + j) != REX_LOAD_U8(in + 1 + j)) { match = false; break; }
    }
    if (match) { handler = func; break; }
  }
  if (g_dbg_dispatch_logged < 400) {
    char s[24] = {};
    for (int i = 0; i < 20; i++) {
      uint8_t c = REX_LOAD_U8(in + i);
      if (!c) break;
      s[i] = (c >= 32 && c < 127) ? char(c) : '.';
    }
    REXLOG_INFO("dbz3 dbg dispatch: in=0x{:08X} str='{}' handler=0x{:08X} lr=0x{:08X}", in, s,
                handler, lr);
    g_dbg_dispatch_logged++;
  }
  if (REX_LOAD_U8(in) == 33) { ctx.r3.s64 = 0; return; }
  if (handler == 0) {
    char m[24] = {};
    for (int i = 0; i < 12; i++) {
      uint8_t c = REX_LOAD_U8(in + i);
      m[i] = (c >= 32 && c < 127) ? char(c) : '.';
    }
    REXLOG_INFO("dbz3 dbg dispatch NO MATCH: in=0x{:08X} lr=0x{:08X} str='{}' raw={:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X}",
                in, lr, m, REX_LOAD_U8(in), REX_LOAD_U8(in + 1), REX_LOAD_U8(in + 2),
                REX_LOAD_U8(in + 3), REX_LOAD_U8(in + 4), REX_LOAD_U8(in + 5),
                REX_LOAD_U8(in + 6), REX_LOAD_U8(in + 7));
    ctx.r3.s64 = 0;
    return;
  }
  ctx.r3.u64 = in;
  ctx.ctr.u64 = handler;
  REX_CALL_INDIRECT_FUNC(handler);
  REX_STORE_U8(in + 0, 33);
  ctx.r3.s64 = 1;
}

// --- TEMPORAL: traza del handler #ACE (sub_821525E8) y de su arreglo recursivo
// sub_8214F2E0, para localizar el nodo con offset invalido. QUITAR luego. ---
namespace {
int g_ace_log = 0;
int g_fix_log = 0;
void Dbz3FixNode(uint8_t* base, uint32_t node, uint32_t blk_base, int depth) {
  if (g_fix_log < 500) {
    REXLOG_INFO("dbz3 dbg fix: node=0x{:08X} base=0x{:08X} depth={}", node, blk_base, depth);
    g_fix_log++;
  }
  if (depth > 64) {
    REXLOG_INFO("dbz3 dbg fix DEPTH LIMIT en node=0x{:08X}", node);
    return;
  }
  uint32_t a = REX_LOAD_U32(node + 36);
  if (a != 0) {
    uint32_t t = a + blk_base;
    REX_STORE_U32(node + 36, t);
    Dbz3FixNode(base, t, blk_base, depth + 1);
  }
  uint32_t b = REX_LOAD_U32(node + 40);
  if (b != 0) {
    uint32_t t = b + blk_base;
    REX_STORE_U32(node + 40, t);
    Dbz3FixNode(base, t, blk_base, depth + 1);
  }
  uint32_t c = REX_LOAD_U32(node + 44);
  if (c != 0) { REX_STORE_U32(node + 44, c + blk_base); }
}
}  // namespace

REX_HOOK_RAW(sub_821525E8) {
  const uint32_t blk = ctx.r3.u32;
  const uint32_t off = REX_LOAD_U32(blk + 12);
  if (g_ace_log < 200) {
    REXLOG_INFO("dbz3 dbg ACE handler: blk=0x{:08X} root_off=0x{:08X} -> root=0x{:08X}", blk, off,
                blk + off);
    g_ace_log++;
  }
  Dbz3FixNode(base, blk + off, blk, 0);
}

REX_HOOK_RAW(sub_8214F2E0) { Dbz3FixNode(base, ctx.r3.u32, ctx.r4.u32, 0); }

// --- TEMPORAL: iterador de sub-bloques de un #AMB (sub_820B0CF8). Logea solo
// los AMB "grandes" (>=20 sub-bloques, el de script/efectos de B2). ---
namespace {
int g_sub_log = 0;
}  // namespace

REX_HOOK_RAW(sub_820B0CF8) {
  const uint32_t amb = ctx.r3.u32;
  const int32_t idx = int32_t(ctx.r4.u32);
  uint32_t res = 0;
  uint32_t count = 0;
  if (idx >= 0 && amb != 0) {
    count = REX_LOAD_U32(amb + 16);
    if (uint32_t(idx) < count) {
      const uint32_t table = REX_LOAD_U32(amb + 20);
      const uint32_t entry = table + uint32_t(idx) * 16;
      if (entry != 0) res = REX_LOAD_U32(entry);
    }
  }
  if (count >= 20 && g_sub_log < 300) {
    char s[24] = {};
    if (res != 0) {
      for (int i = 0; i < 20; i++) {
        uint8_t c = REX_LOAD_U8(res + i);
        if (!c) break;
        s[i] = (c >= 32 && c < 127) ? char(c) : '.';
      }
    }
    REXLOG_INFO("dbz3 dbg sub: amb=0x{:08X} n={} idx={} -> 0x{:08X} '{}'", amb, count, idx, res, s);
    g_sub_log++;
  }
  ctx.r3.u64 = res;
}

// --- TEMPORAL: traza del CARGADOR de escena sub_821370B8 (que #AMB lee y sus
// hijos 0/1) y del registro de nodo sub_8208D2C8 (r5 invalido = el crash). QUITAR
// cuando termine la investigacion. ---
namespace {
bool GuestPtrDbg(uint32_t p) { return p >= 0x40000000u && p < 0x50000000u; }
}  // namespace

REX_HOOK_RAW(sub_821370B8) {
  const uint32_t obj = ctx.r3.u32;
  const uint32_t r30 = GuestPtrDbg(obj) ? REX_LOAD_U32(obj + 144) : 0;
  const uint32_t amb = GuestPtrDbg(r30) ? REX_LOAD_U32(r30 + 212) : 0;
  uint32_t n = 0, c0 = 0, c1 = 0;
  if (GuestPtrDbg(amb)) {
    n = REX_LOAD_U32(amb + 16);
    const uint32_t tbl = REX_LOAD_U32(amb + 20);
    if (GuestPtrDbg(tbl) && n > 0) {
      c0 = REX_LOAD_U32(tbl);
      if (n > 1) c1 = REX_LOAD_U32(tbl + 16);
    }
  }
  REXLOG_INFO("dbz3 dbg loader: obj=0x{:08X} r30=0x{:08X} amb=0x{:08X} n={} c0=0x{:08X} c1=0x{:08X}",
              obj, r30, amb, n, c0, c1);
  __imp__sub_821370B8(ctx, base);
}

REX_HOOK_RAW(sub_8208D2C8) {
  const uint32_t r5 = ctx.r5.u32;
  if (!GuestPtrDbg(r5)) {
    REXLOG_WARN("dbz3 dbg node BAD r5=0x{:08X} r3=0x{:08X} r4=0x{:08X} lr=0x{:08X}", r5, ctx.r3.u32,
                ctx.r4.u32, uint32_t(ctx.lr));
  }
  __imp__sub_8208D2C8(ctx, base);
}

// --- TEMPORAL: traza del procesado de sub-objeto de escena sub_820C2DC8.
// rec: +0 = nº sub-objetos, +8 = fuente, +12 = array de entradas de 16 B
// (campo +4 = nodo). Captura rec y la primera entrada. QUITAR luego. ---
namespace {
int g_rec_log = 0;
}  // namespace

REX_HOOK_RAW(sub_820C2DC8) {
  const uint32_t rec = ctx.r3.u32;
  if (GuestPtrDbg(rec) && g_rec_log < 400) {
    const uint32_t n = REX_LOAD_U32(rec + 0);
    const uint32_t src = REX_LOAD_U32(rec + 8);
    const uint32_t arr = REX_LOAD_U32(rec + 12);
    uint32_t e0 = 0, e1 = 0, e2 = 0, e3 = 0;
    if (GuestPtrDbg(arr)) {
      e0 = REX_LOAD_U32(arr + 0);
      e1 = REX_LOAD_U32(arr + 4);
      e2 = REX_LOAD_U32(arr + 8);
      e3 = REX_LOAD_U32(arr + 12);
    }
    REXLOG_INFO("dbz3 dbg rec: rec=0x{:08X} n={} src=0x{:08X} arr=0x{:08X} e=[{:08X} {:08X} {:08X} {:08X}]",
                rec, n, src, arr, e0, e1, e2, e3);
    g_rec_log++;
  }
  __imp__sub_820C2DC8(ctx, base);
}
