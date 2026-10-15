// dbz3 - Frente Ports: guarda de la ranura de tecnicas (BSP) con mas de 64 recursos (2026-10-15).
//
// sub_82126490 (procesar la ranura de tecnicas j del lado k; ranura = *0x82333994 + 1152k + 544j + 80) apunta
// cada recurso del BSP (hijos de [0], hijos de [1] y hijos de la raiz desde el 2, en ese orden) en
// ranura+32+4i y, si el recurso es un #AWO, crea su instancia en ranura+288+4i. Hay sitio para 64 (los BSP del
// juego llegan a 52). El de Vegeta GT de Infinite World tiene 65: el recurso 64 (#CWV) cae en ranura+288, la
// instancia 0. Durante el combate no pasa nada (sub_820CF580 busca en ranura+32+4*indice y encuentra el #CWV),
// pero al acabar sub_821266A8 libera las 64 "instancias" con sub_82089C10 y libera ese bloque del fichero:
// lectura de 0x10 y el juego se cierra (D:\DBZ3HD\_pruebas\20261015-015446_modos_test_vegeta_gt_sin_modos,
// lr 0x82089C34, r31 = el #CWV en +0xA1700 del BSP).
// Arreglo: antes de liberar, las entradas desbordadas (punteros a datos del BSP, no instancias) se ponen a 0.
// US y EU (la base y las funciones EU, de src/eu_map_gen.h). roster_build.py avisa de los BSP de mas de 64.
//
// Traje de epoca enlazado (ficha 07, 2026-10-15): un traje extra de traje.toml con `enlazado = "<mod>"` lleva en
// mods/_roster/roster.toml `traje_enlazado = [traje, ID]`. Justo antes de montar el combate (sub_820FE668), el
// jugador que lleva ese traje pasa a ser el personaje ID (traje 0), con su moveset, tecnicas, aura y sus capsulas por
// defecto (precedencia: el set del traje manda sobre el del jugador para el personaje base). En el select, mientras
// se mira ese traje, un rotulo lo avisa (nombre de epoca + "golpes y capsulas propios").
#include <imgui.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/ui/imgui_dialog.h>
#include <toml++/toml.hpp>

#include <atomic>
#include <cstdint>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <utility>

#include "generated/dbz3_init.h"
#include "eu_map.h"
#include "ingame/game_style.h"
#include "launcher/i18n.h"
#include "launcher/launcher_state.h"
#include "form_selection_state.h"
#include "mods.h"

bool dbz3_select_live();               // select_ext.cpp
int64_t dbz3_select_browse_ms(int k);
int64_t dbz3_select_now_ms();
namespace dbz3 {
bool GameDimmed();
bool OverlayOpen();
}  // namespace dbz3

namespace {
constexpr dbz3::GA kSlotsBase = 0x82333994;
constexpr uint32_t kCap = 64;
}  // namespace

DBZ3_HOOK_EU(sub_821266A8) {
  {
    const uint32_t base0 = REX_LOAD_U32(kSlotsBase);
    const uint32_t k = ctx.r3.u32, j = ctx.r4.u32;
    if (base0 && k < 2 && j < 2) {
      const uint32_t slot = base0 + 1152 * k + 544 * j + 80;
      const uint32_t bin = REX_LOAD_U32(slot + 4);
      if (bin) {
        const uint32_t n01 = REX_LOAD_U32(slot + 16);   // recursos de [0] y [1]
        const uint32_t ntop = REX_LOAD_U32(bin + 16);  // hijos de la raiz
        const uint32_t total = n01 + (ntop > 2 ? ntop - 2 : 0);
        if (total > kCap && total < 2 * kCap) {
          for (uint32_t g = kCap; g < total; ++g) REX_STORE_U32(slot + 32 + 4 * g, 0);
          REXLOG_WARN("dbz3 ports: tecnicas del lado {} con {} recursos (el juego admite {}): {} sin liberar "
                      "(eran datos del fichero, no instancias)", k, total, kCap, total - kCap);
        }
      }
    }
  }
  orig(ctx, base);
}

