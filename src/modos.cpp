// dbz3 - Modos nativos: Torre, Supervivencia y Kumite (combates seguidos con rivales elegidos), torneo local
// con bots (lista propia de participantes en el Torneo Mundial), ranking Elo y records guardados.
//
// Retos: cada combate se pide con dbz3::hub::RequestBattle (escena 25 con la CFG 0x8245EFC0, hub.cpp). La CFG
// se completa justo antes de que el juego la use (espera de la escena 24, sub_821D7400: traje y vida de los
// dos lados) y el resultado se lee al acabar (sub_820FEBA8, antes de que el juego suelte el combate: ganador
// mgr+35136, vida del jugador X+6080). Vida en el registro de 80 B (sub_82109638, leido en el codigo):
// rec+36 = vida con la que empieza, rec+40 = maxima (se usa la mayor de las dos).
// Tras ganar no sale el menu del final (se elige solo "Reselect", sub_820FF750) y se pasa al combate siguiente;
// tras perder sale (Battle Again? = reintentar). Salir desde la pausa guarda el reto: se sigue en ese combate.
// Entre combates el juego vuelve al menu principal: encima, una pantalla propia con el resultado. Entradas
// "torre", "supervivencia", "kumite" y "ranking" del HUB (hub_panel.h, RegisterMode). Docs: docs/14_frentes/modos.md.
// Datos: user_data/dbz3/hub_modos.toml (records por modo y personaje, retos a medias; .bak = la version anterior)
// y retos.csv (historial). Ranking: Elo por personaje desde combates.csv (lo escribe LabTick en tagteam.cpp).
// Torneo: tras el sorteo nativo (sub_821A28A8) los COM pasan a ser los de la lista; un ID sin casilla propia
// en la rueda va en la casilla de su anfitrion y su ID se pone al preparar cada combate (sub_821A5A30).
// Solo US (direcciones EU sin medir): con g_guest_eu todo queda apagado.
#include <imgui.h>

#include <rex/hook.h>
#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/ui/imgui_dialog.h>

#include <toml++/toml.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "generated/dbz3_init.h"
#include "guest_region.h"
#include "ingame/game_style.h"
#include "ingame/hub_panel.h"
#include "launcher/i18n.h"
#include "launcher/launcher_state.h"
#include "launcher/settings.h"
#include "mods.h"
#include "roster_ext.h"

namespace dbz3 {
bool GameDimmed();
bool OverlayOpen();
}  // namespace dbz3
namespace dbz3::hub {
bool RequestBattle(int id1, int id2, int stage, bool p2_com);   // hub.cpp
bool OwnBattle();
}  // namespace dbz3::hub
namespace dbz3::ingame {
int GameScreen();
}

