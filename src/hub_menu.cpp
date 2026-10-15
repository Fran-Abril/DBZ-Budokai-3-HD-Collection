// dbz3 - Fila HUB del menu principal y pantalla HUB: la logica, en el hilo del juego (ver
// src/ingame/hub_panel.h y docs/12_conectores/11_menu_principal_nativo.md s9). Solo US.
//
// El menu principal (objeto de 172 B en tarea+48) sigue creyendo que tiene sus 6 o 7 filas: la fila HUB
// es virtual y no se escribe nunca en el objeto ni en la partida (0x824BA135). Su estado activo
// (sub_8218C5D8) dibuja (sub_8218BBC0: fondo sub_8218B2F0 ... filas sub_8218B550) y despues lee el mando
// del juego (0x8238B780): al principio de su dibujo (gancho de B2F0, nada pintado aun) se decide con ese
// mismo mando y, si lo usa el HUB, se borra antes de que el menu lo lea. Con el cursor en el HUB, el del
// juego queda en la fila 0 (asi el contador de filas de online_panel nunca ve la ultima) y se dibuja sin
// fila elegida ni franja. En la pantalla HUB el juego sigue dibujando su fondo, su marco, su cinta y su
// bocadillo; se le quitan las etiquetas (+151 a 0) y el texto de la cinta, y hub_panel.cpp pinta lo
// nuestro en su sitio.
// Ida y vuelta entre menu y HUB con las animaciones del propio menu (las de su A y su B: sub_8218C588 /
// sub_8218C520 sin pedir pantalla) y un fundido a negro como el de sus pantallas; en negro se rehacen sus
// piezas de entrada (las de sub_8218C4D0, soltando antes las que habia) y entra otra vez como al nacer.
// Ganchos: sub_8218B2F0, sub_8218B550, sub_8218B3F0, sub_8218B998, sub_8217E800 (uno por funcion en todo
// el exe).
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <mutex>
#include <random>

#include "generated/dbz3_init.h"
#include "guest_region.h"
#include "ingame/hub_panel.h"
#include "ingame/menu_pilot.h"
#include "ingame/online_panel.h"
#include "ingame/tagteam_panel.h"
#include "launcher/launcher_state.h"

REXCVAR_DEFINE_BOOL(dbz3_hub_enabled, true, "DBZ3/Mods", "HUB row in the main menu (Tag Team, Online, quick battle...)");
REXCVAR_DECLARE(bool, dbz3_tag_team_enabled);   // online_panel.cpp
REXCVAR_DECLARE(bool, dbz3_online_enabled);

namespace dbz3::ingame {
bool Online();   // tagteam_panel.cpp: sesion online abierta
}