namespace {
constexpr dbz3::GA kCfg = 0x8245EFC0;

struct Enlace { int id = -1; std::string nombre; };
// (ID, traje) -> personaje enlazado; se lee una vez (el roster no cambia con el juego abierto)
const std::map<std::pair<int, int>, Enlace>& Enlaces() {
  static const std::map<std::pair<int, int>, Enlace> m = [] {
    std::map<std::pair<int, int>, Enlace> r;
    std::ifstream f(dbz3::ModsRoot() / "_roster" / "roster.toml", std::ios::binary);
    if (!f) return r;
    std::stringstream ss;
    ss << f.rdbuf();
    try {
      const toml::table t = toml::parse(ss.str());
      std::map<int, std::string> nombres;
      const toml::array* a = t["personaje"].as_array();
      if (!a) return r;
      for (const toml::node& n : *a)
        if (const toml::table* x = n.as_table())
          if (auto nom = (*x)["nombre"].value<std::string>()) nombres[int((*x)["id"].value_or(int64_t(-1)))] = *nom;
      for (const toml::node& n : *a) {
        const toml::table* x = n.as_table();
        const toml::array* e = x ? (*x)["traje_enlazado"].as_array() : nullptr;
        if (!e || e->size() != 2) continue;
        const int id = int((*x)["id"].value_or(int64_t(-1)));
        const int traje = int((*e)[0].value_or(int64_t(-1))), dest = int((*e)[1].value_or(int64_t(-1)));
        if (id < 0 || traje < 0 || dest < 0 || dest >= 64) continue;
        r[{id, traje}] = {dest, nombres.count(dest) ? nombres[dest] : std::string()};
        REXLOG_INFO("dbz3 ports: traje {} del personaje {} enlazado al personaje {} ({})", traje + 1, id, dest,
                    r[{id, traje}].nombre);
      }
    } catch (const toml::parse_error&) {
    }
    return r;
  }();
  return m;
}

namespace S = dbz3::ingame::style;
class TrajePanel : public rex::ui::ImGuiDialog {
 public:
  explicit TrajePanel(rex::ui::ImGuiDrawer* drawer) : ImGuiDialog(drawer) {}

 protected:
  void OnDraw(ImGuiIO& io) override {
    if (Enlaces().empty() || !dbz3_select_live() || dbz3::GameDimmed() || dbz3::OverlayOpen()) return;
    const S::Area a = S::GameArea(io);
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    for (int side = 0; side < 2; ++side) {
      if (dbz3_select_now_ms() - dbz3_select_browse_ms(side) > 300) continue;
      const auto snap = dbz3::forms::Copy(side).snapshot;
      const auto it = Enlaces().find({snap.id, snap.suit});
      if (it == Enlaces().end()) continue;
      const float cx = side == 0 ? 210.0f : 990.0f, y = 400.0f;   // bajo la ayuda LT/RT de la forma (292-330)
      if (!it->second.nombre.empty()) S::MenuText(dl, a.At(cx, y), 26.0f * a.s, it->second.nombre.c_str(), true, false);
      S::MenuText(dl, a.At(cx, y + 28.0f), 18.0f * a.s,
                  dbz3::i18n::T("Traje de época: golpes y cápsulas propios", "Era costume: own moves and capsules"),
                  false, false);
    }
  }
};

std::atomic<bool> g_panel{false};
void CrearPanel() {   // el panel vive lo que el juego (el dibujante no borra sus dialogos)
  auto* rt = rex::Runtime::instance();
  if (!g_panel && rt && rt->imgui_drawer()) g_panel = true, new TrajePanel(rt->imgui_drawer());
}
}  // namespace

