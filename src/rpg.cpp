// dbz3 - Retos de rol del HUB (fichas 23-28 de studies/11_vision_futura y lo que faltaba de la 03):
//   Simulacion (3 dias de entrenar/explorar/descansar y un jefe), Supervivencia con tienda (vida que se arrastra,
//   Zenie de la partida y tienda entre combates), Modo Heroe (3 variantes de Goku, 2 tecnicas a elegir y una mision),
//   Torres del Tiempo (torre de la semana y 2 fijas: 3 encuentros con tiempo y un modificador), Abyss (3 pisos y 3
//   cartas de mejora tras cada uno), Kumite de jefes (armadura con hueco castigable, golpes duros, aguantar) y Torneo
//   de 4 (cuadro propio con 3 bots).
// Selector: las entradas Torre / Supervivencia / Kumite del HUB abren una lista con el reto clasico y los nuevos.
// Combates: modos.cpp (StartPropio, Continue, SetVida, SetReglas, SetSiguiente) sobre el conector de hub.cpp. Las
// pantallas las dibuja el panel de modos.cpp (Dibujar) con las piezas del juego (native_ui). Mientras tanto el juego no
// recibe el mando.
// Guardado: user_data/dbz3/rpg.toml (version 1; .tmp + rename, la anterior en .bak; roto: se aparta a .roto y se
// empieza de cero; version mas nueva: no se toca). Monedero de Zenie del HUB (premios de los retos) y la partida a
// medias de cada reto (se reanuda tal cual: semilla, eventos y vida). La partida del juego no se toca.
// US y EU (como modos.cpp). Docs: docs/14_frentes/rpg.md.
#include <imgui.h>

#include <rex/logging.h>

#include <toml++/toml.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "guest_region.h"
#include "ingame/game_style.h"
#include "launcher/settings.h"
#include "escenarios.h"
#include "modos.h"

