// dbz3 - Pantalla TAG TEAM dentro del juego (X en el menu principal). Se parece al menu de
// Duelo: cinta con el titulo, marco con filas, anfitriones y bocadillo que explica la fila.
//   - 1P VS COM / 1P VS 2P / COM VS COM (+ formato 2 VS 1, 2 VS 2, 3 VS 3): lleva al juego
//     (piloto de menus) a esa opcion del Duelo; los miembros se eligen en la rueda nativa.
// El combate lo lleva src/tagteam.cpp. El dibujo con aspecto del juego, game_style.cpp.
#include "tagteam_panel.h"
#include "../form_selection_state.h"

#include <algorithm>
#include <cmath>
#include <array>
#include <cctype>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#include <rex/input/input_system.h>
#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/runtime.h>

#include "../guest_region.h"
#include "../launcher/i18n.h"
#include "../launcher/launcher_state.h"
#include "../launcher/ui_kit.h"
#include "../roster_ext.h"
#include "../tagteam.h"
#include "../net/netplay.h"
#include "game_style.h"
#include "menu_pilot.h"
#include "online_panel.h"


bool dbz3_select_live();   // select_ext.cpp
bool dbz3_select_to_stage();
int dbz3_select_preview_max_form(int side);   // roster_ext.cpp
int64_t dbz3_select_browse_ms(int k);
int64_t dbz3_select_now_ms();
int dbz3_select_capsule_option(int k);

namespace dbz3 {
bool GameDimmed();
}

REXCVAR_DEFINE_BOOL(dbz3_form_select_enabled, true, "DBZ3/Mods", "LT/RT starting-form selector at character select (beta)");