// Comprobacion que hace la espera de la escena 24 (sub_821D7400) justo antes de pasar la CFG a sub_820FE668: si
// devuelve 0 con la escena 25 pedida, el combate se monta a continuacion. Es el ultimo punto tras los trajes que
// pone modos.cpp (su gancho de sub_821D7400 escribe la CFG antes de esta llamada). Tras montarlo se devuelve la CFG
// como estaba (la casilla y el traje del select, el ranking de los retos); la revancha vuelve a pasar por aqui.
DBZ3_HOOK_EU(sub_821D7FE8) {
  static uint32_t llamadas = 0;
  if (!g_panel && llamadas++ % 600 == 0) dbz3::launcher::DeferUi(&CrearPanel);   // hasta que haya dibujante
  struct Hecho { int id = -1, traje = 0, dest = -1; uint8_t caps[16] = {}; };   // caps: rec+52..+67
  static Hecho hecho[2];
  const uint32_t lr = uint32_t(ctx.lr);
  for (uint32_t k = 0; k < 2; ++k) {
    const uint32_t rec = kCfg + 16 + 80 * k;
    if (hecho[k].dest >= 0 && REX_LOAD_U16(rec + 16) == uint32_t(hecho[k].dest) && REX_LOAD_U16(rec + 18) == 0) {
      REX_STORE_U16(rec + 16, uint16_t(hecho[k].id));
      REX_STORE_U16(rec + 18, uint16_t(hecho[k].traje));
      for (uint32_t b = 0; b < 16; ++b) REX_STORE_U8(rec + 52 + b, hecho[k].caps[b]);
    }
    hecho[k].dest = -1;
  }
  orig(ctx, base);
  if (lr == dbz3::GA(0x821D7428) && ctx.r3.u32 == 0 && REX_LOAD_U32(dbz3::GA(0x8247A990)) == 25 &&
      !Enlaces().empty()) {
    for (uint32_t k = 0; k < 2; ++k) {
      const uint32_t rec = kCfg + 16 + 80 * k;
      const int id = int(REX_LOAD_U16(rec + 16)), traje = int(REX_LOAD_U16(rec + 18));
      const auto it = Enlaces().find({id, traje});
      if (it == Enlaces().end()) continue;
      hecho[k] = {id, traje, it->second.id};
      for (uint32_t b = 0; b < 16; ++b) hecho[k].caps[b] = REX_LOAD_U8(rec + 52 + b);
      REX_STORE_U16(rec + 16, uint16_t(it->second.id));
      REX_STORE_U16(rec + 18, 0);
      // capsulas del traje = las del personaje enlazado (char96 +80 cuantas, +82 IDs; como roster_ext al
      // cambiar de ID en sub_821007E8): el set del jugador para Vegeta no vale para otro moveset
      const uint32_t c96 = dbz3::GA(0x8234ABB8) + uint32_t(it->second.id) * 96;
      for (uint32_t b = 0; b < 16; ++b) REX_STORE_U8(rec + 52 + b, REX_LOAD_U8(c96 + 80 + b));
      REXLOG_INFO("dbz3 ports: jugador {} con el traje {} del personaje {}: combate con el personaje {} ({})", k + 1,
                  traje + 1, id, it->second.id, it->second.nombre);
    }
  }
}

// El Duelo del juego no pasa por la espera de la escena 24: sub_820FFA80 (llamada desde 0x821D634C) arma la CFG del
// combate en la pila con el ultimo luchador del Duelo (partida G = 0x824BA110: +31+k ID, +33+k traje, u8) y las
// capsulas de 0x8245EF92 + 16k (u16 cuantas, 7 u16), y llama a sub_820FE668. Se cambia la fuente mientras dura y
// se devuelve despues (el select vuelve con su casilla y su traje; la revancha vuelve a pasar por aqui).
DBZ3_HOOK_EU(sub_820FFA80) {
  struct Hecho { int id = -1, traje = 0, dest = -1; uint8_t caps[16] = {}; };
  Hecho hecho[2];
  constexpr dbz3::GA kG = 0x824BA110, kCaps = 0x8245EF92;
  if (!Enlaces().empty()) {
    for (uint32_t k = 0; k < 2; ++k) {
      const int id = int(REX_LOAD_U8(kG + 31 + k)), traje = int(REX_LOAD_U8(kG + 33 + k));
      const auto it = Enlaces().find({id, traje});
      if (it == Enlaces().end()) continue;
      hecho[k] = {id, traje, it->second.id};
      const uint32_t caps = kCaps + 16 * k, c96 = dbz3::GA(0x8234ABB8) + uint32_t(it->second.id) * 96;
      for (uint32_t b = 0; b < 16; ++b) hecho[k].caps[b] = REX_LOAD_U8(caps + b), REX_STORE_U8(caps + b, REX_LOAD_U8(c96 + 80 + b));
      REX_STORE_U8(kG + 31 + k, uint8_t(it->second.id));
      REX_STORE_U8(kG + 33 + k, 0);
      REXLOG_INFO("dbz3 ports: jugador {} con el traje {} del personaje {}: combate con el personaje {} ({}) (Duelo)",
                  k + 1, traje + 1, id, it->second.id, it->second.nombre);
    }
  }
  orig(ctx, base);
  for (uint32_t k = 0; k < 2; ++k) {
    if (hecho[k].dest < 0 || REX_LOAD_U8(kG + 31 + k) != uint32_t(hecho[k].dest)) continue;
    REX_STORE_U8(kG + 31 + k, uint8_t(hecho[k].id));
    REX_STORE_U8(kG + 33 + k, uint8_t(hecho[k].traje));
    for (uint32_t b = 0; b < 16; ++b) REX_STORE_U8(kCaps + 16 * k + b, hecho[k].caps[b]);
  }
}
