// dbz3 - Pasarela de modos: modos de otros juegos de la saga dentro del HUB.
// Primer modo: "La leyenda de Mr. Satan" de Budokai 1 (PS2): 11 retadores seguidos con las charlas del original.
//
// Datos: la ISO de Budokai 1 del jugador (ps2_games junto a dbz3.exe), leida en tiempo de ejecucion por
// pasarela_b1.cpp: combates (tabla del ELF), charlas y avisos (imagenes de texto en el idioma del juego), logo,
// fondo, dibujo de Mr. Satan, dificultades y piezas del resultado. Nada del juego de origen va en la version. Entrada
// del HUB: "satan" (Leyenda de Mr. Satan); sin esa ISO sale en gris y el bocadillo explica que falta (RegisterGate).
// Pasos: presentacion con la dificultad (Novato/Experto/Avanzado, como el titulo de B1) -> por cada retador: charla de
// Mr. Satan (desde el 2.o) -> tarjeta del combate con el aviso del original -> combate (conector nativo
// dbz3::hub::RequestBattleEx, src/hub.cpp; Cell Jr: 5 seguidos) -> resultado con los puntos -> ... -> final.
// Reglas del original: las de la tabla por el conector (vida que no se recupera, 60 s, sin tiempo, 5 Cell Jr. de
// 1000, formas iniciales) y las que B1 tiene en codigo por memoria del luchador mientras se pelea (VigilarCombate):
// ki inicial del rival, armadura del Androide 16, Mr. Satan herido contra Trunks (solo el ki hace dano), ki lento
// contra Piccolo, y el nivel de la IA del COM de la dificultad (mgr+35573, el que el Torneo pone a Novato/Experto/
// Avanzado). Puntos como en B1 (sub 0x198E18 del ELF europeo): LIFE (vida del ganador - la del otro) + TIME
// (segundos que sobran x 20), la mitad para la leyenda; los de los golpes en combate de B1 no se cuentan.
// El resultado se lee sin ganchos: estado del combate mgr+35128 (4 pelea, 5 acabado, 6 pose, 7 menu final) y hueco
// del ganador mgr+35136 (0 = el jugador), mgr = *0x823D6CF4 (los de tagteam.cpp LabTick). Perder: reintentar o salir;
// el avance se guarda en user_data/dbz3/pasarela/progreso.toml.
// Sin ganchos REX_HOOK_RAW propios. US y EU (direcciones con su pareja EU: src/eu_map.h).
#include "pasarela.h"

#include <rex/hook.h>
#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/ui/image_decode.h>
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/immediate_drawer.h>

#include <imgui.h>
#include <toml++/toml.hpp>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "generated/dbz3_init.h"
#include "eu_map.h"
#include "ingame/game_style.h"
#include "launcher/i18n.h"
#include "launcher/launcher_state.h"
#include "launcher/settings.h"
#include "mods.h"

namespace dbz3::hub {
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

constexpr dbz3::GA kMgrPtr = 0x823D6CF4, kStages = 0x82323760, kCfg = 0x8245EFC0;   // escenarios: 17 x 92 B, +0x10 mapa (stage_ext.h)
constexpr int kMainScreen = 5, kJugador = 14;                    // Mr. Satan en B3
constexpr uint16_t kUp = 0x0001, kDown = 0x0002, kStart = 0x0010, kA = 0x1000, kB = 0x2000;
constexpr const char* kClave = "hercule";
constexpr float kAutoSeg = 4.0f;   // prueba (DBZ3_TEST_PASARELA=auto): segundos por pantalla
// Luchador X = mgr + 2096 + 8240 * lado: vida X+6080 (f32), maxima X+6084, ki X+6100, tope X+6104 (docs/15_tecnicas);
// mgr+35573 = nivel de la IA del COM (sub_820FC9B0 lo pone de las opciones o del nivel del Torneo; lo leen las tablas
// por personaje de sub_8213E6C8); mgr+2028 tiempo limite en cuadros, mgr+35144 cuadros pasados (f32, sub_820FD678).
constexpr uint32_t kX0 = 2096, kXPaso = 8240, kVida = 6080, kVidaMax = 6084, kKi = 6100, kKiMax = 6104;
constexpr uint32_t kNivelCom = 35573, kLimite = 2028, kPasado = 35144;
constexpr float kKiBarra = 1000.0f;   // ki de B3 por barra (medido: ver docs/16_pasarela_modos/01 s5)

int64_t NowMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
      .count();
}
uint8_t* Base() {
  rex::Runtime* rt = rex::Runtime::instance();
  return rt ? rt->virtual_membase() : nullptr;
}
float LoadF(uint8_t* base, uint32_t a) {
  const uint32_t u = REX_LOAD_U32(a);
  float f;
  std::memcpy(&f, &u, 4);
  return f;
}
void StoreF(uint8_t* base, uint32_t a, float f) {
  uint32_t u;
  std::memcpy(&u, &f, 4);
  REX_STORE_U32(a, u);
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
      REXLOG_INFO("dbz3 pasarela: {} leida en {} ms: ELF {} tabla 0x{:X}, {} combates, {} de la historia, charlas {} "
                  "({}, {} imagenes), resultado {}", d->iso, NowMs() - t0, d->elf, d->tabla, d->combates.size(),
                  d->historia.size(), d->afs.empty() ? "-" : d->afs, d->idioma, d->charlas.size(),
                  d->res.retadores ? "si" : "no");
    } else {
      REXLOG_INFO("dbz3 pasarela: leyenda de Mr. Satan no disponible ({})", error);
    }
    g_leyendo = false;
  }).detach();
}

// ---- Avance guardado: user_data/dbz3/pasarela/progreso.toml (clave = valor; una por modo) ----
std::mutex g_prog_mu;
std::filesystem::path Carpeta() { return dbz3::settings::UserDataRoot() / "pasarela"; }
std::string LeerFichero(const std::filesystem::path& p) {
  std::ifstream f(p, std::ios::binary);
  if (!f) return {};
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}
std::map<std::string, int64_t> LeerTodo() {   // con g_prog_mu
  std::map<std::string, int64_t> out;
  try {
    const toml::table t = toml::parse(LeerFichero(Carpeta() / "progreso.toml"));
    for (const auto& [k, v] : t) {
      if (const auto n = v.value<int64_t>()) out[std::string(k.str())] = *n;
    }
  } catch (const toml::parse_error&) {
  }
  return out;
}