namespace dbz3::ingame {
bool Online();
bool FormSelectOn();
namespace {

namespace T = dbz3::i18n;
namespace S = dbz3::ingame::style;

constexpr int kMainScreen = 5, kDuelScreen = 14, kDuelRow = 1;
constexpr uint16_t kUp = 0x0001, kDown = 0x0002, kA = 0x1000, kB = 0x2000, kX = 0x4000, kY = 0x8000;

std::atomic<bool> g_wanted{false};
std::atomic<uint16_t> g_pad{0};
std::atomic<bool> g_pad_pending{false};

// Modo Tag Team en la rueda del juego (desde la pantalla TAG TEAM hasta volver al menu principal).
// Cada lado elige 2-5 miembros en la rueda nativa: A anade (el juego confirma y el piloto vuelve
// a la rueda con Y), Y quita el ultimo, START cierra el equipo (el piloto confirma la casilla y el
// juego sigue; al empezar, tagteam_slots.cpp cambia ese personaje por el equipo).
std::atomic<bool> g_team_mode{false};
std::atomic<int> g_mode{0};          // 0 1P VS COM, 1 1P VS 2P, 2 COM VS COM
std::atomic<int> g_format{2};        // 0 2 VS 1, 1 1 VS 2, 2 2 VS 2, 3 3 VS 3
std::atomic<bool> g_after_battle{false};
std::mutex g_mu;
std::array<std::vector<int>, 2> g_picks;
std::array<bool, 2> g_done{};
std::array<bool, 2> g_await{};        // miembro confirmado esperando su menu de capsulas
std::array<bool, 2> g_converted{};    // su A se convirtio en Y (el "atras" que llega es la eleccion)
std::array<bool, 2> g_eat_a{};        // A comida hasta soltarla (ya convertida)
std::array<int, 2> g_choice{};        // opcion de capsulas del ultimo miembro: 0 Normal, 1 Custom
std::array<uint16_t, 2> g_prev{};
std::array<std::atomic<int>, 2> g_start_form{};     // forma inicial elegida con LT/RT por lado
std::array<std::atomic<bool>, 2> g_form_dirty{};    // la vista previa de la rueda se rehace
std::atomic<bool> g_form_used{false};              // ya empezo un combate con la forma elegida
std::array<std::array<bool, 2>, 2> g_trig_prev{};  // LT/RT pulsados el tick anterior por mando
constexpr const char* kFormats[] = {"2 VS 1", "1 VS 2", "2 VS 2", "3 VS 3"};
constexpr int kNumFormats = 4;
// Ultimo aviso de la eleccion (lado, que paso y cuando), para el letrero sobre la tira.
enum class Event { kNone, kAdded, kRemoved, kRepeated };
struct Notice {
  int side = 0;
  Event what = Event::kNone;
  double at = -10;
};
Notice g_notice;
double Now() {
  return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Miembros que elige el lado (1 = un personaje normal, sin tira).
int Need(int side) {
  switch (g_format.load()) {
    case 0: return side == 0 ? 2 : 1;
    case 1: return side == 0 ? 1 : 2;
    case 2: return 2;
    default: return 3;
  }
}

struct Fighter {
  int id;
  const char* es;
  const char* en;
};
constexpr Fighter kFighters[] = {
    {0, "Goku", "Goku"}, {1, "Goku (nino)", "Kid Goku"}, {2, "Gohan (nino)", "Kid Gohan"},
    {3, "Gohan (adolescente)", "Teen Gohan"}, {4, "Gohan", "Gohan"}, {5, "Gran Saiyaman", "Great Saiyaman"},
    {6, "Goten", "Goten"}, {7, "Vegeta", "Vegeta"}, {8, "Trunks", "Trunks"}, {9, "Trunks (nino)", "Kid Trunks"},
    {10, "Krilin", "Krillin"}, {11, "Piccolo", "Piccolo"}, {12, "Ten Shin Han", "Tien"}, {13, "Yamcha", "Yamcha"},
    {14, "Mr. Satan", "Hercule"}, {15, "Videl", "Videl"}, {16, "Kaio-shin", "Supreme Kai"}, {17, "Uub", "Uub"},
    {18, "Raditz", "Raditz"}, {19, "Nappa", "Nappa"}, {20, "Ginyu", "Ginyu"}, {21, "Recoome", "Recoome"},
    {27, "Freezer", "Frieza"}, {28, "Androide 16", "Android 16"}, {29, "Androide 17", "Android 17"},
    {30, "Androide 18", "Android 18"}, {32, "Dr. Gero", "Dr. Gero"}, {33, "Cell", "Cell"},
    {34, "Majin Buu", "Majin Buu"}, {35, "Super Buu", "Super Buu"}, {36, "Kid Buu", "Kid Buu"},
    {37, "Dabura", "Dabura"}, {38, "Cooler", "Cooler"}, {39, "Bardock", "Bardock"}, {40, "Broly", "Broly"},
    {41, "Omega Shenron", "Omega Shenron"}, {42, "Saibaman", "Saibaman"}, {43, "Cell Jr.", "Cell Jr."}};

std::string Name(int id) {
  for (const auto& f : kFighters) {
    if (f.id == id) return T::T(f.es, f.en);
  }
  std::string n = dbz3::roster::InternalName(uint32_t(std::max(id, 0)));
  for (size_t i = 1; i < n.size(); ++i) n[i] = char(std::tolower(uint8_t(n[i])));
  if (!n.empty()) return n;
  char buf[16];
  std::snprintf(buf, sizeof(buf), "#%d", id);
  return buf;
}

// Cara del marcador del personaje (ingame_ui/caras/<ID>.png, tools/ingame_ui.py). Hilo de dibujo.
ImTextureID FaceOf(int id) { return S::Img(("caras/" + std::to_string(id)).c_str()).tex; }

rex::input::InputSystem* Input() {
  auto* rt = rex::Runtime::instance();
  return rt ? static_cast<rex::input::InputSystem*>(rt->input_system()) : nullptr;
}

bool PilotReady() { return true; }   // el piloto espera por el byte de pantalla (US y EU, GuestAddr)

// Del menu principal a la fila `row` del Duelo y a su seleccion de personajes. Las filas se
// cuentan (online_panel.cpp sigue el cursor del menu principal; el de Duelo nace en la fila 0 y
// 1P VS 2P y COM VS 2P cuentan aunque salgan en gris sin mando 2).
void PilotToDuel(int row) {
  using namespace dbz3::ingame::pilot;
  std::vector<Step> steps;
  // sin dar la vuelta: el menu tiene menos filas en una partida sin todo desbloqueado (7 era la de
  // la partida al 100%; un jugador acababa en el Torneo o la Tienda, 2026-10-12). Duelo es la fila 1:
  // desde la fila real del cursor se sube (o se baja una desde la primera)
  // con el objeto del menu a mano (US y EU) el cursor se pone en Duelo sin recorrer filas
  if (const uint32_t obj = MainMenuObj()) {
    steps.push_back(Poke(obj + 150, kDuelRow));
  } else {
    const int from = MainMenuRow();
    for (int n = from - kDuelRow; n > 0; --n) steps.push_back(Press(kUp));
    for (int n = kDuelRow - from; n > 0; --n) steps.push_back(Press(kDown));
  }
  steps.push_back(Press(kA));
  steps.push_back(WaitScreen(kDuelScreen));
  // desde la fila que recuerda el juego, por el camino corto (el menu da la vuelta; las grises cuentan)
  const int down = (row - DuelMenuRow() + 4) % 4;
  for (int n = down <= 2 ? down : 0; n > 0; --n) steps.push_back(Press(kDown));
  for (int n = down > 2 ? 4 - down : 0; n > 0; --n) steps.push_back(Press(kUp));
  steps.push_back(PressToLeave(kA, kDuelScreen));
  SetMainMenuRow(kDuelRow);
  SetDuelMenuRow(row);
  Run(std::move(steps));
}

// Lado que esta eligiendo con el mando `user`: en 1P VS 2P cada uno el suyo; si no, el
// jugador 1 elige los dos equipos (primero el suyo), como en Tenkaichi 3.
int SideOf(int user) {
  if (g_mode.load() == 1) return user;
  return g_done[0] ? 1 : 0;
}

int PilotUser(int side) { return g_mode.load() == 1 ? side : 0; }

// Pasarela (2026-10-12, "directo y nativo"): mientras el piloto recorre los menus del juego la
// pantalla queda en negro, como los fundidos del juego. TAG TEAM -> negro -> rueda; y al salir de
// la rueda (Y sin miembros) -> negro -> TAG TEAM. El menu principal y el de Duelo no se ven.
enum class Gate { kNone, kToWheel, kToPanel };
std::atomic<Gate> g_gate{Gate::kNone};
double g_gate_at = 0, g_gate_out = 0;   // hilo de dibujo
double g_live_at = -10, g_duel_at = 0;  // ultima rueda viva; desde cuando se ve el Duelo en Tag Team

void StartGate(Gate g) {
  g_gate_at = Now();
  g_gate_out = 0;
  g_gate = g;
}

}  // namespace

bool TagTeamGate() { return g_gate.load() != Gate::kNone; }

std::atomic<double> g_opened_at{0};
// el A que abre la pantalla (desde el menu principal) seguia pulsado y elegia 1P VS COM solo
void TagTeamOpen() {
  g_opened_at = Now();
  g_wanted = true;
}
bool TagTeamWanted() { return g_wanted.load(); }
bool TagTeamMode() { return g_team_mode.load(); }
bool FormSelectorEnabled() { return FormSelectOn(); }

// Personaje confirmado en la rueda por el lado `player`: mientras al equipo le falten miembros
// se anade a su tira y el piloto vuelve a la rueda (Y); el ultimo es la confirmacion del juego.
bool OnSelectPicked(int player, int id) {
  forms::Confirm(player, id, Online());
  if (!g_team_mode.load() || player < 0 || player > 1) return false;
  std::lock_guard<std::mutex> lock(g_mu);
  if (g_done[size_t(player)]) return false;
  auto& picks = g_picks[size_t(player)];
  if (std::find(picks.begin(), picks.end(), id) != picks.end()) {   // como Tenkaichi 3: sin repetir
    g_notice = {player, Event::kRepeated, Now()};
    REXLOG_INFO("dbz3 tag: lado {} ya tiene el id {} (no se repite)", player + 1, id);
    dbz3::ingame::pilot::Run({dbz3::ingame::pilot::PressUntilBrowse(kY, player, PilotUser(player))});
    return true;
  }
  picks.push_back(id);
  g_notice = {player, Event::kAdded, Now()};
  g_choice[size_t(player)] = 0;
  g_converted[size_t(player)] = false;
  g_await[size_t(player)] = false;   // el ultimo miembro no se espera: su menu sigue como el del juego
  REXLOG_INFO("dbz3 tag: lado {} miembro {}/{} = id {}", player + 1, picks.size(), Need(player), id);
  if (int(picks.size()) >= Need(player)) {
    g_done[size_t(player)] = true;
    dbz3::tag::SetDynamicTeam(player, picks);
    return false;
  }
  // como un personaje normal: el juego abre su menu de capsulas (Normal / Custom); A en una de las dos
  // se apunta y vuelve a la rueda (TeamPickFilter), a por el siguiente miembro
  g_await[size_t(player)] = true;
  return true;
}

// Menu de capsulas del juego tras confirmar (select_ext.cpp, sub_8217CB10): elegido o atras, con la
// lista Custom de la casilla. La opcion es la que tenia resaltada al pulsar A (g_choice).
void OnCapsuleChoice(int player, bool chosen, int option, const uint16_t* custom) {
  auto confirm_form = [&](int choice) {
    if (player < 0 || player > 1 || Online()) return;
    auto s = forms::Copy(player).snapshot;
    if (choice == 1 && custom) {
      s.capsules.clear();
      for (int c = 0; c < 7; ++c) if (custom[c] != 0xFFFF) s.capsules.push_back(custom[c]);
    } else { s = dbz3::roster::ReadForms(s.id, s.suit); }
    forms::Publish(player, s);
    forms::Confirm(player, s.id);
    g_form_dirty[size_t(player)] = true;
  };
  if (!g_team_mode.load() && chosen) confirm_form(option);
  if (!g_team_mode.load() || player < 0 || player > 1) return;
  std::lock_guard<std::mutex> lock(g_mu);
  auto& picks = g_picks[size_t(player)];
  const bool await = g_await[size_t(player)], converted = g_converted[size_t(player)];
  g_await[size_t(player)] = false;
  g_converted[size_t(player)] = false;
  if (!chosen && !converted) {   // atras de verdad en el menu de capsulas: ese miembro no entra
    if (await && !picks.empty()) {
      forms::ForgetMember(player, picks.back());
      picks.pop_back();
      g_notice = {player, Event::kRemoved, Now()};
    }
    return;
  }
  if (picks.empty()) return;
  option = g_choice[size_t(player)];
  confirm_form(option);
  if (option == 1) {   // Custom: las de la casilla, sin huecos (n + 7, como char96 +80)
    uint16_t caps[8] = {};
    for (int k = 0; k < 7; ++k) {
      if (custom[k] != 0xFFFF) caps[1 + caps[0]++] = custom[k];
    }
    dbz3::tag::SetMemberCapsules(player, picks.back(), caps);
  } else {
    dbz3::tag::SetMemberCapsules(player, picks.back(), nullptr);
  }
  REXLOG_INFO("dbz3 tag: lado {} id {}: capsulas {}", player + 1, picks.back(), option == 1 ? "Custom" : "Normal");
}

// Mando en la rueda con el modo Tag Team: Y quita el ultimo miembro elegido (sin miembros, Y es
// el "atras" del juego). En el menu de capsulas: A en Normal (0) o Custom (1) apunta la opcion y, si
// el miembro no es el ultimo, se convierte en Y: el juego vuelve a la rueda en vez de dar el lado por
// elegido y pasar al rival (2026-10-12: asi saltaba a elegir al COM).
uint16_t TeamPickFilter(uint16_t real, int user) {
  if (user < 0 || user > 1) return real;
  const uint16_t pressed = real & ~g_prev[size_t(user)];
  g_prev[size_t(user)] = real;
  if (!g_team_mode.load() || !dbz3_select_live() || dbz3::ingame::pilot::Busy()) return real;
  std::lock_guard<std::mutex> lock(g_mu);
  int menu = -1;   // lado cuyo menu de capsulas esta abierto (el de este mando en 1P VS 2P)
  for (int k = 0; k < 2; ++k) {
    if (dbz3_select_capsule_option(k) >= 0 && (g_mode.load() != 1 || k == user)) menu = k;
  }
  // A ya convertida: mientras siga pulsada, Y si el menu sigue abierto y nada si ya volvio a la rueda
  // (sin confirmar otra vez el mismo personaje)
  for (int k = 0; k < 2; ++k) {
    if (!g_eat_a[size_t(k)]) continue;
    if (!(real & kA)) {
      g_eat_a[size_t(k)] = false;
      continue;
    }
    return uint16_t((real & ~kA) | (menu == k ? kY : 0));
  }
  if (menu >= 0 && (pressed & kA)) {
    const int opt = dbz3_select_capsule_option(menu);
    if (opt == 0 || opt == 1) {
      g_choice[size_t(menu)] = opt;
      if (g_await[size_t(menu)]) {
        g_converted[size_t(menu)] = true;
        g_eat_a[size_t(menu)] = true;
        return uint16_t((real & ~kA) | kY);
      }
    }
  }
  const int side = SideOf(user);
  auto& picks = g_picks[size_t(side)];
  if (g_done[size_t(side)] || picks.empty()) return real;
  // solo con la rueda eligiendo: en el menu de capsulas Y es su "atras" (OnCapsuleChoice)
  if (dbz3_select_now_ms() - dbz3_select_browse_ms(side) > 250) return real;
  if (pressed & kY) {
    forms::ForgetMember(side, picks.back());
    picks.pop_back();
    g_notice = {side, Event::kRemoved, Now()};
  }
  return uint16_t(real & ~kY);
}

bool Online() { return dbz3::net::Session::Get().link() != dbz3::net::Link::kIdle; }

// Interruptor del launcher. Online se ignora: los dos PCs tienen que contar igual el mando.
bool FormSelectOn() { return REXCVAR_GET(dbz3_form_select_enabled) && !Online(); }

// Lado que maneja este mando en la rueda. Online: el suyo siempre (los dos PCs leen el mismo mando
// en el mismo tick: misma cuenta sin depender del reloj). Local: el que esta eligiendo (los dos a la
// vez = 1P VS 2P: el suyo).
int FormSide(int user) {
  if (Online()) return user;
  const int64_t now = dbz3_select_now_ms();
  const bool b0 = now - dbz3_select_browse_ms(0) < 250, b1 = now - dbz3_select_browse_ms(1) < 250;
  return b0 && b1 ? user : b0 ? 0 : b1 ? 1 : -1;
}

// Forma inicial con LT/RT en la rueda (tambien en Tag Team, para el primer miembro, y online).
// Sustituye FormPickPad; no lee ni escribe memoria del guest.
void FormPickPad(int user, uint8_t lt, uint8_t rt) {
  if (user < 0 || user > 1) return;
  auto& prev = g_trig_prev[size_t(user)];
  const bool l = lt > 64, r = rt > 64;
  const int delta = int(r && !prev[1]) - int(l && !prev[0]);
  prev = {l, r};
  if (!dbz3_select_live() || dbz3::ingame::pilot::Busy() || !FormSelectOn()) return;
  const int side = FormSide(user);
  if (side >= 0 && forms::Move(side, delta, Online())) g_form_dirty[size_t(side)] = true;
}

int ChosenForm(int side) {
  if (side < 0 || side > 1 || !FormSelectOn()) return 0;
  if (const char* env = std::getenv("DBZ3_START_FORM")) {
    const char* comma = std::strchr(env, ',');
    return std::atoi(side == 0 || !comma ? env : comma + 1);
  }
  return forms::Copy(side).requested;
}

bool TakeFormDirty(int side) { return side >= 0 && side < 2 && g_form_dirty[size_t(side)].exchange(false); }

int StartForm(int side) {
  if (side < 0 || side > 1) return 0;
  if (const char* env = std::getenv("DBZ3_START_FORM")) {   // pruebas: "2,1" = forma de cada lado
    const char* comma = std::strchr(env, ',');
    return std::atoi(side == 0 || !comma ? env : comma + 1);
  }
  if (!FormSelectOn()) return 0;
  g_form_used = true;   // se gasta en este combate (y en sus revanchas); la rueda siguiente empieza de 0
  return forms::Copy(side).requested;
}

void DrawFormPick(ImGuiIO& io, bool team) {
  if (Online()) return; // Private F2 selections never enter online presentation.
  const S::Area a = S::GameArea(io);
  auto* dl = ImGui::GetForegroundDrawList();
  for (int side = 0; side < 2; ++side) {
    if (dbz3_select_now_ms() - dbz3_select_browse_ms(side) > 300) continue;
    const auto state = forms::Copy(side); // copia coherente; UI nunca toca tablas
    if (!forms::CountValid(state.snapshot) || state.snapshot.count == 1) continue;
    const auto result = forms::Resolve(state.snapshot, state.requested);
    char text[160];
    if (state.requested == 0 && result.applied == 0) {
      std::snprintf(text, sizeof(text), "%s", T::T("Forma normal", "Normal form"));
    } else {
      std::snprintf(text, sizeof(text), "%s %d -> %d",
                    T::T("Forma inicial", "Starting form"), state.requested, result.applied);
    }
    const float cx = side == 0 ? 210.0f : 990.0f, y = team ? 480.0f : 292.0f;
    const auto pos = a.At(cx, y);
    dl->PushClipRect(a.At(cx - 180, y - 20), a.At(cx + 180, y + 135), true);
    const auto& fonts = dbz3::launcher::ui::GetFonts();
    ImFont* font = fonts.menu ? fonts.menu : ImGui::GetFont();
    const float measured = font->CalcTextSizeA(28 * a.s, 1.0e30f, 0, text).x;
    const float size = 28 * a.s * std::min(1.0f, 220 * a.s / std::max(1.0f, measured));
    S::MenuText(dl, pos, size, text, result.valid, false);
    S::Arrows(dl, a.At(cx - 120, y - 16), a.At(cx + 120, y + 16),
              24*a.s, float(ImGui::GetTime()));
    S::HelpLine(dl, a.At(cx, y + 38), 24*a.s,
                {{"LT", T::T("Anterior", "Previous")}, {"RT", T::T("Siguiente", "Next")}});
    const char* reason = nullptr;
    if (result.reason == forms::Reason::kCapsule) {
      std::snprintf(text, sizeof(text), "%s %d", T::T("Falta la capsula", "Missing capsule"), result.capsule);
      reason = text;
    } else if (result.reason == forms::Reason::kSpecial) {
      reason = T::T("Forma especial: no disponible al iniciar", "Special form: unavailable at start");
    } else if (result.reason == forms::Reason::kCapsuleData) {
      reason = T::T("Tabla de capsulas no disponible", "Capsule table unavailable");
    } else if (result.reason != forms::Reason::kOk) {
      reason = T::T("Modelo o traje no disponible: forma base", "Model or costume unavailable: base form");
    }
    if (reason) S::MenuText(dl, a.At(cx, y + 64), 18*a.s, reason, false, false);
    if (state.requested > 0 && state.requested < state.snapshot.count &&
        forms::Availability(state.snapshot, state.requested, false) == forms::Reason::kOk) {
      const int ki = state.snapshot.forms[size_t(state.requested)].required_ki_tenths;
      if (ki >= 0) {
        std::snprintf(text, sizeof(text), "%s %.1f", T::T("Ki para transformar:", "Ki to transform:"), double(ki)/10.0);
        S::MenuText(dl, a.At(cx, y + (reason ? 90 : 64)), 18*a.s, text, false, false);
      }
    }
    if (team) S::MenuText(dl, a.At(cx, y + 115), 17*a.s,
                          T::T("Primer luchador", "First fighter"), true, false);
    dl->PopClipRect();
  }
}

TagTeamPanel::TagTeamPanel(rex::ui::ImGuiDrawer* drawer)
    : ImGuiDialog(drawer) {}

TagTeamPanel::~TagTeamPanel() {
  forms::FlushPreferences();
  if (blocked_) {
    if (auto* in = Input()) in->RemoveUIInputBlocker();
  }
}

uint16_t TagTeamPanel::Pad() {
  if (!g_pad_pending.exchange(true)) {
    dbz3::launcher::DeferUi([]() {
      uint16_t b = 0;
      if (auto* in = Input()) {
        rex::input::X_INPUT_STATE st = {};
        if (in->GetStateForUI(0, &st) == 0) {
          b = static_cast<uint16_t>(st.gamepad.buttons);
          const int ly = int16_t(st.gamepad.thumb_ly);   // la palanca tambien mueve el cursor
          if (ly > 16000) b |= kUp;
          if (ly < -16000) b |= kDown;
        }
      }
      g_pad = b;
      g_pad_pending = false;
    });
  }
  const uint16_t b = g_pad.load();
  const uint16_t pressed = b & ~last_buttons_;
  last_buttons_ = b;
  return pressed;
}

void TagTeamPanel::DrawMenu(ImGuiIO& io, uint16_t pressed) {
  // el A que abre la pantalla (y cualquier boton aun pulsado) no cuenta hasta soltarlo todo: con solo
  // un plazo, la primera vez elegia 1P VS COM solo (el mando llega tarde)
  if (wait_release_) {
    if (g_pad.load() == 0 && Now() - g_opened_at.load() > 0.3) wait_release_ = false;
    pressed = 0;
  }
  const char* rows[] = {"1P VS COM", "1P VS 2P", "COM VS COM"};
  const char* help[] = {
      T::T("Tu equipo contra el del ordenador. Eliges los dos equipos en la rueda de personajes.",
           "Your team against the computer's. You pick both teams on the character wheel."),
      T::T("Dos jugadores, cada uno elige su equipo en la rueda.", "Two players, each picks a team on the wheel."),
      T::T("Mira un combate entre dos equipos que eliges tu.", "Watch two teams of your choice fight.")};
  constexpr int kRows = 3;
  constexpr uint16_t kLeft = 0x0004, kRight = 0x0008;
  if (pressed & kUp) row_ = (row_ + kRows - 1) % kRows;
  if (pressed & kDown) row_ = (row_ + 1) % kRows;
  if (pressed & kLeft) g_format = (g_format.load() + kNumFormats - 1) % kNumFormats;
  if (pressed & kRight) g_format = (g_format.load() + 1) % kNumFormats;
  if (pressed & (kB | kY)) g_wanted = false;
  // 1P VS 2P sale en gris en el Duelo sin mando 2: el piloto pulsaria A sin fin (2026-10-12)
  const bool no_pad2 = row_ == 1 && !Pad2Connected();
  if (no_pad2) pressed &= uint16_t(~kA);
  if (pressed & kA) {
    g_wanted = false;
    {
      std::lock_guard<std::mutex> lock(g_mu);
      for (auto& p : g_picks) p.clear();
      g_done = {};
      g_await = {};
    }
    dbz3::tag::ClearDynamicTeams();
    g_mode = row_;
    dbz3::tag::SetComSide(0, row_ == 2);   // 1P VS COM: el lado 2; COM VS COM: los dos
    dbz3::tag::SetComSide(1, row_ != 1);
    g_team_mode = true;
    g_after_battle = false;
    StartGate(Gate::kToWheel);
    if (PilotReady()) PilotToDuel(row_ == 0 ? 0 : row_ == 1 ? 1 : 3);   // Duelo: 1P VS COM, 1P VS 2P, COM VS 2P, COM VS COM
  }

  // Como el Duelo: fondo del juego, cinta con el titulo, marco con las filas del juego y los
  // anfitriones (dos Kaio del menu de Duelo, sacados del juego) que hablan por turnos en el bocadillo.
  const S::Area a = S::GameArea(io);
  auto* dl = ImGui::GetForegroundDrawList();
  const float t = float(ImGui::GetTime());
  // el anfitrion de la izquierda habla en las filas 1 y 3, la de la derecha en la 2 (el pico va hacia quien habla)
  const bool right = row_ == 1;
  S::Backdrop(dl, a, "fondo_duelo");
  S::Host(dl, a, "anfitrion_tagteam_a", -68, 262, false);   // donde esta en el Duelo (medido)
  S::Host(dl, a, "anfitrion_tagteam_b", 800, 540, true);
  S::Sides(dl, a);
  S::Banner(dl, a, "Tag Team", "titulo_tagteam");
  const ImVec2 f0 = a.At(330, 170), f1 = a.At(870, 170 + 82 * kRows + 20);
  S::Frame(dl, f0, f1, a.s);
  // las filas son las del Duelo (data_usi 2055, "filas_vs"): 1P VS COM, 1P VS 2P y COM VS COM
  const S::Pic native_rows = S::Img("filas_vs");
  constexpr ImVec4 kRowSrc[] = {ImVec4(52, 9, 338, 54), ImVec4(53, 69, 297, 114), ImVec4(3, 189, 339, 234)};
  for (int i = 0; i < kRows; ++i) {
    const ImVec2 r0(f0.x, f0.y + (10 + 82 * i) * a.s), r1(f1.x, r0.y + 80 * a.s);
    const ImVec2 c((r0.x + r1.x) * 0.5f, (r0.y + r1.y) * 0.5f);
    const bool disabled = i == 1 && !Pad2Connected();
    if (i == row_ && !disabled) S::Highlight(dl, r0, r1, t);
    if (native_rows) {
      const ImVec4 src = kRowSrc[i];
      const float h = 54 * a.s, w = (src.z - src.x) * h / (src.w - src.y);
      S::DrawPic(dl, native_rows, ImVec2(c.x - w * 0.5f, c.y - h * 0.5f), ImVec2(c.x + w * 0.5f, c.y + h * 0.5f), src,
                 disabled ? IM_COL32(70, 82, 120, 200) : i == row_ ? IM_COL32_WHITE : IM_COL32(105, 135, 150, 255));
    } else {
      S::MenuText(dl, c, 56 * a.s, rows[i], i == row_ && !disabled, disabled);
    }
  }
  // formato: izquierda/derecha, con las flechas del juego
  const ImVec2 g0 = a.At(430, 480), g1 = a.At(770, 548);
  S::Frame(dl, g0, g1, a.s);
  S::MenuText(dl, ImVec2((g0.x + g1.x) * 0.5f, (g0.y + g1.y) * 0.5f), 50 * a.s, kFormats[g_format.load()], true, false);
  S::Arrows(dl, ImVec2(g0.x - 18 * a.s, g0.y), ImVec2(g1.x + 18 * a.s, g1.y), 46 * a.s, t);
  std::string text = no_pad2 ? T::T("Conecta un segundo mando para jugar 1P VS 2P.",
                                     "Connect a second controller to play 1P VS 2P.")
                             : help[row_];
  text += '\n';
  text += g_format.load() == 3
              ? T::T("3 VS 3: SELECT cambia al segundo y SELECT + L al tercero.",
                     "3 VS 3: SELECT switches to the second, SELECT + L to the third.")
              : T::T("SELECT cambia al compañero. Izquierda/derecha: formato.",
                     "SELECT switches to your partner. Left/right: format.");
  S::Speech(dl, a.At(200, 615), a.At(840, 845), a.s, text.c_str(), right ? 0.92f : 0.12f);
  S::HelpLine(dl, a.At(600, 868), 40 * a.s, {{"A", T::T("Aceptar", "OK")}, {"B", T::T("Atras", "Back")}});
}

// Sobre la rueda del juego: la tira de cada lado (casillas numeradas, como Tenkaichi 3).
void TagTeamPanel::DrawPicking(ImGuiIO& io) {
  std::array<std::vector<int>, 2> picks;
  std::array<bool, 2> done;
  Notice notice;
  {
    std::lock_guard<std::mutex> lock(g_mu);
    picks = g_picks;
    done = g_done;
    notice = g_notice;
  }
  const S::Area a = S::GameArea(io);
  auto* dl = ImGui::GetForegroundDrawList();
  const float t = float(ImGui::GetTime());
  const int mode = g_mode.load();
  const int cur = mode == 1 ? -1 : (done[0] ? 1 : 0);
  const float since = float(Now() - notice.at);
  for (int side = 0; side < 2; ++side) {
    const int need = Need(side);
    if (need < 2) continue;
    std::vector<std::string> names;
    std::vector<ImTextureID> faces;
    for (int id : picks[size_t(side)]) {
      names.push_back(Name(id));
      faces.push_back(FaceOf(id));
    }
    const bool active = !done[size_t(side)] && (cur < 0 || cur == side);
    const ImVec2 p0 = side == 0 ? a.At(30, 285) : a.At(1170, 285);
    S::TeamStrip(dl, p0, a.s, side == 1, need, names, faces, active, done[size_t(side)], t);
    // quien elige este equipo y cuantos lleva, bajo la tira (encima esta el nombre del juego)
    const bool com = (mode == 0 && side == 1) || mode == 2;
    char head[64];
    std::snprintf(head, sizeof(head), "%s  %d/%d", com ? "COM" : side == 0 ? "1P" : "2P",
                  int(picks[size_t(side)].size()), need);
    const float stripw = need * 84 + (need - 1) * 8;
    const ImVec2 hc(side == 0 ? p0.x + stripw * 0.5f * a.s : p0.x - stripw * 0.5f * a.s, p0.y + 128 * a.s);
    // rotulo y aviso solo con la rueda de ese lado eligiendo: debajo sale el menu de capsulas del juego
    const bool browsing = dbz3_select_now_ms() - dbz3_select_browse_ms(side) < 300;
    if (!done[size_t(side)] && browsing) S::MenuText(dl, hc, 30 * a.s, head, active, !active);
    // aviso de lo ultimo que paso en este lado (3,5 s: el juego tarda en volver a la rueda)
    if (browsing && notice.side == side && notice.what != Event::kNone && since < 3.5f) {
      const char* msg = notice.what == Event::kRepeated ? T::T("Ya esta en el equipo", "Already in the team")
                        : notice.what == Event::kRemoved ? T::T("Quitado", "Removed")
                                                         : T::T("Anadido", "Added");
      const ImVec2 mc(hc.x, p0.y + 162 * a.s);
      S::MenuText(dl, mc, 28 * a.s, msg, notice.what != Event::kRepeated, false);
    }
  }
  // ayuda segun el momento: de quien es el turno y que hace cada boton
  std::string who;
  if (cur >= 0 && !done[size_t(cur)]) {
    who = cur == 0 ? T::T("Elige tu equipo", "Pick your team") : T::T("Elige el equipo rival", "Pick the rival team");
  } else if (cur < 0) {
    who = T::T("Cada jugador elige su equipo", "Each player picks a team");
  }
  if (!who.empty()) S::MenuText(dl, a.At(600, 836), 30 * a.s, who.c_str(), true, false);
  S::HelpLine(dl, a.At(600, 878), 34 * a.s,
              {{"A", T::T("Anadir", "Add")}, {"Y", T::T("Quitar el ultimo", "Remove last")}});
}

// En combate: fichas de los compañeros bajo el marcador de cada jugador (P1 izquierda, P2
// derecha). El marcador del juego ocupa x 15..540 / 660..1185, y 50..160 del area 4:3.
void DrawTeamHud(ImGuiIO& io) {
  const S::Area a = S::GameArea(io);
  auto* dl = ImGui::GetForegroundDrawList();
  const float t = float(ImGui::GetTime());
  for (int p = 0; p < 2; ++p) {
    const dbz3::tag::HudSide h = dbz3::tag::Hud(p);
    if (h.n < 2) continue;
    float y = 168;
    int alive_seen = 0;
    for (int s = 1; s < h.n; ++s) {   // en el orden del relevo
      const int k = (h.active + s) % h.n;
      const char* button = nullptr;
      if (!h.down[k]) button = alive_seen++ == 0 ? "BACK" : "BACK+LB";   // SELECT del mando = BACK
      const ImVec2 p0 = p == 0 ? a.At(40, y) : a.At(1160, y);
      // SELECT rechazado (no libre, enfriamiento, sin pelea): la ficha parpadea en gris medio segundo
      const bool grey = h.down[k] || (h.refused && std::fmod(t * 8.0f, 2.0f) < 1.0f);
      S::PartnerCard(dl, p0, a.s, p == 1, Name(h.ids[k]).c_str(), FaceOf(h.ids[k]), h.hp[k], grey, button,
                     h.switching && s == 1, t);
      y += 48;
    }
  }
}

void TagTeamPanel::OnDraw(ImGuiIO& io) {
  if (dbz3::GameDimmed()) return;   // fuera de la ventana: solo la pantalla oscura del juego
  const bool battle = dbz3::tag::InBattle();
  if (battle || !dbz3_select_live()) forms::FlushPreferences();
  const auto preference_error = forms::PreferenceError();
  if (!preference_error.empty() && !battle && dbz3_select_live()) {
    S::MenuText(ImGui::GetForegroundDrawList(), S::GameArea(io).At(600, 810), 22*S::GameArea(io).s,
                T::T("Preferencias de formas no guardadas", "Form preferences not saved"), false, false);
  }
  if (battle) DrawTeamHud(io);
  if (g_team_mode.load()) {
    if (battle) {
      g_after_battle = true;
    } else if (GameScreen() == kMainScreen && !dbz3::ingame::pilot::Busy()) {
      g_team_mode = false;                 // fuera del modo Tag Team: el Duelo vuelve a ser normal
      dbz3::tag::ClearDynamicTeams();
      dbz3::tag::SetComSide(0, false);
      dbz3::tag::SetComSide(1, false);
    } else if (g_after_battle.load() && dbz3_select_live()) {
      g_after_battle = false;              // otra eleccion tras el combate: equipos nuevos
      std::lock_guard<std::mutex> lock(g_mu);
      for (auto& p : g_picks) p.clear();
      g_done = {};
      g_await = {};
      dbz3::tag::ClearDynamicTeams();
    }
    if (!battle && dbz3_select_live()) DrawPicking(io);
  }
  if (!battle && dbz3_select_live() && !dbz3::ingame::pilot::Busy() && FormSelectOn()) DrawFormPick(io, g_team_mode.load());
  if (GameScreen() == kMainScreen && !Online()) {   // fuera de los combates (historias incluidas); online lo hace FormPickPad
    g_start_form[0] = 0;
    g_start_form[1] = 0;
  }
  const bool show = g_wanted.load() && !battle && !dbz3::ingame::pilot::Busy();
  auto* in = Input();
  if (show != blocked_ && in) {
    if (show) in->AddUIInputBlocker(); else in->RemoveUIInputBlocker();
    blocked_ = show;
  }
  if (show && !shown_) wait_release_ = true;
  shown_ = show;
  if (show) DrawMenu(io, Pad());

  // de la rueda al menu de Duelo en modo Tag Team (Y sin miembros): de vuelta a TAG TEAM por debajo
  const double now = Now();
  if (dbz3_select_live()) g_live_at = now;
  const bool duel_menu = g_team_mode.load() && !battle && GameScreen() == kDuelScreen && !dbz3_select_live() &&
                         !dbz3_select_to_stage() && !dbz3::ingame::pilot::Busy() && g_gate.load() == Gate::kNone;
  if (!duel_menu) {
    g_duel_at = 0;
  } else if (g_duel_at == 0) {
    g_duel_at = now;
  } else if (now - g_live_at < 4.0 && now - g_duel_at > 0.3) {
    StartGate(Gate::kToPanel);
    using namespace dbz3::ingame::pilot;
    Run({PressToLeave(kY, kDuelScreen), WaitScreen(kMainScreen)});
  }
  const Gate gate = g_gate.load();
  if (gate == Gate::kNone) return;
  const double age = now - g_gate_at;
  if (!dbz3::ingame::pilot::Busy() && g_gate_out == 0) {
    if (gate == Gate::kToWheel ? dbz3_select_live() : GameScreen() == kMainScreen) {
      if (gate == Gate::kToPanel) TagTeamOpen();
      g_gate_out = now;
    } else if (age > 1.0) {   // el piloto no llego (o el jugador pulso atras): se ve donde este
      g_gate_out = now;
    }
  }
  float alpha = std::clamp(float(age / 0.25), 0.0f, 1.0f);
  if (g_gate_out > 0) alpha = std::min(alpha, 1.0f - std::clamp(float((now - g_gate_out) / 0.4), 0.0f, 1.0f));
  if (gate == Gate::kToWheel && age < 0.25) DrawMenu(io, 0);   // la pantalla TAG TEAM funde a negro
  ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(0, 0, 0, int(alpha * 255)));
  if (g_gate_out > 0 && now - g_gate_out >= 0.4) g_gate = Gate::kNone;
}

}  // namespace dbz3::ingame
