// dbz3 - Roster/select runtime tracing (diagnostic only).
//
// Implements DICTAMEN GPT-6 Astra hito 2 ("trazar conteo, celdas y registro de
// 184 B"): instrument the guest select entry points to distinguish capacity
// vs identity vs enumeration. All logging is gated by dbz1_diag_logging (F10/
// dev), so nothing is written in normal play. Writes dbz1_roster_trace.log.
//
// Hooked guest functions (US codegen):
//   sub_8217F478  - select cell entry: reads slot id u16 at r3+64; 0xFFFF = empty
//   sub_8217F3F0  - portrait consumer: index = slot*8 + flag*4 over 0x82372818
//   sub_82180AA0  - selection update: reads P1/P2 char base indices at
//                   0x8238B670/0x8238B674 (stride 184 over 0x8238B780)
//
// The originals are kept behind __imp__ and re-run unchanged. Only sub_8217A920 (it also
// applies the extra costumes) is hooked in the EU/PAL image too; the rest are US-only traces.

#include <rex/hook.h>
#include <rex/cvar.h>
#include <rex/logging.h>

#include "generated/dbz3_init.h"
#include "roster_ext.h"
#include "guest_region.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

// dbz1_diag_logging is defined in rexruntime.dll (shared runtime).
REXCVAR_DECLARE(bool, dbz1_diag_logging);

namespace dbz3 {
namespace {

constexpr uint32_t kPortraitTable = 0x82372818;  // 39 slots x 2 u32
constexpr uint32_t kCharBase = 0x8238B780;       // stride-184 char records
constexpr uint32_t kIdxP1 = 0x8238B670;          // P1 char index (u32)
constexpr uint32_t kIdxP2 = 0x8238B674;          // P2 char index (u32)
constexpr uint32_t kAvailTable = 0x8245F150;     // per-slot availability bytes
constexpr uint32_t kAvailSrc = 0x820206C0;       // static source (39 bytes)
constexpr uint32_t kUnlockStruct = 0x824BA110;   // unlock/profile flags struct

bool TraceEnabled() {
  return REXCVAR_GET(dbz1_diag_logging);
}

void TraceLine(const std::string& line) {
  if (!TraceEnabled()) return;
  std::ofstream f("dbz1_roster_trace.log", std::ios::app);
  if (f) f << line << std::endl;
}

// Hex/dec helpers to keep format lines readable.
std::string Hx(uint32_t v) {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%08x", v);
  return buf;
}

std::string Dec(uint32_t v) {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%u", v);
  return buf;
}

}  // namespace
}  // namespace dbz3

//------------------------------------------------------------------------------
// sub_8217F478: select cell slot-id reader / empty-slot gate.
//   r3 = widget; slot id = u16 at r3+64; 0xFFFF => returns (empty).
//------------------------------------------------------------------------------
REX_HOOK_RAW(sub_8217F478) {
  const bool trace = dbz3::TraceEnabled();
  uint32_t obj = ctx.r3.u32;
  uint32_t slot = REX_LOAD_U16(obj + 64);
  if (trace) {
    dbz3::TraceLine("F478 obj=" + dbz3::Hx(obj) + " slot=" + dbz3::Dec(slot) +
                    (slot == 0xFFFF ? " EMPTY" : "") + " caller=" +
                    dbz3::Hx(ctx.lr));
  }
  __imp__sub_8217F478(ctx, base);
}

//------------------------------------------------------------------------------
// sub_8217F3F0: portrait resolver. r3 = widget; slot = u16(r3+64); flag = bit
// (u16(r3+62) & 1). index = slot*8 + flag*4 over 0x82372818.
//------------------------------------------------------------------------------
// Llamado desde el gancho de select_ext.cpp (un solo gancho por funcion).
void dbz3_trace_sub_8217F3F0(PPCContext& ctx, uint8_t* base) {
  const bool trace = dbz3::TraceEnabled();
  uint32_t obj = ctx.r3.u32;
  uint32_t slot = REX_LOAD_U16(obj + 64);
  uint32_t flag = REX_LOAD_U16(obj + 62) & 1;
  if (trace) {
    uint32_t index = slot * 8 + flag * 4;
    uint32_t portrait = REX_LOAD_U32(dbz3::kPortraitTable + index);
    dbz3::TraceLine("F3F0 obj=" + dbz3::Hx(obj) + " slot=" + dbz3::Dec(slot) +
                    " flag=" + dbz3::Dec(flag) + " idx=" + dbz3::Dec(index) +
                    " portrait=" + dbz3::Dec(portrait) + " caller=" +
                    dbz3::Hx(ctx.lr));
  }
}