namespace dbz3::hub {
bool RequestBattle(int id1, int id2, int stage, bool p2_com);   // hub.cpp

namespace {
constexpr uint32_t kPad = 0x8238B780, kPadIdx = 0x8238B670;     // mando del juego: 184 B por jugador
constexpr uint32_t kGame = 0x824BA110, kHelp = 0x82020D54;       // bocadillo de cada fila (u16)
constexpr uint32_t kSeVolume = kGame + 0x2C0C0, kSeFloor = 0x8201F60C;
constexpr uint32_t kArenaFlag = kGame + 169;                      // Dragon Arena (lo lee sub_821D3B78)
// tarea del menu principal: *(*(0x8245F230) + 16) (sub_8218CB88); sus estados
constexpr uint32_t kMenuMgr = 0x8245F230, kStActive = 0x8218C5D8, kStLeaving = 0x8218C4C8;
// botones como los lee el menu (+14 pulsado, +18|+114 con repeticion). Disposicion de PS2: L2 0x100,
// R2 0x200, L1 0x400, R1 0x800; cruceta arriba 0x10, derecha 0x20, abajo 0x40, izquierda 0x80.
constexpr uint16_t kBackBtn = 0x0001, kUp = 0x0010, kRight = 0x0020, kDown = 0x0040, kLeft = 0x0080, kL2 = 0x0100,
                   kR2 = 0x0200, kL1 = 0x0400, kR1 = 0x0800, kA = 0x4000, kB = 0x1000, kCircle = 0x2000;
constexpr uint16_t kPrevPage = kL1 | kL2 | kLeft, kNextPage = kR1 | kR2 | kRight;
constexpr int kSeMove = 2, kSeOk = 3, kSeBack = 5;               // los del propio menu (sub_820922A0)
// texto de la cinta "Menu principal" (sprite +80, lo dibuja sub_8218B3F0 con la cinta +76; medido
// ocultando cada sprite del menu, 2026-10-15): fuera en la pantalla HUB, la cinta se queda
constexpr uint32_t kTitleSlot = 80;
// IDs jugables de serie (los de tagteam_panel.cpp) para el combate rapido
constexpr int kFighters[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18,
                             19, 20, 21, 27, 28, 29, 30, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43};

const char* const kKeys[kEntries] = {"tagteam", "online",      "rapido",   "torre",      "supervivencia", "kumite",
                                     "ranking", "laboratorio", "escenarios", "pasarela", "minijuegos",    "satan"};
struct Mode {
  std::atomic<void (*)()> open{nullptr};
  std::atomic<bool (*)()> busy{nullptr};
  std::atomic<bool (*)()> ready{nullptr};          // RegisterGate: false = en gris
  std::atomic<const char* (*)()> why{nullptr};     // ... y lo que dice el bocadillo
};
Mode g_modes[kEntries];

std::mutex g_mu;
View g_view;
std::atomic<int64_t> g_seen_ms{0};

int64_t NowMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

// Estado del HUB: solo el hilo del juego.
struct State {
  bool focus = false;    // cursor en la fila HUB
  bool open = false;     // pantalla HUB
  bool leaving = false;  // el menu se va hacia un modo elegido en el HUB (sin sus etiquetas)
  int leave_frames = 0;
  bool back = false;     // al volver al menu principal, abrir el HUB
  bool armed = false;    // ... y ya se salio de el (o la pantalla del modo se abrio encima)
  int cursor = 0;
  int last_state = -1;
  uint32_t obj = 0;      // menu que se esta dibujando
  uint32_t bubble = 0;   // tarea del bocadillo del menu (+144)
  bool panel = false;    // dialogo pedido
  Fade fade = Fade::kNone;
  bool to_hub = false;
  int64_t t0 = 0;
  uint32_t task = 0;     // tarea del menu durante el fundido
  bool test = false;     // DBZ3_TEST_HUB
} g;

bool Enabled() { return REXCVAR_GET(dbz3_hub_enabled) && !dbz3::g_guest_eu; }
// el bocadillo del menu dice lo del HUB (sin el texto del juego)
bool OwnsBubble() { return g.focus || g.open || g.leaving || (g.back && g.armed) || g.fade != Fade::kNone; }

bool OtherScreen() {
  using namespace dbz3::ingame;
  if (TagTeamWanted() || TagTeamGate() || TagTeamMode() || pilot::Busy() || OnlinePanel::IsOpen() || Online()) {
    return true;
  }
  for (const Mode& m : g_modes) {
    if (bool (*busy)() = m.busy.load(); busy && busy()) return true;
  }
  return false;
}

float LoadF32(uint8_t* base, uint32_t addr) {
  const uint32_t u = REX_LOAD_U32(addr);
  float f;
  std::memcpy(&f, &u, sizeof(f));
  return f;
}
void StoreF32(uint8_t* base, uint32_t addr, float f) {
  uint32_t u;
  std::memcpy(&u, &f, sizeof(u));
  REX_STORE_U32(addr, u);
}

// Llamada a una funcion del juego con r3 (y r4) sin ensuciar los registros del llamador.
template <typename F>
void Call(PPCContext& ctx, uint8_t* base, F fn, uint32_t r3, uint32_t r4 = 0) {
  PPCContext c = ctx;
  c.r3.u64 = r3;
  c.r4.u64 = r4;
  fn(c, base);
}

// Sonido del menu, como lo hace sub_8218C5D8 (volumen de efectos y, si suena, el efecto).
void PlaySe(PPCContext& ctx, uint8_t* base, int id) {
  const float volume = LoadF32(base, kSeVolume);
  PPCContext c = ctx;
  c.f1.f64 = volume;
  c.r3.u64 = 0;
  c.r4.u64 = 0;
  sub_82092570(c, base);
  if (volume > LoadF32(base, kSeFloor)) {
    c = ctx;
    c.r3.u64 = 0;
    c.r4.u64 = 0;
    c.r5.u64 = uint64_t(id);
    c.r6.u64 = 0;
    sub_820922A0(c, base);
  }
}

// Texto del bocadillo del menu (-1 = sin texto: el juego lo funde y queda el bocadillo vacio).
void Bubble(uint8_t* base, int id) {
  if (g.bubble && int32_t(REX_LOAD_U32(g.bubble + 60)) != id) REX_STORE_U32(g.bubble + 60, uint32_t(id));
}

// Cursor del juego a una fila nativa, como al moverse en el menu: sonido, ayuda y voz de la fila.
void ToNativeRow(PPCContext& ctx, uint8_t* base, uint32_t obj, int row) {
  REX_STORE_U8(obj + 150, uint8_t(row));
  PlaySe(ctx, base, kSeMove);
  Bubble(base, int16_t(REX_LOAD_U16(kHelp + 2 * row)));
  Call(ctx, base, sub_8218B8E0, obj);
}

// ---- Fundido entre el menu y la pantalla HUB ----
// Sale el menu con su propia animacion (la de su A al ir al HUB, la de su B al volver) y queda en su
// estado "saliendo" (sub_8218C4C8: solo dibuja), sin pedir pantalla. false = no se puede (se abre en seco).
bool StartFade(PPCContext& ctx, uint8_t* base, uint32_t obj, bool to_hub) {
  const uint32_t mgr = REX_LOAD_U32(kMenuMgr);
  const uint32_t task = mgr ? REX_LOAD_U32(mgr + 16) : 0;
  if (!task || REX_LOAD_U32(task + 48) != obj || REX_LOAD_U32(task + 28) != kStActive) {
    REXLOG_WARN("dbz3 hub: sin fundido (tarea del menu {:08X})", task);
    return false;
  }
  if (to_hub) {
    Call(ctx, base, sub_8218C588, obj);   // como su A: piezas de salida, +148 = 5, +156 = 0
  } else {                                // como su B (sub_8218C520) sin pedir el titulo
    Call(ctx, base, sub_8218C3A8, obj);
    Call(ctx, base, sub_8218C0C8, obj);
    Call(ctx, base, sub_8218BF38, obj);
    REX_STORE_U16(obj + 148, 4);
    StoreF32(base, obj + 156, 0.0f);
  }
  Call(ctx, base, sub_82081A48, REX_LOAD_U32(obj + 164));   // calla la voz de la fila
  REX_STORE_U32(task + 28, kStLeaving);
  g.task = task;
  g.fade = Fade::kOut;
  g.to_hub = to_hub;
  g.t0 = NowMs();
  return true;
}

// En negro: las piezas que entran (cinta, titulo, marco, etiquetas, franja) otra vez desde el principio,
// como sub_8218C4D0 al nacer el menu (el cielo y el reparto no se mueven). Antes se sueltan las que hay
// (sub_8218C190 no las suelta; sub_8218BE70 y sub_8218C000 si). Al principio del dibujo: nada pintado.
void ReplayEntry(PPCContext& ctx, uint8_t* base, uint32_t obj) {
  auto drop = [&](uint32_t off) {
    if (const uint32_t inst = REX_LOAD_U32(obj + off)) {
      Call(ctx, base, sub_820ACCF0, inst, 1);
      REX_STORE_U32(obj + off, 0);
    }
  };
  drop(84);
  drop(88);
  for (uint32_t i = 0; i < 7; ++i) drop(92 + 4 * i);
  drop(120);
  drop(124);
  drop(136);
  Call(ctx, base, sub_8218BE70, obj);
  Call(ctx, base, sub_8218C000, obj);
  Call(ctx, base, sub_8218C190, obj);
  REX_STORE_U16(obj + 148, 2);
  StoreF32(base, obj + 156, 0.0f);
}

void FadeStep(PPCContext& ctx, uint8_t* base, uint32_t obj) {
  const int64_t now = NowMs();
  const bool same = g.task && REX_LOAD_U32(g.task + 48) == obj;
  if (!same) {   // el menu ya no es el mismo: se deja estar
    g.fade = Fade::kNone;
    return;
  }
  if (g.fade == Fade::kOut && now - g.t0 >= kFadeOutMs + kFadeHoldMs) {
    ReplayEntry(ctx, base, obj);
    g.open = g.to_hub;
    g.focus = !g.to_hub;
    g.fade = Fade::kIn;
    g.t0 = now;
    REXLOG_INFO("dbz3 hub: {} (fundido)", g.to_hub ? "entra la pantalla HUB" : "vuelve el menu principal");
  } else if (g.fade == Fade::kIn && now - g.t0 >= kEntryMs) {
    REX_STORE_U16(obj + 148, 3);
    REX_STORE_U8(obj + 168, 0);
    REX_STORE_U32(g.task + 28, kStActive);
    g.fade = Fade::kNone;
  }
}

// escenarios normales (los registros 11-16 son variantes; stage_ext.h)
int Stage(std::mt19937& rng) { return std::uniform_int_distribution<int>(0, 10)(rng); }

// A en una entrada del HUB. true = deja una pulsacion para el menu del juego (no borrar el mando).
bool Launch(PPCContext& ctx, uint8_t* base, uint32_t obj, uint32_t pad) {
  const int e = g.cursor;
  if (!EntryReady(e)) return false;   // por llegar (otro frente): el bocadillo ya lo dice
  auto away = [](bool same_menu) {   // el HUB vuelve cuando el modo acabe
    g.open = false;
    g.back = true;
    g.armed = same_menu;
    g.leaving = !same_menu;
    g.leave_frames = 0;
  };
  switch (e) {
    case kTagTeam:   // su pantalla, encima del menu principal (B en ella vuelve aqui)
      PlaySe(ctx, base, kSeOk);
      away(true);
      dbz3::ingame::TagTeamOpen();
      break;
    case kOnline:    // el Duelo del juego, que lleva la fila ONLINE: se confirma su fila como haria el jugador
      dbz3::ingame::OnlineOpen();   // y al llegar al Duelo se abre ya el panel ONLINE
      away(false);
      REX_STORE_U8(obj + 150, 1);
      REX_STORE_U16(pad + 14, kA);
      REX_STORE_U16(pad + 18, 0);
      REX_STORE_U16(pad + 114, 0);
      return true;
    case kQuick: {   // 1P VS COM al azar por el conector nativo (hub.cpp)
      static std::mt19937 rng{uint32_t(NowMs())};
      std::uniform_int_distribution<size_t> pick(0, std::size(kFighters) - 1);
      const int p1 = kFighters[pick(rng)];
      int p2 = kFighters[pick(rng)];
      while (p2 == p1) p2 = kFighters[pick(rng)];
      const int stage = Stage(rng);
      if (RequestBattle(p1, p2, stage, true)) {
        PlaySe(ctx, base, kSeOk);
        away(false);
        REXLOG_INFO("dbz3 hub: combate rapido {} contra {} (escenario {})", p1, p2, stage);
      }
      break;
    }
    default: {
      const Mode& m = g_modes[e];
      PlaySe(ctx, base, kSeOk);
      away(m.busy.load() != nullptr);
      m.open.load()();
      REXLOG_INFO("dbz3 hub: modo {}", kKeys[e]);
      break;
    }
  }
  return false;
}

// Mueve el cursor del HUB: paso (+1/-1, con vuelta al final) o pagina (+1/-1, misma fila de la otra).
void Move(int step, int page_step) {
  int vis[kEntries];
  const int n = VisibleEntries(vis);
  if (n <= 0) return;
  int pos = 0;
  while (pos < n && vis[pos] != g.cursor) ++pos;
  if (pos == n) pos = 0;
  if (step) pos = (pos + step + n) % n;
  if (page_step) {
    const int pages = (n + kPerPage - 1) / kPerPage;
    const int page = (pos / kPerPage + page_step + pages) % pages;
    pos = std::min(page * kPerPage + pos % kPerPage, n - 1);
  }
  g.cursor = vis[pos];
}

// Mando del menu con la fila o la pantalla HUB: true = el HUB se lo queda (el menu no lo ve).
bool Input(PPCContext& ctx, uint8_t* base, uint32_t obj, uint32_t pad) {
  const uint16_t pressed = REX_LOAD_U16(pad + 14), rep = REX_LOAD_U16(pad + 18) | REX_LOAD_U16(pad + 114);
  const int rows = REX_LOAD_U8(obj + 151), row = REX_LOAD_U8(obj + 150);
  if (g.test && (pressed || rep)) REXLOG_INFO("dbz3 hub: prueba: botones {:04X} (repeticion {:04X})", pressed, rep);
  if (g.open) {
    if (pressed & (kB | kCircle)) {   // atras: el del menu (0x1000) y B del mando (0x2000)
      PlaySe(ctx, base, kSeBack);
      g.back = false;
      if (!StartFade(ctx, base, obj, false)) {
        g.open = false;
        g.focus = true;
      }
    } else if (pressed & kA) {
      return !Launch(ctx, base, obj, pad);   // Online: la A queda para el menu del juego
    } else if (rep & (kUp | kDown)) {        // con la repeticion del juego (mantener)
      Move((rep & kDown) ? 1 : -1, 0);
      PlaySe(ctx, base, kSeMove);
    } else if (pressed & (kPrevPage | kNextPage)) {
      int vis[kEntries];
      if (VisibleEntries(vis) > kPerPage) {
        Move(0, (pressed & kNextPage) ? 1 : -1);
        PlaySe(ctx, base, kSeMove);
      }
    }
    return true;
  }
  if (g.focus) {
    if (pressed & kB) {   // como en el resto del menu: el juego vuelve al titulo
      g.focus = false;
      return false;
    }
    if (pressed & kA) {
      PlaySe(ctx, base, kSeOk);
      if (!StartFade(ctx, base, obj, true)) g.open = true;
    } else if (rep & kUp) {
      g.focus = false;
      ToNativeRow(ctx, base, obj, rows - 1);
    } else if (rep & kDown) {
      g.focus = false;
      ToNativeRow(ctx, base, obj, 0);
    }
    return true;   // tambien BACK (la ayuda del juego no habla del HUB)
  }
  // la fila HUB va despues de la ultima nativa: abajo desde la ultima o arriba desde la primera
  if (((rep & kDown) && row == rows - 1) || ((rep & kUp) && row == 0)) {
    g.focus = true;
    REX_STORE_U8(obj + 150, 0);
    PlaySe(ctx, base, kSeMove);
    return true;
  }
  return false;
}

// Pruebas (pruebas.py): DBZ3_TEST_HUB=1 registra entradas de mentira para ver las paginas y escribe los
// botones en el log; DBZ3_TEST_HUB_ARENA=1 activa Dragon Arena (7 filas) para el siguiente menu
// principal (la copia de pruebas guarda la partida asi: no usarlo con la partida real).
void TestHooks(uint8_t* base) {
  static bool done = false;
  if (done) return;
  done = true;
  if (std::getenv("DBZ3_TEST_HUB")) {
    g.test = true;
    for (const char* k : {"escenarios", "pasarela", "minijuegos", "satan"}) {
      RegisterMode(k, [] { REXLOG_INFO("dbz3 hub: prueba: entrada de mentira"); }, [] { return false; });
    }
    REXLOG_INFO("dbz3 hub: prueba: 4 entradas de mentira (escenarios, pasarela, minijuegos, satan)");
  }
  if (std::getenv("DBZ3_TEST_HUB_ARENA") && REX_LOAD_U8(kArenaFlag) == 0) {
    REX_STORE_U8(kArenaFlag, 1);
    REXLOG_INFO("dbz3 hub: prueba: Dragon Arena activado (vale desde el siguiente menu principal)");
  }
}

}  // namespace

const char* EntryKey(int e) { return e >= 0 && e < kEntries ? kKeys[e] : ""; }

bool RegisterMode(const char* key, void (*open)(), bool (*busy)()) {
  for (int e = kTower; e < kEntries; ++e) {
    if (std::strcmp(key, kKeys[e]) == 0 && open) {
      g_modes[e].busy = busy;
      g_modes[e].open = open;
      return true;
    }
  }
  return false;
}

bool RegisterGate(const char* key, bool (*ready)(), const char* (*why)()) {
  for (int e = kTower; e < kEntries; ++e) {
    if (std::strcmp(key, kKeys[e]) == 0) {
      g_modes[e].why = why;
      g_modes[e].ready = ready;
      return true;
    }
  }
  return false;
}

const char* EntryWhy(int e) {
  if (e <= kQuick || e >= kEntries) return nullptr;
  const char* (*why)() = g_modes[e].why.load();
  return why ? why() : nullptr;
}

bool EntryReady(int e) {
  switch (e) {
    case kTagTeam: return REXCVAR_GET(dbz3_tag_team_enabled);
    case kOnline: return REXCVAR_GET(dbz3_online_enabled);
    case kQuick: return true;
    default: {
      if (e <= kQuick || e >= kEntries || g_modes[e].open.load() == nullptr) return false;
      bool (*ready)() = g_modes[e].ready.load();
      return !ready || ready();
    }
  }
}

bool EntryVisible(int e) {
  return e >= 0 && e < kEntries && (e < kAlwaysShown || g_modes[e].open.load() != nullptr);
}

int VisibleEntries(int* out) {
  int n = 0;
  for (int e = 0; e < kEntries; ++e) {
    if (EntryVisible(e)) out[n++] = e;
  }
  return n;
}

bool RowActive() { return Enabled(); }

int64_t ClockMs() { return NowMs(); }

View Snapshot() {
  std::lock_guard<std::mutex> lock(g_mu);
  View v = g_view;
  v.menu = NowMs() - g_seen_ms.load() < 200;
  return v;
}

}  // namespace dbz3::hub

