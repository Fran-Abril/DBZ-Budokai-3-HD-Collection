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

// Efecto de tecnica sin funcion (s9, 2026-10-19): sub_82130DA0(luchador r3, ...) busca con sub_820CD6C0(*(r3+4944) como
// s16, 1) el registro del efecto y llama por la tabla 0x823339A0[tipo] (tipo = s16 en *(registro+32+12)+16) sin mirar si
// la entrada existe. Con Paikuhan (port de IW, capsulas 616-618) el tipo no tiene funcion: llamada a 0 y cierre
// (lr 0x82130F24, medido: tipo = s16 en r4+16; D:\DBZ3HD\_pruebas61019-*_s56_test_iw_dm_087, mision 87 de Dragon Mission). Si la entrada es 0
// se salta el efecto entero (sin el, la tecnica sigue; solo falta su chispa) en vez de cerrar el juego.
DBZ3_HOOK_EU(sub_82130DA0) {
  // el descriptor del efecto es el 2.o argumento (r4; la funcion lo guarda en registro+32+12 y llama por su tipo, +16)
  if (const uint32_t desc = ctx.r4.u32) {
    constexpr dbz3::GA kTablaEfectos = 0x823339A0;
    const int16_t tipo = int16_t(REX_LOAD_U16(desc + 16));
    if (tipo < 0 || tipo > 4096 || !REX_LOAD_U32(uint32_t(kTablaEfectos) + 4 * uint32_t(tipo))) {
      static std::atomic<int> avisos{0};
      if (avisos.fetch_add(1) < 8)
        REXLOG_WARN("dbz3 ports: efecto de tecnica de tipo {} sin funcion (descriptor {:08X}): se salta", tipo, desc);
      ctx.r3.u64 = 0;   // como cuando sub_820CD6C0 no encuentra el registro (salto directo al final con r3 = 0)
      return;
    }
  }
  orig(ctx, base);
}

// Cadenas de fisica del modelo (feedback 1.4.6, Tag Team Janemba + Super Bu, logs/dbz3_1034.log): sub_82134640
// (cargar modelo: r27 ID, r23 indice de forma, r25 traje, r24 jugador) lee el registro de 12 B
// char96[ID].lista[byte(+18+8*indice) + por_traje*traje] y pasa su +4 a esta funcion, que lee alli 3 cuentas y
// sus punteros. Un registro fuera de la lista (traje o indice que el ID no tiene) trae basura (0x54): cierre en
// lr 0x82138910. Un +4 que no es puntero se trata como "sin cadenas" (0, lo mismo que los ports sin `fisica`).
DBZ3_HOOK_EU(sub_821388D0) {
  if (const uint32_t p = ctx.r3.u32; p && p < 0x1000) {
    static std::atomic<int> avisos{0};
    if (avisos.fetch_add(1) < 8)
      REXLOG_WARN("dbz3 ports: cadenas de fisica no validas ({:X}) en el modelo de id {} forma {} traje {} jugador {} "
                  "(registro {:08X}): se carga sin ellas", p, ctx.r27.s32, ctx.r23.s32, ctx.r25.s32, ctx.r24.s32,
                  ctx.r28.u32);
    ctx.r3.u64 = 0;
    return;
  }
  orig(ctx, base);
}

// Clip de camara que el fichero no tiene (R9, 2026-10-16): sub_82082468(destino r3, #ACC r4, clip r5, frame f1) lee
// la tabla de clips (+0x14; 16 B: +8 frames, +12 claves) sin mirar la cuenta (+0x10). Con la camara de un port que
// tiene menos clips que la del donante (roster montado antes de cam_pad_clips de roster_build.py) lee basura: cierre
// al acabar el vuelo de la presentacion (lr 0x820825A8) o en la pose de victoria (Pan de IW, 21 clips, pide el 23:
// frames 0x40D4A316 y claves en 0x1, lr 0x82082470; D:\DBZ3HD\_pruebas\20261016-024812_modos_test_iw_mapa).
// Se usa el ultimo clip que hay; sin clips no se mueve la camara. El montaje actual ya rellena los clips del donante.
DBZ3_HOOK_EU(sub_82082468) {
  if (const uint32_t acc = ctx.r4.u32) {
    const uint32_t n = REX_LOAD_U32(acc + 16);
    if (ctx.r5.u32 >= n) {
      static std::atomic<int> avisos{0};
      if (avisos.fetch_add(1) < 8)
        REXLOG_WARN("dbz3 ports: clip de camara {} y la camara ({:08X}) tiene {}: {}", ctx.r5.u32, acc, n,
                    n ? "se usa el ultimo" : "no se mueve");
      if (!n) return;
      ctx.r5.u64 = n - 1;
    }
  }
  orig(ctx, base);
}

