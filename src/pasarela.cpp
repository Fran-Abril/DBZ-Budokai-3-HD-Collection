// dbz3 - Pasarela de modos: modos de otros juegos de la saga dentro del HUB.
// Primer modo: "La leyenda de Mr. Satan" de Budokai 1 (PS2): 11 retadores seguidos con las charlas del original.
//
// Datos: la ISO de Budokai 1 del jugador (ps2_games junto a dbz3.exe), leida en tiempo de ejecucion por
// pasarela_b1.cpp: combates (tabla del ELF), charlas y avisos (imagenes de texto en el idioma del juego), logo,
// fondo y dibujo de Mr. Satan. Nada del juego de origen va en la version. Entrada del HUB: "satan" (Leyenda de
// Mr. Satan); sin esa ISO sale en gris y el bocadillo explica que falta (RegisterGate / Motivo).
// Pasos: presentacion -> por cada retador: charla de Mr. Satan (desde el 2.o) -> tarjeta del combate con el aviso del
// original -> combate (conector nativo dbz3::hub::RequestBattle, src/hub.cpp; Cell Jr: 5 seguidos) -> ... -> final.
// El resultado se lee sin ganchos: estado del combate mgr+35128 (4 pelea, 5 acabado, 6 pose, 7 menu final) y hueco
// del ganador mgr+35136 (0 = el jugador), mgr = *0x823D6CF4 (los de tagteam.cpp LabTick). Perder: reintentar o salir;
// el avance se guarda en user_data/dbz3/pasarela/progreso.toml. Las reglas especiales del original (vida que no se
// recupera, 60 s, sin tiempo, 5 Cell Jr. de 1000, formas iniciales) van por dbz3::hub::RequestBattleEx.
// Sin ganchos REX_HOOK_RAW propios. Solo US (con g_guest_eu no hay modo).
#include "pasarela.h"

#include <rex/hook.h>
#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/immediate_drawer.h>

#include <imgui.h>
#include <toml++/toml.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "generated/dbz3_init.h"
#include "guest_region.h"
#include "ingame/game_style.h"
#include "launcher/i18n.h"
#include "launcher/launcher_state.h"
#include "launcher/settings.h"

namespace dbz3::hub {
bool RequestBattle(int id1, int id2, int stage, bool p2_com);   // hub.cpp
// 0 = lo normal en cada campo; time_s > 0 segundos, < 0 sin tiempo; hp > 0 vida inicial y maxima, < 0 solo la
// inicial; form = forma inicial
bool RequestBattleEx(int id1, int id2, int stage, bool p2_com, int time_s, int hp1, int hp2, int form1, int form2);
bool OwnBattle();
bool RegisterMode(const char* key, void (*open)(), bool (*busy)());   // hub_menu.cpp
bool RegisterGate(const char* key, bool (*ready)(), const char* (*why)());
}  // namespace dbz3::hub
namespace dbz3 {
bool GameDimmed();
bool OverlayOpen();
}  // namespace dbz3
namespace dbz3::ingame {
int GameScreen();   // online_panel.cpp
}