namespace dbz3::hub {
namespace {
// Un fotograma del menu, al principio de su dibujo (nada pintado aun) y antes de que su estado lea el
// mando (sub_8218BBC0 dibuja y despues el estado activo lee el mando).
void Frame(PPCContext& ctx, uint8_t* base, uint32_t obj) {
  if (!g.panel) {
    g.panel = true;
    dbz3::launcher::DeferUi(&CreatePanel);
  }
  TestHooks(base);
  if (obj != g.obj) {   // otro menu (nacio de nuevo): ningun fundido a medias
    g.obj = obj;
    g.fade = Fade::kNone;
  }
  g.bubble = REX_LOAD_U32(obj + 144);
  const int state = REX_LOAD_U16(obj + 148);
  if (state != g.last_state) {
    REXLOG_INFO("dbz3 hub: menu principal estado {} (filas {}, cursor {})", state, REX_LOAD_U8(obj + 151),
                REX_LOAD_U8(obj + 150));
    if (state == 2 && g.fade == Fade::kNone) g.focus = g.open = g.leaving = false;   // menu nuevo: nada elegido
    g.last_state = state;
  }
  if (g.fade != Fade::kNone) FadeStep(ctx, base, obj);
  const bool other = OtherScreen();
  if (g.fade == Fade::kNone) {
    if (g.back && !g.armed && state != 3) g.armed = true;   // ya se fue del menu: al volver, el HUB
    if (state == 3 && !other && g.back && g.armed) {
      g.back = g.armed = g.leaving = false;
      g.open = g.focus = true;
    }
    if (g.leaving && state == 3 && ++g.leave_frames > 180) {   // el modo no arranco: se ve el menu
      g.leaving = false;
      g.focus = true;
    }
    const uint32_t pad = kPad + REX_LOAD_U32(kPadIdx) * 184;
    if (state == 3 && !other && Input(ctx, base, obj, pad)) {
      REX_STORE_U16(pad + 14, 0);
      REX_STORE_U16(pad + 18, 0);
      REX_STORE_U16(pad + 114, 0);
    }
  }
  if (OwnsBubble()) Bubble(base, -1);
  {
    std::lock_guard<std::mutex> lock(g_mu);
    g_view = View{true,  state, other,    g.focus, g.open, g.leaving || (g.back && g.armed), g.cursor, g.fade,
                  g.to_hub, g.t0, REX_LOAD_U8(obj + 151)};
  }
  g_seen_ms = NowMs();
}

bool Screen() { return g.open || g.leaving || (g.back && g.armed); }

}  // namespace
}  // namespace dbz3::hub