//------------------------------------------------------------------------------
// sub_82180AA0: selection update. Reads P1/P2 char indices (stride 184) and the
// +14/+18/+114 u16 fields of each record. Logs the state before delegating.
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// sub_8217F520: cursor slot setter. r3 = cursor widget, r4 = new slot id (u16).
// Called by the select navigation handlers (sub_8217C5C0 / sub_8217ADE8) every
// time the cursor moves. Traces the ACTUAL set of reachable slots (capacity).
//------------------------------------------------------------------------------
// Llamado desde el gancho de select_ext.cpp (un solo gancho por funcion).
void dbz3_trace_sub_8217F520(PPCContext& ctx, uint8_t* base) {
  const bool trace = dbz3::TraceEnabled();
  uint32_t obj = ctx.r3.u32;
  uint32_t slot = ctx.r4.u32;
  if (trace) {
    std::string line = "F520 obj=" + dbz3::Hx(obj) + " set_slot=" + dbz3::Dec(slot) +
                       " caller=" + dbz3::Hx(ctx.lr);
    if (slot < 200) {
      uint32_t rec = dbz3::kCharBase + slot * 184;
      line += " rec+12=" + dbz3::Dec(REX_LOAD_U16(rec + 12)) +
              " +14=" + dbz3::Dec(REX_LOAD_U16(rec + 14)) +
              " +18=" + dbz3::Dec(REX_LOAD_U16(rec + 18)) +
              " +114=" + dbz3::Dec(REX_LOAD_U16(rec + 114));
    }
    dbz3::TraceLine(line);
  }
}

//------------------------------------------------------------------------------
// sub_8217A920: availability table init. Copies 39 bytes from kAvailSrc to
// kAvailTable, then ORs unlock flags from kUnlockStruct (+20/21/22 and
// +192/193/194) into the runtime table. Dumps the state so we can see which
// slots get unlocked.
//------------------------------------------------------------------------------
DBZ3_HOOK(sub_8217A920, dbz3eu_sub_8217A8D8) {
  const bool trace = dbz3::TraceEnabled();
  if (trace) {
    uint32_t st = dbz3::GuestAddr(dbz3::kUnlockStruct, 0x824BA100);
    uint32_t src_off = dbz3::GuestAddr(dbz3::kAvailSrc, 0x820206C8);
    std::string line = "A920 unlock:";
    line += " f20=" + dbz3::Dec(REX_LOAD_U8(st + 20)) +
            " f21=" + dbz3::Dec(REX_LOAD_U8(st + 21)) +
            " f22=" + dbz3::Dec(REX_LOAD_U8(st + 22));
    line += " f192=" + dbz3::Dec(REX_LOAD_U8(st + 192)) +
            " f193=" + dbz3::Dec(REX_LOAD_U8(st + 193)) +
            " f194=" + dbz3::Dec(REX_LOAD_U8(st + 194));
    line += " r3=" + dbz3::Hx(ctx.r3.u32);
    dbz3::TraceLine(line);
  }
  orig(ctx, base);
  dbz3::roster::AddExtraCostumes(base);
}