namespace dbz3::pasarela {
namespace {

namespace T = dbz3::i18n;
namespace S = dbz3::ingame::style;

constexpr uint32_t kMgrPtr = 0x823D6CF4, kStages = 0x82323760;   // escenarios: 17 x 92 B, +0x10 mapa (stage_ext.h)
constexpr int kMainScreen = 5, kJugador = 14;                    // Mr. Satan en B3
constexpr uint16_t kUp = 0x0001, kDown = 0x0002, kStart = 0x0010, kA = 0x1000, kB = 0x2000;
constexpr const char* kClave = "hercule";
constexpr float kAutoSeg = 4.0f;   // prueba (DBZ3_TEST_PASARELA=auto): segundos por pantalla

int64_t NowMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
      .count();
}
uint8_t* Base() {
  rex::Runtime* rt = rex::Runtime::instance();
  return rt ? rt->virtual_membase() : nullptr;
}

// ---- Datos leidos de la ISO (hilo propio; se cambian enteros) ----
std::mutex g_datos_mu;
std::shared_ptr<const DatosB1> g_datos;   // nullptr = sin ISO o sin leer
std::string g_error;                      // por que no hay datos
std::atomic<bool> g_leyendo{false};
std::atomic<int64_t> g_ultimo_intento{-100000};
std::atomic<uint32_t> g_generacion{0};    // sube con cada lectura (el panel rehace sus texturas)

std::shared_ptr<const DatosB1> Datos() {
  std::lock_guard<std::mutex> lk(g_datos_mu);
  return g_datos;
}

// Busca la ISO y la lee en un hilo (como mucho una lectura a la vez; reintenta cada 10 s si no hay).
void Leer(bool forzar) {
  if (dbz3::g_guest_eu) return;
  if (!forzar && NowMs() - g_ultimo_intento.load() < 10000) return;
  if (g_leyendo.exchange(true)) return;
  g_ultimo_intento = NowMs();
  std::thread([lang = std::string(T::LangCode())] {
    auto d = std::make_shared<DatosB1>();
    std::string error;
    const std::filesystem::path iso = BuscarIsoB1();
    bool ok = !iso.empty();
    if (!ok) error = "sin ISO de Budokai 1 en ps2_games";
    const int64_t t0 = NowMs();
    if (ok) ok = LeerLeyendaSatan(iso, lang, *d, error);
    {
      std::lock_guard<std::mutex> lk(g_datos_mu);
      if (ok) g_datos = d;
      g_error = error;
    }
    if (ok) {
      ++g_generacion;
      REXLOG_INFO("dbz3 pasarela: {} leida en {} ms: ELF {} tabla 0x{:X}, {} combates, charlas {} ({}, {} imagenes)",
                  d->iso, NowMs() - t0, d->elf, d->tabla, d->combates.size(), d->afs.empty() ? "-" : d->afs,
                  d->idioma, d->charlas.size());
    } else {
      REXLOG_INFO("dbz3 pasarela: leyenda de Mr. Satan no disponible ({})", error);
    }
    g_leyendo = false;
  }).detach();
}

// ---- Pasos del modo ----
enum Tipo : int { kPresentacion, kCharla, kCombate };
struct Paso {
  int tipo = kPresentacion;
  int ronda = 0;      // retador (0..10)
  int vez = 0;        // combate: cual de los `cantidad` seguidos
};

std::vector<Paso> Pasos(const DatosB1& d) {
  std::vector<Paso> p;
  p.push_back({kPresentacion, 0, 0});
  for (int r = 0; r < int(d.combates.size()); ++r) {
    if (r > 0) p.push_back({kCharla, r, 0});
    for (int v = 0; v < std::max(1, d.combates[size_t(r)].cantidad); ++v) p.push_back({kCombate, r, v});
  }
  return p;
}

// Avance guardado: user_data/dbz3/pasarela/progreso.toml (clave = paso por el que se sigue).
std::filesystem::path Carpeta() { return dbz3::settings::UserDataRoot() / "pasarela"; }
std::string LeerFichero(const std::filesystem::path& p) {
  std::ifstream f(p, std::ios::binary);
  if (!f) return {};
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}
int LeerProgreso() {
  try {
    const toml::table t = toml::parse(LeerFichero(Carpeta() / "progreso.toml"));
    return int(t[kClave].value_or(int64_t(0)));
  } catch (const toml::parse_error&) {
    return 0;
  }
}
void GuardarProgreso(int paso) {
  std::error_code ec;
  std::filesystem::create_directories(Carpeta(), ec);
  if (std::FILE* f = std::fopen((Carpeta() / "progreso.toml").string().c_str(), "w")) {
    std::fprintf(f, "# Pasarela de modos: paso por el que sigue cada modo (lo escribe el juego)\n%s = %d\n", kClave, paso);
    std::fclose(f);
  }
}

// Nombres de B3 (los de tagteam_panel.cpp).
struct Nombre {
  int id;
  const char *es, *en;
};
constexpr Nombre kNombres[] = {
    {0, "Goku", "Goku"}, {2, "Gohan (niño)", "Kid Gohan"}, {3, "Gohan (adolescente)", "Teen Gohan"},
    {5, "Gran Saiyaman", "Great Saiyaman"}, {7, "Vegeta", "Vegeta"}, {8, "Trunks", "Trunks"},
    {10, "Krilin", "Krillin"}, {11, "Piccolo", "Piccolo"}, {12, "Ten Shin Han", "Tien"}, {13, "Yamcha", "Yamcha"},
    {14, "Mr. Satán", "Hercule"}, {18, "Raditz", "Raditz"}, {19, "Nappa", "Nappa"}, {20, "Ginyu", "Ginyu"},
    {21, "Recoome", "Recoome"}, {27, "Freezer", "Frieza"}, {28, "Androide 16", "Android 16"},
    {29, "Androide 17", "Android 17"}, {30, "Androide 18", "Android 18"}, {32, "Dr. Gero", "Dr. Gero"},
    {33, "Cell", "Cell"}, {38, "Cooler", "Cooler"}, {42, "Saibaman", "Saibaman"}, {43, "Cell Jr.", "Cell Jr."}};
const char* NombreDe(int id) {
  for (const auto& n : kNombres) {
    if (n.id == id) return T::T(n.es, n.en);
  }
  return "";
}

// Escenario de B3 cuyo mapa (entrada de data_cmn) es `mapa`: el de los Juegos de Cell es la 50 (CELL_RING_MAP).
int EscenarioB3(uint8_t* base, const std::string& nombre_b1) {
  int mapa = 50;
  std::string n = nombre_b1;
  if (n.rfind("CHG_", 0) == 0) n = n.substr(4);
  static const std::pair<const char*, int> kMapas[] = {{"CELL RING", 50}, {"NAMEC", 60},   {"RETTOU", 56},
                                                       {"SEISHIN", 58},   {"TENKAITI", 46}, {"BOKUSOUTI", 62},
                                                       {"IWAYAMA", 64},   {"SANKAN", 64},   {"TIKYUU", 68}};
  for (const auto& [k, m] : kMapas) {
    if (n.rfind(k, 0) == 0) mapa = m;
  }
  for (uint32_t i = 0; base && i < 17; ++i) {
    if (REX_LOAD_U32(kStages + 92 * i + 0x10) == uint32_t(mapa)) return int(i);
  }
  return 0;
}

// ---- Estado (g_mu) ----
enum Fase : int { kNada = 0, kPantalla, kLuchando, kPerdido, kFin };
const char* const kFaseNombre[] = {"nada", "pantalla", "combate", "perdido", "fin"};

std::mutex g_mu;
std::shared_ptr<const DatosB1> g_d;   // datos del modo en curso
std::vector<Paso> g_pasos;
int g_fase = kNada, g_paso = 0, g_intentos = 0;
int g_vida_ganada = 0;   // vida de Mr. Satan al ganar el ultimo combate (para los que no la recuperan)
uint64_t g_cambio = 0;   // sube con cada pantalla nueva (el panel reinicia el texto)
std::atomic<int> g_fase_pub{kNada};
std::atomic<bool> g_auto{false};
std::atomic<bool> g_panel{false};

void Publicar(int fase) {   // con g_mu
  if (fase == kFin) {
    REXLOG_INFO("dbz3 pasarela: final de la leyenda ({} pasos)", g_pasos.size());
  } else if (fase != g_fase || fase == kPantalla) {
    const Paso& p = g_pasos[size_t(std::min<int>(g_paso, int(g_pasos.size()) - 1))];
    REXLOG_INFO("dbz3 pasarela: paso {}/{} ({} ronda {}{}): {}", g_paso + 1, g_pasos.size(),
                p.tipo == kPresentacion ? "presentacion" : p.tipo == kCharla ? "charla" : "combate", p.ronda + 1,
                p.tipo == kCombate && p.vez ? " vez " + std::to_string(p.vez + 1) : std::string(), kFaseNombre[fase]);
  }
  g_fase = fase;
  g_fase_pub = fase;
  ++g_cambio;
}

void Salir(bool completo) {   // con g_mu
  if (completo) {
    GuardarProgreso(0);
  } else {   // se vuelve a la charla (o la tarjeta) del combate pendiente
    int vuelta = std::min<int>(g_paso, int(g_pasos.size()) - 1);
    while (vuelta > 0 && g_pasos[size_t(vuelta)].tipo == kCombate && g_pasos[size_t(vuelta)].vez > 0) --vuelta;
    if (vuelta > 0 && g_pasos[size_t(vuelta)].tipo == kCombate && g_pasos[size_t(vuelta - 1)].tipo == kCharla) --vuelta;
    GuardarProgreso(vuelta);
  }
  REXLOG_INFO("dbz3 pasarela: {}", completo ? "leyenda completada" : "fuera del modo (se guarda el avance)");
  g_paso = 0;
  Publicar(kNada);
}

void Entrar(int paso) {   // con g_mu
  g_paso = paso;
  g_intentos = 0;
  Publicar(paso >= int(g_pasos.size()) ? kFin : kPantalla);
}

// Vigila el combate pedido: ganador al acabar (estados 5-7) y final del combate nuestro (hub.cpp suelta OwnBattle al
// volver al menu principal). Solo lee memoria.
void Vigilar(uint64_t cambio, int paso) {
  std::thread([cambio, paso] {
    uint8_t* base = Base();
    bool visto = false;
    int ganador = -1, vida = 0;
    uint32_t ultimo = 0;
    const int64_t t0 = NowMs();
    while (base) {
      const bool own = dbz3::hub::OwnBattle();
      visto = visto || own;
      const uint32_t mgr = REX_LOAD_U32(kMgrPtr);
      const uint32_t st = own && mgr ? REX_LOAD_U32(mgr + 35128) : 0;
      if (own && st == 4) ganador = -1;   // pelea (tambien la revancha de "Battle Again?")
      if (own && st >= 5 && st <= 7) {
        ganador = int32_t(REX_LOAD_U32(mgr + 35136));
        const uint32_t u = REX_LOAD_U32(mgr + 2096 + 6080);   // vida del jugador (float, la de modos.cpp)
        float f;
        std::memcpy(&f, &u, sizeof(f));
        vida = int(f);
      }
      if (own && st != ultimo) REXLOG_INFO("dbz3 pasarela: paso {} estado del combate {} (ganador {})", paso + 1, st, ganador);
      ultimo = st;
      if (visto && !own) break;
      if (!visto && NowMs() - t0 > 60000) {
        REXLOG_WARN("dbz3 pasarela: el combate no empezo en 60 s");
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
    std::lock_guard<std::mutex> lk(g_mu);
    if (g_fase != kLuchando || g_cambio != cambio) return;
    REXLOG_INFO("dbz3 pasarela: fin del combate (paso {}): gana el lado {}, vida de Mr. Satan {}", g_paso + 1,
                ganador + 1, vida);
    if (ganador == 0) {
      g_vida_ganada = vida;
      GuardarProgreso(g_paso + 1);
      Entrar(g_paso + 1);
    } else {
      ++g_intentos;
      Publicar(kPerdido);
    }
  }).detach();
}

// Forma de B1 (+0x52) -> forma inicial de B3 (rec+20), medida en el juego 2026-10-15: Goku 2 = SSJ (la 1 es el
// Kaioken), Gohan adolescente 1 = SSJ, Cell 2 = perfecto. Piccolo (forma 2 en B1) no cambia en B3: se queda en 0.
int FormaB3(int id, int forma_b1) {
  static constexpr int kFormas[][3] = {{0, 4, 2}, {3, 1, 1}, {33, 2, 2}};   // ID de B3, forma de B1, forma de B3
  for (const auto& f : kFormas) {
    if (f[0] == id && f[1] == forma_b1) return f[2];
  }
  return 0;
}

bool Lanzar() {   // con g_mu: el combate del paso actual, con las reglas del original
  const Paso& p = g_pasos[size_t(g_paso)];
  const CombateB1& c = g_d->combates[size_t(p.ronda)];
  const int escenario = EscenarioB3(Base(), c.escenario_nombre);
  // vida que no se recupera (Krilin, Gohan) y los Cell Jr. seguidos: la que le quedo al ganar el anterior
  const bool arrastra = c.arrastra_vida || p.vez > 0;
  const int vida1 = arrastra && g_vida_ganada > 0 ? -g_vida_ganada : 0;   // < 0: solo la inicial (maxima de serie, como B1)
  const int vida2 = c.vida_rival > 0 && c.vida_rival != 3000 ? c.vida_rival : 0;   // Cell Jr.: 1000
  const int tiempo = c.tiempo > 0 ? (c.tiempo + 30) / 60 : c.tiempo == -1 ? -1 : 0;  // cuadros -> s; -1 sin tiempo
  const int forma2 = FormaB3(c.rival, c.forma);
  if (!dbz3::hub::RequestBattleEx(kJugador, c.rival, escenario, true, tiempo, vida1, vida2, 0, forma2)) return false;
  REXLOG_INFO("dbz3 pasarela: ronda {} ({}/{}): id {} contra id {} (B1 {}, forma B1 {} -> {}), escenario {} ({}), "
              "tiempo {}, vida {} / {}", p.ronda + 1, p.vez + 1, c.cantidad, kJugador, c.rival, c.rival_b1, c.forma,
              forma2, escenario, c.escenario_nombre, tiempo, vida1, vida2);
  Publicar(kLuchando);
  Vigilar(g_cambio, g_paso);
  return true;
}

void CreatePanel();

}  // namespace

// ---- API (HUB y pruebas) ----

bool Disponible() {
  if (dbz3::g_guest_eu) return false;
  if (!Datos()) Leer(false);
  return Datos() != nullptr;
}

const char* Motivo() {
  if (dbz3::g_guest_eu) return T::T("Solo con la versión americana del juego.", "Only with the American version of the game.");
  if (Disponible()) {
    return T::T("La leyenda de Mr. Satán de Budokai 1: once retadores seguidos con las charlas de tu copia del juego.",
                "Hercule's legend from Budokai 1: eleven challengers in a row, with the talks from your copy of the game.");
  }
  return T::T("Necesita tu copia de Dragon Ball Z Budokai (PS2): pon la ISO en la carpeta ps2_games, junto al juego.",
              "Needs your copy of Dragon Ball Z Budokai (PS2): put the ISO in the ps2_games folder, next to the game.");
}

// Empieza el modo (hilo del juego con el menu principal activo, o el hilo de prueba). false si no hay datos.
bool Start() {
  const auto d = Datos();
  if (dbz3::g_guest_eu || !d || d->combates.empty()) {
    REXLOG_WARN("dbz3 pasarela: no se puede empezar (sin datos de Budokai 1)");
    Leer(true);
    return false;
  }
  if (d->idioma != T::LangCode()) Leer(true);   // otro idioma: se releen las imagenes (mientras, las de antes)
  if (!g_panel.exchange(true)) dbz3::launcher::DeferUi(&CreatePanel);
  std::lock_guard<std::mutex> lk(g_mu);
  if (g_fase != kNada) return false;
  g_d = d;
  g_pasos = Pasos(*d);
  int desde = LeerProgreso();
  if (desde < 0 || desde >= int(g_pasos.size())) desde = 0;
  REXLOG_INFO("dbz3 pasarela: la leyenda de Mr. Satan ({} combates, {} pasos), desde el paso {}", d->combates.size(),
              g_pasos.size(), desde + 1);
  Entrar(desde);
  return true;
}

bool Busy() { return g_fase_pub.load() != kNada; }

namespace {

// ---- Pantalla (hilo de la interfaz) ----
rex::input::InputSystem* Input() {
  auto* rt = rex::Runtime::instance();
  return rt ? static_cast<rex::input::InputSystem*>(rt->input_system()) : nullptr;
}

std::atomic<uint16_t> g_pad{0};
std::atomic<bool> g_pad_pending{false};

float Ease(float t) {
  t = std::clamp(t, 0.0f, 1.0f);
  return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}

class Panel : public rex::ui::ImGuiDialog {
 public:
  explicit Panel(rex::ui::ImGuiDrawer* drawer) : ImGuiDialog(drawer) {}

 protected:
  void OnDraw(ImGuiIO& io) override {
    const int fase = g_fase_pub.load();
    const bool menu = dbz3::ingame::GameScreen() == kMainScreen;
    const bool show = (fase == kPantalla || fase == kPerdido || fase == kFin) && menu && !dbz3::GameDimmed() &&
                      !dbz3::OverlayOpen();
    if (auto* in = Input(); in && show != blocked_) {   // mientras se ve, el juego no recibe los mandos
      if (show) in->AddUIInputBlocker(); else in->RemoveUIInputBlocker();
      blocked_ = show;
    }
    const float t = float(ImGui::GetTime()), dt = std::min(io.DeltaTime, 0.1f);
    alpha_ = std::clamp(alpha_ + (show ? dt * 4.0f : -dt * 8.0f), 0.0f, 1.0f);
    if (!show) {
      wait_release_ = true;   // la A que abrio el modo (o la del combate) no cuenta
      if (alpha_ <= 0.0f) return;
    }
    uint16_t pressed = Pad();
    if (wait_release_) {
      if (g_pad.load() == 0 && alpha_ >= 1.0f) wait_release_ = false;
      pressed = 0;
    }
    if (!show) pressed = 0;
    std::unique_lock<std::mutex> lk(g_mu);
    if (!g_d || g_pasos.empty()) return;
    if (g_cambio != visto_) {
      visto_ = g_cambio;
      cambio_at_ = t;
    }
    Texturas();
    const auto a = S::GameArea(io);
    auto* dl = ImGui::GetForegroundDrawList();
    const int v0 = dl->VtxBuffer.Size;
    Fondo(dl, a);
    const bool auto_on = g_auto.load();
    if (g_fase == kFin) {
      DrawFin(dl, a, t, pressed, auto_on);
    } else if (g_fase == kPerdido) {
      DrawCombate(dl, a, t, pressed, auto_on, true);
    } else {
      const Paso& p = g_pasos[size_t(g_paso)];
      if (p.tipo == kPresentacion) DrawPresentacion(dl, a, t, pressed, auto_on);
      else if (p.tipo == kCharla) DrawCharla(dl, a, t, pressed, auto_on);
      else DrawCombate(dl, a, t, pressed, auto_on, false);
    }
    lk.unlock();
    if (alpha_ < 1.0f) {
      for (int i = v0; i < dl->VtxBuffer.Size; ++i) {
        ImU32& c = dl->VtxBuffer[i].col;
        c = (c & 0x00FFFFFF) | (ImU32(float(c >> 24) * alpha_) << 24);
      }
    }
  }

 private:
  struct Tex {
    std::unique_ptr<rex::ui::ImmediateTexture> tex;
    float w = 0, h = 0;
  };

  uint16_t Pad() {
    if (!g_pad_pending.exchange(true)) {
      dbz3::launcher::DeferUi([]() {
        uint16_t b = 0;
        if (auto* in = Input()) {
          rex::input::X_INPUT_STATE st = {};
          if (in->GetStateForUI(0, &st) == 0) {
            b = static_cast<uint16_t>(st.gamepad.buttons);
            const int ly = int16_t(st.gamepad.thumb_ly);
            if (ly > 16000) b |= kUp;
            if (ly < -16000) b |= kDown;
          }
        }
        g_pad = b;
        g_pad_pending = false;
      });
    }
    const uint16_t b = g_pad.load();
    const uint16_t pressed = b & ~last_;
    last_ = b;
    return pressed;
  }

  // Texturas de las imagenes del juego de origen (se rehacen si se releyo la ISO, p. ej. otro idioma).
  void Texturas() {   // con g_mu
    const auto d = Datos();
    if (d && d != g_d && d->combates.size() == g_d->combates.size()) g_d = d;   // idioma nuevo: mismas rondas
    if (tex_gen_ == g_generacion.load() && tex_src_ == g_d.get()) return;
    auto* drawer = dbz3::launcher::PreviewDrawer();
    if (!drawer) return;
    tex_gen_ = g_generacion.load();
    tex_src_ = g_d.get();
    tex_.clear();
    auto subir = [&](int key, const Imagen& img) {
      if (!img) return;
      Tex t;
      t.tex = drawer->CreateTexture(uint32_t(img.w), uint32_t(img.h), rex::ui::ImmediateTextureFilter::kLinear, false,
                                    img.rgba.data());
      t.w = float(img.w);
      t.h = float(img.h);
      if (t.tex) tex_[key] = std::move(t);
    };
    for (int i = 0; i < int(g_d->charlas.size()); ++i) subir(i, g_d->charlas[size_t(i)]);
    subir(kLogo, g_d->logo);
    subir(kFondo, g_d->fondo);
    subir(kSatan, g_d->satan);
  }
  const Tex* Img(int key) const {
    auto it = tex_.find(key);
    return it != tex_.end() && it->second.tex ? &it->second : nullptr;
  }
  void Pic(ImDrawList* dl, const Tex& t, ImVec2 p0, ImVec2 p1, ImU32 tint = IM_COL32_WHITE, bool mirror = false) {
    ImVec2 uv0(0, 0), uv1(1, 1);
    if (mirror) std::swap(uv0.x, uv1.x);
    dl->AddImage(reinterpret_cast<ImTextureID>(t.tex.get()), p0, p1, uv0, uv1, tint);
  }
  // Imagen centrada en c con alto h (px de 1200x900).
  void PicAt(ImDrawList* dl, const S::Area& a, const Tex& t, float cx, float cy, float h, ImU32 tint = IM_COL32_WHITE) {
    const float w = t.w * h / t.h;
    Pic(dl, t, a.At(cx - w * 0.5f, cy - h * 0.5f), a.At(cx + w * 0.5f, cy + h * 0.5f), tint);
  }

  // Fondo azul y logo de la pantalla del modo de Budokai 1; sin ellos, los del Duelo de B3.
  void Fondo(ImDrawList* dl, const S::Area& a) {
    if (const Tex* f = Img(kFondo)) Pic(dl, *f, a.p0, a.p1);
    else S::Backdrop(dl, a, "fondo_duelo");
    S::Sides(dl, a);
    if (const Tex* l = Img(kLogo)) {
      const float h = 150, w = l->w * h / l->h;
      Pic(dl, *l, a.At(60, 18), a.At(60 + w, 18 + h));
    } else {
      S::Banner(dl, a, T::T("La leyenda de Mr. Satán", "Hercule's legend"));
    }
  }

  // Mr. Satan (su dibujo del juego de origen o su cara del marcador de B3) a la izquierda.
  void Satan(ImDrawList* dl, const S::Area& a, float t, bool habla) {
    const float bob = habla ? 4.0f * std::sin(t * 3.0f) : 0.0f;
    if (const Tex* s = Img(kSatan)) {
      const float h = 450, w = s->w * h / s->h;
      Pic(dl, *s, a.At(70, 175 + bob), a.At(70 + w, 175 + h + bob), habla ? IM_COL32_WHITE : IM_COL32(150, 150, 160, 255));
      return;
    }
    Cara(dl, a, kJugador, 0, habla, NombreDe(kJugador), t);
  }

  // Cara del marcador de B3 en un marco del juego.
  void Cara(ImDrawList* dl, const S::Area& a, int id, int lado, bool on, const char* nombre, float t) {
    const float x0 = lado == 0 ? 120.0f : 800.0f, y0 = 190.0f, w = 280.0f;
    const float bob = on ? 4.0f * std::sin(t * 3.0f) : 0.0f;
    const ImVec2 p0 = a.At(x0, y0 + bob), p1 = a.At(x0 + w, y0 + w + bob);
    S::Frame(dl, p0, p1, a.s);
    if (const S::Pic f = S::Img(("caras/" + std::to_string(id)).c_str())) {
      const float k = std::min((p1.x - p0.x) / f.w, (p1.y - p0.y) / f.h) * 0.92f;
      const ImVec2 c((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f), h(f.w * k * 0.5f, f.h * k * 0.5f);
      S::DrawPic(dl, f, ImVec2(c.x - h.x, c.y - h.y), ImVec2(c.x + h.x, c.y + h.y), ImVec4(0, 0, 0, 0),
                 on ? IM_COL32_WHITE : IM_COL32(120, 120, 130, 255), lado == 1);
    }
    if (nombre && *nombre) S::MenuText(dl, a.At(x0 + w * 0.5f, y0 + w + 42 + bob), 44 * a.s, nombre, on, false);
  }

  // Bocadillo del juego abajo; dentro, la imagen de texto del original (oscurecida: su gris es para fondo oscuro)
  // o un texto nuestro.
  void Bocadillo(ImDrawList* dl, const S::Area& a, float t, int imagen, const char* texto, float cola) {
    const float in = Ease((t - cambio_at_) / 0.3f);
    const Tex* img = imagen >= 0 ? Img(imagen) : nullptr;
    S::Speech(dl, a.At(60, 640 + 30 * (1 - in)), a.At(1140, 850 + 30 * (1 - in)), a.s, img ? "" : texto, cola);
    if (img) PicAt(dl, a, *img, 600, 745 + 30 * (1 - in), std::min(185.0f, img->h * 1.45f), IM_COL32(150, 150, 150, 255));
  }

  // Franja oscura con el aviso del combate (imagen del original, letra clara) o un texto nuestro.
  void Aviso(ImDrawList* dl, const S::Area& a, int imagen, const std::string& texto) {
    const Tex* img = imagen >= 0 ? Img(imagen) : nullptr;
    if (!img && texto.empty()) return;
    dl->AddRectFilled(a.At(150, 540), a.At(1050, 625), IM_COL32(8, 30, 48, 215), 10 * a.s);
    if (img) PicAt(dl, a, *img, 600, 582, std::min(78.0f, img->h * 1.25f));
    else S::MenuText(dl, a.At(600, 582), 34 * a.s, texto.c_str(), true, false);
  }

  void Ayuda(ImDrawList* dl, const S::Area& a, const std::vector<std::pair<const char*, const char*>>& items) {
    S::HelpLine(dl, a.At(600, 878), 34 * a.s, items);
  }

  void DrawPresentacion(ImDrawList* dl, const S::Area& a, float t, uint16_t pressed, bool auto_on) {
    Satan(dl, a, t, true);
    Bocadillo(dl, a, t, -1,
              T::T("¡Soy Mr. Satán, el campeón del mundo! En los Juegos de Cell me esperan once retadores. ¡Ja, ja, ja!",
                   "I'm Hercule, the world champion! Eleven challengers await me at the Cell Games. Ha ha ha!"),
              0.22f);
    Ayuda(dl, a, {{"A", T::T("Empezar", "Start")}, {"B", T::T("Salir", "Quit")}});
    if (auto_on && t - cambio_at_ > kAutoSeg) pressed |= kA;
    if (pressed & kB) return Salir(false);
    if (pressed & (kA | kStart)) Entrar(g_paso + 1);
  }

  void DrawCharla(ImDrawList* dl, const S::Area& a, float t, uint16_t pressed, bool auto_on) {
    const Paso& p = g_pasos[size_t(g_paso)];
    const CombateB1& c = g_d->combates[size_t(p.ronda)];
    Satan(dl, a, t, true);
    Cara(dl, a, c.rival, 1, false, NombreDe(c.rival), t);
    Bocadillo(dl, a, t, 2 * (p.ronda - 1),
              T::T("¡Siguiente! ¡El campeón no se rinde nunca!", "Next! The champion never gives up!"), 0.22f);
    Ayuda(dl, a, {{"A", T::T("Siguiente", "Next")}, {"B", T::T("Salir", "Quit")}});
    if (auto_on && t - cambio_at_ > kAutoSeg) pressed |= kA;
    if (pressed & kB) return Salir(false);
    if (pressed & (kA | kStart)) Entrar(g_paso + 1);
  }

  // Tarjeta del combate (y, tras perder, la de reintentar): "Retador N de 11", las dos caras y el aviso del original.
  void DrawCombate(ImDrawList* dl, const S::Area& a, float t, uint16_t pressed, bool auto_on, bool perdido) {
    const Paso& p = g_pasos[size_t(g_paso)];
    const CombateB1& c = g_d->combates[size_t(p.ronda)];
    Cara(dl, a, kJugador, 0, !perdido, NombreDe(kJugador), t);
    Cara(dl, a, c.rival, 1, true, NombreDe(c.rival), t);
    const float in = Ease((t - cambio_at_) / 0.35f);
    S::MenuText(dl, a.At(600, 330), (110 + 40 * (1 - in)) * a.s, "VS", true, false);
    char titulo[96];
    if (c.cantidad > 1) {
      std::snprintf(titulo, sizeof(titulo), T::T("Retador %d de %d (%d/%d)", "Challenger %d of %d (%d/%d)"),
                    p.ronda + 1, int(g_d->combates.size()), p.vez + 1, c.cantidad);
    } else {
      std::snprintf(titulo, sizeof(titulo), T::T("Retador %d de %d", "Challenger %d of %d"), p.ronda + 1,
                    int(g_d->combates.size()));
    }
    S::MenuText(dl, a.At(730, 115), 50 * a.s, titulo, true, false);
    if (perdido) {
      Aviso(dl, a, -1, T::T("¡Mr. Satán ha caído!", "Hercule is down!"));
      S::Speech(dl, a.At(60, 660), a.At(1140, 840), a.s,
                T::T("¡El campeón del mundo nunca se rinde! ¿Lo intentas otra vez?",
                     "The world champion never gives up! Want to try again?"),
                -1);
      Ayuda(dl, a, {{"A", T::T("Reintentar", "Retry")}, {"B", T::T("Salir", "Quit")}});
      if (auto_on && t - cambio_at_ > kAutoSeg) pressed |= g_intentos < 2 ? kA : kB;
      if (pressed & kB) return Salir(false);
      if (pressed & kA) Publicar(kPantalla);
      return;
    }
    if (p.ronda > 0) Aviso(dl, a, 2 * (p.ronda - 1) + 1, {});
    std::string texto = std::string(NombreDe(kJugador)) + T::T(" contra ", " versus ") + NombreDe(c.rival) + ".";
    if (g_intentos > 0) {
      char k[48];
      std::snprintf(k, sizeof(k), T::T(" Intento %d.", " Try %d."), g_intentos + 1);
      texto += k;
    }
    S::Speech(dl, a.At(60, 660), a.At(1140, 840), a.s, texto.c_str(), -1);
    Ayuda(dl, a, {{"A", T::T("¡A luchar!", "Fight!")}, {"B", T::T("Salir", "Quit")}});
    if (auto_on && t - cambio_at_ > kAutoSeg) pressed |= kA;
    if (pressed & kB) return Salir(false);
    if ((pressed & kA) && !Lanzar()) REXLOG_WARN("dbz3 pasarela: el conector no acepto el combate (otro pendiente)");
  }

  void DrawFin(ImDrawList* dl, const S::Area& a, float t, uint16_t pressed, bool auto_on) {
    Satan(dl, a, t, true);
    Bocadillo(dl, a, t, -1,
              T::T("¡Once retadores y ni una derrota! ¡El campeón del mundo lo ha vuelto a hacer! ¡Leyenda completada!",
                   "Eleven challengers and not one defeat! The world champion did it again! Legend complete!"),
              0.22f);
    Ayuda(dl, a, {{"A", T::T("Volver al HUB", "Back to the HUB")}});
    if (auto_on && t - cambio_at_ > kAutoSeg) pressed |= kA;
    if (pressed & kA) Salir(true);
  }

  enum : int { kLogo = 100, kFondo, kSatan };
  std::map<int, Tex> tex_;
  uint32_t tex_gen_ = ~0u;
  const DatosB1* tex_src_ = nullptr;
  bool blocked_ = false, wait_release_ = true;
  uint16_t last_ = 0;
  uint64_t visto_ = ~0ull;
  float cambio_at_ = 0, alpha_ = 0;
};

void CreatePanel() {
  static Panel* panel = nullptr;   // vive lo que el juego
  auto* rt = rex::Runtime::instance();
  if (!panel && rt && rt->imgui_drawer()) panel = new Panel(rt->imgui_drawer());
}

// HUB: entrada "satan" ("Leyenda de Mr. Satan", la reserva hub_panel.h): siempre a la vista; sin la ISO de
// Budokai 1, en gris y el bocadillo dice Motivo() (RegisterGate).
void Abrir() {
  if (!Start()) REXLOG_WARN("dbz3 pasarela: no se pudo empezar");
}
const bool g_hub = dbz3::hub::RegisterMode("satan", &Abrir, &Busy) &&
                   dbz3::hub::RegisterGate("satan", &Disponible, &Motivo);

// PRUEBA: DBZ3_TEST_PASARELA=1 (o "auto": avanza solo; "auto,desde=N": empieza en el paso N) empieza el modo cuando
// el menu principal lleva 3 s en pantalla y escribe el avance en el log ("dbz3 pasarela: ...").
const bool g_test = [] {
  const char* t = std::getenv("DBZ3_TEST_PASARELA");
  if (!t) return false;
  g_auto = std::strstr(t, "auto") != nullptr;
  int desde = -1;
  if (const char* d = std::strstr(t, "desde=")) desde = std::atoi(d + 6);
  std::thread([desde] {
    for (int quieto = 0; quieto < 120;) {
      quieto = dbz3::ingame::GameScreen() == kMainScreen ? quieto + 1 : 0;
      std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    Leer(true);
    for (int i = 0; i < 400 && g_leyendo.load(); ++i) std::this_thread::sleep_for(std::chrono::milliseconds(25));
    if (desde >= 0) GuardarProgreso(desde);
    REXLOG_INFO("dbz3 pasarela: prueba: disponible {}; {}", Disponible(), Start() ? "empieza" : "no empieza");
  }).detach();
  return true;
}();

}  // namespace
}  // namespace dbz3::pasarela