// Fondo del menu principal: lo primero que dibuja sub_8218BBC0. Aqui decide el HUB (mando, fundido).
REX_HOOK_RAW(sub_8218B2F0) {
  using namespace dbz3::hub;
  if (Enabled() && ctx.r3.u32) Frame(ctx, base, ctx.r3.u32);
  __imp__sub_8218B2F0(ctx, base);
}

// Filas del menu principal: el juego dibuja sin sus etiquetas (+151 a 0) en la pantalla HUB y sin franja
// (+120) ni fila elegida con el cursor en el HUB.
REX_HOOK_RAW(sub_8218B550) {
  using namespace dbz3::hub;
  const uint32_t obj = ctx.r3.u32;
  if (!Enabled() || !obj) {
    __imp__sub_8218B550(ctx, base);
    return;
  }
  const uint8_t rows = REX_LOAD_U8(obj + 151);
  const uint32_t stripe = REX_LOAD_U32(obj + 120);
  const uint8_t row = REX_LOAD_U8(obj + 150);
  if (Screen() || g.focus) {   // ninguna fila nativa elegida: ni franja ni etiqueta a pleno brillo
    REX_STORE_U32(obj + 120, 0);
    REX_STORE_U8(obj + 150, 0xFF);
  }
  if (Screen()) REX_STORE_U8(obj + 151, 0);
  __imp__sub_8218B550(ctx, base);
  REX_STORE_U32(obj + 120, stripe);
  REX_STORE_U8(obj + 151, rows);
  REX_STORE_U8(obj + 150, row);
}
// Cinta del titulo: en la pantalla HUB sin su texto "Menu principal" (hub_panel.cpp pone el logo encima).
REX_HOOK_RAW(sub_8218B3F0) {
  using namespace dbz3::hub;
  const uint32_t obj = ctx.r3.u32;
  if (!Enabled() || !obj || !Screen()) {
    __imp__sub_8218B3F0(ctx, base);
    return;
  }
  const uint32_t text = REX_LOAD_U32(obj + kTitleSlot);
  REX_STORE_U32(obj + kTitleSlot, 0);
  __imp__sub_8218B3F0(ctx, base);
  REX_STORE_U32(obj + kTitleSlot, text);
}

// Espera del menu sin mando: al callar la voz de bienvenida pone la ayuda y la voz de su fila. Con la
// fila o la pantalla HUB, ni una ni otra.
REX_HOOK_RAW(sub_8218B998) {
  using namespace dbz3::hub;
  if (Enabled() && (g.focus || g.open)) return;
  __imp__sub_8218B998(ctx, base);
}

// Texto del bocadillo de un menu: el del menu principal es del HUB mientras lo usa (el juego pone el
// saludo al terminar de entrar y la ayuda de su fila al moverse).
REX_HOOK_RAW(sub_8217E800) {
  using namespace dbz3::hub;
  if (Enabled() && ctx.r3.u32 == g.bubble && ctx.r4.s16 != -1 && OwnsBubble()) return;
  __imp__sub_8217E800(ctx, base);
}