//------------------------------------------------------------------------------
// sub_8217A8B8: writes cursor position into the per-mode screen table
// (0x8245F178). Dumps the screen table (positions -> slot u16) and the
// character-ID field (+18) of every 184B record, so we can map the full grid
// and locate the reserved "?" cell and the blocked/aliased slots.
//------------------------------------------------------------------------------
REX_HOOK_RAW(sub_8217A8B8) {
  const bool trace = dbz3::TraceEnabled();
  if (trace) {
    std::string avail;
    for (int i = 0; i < 39; ++i) {
      if (i) avail += ",";
      avail += dbz3::Dec(REX_LOAD_U8(dbz3::kAvailTable + i));
    }
    dbz3::TraceLine("A8B8 avail[39]=" + avail + " caller=" + dbz3::Hx(ctx.lr));

    // screen table 0x8245F178: u16 slot per cursor position (+4 offset).
    std::string scr;
    for (int i = 0; i < 64; ++i) {
      uint32_t v = REX_LOAD_U16(0x8245F178 + 4 + i * 2);
      if (i) scr += ",";
      scr += dbz3::Dec(v);
    }
    dbz3::TraceLine("A8B8 screen[64]=" + scr);

    // character IDs (+18) and +12/+14 for all 39 records.
    std::string cid;
    std::string c14;
    for (int i = 0; i < 39; ++i) {
      uint32_t rec = dbz3::kCharBase + i * 184;
      if (i) { cid += ","; c14 += ","; }
      cid += dbz3::Dec(REX_LOAD_U16(rec + 18));
      c14 += dbz3::Dec(REX_LOAD_U16(rec + 14));
    }
    dbz3::TraceLine("A8B8 rec+18[39]=" + cid);
    dbz3::TraceLine("A8B8 rec+14[39]=" + c14);
  }
  __imp__sub_8217A8B8(ctx, base);
}

//------------------------------------------------------------------------------
// sub_8217A618: cursor movement resolver. r3 = current slot (s16), r4 = step.
// Computes the next reachable slot (skipping unavailable records) and returns
// it in r3. Traces the pre/post slot to expose the skip logic.
//------------------------------------------------------------------------------
REX_HOOK_RAW(sub_8217A618) {
  const bool trace = dbz3::TraceEnabled();
  int32_t cur = static_cast<int16_t>(ctx.r3.u32 & 0xFFFF);
  int32_t step = static_cast<int16_t>(ctx.r4.u32 & 0xFFFF);
  __imp__sub_8217A618(ctx, base);
  if (trace) {
    int32_t next = static_cast<int16_t>(ctx.r3.u32 & 0xFFFF);
    dbz3::TraceLine("A618 cur=" + dbz3::Dec(static_cast<uint32_t>(cur)) +
                    " step=" + dbz3::Dec(static_cast<uint32_t>(step)) +
                    " next=" + dbz3::Dec(static_cast<uint32_t>(next)) +
                    " caller=" + dbz3::Hx(ctx.lr));
  }
}

//------------------------------------------------------------------------------
// sub_8217ADE8: select confirm handler. When the cursor is on the "?" cell
// (slot id -> 0xFFFF via the u16 table at 0x82020618), the guest builds a
// bitmap of available chars (u64 at r31+16), runs the LFSR RNG (sub_82084A78),
// and picks a random available slot. Dump the bitmap so we can see whether
// Android 16 (slot 28) is included, deciding patch-vs-hook for the alias hito.
//------------------------------------------------------------------------------
REX_HOOK_RAW(sub_8217ADE8) {
  const bool trace = dbz3::TraceEnabled();
  uint32_t obj = ctx.r3.u32;
  uint32_t slot = REX_LOAD_U8(obj + 4);
  uint32_t bitmap_l = REX_LOAD_U32(obj + 16);
  uint32_t bitmap_h = REX_LOAD_U32(obj + 20);
  if (trace) {
    std::string line = "ADE8 obj=" + dbz3::Hx(obj) + " slot=" + dbz3::Dec(slot);
    char b64[32];
    std::snprintf(b64, sizeof(b64), "%08x%08x", bitmap_h, bitmap_l);
    line += " bitmap=" + std::string(b64);
    uint64_t bm = (uint64_t(bitmap_h) << 32) | bitmap_l;
    int nset = 0;
    std::string bits;
    for (int i = 0; i < 64; ++i) {
      if (bm & (uint64_t(1) << i)) {
        if (nset) bits += ",";
        bits += dbz3::Dec(i);
        ++nset;
      }
    }
    line += " set(" + dbz3::Dec(nset) + ")=" + bits;
    dbz3::TraceLine(line);
  }
  __imp__sub_8217ADE8(ctx, base);
  if (trace) {
    uint32_t resolved = REX_LOAD_U8(obj + 4);
    uint32_t pbm_l = REX_LOAD_U32(obj + 16);
    uint32_t pbm_h = REX_LOAD_U32(obj + 20);
    if (resolved != slot || pbm_l != bitmap_l || pbm_h != bitmap_h) {
      char b64[32];
      std::snprintf(b64, sizeof(b64), "%08x%08x", pbm_h, pbm_l);
      std::string line = "ADE8 post obj=" + dbz3::Hx(obj) +
                         " slot=" + dbz3::Dec(slot) + " -> " + dbz3::Dec(resolved) +
                         " bitmap=" + std::string(b64);
      uint64_t bm = (uint64_t(pbm_h) << 32) | pbm_l;
      int nset = 0;
      std::string bits;
      for (int i = 0; i < 64; ++i) {
        if (bm & (uint64_t(1) << i)) {
          if (nset) bits += ",";
          bits += dbz3::Dec(i);
          ++nset;
        }
      }
      line += " set(" + dbz3::Dec(nset) + ")=" + bits;
      dbz3::TraceLine(line);
    }
  }
}