// Nombres de B3 (los de tagteam_panel.cpp, en espanol e ingles como el resto de textos propios).
struct Nombre {
  int id;
  const char *es, *en;
};
constexpr Nombre kNombres[] = {
    {0, "Goku", "Goku"}, {2, "Gohan (niño)", "Kid Gohan"}, {3, "Gohan (adolescente)", "Teen Gohan"}, {4, "Gohan", "Gohan"},
    {5, "Gran Saiyaman", "Great Saiyaman"}, {6, "Goten", "Goten"}, {7, "Vegeta", "Vegeta"}, {8, "Trunks", "Trunks"},
    {9, "Trunks (niño)", "Kid Trunks"}, {10, "Krilin", "Krillin"}, {11, "Piccolo", "Piccolo"},
    {12, "Ten Shin Han", "Tien"}, {13, "Yamcha", "Yamcha"}, {14, "Mr. Satán", "Hercule"}, {18, "Raditz", "Raditz"},
    {19, "Nappa", "Nappa"}, {20, "Ginyu", "Ginyu"}, {21, "Recoome", "Recoome"}, {27, "Freezer", "Frieza"},
    {28, "Androide 16", "Android 16"}, {29, "Androide 17", "Android 17"}, {30, "Androide 18", "Android 18"},
    {32, "Dr. Gero", "Dr. Gero"}, {33, "Cell", "Cell"}, {34, "Majin Bu", "Majin Buu"}, {35, "Súper Bu", "Super Buu"},
    {36, "Bu (niño)", "Kid Buu"}, {38, "Cooler", "Cooler"}, {42, "Saibaman", "Saibaman"}, {43, "Cell Jr.", "Cell Jr."}};

// Personajes de mods: mods/_roster/roster.toml ("# <Nombre> (mod <carpeta>)" + "id = N", como tagteam_panel.cpp) y el
// origen de cada uno en mods/<carpeta>/personaje.toml (origen_fuente / origen_clave, los escribe importar.py).
struct Port {
  std::string nombre, carpeta, fuente, clave;
};
// Los trajes extra (traje.toml) salen como "# <NOMBRE>: traje extra (mod <carpeta>)" + "id = N".
struct Roster {
  std::map<int, Port> ports;
  std::map<int, std::vector<std::string>> trajes;   // ID -> carpetas, en el orden en que se anaden
};
std::mutex g_ports_mu;
Roster LeerRoster() {
  static Roster r;
  static std::filesystem::file_time_type sello{};
  static int64_t visto = -100000;
  std::lock_guard<std::mutex> lk(g_ports_mu);
  if (NowMs() - visto < 2000) return r;
  visto = NowMs();
  std::error_code ec;
  const auto fichero = dbz3::ModsRoot() / "_roster" / "roster.toml";
  const auto t = std::filesystem::last_write_time(fichero, ec);
  if (ec) {
    r = {};
    return r;
  }
  if (t == sello) return r;
  sello = t;
  r = {};
  std::ifstream in(fichero);
  Port pendiente;
  bool traje = false;
  for (std::string l; std::getline(in, l);) {
    if (!l.empty() && l.back() == '\r') l.pop_back();
    if (l.rfind("# ", 0) == 0) {
      const size_t at = l.rfind(" (mod ");
      if (at != std::string::npos && l.size() > at + 7 && l.back() == ')') {
        pendiente = {l.substr(2, at - 2), l.substr(at + 6, l.size() - at - 7), {}, {}};
        traje = pendiente.nombre.find(": traje extra") != std::string::npos;
      }
    } else if (l.rfind("id = ", 0) == 0 && !pendiente.carpeta.empty()) {
      const int id = std::atoi(l.c_str() + 5);
      if (traje) {
        r.trajes[id].push_back(pendiente.carpeta);
      } else {
        try {
          const toml::table p = toml::parse(LeerFichero(dbz3::ModsRoot() / pendiente.carpeta / "personaje.toml"));
          pendiente.fuente = p["personaje"]["origen_fuente"].value_or(std::string());
          pendiente.clave = p["personaje"]["origen_clave"].value_or(std::string());
        } catch (const toml::parse_error&) {
        }
        r.ports[id] = pendiente;
      }
      pendiente = {};
    }
  }
  return r;
}
std::map<int, Port> Ports() { return LeerRoster().ports; }

}  // namespace

std::shared_ptr<const DatosB1> DatosLeidos() {
  auto d = Datos();
  if (!d) Leer(false);
  return d;
}

int LeerProgreso(const std::string& clave, int defecto) {
  std::lock_guard<std::mutex> lk(g_prog_mu);
  const auto t = LeerTodo();
  auto it = t.find(clave);
  return it == t.end() ? defecto : int(it->second);
}

void GuardarProgreso(const std::string& clave, int valor) {
  std::lock_guard<std::mutex> lk(g_prog_mu);
  auto t = LeerTodo();
  t[clave] = valor;
  std::error_code ec;
  std::filesystem::create_directories(Carpeta(), ec);
  const auto tmp = Carpeta() / "progreso.toml.tmp";
  if (std::FILE* f = std::fopen(tmp.string().c_str(), "w")) {
    std::fprintf(f, "# Pasarela de modos: avance de cada modo (lo escribe el juego)\n");
    for (const auto& [k, v] : t) std::fprintf(f, "%s = %lld\n", k.c_str(), static_cast<long long>(v));
    std::fclose(f);
    std::filesystem::rename(tmp, Carpeta() / "progreso.toml", ec);
  }
}

int TrajeExtra(int id, const std::string& carpeta) {
  const auto r = LeerRoster();
  auto it = r.trajes.find(id);
  if (it == r.trajes.end() || id < 0 || id >= 105) return -1;
  const auto& v = it->second;
  const auto k = std::find(v.begin(), v.end(), carpeta);
  uint8_t* base = Base();
  if (k == v.end() || !base) return -1;
  // roster_ext.cpp AppendCostume: cada traje extra suma uno a los trajes del personaje (char96 +4)
  const int ahora = REX_LOAD_U8(dbz3::GA(0x8234ABB8u) + 96u * uint32_t(id) + 4);
  return ahora - int(v.size()) + int(k - v.begin());
}

int IdPort(const std::string& fuente, const std::string& clave) {
  for (const auto& [id, p] : Ports()) {
    if (p.fuente == fuente && p.clave == clave) return id;
  }
  return -1;
}

std::string NombreDe(int id) {
  for (const auto& n : kNombres) {
    if (n.id == id) return T::T(n.es, n.en);
  }
  const auto ports = Ports();
  if (auto it = ports.find(id); it != ports.end()) return it->second.nombre;
  return "#" + std::to_string(id);
}

int EscenarioPorMapa(int mapa) {
  uint8_t* base = Base();
  for (uint32_t i = 0; base && i < 17; ++i) {
    if (REX_LOAD_U32(kStages + 92 * i + 0x10) == uint32_t(mapa)) return int(i);
  }
  return 0;
}