namespace dbz3::rpg {
namespace {
namespace M = dbz3::modos;
namespace S = dbz3::ingame::style;
using M::L;

constexpr uint16_t kUp = 0x0001, kDown = 0x0002, kLeft = 0x0004, kRight = 0x0008, kLb = 0x0100, kRb = 0x0200,
                   kA = 0x1000, kB = 0x2000;

uint32_t Hash(uint32_t a, uint32_t b) {   // mezcla entera (semilla, paso): lo mismo da lo mismo
  uint32_t h = a * 0x9E3779B1u ^ (b * 0x85EBCA6Bu + 0x7F4A7C15u);
  h ^= h >> 15;
  h *= 0x2C1B3C6Du;
  h ^= h >> 12;
  h *= 0x297A2D39u;
  h ^= h >> 15;
  return h;
}
std::string Num(int n) { return std::to_string(n); }

// ---- Catalogos ----
// Simulacion (23): jefe del dia 4 segun la semilla
constexpr int kSimJefes[] = {33, 40, 36};
constexpr int kSimDias = 3;
// Tienda (24): 5 rivales con mas vida cada vez; la vida del jugador se arrastra
constexpr int kTiendaRivales[] = {42, 18, 19, 21, 20};
constexpr int kZeniVictoria = 100, kZeniPerfecto = 50;
struct Articulo {
  int precio;
  const char* es;
  const char* en;
  const char* fr;
  const char* de;
  const char* it;
};
const Articulo kArticulos[] = {
    {60, "Cura: +1000 de vida", "Heal: +1000 health", "Soin : +1000 vie", "Heilung: +1000 Leben", "Cura: +1000 vita"},
    {80, "Fuerza: +25% de daño (próximo combate)", "Strength: +25% damage (next fight)",
     "Force : +25% de dégâts (prochain combat)", "Kraft: +25% Schaden (nächster Kampf)",
     "Forza: +25% di danni (prossimo incontro)"},
    {80, "Defensa: -25% de daño recibido (próximo combate)", "Defense: -25% damage taken (next fight)",
     "Défense : -25% de dégâts subis (prochain combat)", "Abwehr: -25% erlittener Schaden (nächster Kampf)",
     "Difesa: -25% di danni subiti (prossimo incontro)"},
};
// Heroe (25): una familia (el cuerpo de Goku, trajes 0-2; el 3 es de un evento) y sus tecnicas compatibles
struct Variante {
  int traje, dano, vida, defensa;
  const char* es;
  const char* en;
  const char* fr;
  const char* de;
  const char* it;
};
const Variante kVariantes[] = {
    {0, 115, 3000, 100, "Guerrero: +15% de daño", "Warrior: +15% damage", "Guerrier : +15% de dégâts",
     "Krieger: +15% Schaden", "Guerriero: +15% di danni"},
    {1, 100, 4000, 100, "Guardián: +1000 de vida", "Guardian: +1000 health", "Gardien : +1000 vie",
     "Wächter: +1000 Leben", "Guardiano: +1000 vita"},
    {2, 100, 3000, 80, "Viajero: -20% de daño recibido", "Traveler: -20% damage taken",
     "Voyageur : -20% de dégâts subis", "Reisender: -20% erlittener Schaden", "Viaggiatore: -20% di danni subiti"},
};
struct Tecnica {
  int capsula;
  const char* nombre;
};
const Tecnica kTecnicas[] = {{6, "Kamehameha"}, {10, "Genki Dama / Spirit Bomb"}, {9, "Warp Kamehameha"}};
constexpr int kHeroeId = 0;
struct Paso {
  int rival, vida, escenario;
  const char* es;
  const char* en;
  const char* fr;
  const char* de;
  const char* it;
};
const Paso kMision[] = {
    {42, 1500, 5,
     "Unos Saibaman atacan la ciudad del Oeste. ¡Demuestra quién eres!",
     "Saibamen are attacking West City. Show them who you are!",
     "Des Saibaman attaquent la Capitale de l'Ouest. Montre-leur qui tu es !",
     "Saibaman greifen die Westliche Hauptstadt an. Zeig ihnen, wer du bist!",
     "Dei Saibaman attaccano la Città dell'Ovest. Mostra loro chi sei!"},
    {19, 3000, 4,
     "Nappa ha visto tu fuerza y viene a por ti. ¡No te rindas!",
     "Nappa saw your strength and is coming for you. Don't give up!",
     "Nappa a vu ta force et vient te chercher. N'abandonne pas !",
     "Nappa hat deine Stärke gesehen und kommt zu dir. Gib nicht auf!",
     "Nappa ha visto la tua forza e viene a cercarti. Non arrenderti!"},
    {7, 3500, 6,
     "El príncipe Vegeta en persona. ¡Es el combate decisivo!",
     "Prince Vegeta himself. This is the decisive fight!",
     "Le prince Vegeta en personne. C'est le combat décisif !",
     "Prinz Vegeta höchstpersönlich. Das ist der entscheidende Kampf!",
     "Il principe Vegeta in persona. È lo scontro decisivo!"},
};
// Torres del Tiempo (26): catalogo (version 1). La de la semana rota cada lunes (semana UTC); 0 y 1, siempre.
struct Torre {
  const char* es;
  const char* en;
  int rivales[3];
  int tiempo;
  M::Reglas reglas;
  int regen;
  const char* mod_es;
  const char* mod_en;
  const char* mod_fr;
  const char* mod_de;
  const char* mod_it;
};
M::Reglas R(int hecho, int recibido, int armadura, int ki) {
  M::Reglas r;
  r.dano_hecho = hecho;
  r.dano_recibido = recibido;
  r.armadura = armadura;
  r.ki_drena = ki;
  return r;
}
const Torre kTorres[] = {
    {"Torre Saiyan", "Saiyan Tower", {18, 19, 7}, 60, R(100, 100, 0, 4), 0, "Suelo que drena el ki",
     "Floor that drains ki", "Sol qui draine le ki", "Boden, der Ki entzieht", "Suolo che prosciuga il ki"},
    {"Torre Androide", "Android Tower", {29, 30, 33}, 90, R(100, 100, 0, 0), 20, "El rival se regenera",
     "The rival regenerates", "L'adversaire se régénère", "Der Gegner regeneriert sich", "L'avversario si rigenera"},
    {"Torre Ginyu", "Ginyu Tower", {21, 20, 27}, 75, R(100, 150, 0, 0), 0, "Golpes duros: +50% de daño recibido",
     "Hard hits: +50% damage taken", "Coups durs : +50% de dégâts subis", "Harte Treffer: +50% erlittener Schaden",
     "Colpi duri: +50% di danni subiti"},
    {"Torre Majin", "Majin Tower", {34, 35, 36}, 99, R(100, 100, 30, 0), 0, "Rivales blindados: -30% de daño",
     "Armored rivals: -30% damage", "Adversaires blindés : -30% de dégâts", "Gepanzerte Gegner: -30% Schaden",
     "Avversari corazzati: -30% di danni"},
};
constexpr int kNumTorres = int(std::size(kTorres));
int SemanaUtc() {   // PRUEBA: DBZ3_TEST_SEMANA=N simula otra semana (otra fecha)
  static const char* e = std::getenv("DBZ3_TEST_SEMANA");
  return e ? std::atoi(e) : int(std::time(nullptr) / (7 * 86400));
}
// Cambia cada semana (con el hash salian muchas semanas seguidas la misma: medido 2963-2968).
int TorreDeLaSemana(int semana) { return 2 + int(uint32_t(semana) % uint32_t(kNumTorres - 2)); }
// Abyss (27): 3 pisos (rival de cada piso por la semilla) y cartas de mejora
constexpr int kAbyssPisos[3][3] = {{42, 18, 21}, {19, 20, 28}, {33, 27, 36}};
struct Carta {
  int max;
  const char* es;
  const char* en;
  const char* fr;
  const char* de;
  const char* it;
};
const Carta kCartas[] = {
    {2, "Fuerza: +25% de daño", "Strength: +25% damage", "Force : +25% de dégâts", "Kraft: +25% Schaden",
     "Forza: +25% di danni"},
    {2, "Piel de acero: -25% de daño recibido", "Steel skin: -25% damage taken", "Peau d'acier : -25% de dégâts subis",
     "Stahlhaut: -25% erlittener Schaden", "Pelle d'acciaio: -25% di danni subiti"},
    {2, "Vitalidad: +1000 de vida máxima", "Vitality: +1000 max health", "Vitalité : +1000 vie max",
     "Vitalität: +1000 max. Leben", "Vitalità: +1000 vita massima"},
    {1, "Ki eterno: el ki se recarga solo", "Eternal ki: ki recharges by itself", "Ki éternel : le ki se recharge seul",
     "Ewiges Ki: Ki lädt sich selbst auf", "Ki eterno: il ki si ricarica da solo"},
    {1, "Cápsula Rage!!!: +28% de ataque", "Rage!!! capsule: +28% attack", "Capsule Rage!!! : +28% d'attaque",
     "Kapsel Rage!!!: +28% Angriff", "Capsula Rage!!!: +28% attacco"},
};
constexpr int kNumCartas = int(std::size(kCartas));
// Kumite de jefes (28): tres jefes de tamano normal, vida finita, armadura con un hueco de 3 s y golpes duros
struct Jefe {
  int id, vida, armadura, ventana, recibido, objetivo;
  float gigante;   // escala (escenarios.md R1: gigantes.cpp); el 2 es el jefe gigante, con tormenta (clima.cpp)
};
const Jefe kJefes[] = {{33, 5000, 50, 10, 150, 0, 0}, {40, 6000, 60, 8, 175, 0, 1.8f}, {36, 7000, 70, 12, 150, 45, 0}};

// ---- Partida a medias de cada reto y monedero (user_data/dbz3/rpg.toml) ----
struct Run {
  int modo = -1;   // M::Tipo (3-9); -1 = ninguna
  int jugador = 0, traje = 0;
  uint32_t semilla = 0;
  int ganados = 0;
  int vida = 3000, vida_max = 3000;
  int zeni = 0;           // Tienda: Zenie de la partida
  std::string eventos;    // Simulacion: E/X/D por dia; Abyss: cartas elegidas (0-4); Tienda: compras del combate
  int extra = 0;          // Heroe: variante; Torres: torre; Torneo: semifinal 2 ya jugada (1) y su ganador
  int tecnicas = 0;       // Heroe: mascara de kTecnicas
  int bots[3] = {-1, -1, -1};   // Torneo de 4
  int finalista = -1;            // Torneo de 4: ganador de la semifinal 2
};
std::mutex g_mu;
std::map<int, Run> g_runs;   // por modo
int g_zeni = 0;              // monedero del HUB (premios)
int g_tienda_record = 0;     // mas Zenie sin gastar al acabar una partida de la Tienda
int g_heroe_variante = 0, g_heroe_tecnicas = 3;   // receta del heroe (la ultima elegida)
bool g_leido = false, g_solo_lectura = false;
std::atomic<int> g_premio{0};   // Premio() desde modos.cpp (con su candado): se suma aqui despues

std::filesystem::path Ruta() { return dbz3::settings::UserDataRoot() / "rpg.toml"; }

bool Leer(const std::filesystem::path& p, bool* nueva) {
  std::ifstream f(p, std::ios::binary);
  if (!f) return false;
  std::stringstream ss;
  ss << f.rdbuf();
  try {
    const toml::table t = toml::parse(ss.str());
    const int64_t v = t["version"].value_or(int64_t(0));
    if (v > 1) {   // de una version mas nueva del port: no se reinterpreta ni se pisa
      *nueva = true;
      return false;
    }
    if (v != 1) return false;
    g_zeni = std::clamp(int(t["zeni"].value_or(int64_t(0))), 0, 9999999);
    g_tienda_record = int(t["tienda_record"].value_or(int64_t(0)));
    g_heroe_variante = std::clamp(int(t["heroe_variante"].value_or(int64_t(0))), 0, 2);
    g_heroe_tecnicas = int(t["heroe_tecnicas"].value_or(int64_t(3))) & 7;
    g_runs.clear();
    if (const toml::array* a = t["partida"].as_array()) {
      for (const toml::node& n : *a) {
        const toml::table* x = n.as_table();
        if (!x) continue;
        Run r;
        auto i = [&](const char* k, int d) { return int((*x)[k].value_or(int64_t(d))); };
        r.modo = i("modo", -1);
        r.jugador = i("jugador", 0);
        r.traje = i("traje", 0);
        r.semilla = uint32_t((*x)["semilla"].value_or(int64_t(0)));
        r.ganados = i("ganados", 0);
        r.vida_max = std::clamp(i("vida_max", 3000), 1, 7000);
        r.vida = std::clamp(i("vida", r.vida_max), 1, r.vida_max);
        r.zeni = std::max(i("zeni", 0), 0);
        r.eventos = (*x)["eventos"].value_or(std::string());
        r.extra = i("extra", 0);
        r.tecnicas = i("tecnicas", 0);
        r.finalista = i("finalista", -1);
        if (const toml::array* b = (*x)["bots"].as_array()) {
          for (size_t k = 0; k < 3 && k < b->size(); ++k) r.bots[k] = int((*b)[k].value_or(int64_t(-1)));
        }
        if (r.modo > M::kKumite && r.modo < M::kTipos && r.ganados >= 0 && M::EsJugable(r.jugador)) g_runs[r.modo] = r;
      }
    }
    return true;
  } catch (const toml::parse_error&) {
    return false;
  }
}

void Cargar() {   // con g_mu, una vez
  if (g_leido) return;
  g_leido = true;
  const auto p = Ruta();
  bool nueva = false;
  if (Leer(p, &nueva)) return;
  if (nueva) {
    g_solo_lectura = true;
    REXLOG_WARN("dbz3 rpg: {} es de una version mas nueva: no se usa ni se toca", p.string());
    return;
  }
  std::error_code ec;
  if (std::filesystem::exists(p, ec)) {   // roto: se aparta para mirarlo y se prueba la copia
    std::filesystem::rename(p, p.string() + ".roto", ec);
    REXLOG_WARN("dbz3 rpg: {} no se puede leer: apartado a .roto", p.string());
  }
  if (Leer(p.string() + ".bak", &nueva)) REXLOG_WARN("dbz3 rpg: se usa la copia de seguridad {}.bak", p.string());
}

void Guardar() {   // con g_mu
  if (g_solo_lectura) return;
  std::ostringstream o;
  o << "# Retos de rol del HUB (src/rpg.cpp): monedero de Zenie y partidas a medias. No editar con el juego abierto.\n"
       "version = 1\nzeni = "
    << g_zeni << "\ntienda_record = " << g_tienda_record << "\nheroe_variante = " << g_heroe_variante
    << "\nheroe_tecnicas = " << g_heroe_tecnicas << "\n";
  for (const auto& [modo, r] : g_runs) {
    o << "\n[[partida]]\nmodo = " << r.modo << "\njugador = " << r.jugador << "\ntraje = " << r.traje
      << "\nsemilla = " << r.semilla << "\nganados = " << r.ganados << "\nvida = " << r.vida << "\nvida_max = "
      << r.vida_max << "\nzeni = " << r.zeni << "\neventos = " << toml::value<std::string>(r.eventos)
      << "\nextra = " << r.extra << "\ntecnicas = " << r.tecnicas << "\nbots = [" << r.bots[0] << ", " << r.bots[1]
      << ", " << r.bots[2] << "]\nfinalista = " << r.finalista << "\n";
  }
  std::error_code ec;
  const auto p = Ruta();
  std::filesystem::create_directories(p.parent_path(), ec);
  const std::filesystem::path tmp = p.string() + ".tmp", bak = p.string() + ".bak";
  {
    std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
    f << o.str();
    if (!f.flush()) {
      REXLOG_WARN("dbz3 rpg: no se puede escribir {}", tmp.string());
      return;
    }
  }
  if (std::filesystem::exists(p, ec)) std::filesystem::copy_file(p, bak, std::filesystem::copy_options::overwrite_existing, ec);
  std::filesystem::rename(tmp, p, ec);
  if (ec) REXLOG_WARN("dbz3 rpg: no se puede guardar {} ({})", p.string(), ec.message());
}

// ---- Simulacion (23): el estado sale de la semilla y los eventos (lo mismo da lo mismo) ----
struct Sim {
  int dia = 1;              // 1-3; 4 = toca el jefe
  int vida = 3000, vida_max = 3000;
  int fuerza = 0, fatiga = 0;
  std::vector<int> caps;    // capsulas encontradas (equipo de la partida, no de la del juego)
  std::string ultimo_es, ultimo_en, ultimo_fr, ultimo_de, ultimo_it;   // lo que paso el ultimo dia
};
Sim SimDe(const Run& r) {
  Sim s;
  for (const char e : r.eventos) {
    const uint32_t roll = Hash(r.semilla, uint32_t(s.dia)) % 4;
    if (e == 'E') {   // entrenar: +15% de dano, una de fatiga y -300 de vida
      s.fuerza += 15;
      s.fatiga += 1;
      s.vida = std::max(1, s.vida - 300);
      s.ultimo_es = "Entrenamiento duro: +15% de daño, +1 de fatiga.";
      s.ultimo_en = "Hard training: +15% damage, +1 fatigue.";
      s.ultimo_fr = "Entraînement intense : +15% de dégâts, +1 de fatigue.";
      s.ultimo_de = "Hartes Training: +15% Schaden, +1 Erschöpfung.";
      s.ultimo_it = "Allenamento duro: +15% di danni, +1 di fatica.";
    } else if (e == 'X') {   // explorar: tirada con la semilla de la partida
      if (roll == 0) {
        s.caps.push_back(366);
        s.ultimo_es = "Has encontrado la cápsula Potential (+24% de ataque en el combate).";
        s.ultimo_en = "You found the Potential capsule (+24% attack in the fight).";
        s.ultimo_fr = "Tu as trouvé la capsule Potential (+24% d'attaque au combat).";
        s.ultimo_de = "Du hast die Kapsel Potential gefunden (+24% Angriff im Kampf).";
        s.ultimo_it = "Hai trovato la capsula Potential (+24% di attacco nell'incontro).";
      } else if (roll == 1) {
        s.vida_max = std::min(7000, s.vida_max + 500);
        s.vida = std::min(s.vida_max, s.vida + 500);
        s.ultimo_es = "Has encontrado una alubia mágica: +500 de vida máxima.";
        s.ultimo_en = "You found a magic bean: +500 max health.";
        s.ultimo_fr = "Tu as trouvé un haricot magique : +500 vie max.";
        s.ultimo_de = "Du hast eine magische Bohne gefunden: +500 max. Leben.";
        s.ultimo_it = "Hai trovato un fagiolo magico: +500 vita massima.";
      } else if (roll == 2) {
        s.vida = std::max(1, s.vida - 500);
        s.ultimo_es = "¡Una emboscada! Pierdes 500 de vida.";
        s.ultimo_en = "An ambush! You lose 500 health.";
        s.ultimo_fr = "Une embuscade ! Tu perds 500 de vie.";
        s.ultimo_de = "Ein Hinterhalt! Du verlierst 500 Leben.";
        s.ultimo_it = "Un'imboscata! Perdi 500 di vita.";
      } else {
        s.caps.push_back(382);
        s.ultimo_es = "Has encontrado la cápsula Spirit!! (+20% de defensa un rato).";
        s.ultimo_en = "You found the Spirit!! capsule (+20% defense for a while).";
        s.ultimo_fr = "Tu as trouvé la capsule Spirit!! (+20% de défense un moment).";
        s.ultimo_de = "Du hast die Kapsel Spirit!! gefunden (+20% Abwehr für eine Weile).";
        s.ultimo_it = "Hai trovato la capsula Spirit!! (+20% di difesa per un po').";
      }
    } else {   // descansar: +1500 de vida (sin pasar del maximo) y sin fatiga
      s.vida = std::min(s.vida_max, s.vida + 1500);
      s.fatiga = 0;
      s.ultimo_es = "Has descansado: +1500 de vida y sin fatiga.";
      s.ultimo_en = "You rested: +1500 health and no fatigue.";
      s.ultimo_fr = "Tu t'es reposé : +1500 vie et plus de fatigue.";
      s.ultimo_de = "Du hast dich ausgeruht: +1500 Leben und keine Erschöpfung.";
      s.ultimo_it = "Ti sei riposato: +1500 vita e niente fatica.";
    }
    ++s.dia;
  }
  return s;
}

// ---- Abyss (27): cartas elegidas (eventos) y oferta de cada piso (de la semilla: no cambia al reabrir) ----
struct Abyss {
  int stacks[kNumCartas] = {};
};
Abyss AbyssDe(const Run& r) {
  Abyss a;
  for (const char c : r.eventos) {
    if (c >= '0' && c < '0' + kNumCartas) ++a.stacks[c - '0'];
  }
  return a;
}
std::vector<int> Oferta(const Run& r, int piso) {   // hasta 3 cartas que aun no estan al maximo
  const Abyss a = AbyssDe(r);
  std::vector<int> v;
  for (int k = 0; k < kNumCartas; ++k) {
    if (a.stacks[k] < kCartas[k].max) v.push_back(k);
  }
  for (int i = int(v.size()) - 1; i > 0; --i) {
    std::swap(v[size_t(i)], v[Hash(r.semilla + uint32_t(piso), uint32_t(i)) % uint32_t(i + 1)]);
  }
  if (v.size() > 3) v.resize(3);
  return v;
}

// Reglas del combate siguiente de una partida (con g_mu).
M::Reglas ReglasDe(const Run& r) {
  M::Reglas g;
  if (r.modo == M::kSimulacion) {
    const Sim s = SimDe(r);
    g.dano_hecho = 100 + s.fuerza;
    g.dano_recibido = 100 + 10 * s.fatiga;   // la fatiga se paga en el combate
    g.ncaps = int(std::min<size_t>(s.caps.size(), 7));
    for (int i = 0; i < g.ncaps; ++i) g.caps[i] = s.caps[size_t(i)];
  } else if (r.modo == M::kTienda) {
    if (r.eventos.find('1') != std::string::npos) g.dano_hecho = 125;
    if (r.eventos.find('2') != std::string::npos) g.dano_recibido = 75;
  } else if (r.modo == M::kHeroe) {
    const Variante& v = kVariantes[std::clamp(r.extra, 0, 2)];
    g.dano_hecho = v.dano;
    g.dano_recibido = v.defensa;
    g.ncaps = 0;
    for (int k = 0; k < int(std::size(kTecnicas)); ++k) {
      if (r.tecnicas & (1 << k)) g.caps[g.ncaps++] = kTecnicas[k].capsula;
    }
    g.reemplazar = true;   // solo sus dos tecnicas (sin las de la partida de Goku)
  } else if (r.modo == M::kTorres) {
    g = kTorres[std::clamp(r.extra, 0, kNumTorres - 1)].reglas;
  } else if (r.modo == M::kAbyss) {
    const Abyss a = AbyssDe(r);
    g.dano_hecho = 100 + 25 * a.stacks[0];
    g.dano_recibido = 100 - 25 * a.stacks[1];
    if (a.stacks[3] > 0) g.ki_drena = -5;   // el ki sube un 5% del tope por segundo
    if (a.stacks[4] > 0) {
      g.ncaps = 1;
      g.caps[0] = 380;
    }
  } else if (r.modo == M::kJefes) {
    const Jefe& j = kJefes[std::clamp(r.ganados, 0, 2)];
    g.armadura = j.armadura;
    g.ventana = j.ventana;
    g.dano_recibido = j.recibido;
  }
  return g;
}

// Combates de una partida (para StartPropio; con g_mu).
std::vector<M::Pelea> PeleasDe(const Run& r) {
  std::vector<M::Pelea> v;
  auto add = [&v](int id, int vida, int esc, int tiempo = 0, int regen = 0, int objetivo = 0) {
    M::Pelea p;
    p.rival = id;
    p.vida = vida;
    p.escenario = esc;
    p.tiempo = tiempo;
    p.regen = regen;
    p.objetivo = objetivo;
    v.push_back(p);
  };
  switch (r.modo) {
    case M::kSimulacion: add(kSimJefes[r.semilla % 3], 4000, 6, 99); break;
    case M::kTienda:
      for (int k = 0; k < 5; ++k) add(kTiendaRivales[k], 1500 + 400 * k, k + 1);
      break;
    case M::kHeroe:
      for (const Paso& p : kMision) add(p.rival, p.vida, p.escenario);
      break;
    case M::kTorres: {
      const Torre& t = kTorres[std::clamp(r.extra, 0, kNumTorres - 1)];
      for (int k = 0; k < 3; ++k) add(t.rivales[k], 2000 + 500 * k, (r.extra * 3 + k) % 8, t.tiempo, t.regen);
      break;
    }
    case M::kAbyss:
      for (int k = 0; k < 3; ++k) add(kAbyssPisos[k][Hash(r.semilla, 100 + uint32_t(k)) % 3], 2000 + 1000 * k, 7 - k);
      break;
    case M::kJefes:
      for (int k = 0; k < 3; ++k) {
        add(kJefes[k].id, kJefes[k].vida, 3 + k, kJefes[k].objetivo ? 0 : 120, 0, kJefes[k].objetivo);
        v.back().gigante = kJefes[k].gigante;
        v.back().clima = kJefes[k].gigante > 0;
      }
      break;
    case M::kTorneo4:   // semifinal contra el bot 1; la final contra quien gane la otra (SetSiguiente)
      add(r.bots[0], 3000, 0);
      add(r.finalista >= 0 ? r.finalista : r.bots[1], 3000, 0);
      break;
    default: break;
  }
  return v;
}

const char* Palabra(int modo) {
  switch (modo) {
    case M::kTienda: return L("Ronda", "Round", "Manche", "Runde", "Round");
    case M::kHeroe: return L("Misión", "Mission", "Mission", "Mission", "Missione");
    case M::kTorres:
    case M::kAbyss: return L("Piso", "Floor", "Étage", "Etage", "Piano");
    case M::kTorneo4: return L("Ronda", "Round", "Tour", "Runde", "Turno");
    default: return L("Jefe", "Boss", "Boss", "Boss", "Boss");
  }
}
int PremioDe(int modo) {
  switch (modo) {
    case M::kSimulacion: return 500;
    case M::kHeroe: return 700;
    case M::kTorres: return 600;
    case M::kAbyss: return 800;
    case M::kJefes: return 1500;
    case M::kTorneo4: return 600;
    default: return 0;   // Tienda: su Zenie de la partida
  }
}

// ---- Pantallas ----
enum Vista { kCerrada, kSelector, kSimDia, kHeroeP, kTorresP, kCuadro };
Vista g_vista = kCerrada;
int g_entrada = 0, g_sel = 0, g_cursor = 0, g_cursor2 = 0;   // selector, pantallas, torre
int g_jugador = 0, g_traje = 0;
std::string g_aviso;          // lo que dice el bocadillo tras una accion
std::string g_rango;          // rango del jugador (ficha 01), al abrir el selector
int g_reto_visto = -1;        // Estado::ganados ya contado (resultados)
bool g_fin = false;           // pantalla final de un reto propio
bool g_fin_hecho = false;     // ... ya vista (hasta el siguiente StartPropio)
M::Estado g_fin_st;           // ... su estado
Run g_fin_run;
std::string g_fin_texto;

// Retos de cada entrada del HUB: el clasico (0-2) y los nuevos.
const std::vector<int>& Retos(int entrada) {
  static const std::vector<int> k[3] = {{M::kTorre, M::kTorres, M::kAbyss},
                                        {M::kSupervivencia, M::kTienda, M::kSimulacion},
                                        {M::kKumite, M::kJefes, M::kTorneo4, M::kHeroe}};
  return k[std::clamp(entrada, 0, 2)];
}

std::string Descripcion(int modo) {
  switch (modo) {
    case M::kTorre:
    case M::kSupervivencia:
    case M::kKumite:
      return L("El reto de siempre (o el que hayas puesto con el Kit). Si lo dejaste a medias, sigues ahí.",
               "The usual challenge (or the one set with the Kit). If you left it halfway, you continue there.",
               "Le défi habituel (ou celui choisi avec le Kit). Si tu l'as laissé en cours, tu reprends là.",
               "Die übliche Herausforderung (oder die im Kit gewählte). Wenn du mittendrin aufgehört hast, geht es dort weiter.",
               "La solita sfida (o quella scelta con il Kit). Se l'hai lasciata a metà, riprendi da lì.");
    case M::kSimulacion:
      return L("Tres días antes de un gran combate: entrena, explora o descansa. Cada elección gasta un día. "
               "El cuarto día llega el jefe.",
               "Three days before a great battle: train, explore or rest. Each choice uses a day. "
               "On the fourth day the boss arrives.",
               "Trois jours avant un grand combat : entraîne-toi, explore ou repose-toi. Chaque choix coûte un jour. "
               "Le quatrième jour, le boss arrive.",
               "Drei Tage vor einem großen Kampf: trainiere, erkunde oder ruh dich aus. Jede Wahl kostet einen Tag. "
               "Am vierten Tag kommt der Boss.",
               "Tre giorni prima di un grande scontro: allenati, esplora o riposa. Ogni scelta costa un giorno. "
               "Il quarto giorno arriva il boss.");
    case M::kTienda:
      return L("Cinco combates seguidos con la misma vida. Cada victoria da Zenie (más si no te tocan) para la "
               "tienda: curas y mejoras para el combate siguiente.",
               "Five fights in a row with the same health. Each win gives Zenie (more without taking damage) for "
               "the shop: heals and boosts for the next fight.",
               "Cinq combats d'affilée avec la même vie. Chaque victoire rapporte des Zenie (plus sans être touché) "
               "pour la boutique : soins et bonus pour le combat suivant.",
               "Fünf Kämpfe am Stück mit demselben Leben. Jeder Sieg bringt Zenie (mehr ohne Treffer) für den "
               "Laden: Heilung und Boosts für den nächsten Kampf.",
               "Cinque incontri di fila con la stessa vita. Ogni vittoria dà Zenie (di più senza subire danni) "
               "per il negozio: cure e potenziamenti per l'incontro successivo.");
    case M::kHeroe:
      return L("Crea tu héroe: elige una de tres variantes del guerrero Saiyan y dos técnicas, y vive su misión.",
               "Create your hero: pick one of three Saiyan warrior variants and two techniques, and live his mission.",
               "Crée ton héros : choisis une des trois variantes du guerrier Saiyan et deux techniques, et vis sa mission.",
               "Erschaffe deinen Helden: Wähle eine von drei Varianten des Saiyajin-Kriegers und zwei Techniken und erlebe seine Mission.",
               "Crea il tuo eroe: scegli una delle tre varianti del guerriero Saiyan e due tecniche, e vivi la sua missione.");
    case M::kTorres:
      return L("Torres de tres combates con tiempo límite y una regla especial. La de la semana cambia cada lunes.",
               "Three-fight towers with a time limit and a special rule. The weekly one changes every Monday.",
               "Tours de trois combats avec limite de temps et une règle spéciale. Celle de la semaine change chaque lundi.",
               "Türme mit drei Kämpfen, Zeitlimit und einer Sonderregel. Der Wochenturm wechselt jeden Montag.",
               "Torri di tre incontri con limite di tempo e una regola speciale. Quella della settimana cambia ogni lunedì.");
    case M::kAbyss:
      return L("Baja tres pisos. Tras cada victoria eliges una de tres cartas de mejora, solo para esta partida. "
               "Si caes, se acabó.",
               "Go down three floors. After each win you pick one of three upgrade cards, for this run only. "
               "If you fall, it's over.",
               "Descends trois étages. Après chaque victoire, choisis une carte d'amélioration parmi trois, pour "
               "cette partie seulement. Si tu tombes, c'est fini.",
               "Steige drei Etagen hinab. Nach jedem Sieg wählst du eine von drei Verbesserungskarten, nur für "
               "diesen Lauf. Wenn du fällst, ist es vorbei.",
               "Scendi tre piani. Dopo ogni vittoria scegli una di tre carte potenziamento, solo per questa "
               "partita. Se cadi, è finita.");
    case M::kJefes:
      return L("Tres jefes con armadura y golpes duros. Su armadura cae 3 segundos de vez en cuando: ¡aprovéchalo! "
               "Contra el último, aguanta.",
               "Three armored bosses that hit hard. Their armor drops for 3 seconds now and then: use it! "
               "Against the last one, survive.",
               "Trois boss blindés qui frappent fort. Leur armure tombe 3 secondes de temps en temps : profites-en ! "
               "Contre le dernier, tiens bon.",
               "Drei gepanzerte Bosse, die hart zuschlagen. Ihre Panzerung fällt ab und zu für 3 Sekunden: nutze es! "
               "Gegen den letzten musst du durchhalten.",
               "Tre boss corazzati che colpiscono duro. La loro armatura cade per 3 secondi ogni tanto: sfruttalo! "
               "Contro l'ultimo, resisti.");
    case M::kTorneo4:
      return L("Un torneo de cuatro con tres rivales de la máquina: semifinal y final. El otro combate se decide "
               "por la fuerza de cada luchador.",
               "A four-fighter tournament against three CPU rivals: semifinal and final. The other match is "
               "decided by each fighter's strength.",
               "Un tournoi à quatre contre trois adversaires de la console : demi-finale et finale. L'autre combat "
               "se décide selon la force de chaque combattant.",
               "Ein Turnier zu viert gegen drei CPU-Gegner: Halbfinale und Finale. Der andere Kampf wird nach der "
               "Stärke der Kämpfer entschieden.",
               "Un torneo a quattro contro tre avversari della CPU: semifinale e finale. L'altro incontro si decide "
               "in base alla forza di ogni lottatore.");
    default: return {};
  }
}

std::string Lang(const char* es, const char* en, const char* fr, const char* de, const char* it) {
  return L(es, en, fr, de, it);
}

// ---- Arranque de los combates ----
// Lanza el combate `r.ganados` de la partida (con g_mu). Primera vez: StartPropio; despues, Continue.
bool Lanzar(Run& r, bool primero, bool pausado = false) {
  const M::Reglas g = ReglasDe(r);
  bool ok;
  if (primero) {
    ok = M::StartPropio(r.modo, M::NombreModo(r.modo), Palabra(r.modo), r.jugador, r.traje, r.vida, r.vida_max,
                        PeleasDe(r), r.ganados, r.modo == M::kTorres || r.modo == M::kJefes, PremioDe(r.modo),
                        pausado);
    if (ok) M::SetReglas(g);   // tras StartPropio: es para el combate que se acaba de pedir
    if (ok) g_fin_hecho = false;
  } else {
    M::SetVida(r.vida, r.vida_max);
    M::SetReglas(g);
    ok = M::Continue();
  }
  REXLOG_INFO("dbz3 rpg: {} combate {}: {} (vida {}/{}, dano {}%, recibido {}%, armadura {}%, ki {}/s, {} capsulas)",
              M::NombreModo(r.modo), r.ganados + 1, !ok ? "no se pudo pedir" : pausado ? "reanudada (aun sin pedir)" : "pedido", r.vida, r.vida_max,
              g.dano_hecho, g.dano_recibido, g.armadura, g.ki_drena, g.ncaps);
  return ok;
}

uint32_t Semilla() {
  return uint32_t(std::chrono::steady_clock::now().time_since_epoch().count()) ^ uint32_t(std::time(nullptr));
}

// Empieza (o sigue) un reto del selector (con g_mu).
void Empezar(int modo) {
  g_aviso.clear();
  if (modo <= M::kKumite) {
    if (M::Start(modo, g_jugador, g_traje)) g_vista = kCerrada;
    else g_aviso = L("No se pudo empezar el reto.", "The challenge could not start.", "Le défi n'a pas pu commencer.",
                     "Die Herausforderung konnte nicht starten.", "Impossibile avviare la sfida.");
    return;
  }
  auto it = g_runs.find(modo);
  const bool sigue = it != g_runs.end();
  if (!sigue) {   // partida nueva
    Run r;
    r.modo = modo;
    r.jugador = g_jugador;
    r.traje = g_traje;
    r.semilla = Semilla();
    if (modo == M::kHeroe) {
      r.jugador = kHeroeId;
      r.extra = g_heroe_variante;
      r.traje = kVariantes[r.extra].traje;
      r.tecnicas = g_heroe_tecnicas;
      r.vida = r.vida_max = kVariantes[r.extra].vida;
    } else if (modo == M::kTorres) {
      r.extra = g_cursor2 == 0 ? TorreDeLaSemana(SemanaUtc()) : g_cursor2 - 1;
      r.semilla = uint32_t(SemanaUtc());   // la torre queda fija hasta acabar, aunque cambie la semana
    } else if (modo == M::kTorneo4) {
      std::vector<int> pool;
      for (int id = 0; id < 44; ++id) {
        if (id != r.jugador && M::EsJugable(id) && id != 42 && id != 43) pool.push_back(id);
      }
      for (int k = 0; k < 3 && !pool.empty(); ++k) {
        const size_t i = Hash(r.semilla, uint32_t(k)) % pool.size();
        r.bots[k] = pool[i];
        pool.erase(pool.begin() + std::ptrdiff_t(i));
      }
    }
    it = g_runs.emplace(modo, r).first;
    Guardar();
    REXLOG_INFO("dbz3 rpg: {}: partida nueva (id {}, semilla {})", M::NombreModo(modo), r.jugador, r.semilla);
  }
  Run& r = it->second;
  if (sigue && r.ganados > 0 && modo != M::kSimulacion) {   // reanudar: primero su pantalla entre combates
    g_cursor = 0;
    if (Lanzar(r, true, true)) g_vista = kCerrada;
    return;
  }
  switch (modo) {
    case M::kSimulacion:
      g_vista = kSimDia;
      g_cursor = 0;
      break;
    case M::kTorneo4:
      g_vista = kCuadro;
      break;
    default:
      if (Lanzar(r, true)) g_vista = kCerrada;
      break;
  }
}

// ---- Resultados: lo que pasa en modos.cpp se apunta aqui (con g_mu, al volver al menu) ----
void Sincronizar() {
  Cargar();
  if (const int p = g_premio.exchange(0); p > 0) {
    g_zeni = std::min(g_zeni + p, 9999999);
    Guardar();
    REXLOG_INFO("dbz3 rpg: premio de {} Zenie (monedero {})", p, g_zeni);
  }
  const M::Estado st = M::Get();
  if (!st.propio) return;
  auto it = g_runs.find(st.tipo);
  if (it == g_runs.end()) return;
  Run& r = it->second;
  if (st.fase == M::kEntre && st.ganados > r.ganados) {   // victoria con mas combates por delante
    r.ganados = st.ganados;
    r.vida = st.vida;
    r.vida_max = st.vida_max;
    if (r.modo == M::kTienda) {
      const int z = kZeniVictoria + (st.recibido == 0 ? kZeniPerfecto : 0);
      r.zeni += z;
      r.eventos.clear();   // las mejoras compradas eran para ese combate
      g_aviso = Lang("¡Victoria! +", "Victory! +", "Victoire ! +", "Sieg! +", "Vittoria! +") + Num(z) + " Zenie" +
                (st.recibido == 0 ? Lang(" (perfecto)", " (perfect)", " (parfait)", " (perfekt)", " (perfetto)") : "");
    } else if (r.modo == M::kJefes) {
      r.vida = r.vida_max;   // cada jefe con la vida llena
    } else if (r.modo == M::kAbyss) {
      r.vida = std::min(r.vida_max, r.vida + 300);
    } else if (r.modo == M::kTorneo4) {   // la otra semifinal: por la fuerza (Elo) y la semilla
      std::vector<M::Puesto> top(64);
      const int n = M::Ranking(top.data(), 64);
      auto elo = [&](int id) {
        for (int i = 0; i < n; ++i) {
          if (top[size_t(i)].id == id) return double(top[size_t(i)].elo);
        }
        return 1000.0;
      };
      const double p2 = 1.0 / (1.0 + std::pow(10.0, (elo(r.bots[2]) - elo(r.bots[1])) / 400.0));
      r.finalista = (Hash(r.semilla, 77) % 1000) < uint32_t(p2 * 1000) ? r.bots[1] : r.bots[2];
      M::Pelea f;
      f.rival = r.finalista;
      f.vida = 3000;
      M::SetSiguiente(f);
    }
    Guardar();
    REXLOG_INFO("dbz3 rpg: {}: combate {} ganado (vida {}/{}, dano recibido {}, zeni {})", M::NombreModo(r.modo),
                r.ganados, r.vida, r.vida_max, st.recibido, r.zeni);
  } else if (st.fase == M::kFin && !g_fin && !g_fin_hecho) {   // fin del reto: su pantalla, y la partida se cierra (o se guarda)
    g_fin = true;
    g_fin_st = st;
    g_fin_run = r;
    g_cursor = 0;
    if (st.resultado == M::kGuardado || st.resultado == M::kAbandono) {   // se sigue la proxima vez
      r.ganados = st.ganados;
    } else {
      if (r.modo == M::kTienda) g_tienda_record = std::max(g_tienda_record, r.zeni);
      if (r.modo == M::kTorneo4) {   // campeon (extra): el jugador, o el resto del cuadro por la fuerza
        if (r.finalista < 0) r.finalista = (Hash(r.semilla, 77) & 1) ? r.bots[1] : r.bots[2];
        r.extra = st.resultado == M::kVictoria ? r.jugador
                  : st.ganados == 0           ? ((Hash(r.semilla, 78) & 1) ? r.bots[0] : r.finalista)
                                              : r.finalista;
        g_fin_run = r;
      }
      g_runs.erase(it);
    }
    Guardar();
    REXLOG_INFO("dbz3 rpg: fin de {} ({}; {} de {})", M::NombreModo(st.tipo), st.resultado, st.ganados, st.total);
  }
}

// ---- Dibujo ----
void Lista(ImDrawList* dl, const S::Area& a, float y0, const std::vector<std::string>& filas, int cursor, float t,
           const std::vector<bool>& apagadas = {}, float letra = 34.0f, float x0 = 300, float x1 = 900) {
  const float paso = 54;
  S::Frame(dl, a.At(x0, y0), a.At(x1, y0 + 20 + paso * float(filas.size())), a.s);
  for (size_t i = 0; i < filas.size(); ++i) {
    const float y = y0 + 10 + paso * (float(i) + 0.5f);
    const bool sel = int(i) == cursor, off = i < apagadas.size() && apagadas[i];
    if (sel) S::Highlight(dl, a.At(x0, y - paso * 0.5f + 2), a.At(x1, y + paso * 0.5f - 2), t);
    S::MenuText(dl, a.At((x0 + x1) * 0.5f, y), letra * a.s, filas[i].c_str(), sel, off);
  }
}
void Lineas(ImDrawList* dl, const S::Area& a, float y0, const std::vector<std::string>& l, float letra = 30.0f) {
  const float paso = 46;
  S::Frame(dl, a.At(300, y0), a.At(900, y0 + 16 + paso * float(l.size())), a.s);
  for (size_t i = 0; i < l.size(); ++i) {
    S::MenuText(dl, a.At(600, y0 + 8 + paso * (float(i) + 0.5f)), (i == 0 ? letra + 6 : letra) * a.s, l[i].c_str(),
                i == 0, false);
  }
}
void Bocadillo(ImDrawList* dl, const S::Area& a, const std::string& s) {
  S::Speech(dl, a.At(200, 640), a.At(840, 845), a.s, s.c_str(), 0.12f);
}
void Cara(ImDrawList* dl, const S::Area& a, int id, float x, float y, float h) {
  if (const S::Pic p = S::Img(("caras/" + Num(id)).c_str())) {
    const float w = h * p.w / std::max(p.h, 1.0f);
    S::DrawPic(dl, p, a.At(x - w * 0.5f, y - h * 0.5f), a.At(x + w * 0.5f, y + h * 0.5f));
  }
}
const char* Aceptar() { return L("Aceptar", "OK", "Valider", "OK", "Conferma"); }
const char* Atras() { return L("Atrás", "Back", "Retour", "Zurück", "Indietro"); }
std::string Vida(int v, int m) { return Lang("Vida ", "Health ", "Vie ", "Leben ", "Vita ") + Num(v) + " / " + Num(m); }

// Siguiente luchador jugable (dir +1/-1) para el selector.
int Siguiente(int id, int dir) {
  for (int k = 0; k < 64; ++k) {
    id = (id + dir + 64) % 64;
    if (M::EsJugable(id)) return id;
  }
  return id;
}

void DibujarSelector(ImDrawList* dl, const S::Area& a, float t, uint16_t p) {
  const std::vector<int>& lista = Retos(g_entrada);
  const int n = int(lista.size());
  g_sel = std::clamp(g_sel, 0, n - 1);
  if (p & kUp) g_sel = (g_sel + n - 1) % n;
  if (p & kDown) g_sel = (g_sel + 1) % n;
  const int modo = lista[size_t(g_sel)];
  const bool torres = modo == M::kTorres;
  if (torres && g_runs.find(modo) == g_runs.end()) {   // LB/RB: torre de la semana o una fija
    if (p & kLb) g_cursor2 = (g_cursor2 + 2) % 3;
    if (p & kRb) g_cursor2 = (g_cursor2 + 1) % 3;
  }
  if (modo != M::kHeroe) {   // izquierda/derecha: el luchador (el heroe es el suyo)
    if (p & kLeft) g_jugador = Siguiente(g_jugador, -1), g_traje = 0;
    if (p & kRight) g_jugador = Siguiente(g_jugador, +1), g_traje = 0;
  } else {   // izquierda/derecha: la variante; LB/RB: las tecnicas (dos de tres)
    if (p & kLeft) g_heroe_variante = (g_heroe_variante + 2) % 3;
    if (p & kRight) g_heroe_variante = (g_heroe_variante + 1) % 3;
    static const int kPares[3] = {3, 5, 6};
    int k = 0;
    while (k < 3 && kPares[k] != g_heroe_tecnicas) ++k;
    if (p & kLb) g_heroe_tecnicas = kPares[(k + 2) % 3];
    if (p & kRb) g_heroe_tecnicas = kPares[(k + 1) % 3];
  }
  if (p & kB) {
    Guardar();
    g_vista = kCerrada;
    return;
  }
  if (p & kA) {
    Guardar();   // la receta del heroe
    Empezar(modo);
    if (g_vista != kSelector) return;
  }
  const char* titulos[3] = {M::NombreModo(M::kTorre), M::NombreModo(M::kSupervivencia), M::NombreModo(M::kKumite)};
  S::Banner(dl, a, titulos[std::clamp(g_entrada, 0, 2)]);
  // arriba: el luchador (o la variante del heroe), con las flechas del juego
  const ImVec2 g0 = a.At(420, 150), g1 = a.At(870, 214);
  S::Frame(dl, g0, g1, a.s);
  std::string quien;
  if (modo == M::kHeroe) {
    const Variante& v = kVariantes[g_heroe_variante];
    quien = L(v.es, v.en, v.fr, v.de, v.it);
  } else {
    quien = M::Nombre(g_jugador);
  }
  Cara(dl, a, modo == M::kHeroe ? kHeroeId : g_jugador, 470, 182, 62);
  S::MenuText(dl, ImVec2((g0.x + g1.x) * 0.5f + 30 * a.s, (g0.y + g1.y) * 0.5f), 34 * a.s, quien.c_str(), true, false);
  S::Arrows(dl, ImVec2(g0.x - 18 * a.s, g0.y), ImVec2(g1.x + 18 * a.s, g1.y), 40 * a.s, t);
  // la lista de retos
  std::vector<std::string> filas;
  for (const int m : lista) {
    std::string f = M::NombreModo(m);
    int gid = 0, gg = 0;
    if (g_runs.count(m) || (m <= M::kKumite && M::Guardado(m, &gid, &gg))) f += Lang("  (a medias)", "  (in progress)", "  (en cours)", "  (läuft)", "  (in corso)");
    filas.push_back(f);
  }
  Lista(dl, a, 232, filas, g_sel, t);
  // bocadillo: que es, record y Zenie (y la torre / las tecnicas)
  std::string s = Descripcion(modo);
  if (torres) {
    const int sem = SemanaUtc(), tw = TorreDeLaSemana(sem);
    const int elegida = g_runs.count(modo) ? g_runs[modo].extra : g_cursor2 == 0 ? tw : g_cursor2 - 1;
    const Torre& tr = kTorres[elegida];
    s += "\n" + Lang("LB/RB: ", "LB/RB: ", "LB/RB : ", "LB/RB: ", "LB/RB: ") +
         (g_cursor2 == 0 && !g_runs.count(modo) ? Lang("de la semana: ", "weekly: ", "de la semaine : ", "Wochenturm: ",
                                                        "della settimana: ")
                                                : std::string()) +
         L(tr.es, tr.en, tr.en, tr.en, tr.en) + " (" + L(tr.mod_es, tr.mod_en, tr.mod_fr, tr.mod_de, tr.mod_it) + ")";
  } else if (modo == M::kHeroe) {
    std::string tec;
    for (int k = 0; k < 3; ++k) {
      if (g_heroe_tecnicas & (1 << k)) tec += std::string(tec.empty() ? "" : " + ") + kTecnicas[k].nombre;
    }
    s += "\n" + Lang("LB/RB técnicas: ", "LB/RB techniques: ", "LB/RB techniques : ", "LB/RB Techniken: ",
                     "LB/RB tecniche: ") + tec;
  }
  int id = 0, mejor = 0, total = 0, comp = 0;
  if (M::Record(modo, &id, &mejor, &total, &comp)) {
    s += "\n" + Lang("Récord: ", "Record: ", "Record : ", "Rekord: ", "Record: ") + Num(mejor) + "/" + Num(total) + " (" +
         M::Nombre(id) + ")" + (comp > 0 ? "  x" + Num(comp) : "");
  }
  if (modo == M::kTienda && g_tienda_record > 0) {
    s += Lang("  Zenie: ", "  Zenie: ", "  Zenie : ", "  Zenie: ", "  Zenie: ") + Num(g_tienda_record);
  }
  s += "\n" + Lang("Monedero: ", "Wallet: ", "Porte-monnaie : ", "Geldbörse: ", "Portafoglio: ") + Num(g_zeni) + " Zenie";
  if (!g_rango.empty()) s += "   " + g_rango;
  if (g_solo_lectura) {
    s += "\n" + Lang("(rpg.toml es de una versión más nueva: no se guarda)", "(rpg.toml is from a newer version: not saved)",
                     "(rpg.toml vient d'une version plus récente : non enregistré)",
                     "(rpg.toml stammt aus einer neueren Version: wird nicht gespeichert)",
                     "(rpg.toml è di una versione più recente: non viene salvato)");
  }
  if (!g_aviso.empty()) s = g_aviso;
  Bocadillo(dl, a, s);
  S::HelpLine(dl, a.At(600, 868), 38 * a.s,
              {{"A", L("Empezar", "Start", "Commencer", "Starten", "Inizia")}, {"B", Atras()}});
}

// Simulacion: un dia (entrenar / explorar / descansar) o, gastados los tres, el jefe.
void DibujarSim(ImDrawList* dl, const S::Area& a, float t, uint16_t p) {
  auto it = g_runs.find(M::kSimulacion);
  if (it == g_runs.end()) {
    g_vista = kSelector;
    return;
  }
  Run& r = it->second;
  const Sim s = SimDe(r);
  const bool jefe = s.dia > kSimDias;
  std::vector<std::string> ops;
  if (jefe) {
    ops = {Lang("¡Al combate contra ", "To battle against ", "Au combat contre ", "In den Kampf gegen ",
                "Allo scontro con ") + M::Nombre(kSimJefes[r.semilla % 3]) + "!"};
  } else {
    ops = {L("Entrenar", "Train", "S'entraîner", "Trainieren", "Allenarsi"),
           L("Explorar", "Explore", "Explorer", "Erkunden", "Esplorare"),
           L("Descansar", "Rest", "Se reposer", "Ausruhen", "Riposare")};
  }
  const int n = int(ops.size());
  g_cursor = std::clamp(g_cursor, 0, n - 1);
  if (p & kUp) g_cursor = (g_cursor + n - 1) % n;
  if (p & kDown) g_cursor = (g_cursor + 1) % n;
  if (p & kB) {   // la partida queda guardada en ese dia
    g_vista = kSelector;
    return;
  }
  static float ultima = -10.0f;
  if ((p & kA) && t - ultima < 0.5f) {
    REXLOG_INFO("dbz3 rpg: simulacion: pulsacion repetida ignorada");
    p &= ~kA;
  }
  if (p & kA) {
    ultima = t;
    if (jefe) {
      r.vida = s.vida;
      r.vida_max = s.vida_max;
      if (Lanzar(r, true)) g_vista = kCerrada;
      return;
    }
    r.eventos += "EXD"[g_cursor];   // un dia por eleccion (una pulsacion = un evento)
    Guardar();
    const Sim d = SimDe(r);
    g_aviso = L(d.ultimo_es.c_str(), d.ultimo_en.c_str(), d.ultimo_fr.c_str(), d.ultimo_de.c_str(), d.ultimo_it.c_str());
    REXLOG_INFO("dbz3 rpg: simulacion: dia {} -> {} (vida {}/{}, fuerza +{}%, fatiga {}, {} capsulas)", s.dia,
                "EXD"[g_cursor], d.vida, d.vida_max, d.fuerza, d.fatiga, d.caps.size());
    g_cursor = 0;
    return;
  }
  S::Banner(dl, a, M::NombreModo(M::kSimulacion));
  const std::string dia = jefe ? Lang("¡Ha llegado el día!", "The day has come!", "Le jour est venu !",
                                      "Der Tag ist gekommen!", "È arrivato il giorno!")
                               : Lang("Día ", "Day ", "Jour ", "Tag ", "Giorno ") + Num(s.dia) + " / " + Num(kSimDias);
  Lineas(dl, a, 150, {dia, Vida(s.vida, s.vida_max) + "   " + Lang("Fuerza +", "Strength +", "Force +", "Kraft +", "Forza +") +
                               Num(s.fuerza) + "%   " + Lang("Fatiga ", "Fatigue ", "Fatigue ", "Erschöpfung ", "Fatica ") +
                               Num(s.fatiga)});
  Lista(dl, a, 290, ops, g_cursor, t);
  std::string b;
  if (!g_aviso.empty()) b = g_aviso + "\n";
  if (jefe) {
    b += Lang("La fatiga hace que recibas más daño (+10% cada punto). ", "Fatigue makes you take more damage (+10% each). ",
              "La fatigue te fait subir plus de dégâts (+10% par point). ",
              "Erschöpfung lässt dich mehr Schaden nehmen (+10% pro Punkt). ",
              "La fatica ti fa subire più danni (+10% per punto). ");
  } else if (g_cursor == 0) {
    b += Lang("Entrenar: +15% de daño, +1 de fatiga y -300 de vida.", "Train: +15% damage, +1 fatigue and -300 health.",
              "S'entraîner : +15% de dégâts, +1 de fatigue et -300 de vie.",
              "Trainieren: +15% Schaden, +1 Erschöpfung und -300 Leben.",
              "Allenarsi: +15% di danni, +1 di fatica e -300 di vita.");
  } else if (g_cursor == 1) {
    b += Lang("Explorar: puedes encontrar cápsulas o vida... o una emboscada.",
              "Explore: you may find capsules or health... or an ambush.",
              "Explorer : tu peux trouver des capsules ou de la vie... ou une embuscade.",
              "Erkunden: Du kannst Kapseln oder Leben finden... oder einen Hinterhalt.",
              "Esplorare: puoi trovare capsule o vita... o un'imboscata.");
  } else {
    b += Lang("Descansar: +1500 de vida y quita la fatiga.", "Rest: +1500 health and removes fatigue.",
              "Se reposer : +1500 de vie et enlève la fatigue.", "Ausruhen: +1500 Leben und keine Erschöpfung mehr.",
              "Riposare: +1500 di vita e toglie la fatica.");
  }
  if (!s.caps.empty()) {
    b += Lang("  Cápsulas: ", "  Capsules: ", "  Capsules : ", "  Kapseln: ", "  Capsule: ") + Num(int(s.caps.size()));
  }
  Bocadillo(dl, a, b);
  S::HelpLine(dl, a.At(600, 868), 38 * a.s, {{"A", Aceptar()}, {"B", L("Guardar y salir", "Save and exit",
                                                                         "Sauver et quitter", "Speichern und raus",
                                                                         "Salva ed esci")}});
}

// Torneo de 4: el cuadro (semifinales y final) con las caras del juego.
void DibujarCuadro(ImDrawList* dl, const S::Area& a, float t, uint16_t p, const Run& r, bool fin, bool campeon) {
  S::Banner(dl, a, M::NombreModo(M::kTorneo4));
  const int sf1 = r.ganados >= 1 || campeon ? r.jugador : (fin ? r.bots[0] : -1);   // ganador de la semifinal 1
  const int champ = fin ? r.extra : -1;
  struct Caja {
    int id;
    float x, y;
    bool gana;
  };
  constexpr float y1 = 225, y2 = 345, ym = (y1 + y2) * 0.5f;   // semifinales arriba y abajo; la final en medio
  const Caja cajas[] = {{r.jugador, 330, y1, sf1 == r.jugador}, {r.bots[0], 330, y2, sf1 == r.bots[0]},
                        {r.bots[1], 870, y1, r.finalista == r.bots[1]}, {r.bots[2], 870, y2, r.finalista == r.bots[2]}};
  const ImU32 oro = IM_COL32(255, 210, 90, 255), gris = IM_COL32(200, 200, 200, 160);
  for (const Caja& c : cajas) {
    const bool izq = c.x < 600;
    const ImVec2 p0 = a.At(c.x - 140, c.y - 40), p1 = a.At(c.x + 140, c.y + 40);
    S::Frame(dl, p0, p1, a.s);
    Cara(dl, a, c.id, izq ? c.x - 100 : c.x + 100, c.y, 58);
    S::MenuText(dl, a.At(c.x + (izq ? 25 : -25), c.y), 26 * a.s, M::Nombre(c.id).c_str(), c.gana, false);
    const float xe = izq ? c.x + 140 : c.x - 140, xm = izq ? 470 : 730;
    dl->AddLine(a.At(xe, c.y), a.At(xm, c.y), c.gana ? oro : gris, 4 * a.s);
  }
  dl->AddLine(a.At(470, y1), a.At(470, y2), sf1 >= 0 ? oro : gris, 4 * a.s);
  dl->AddLine(a.At(730, y1), a.At(730, y2), r.finalista >= 0 ? oro : gris, 4 * a.s);
  dl->AddLine(a.At(470, ym), a.At(540, ym), sf1 >= 0 ? oro : gris, 4 * a.s);
  dl->AddLine(a.At(660, ym), a.At(730, ym), r.finalista >= 0 ? oro : gris, 4 * a.s);
  S::Frame(dl, a.At(540, ym - 40), a.At(660, ym + 40), a.s);
  if (champ >= 0) Cara(dl, a, champ, 600, ym, 66);
  else S::MenuText(dl, a.At(600, ym), 36 * a.s, "VS", true, false);
  (void)t;
  (void)p;
}

void DibujarTorneo(ImDrawList* dl, const S::Area& a, float t, uint16_t p) {
  auto it = g_runs.find(M::kTorneo4);
  if (it == g_runs.end()) {
    g_vista = kSelector;
    return;
  }
  Run& r = it->second;
  if (p & kB) {
    g_vista = kSelector;
    return;
  }
  if (p & kA) {
    if (Lanzar(r, true)) g_vista = kCerrada;
    return;
  }
  DibujarCuadro(dl, a, t, p, r, false, false);
  Lineas(dl, a, 410, {Lang("Semifinal: ", "Semifinal: ", "Demi-finale : ", "Halbfinale: ", "Semifinale: ") +
                          M::Nombre(r.jugador) + " - " + M::Nombre(r.bots[0])});
  Bocadillo(dl, a, Descripcion(M::kTorneo4));
  S::HelpLine(dl, a.At(600, 868), 38 * a.s, {{"A", L("¡A luchar!", "Fight!", "Au combat !", "Kämpfen!", "Combatti!")},
                                             {"B", Atras()}});
}

// Entre combates de un reto propio: tienda, cartas, cuadro o el aviso del siguiente; A = siguiente combate.
void DibujarEntre(ImDrawList* dl, const S::Area& a, float t, uint16_t p, const M::Estado& st, Run& r) {
  S::Banner(dl, a, M::NombreModo(r.modo));
  const bool cargando = st.fase == M::kCombate;   // ya pedido: el juego esta entrando en el combate
  if (cargando) p = 0;
  const std::string siguiente = Lang("Siguiente: ", "Next: ", "Suivant : ", "Nächster: ", "Prossimo: ") +
                                M::Nombre(st.rival) + "  (" + dbz3::escenarios::Nombre(st.escenario) + ")";
  const std::string paso = std::string(Palabra(r.modo)) + " " + Num(std::min(r.ganados + 1, st.total)) + " / " + Num(st.total);
  std::string b = g_aviso;
  auto mas = [&b](const std::string& s) { b += (b.empty() ? "" : "\n") + s; };
  if (r.modo == M::kTienda) {
    // tienda: arriba/abajo, A compra; la ultima fila sigue
    std::vector<std::string> filas;
    std::vector<bool> off;
    for (int k = 0; k < 3; ++k) {
      const Articulo& ar = kArticulos[k];
      filas.push_back(std::string(L(ar.es, ar.en, ar.fr, ar.de, ar.it)) + "  " + Num(ar.precio) + " Z");
      const bool comprado = k > 0 && r.eventos.find(char('0' + k)) != std::string::npos;
      off.push_back(r.zeni < ar.precio || comprado || (k == 0 && r.vida >= r.vida_max));
    }
    filas.push_back(L("Siguiente combate", "Next fight", "Combat suivant", "Nächster Kampf", "Prossimo incontro"));
    off.push_back(false);
    const int n = int(filas.size());
    g_cursor = std::clamp(g_cursor, 0, n - 1);
    if (p & kUp) g_cursor = (g_cursor + n - 1) % n;
    if (p & kDown) g_cursor = (g_cursor + 1) % n;
    if ((p & kA) && g_cursor < 3) {
      const Articulo& ar = kArticulos[g_cursor];
      if (off[size_t(g_cursor)]) {
        g_aviso = Lang("No puedes comprarlo ahora.", "You can't buy that now.", "Tu ne peux pas l'acheter maintenant.",
                       "Das kannst du jetzt nicht kaufen.", "Non puoi comprarlo ora.");
      } else {   // una compra, un recibo: precio fuera, efecto dentro y se guarda ya
        r.zeni -= ar.precio;
        if (g_cursor == 0) r.vida = std::min(r.vida_max, r.vida + 1000);
        else r.eventos += char('0' + g_cursor);
        Guardar();
        g_aviso = Lang("Comprado: ", "Bought: ", "Acheté : ", "Gekauft: ", "Comprato: ") + L(ar.es, ar.en, ar.fr, ar.de, ar.it);
        REXLOG_INFO("dbz3 rpg: tienda: compra {} por {} (quedan {} Zenie, vida {}/{})", g_cursor, ar.precio, r.zeni,
                    r.vida, r.vida_max);
      }
      p = 0;
    }
    Lineas(dl, a, 150, {paso + "   " + Vida(r.vida, r.vida_max) + "   " + Num(r.zeni) + " Zenie"});
    Lista(dl, a, 230, filas, g_cursor, t, off, 30.0f, 220, 980);
    mas(siguiente);
    if (g_cursor == 3 && (p & kA)) p = kA;   // sigue
    else p &= ~kA;
  } else if (r.modo == M::kAbyss) {
    const std::vector<int> oferta = Oferta(r, r.ganados);
    const bool elegida = int(r.eventos.size()) >= r.ganados || oferta.empty();
    if (!elegida) {   // elegir una carta (una vez por piso; queda guardada)
      const int n = int(oferta.size());
      g_cursor = std::clamp(g_cursor, 0, n - 1);
      if (p & kUp) g_cursor = (g_cursor + n - 1) % n;
      if (p & kDown) g_cursor = (g_cursor + 1) % n;
      if (p & kA) {
        const int c = oferta[size_t(g_cursor)];
        r.eventos += char('0' + c);
        if (c == 2) {
          r.vida_max = std::min(7000, r.vida_max + 1000);
          r.vida = std::min(r.vida_max, r.vida + 1000);
        }
        Guardar();
        g_aviso = Lang("Carta: ", "Card: ", "Carte : ", "Karte: ", "Carta: ") +
                  L(kCartas[c].es, kCartas[c].en, kCartas[c].fr, kCartas[c].de, kCartas[c].it);
        REXLOG_INFO("dbz3 rpg: abyss: piso {}: carta {} (cartas {})", r.ganados, c, r.eventos);
        g_cursor = 0;
        p = 0;
      } else {
        p &= ~kA;
      }
      std::vector<std::string> filas;
      for (const int c : oferta) filas.push_back(L(kCartas[c].es, kCartas[c].en, kCartas[c].fr, kCartas[c].de, kCartas[c].it));
      Lineas(dl, a, 150, {paso + "   " + Vida(r.vida, r.vida_max)});
      Lista(dl, a, 230, filas, g_cursor, t, {}, 30.0f, 220, 980);
      mas(Lang("Elige una carta: dura hasta el final de esta partida.", "Pick a card: it lasts until the end of this run.",
               "Choisis une carte : elle dure jusqu'à la fin de cette partie.",
               "Wähle eine Karte: Sie gilt bis zum Ende dieses Laufs.",
               "Scegli una carta: dura fino alla fine di questa partita."));
    } else {
      const Abyss ab = AbyssDe(r);
      std::vector<std::string> l = {paso, Vida(r.vida, r.vida_max), siguiente};
      std::string cartas;
      for (int k = 0; k < kNumCartas; ++k) {
        if (ab.stacks[k]) {
          cartas += std::string(cartas.empty() ? "" : ", ") + L(kCartas[k].es, kCartas[k].en, kCartas[k].fr, kCartas[k].de,
                                                                kCartas[k].it) + (ab.stacks[k] > 1 ? " x2" : "");
        }
      }
      Lineas(dl, a, 150, l);
      if (!cartas.empty()) mas(cartas);
    }
  } else if (r.modo == M::kTorneo4) {
    DibujarCuadro(dl, a, t, p, r, false, false);
    Lineas(dl, a, 410, {Lang("Final: ", "Final: ", "Finale : ", "Finale: ", "Finale: ") + M::Nombre(r.jugador) + " - " +
                        M::Nombre(r.finalista)});
    mas(M::Nombre(r.finalista) + Lang(" ha ganado la otra semifinal.", " won the other semifinal.",
                                      " a gagné l'autre demi-finale.", " hat das andere Halbfinale gewonnen.",
                                      " ha vinto l'altra semifinale."));
  } else {   // Heroe, Torres, Jefes, Simulacion: el siguiente combate
    std::vector<std::string> l = {paso, siguiente, Vida(r.vida, r.vida_max)};
    if (r.modo == M::kTorres) {
      const Torre& tr = kTorres[std::clamp(r.extra, 0, kNumTorres - 1)];
      l.push_back(L(tr.mod_es, tr.mod_en, tr.mod_fr, tr.mod_de, tr.mod_it));
    } else if (r.modo == M::kJefes) {
      const Jefe& j = kJefes[std::clamp(r.ganados, 0, 2)];
      l.push_back(j.objetivo ? Lang("Objetivo: aguanta ", "Goal: survive ", "Objectif : tiens ", "Ziel: überlebe ",
                                    "Obiettivo: resisti ") + Num(j.objetivo) + " s"
                             : Lang("Armadura ", "Armor ", "Armure ", "Panzerung ", "Armatura ") + Num(j.armadura) + "%" +
                                   (j.gigante > 0 ? Lang("  -  ¡Gigante!", "  -  Giant!", "  -  Géant !", "  -  Riesig!",
                                                         "  -  Gigante!")
                                                  : std::string()));
    }
    Lineas(dl, a, 150, l);
    if (r.modo == M::kHeroe) {
      const Paso& m = kMision[std::clamp(r.ganados, 0, 2)];
      mas(L(m.es, m.en, m.fr, m.de, m.it));
    }
  }
  if (cargando) {
    mas(Lang("¡Prepárate!", "Get ready!", "Prépare-toi !", "Mach dich bereit!", "Preparati!"));
  } else {
    if (b.empty()) {
      mas(Lang("¡Prepárate para el siguiente combate! Si sales ahora, la próxima vez sigues aquí.",
               "Get ready for the next fight! If you leave now, next time you continue here.",
               "Prépare-toi pour le prochain combat ! Si tu pars maintenant, tu reprendras ici.",
               "Mach dich bereit für den nächsten Kampf! Wenn du jetzt gehst, machst du nächstes Mal hier weiter.",
               "Preparati per il prossimo incontro! Se esci ora, la prossima volta riprendi da qui."));
    }
    if (p & kA) {
      g_aviso.clear();
      g_cursor = 0;
      Lanzar(r, false);
    } else if (p & kB) {   // deja el reto (la partida queda guardada: se sigue la proxima vez)
      g_aviso.clear();
      M::Abort();
    }
  }
  Bocadillo(dl, a, b);
  if (!cargando) {
    S::HelpLine(dl, a.At(600, 868), 38 * a.s,
                {{"A", r.modo == M::kTienda && g_cursor < 3 ? L("Comprar", "Buy", "Acheter", "Kaufen", "Compra")
                                                            : L("Siguiente", "Next", "Suivant", "Weiter", "Avanti")},
                 {"B", L("Guardar y salir", "Save and exit", "Sauver et quitter", "Speichern und raus", "Salva ed esci")}});
  }
}

void DibujarFin(ImDrawList* dl, const S::Area& a, float t, uint16_t p) {
  const M::Estado& st = g_fin_st;
  const Run& r = g_fin_run;
  if (p & (kA | kB)) {
    g_fin = false;
    g_fin_hecho = true;
    g_aviso.clear();
    M::CerrarFin();   // el HUB vuelve
    return;
  }
  S::Banner(dl, a, M::NombreModo(st.tipo));
  std::string titulo;
  if (st.resultado == M::kVictoria) {
    titulo = st.tipo == M::kTorneo4 ? Lang("¡Campeón del torneo!", "Tournament champion!", "Champion du tournoi !",
                                          "Turniersieger!", "Campione del torneo!")
                                    : Lang("¡Reto superado!", "Challenge cleared!", "Défi réussi !",
                                           "Herausforderung geschafft!", "Sfida superata!");
  } else if (st.resultado == M::kGuardado || st.resultado == M::kAbandono) {
    titulo = Lang("Partida guardada", "Run saved", "Partie sauvegardée", "Lauf gespeichert", "Partita salvata");
  } else {
    titulo = Lang("Derrota", "Defeat", "Défaite", "Niederlage", "Sconfitta");
  }
  std::vector<std::string> l = {titulo, M::Nombre(r.jugador) + ": " + Num(st.ganados) + " / " + Num(st.total)};
  if (st.tipo == M::kTienda) {
    l.push_back(Lang("Zenie sin gastar: ", "Unspent Zenie: ", "Zenie non dépensés : ", "Nicht ausgegebene Zenie: ",
                     "Zenie non spesi: ") + Num(r.zeni));
  }
  if (st.resultado == M::kVictoria && PremioDe(st.tipo) > 0) {
    l.push_back(Lang("Premio: ", "Prize: ", "Prix : ", "Preis: ", "Premio: ") + Num(PremioDe(st.tipo)) + " Zenie");
  }
  if (st.tipo == M::kTorneo4) {
    DibujarCuadro(dl, a, t, p, r, true, st.resultado == M::kVictoria);
    Lineas(dl, a, 410, l);
  } else {
    Lineas(dl, a, 170, l);
  }
  std::string b;
  if (st.resultado == M::kVictoria && PremioDe(st.tipo) == 0) {
    b = Lang("¡Increíble! Los Zenie que no gastaste quedan como tu récord.",
             "Amazing! The Zenie you didn't spend stay as your record.",
             "Incroyable ! Les Zenie non dépensés restent comme ton record.",
             "Unglaublich! Die nicht ausgegebenen Zenie bleiben als dein Rekord.",
             "Incredibile! Gli Zenie non spesi restano come tuo record.");
  } else if (st.resultado == M::kVictoria) {
    b = Lang("¡Increíble! El premio va a tu monedero del HUB.", "Amazing! The prize goes to your HUB wallet.",
             "Incroyable ! Le prix va dans ton porte-monnaie du HUB.", "Unglaublich! Der Preis kommt in deine HUB-Geldbörse.",
             "Incredibile! Il premio va nel tuo portafoglio del HUB.");
  } else if (st.resultado == M::kGuardado || st.resultado == M::kAbandono) {
    b = Lang("La próxima vez seguirás desde aquí.", "Next time you will continue from here.",
             "La prochaine fois, tu reprendras d'ici.", "Nächstes Mal machst du hier weiter.",
             "La prossima volta riprenderai da qui.");
  } else {
    b = Lang("Esta partida ha terminado. ¡Vuelve a intentarlo!", "This run is over. Try again!",
             "Cette partie est terminée. Réessaie !", "Dieser Lauf ist vorbei. Versuch es noch einmal!",
             "Questa partita è finita. Riprova!");
  }
  b += "\n" + Lang("Monedero: ", "Wallet: ", "Porte-monnaie : ", "Geldbörse: ", "Portafoglio: ") + Num(g_zeni) + " Zenie";
  Bocadillo(dl, a, b);
  S::HelpLine(dl, a.At(600, 868), 38 * a.s, {{"A", Aceptar()}});
}

}  // namespace

// Premio de un reto (modos.cpp, con su candado puesto): se suma en el siguiente Sincronizar.
void Premio(int zeni) { g_premio += std::max(zeni, 0); }

bool Activa() {
  std::lock_guard<std::mutex> lk(g_mu);
  Sincronizar();
  if (g_vista != kCerrada || g_fin) return true;
  const M::Estado st = M::Get();
  return st.propio && (st.fase == M::kEntre || st.fase == M::kCombate) && g_runs.count(st.tipo);
}

bool Busy() {
  std::lock_guard<std::mutex> lk(g_mu);
  return g_vista != kCerrada || g_fin;
}

void AbrirSelector(int entrada) {
  std::lock_guard<std::mutex> lk(g_mu);
  Cargar();
  g_entrada = entrada;
  g_sel = g_cursor = g_cursor2 = 0;
  g_aviso.clear();
  M::UltimoDuelo(&g_jugador, &g_traje);
  int elo = 0, tramo = 0;
  const char* nombre = nullptr;
  g_rango = M::PerfilJugador(&elo, &tramo, &nombre)
                ? Lang("Rango: ", "Rank: ", "Rang : ", "Rang: ", "Grado: ") + nombre + " (" + Num(elo) + ")"
                : std::string();
  g_vista = kSelector;
  REXLOG_INFO("dbz3 rpg: selector de {} (luchador {})", M::NombreModo(entrada), g_jugador);
}

void Dibujar(ImDrawList* dl, const S::Area& a, float t, uint16_t pulsado) {
  std::lock_guard<std::mutex> lk(g_mu);
  if (g_fin) return DibujarFin(dl, a, t, pulsado);
  switch (g_vista) {
    case kSelector: return DibujarSelector(dl, a, t, pulsado);
    case kSimDia: return DibujarSim(dl, a, t, pulsado);
    case kCuadro: return DibujarTorneo(dl, a, t, pulsado);
    default: break;
  }
  const M::Estado st = M::Get();
  auto it = g_runs.find(st.tipo);
  if (st.propio && it != g_runs.end()) DibujarEntre(dl, a, t, pulsado, st, it->second);
}

// Durante la pelea de un reto propio, bajo el marcador: armadura del jefe y reglas del combate.
void Hud(ImDrawList* dl, const S::Area& a) {
  std::lock_guard<std::mutex> lk(g_mu);
  const M::Estado st = M::Get();
  auto it = g_runs.find(st.tipo);
  if (it == g_runs.end()) return;
  const Run& r = it->second;
  std::string s;
  if (r.modo == M::kJefes && kJefes[std::clamp(r.ganados, 0, 2)].armadura > 0) {
    const bool on = M::ArmaduraActiva();
    S::MenuText(dl, a.At(600, 262), 30 * a.s,
                on ? L("Armadura", "Armor", "Armure", "Panzerung", "Armatura") : L("¡Ahora!", "Now!", "Maintenant !",
                                                                                     "Jetzt!", "Ora!"),
                !on, on);
    return;
  }
  if (r.modo == M::kTorres) {
    const Torre& tr = kTorres[std::clamp(r.extra, 0, kNumTorres - 1)];
    s = L(tr.mod_es, tr.mod_en, tr.mod_fr, tr.mod_de, tr.mod_it);
  } else {
    const M::Reglas g = ReglasDe(r);
    if (g.dano_hecho != 100) s += Lang("Daño ", "Damage ", "Dégâts ", "Schaden ", "Danni ") + Num(g.dano_hecho) + "%";
    if (g.dano_recibido != 100) {
      s += std::string(s.empty() ? "" : "  ") + Lang("Recibido ", "Taken ", "Subis ", "Erlitten ", "Subiti ") +
           Num(g.dano_recibido) + "%";
    }
  }
  if (!s.empty()) S::MenuText(dl, a.At(600, 262), 24 * a.s, s.c_str(), true, false);
}

}  // namespace dbz3::rpg
