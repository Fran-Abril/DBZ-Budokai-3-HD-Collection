// dbz3 - Fusion en combate (ronda 3, frente B; docs/14_frentes/RONDA3_ENJAMBRE.md).
//
// En B3 la fusion (Gogeta, Gotenks, Vegito) no es una forma: es una tecnica que cambia el personaje en pleno
// combate. Aqui igual, con el cambio de personaje del relevo del Tag Team (tagteam.cpp Fuse: cargas del juego,
// misma vida, pose de entrada = carga de ki): Yamcha (13) o Ten Shin Han (12), en un combate normal, con 3
// barras de ki o mas y sin forma a la que subir, MANTIENEN el boton de transformarse (P+K+G) 1 s y al soltar pasan a
// Yamhan (mod vis_b2_yamhan, nombre interno "YAMHAN"); gasta las 3 barras. Sin el mod, o online, no hace nada.
// PRUEBA: DBZ3_TEST_FUSION=N -> el lado 1 se fusiona a los N s de pelea (con el ki que haga falta).
#include <rex/logging.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <string>

#include "generated/dbz3_init.h"
#include "eu_map.h"
#include "net/netplay.h"
#include "roster_ext.h"

namespace dbz3::tag { bool Fuse(PPCContext& ctx, uint8_t* base, int side, int id); }