// Escenario de B3 para un escenario de B1: el de los Juegos de Cell es el mapa 50 (CELL_RING_MAP).
int EscenarioB3(const std::string& nombre_b1) {
  int mapa = 50;
  std::string n = nombre_b1;
  if (n.rfind("CHG_", 0) == 0) n = n.substr(4);
  static const std::pair<const char*, int> kMapas[] = {{"CELL RING", 50}, {"NAMEC", 60},   {"RETTOU", 56},
                                                       {"SEISHIN", 58},   {"TENKAITI", 46}, {"BOKUSOUTI", 62},
                                                       {"IWAYAMA", 64},   {"SANKAN", 64},   {"TIKYUU", 68}};
  for (const auto& [k, m] : kMapas) {
    if (n.rfind(k, 0) == 0) mapa = m;
  }
  return EscenarioPorMapa(mapa);
}

// Forma de B1 (+0x52) -> forma inicial de B3 (rec+20), medida en el juego 2026-10-15: Goku 2 = SSJ (la 1 es el
// Kaioken), Gohan adolescente 1 = SSJ, Cell 2 = perfecto; Freezer: B1 1 = forma final (B3 3), 2 = al 100 % (B3 4).
// Piccolo (forma 2 en B1) no cambia en B3: se queda en 0. Los personajes de mods (Zarbon de B1) usan las suyas.
int FormaB3(int id, int forma_b1) {
  static constexpr int kFormas[][3] = {{0, 4, 2}, {3, 1, 1}, {33, 2, 2}, {27, 1, 3}, {27, 2, 4}};   // ID de B3, B1, B3
  for (const auto& f : kFormas) {
    if (f[0] == id && f[1] == forma_b1) return f[2];
  }
  if (Ports().count(id)) return forma_b1;
  return 0;
}

namespace {
std::atomic<int> g_estado_combate{0};
}  // namespace
int EstadoCombate() { return g_estado_combate.load(); }