// Ordenes de guion (#SPX) de Infinite World que B3 no tiene (R9, 2026-10-16). El interprete (sub_8209F568) busca la
// orden N en filas[N >> 4][N & 15] (tabla 0x82332670) sin mirar el largo de la fila. La fila 8 de B3 tiene 4 ordenes
// (0x80-0x83) y la de IW 12 (tabla del ELF USA 0x457AC0): las 0x86 y 0x88 de IW (0x23AF00: nivel de las capsulas
// 0x49-0x4D del lado p[0]; 0x23AD00: dato del registro de su capsula actual, 1 si no hay) caen en B3 en las 0x92/0x94,
// sub_8211E520, que hace luchador = mgr[8797 + p[1]] y cambia su estado: con un solo argumento p[1] es basura y el
// juego se cerraba (Goku GT de IW, definitiva 359: p[1] = 0x4CE81EF, lr 0x8209F668). Aqui, con clase/fila de entrada
// (r10/4, r9/4 del interprete): las de IW se saltan sin tocar nada (su resultado se descarta en el guion: `12 10 04`;
// escribir en cmd+0, donde IW deja el resultado, colgaba la definitiva: en B3 cmd+0 es del interprete), y cualquier
// otra llamada con p[1] que no es un lado tambien.
DBZ3_HOOK_EU(sub_8211E520) {
  const uint32_t cmd = ctx.r4.u32, p = cmd ? REX_LOAD_U32(cmd + 44) : 0;
  const uint32_t clase = ctx.r10.u32 / 4, fila = ctx.r9.u32 / 4;
  const bool iw = clase == 8 && (fila == 6 || fila == 8);
  if (cmd && (iw || (p && REX_LOAD_U32(p + 4) > 1))) {
    static std::atomic<int> avisos{0};
    if (avisos.fetch_add(1) < 8)
      REXLOG_WARN("dbz3 ports: orden de guion {:X}{} (lado {:X}, pc {:08X}): {}", clase * 16 + fila,
                  iw ? " de Infinite World" : "", p ? REX_LOAD_U32(p + (iw ? 0 : 4)) : 0u, REX_LOAD_U32(cmd + 36),
                  "se salta");
    ctx.r3.u64 = 1;
    return;
  }
  orig(ctx, base);
}

// Las otras ordenes de IW que usan sus ports (Goku GT: 0x35-0x37 aura/malla y 0x99-0x9A rafagas de ki, filas 3 y 9
// mas largas en IW) caen en B3 en las 0x60-0x62 y 0xA0-0xA1 y cambiaban otra cosa: se saltan.
namespace {
// true (y avisa) si la orden que llega (clase r10/4, fila r9/4) es la de IW que cae en este manejador de B3
bool SaltaOrdenIW(PPCContext& ctx, uint8_t* base, uint32_t c, uint32_t f) {
  if (!ctx.r4.u32 || ctx.r10.u32 / 4 != c || ctx.r9.u32 / 4 != f) return false;
  static std::atomic<int> avisos{0};
  if (avisos.fetch_add(1) < 8)
    REXLOG_WARN("dbz3 ports: orden de guion {:X} de Infinite World (pc {:08X}): se salta", c * 16 + f,
                REX_LOAD_U32(ctx.r4.u32 + 36));
  ctx.r3.u64 = 1;
  return true;
}
}  // namespace
DBZ3_HOOK_EU(sub_8211E148) { if (!SaltaOrdenIW(ctx, base, 3, 5)) orig(ctx, base); }
DBZ3_HOOK_EU(sub_8211E178) { if (!SaltaOrdenIW(ctx, base, 3, 6)) orig(ctx, base); }
DBZ3_HOOK_EU(sub_8211E1B0) { if (!SaltaOrdenIW(ctx, base, 3, 7)) orig(ctx, base); }
DBZ3_HOOK_EU(sub_8211E6A0) { if (!SaltaOrdenIW(ctx, base, 9, 9)) orig(ctx, base); }
DBZ3_HOOK_EU(sub_8211E6F8) { if (!SaltaOrdenIW(ctx, base, 9, 10)) orig(ctx, base); }
DBZ3_HOOK_EU(sub_8211E578) { if (!SaltaOrdenIW(ctx, base, 8, 7)) orig(ctx, base); }   // Vegeta saga Saiyan de IW
DBZ3_HOOK_EU(sub_8211E2B8) { if (!SaltaOrdenIW(ctx, base, 3, 11)) orig(ctx, base); }

// Nodo de efecto reutilizado en uso (R9, 2026-10-16): sub_82155990 coge el siguiente de un anillo de 128 nodos
// (punteros en 0x824370B0, indice en 0x823EE81C) sin mirar si sigue enlazado en la lista ordenada (centinela
// 0x823EE820; nodo: +0 siguiente, +4 anterior, +12 prioridad), lo pone a cero y lo vuelve a insertar: si seguia
// en la lista, sus vecinos apuntan a un nodo con siguiente = 0 y el recorrido lee 0xC (Goku GT de IW con muchas
// definitivas: lr 0x82155A04, D:\DBZ3HD\_pruebas\20261016-051458_ports_test_combate_port). Antes de reutilizarlo
// se saca de la lista; el efecto que lo tenia pierde ese nodo, como antes, pero la lista queda bien.
DBZ3_HOOK_EU(sub_82155990) {
  constexpr dbz3::GA kIdx = 0x823EE81C, kPool = 0x824370B0;
  const uint32_t i = REX_LOAD_U32(kIdx);
  if (const uint32_t node = i < 128 ? REX_LOAD_U32(kPool + 4 * i) : 0) {
    const uint32_t nx = REX_LOAD_U32(node), pv = REX_LOAD_U32(node + 4);
    if (nx && pv && nx != node && REX_LOAD_U32(pv) == node && REX_LOAD_U32(nx + 4) == node) {
      REX_STORE_U32(pv, nx);
      REX_STORE_U32(nx + 4, pv);
      static std::atomic<int> avisos{0};
      if (avisos.fetch_add(1) < 4)
        REXLOG_WARN("dbz3 ports: nodo de efecto {} reutilizado aun en la lista ({:08X}): se saca antes", i, node);
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