namespace dbz3::fusion {
namespace {

constexpr dbz3::GA kMgr = 0x823D6CF4, kPadRec = 0x8238B780;   // luchador k: mgr + 2096 + 8240k; mando: 184 B por hueco
constexpr uint32_t kAct = 3244, kForm = 2980, kId = 7420, kPadSlot = 5028, kKi = 6100;
constexpr float kCost = 3000.0f;       // 3 barras (1 barra = 1000)
constexpr uint32_t kHold = 60;         // ticks manteniendo P+K+G

float LoadF(uint8_t* base, uint32_t a) {
  const uint32_t v = REX_LOAD_U32(a);
  float f;
  std::memcpy(&f, &v, 4);
  return f;
}
void StoreF(uint8_t* base, uint32_t a, float f) {
  uint32_t v;
  std::memcpy(&v, &f, 4);
  REX_STORE_U32(a, v);
}

// ID de Yamhan en el roster de mods (roster.toml, nombre = "YAMHAN"); -1 si el mod no esta
int YamhanId() {
  static int id = -2;
  if (id == -2) {
    id = -1;
    for (uint32_t k = 22; k < 105 && id < 0; ++k)
      if (dbz3::roster::InternalName(k) == "YAMHAN") id = int(k);
    REXLOG_INFO("dbz3 fusion: Yamhan {}", id >= 0 ? "id " + std::to_string(id) : std::string("no esta (sin mod)"));
  }
  return id;
}

// P, K y G mantenidos en el mando del luchador (registro de 184 B: +12 pulsados, bit 0x100 << j; la configuracion
// del jugador da a cada j su boton: mgr + (hueco + 4382) * 8 + j = 1 P, 2 K, 4 G; como formas::PruebaMando)
bool PkgHeld(uint8_t* base, uint32_t mgr, uint32_t x) {
  const int16_t slot = int16_t(REX_LOAD_U16(x + kPadSlot));
  if (slot < 0 || slot > 3) return false;   // el COM no tiene mando
  const uint16_t held = REX_LOAD_U16(kPadRec + 184 * uint32_t(slot) + 12);
  static const bool probe = std::getenv("DBZ3_FUSION_SONDA") != nullptr;   // botones que ve el combate
  if (static uint16_t last = 0; probe && held != last) {
    last = held;
    REXLOG_INFO("dbz3 fusion sonda: hueco {} botones {:04X} (config {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X})", slot,
                held, REX_LOAD_U8(mgr + (uint32_t(slot) + 4382) * 8), REX_LOAD_U8(mgr + (uint32_t(slot) + 4382) * 8 + 1),
                REX_LOAD_U8(mgr + (uint32_t(slot) + 4382) * 8 + 2), REX_LOAD_U8(mgr + (uint32_t(slot) + 4382) * 8 + 3),
                REX_LOAD_U8(mgr + (uint32_t(slot) + 4382) * 8 + 4), REX_LOAD_U8(mgr + (uint32_t(slot) + 4382) * 8 + 5),
                REX_LOAD_U8(mgr + (uint32_t(slot) + 4382) * 8 + 6), REX_LOAD_U8(mgr + (uint32_t(slot) + 4382) * 8 + 7));
  }
  for (const uint8_t want : {1, 2, 4}) {
    bool ok = false;
    for (uint32_t j = 0; j < 8 && !ok; ++j)
      ok = REX_LOAD_U8(mgr + (uint32_t(slot) + 4382) * 8 + j) == want && (held & (0x100u << j));
    if (!ok) return false;
  }
  return true;
}

// la subida nativa (sub_82119108 solo calcula la forma a la que subiria P+K+G) no tiene adonde ir
bool NoFormLeft(PPCContext& ctx, uint8_t* base, uint32_t x) {
  PPCContext c = ctx;
  c.r3.u64 = x;
  c.r4.u64 = 0;
  DBZ3_CALL_EU(sub_82119108)(c, base);
  return c.r3.s32 <= int32_t(REX_LOAD_U32(x + kForm));
}

}  // namespace

void Tick(PPCContext& ctx, uint8_t* base) {
  static std::array<uint32_t, 2> hold{}, armed{};
  static uint32_t fight = 0;
  static const int test_s = std::getenv("DBZ3_TEST_FUSION") ? std::atoi(std::getenv("DBZ3_TEST_FUSION")) : -1;
  const uint32_t mgr = REX_LOAD_U32(kMgr);
  if (!mgr || REX_LOAD_U32(mgr + 35128) != 4 || dbz3::net::Session::Get().link() != dbz3::net::Link::kIdle) {
    hold = {};
    armed = {};
    if (!mgr || REX_LOAD_U32(mgr + 35128) < 4) fight = 0;
    return;
  }
  ++fight;
  for (int p = 0; p < 2; ++p) {
    const uint32_t x = mgr + 2096 + 8240 * uint32_t(p);
    const uint32_t id = REX_LOAD_U32(x + kId);
    const bool test = p == 0 && test_s >= 0 && fight >= uint32_t(test_s) * 60 && fight < uint32_t(test_s) * 60 + 600;
    hold[size_t(p)] = PkgHeld(base, mgr, x) ? hold[size_t(p)] + 1 : 0;
    if ((id == 12 || id == 13) && hold[size_t(p)] == kHold)
      REXLOG_INFO("dbz3 fusion: lado {} mantiene P+K+G (id {}, ki {:.0f}, accion {})", p + 1, id, LoadF(base, x + kKi),
                  REX_LOAD_U16(x + kAct));
    // mientras se mantiene el juego lo tiene en otra accion (16): la fusion sale al soltar (hasta 1,5 s despues)
    if (hold[size_t(p)] >= kHold) armed[size_t(p)] = fight + 90;
    const bool want = test || (armed[size_t(p)] && fight <= armed[size_t(p)]);
    if ((id != 12 && id != 13) || REX_LOAD_U16(x + kAct) > 2 || !want) continue;
    if (test && LoadF(base, x + kKi) < kCost) StoreF(base, x + kKi, kCost);
    const int to = YamhanId();
    if (to < 0 || LoadF(base, x + kKi) < kCost || !NoFormLeft(ctx, base, x)) continue;
    if (!dbz3::tag::Fuse(ctx, base, p, to)) continue;
    StoreF(base, x + kKi, LoadF(base, x + kKi) - kCost);
    hold[size_t(p)] = armed[size_t(p)] = 0;
    REXLOG_INFO("dbz3 fusion: lado {}: id {} -> Yamhan (id {}), ki que queda {:.0f}{}", p + 1, id, to, LoadF(base, x + kKi),
                test ? " (prueba)" : "");
  }
}

}  // namespace dbz3::fusion