namespace dbz3::modos {

enum Tipo : int { kTorre = 0, kSupervivencia = 1, kKumite = 2 };
enum Fase : int { kNada = 0, kCombate = 1, kEntre = 2, kFin = 3 };
enum Resultado : int { kNinguno = -1, kDerrota = 0, kVictoria = 1, kAbandono = 2, kGuardado = 3 };

// Estado para el HUB (copiar tal cual; solo enteros).
struct Estado {
  int fase = kNada, tipo = kTorre;
  int ganados = 0, total = 0;   // combates ganados / del reto
  int rival = -1;               // ID del rival en curso o del siguiente (al terminar, el ultimo)
  int resultado = kNinguno;     // del ultimo combate; al terminar, el del reto
  int vida = 0, vida_max = 0;   // del jugador para el combate siguiente
  int jugador = 0;
};
// Puesto del ranking (Elo por personaje).
struct Puesto {
  int id = 0, elo = 1000, ganados = 0, perdidos = 0, empates = 0;
};

namespace {
namespace T = dbz3::i18n;
namespace S = dbz3::ingame::style;

constexpr uint32_t kGame = 0x824BA110, kCfg = 0x8245EFC0, kReqCur = 0x8247A990, kMgrPtr = 0x823D6CF4,
                   kResult = 0x823D6CE0;
constexpr uint32_t kHud = 0x82324064;   // marcador del combate (su menu: +4272, +4292)
constexpr uint32_t kWt = 0x8245F270, kWtScreen = 0x824681D0, kSlotToId = 0x82020618, kIdToSlot = 0x82020668;
constexpr int kVidaNormal = 3000, kVidaTope = 7000;   // CFG por defecto: 3000; 7000 = rec+48 (tope de barras)
const char* const kNombres[] = {"torre", "supervivencia", "kumite"};
const char* const kResNombres[] = {"derrota", "victoria", "abandono", "guardado"};

int64_t NowMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

int TipoDe(const std::string& s) {
  for (int k = 0; k < 3; ++k) {
    if (s == kNombres[k]) return k;
  }
  return -1;
}

struct Combate {
  int id = 0, traje = 0, escenario = 0;
  int vida = 0;              // del rival (0 = la normal)
  int tiempo = -1;           // limite en segundos (0 = sin limite; -1 = el del reto)
  int regen = -1;            // vida por segundo que recupera el rival (-1 = la del reto)
  int objetivo = -1;         // aguantar estos segundos = victoria (-1 = el del reto)
};
struct Reto {
  std::string nombre, name;  // nombre del reto (catalogo); vacio = el del modo
  int tiempo = 0, regen = 0, objetivo = 0;   // para todos sus combates (cada combate puede cambiarlos)
  int vida = kVidaNormal;    // vida maxima del jugador
  int vida_inicial = 0;      // del primer combate (0 = la maxima)
  bool arrastra = false;     // la vida que le queda pasa al combate siguiente
  int recupera = 0;          // vida que recupera entre combates (si arrastra)
  bool pausa = false;        // entre combates espera a Continue() (pantalla de resultados del HUB)
  std::vector<Combate> combates;
};

// Escenarios del Duelo (indice de la CFG +340) por los que van rotando los retos por defecto.
constexpr int kEscenarios[] = {0, 1, 2, 3, 4, 5, 6, 7};

Reto PorDefecto(int tipo) {
  Reto r;
  auto add = [&r](std::initializer_list<int> ids, int vida0, int paso) {
    int k = 0;
    for (const int id : ids) {
      r.combates.push_back({id, 0, kEscenarios[size_t(k) % std::size(kEscenarios)], vida0 + paso * k});
      ++k;
    }
  };
  if (tipo == kTorre) {   // 8 pisos: de Saibaman a Kid Buu, cada rival con mas vida
    add({42, 18, 19, 21, 20, 27, 33, 36}, 2000, 300);
  } else if (tipo == kSupervivencia) {   // 10 oleadas: la vida se arrastra, +500 entre oleadas
    r.arrastra = true;
    r.recupera = 500;
    add({42, 43, 29, 28, 30, 38, 37, 35, 40, 41}, 1500, 150);
  } else {   // kumite: 5 jefes con mucha vida; la del jugador se arrastra, +1000 entre jefes
    r.arrastra = true;
    r.recupera = 1000;
    add({27, 33, 38, 40, 36}, 5000, 500);
  }
  return r;
}

std::string LeerTexto(const std::filesystem::path& p) {
  std::ifstream f(p, std::ios::binary);
  if (!f) return {};
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

// user_data/dbz3/modos/<tipo>.toml (modos_reto.py nativo); sin el, el reto por defecto.
Reto Cargar(int tipo) {
  Reto r = PorDefecto(tipo);
  const auto path = dbz3::settings::UserDataRoot() / "modos" / (std::string(kNombres[tipo]) + ".toml");
  const std::string text = LeerTexto(path);
  if (text.empty()) return r;
  try {
    const toml::table t = toml::parse(text);
    r.vida = std::clamp(int(t["vida"].value_or(int64_t(r.vida))), 1, kVidaTope);
    r.vida_inicial = int(t["vida_inicial"].value_or(int64_t(0)));
    r.arrastra = t["arrastra_vida"].value_or(r.arrastra);
    r.recupera = int(t["recupera"].value_or(int64_t(r.recupera)));
    r.pausa = t["pausa"].value_or(false);
    r.nombre = t["nombre"].value_or(std::string());
    r.name = t["name"].value_or(std::string());
    r.tiempo = std::clamp(int(t["tiempo"].value_or(int64_t(0))), 0, 999);
    r.regen = std::clamp(int(t["regen"].value_or(int64_t(0))), 0, 1000);
    r.objetivo = std::clamp(int(t["objetivo_tiempo"].value_or(int64_t(0))), 0, 999);
    std::vector<Combate> cs;
    if (const toml::array* arr = t["combate"].as_array()) {
      for (const toml::node& n : *arr) {
        const toml::table* c = n.as_table();
        if (!c) continue;
        Combate x;
        x.id = int((*c)["rival"].value_or(int64_t(-1)));
        x.traje = int((*c)["traje"].value_or(int64_t(0)));
        x.escenario = int((*c)["escenario"].value_or(int64_t(0)));
        x.vida = std::clamp(int((*c)["vida"].value_or(int64_t(0))), 0, kVidaTope);
        x.tiempo = std::clamp(int((*c)["tiempo"].value_or(int64_t(-1))), -1, 999);
        x.regen = std::clamp(int((*c)["regen"].value_or(int64_t(-1))), -1, 1000);
        x.objetivo = std::clamp(int((*c)["objetivo_tiempo"].value_or(int64_t(-1))), -1, 999);
        if (x.id >= 0 && x.id < 64) cs.push_back(x);
      }
    }
    if (!cs.empty()) r.combates = std::move(cs);
    REXLOG_INFO("dbz3 modos: {} desde {} ({} combates)", kNombres[tipo], path.string(), r.combates.size());
  } catch (const toml::parse_error& e) {
    REXLOG_WARN("dbz3 modos: {} no se puede leer ({}): reto por defecto", path.string(), e.description());
  }
  return r;
}

std::mutex g_mu;   // g_reto, g_st, g_traje, g_marcas, g_curso, g_torneo
Reto g_reto;
Estado g_st;
int g_traje = 0;
std::atomic<uint8_t*> g_base{nullptr};   // memoria del guest (la de los ganchos)
std::atomic<int64_t> g_lanzado_ms{0};    // ultimo combate pedido
std::atomic<bool> g_fin_visible{false};  // pantalla del final del reto (hasta pulsar A)
std::atomic<bool> g_tras_victoria{false};  // el combate pedido sigue a uno ganado (pantalla entre combates)

uint8_t* Base() {
  if (uint8_t* b = g_base.load()) return b;
  rex::Runtime* rt = rex::Runtime::instance();
  return rt ? rt->virtual_membase() : nullptr;
}

// Se puede pelear con el: los de serie (sin los recortados 22-26 y 31) o un ID con mod de roster.
bool Jugable(int id) {
  if (id < 0 || id >= 64) return false;
  if (dbz3::roster::DonorOf(uint32_t(id)) >= 0) return true;
  return id < 44 && (id < 22 || id > 26) && id != 31;
}

const Combate& Actual() { return g_reto.combates[size_t(g_st.ganados)]; }   // con g_mu
int Objetivo() { return Actual().objetivo >= 0 ? Actual().objetivo : g_reto.objetivo; }
int Tiempo() { return Objetivo() > 0 ? Objetivo() : Actual().tiempo >= 0 ? Actual().tiempo : g_reto.tiempo; }
int Regen() { return Actual().regen >= 0 ? Actual().regen : g_reto.regen; }
// Gana el jugador: el lado 1, o con objetivo de tiempo, acabar por tiempo (motivo 3) sin caer.
bool Gana(int winner, int reason) { return winner == 0 || (Objetivo() > 0 && reason == 3); }

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

// ---- Datos guardados: user_data/dbz3/hub_modos.toml ----
// El juego solo tiene una partida (ranura 00000001 del perfil): el fichero va junto a ella en UserDataRoot,
// como historias_extra.bin. Escritura atomica (.tmp + rename) y la version anterior queda en .bak.
struct Marca {
  int tipo = 0, id = 0;
  std::string reto;                   // nombre del reto puesto (vacio = el de serie): otro reto, marca nueva
  int mejor = 0, total = 0;           // mas combates ganados seguidos (de total)
  int intentos = 0, completados = 0;
};
struct EnCurso {   // reto a medias (salida desde la pausa o juego cerrado): se sigue desde aqui
  int tipo = 0, id = 0, traje = 0, ganados = 0, vida = 0, total = 0;
};
// ---- Laboratorio: cada combate deja su resumen (y, si se pide, su captura por fotograma) ----
struct Lado {
  int id = -1;
  float dano = 0;                   // dano hecho al otro
  int golpes = 0;                   // veces que le bajo la vida (bajadas separadas por un fotograma sin bajar)
  int combo = 0, combo_mejor = 0;   // golpes seguidos (menos de 1 s entre uno y otro)
  float combo_dano = 0, combo_mejor_dano = 0;
  int ultimo = -1000;               // fotograma del ultimo golpe
  float vida_prev = -1;
  bool bajando = false;
};
struct Lab {
  uint32_t mgr = 0;
  int cuadros = 0;
  std::array<Lado, 2> lado;
} g_lab;                            // con g_mu
std::vector<Marca> g_marcas;
std::vector<EnCurso> g_curso;
bool g_datos_leidos = false;
bool g_lab_captura = false;   // laboratorio: captura por fotograma (user_data/dbz3/laboratorio.csv)

std::filesystem::path DatosPath() { return dbz3::settings::UserDataRoot() / "hub_modos.toml"; }

bool LeerDatos(const std::filesystem::path& p) {
  const std::string text = LeerTexto(p);
  if (text.empty()) return false;
  try {
    const toml::table t = toml::parse(text);
    if (t["version"].value_or(int64_t(0)) != 1) return false;
    std::vector<Marca> m;
    std::vector<EnCurso> c;
    if (const toml::array* a = t["marca"].as_array()) {
      for (const toml::node& n : *a) {
        const toml::table* x = n.as_table();
        if (!x) continue;
        Marca r;
        r.tipo = TipoDe((*x)["tipo"].value_or(std::string()));
        r.id = int((*x)["id"].value_or(int64_t(-1)));
        r.mejor = int((*x)["mejor"].value_or(int64_t(0)));
        r.total = int((*x)["total"].value_or(int64_t(0)));
        r.intentos = int((*x)["intentos"].value_or(int64_t(0)));
        r.completados = int((*x)["completados"].value_or(int64_t(0)));
        r.reto = (*x)["reto"].value_or(std::string());
        if (r.tipo >= 0 && r.id >= 0 && r.id < 64) m.push_back(r);
      }
    }
    if (const toml::array* a = t["en_curso"].as_array()) {
      for (const toml::node& n : *a) {
        const toml::table* x = n.as_table();
        if (!x) continue;
        EnCurso r;
        r.tipo = TipoDe((*x)["tipo"].value_or(std::string()));
        r.id = int((*x)["id"].value_or(int64_t(-1)));
        r.traje = int((*x)["traje"].value_or(int64_t(0)));
        r.ganados = int((*x)["ganados"].value_or(int64_t(0)));
        r.vida = int((*x)["vida"].value_or(int64_t(0)));
        r.total = int((*x)["total"].value_or(int64_t(0)));
        if (r.tipo >= 0 && r.id >= 0 && r.id < 64) c.push_back(r);
      }
    }
    g_lab_captura = t["laboratorio"].value_or(false);
    if (const toml::table* u = t["ultimo_combate"].as_table()) {   // resumen del ultimo combate (laboratorio)
      g_lab = {};
      g_lab.cuadros = int((*u)["cuadros"].value_or(int64_t(0)));
      for (size_t k = 0; k < 2; ++k) {
        Lado& l = g_lab.lado[k];
        auto num = [&](const char* clave) { return (*u)[clave][k].value_or(0.0); };
        l.id = int(num("id"));
        l.dano = float(num("dano"));
        l.golpes = int(num("golpes"));
        l.combo_mejor = int(num("combo"));
      }
    }
    g_marcas = std::move(m);
    g_curso = std::move(c);
    return true;
  } catch (const toml::parse_error&) {
    return false;
  }
}

void CargarDatos() {   // con g_mu, una vez
  if (g_datos_leidos) return;
  g_datos_leidos = true;
  const auto p = DatosPath();
  if (LeerDatos(p)) {
    REXLOG_INFO("dbz3 modos: datos: {} records y {} retos a medias ({})", g_marcas.size(), g_curso.size(), p.string());
  } else if (LeerDatos(p.string() + ".bak")) {
    REXLOG_WARN("dbz3 modos: datos: {} sin leer: se usa la copia de seguridad", p.string());
  }
}

void GuardarDatos() {   // con g_mu
  std::ostringstream o;
  o << "# Records y retos a medias de los modos del HUB (src/modos.cpp). No editar con el juego abierto.\n"
       "version = 1\n";
  o << "laboratorio = " << (g_lab_captura ? "true" : "false") << "\n";
  if (g_lab.cuadros > 0) {
    const auto& l = g_lab.lado;
    o << "\n[ultimo_combate]\ncuadros = " << g_lab.cuadros << "\nid = [" << l[0].id << ", " << l[1].id
      << "]\ndano = [" << int(l[0].dano) << ", " << int(l[1].dano) << "]\ngolpes = [" << l[0].golpes << ", "
      << l[1].golpes << "]\ncombo = [" << l[0].combo_mejor << ", " << l[1].combo_mejor << "]\n";
  }
  for (const Marca& m : g_marcas) {
    o << "\n[[marca]]\ntipo = \"" << kNombres[m.tipo] << "\"\nid = " << m.id << "\nmejor = " << m.mejor
      << "\ntotal = " << m.total << "\nintentos = " << m.intentos << "\ncompletados = " << m.completados
      << "\nreto = " << toml::value<std::string>(m.reto) << "\n";
  }
  for (const EnCurso& c : g_curso) {
    o << "\n[[en_curso]]\ntipo = \"" << kNombres[c.tipo] << "\"\nid = " << c.id << "\ntraje = " << c.traje
      << "\nganados = " << c.ganados << "\nvida = " << c.vida << "\ntotal = " << c.total << "\n";
  }
  std::error_code ec;
  const auto p = DatosPath();
  std::filesystem::create_directories(p.parent_path(), ec);
  const std::filesystem::path tmp = p.string() + ".tmp", bak = p.string() + ".bak";
  {
    std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
    f << o.str();
    if (!f.flush()) {
      REXLOG_WARN("dbz3 modos: datos: no se puede escribir {}", tmp.string());
      return;
    }
  }
  if (std::filesystem::exists(p, ec)) std::filesystem::copy_file(p, bak, std::filesystem::copy_options::overwrite_existing, ec);
  std::filesystem::rename(tmp, p, ec);
  if (ec) REXLOG_WARN("dbz3 modos: datos: no se puede guardar {} ({})", p.string(), ec.message());
}

// Otro reto puesto (otro nombre u otra lista de combates): la marca de ese modo y personaje empieza de nuevo.
void NuevaMarca(Marca& m) {   // con g_mu
  const std::string reto = g_reto.nombre.empty() ? g_reto.name : g_reto.nombre;
  if (m.total == g_st.total && m.reto == reto) return;
  m.mejor = m.completados = 0;
  m.total = g_st.total;
  m.reto = reto;
}

Marca& MarcaDe(int tipo, int id) {   // con g_mu
  for (Marca& m : g_marcas) {
    if (m.tipo == tipo && m.id == id) return m;
  }
  g_marcas.push_back({tipo, id});
  return g_marcas.back();
}
EnCurso* CursoDe(int tipo) {   // con g_mu
  for (EnCurso& c : g_curso) {
    if (c.tipo == tipo) return &c;
  }
  return nullptr;
}
void QuitarCurso(int tipo) {
  std::erase_if(g_curso, [tipo](const EnCurso& c) { return c.tipo == tipo; });
}
void AnotarCurso() {   // con g_mu: el reto en curso, tal como empieza su combate siguiente
  EnCurso* c = CursoDe(g_st.tipo);
  if (!c) {
    g_curso.push_back({g_st.tipo});
    c = &g_curso.back();
  }
  *c = {g_st.tipo, g_st.jugador, g_traje, g_st.ganados, g_st.vida, g_st.total};
}
void AnotarMarca(bool completado) {   // con g_mu
  Marca& m = MarcaDe(g_st.tipo, g_st.jugador);
  NuevaMarca(m);
  m.mejor = std::max(m.mejor, g_st.ganados);
  if (completado) ++m.completados;
}

void AnotarFin() {   // una linea por reto en user_data/dbz3/retos.csv
  std::error_code ec;
  const auto dir = dbz3::settings::UserDataRoot();
  std::filesystem::create_directories(dir, ec);
  const auto path = dir / "retos.csv";
  const bool fresh = !std::filesystem::exists(path, ec);
  if (std::FILE* f = std::fopen(path.string().c_str(), "a")) {
    if (fresh) std::fputs("fecha,tipo,jugador,ganados,total,resultado\n", f);
    const std::time_t now = std::time(nullptr);
    char when[32] = {};
    std::strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    std::fprintf(f, "%s,%s,%d,%d,%d,%s\n", when, kNombres[g_st.tipo], g_st.jugador, g_st.ganados, g_st.total,
                 kResNombres[g_st.resultado]);
    std::fclose(f);
  }
}

// Fin del reto (con g_mu). kGuardado: se sigue en el combate g_st.ganados la proxima vez.
void Terminar(int res) {
  g_st.fase = kFin;
  g_st.resultado = res;
  AnotarMarca(res == kVictoria);
  if (res == kGuardado) AnotarCurso();
  else QuitarCurso(g_st.tipo);
  GuardarDatos();
  AnotarFin();
  g_fin_visible = true;
  REXLOG_INFO("dbz3 modos: {} terminado: {} ({} de {} combates ganados)", kNombres[g_st.tipo], kResNombres[res],
              g_st.ganados, g_st.total);
}

bool Lanzar() {   // con g_mu: pide el combate g_st.ganados
  const Combate& c = g_reto.combates[size_t(g_st.ganados)];
  if (!dbz3::hub::RequestBattle(g_st.jugador, c.id, c.escenario, true)) return false;
  g_st.fase = kCombate;
  g_st.rival = c.id;
  g_lanzado_ms = NowMs();
  AnotarCurso();   // si el juego se cierra a mitad, se sigue en este combate
  GuardarDatos();
  REXLOG_INFO("dbz3 modos: {} {}/{}: id {} contra id {} (escenario {}, vida {}/{})", kNombres[g_st.tipo],
              g_st.ganados + 1, g_st.total, g_st.jugador, c.id, c.escenario, g_st.vida, g_st.vida_max);
  return true;
}

// Siguiente combate cuando el menu principal lleva ~2,5 s en pantalla (la de resultados encima; el conector
// pide la pantalla 5 activa).
void SiguienteAlMenu() {
  std::thread([] {
    uint8_t* base = Base();
    for (int t = 0, quieto = 0; base && t < 3000 && quieto < 150; ++t) {   // como mucho ~50 s
      quieto = REX_LOAD_U8(kGame) == 5 ? quieto + 1 : 0;
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    std::lock_guard<std::mutex> lk(g_mu);
    if (g_st.fase == kEntre && !g_reto.pausa && !Lanzar()) Terminar(kGuardado);
  }).detach();
}

// Fin de un combate nuestro (hilo del juego, ya de vuelta hacia el menu principal).
void FinCombate(int raw, int winner, int reason, int hp, int rival_hp) {
  std::lock_guard<std::mutex> lk(g_mu);
  if (g_st.fase != kCombate) return;
  REXLOG_INFO("dbz3 modos: fin del combate {}/{}: gana el lado {} (motivo {}, resultado del juego {}), vida del "
              "jugador {}, del rival {}", g_st.ganados + 1, g_st.total, winner + 1, reason, raw, hp, rival_hp);
  if (hp <= 0 || !Gana(winner, reason)) return Terminar(winner < 0 && raw == 2 ? kGuardado : kDerrota);   // salir desde la pausa: guardado
  ++g_st.ganados;
  g_st.resultado = kVictoria;
  if (g_reto.arrastra) g_st.vida = std::clamp(hp + g_reto.recupera, 1, g_st.vida_max);
  if (g_st.ganados >= g_st.total) return Terminar(kVictoria);
  if (raw == 2) return Terminar(kGuardado);     // "Return to Main Menu" (si llego a salir el menu): guardado
  g_st.fase = kEntre;
  g_tras_victoria = true;
  g_st.rival = g_reto.combates[size_t(g_st.ganados)].id;
  AnotarMarca(false);
  AnotarCurso();
  GuardarDatos();
  if (!g_reto.pausa) SiguienteAlMenu();
}

std::FILE* g_lab_csv = nullptr;     // solo el hilo del juego
uint32_t g_lab_combate = 1;         // contador de combates (el gestor del combate se reutiliza)
uint32_t g_lab_visto = 0;

void LabCuadro(uint8_t* base, uint32_t mgr) {   // hilo del juego, cada fotograma de pelea (estado 4)
  const uint32_t x[2] = {mgr + 2096, mgr + 2096 + 8240};
  std::lock_guard<std::mutex> lk(g_mu);
  const int id0 = int(REX_LOAD_U32(x[0] + 7420)), id1 = int(REX_LOAD_U32(x[1] + 7420));
  if (g_lab_visto != g_lab_combate) {   // combate nuevo (un relevo del Tag Team sigue en el mismo)
    g_lab_visto = g_lab_combate;
    CargarDatos();   // antes de empezar el resumen: el fichero trae el del combate anterior
    g_lab = {};
    g_lab.mgr = mgr;
    if (g_lab_csv) std::fclose(g_lab_csv);
    g_lab_csv = nullptr;
    if (g_lab_captura) {
      g_lab_csv = std::fopen((dbz3::settings::UserDataRoot() / "laboratorio.csv").string().c_str(), "w");
      if (g_lab_csv) std::fputs("cuadro,id1,accion1,vida1,x1,y1,z1,id2,accion2,vida2,x2,y2,z2\n", g_lab_csv);
    }
  }
  g_lab.lado[0].id = id0;
  g_lab.lado[1].id = id1;
  const int f = g_lab.cuadros++;
  for (int v = 0; v < 2; ++v) {   // v = el que recibe; a = el que pega
    Lado& victima = g_lab.lado[size_t(v)];
    Lado& a = g_lab.lado[size_t(1 - v)];
    const float hp = LoadF(base, x[v] + 6080);
    const bool baja = victima.vida_prev >= 0 && hp < victima.vida_prev;
    if (baja) {
      const float d = victima.vida_prev - hp;
      a.dano += d;
      if (!victima.bajando) {
        ++a.golpes;
        if (f - a.ultimo > 60) {
          a.combo = 0;
          a.combo_dano = 0;
        }
        ++a.combo;
      }
      a.combo_dano += d;
      a.ultimo = f;
      a.combo_mejor = std::max(a.combo_mejor, a.combo);
      a.combo_mejor_dano = std::max(a.combo_mejor_dano, a.combo_dano);
    }
    victima.bajando = baja;
    victima.vida_prev = hp;
  }
  if (g_lab_csv) {
    std::fprintf(g_lab_csv, "%d,%d,%u,%.0f,%.2f,%.2f,%.2f,%d,%u,%.0f,%.2f,%.2f,%.2f\n", f, id0, REX_LOAD_U16(x[0] + 3244),
                 LoadF(base, x[0] + 6080), LoadF(base, x[0] + 64), LoadF(base, x[0] + 68), LoadF(base, x[0] + 72), id1,
                 REX_LOAD_U16(x[1] + 3244), LoadF(base, x[1] + 6080), LoadF(base, x[1] + 64), LoadF(base, x[1] + 68),
                 LoadF(base, x[1] + 72));
    if (f % 600 == 0) std::fflush(g_lab_csv);
  }
}
void LabFinCombate() {   // hilo del juego: el combate acabo (el siguiente empieza de cero)
  std::lock_guard<std::mutex> lk(g_mu);
  ++g_lab_combate;
  if (g_lab_csv) std::fclose(g_lab_csv);
  g_lab_csv = nullptr;
  if (g_lab.cuadros > 0) GuardarDatos();   // el laboratorio lo ensena tambien tras reabrir el juego
}

// ---- Torneo con bots ----
std::vector<int> g_torneo;                        // IDs de los COM (vacio = sorteo nativo); con g_mu
std::array<int, 32> g_t_cell{};                   // por plaza: casilla puesta
std::array<int, 32> g_t_id = [] {                 // e ID que juega (-1 = el de la casilla)
  std::array<int, 32> a;
  a.fill(-1);
  return a;
}();

// Casilla de la rueda de ese ID: la suya, la nueva de un mod de roster (la de su anfitrion) o -1 (sin casilla:
// el cuadro ensena la del sorteo y el combate se juega con su ID).
int SlotOf(uint8_t* base, int id) {
  if (const int host = dbz3::roster::HostSlotOf(uint32_t(id)); host >= 0) return host;
  if (id >= 44) return -1;
  const int slot = int16_t(REX_LOAD_U16(kIdToSlot + 2 * uint32_t(id)));
  return slot >= 0 && slot < 40 && REX_LOAD_U16(kSlotToId + 2 * uint32_t(slot)) == uint32_t(id) ? slot : -1;
}

}  // namespace

bool Start(int tipo, int jugador, int traje) {
  if (dbz3::g_guest_eu || tipo < kTorre || tipo > kKumite) return false;
  std::lock_guard<std::mutex> lk(g_mu);
  if (g_st.fase == kCombate) return false;
  CargarDatos();
  g_reto = Cargar(tipo);
  std::erase_if(g_reto.combates, [](const Combate& c) {
    if (Jugable(c.id)) return false;
    REXLOG_WARN("dbz3 modos: rival {} sin personaje (falta su mod de roster): fuera del reto", c.id);
    return true;
  });
  const EnCurso* sigue = CursoDe(tipo);   // reto a medias: con su personaje, su combate y su vida
  if (sigue && (!Jugable(sigue->id) || sigue->ganados >= int(g_reto.combates.size()))) {
    QuitarCurso(tipo);
    sigue = nullptr;
  }
  if (sigue) {
    jugador = sigue->id;
    traje = sigue->traje;
  }
  if (g_reto.combates.empty() || !Jugable(jugador)) return false;
  g_st = Estado{};
  g_st.tipo = tipo;
  g_st.jugador = jugador;
  g_st.total = int(g_reto.combates.size());
  g_st.vida_max = g_reto.vida;
  g_st.vida = g_reto.vida_inicial > 0 ? std::min(g_reto.vida_inicial, g_reto.vida) : g_reto.vida;
  if (sigue) {
    g_st.ganados = sigue->ganados;
    if (sigue->vida > 0) g_st.vida = std::clamp(sigue->vida, 1, g_st.vida_max);
    REXLOG_INFO("dbz3 modos: {}: se sigue el reto guardado (combate {}, vida {})", kNombres[tipo], g_st.ganados + 1,
                g_st.vida);
  } else {
    Marca& m = MarcaDe(tipo, jugador);
    NuevaMarca(m);
    ++m.intentos;
  }
  g_traje = traje;
  g_fin_visible = false;
  g_tras_victoria = false;
  if (Lanzar()) return true;
  g_st.fase = kNada;
  return false;
}

// Siguiente combate (con `pausa = true`, tras la pantalla de resultados del HUB).
bool Continue() {
  std::lock_guard<std::mutex> lk(g_mu);
  return g_st.fase == kEntre && Lanzar();
}

// Deja el reto (sin guardarlo para seguir); un combate en curso acaba normal.
void Abort() {
  std::lock_guard<std::mutex> lk(g_mu);
  if (g_st.fase == kCombate || g_st.fase == kEntre) Terminar(kAbandono);
}

Estado Get() {
  std::lock_guard<std::mutex> lk(g_mu);
  return g_st;
}

// Participantes COM del proximo Torneo Mundial (IDs 0-63; n = 0 vuelve al sorteo nativo).
void SetTorneo(const int* ids, int n) {
  std::lock_guard<std::mutex> lk(g_mu);
  g_torneo.assign(ids, ids + std::max(n, 0));
}

// Ranking Elo por personaje (K = 32, todos empiezan en 1000, en el orden del archivo; espejos fuera).
// Mismo calculo que tools/ranking.py. Devuelve cuantos puestos escribe (de mas a menos Elo).
int Ranking(Puesto* out, int max) {
  std::istringstream in(LeerTexto(dbz3::settings::UserDataRoot() / "combates.csv"));
  std::map<int, double> elo;
  std::map<int, Puesto> p;
  std::string line;
  while (std::getline(in, line)) {
    int a = -1, b = -1, w = -1;
    if (std::sscanf(line.c_str(), "%*[^,],%d,%d,%d", &a, &b, &w) != 3 || a == b || a < 0 || b < 0) continue;
    const double ra = elo.try_emplace(a, 1000.0).first->second, rb = elo.try_emplace(b, 1000.0).first->second;
    const double sa = w == 1 ? 1.0 : w == 2 ? 0.0 : 0.5;
    const double ea = 1.0 / (1.0 + std::pow(10.0, (rb - ra) / 400.0));
    elo[a] = ra + 32.0 * (sa - ea);
    elo[b] = rb - 32.0 * (sa - ea);
    Puesto& pa = p[a];
    Puesto& pb = p[b];
    pa.id = a;
    pb.id = b;
    (w == 1 ? pa.ganados : w == 2 ? pa.perdidos : pa.empates)++;
    (w == 2 ? pb.ganados : w == 1 ? pb.perdidos : pb.empates)++;
  }
  std::vector<Puesto> v;
  for (auto& [id, q] : p) {
    q.elo = int(std::lround(elo[id]));
    v.push_back(q);
  }
  std::stable_sort(v.begin(), v.end(), [](const Puesto& x, const Puesto& y) { return x.elo > y.elo; });
  const int n = std::min(int(v.size()), std::max(max, 0));
  std::copy_n(v.begin(), n, out);
  return n;
}

// Record de un modo: el mejor de todos los personajes (false si no hay). Para el HUB.
bool Record(int tipo, int* id, int* mejor, int* total, int* completados) {
  std::lock_guard<std::mutex> lk(g_mu);
  CargarDatos();
  const Marca* best = nullptr;
  for (const Marca& m : g_marcas) {
    if (m.tipo == tipo && (!best || m.completados > best->completados ||
                           (m.completados == best->completados && m.mejor > best->mejor))) {
      best = &m;
    }
  }
  if (!best) return false;
  *id = best->id;
  *mejor = best->mejor;
  *total = best->total;
  *completados = best->completados;
  return true;
}

// Reto a medias de un modo (false si no hay): se sigue en el combate ganados + 1. Para el HUB.
bool Guardado(int tipo, int* id, int* ganados) {
  std::lock_guard<std::mutex> lk(g_mu);
  CargarDatos();
  const EnCurso* c = CursoDe(tipo);
  if (!c) return false;
  *id = c->id;
  *ganados = c->ganados;
  return true;
}

namespace {
std::string NombrePj(int id);
}
// Nombre del luchador en el idioma del juego (mods incluidos). Para el HUB.
std::string Nombre(int id) { return NombrePj(id); }

namespace {

// ---- Pantallas propias (resultado entre combates, final del reto, ranking) ----
// Como las del juego: fondo del Duelo, cinta del titulo, marco con letra turquesa, bocadillo y botones
// (piezas de native_ui, sacadas del AFS del jugador). Mientras se ven, el juego no recibe el mando.
constexpr uint16_t kUp = 0x0001, kDown = 0x0002, kLeft = 0x0004, kRight = 0x0008, kLb = 0x0100, kRb = 0x0200,
                   kA = 0x1000, kB = 0x2000;

const char* const kPjEn[44] = {"Goku", "Kid Goku", "Kid Gohan", "Teen Gohan", "Gohan", "Great Saiyaman", "Goten",
                               "Vegeta", "Trunks", "Kid Trunks", "Krillin", "Piccolo", "Tien", "Yamcha", "Hercule",
                               "Videl", "Supreme Kai", "Uub", "Raditz", "Nappa", "Ginyu", "Recoome", nullptr,
                               nullptr, nullptr, nullptr, nullptr, "Frieza", "Android 16", "Android 17", "Android 18",
                               nullptr, "Dr. Gero", "Cell", "Majin Buu", "Super Buu", "Kid Buu", "Dabura", "Cooler",
                               "Bardock", "Broly", "Omega Shenron", "Saibaman", "Cell Jr."};
const char* const kPjEs[44] = {"Goku", "Goku niño", "Gohan niño", "Gohan joven", "Gohan", "Gran Saiyaman", "Goten",
                               "Vegeta", "Trunks", "Trunks niño", "Krilin", "Piccolo", "Ten Shin Han", "Yamcha",
                               "Mr. Satán", "Videl", "Kaio-shin", "Oob", "Raditz", "Nappa", "Ginyu", "Reacum", nullptr,
                               nullptr, nullptr, nullptr, nullptr, "Freezer", "C-16", "C-17", "C-18", nullptr,
                               "Dr. Gero", "Cell", "Majin Buu", "Super Buu", "Buu niño", "Dabra", "Cooler", "Bardock",
                               "Broly", "Omega Shenron", "Saibaman", "Cell Jr."};

// Nombres de los personajes de mods (mods/_roster/roster.toml, `nombre` de cada [[personaje]]): los IDs recortados
// 22-26 y 31 conservan en el juego el nombre del recortado (GULDO...), el del mod es el bueno.
const std::map<int, std::string>& NombresMod() {
  static const std::map<int, std::string> m = [] {
    std::map<int, std::string> r;
    try {
      const std::string text = LeerTexto(dbz3::ModsRoot() / "_roster" / "roster.toml");
      if (text.empty()) return r;
      const toml::table t = toml::parse(text);
      if (const toml::array* a = t["personaje"].as_array()) {
        for (const toml::node& n : *a) {
          if (const toml::table* x = n.as_table()) {
            const int id = int((*x)["id"].value_or(int64_t(-1)));
            const std::string nombre = (*x)["nombre"].value_or(std::string());
            if (id >= 0 && id < 64 && !nombre.empty()) r[id] = nombre;
          }
        }
      }
    } catch (const toml::parse_error&) {
    }
    return r;
  }();
  return m;
}

std::string NombrePj(int id) {
  const bool es = std::strcmp(T::LangCode(), "es") == 0;
  if (dbz3::roster::DonorOf(uint32_t(id)) >= 0) {   // personaje de un mod
    if (const auto it = NombresMod().find(id); it != NombresMod().end()) return it->second;
    if (std::string n = dbz3::roster::InternalName(uint32_t(id)); !n.empty()) return n;
  }
  if (id >= 0 && id < 44 && kPjEn[id]) return es ? kPjEs[id] : kPjEn[id];
  if (std::string n = dbz3::roster::InternalName(uint32_t(id)); !n.empty()) return n;
  return "#" + std::to_string(id);
}
const char* NombreTipo(int tipo) {
  return tipo == kTorre ? T::T("Torre", "Tower") : tipo == kSupervivencia ? T::T("Supervivencia", "Survival") : "Kumite";
}
// "Piso 3 de 8" / "Oleada 3 de 10" / "Jefe 3 de 5"
std::string Paso(int tipo, int n, int total) {
  const char* w = tipo == kTorre ? T::T("Piso", "Floor") : tipo == kSupervivencia ? T::T("Oleada", "Wave")
                                                                                 : T::T("Jefe", "Boss");
  return std::string(w) + " " + std::to_string(n) + " " + T::T("de", "of") + " " + std::to_string(total);
}

int Pantalla() {
  uint8_t* base = Base();
  return base ? int(REX_LOAD_U8(kGame)) : -1;
}
bool MenuPrincipal() { return Pantalla() == 5; }
// Nombre del reto puesto (claves nombre/name de su TOML) o el del modo.
std::string NombreReto(int tipo) {
  std::lock_guard<std::mutex> lk(g_mu);
  const bool es = std::strcmp(T::LangCode(), "es") == 0;
  const std::string& n = es || g_reto.name.empty() ? g_reto.nombre : g_reto.name;
  return n.empty() ? NombreTipo(tipo) : n;
}

std::atomic<bool> g_rank_abierto{false};
std::atomic<bool> g_lab_abierto{false};
std::atomic<uint16_t> g_pad{0};
std::atomic<bool> g_pad_pendiente{false};

rex::input::InputSystem* Input() {
  auto* rt = rex::Runtime::instance();
  return rt ? static_cast<rex::input::InputSystem*>(rt->input_system()) : nullptr;
}

class ModosPanel : public rex::ui::ImGuiDialog {
 public:
  explicit ModosPanel(rex::ui::ImGuiDrawer* drawer) : ImGuiDialog(drawer) {}
  ~ModosPanel() override { Bloquear(false); }

 protected:
  void OnDraw(ImGuiIO& io) override {
    enum { kNinguna, kEntreP, kFinP, kRankP, kLabP } que = kNinguna;
    const Estado st = Get();
    const bool menu = MenuPrincipal();
    if (g_rank_abierto.load()) {
      que = kRankP;
    } else if (g_lab_abierto.load()) {
      que = kLabP;
    } else if (g_fin_visible.load() && menu && st.fase == kFin) {
      que = kFinP;
    } else if (menu && (st.fase == kEntre || (st.fase == kCombate && g_tras_victoria.load() &&
                                                NowMs() - g_lanzado_ms.load() < 1500))) {
      que = kEntreP;
    }
    const bool tapado = dbz3::GameDimmed() || dbz3::OverlayOpen();
    Bloquear(que != kNinguna && !tapado);
    if (que == kNinguna && !tapado && st.fase == kCombate && Pantalla() == 25) Contador(io, st);
    if (que != ultima_) {
      ultima_ = que;
      abierta_ = float(ImGui::GetTime());
      esperar_suelta_ = true;
      if (que == kRankP) Refrescar();
    }
    if (que == kNinguna || tapado) return;
    uint16_t pulsado = Pad();
    if (esperar_suelta_) {   // el boton que abrio la pantalla no cuenta hasta soltarlo todo
      if (g_pad.load() == 0 && float(ImGui::GetTime()) - abierta_ > 0.25f) esperar_suelta_ = false;
      pulsado = 0;
    }
    const S::Area a = S::GameArea(io);
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const float t = float(ImGui::GetTime());
    const float in = std::clamp((t - abierta_) / 0.2f, 0.0f, 1.0f);
    const int v0 = dl->VtxBuffer.Size;
    S::Backdrop(dl, a, "fondo_duelo");
    S::Host(dl, a, "anfitrion_tagteam_a", -68, 262, false);
    S::Sides(dl, a);
    if (que == kEntreP) DibujarEntre(dl, a, st);
    else if (que == kFinP) DibujarFin(dl, a, st, pulsado);
    else if (que == kLabP) DibujarLab(dl, a, pulsado);
    else DibujarRanking(dl, a, t, pulsado);
    for (int i = v0; i < dl->VtxBuffer.Size; ++i) {   // entra con un fundido corto, como los menus del juego
      ImU32& c = dl->VtxBuffer[i].col;
      c = (c & 0x00FFFFFF) | (ImU32(float(c >> 24) * in) << 24);
    }
  }

 private:
  void Bloquear(bool si) {
    if (si == bloqueado_) return;
    if (auto* inp = Input()) {
      if (si) inp->AddUIInputBlocker();
      else inp->RemoveUIInputBlocker();
      bloqueado_ = si;
    }
  }

  // Botones recien pulsados del mando 1 (la palanca izquierda cuenta como cruceta).
  uint16_t Pad() {
    if (!g_pad_pendiente.exchange(true)) {
      dbz3::launcher::DeferUi([]() {
        uint16_t b = 0;
        if (auto* inp = Input()) {
          rex::input::X_INPUT_STATE s = {};
          if (inp->GetStateForUI(0, &s) == 0) {
            b = static_cast<uint16_t>(s.gamepad.buttons);
            const int lx = int16_t(s.gamepad.thumb_lx), ly = int16_t(s.gamepad.thumb_ly);
            if (ly > 16000) b |= kUp;
            if (ly < -16000) b |= kDown;
            if (lx < -16000) b |= kLeft;
            if (lx > 16000) b |= kRight;
          }
        }
        g_pad = b;
        g_pad_pendiente = false;
      });
    }
    const uint16_t b = g_pad.load(), pulsado = uint16_t(b & ~ultimo_);
    ultimo_ = b;
    return pulsado;
  }

  // Lineas centradas en el marco del juego (letra del menu, turquesa; la primera resaltada).
  static void Marco(ImDrawList* dl, const S::Area& a, float y0, const std::vector<std::string>& lineas, float t,
                    float letra = 38.0f) {
    const float paso = 62, y1 = y0 + 24 + paso * float(lineas.size());
    const ImVec2 f0 = a.At(300, y0), f1 = a.At(900, y1);
    S::Frame(dl, f0, f1, a.s);
    S::Highlight(dl, a.At(300, y0 + 10), a.At(900, y0 + 12 + paso), t);
    for (size_t i = 0; i < lineas.size(); ++i) {
      S::MenuText(dl, a.At(600, y0 + 12 + paso * (float(i) + 0.5f)), (i == 0 ? 46.0f : letra) * a.s,
                  lineas[i].c_str(), i == 0, false);
    }
  }

  // Durante el combate, bajo el reloj: "Piso 2 de 8" (letra del menu, sin marco: no tapa el marcador).
  // Marcador del HUB (res/ingame_ui/retos: marcador_<modo> 560x128 con la insignia a la izquierda y la caja del
  // texto en (128,25)-(538,106); cifras doradas num_*): palabra con la letra del menu y "N/M" con las cifras.
  static void Contador(ImGuiIO& io, const Estado& st) {
    const S::Area a = S::GameArea(io);
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    bool aguanta = false;
    {
      std::lock_guard<std::mutex> lk(g_mu);
      aguanta = g_st.fase == kCombate && Objetivo() > 0;
    }
    const char* palabra = st.tipo == kTorre ? T::T("Piso", "Floor") : st.tipo == kSupervivencia ? T::T("Oleada", "Wave")
                                                                                                : T::T("Jefe", "Boss");
    const S::Pic m = S::Img((std::string("retos/marcador_") + kNombres[st.tipo]).c_str());
    const std::string cifras = std::to_string(st.ganados + 1) + "/" + std::to_string(st.total);
    std::vector<S::Pic> nums;
    for (const char c : cifras) {
      const S::Pic n = S::Img(c == '/' ? "retos/num_barra" : (std::string("retos/num_") + c).c_str());
      if (!n) {
        nums.clear();
        break;
      }
      nums.push_back(n);
    }
    if (!m || nums.empty()) {   // sin las piezas: solo la letra del menu
      std::string txt = Paso(st.tipo, st.ganados + 1, st.total);
      if (aguanta) txt += T::T("  ¡Aguanta!", "  Survive!");
      S::MenuText(dl, a.At(600, 182), 30 * a.s, txt.c_str(), true, false);
      return;
    }
    const float h = 70, k = h / 128.0f, w = 560 * k, x0 = 600 - w * 0.5f, y0 = 148;
    S::DrawPic(dl, m, a.At(x0, y0), a.At(x0 + w, y0 + h));
    const float bx0 = x0 + 128 * k, bx1 = x0 + 538 * k, cy = y0 + 65.5f * k, dh = 81 * k * 0.62f;
    float dw = 0;
    for (const S::Pic& n : nums) dw += n.w * dh / n.h - 2;
    float x = bx1 - 8 - dw;
    for (const S::Pic& n : nums) {
      const float nw = n.w * dh / n.h;
      S::DrawPic(dl, n, a.At(x, cy - dh * 0.5f), a.At(x + nw, cy + dh * 0.5f));
      x += nw - 2;
    }
    S::MenuText(dl, a.At((bx0 + bx1 - 8 - dw) * 0.5f, cy), 32 * a.s, palabra, true, false);
    if (aguanta) S::MenuText(dl, a.At(600, y0 + h + 16), 26 * a.s, T::T("¡Aguanta!", "Survive!"), true, false);
  }

  void DibujarEntre(ImDrawList* dl, const S::Area& a, const Estado& st) {
    S::Banner(dl, a, NombreReto(st.tipo).c_str());
    bool arrastra = false;
    {
      std::lock_guard<std::mutex> lk(g_mu);
      arrastra = g_reto.arrastra;
    }
    std::vector<std::string> l = {Paso(st.tipo, st.ganados, st.total) + T::T(": ¡superado!", ": cleared!"),
                                  std::string(T::T("Siguiente: ", "Next: ")) + NombrePj(st.rival)};
    if (arrastra) l.push_back(std::string(T::T("Vida ", "Health ")) + std::to_string(st.vida) + " / " +
                                     std::to_string(st.vida_max));
    Marco(dl, a, 190, l, float(ImGui::GetTime()));
    const std::string bocadillo = Paso(st.tipo, st.ganados + 1, st.total) + ". " +
                                  T::T("¡Prepárate para el siguiente combate!", "Get ready for the next fight!");
    S::Speech(dl, a.At(200, 615), a.At(840, 845), a.s, bocadillo.c_str(), 0.12f);
  }

  void DibujarFin(ImDrawList* dl, const S::Area& a, const Estado& st, uint16_t pulsado) {
    if (pulsado & (kA | kB)) g_fin_visible = false;   // vuelve el HUB
    S::Banner(dl, a, NombreReto(st.tipo).c_str());
    std::string titulo, dice;
    if (st.resultado == kVictoria) {
      titulo = st.tipo == kTorre ? T::T("¡Torre completada!", "Tower cleared!")
               : st.tipo == kSupervivencia ? T::T("¡Has sobrevivido!", "You survived!")
                                           : T::T("¡Kumite superado!", "Kumite cleared!");
      dice = T::T("¡Increíble! Nadie ha llegado tan lejos.", "Amazing! Nobody has made it this far.");
    } else if (st.resultado == kGuardado) {
      titulo = T::T("Reto guardado", "Challenge saved");
      dice = Paso(st.tipo, st.ganados + 1, st.total) + ": " +
             T::T("la próxima vez seguirás desde aquí.", "next time you will continue from here.");
    } else {
      titulo = st.resultado == kAbandono ? T::T("Reto abandonado", "Challenge abandoned")
                                         : T::T("Derrota", "Defeat");
      dice = T::T("¡Vuelve a intentarlo! Cada combate cuenta para tu récord.",
                  "Try again! Every fight counts toward your record.");
    }
    int id = 0, mejor = 0, total = 0, comp = 0;
    {
      std::lock_guard<std::mutex> lk(g_mu);
      const Marca& m = MarcaDe(st.tipo, st.jugador);
      mejor = m.mejor;
      total = m.total;
      comp = m.completados;
      id = st.jugador;
    }
    std::vector<std::string> l = {titulo, NombrePj(id) + ": " + std::to_string(st.ganados) + " / " +
                                              std::to_string(st.total),
                                  std::string(T::T("Récord: ", "Record: ")) + std::to_string(mejor) + " / " +
                                      std::to_string(total) + (comp > 0 ? "  x" + std::to_string(comp) : "")};
    Marco(dl, a, 190, l, float(ImGui::GetTime()));
    S::Speech(dl, a.At(200, 615), a.At(840, 845), a.s, dice.c_str(), 0.12f);
    S::HelpLine(dl, a.At(600, 868), 40 * a.s, {{"A", T::T("Aceptar", "OK")}});
  }

  // ---- Ranking: pagina 0 = Elo por personaje; 1-3 = records de cada modo por personaje ----
  struct Fila {
    std::string pos, nombre, valor, extra;
    int medalla = 0;   // 1 bronce .. 5 diamante (retos/medalla_*), 0 = sin medalla
  };
  void Refrescar() {
    paginas_.assign(4, {});
    std::array<Puesto, 10> top{};
    const int n = Ranking(top.data(), int(top.size()));
    for (int i = 0; i < n; ++i) {
      // medalla por Elo: menos de 1000 bronce, 1000 plata, 1100 oro, 1200 platino, 1300 diamante
      paginas_[0].push_back({std::to_string(i + 1), NombrePj(top[size_t(i)].id), std::to_string(top[size_t(i)].elo),
                             std::to_string(top[size_t(i)].ganados) + "-" + std::to_string(top[size_t(i)].perdidos),
                             std::clamp(1 + (top[size_t(i)].elo - 900) / 100, 1, 5)});
    }
    std::lock_guard<std::mutex> lk(g_mu);
    CargarDatos();
    for (int tipo = 0; tipo < 3; ++tipo) {
      std::vector<Marca> v;
      for (const Marca& m : g_marcas) {
        if (m.tipo == tipo) v.push_back(m);
      }
      std::stable_sort(v.begin(), v.end(), [](const Marca& x, const Marca& y) {
        return x.completados != y.completados ? x.completados > y.completados : x.mejor > y.mejor;
      });
      for (size_t i = 0; i < v.size() && i < 10; ++i) {
        paginas_[size_t(tipo) + 1].push_back(
            {std::to_string(i + 1), NombrePj(v[i].id), std::to_string(v[i].mejor) + " / " + std::to_string(v[i].total),
             v[i].completados > 0 ? "x" + std::to_string(v[i].completados) : "-",
             std::min(v[i].completados, 5)});   // medalla por veces completado
      }
      const EnCurso* c = CursoDe(tipo);
      curso_[size_t(tipo)] = c ? Paso(tipo, c->ganados + 1, c->total) + " (" + NombrePj(c->id) + ")" : std::string();
    }
  }

  void DibujarRanking(ImDrawList* dl, const S::Area& a, float t, uint16_t pulsado) {
    if (pulsado & (kLeft | kLb)) pagina_ = (pagina_ + 3) % 4;
    if (pulsado & (kRight | kRb | kA)) pagina_ = (pagina_ + 1) % 4;
    if (pulsado & kB) g_rank_abierto = false;   // vuelve el HUB
    S::Banner(dl, a, "Ranking");
    // pagina: el selector de formato del juego (marco, letra y flechas)
    const char* titulos[4] = {T::T("Ranking de luchadores", "Fighter ranking"), NombreTipo(kTorre),
                              NombreTipo(kSupervivencia), NombreTipo(kKumite)};
    const ImVec2 g0 = a.At(330, 118), g1 = a.At(870, 180);
    S::Frame(dl, g0, g1, a.s);
    S::MenuText(dl, ImVec2((g0.x + g1.x) * 0.5f, (g0.y + g1.y) * 0.5f), 42 * a.s, titulos[pagina_], true, false);
    S::Arrows(dl, ImVec2(g0.x - 18 * a.s, g0.y), ImVec2(g1.x + 18 * a.s, g1.y), 42 * a.s, t);
    // tabla: 8 filas con la letra del menu; la primera (el lider) con la franja del juego
    const float y0 = 196, paso = 49;
    const ImVec2 f0 = a.At(330, y0), f1 = a.At(870, y0 + 16 + paso * 8);
    S::Frame(dl, f0, f1, a.s);
    const auto& filas = paginas_[size_t(pagina_)];
    if (filas.empty()) {
      S::MenuText(dl, a.At(600, y0 + 16 + paso * 4), 36 * a.s, T::T("Aún no hay combates", "No fights yet"), false,
                  true);
    }
    for (size_t i = 0; i < filas.size() && i < 8; ++i) {
      const float y = y0 + 8 + paso * (float(i) + 0.5f);
      if (i == 0) S::Highlight(dl, a.At(330, y - paso * 0.5f + 2), a.At(870, y + paso * 0.5f - 2), t);
      const float k = 32 * a.s;
      S::MenuText(dl, a.At(366, y), k, filas[i].pos.c_str(), i == 0, false);
      static const char* const kMedallas[] = {"retos/medalla_1_bronce", "retos/medalla_2_plata", "retos/medalla_3_oro",
                                              "retos/medalla_4_platino", "retos/medalla_5_diamante"};
      if (filas[i].medalla > 0) {
        if (const S::Pic md = S::Img(kMedallas[filas[i].medalla - 1])) {
          const float r = paso * 0.42f;
          S::DrawPic(dl, md, a.At(410 - r, y - r), a.At(410 + r, y + r));
        }
      }
      S::MenuText(dl, a.At(560, y), k, filas[i].nombre.c_str(), i == 0, false);
      S::MenuText(dl, a.At(728, y), k, filas[i].valor.c_str(), i == 0, false);
      S::MenuText(dl, a.At(830, y), k * 0.85f, filas[i].extra.c_str(), i == 0, false);
    }
    std::string dice;
    if (pagina_ == 0) {
      dice = T::T("Elo de cada luchador en tus combates: sube al ganar y baja al perder. "
                  "A la derecha, victorias y derrotas.",
                  "Each fighter's Elo in your fights: it rises when you win and drops when you lose. "
                  "On the right, wins and losses.");
    } else {
      dice = T::T("Mejor marca con cada luchador (combates ganados seguidos). x = veces completado.",
                  "Best run with each fighter (fights won in a row). x = times cleared.");
      if (!curso_[size_t(pagina_ - 1)].empty()) {
        dice += std::string("\n") + T::T("Guardado: ", "Saved: ") + curso_[size_t(pagina_ - 1)];
      }
    }
    S::Speech(dl, a.At(200, 615), a.At(840, 845), a.s, dice.c_str(), 0.12f);
    S::HelpLine(dl, a.At(600, 868), 40 * a.s, {{"LB", T::T("Anterior", "Previous")}, {"RB", T::T("Siguiente", "Next")},
                                               {"B", T::T("Atrás", "Back")}});
  }

  // Laboratorio: el ultimo combate (de cualquier modo) por luchador, y la captura por fotograma (A).
  void DibujarLab(ImDrawList* dl, const S::Area& a, uint16_t pulsado) {
    Lab lab;
    bool captura = false;
    {
      std::lock_guard<std::mutex> lk(g_mu);
      CargarDatos();
      if (pulsado & kA) {
        g_lab_captura = !g_lab_captura;
        GuardarDatos();
      }
      lab = g_lab;
      captura = g_lab_captura;
    }
    if (pulsado & kB) g_lab_abierto = false;   // vuelve el HUB
    S::Banner(dl, a, T::T("Laboratorio", "Laboratory"));
    const float t = float(ImGui::GetTime());
    if (lab.cuadros == 0) {
      Marco(dl, a, 190, {T::T("Último combate", "Last fight"), T::T("Aún no hay combates", "No fights yet")}, t);
    } else {
      char buf[128];
      std::vector<std::string> l;
      std::snprintf(buf, sizeof(buf), "%s  %d:%02d", T::T("Último combate", "Last fight"), lab.cuadros / 3600,
                    lab.cuadros / 60 % 60);
      l.emplace_back(buf);
      for (const Lado& s : lab.lado) {
        std::snprintf(buf, sizeof(buf), "%s: %.0f %s, %d %s, combo %d", NombrePj(s.id).c_str(), double(s.dano),
                      T::T("de daño", "damage"), s.golpes, T::T("golpes", "hits"), s.combo_mejor);
        l.emplace_back(buf);
      }
      Marco(dl, a, 190, l, t, 28.0f);
    }
    const std::string estado = std::string(T::T("Captura por fotograma: ", "Frame capture: ")) +
                               (captura ? T::T("SÍ", "ON") : T::T("NO", "OFF"));
    S::MenuText(dl, a.At(600, 470), 36 * a.s, estado.c_str(), captura, !captura);
    S::Speech(dl, a.At(200, 615), a.At(840, 845), a.s,
              T::T("Resumen del último combate que has jugado. Con la captura, cada fotograma (acción, vida y "
                   "posición de los dos) queda en user_data/dbz3/laboratorio.csv.",
                   "Summary of the last fight you played. With capture on, every frame (action, health and "
                   "position of both fighters) goes to user_data/dbz3/laboratorio.csv."),
              0.12f);
    S::HelpLine(dl, a.At(600, 868), 40 * a.s, {{"A", T::T("Captura", "Capture")}, {"B", T::T("Atrás", "Back")}});
  }

  int ultima_ = -1, pagina_ = 0;
  float abierta_ = 0;
  bool esperar_suelta_ = true, bloqueado_ = false;
  uint16_t ultimo_ = 0;
  std::vector<std::vector<Fila>> paginas_ = std::vector<std::vector<Fila>>(4);
  std::array<std::string, 3> curso_;
};

std::atomic<bool> g_panel_pedido{false};
void CrearPanel() {
  static ModosPanel* panel = nullptr;   // vive lo que el juego (el dibujante no borra sus dialogos)
  auto* rt = rex::Runtime::instance();
  if (!panel && rt && rt->imgui_drawer()) panel = new ModosPanel(rt->imgui_drawer());
}
void PedirPanel() {
  if (!g_panel_pedido.exchange(true)) dbz3::launcher::DeferUi(&CrearPanel);
}

// ---- HUB (hub_menu.cpp): A en la entrada (hilo del juego, menu principal activo) ----
// Retos: con el ultimo personaje del Duelo (partida+31/+33) o, si hay uno guardado, con el suyo. Mientras dure
// (y hasta cerrar la pantalla del final) el HUB no se dibuja (busy); al acabar vuelve a abrirse.
void AbrirReto(int tipo) {
  PedirPanel();
  int jugador = 0, traje = 0;
  if (uint8_t* base = Base()) {
    jugador = int8_t(REX_LOAD_U8(kGame + 31));
    traje = int8_t(REX_LOAD_U8(kGame + 33));
  }
  if (!Jugable(jugador)) jugador = traje = 0;
  if (traje < 0 || traje > 3) traje = 0;
  if (!Start(tipo, jugador, traje)) REXLOG_WARN("dbz3 modos: {} no se pudo empezar", kNombres[tipo]);
}
bool RetoEnCurso() {
  const int f = Get().fase;
  return f == kCombate || f == kEntre || (f == kFin && g_fin_visible.load());
}
void AbrirRanking() {
  PedirPanel();
  g_rank_abierto = true;
}
bool RankingAbierto() { return g_rank_abierto.load(); }
void AbrirLab() {
  PedirPanel();
  g_lab_abierto = true;
}
bool LabAbierto() { return g_lab_abierto.load(); }
const bool g_hub = dbz3::hub::RegisterMode("torre", [] { AbrirReto(kTorre); }, &RetoEnCurso) &&
                   dbz3::hub::RegisterMode("supervivencia", [] { AbrirReto(kSupervivencia); }, &RetoEnCurso) &&
                   dbz3::hub::RegisterMode("kumite", [] { AbrirReto(kKumite); }, &RetoEnCurso) &&
                   dbz3::hub::RegisterMode("ranking", &AbrirRanking, &RankingAbierto) &&
                   dbz3::hub::RegisterMode("laboratorio", &AbrirLab, &LabAbierto);

// PRUEBAS. DBZ3_TEST_MODO="tipo,jugador[,traje]" (tipo: torre, supervivencia, kumite): empieza el reto cuando
// el menu principal lleva 3 s en pantalla y, al acabar, escribe el ranking en el log.
// DBZ3_TEST_TORNEO="id,id,...": participantes COM del Torneo Mundial.
const bool g_test = [] {
  if (const char* t = std::getenv("DBZ3_TEST_TORNEO")) {
    std::vector<int> ids;
    for (std::istringstream ss(t); ss.good();) {
      int id = -1;
      char sep = 0;
      if (ss >> id) ids.push_back(id);
      ss >> sep;
    }
    SetTorneo(ids.data(), int(ids.size()));
  }
  const char* t = std::getenv("DBZ3_TEST_MODO");
  if (!t) return false;
  std::thread([s = std::string(t)] {
    char name[32] = {};
    int jugador = 0, traje = 0;
    std::sscanf(s.c_str(), "%31[^,],%d,%d", name, &jugador, &traje);
    const int tipo = std::max(TipoDe(name), 0);
    for (int quieto = 0; quieto < 120;) {   // 3 s en el menu principal
      quieto = dbz3::ingame::GameScreen() == 5 ? quieto + 1 : 0;
      std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    PedirPanel();
    REXLOG_INFO("dbz3 modos: prueba: {} con id {} (traje {}): {}", kNombres[tipo], jugador, traje,
                Start(tipo, jugador, traje) ? "empieza" : "no se pudo empezar");
    while (Get().fase != kFin) std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::array<Puesto, 5> top{};
    const int n = Ranking(top.data(), int(top.size()));
    for (int i = 0; i < n; ++i)
      REXLOG_INFO("dbz3 modos: ranking {}: id {} elo {} ({}-{}-{})", i + 1, top[i].id, top[i].elo, top[i].ganados,
                  top[i].perdidos, top[i].empates);
  }).detach();
  return true;
}();
}  // namespace

}  // namespace dbz3::modos

// Espera de la escena 24: pedida la 25, el juego pasa la CFG a sub_820FE668. Con un combate del reto se
// completa antes: trajes y vida de los dos lados.
REX_HOOK_RAW(sub_821D7400) {
  using namespace dbz3::modos;
  if (!dbz3::g_guest_eu && dbz3::hub::OwnBattle() && int32_t(REX_LOAD_U32(kReqCur)) == 25) {
    g_base = base;
    std::lock_guard<std::mutex> lk(g_mu);
    if (g_st.fase == kCombate) {
      const Combate& c = g_reto.combates[size_t(g_st.ganados)];
      constexpr uint32_t p1 = kCfg + 16, p2 = kCfg + 96;
      REX_STORE_U16(p1 + 18, uint16_t(g_traje));
      REX_STORE_U32(p1 + 36, uint32_t(g_st.vida));
      REX_STORE_U32(p1 + 40, uint32_t(g_st.vida_max));
      REX_STORE_U16(p2 + 18, uint16_t(c.traje));
      if (c.vida > 0) {
        REX_STORE_U32(p2 + 36, uint32_t(c.vida));
        REX_STORE_U32(p2 + 40, uint32_t(c.vida));
      }
      // tiempo (+348) en segundos: sub_820FE668 lo pasa a cuadros (x 60) y sub_820FD678 acaba el combate al llegar
      // (motivo 3). Medido 2026-10-15: segundos x 60 (como deja sub_82179408 las opciones) duraba 60 veces mas.
      if (const int sec = Tiempo(); sec > 0) REX_STORE_U32(kCfg + 348, uint32_t(sec));
      REXLOG_INFO("dbz3 modos: CFG: id {} traje {} vida {}/{} contra id {} traje {} vida {} (tiempo {} s, regen {}/s, "
                  "objetivo {} s)",
                  REX_LOAD_U16(p1 + 16), g_traje, g_st.vida, g_st.vida_max, REX_LOAD_U16(p2 + 16), c.traje,
                  int32_t(REX_LOAD_U32(p2 + 36)), Tiempo(), Regen(), Objetivo());
    }
  }
  __imp__sub_821D7400(ctx, base);
}

// Fin de combate (antes de soltarlo): ganador y vida del jugador para el reto.
REX_HOOK_RAW(sub_820FEBA8) {
  using namespace dbz3::modos;
  bool mine = false;
  int raw = 0, winner = -1, reason = 0, hp = 0, rival_hp = 0;
  if (!dbz3::g_guest_eu && dbz3::hub::OwnBattle()) {
    std::lock_guard<std::mutex> lk(g_mu);
    mine = g_st.fase == kCombate;
  }
  if (mine) {
    g_base = base;
    raw = int32_t(REX_LOAD_U32(kResult));
    if (const uint32_t mgr = REX_LOAD_U32(kMgrPtr)) {
      winner = int32_t(REX_LOAD_U32(mgr + 35136));
      reason = int32_t(REX_LOAD_U32(mgr + 35132));
      hp = int(LoadF(base, mgr + 2096 + 6080));
      rival_hp = int(LoadF(base, mgr + 2096 + 8240 + 6080));
    }
  }
  __imp__sub_820FEBA8(ctx, base);
  if (!dbz3::g_guest_eu) LabFinCombate();
  if (mine) FinCombate(raw, winner, reason, hp, rival_hp);
}

// Torneo Mundial: tras el sorteo nativo, los COM son los de la lista (en orden, hasta acabarla).
REX_HOOK_RAW(sub_821A28A8) {
  using namespace dbz3::modos;
  __imp__sub_821A28A8(ctx, base);
  if (dbz3::g_guest_eu) return;
  std::lock_guard<std::mutex> lk(g_mu);
  g_t_id.fill(-1);
  std::vector<int> lista = g_torneo;
  if (lista.empty()) {   // user_data/dbz3/modos/torneo.toml: participantes = [id, ...]
    const std::string text = LeerTexto(dbz3::settings::UserDataRoot() / "modos" / "torneo.toml");
    try {
      if (!text.empty()) {
        const toml::table t = toml::parse(text);
        if (const toml::array* a = t["participantes"].as_array()) {
          for (const toml::node& n : *a) lista.push_back(int(n.value_or(int64_t(-1))));
        }
      }
    } catch (const toml::parse_error& e) {
      REXLOG_WARN("dbz3 modos: torneo.toml no se puede leer ({})", e.description());
    }
  }
  if (lista.empty()) return;
  const uint32_t obj = REX_LOAD_U32(kWtScreen);
  const int rounds = int8_t(REX_LOAD_U8(kWt + 36690));
  if (!obj || rounds < 1 || rounds > 5) return;
  size_t k = 0;
  for (int i = 0; i < (1 << rounds) && k < lista.size(); ++i) {
    const uint32_t e = obj + 748 + 20 * uint32_t(i);
    if (int16_t(REX_LOAD_U16(e + 10)) >= 0) continue;   // humano
    const int id = lista[k++];
    if (!Jugable(id)) continue;
    int slot = SlotOf(base, id);
    if (slot >= 0) REX_STORE_U16(e + 8, uint16_t(slot));
    else slot = int16_t(REX_LOAD_U16(e + 8));
    g_t_cell[size_t(i)] = slot;
    if (REX_LOAD_U16(kSlotToId + 2 * uint32_t(slot)) != uint32_t(id)) g_t_id[size_t(i)] = id;
    REXLOG_INFO("dbz3 modos: torneo: plaza {} = id {} (casilla {})", i, id, slot);
  }
}

// Torneo Mundial: preparacion del combate (cada tick del cuadro). Un participante sin casilla propia juega con
// su ID (la casilla es la de su anfitrion).
REX_HOOK_RAW(sub_821A5A30) {
  using namespace dbz3::modos;
  __imp__sub_821A5A30(ctx, base);
  if (dbz3::g_guest_eu) return;
  std::lock_guard<std::mutex> lk(g_mu);
  const uint32_t obj = REX_LOAD_U32(kWtScreen);
  for (uint32_t p = 0; obj && p < 2; ++p) {
    const int i = int8_t(REX_LOAD_U8(kWt + 36688 + p));
    if (i < 0 || i >= 32 || g_t_id[size_t(i)] < 0) continue;
    if (REX_LOAD_U16(obj + 748 + 20 * uint32_t(i) + 8) != uint32_t(g_t_cell[size_t(i)])) continue;
    if (REX_LOAD_U8(kGame + 31 + p) != uint32_t(g_t_id[size_t(i)])) {
      REX_STORE_U8(kGame + 31 + p, uint8_t(g_t_id[size_t(i)]));
      REXLOG_INFO("dbz3 modos: torneo: lado {} juega con id {}", p + 1, g_t_id[size_t(i)]);
    }
  }
}

// Menu del final del combate (estado 7 del combate, sub_820FF750 cada fotograma): sub_820DF568 lee la fila elegida
// del menu del marcador (B = *0x82324064: +4272 bit 0 = aun eligiendo, +4292 = fila). Fila 2 ("Reselect
// Characters") = R 4 y salir (sub_820FF820). En un reto ganado se elige sola: del WINNER! al combate siguiente sin
// menu. Tras perder sale el menu (Battle Again? = reintentar el mismo combate).
REX_HOOK_RAW(sub_820FF750) {
  using namespace dbz3::modos;
  if (!dbz3::g_guest_eu && dbz3::hub::OwnBattle()) {
    const uint32_t mgr = REX_LOAD_U32(kMgrPtr), hud = REX_LOAD_U32(kHud);
    bool gana = false;
    if (mgr) {
      std::lock_guard<std::mutex> lk(g_mu);
      gana = g_st.fase == kCombate &&
             Gana(int32_t(REX_LOAD_U32(mgr + 35136)), int32_t(REX_LOAD_U32(mgr + 35132))) &&
             LoadF(base, mgr + 2096 + 6080) > 0.0f;
    }
    if (gana && hud && (REX_LOAD_U32(hud + 4272) & 1)) {
      REX_STORE_U32(hud + 4272, REX_LOAD_U32(hud + 4272) & ~1u);
      REX_STORE_U16(hud + 4292, 2);
      REXLOG_INFO("dbz3 modos: victoria: sin menu final, al combate siguiente");
    }
  }
  __imp__sub_820FF750(ctx, base);
}

// Comprobacion de cada fotograma de la pelea (sub_820FD678: KO, fuera del ring y tiempo; medido 2026-10-15 en el
// modo 5: una llamada por fotograma del estado 4). Con `regen`, el rival del reto recupera vida mientras no ha caido.
REX_HOOK_RAW(sub_820FD678) {
  using namespace dbz3::modos;
  if (!dbz3::g_guest_eu) {   // laboratorio: resumen de cualquier combate
    const uint32_t mgr = REX_LOAD_U32(kMgrPtr);
    if (mgr && REX_LOAD_U32(mgr + 35128) == 4) LabCuadro(base, mgr);
  }
  if (!dbz3::g_guest_eu && dbz3::hub::OwnBattle()) {
    int regen = 0;
    {
      std::lock_guard<std::mutex> lk(g_mu);
      if (g_st.fase == kCombate) regen = Regen();
    }
    const uint32_t mgr = regen > 0 ? REX_LOAD_U32(kMgrPtr) : 0;
    if (mgr && REX_LOAD_U32(mgr + 35128) == 4) {
      const uint32_t x = mgr + 2096 + 8240;   // luchador del lado 2 (el rival)
      const float hp = LoadF(base, x + 6080), mx = LoadF(base, x + 6084);
      if (hp > 0.0f && hp < mx) StoreF(base, x + 6080, std::min(mx, hp + float(regen) / 60.0f));
      static int64_t log_ms = 0;
      if (NowMs() - log_ms > 5000) {
        log_ms = NowMs();
        REXLOG_INFO("dbz3 modos: regen {}/s: rival {:.0f}/{:.0f}", regen, LoadF(base, x + 6080), mx);
      }
    }
  }
  __imp__sub_820FD678(ctx, base);
}