//------------------------------------------------------------------------------
// sub_82180AA0: selection update. Reads P1/P2 char indices (stride 184) and the
// +14/+18/+114 u16 fields of each record. Logs the state before delegating.
//------------------------------------------------------------------------------
// Llamado desde el gancho de select_ext.cpp (un solo gancho por funcion).
void dbz3_trace_sub_82180AA0(PPCContext& ctx, uint8_t* base) {
  const bool trace = dbz3::TraceEnabled();
  uint32_t obj = ctx.r3.u32;
  uint32_t inner = REX_LOAD_U32(obj + 48);  // r30 = *(r3+48)
  uint32_t flags = REX_LOAD_U8(inner + 3);  // r6 = byte flags
  uint32_t p1 = REX_LOAD_U32(dbz3::kIdxP1);
  uint32_t p2 = REX_LOAD_U32(dbz3::kIdxP2);
  if (trace) {
    std::string line = "80AA0 obj=" + dbz3::Hx(obj) + " inner=" + dbz3::Hx(inner) +
                       " flags=0x" + dbz3::Hx(flags) + " P1=" + dbz3::Dec(p1) +
                       " P2=" + dbz3::Dec(p2);
    if (flags & 1) {
      uint32_t rec = dbz3::kCharBase + p1 * 184;
      line += " rec1+14=" + dbz3::Dec(REX_LOAD_U16(rec + 14)) +
              " +18=" + dbz3::Dec(REX_LOAD_U16(rec + 18)) +
              " +114=" + dbz3::Dec(REX_LOAD_U16(rec + 114));
    }
    if (flags & 2) {
      uint32_t rec = dbz3::kCharBase + p2 * 184;
      line += " rec2+14=" + dbz3::Dec(REX_LOAD_U16(rec + 14)) +
              " +18=" + dbz3::Dec(REX_LOAD_U16(rec + 18)) +
              " +114=" + dbz3::Dec(REX_LOAD_U16(rec + 114));
    }
    dbz3::TraceLine(line);
  }
}

// Traza de voces (prueba, DBZ3_TEST_VOCES=1; R7 ginyu_janemba): que ID y "situacion" pide una voz de combate
// (sub_820F3980: r4 ID, r5 situacion -> tabla 0x823280B0) y que ADX se abre al final (sub_82081FE8: r4 =
// fichero<<16 | entrada). Sirve para comprobar de quien es la voz que suena sin oirla.
namespace {
bool VocesTrace() {
  static const bool on = std::getenv("DBZ3_TEST_VOCES") != nullptr;
  return on;
}
}  // namespace

REX_HOOK_RAW(sub_820F3980) {
  if (VocesTrace()) {
    const int32_t id = ctx.r4.s32, sit = ctx.r5.s32;
    int32_t adx = -9;
    if (id >= 0 && id < 122 && sit >= 0 && sit < int32_t(REX_LOAD_U32(0x82328298 + 4 * id))) {
      if (const uint32_t blk = REX_LOAD_U32(0x823280B0 + 4 * id)) adx = int32_t(REX_LOAD_U32(blk + 4 * sit));
    }
    REXLOG_INFO("dbz3 voces: id {} situacion {} -> adx {} (lr {:08X})", id, sit, adx, uint32_t(ctx.lr));
  }
  __imp__sub_820F3980(ctx, base);
}

REX_HOOK_RAW(sub_82081FE8) {
  if (VocesTrace())
    REXLOG_INFO("dbz3 voces: stream fichero {} entrada {} (lr {:08X})", ctx.r4.u32 >> 16, ctx.r4.u32 & 0xFFFF,
                uint32_t(ctx.lr));
  __imp__sub_82081FE8(ctx, base);
}