void VigilarCombate(const char* quien, const Reglas& r, std::function<void(const Fin&)> fin) {
  std::thread([quien = std::string(quien), r, fin = std::move(fin)] {
    uint8_t* base = Base();
    bool visto = false;
    Fin f;
    uint32_t ultimo = 0;
    float vida_rival = -1, ki_jug = -1, anulado = 0, ki_perdido = 0;
    int64_t gasto_ki = -100000, gasto_ki_rival = -100000;
    float ki_riv = -1;
    const int64_t t0 = NowMs();
    // traje del jugador: el conector deja el 0 en la CFG (rec+18) y el juego la copia en la espera de la escena 24
    // (el fotograma siguiente): se escribe en cuanto el combate es nuestro. ponytail: carrera de un fotograma con el
    // hilo del juego; lo limpio es un parametro de traje en RequestBattleEx (propuesta en docs/14_frentes/campanas.md)
    // Capsulas: el conector deja las listas vacias (rec+52 = 0), asi nadie lanza tecnicas; como en el Duelo, cada
    // lado lleva la lista Normal de su personaje (tabla 0x8234ABB8 + 96 * ID: +80 cuantas, +82 7 IDs; la que el
    // Torneo pone al COM, docs/12_conectores/12_torneo_como_motor.md s4.1). Una lista ya puesta (DBZ3_TEST_CAPS) se
    // respeta.
    if (base) {
      while (!dbz3::hub::OwnBattle() && NowMs() - t0 < 3000) std::this_thread::yield();
      if (dbz3::hub::OwnBattle()) {
        if (r.traje_jugador > 0) REX_STORE_U16(kCfg + 16 + 18, uint16_t(r.traje_jugador));
        if (r.traje_rival > 0) REX_STORE_U16(kCfg + 96 + 18, uint16_t(r.traje_rival));
        std::string caps;
        for (uint32_t k = 0; r.capsulas && k < 2; ++k) {
          const uint32_t rec = kCfg + 16 + 80 * k, id = REX_LOAD_U16(rec + 16);
          // la lista Normal ya la pone el conector (hub.cpp); aqui solo la del rival a medida (capsulas_rival)
          const uint32_t tab = dbz3::GA(0x8234ABB8u) + 96u * id, propia = k == 1 ? uint32_t(r.capsulas_rival[0]) : 0;
          if ((REX_LOAD_U16(rec + 52) && !propia) || id >= 105) continue;
          const uint32_t n = std::min<uint32_t>(propia ? propia : REX_LOAD_U16(tab + 80), 7);
          REX_STORE_U16(rec + 52, uint16_t(n));
          for (uint32_t c = 0; c < 7; ++c) {
            const uint32_t cap = c >= n ? 0 : propia ? uint32_t(r.capsulas_rival[1 + c]) : REX_LOAD_U16(tab + 82 + 2 * c);
            REX_STORE_U16(rec + 54 + 2 * c, uint16_t(cap));
          }
          caps += " lado " + std::to_string(k + 1) + ": " + std::to_string(n);
        }
        REXLOG_INFO("dbz3 {}: traje del jugador {}, del rival {}; capsulas de serie{}", quien, r.traje_jugador,
                    r.traje_rival, caps.empty() ? " (ninguna)" : caps);
      }
    }
    float ki_lado[2] = {-1, -1};
    int forma_lado[2] = {-1, -1};
    int tecnicas = 0;
    bool empezado = false;   // reglas de inicio ya aplicadas en esta pelea
    int64_t traza_ms = 0;
    while (base) {
      const bool own = dbz3::hub::OwnBattle();
      visto = visto || own;
      const uint32_t mgr = own ? REX_LOAD_U32(kMgrPtr) : 0;
      const uint32_t st = mgr ? REX_LOAD_U32(mgr + 35128) : 0;
      const uint32_t x0 = mgr + kX0, x1 = x0 + kXPaso;
      if (mgr && st >= 1 && st <= 4) {
        f.empezo = true;
        if (r.nivel_com >= 0 && REX_LOAD_U8(mgr + kNivelCom) != uint8_t(r.nivel_com)) {
          REXLOG_INFO("dbz3 {}: nivel del COM {} -> {}", quien, REX_LOAD_U8(mgr + kNivelCom), r.nivel_com);
          REX_STORE_U8(mgr + kNivelCom, uint8_t(r.nivel_com));
        }
      }
      // al llegar al READY (o a la pelea sin el), con los luchadores ya puestos: ki inicial del rival; tambien en la
      // revancha
      if (mgr && (st == 3 || st == 4) && !empezado && LoadF(base, x1 + kVidaMax) > 0) {
        empezado = true;
        const float k0 = LoadF(base, x0 + kKi), k1 = LoadF(base, x1 + kKi), m1 = LoadF(base, x1 + kKiMax);
        if (r.ki_rival > 0) StoreF(base, x1 + kKi, std::min(m1 > 0 ? m1 : 1e9f, r.ki_rival * kKiBarra));
        REXLOG_INFO("dbz3 {}: ki al empezar {:.0f}/{:.0f} y {:.0f}/{:.0f} -> rival {:.0f}; vida {:.0f}/{:.0f} y "
                    "{:.0f}/{:.0f}; nivel del COM {}", quien, k0, LoadF(base, x0 + kKiMax), k1, m1,
                    LoadF(base, x1 + kKi), LoadF(base, x0 + kVida), LoadF(base, x0 + kVidaMax),
                    LoadF(base, x1 + kVida), LoadF(base, x1 + kVidaMax), REX_LOAD_U8(mgr + kNivelCom));
        vida_rival = LoadF(base, x1 + kVida);
        ki_jug = LoadF(base, x0 + kKi);
        {   // capsulas de cada luchador (registro de seleccion *(X+7412): +52 cuantas, +54 IDs)
          for (int k = 0; k < 2; ++k) {
            const uint32_t sel = REX_LOAD_U32(x0 + kXPaso * uint32_t(k) + 7412);
            std::string caps;
            const uint32_t n = sel ? REX_LOAD_U16(sel + 52) : 0;
            for (uint32_t c = 0; c < n && c < 7; ++c) caps += " " + std::to_string(REX_LOAD_U16(sel + 54 + 2 * c));
            REXLOG_INFO("dbz3 {}: capsulas del lado {}: {}{} (modo {})", quien, k + 1, n, caps, REX_LOAD_U16(mgr + 2018));
          }
        }
      }
      if (mgr && st == 4) {
        f.ganador = -1;   // pelea (tambien la revancha de "Battle Again?")
        if (r.traza && NowMs() - traza_ms >= 1000) {
          traza_ms = NowMs();
          REXLOG_INFO("dbz3 {}: traza: vida {:.0f} / {:.0f}, ki {:.0f} / {:.0f}", quien, LoadF(base, x0 + kVida),
                      LoadF(base, x1 + kVida), LoadF(base, x0 + kKi), LoadF(base, x1 + kKi));
        }
        for (int k = 0; k < 2; ++k) {   // formas (X+2980 indice de forma): transformaciones y absorciones
          const int fo = int(REX_LOAD_U32(x0 + kXPaso * uint32_t(k) + 2980));
          if (forma_lado[k] >= 0 && fo != forma_lado[k]) REXLOG_INFO("dbz3 {}: el lado {} pasa a la forma {}", quien, k + 1, fo);
          forma_lado[k] = fo;
        }
        // tecnicas (para las pruebas): de un fotograma a otro se gasta una barra de ki o mas (las rafagas gastan poco)
        for (int k = 0; k < 2; ++k) {
          const float ki = LoadF(base, x0 + kXPaso * uint32_t(k) + kKi);
          if (ki_lado[k] >= 0 && ki_lado[k] - ki >= 900.0f && tecnicas < 12) {
            ++tecnicas;
            REXLOG_INFO("dbz3 {}: el lado {} gasta {:.0f} de ki de golpe (tecnica)", quien, k + 1, ki_lado[k] - ki);
          }
          ki_lado[k] = ki;
        }
        // ki del jugador: lo que gasta (ki o tecnicas: ventana de 1,5 s para "solo ki") y lo que gana (ki lento)
        const float ki = LoadF(base, x0 + kKi);
        // gastar ki = bajar 30 o mas de un vistazo a otro (8 ms): rafagas y tecnicas; el modo hiper gasta poco a poco
        if (ki_jug >= 0 && ki < ki_jug - 30.0f) gasto_ki = NowMs();
        if (const float kr = LoadF(base, x1 + kKi); ki_riv >= 0 && kr < ki_riv - 30.0f) gasto_ki_rival = NowMs();
        ki_riv = LoadF(base, x1 + kKi);
        if (r.ki_lento > 0 && ki_jug >= 0 && ki > ki_jug) {
          const float quita = (ki - ki_jug) * r.ki_lento;
          StoreF(base, x0 + kKi, ki - quita);
          ki_perdido += quita;
        }
        ki_jug = LoadF(base, x0 + kKi);
        // dano al rival: armadura (se anula una parte) y, herido, el que no viene del ki (se anula el 75 %)
        const float v = LoadF(base, x1 + kVida);
        // retos de IW: golpes que quitan vida al rival (cada bajada cuenta uno). Al cumplir (o al fallar el de no
        // atacar) el combate acaba como un ring out: mgr+35569 = hueco del ganador + 1 (el aviso que pone el juego al
        // caer del ring; tagteam.cpp DBZ3_TEST_RINGOUT)
        // golpe basico del jugador: el rival pierde vida sin que el jugador acabe de gastar ki (rafaga, tecnica) ni el
        // rival (su rafaga devuelta al cubrirse)
        const int64_t ahora = NowMs();
        const bool basico = ahora - gasto_ki > 400 && ahora - gasto_ki_rival > 2000;
        if (vida_rival > 0 && v < vida_rival - 0.5f && basico) {
          ++f.golpes;
          if (r.contador) r.contador->store(f.golpes);
          if (r.sin_atacar && !f.ataco) {
            f.ataco = true;
            REX_STORE_U8(mgr + 35569, 2);
            REXLOG_INFO("dbz3 {}: reto: el jugador ha atacado (fallo): el rival pierde {:.0f} (ki gastado hace {} ms el "
                        "jugador, {} ms el rival; accion del jugador {}, del rival {})", quien, vida_rival - v,
                        ahora - gasto_ki, ahora - gasto_ki_rival, REX_LOAD_U16(x0 + 3244), REX_LOAD_U16(x1 + 3244));
          }
          if (r.golpes > 0 && f.golpes == r.golpes) {
            REX_STORE_U8(mgr + 35569, 1);
            REXLOG_INFO("dbz3 {}: reto: {} golpes, cumplido", quien, f.golpes);
          } else if (r.golpes > 0 && (f.golpes % 5) == 0) {
            REXLOG_INFO("dbz3 {}: reto: {} de {} golpes", quien, f.golpes, r.golpes);
          }
        }
        if (vida_rival > 0 && v < vida_rival && v > 0) {
          float parte = r.armadura_rival;
          if (r.solo_ki && NowMs() - gasto_ki > 1500) parte = std::max(parte, 0.75f);
          if (parte > 0) {
            const float vuelve = (vida_rival - v) * parte;
            StoreF(base, x1 + kVida, v + vuelve);
            anulado += vuelve;
          }
        }
        vida_rival = LoadF(base, x1 + kVida);
      }
      if (mgr && st >= 5 && st <= 7) {
        if (empezado && (anulado > 0 || ki_perdido > 0)) {   // al acabar (tambien si sigue una revancha)
          REXLOG_INFO("dbz3 {}: reglas: {:.0f} de dano anulado (armadura {:.2f}, solo ki {}), {:.0f} de ki perdido "
                      "(ki lento {:.2f})", quien, anulado, r.armadura_rival, r.solo_ki, ki_perdido, r.ki_lento);
          anulado = ki_perdido = 0;
        }
        empezado = false;
        f.ganador = int32_t(REX_LOAD_U32(mgr + 35136));
        for (int k = 0; k < 2; ++k) {
          f.vida[k] = int(LoadF(base, x0 + kXPaso * uint32_t(k) + kVida));
          f.vida_max[k] = int(LoadF(base, x0 + kXPaso * uint32_t(k) + kVidaMax));
        }
        const int32_t limite = int32_t(REX_LOAD_U32(mgr + kLimite));
        f.segundos = limite > 0 ? std::max(0, int((float(limite) - LoadF(base, mgr + kPasado)) / 60.0f)) : -1;
      }
      g_estado_combate = own ? int(st) : 0;
      if (own && st != ultimo) REXLOG_INFO("dbz3 {} estado del combate {} (ganador {})", quien, st, f.ganador);
      ultimo = st;
      if (visto && !own) break;
      if (!visto && NowMs() - t0 > 60000) {
        REXLOG_WARN("dbz3 {}: el combate no empezo en 60 s", quien);
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
    fin(f);
  }).detach();
}

namespace {
S::Pic CaraPic(int id) {   // hilo de dibujo
  if (const S::Pic f = S::Img(("caras/" + std::to_string(id)).c_str())) return f;
  static std::map<int, std::pair<std::string, std::unique_ptr<rex::ui::ImmediateTexture>>> cache;
  static std::map<int, S::Pic> pics;
  const auto ports = Ports();
  const auto it = ports.find(id);
  if (it == ports.end()) return {};
  auto& c = cache[id];
  if (c.first == it->second.carpeta) return pics[id];
  auto* drawer = dbz3::launcher::PreviewDrawer();
  if (!drawer) return {};
  c.first = it->second.carpeta;
  c.second.reset();
  pics[id] = {};
  const auto dir = dbz3::ModsRoot() / it->second.carpeta / "ui" / "_vista";
  for (const char* file : {"hud.png", "icono.png"}) {
    std::ifstream in(dir / file, std::ios::binary);
    if (!in) continue;
    const std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    int w = 0, h = 0;
    const std::vector<uint8_t> px = rex::ui::DecodeImageRGBA(data.data(), data.size(), w, h);
    if (px.empty()) continue;
    c.second = drawer->CreateTexture(uint32_t(w), uint32_t(h), rex::ui::ImmediateTextureFilter::kLinear, false,
                                     px.data());
    if (!c.second) continue;
    S::Pic p;
    p.tex = reinterpret_cast<ImTextureID>(c.second.get());
    p.w = float(w);
    p.h = float(h);
    pics[id] = p;
    break;
  }
  return pics[id];
}
}  // namespace

ImTextureID CaraDe(int id) { return CaraPic(id).tex; }

void DibujarCara(ImDrawList* dl, const S::Area& a, int id, float x0, float y0, float w, bool on, const char* nombre,
                 float t, bool espejo) {
  const float bob = on ? 4.0f * std::sin(t * 3.0f) : 0.0f;
  const ImVec2 p0 = a.At(x0, y0 + bob), p1 = a.At(x0 + w, y0 + w + bob);
  S::Frame(dl, p0, p1, a.s);
  if (const S::Pic f = CaraPic(id)) {
    const float k = std::min((p1.x - p0.x) / f.w, (p1.y - p0.y) / f.h) * 0.92f;
    const ImVec2 c((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f), h(f.w * k * 0.5f, f.h * k * 0.5f);
    S::DrawPic(dl, f, ImVec2(c.x - h.x, c.y - h.y), ImVec2(c.x + h.x, c.y + h.y), ImVec4(0, 0, 0, 0),
               on ? IM_COL32_WHITE : IM_COL32(120, 120, 130, 255), espejo);
  }
  if (nombre && *nombre) S::MenuText(dl, a.At(x0 + w * 0.5f, y0 + w + 42 + bob), 44 * a.s, nombre, on, false);
}

namespace {

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

// ---- Estado (g_mu) ----
enum Fase : int { kNada = 0, kPantalla, kLuchando, kPerdido, kFin, kResultado };
const char* const kFaseNombre[] = {"nada", "pantalla", "combate", "perdido", "fin", "resultado"};
const char* const kDificultad[] = {"Novato", "Experto", "Avanzado"};   // registro
const char* NombreDificultad(int k) {   // los nombres de B1 en espanol (las imagenes 666 #7-9 de la ISO espanola)
  return k == 0 ? T::T("Novel", "Novice") : k == 1 ? T::T("Avanzado", "Adept") : T::T("Experto", "Advanced");
}

std::mutex g_mu;
std::shared_ptr<const DatosB1> g_d;   // datos del modo en curso
std::vector<Paso> g_pasos;
int g_fase = kNada, g_paso = 0, g_intentos = 0;
int g_vida_ganada = 0;   // vida de Mr. Satan al ganar el ultimo combate (para los que no la recuperan)
int g_dificultad = 0;    // 0 Novato, 1 Experto, 2 Avanzado (nivel del COM)
int g_puntos = 0;        // de esta partida
int g_ronda_pts = 0;     // del ultimo retador (la pantalla del resultado)
int g_record = 0;
uint64_t g_cambio = 0;   // sube con cada pantalla nueva (el panel reinicia el texto)
std::atomic<int> g_fase_pub{kNada};
std::atomic<bool> g_auto{false};
std::atomic<bool> g_panel{false};

void Publicar(int fase) {   // con g_mu
  if (fase == kFin) {
    REXLOG_INFO("dbz3 pasarela: final de la leyenda ({} pasos, {} puntos, record {})", g_pasos.size(), g_puntos,
                g_record);
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
    GuardarProgreso(kClave, 0);
    GuardarProgreso("hercule_puntos", 0);
  } else {   // se vuelve a la charla (o la tarjeta) del combate pendiente
    int vuelta = std::min<int>(g_paso, int(g_pasos.size()) - 1);
    while (vuelta > 0 && g_pasos[size_t(vuelta)].tipo == kCombate && g_pasos[size_t(vuelta)].vez > 0) --vuelta;
    if (vuelta > 0 && g_pasos[size_t(vuelta)].tipo == kCombate && g_pasos[size_t(vuelta - 1)].tipo == kCharla) --vuelta;
    GuardarProgreso(kClave, vuelta);
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

// Puntos de un combate ganado como en B1: LIFE (vida del ganador - la del otro) + TIME (segundos que sobran x 20),
// la mitad para la leyenda (0x198F00: score / 2). ponytail: sin los puntos por golpe de B1 (HIT, COMBO FIN,
// THROW...: necesitan ganchos en el combate de B3).
int PuntosB1(const Fin& f) {
  const int life = std::max(0, f.vida[0] - f.vida[1]);
  const int time = f.segundos > 0 ? f.segundos * 20 : 0;
  return (life + time) / 2;
}

// Reglas que B1 tiene en codigo (satanmode.c) segun el retador: Androide 16 (B1 16) "dificil de danar", Trunks (5)
// "manos y piernas heridas: ataca con ki", Piccolo (6) "cuesta reunir ki". Ki inicial del rival: el de la tabla.
Reglas ReglasDe(const CombateB1& c, int dificultad) {
  Reglas r;
  r.nivel_com = dificultad;
  r.ki_rival = float(c.ki_rival) / 100.0f;
  if (c.rival_b1 == 16) r.armadura_rival = 0.5f;
  if (c.rival_b1 == 5) r.solo_ki = true;
  if (c.rival_b1 == 6) r.ki_lento = 0.5f;
  return r;
}

bool Lanzar() {   // con g_mu: el combate del paso actual, con las reglas del original
  const Paso& p = g_pasos[size_t(g_paso)];
  const CombateB1& c = g_d->combates[size_t(p.ronda)];
  const int escenario = EscenarioB3(c.escenario_nombre);
  // vida que no se recupera (Krilin, Gohan) y los Cell Jr. seguidos: la que le quedo al ganar el anterior
  const bool arrastra = c.arrastra_vida || p.vez > 0;
  const int vida1 = arrastra && g_vida_ganada > 0 ? -g_vida_ganada : 0;   // < 0: solo la inicial (maxima de serie, como B1)
  const int vida2 = c.vida_rival > 0 && c.vida_rival != 3000 ? c.vida_rival : 0;   // Cell Jr.: 1000
  const int tiempo = c.tiempo > 0 ? (c.tiempo + 30) / 60 : c.tiempo == -1 ? -1 : 0;  // cuadros -> s; -1 sin tiempo
  const int forma2 = FormaB3(c.rival, c.forma);
  if (!dbz3::hub::RequestBattleEx(kJugador, c.rival, escenario, true, tiempo, vida1, vida2, 0, forma2)) return false;
  const Reglas r = ReglasDe(c, g_dificultad);
  REXLOG_INFO("dbz3 pasarela: ronda {} ({}/{}): id {} contra id {} (B1 {}, forma B1 {} -> {}), escenario {} ({}), "
              "tiempo {}, vida {} / {}; reglas: {} (COM {}), ki del rival {:.1f} barras, armadura {:.2f}, solo ki {}, "
              "ki lento {:.2f}", p.ronda + 1, p.vez + 1, c.cantidad, kJugador, c.rival, c.rival_b1, c.forma, forma2,
              escenario, c.escenario_nombre, tiempo, vida1, vida2, kDificultad[g_dificultad], r.nivel_com, r.ki_rival,
              r.armadura_rival, r.solo_ki, r.ki_lento);
  Publicar(kLuchando);
  const uint64_t cambio = g_cambio;
  VigilarCombate(("pasarela: paso " + std::to_string(g_paso + 1)).c_str(), r, [cambio](const Fin& f) {
    std::lock_guard<std::mutex> lk(g_mu);
    if (g_fase != kLuchando || g_cambio != cambio) return;
    REXLOG_INFO("dbz3 pasarela: fin del combate (paso {}): gana el lado {}, vida de Mr. Satan {}", g_paso + 1,
                f.ganador + 1, f.vida[0]);
    if (f.ganador == 0) {
      const int pts = PuntosB1(f);
      g_vida_ganada = f.vida[0];
      g_puntos += pts;
      const Paso& p = g_pasos[size_t(g_paso)];
      const bool ultima_vez = p.vez + 1 >= std::max(1, g_d->combates[size_t(p.ronda)].cantidad);
      g_ronda_pts = (p.vez == 0 ? 0 : g_ronda_pts) + pts;
      REXLOG_INFO("dbz3 pasarela: puntos del combate {} (vida {} - {}, {} s), ronda {}, total {}", pts, f.vida[0],
                  f.vida[1], f.segundos, g_ronda_pts, g_puntos);
      GuardarProgreso(kClave, g_paso + 1);
      GuardarProgreso("hercule_puntos", g_puntos);
      if (ultima_vez) {
        if (g_paso + 1 >= int(g_pasos.size()) && g_puntos > g_record) {
          g_record = g_puntos;
          GuardarProgreso("hercule_record", g_record);
        }
        Publicar(kResultado);
      } else {
        Entrar(g_paso + 1);
      }
    } else {
      ++g_intentos;
      Publicar(kPerdido);
    }
  });
  return true;
}

void CreatePanel();

}  // namespace

// ---- API (HUB y pruebas) ----

bool Disponible() {
  if (!Datos()) Leer(false);
  return Datos() != nullptr;
}

const char* Motivo() {
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
  if (!d || d->combates.empty()) {
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
  int desde = LeerProgreso(kClave);
  if (desde < 0 || desde >= int(g_pasos.size())) desde = 0;
  g_dificultad = std::clamp(LeerProgreso("hercule_dificultad", 0), 0, 2);
  g_puntos = desde > 0 ? LeerProgreso("hercule_puntos", 0) : 0;
  g_record = LeerProgreso("hercule_record", 0);
  REXLOG_INFO("dbz3 pasarela: la leyenda de Mr. Satan ({} combates, {} pasos), desde el paso {}, {} ({} puntos)",
              d->combates.size(), g_pasos.size(), desde + 1, kDificultad[g_dificultad], g_puntos);
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
    const bool show = (fase == kPantalla || fase == kPerdido || fase == kFin || fase == kResultado) && menu &&
                      !dbz3::GameDimmed() && !dbz3::OverlayOpen();
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
    if (!show || g_auto.load()) pressed = 0;   // prueba automatica: solo avanza el modo (teclas de mas no cuentan)
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
    } else if (g_fase == kResultado) {
      DrawResultado(dl, a, t, pressed, auto_on);
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
    const ResultadoB1& r = g_d->res;
    subir(kTablon, r.tablon);
    subir(kRetadores, r.retadores);
    subir(kTotal, r.total);
    subir(kAhora, r.ahora);
    subir(kTienes, r.tienes);
    subir(kCifras, r.cifras);
    subir(kChibis, r.chibis);
    subir(kLetrero, r.letrero);
    for (int k = 0; k < 3; ++k) subir(kDif + k, r.dificultad[k]);
  }
  const Tex* Img(int key) const {
    auto it = tex_.find(key);
    return it != tex_.end() && it->second.tex ? &it->second : nullptr;
  }
  void Pic(ImDrawList* dl, const Tex& t, ImVec2 p0, ImVec2 p1, ImU32 tint = IM_COL32_WHITE, bool mirror = false,
           ImVec2 uv0 = ImVec2(0, 0), ImVec2 uv1 = ImVec2(1, 1)) {
    if (mirror) std::swap(uv0.x, uv1.x);
    dl->AddImage(reinterpret_cast<ImTextureID>(t.tex.get()), p0, p1, uv0, uv1, tint);
  }
  // Imagen centrada en c con alto h (px de 1200x900).
  void PicAt(ImDrawList* dl, const S::Area& a, const Tex& t, float cx, float cy, float h, ImU32 tint = IM_COL32_WHITE) {
    const float w = t.w * h / t.h;
    Pic(dl, t, a.At(cx - w * 0.5f, cy - h * 0.5f), a.At(cx + w * 0.5f, cy + h * 0.5f), tint);
  }
  // Recorte (px de la textura) centrado en (cx, cy) con escala k.
  void Trozo(ImDrawList* dl, const S::Area& a, const Tex& t, float x0, float y0, float x1, float y1, float cx, float cy,
             float k, ImU32 tint = IM_COL32_WHITE) {
    const float w = (x1 - x0) * k, h = (y1 - y0) * k;
    Pic(dl, t, a.At(cx - w * 0.5f, cy - h * 0.5f), a.At(cx + w * 0.5f, cy + h * 0.5f), tint, false,
        ImVec2(x0 / t.w, y0 / t.h), ImVec2(x1 / t.w, y1 / t.h));
  }
  // Numero con las cifras del resultado de B1 (5 x 2 de 48 x 64), alineado a la derecha en (x1, cy).
  void Numero(ImDrawList* dl, const S::Area& a, int n, float x1, float cy, float k) {
    const std::string s = std::to_string(std::max(0, n));
    if (const Tex* c = Img(kCifras)) {
      float x = x1;
      for (auto it = s.rbegin(); it != s.rend(); ++it) {
        const int d = *it - '0', cx = (d % 5) * 48, cy0 = (d / 5) * 64;
        Trozo(dl, a, *c, float(cx), float(cy0), float(cx + 48), float(cy0 + 64), x - 20 * k, cy, k);
        x -= 40 * k;
      }
      return;
    }
    S::MenuText(dl, a.At(x1 - 18.0f * float(s.size()) * k, cy), 52 * k * a.s, s.c_str(), true, false);
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
    Cara(dl, a, kJugador, 0, habla, t);
  }

  // Cara del marcador de B3 en un marco del juego.
  void Cara(ImDrawList* dl, const S::Area& a, int id, int lado, bool on, float t) {
    DibujarCara(dl, a, id, lado == 0 ? 120.0f : 800.0f, 190.0f, 280.0f, on, NombreDe(id).c_str(), t, lado == 1);
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

  // Titulo con la dificultad como el de B1: letrero de madera con Novato / Experto / Avanzado (arriba/abajo).
  void DrawPresentacion(ImDrawList* dl, const S::Area& a, float t, uint16_t pressed, bool auto_on) {
    Satan(dl, a, t, true);
    const bool elige = g_paso == 0;
    if (elige) {
      const float cx = 860, cy = 330;
      if (const Tex* l = Img(kLetrero)) PicAt(dl, a, *l, cx, cy, 300);
      else S::Frame(dl, a.At(cx - 210, cy - 140), a.At(cx + 210, cy + 140), a.s);
      for (int k = 0; k < 3; ++k) {
        const float y = cy - 80 + 80 * float(k);
        const bool sel = k == g_dificultad;
        if (sel) S::Highlight(dl, a.At(cx - 190, y - 30), a.At(cx + 190, y + 30), t);
        if (const Tex* d = Img(kDif + k)) PicAt(dl, a, *d, cx, y, sel ? 62.0f : 50.0f, sel ? IM_COL32_WHITE : IM_COL32(150, 150, 150, 255));
        else S::MenuText(dl, a.At(cx, y), 44 * a.s, NombreDificultad(k), sel, false);
      }
      if (g_record > 0) {
        char rec[64];
        std::snprintf(rec, sizeof(rec), T::T("Récord: %d puntos", "Record: %d points"), g_record);
        S::MenuText(dl, a.At(cx, cy + 175), 30 * a.s, rec, false, false);
      }
      if (pressed & kUp) g_dificultad = (g_dificultad + 2) % 3;
      if (pressed & kDown) g_dificultad = (g_dificultad + 1) % 3;
    }
    Bocadillo(dl, a, t, -1,
              T::T("¡Soy Mr. Satán, el campeón del mundo! En los Juegos de Cell me esperan once retadores. ¡Ja, ja, ja!",
                   "I'm Hercule, the world champion! Eleven challengers await me at the Cell Games. Ha ha ha!"),
              0.22f);
    if (elige) Ayuda(dl, a, {{"A", T::T("Empezar", "Start")}, {"B", T::T("Salir", "Quit")}});
    else Ayuda(dl, a, {{"A", T::T("Seguir", "Continue")}, {"B", T::T("Salir", "Quit")}});
    if (auto_on && t - cambio_at_ > kAutoSeg) pressed |= kA;
    if (pressed & kB) return Salir(false);
    if (pressed & (kA | kStart)) {
      if (elige) {
        GuardarProgreso("hercule_dificultad", g_dificultad);
        g_puntos = 0;
        GuardarProgreso("hercule_puntos", 0);
        REXLOG_INFO("dbz3 pasarela: dificultad {} (nivel del COM {})", kDificultad[g_dificultad], g_dificultad);
      }
      Entrar(g_paso + 1);
    }
  }

  void DrawCharla(ImDrawList* dl, const S::Area& a, float t, uint16_t pressed, bool auto_on) {
    const Paso& p = g_pasos[size_t(g_paso)];
    const CombateB1& c = g_d->combates[size_t(p.ronda)];
    Satan(dl, a, t, true);
    Cara(dl, a, c.rival, 1, false, t);
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
    Cara(dl, a, kJugador, 0, !perdido, t);
    Cara(dl, a, c.rival, 1, true, t);
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
    std::string texto = NombreDe(kJugador) + T::T(" contra ", " versus ") + NombreDe(c.rival) + ".";
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

  // Resultado tras cada retador, como el de B1 (entrada 668): "Challenger Nth", "Now!! X Pts.", "You have Y Pts.",
  // las caras de los 11 (los vencidos con su color, los demas en silueta) y "NEXT BATTLE".
  void DrawResultado(ImDrawList* dl, const S::Area& a, float t, uint16_t pressed, bool auto_on) {
    const Paso& p = g_pasos[size_t(g_paso)];
    const int ronda = p.ronda, n = int(g_d->combates.size());
    const float in = Ease((t - cambio_at_) / 0.35f);
    // tablon con el logo arriba; en medio "Challenger Nth"; debajo los puntos (como el resultado de B1)
    if (const Tex* tb = Img(kTablon)) {
      Trozo(dl, a, *tb, 0, 0, tb->w, 262, 600, 255 + 20 * (1 - in), 1.25f);
    } else {
      S::Frame(dl, a.At(240, 110), a.At(960, 400), a.s);
    }
    if (const Tex* r = Img(kRetadores); r && ronda < 11) {
      const float cx = float((ronda % 3) * 136), cy = float((ronda / 3) * 72);
      Trozo(dl, a, *r, cx, cy, cx + 136, cy + 72, 600, 300, 1.5f);
    } else {
      char tit[64];
      std::snprintf(tit, sizeof(tit), T::T("Retador %d", "Challenger %d"), ronda + 1);
      S::MenuText(dl, a.At(600, 300), 54 * a.s, tit, true, false);
    }
    // puntos: los de este retador ("Now!!") y los de la partida ("You have")
    if (const Tex* ah = Img(kAhora)) Trozo(dl, a, *ah, 0, 0, ah->w, ah->h, 400, 455, 1.0f);
    else S::MenuText(dl, a.At(320, 455), 40 * a.s, T::T("¡Ahora!", "Now!!"), true, false);
    Numero(dl, a, g_ronda_pts, 520, 460, 0.9f);
    if (const Tex* ti = Img(kTienes)) Trozo(dl, a, *ti, 0, 0, ti->w, ti->h, 800, 455, 1.0f);
    else S::MenuText(dl, a.At(720, 455), 40 * a.s, T::T("Tienes", "You have"), true, false);
    Numero(dl, a, g_puntos, 920, 460, 0.9f);
    // caras de los retadores (fila de B1): vencidos en color, el resto en silueta (segunda mitad del atlas)
    if (const Tex* ch = Img(kChibis)) {
      for (int k = 0; k < std::min(n, 11); ++k) {
        const int i = k <= ronda ? k : k + 10;   // 0-10 color; siluetas del 2.o al 11.o: 11-20
        const float sx = float((i % 7) * 72), sy = float((i / 7) * 72);
        const float x = 600 + (float(k) - 5.0f) * 86;
        Trozo(dl, a, *ch, sx, sy, sx + 72, sy + 72, x, 585, 1.0f);
      }
    }
    const bool fin = g_paso + 1 >= int(g_pasos.size());
    S::Speech(dl, a.At(60, 640), a.At(1140, 830), a.s,
              fin ? T::T("¡El último! ¡El campeón del mundo lo ha conseguido!", "The last one! The world champion did it!")
                  : T::T("¡Ja, ja, ja! ¡Otro más que cae ante el campeón del mundo!",
                         "Ha ha ha! Another one falls before the world champion!"),
              -1);
    Ayuda(dl, a, {{"A", fin ? T::T("Seguir", "Continue") : T::T("Siguiente combate", "Next battle")}});
    if (auto_on && t - cambio_at_ > kAutoSeg) pressed |= kA;
    if (pressed & (kA | kStart)) Entrar(g_paso + 1);
  }

  void DrawFin(ImDrawList* dl, const S::Area& a, float t, uint16_t pressed, bool auto_on) {
    Satan(dl, a, t, true);
    if (const Tex* to = Img(kTotal)) {
      PicAt(dl, a, *to, 860, 330, 360);
      Numero(dl, a, g_puntos, 935, 428, 0.8f);
    } else {
      char tot[64];
      std::snprintf(tot, sizeof(tot), T::T("Total: %d puntos", "Total: %d points"), g_puntos);
      S::MenuText(dl, a.At(860, 330), 44 * a.s, tot, true, false);
    }
    char rec[96];
    std::snprintf(rec, sizeof(rec), T::T("%s - Récord: %d", "%s - Record: %d"), NombreDificultad(g_dificultad), g_record);
    S::MenuText(dl, a.At(860, 560), 32 * a.s, rec, false, false);
    Bocadillo(dl, a, t, -1,
              T::T("¡Once retadores y ni una derrota! ¡El campeón del mundo lo ha vuelto a hacer! ¡Leyenda completada!",
                   "Eleven challengers and not one defeat! The world champion did it again! Legend complete!"),
              0.22f);
    Ayuda(dl, a, {{"A", T::T("Volver al HUB", "Back to the HUB")}});
    if (auto_on && t - cambio_at_ > kAutoSeg) pressed |= kA;
    if (pressed & kA) Salir(true);
  }

  enum : int { kLogo = 100, kFondo, kSatan, kTablon, kRetadores, kTotal, kAhora, kTienes, kCifras, kChibis, kLetrero,
               kDif };
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

// PRUEBA: DBZ3_TEST_PASARELA=1 (o "auto": avanza solo; "auto,desde=N": empieza en el paso N; ",dificultad=D")
// empieza el modo cuando el menu principal lleva 3 s en pantalla y escribe el avance en el log ("dbz3 pasarela: ...").
const bool g_test = [] {
  const char* t = std::getenv("DBZ3_TEST_PASARELA");
  if (!t) return false;
  g_auto = std::strstr(t, "auto") != nullptr;
  int desde = -1, dif = -1;
  if (const char* d = std::strstr(t, "desde=")) desde = std::atoi(d + 6);
  if (const char* d = std::strstr(t, "dificultad=")) dif = std::atoi(d + 11);
  std::thread([desde, dif] {
    for (int quieto = 0; quieto < 120;) {
      quieto = dbz3::ingame::GameScreen() == kMainScreen ? quieto + 1 : 0;
      std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    Leer(true);
    for (int i = 0; i < 400 && g_leyendo.load(); ++i) std::this_thread::sleep_for(std::chrono::milliseconds(25));
    if (desde >= 0) GuardarProgreso(kClave, desde);
    if (dif >= 0) GuardarProgreso("hercule_dificultad", dif);
    REXLOG_INFO("dbz3 pasarela: prueba: disponible {}; {}", Disponible(), Start() ? "empieza" : "no empieza");
  }).detach();
  return true;
}();

}  // namespace
}  // namespace dbz3::pasarela
