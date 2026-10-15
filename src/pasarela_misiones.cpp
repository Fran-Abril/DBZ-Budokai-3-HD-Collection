// dbz3 - Pasarela de modos (2): entrada "Pasarela" del HUB con contenido de Budokai 1 y 2 en el motor de B3.
//   - Saga de Vegeta en Namek (Budokai 1, ficha 04): los combates de la historia de B1 leidos de la ISO del jugador
//     (tabla del ELF, pasarela_b1.cpp), con Zarbon importado de B1 por el Mod Kit (personaje de su mod).
//   - Tablero de la saga Saiyan (estilo Budokai 2, ficha 17): 6 casillas propias en un grafo; se mueve por las
//     casillas vecinas y se lucha desde ellas (vida que se arrastra, semilla del ermitano, jefe final). Perder un
//     combate es el fracaso: el tablero vuelve al principio.
//   - ¡Fusion fallida! (ficha 20): mision original con Gotenks gordo (port de B2 instalado con el Mod Kit).
//   - Las absorciones de Super Bu (ficha 21): Bu absorbe a quien vence y antes de cada combate se elige con que
//     absorcion luchar (trajes de Bu absorbido); al salir, Bu vuelve a ser el de siempre en la seleccion.
//   - Retos de Infinite World (ficha 02 C): misiones de IW rehechas como combates de B3 con sus reglas ("30 ataques
//     basicos en 180 s", "aguanta 60 s sin atacar") y su titulo y texto de la copia del jugador (DATA_USA.AFS).
// Todo con piezas del juego: combate nativo (dbz3::hub::RequestBattleEx), caras del marcador, marco, bocadillo y
// letra de los menus (game_style). Textos propios (es/en). Nada de B1/B2 va en la version: la saga lee la ISO del
// jugador y los personajes importados son mods suyos; si falta algo, la entrada lo dice y no empieza.
// Avance en user_data/dbz3/pasarela/progreso.toml (claves saga_b1, tablero_*, gotenks, buu*). US y EU.
// Sin ganchos propios. Docs: docs/16_pasarela_modos/04_misiones_y_tablero.md.
#include "pasarela.h"

#include <rex/hook.h>
#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/immediate_drawer.h>

#include <imgui.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "generated/dbz3_init.h"
#include "eu_map.h"
#include "ingame/game_style.h"
#include "launcher/i18n.h"
#include "launcher/launcher_state.h"

namespace dbz3::hub {
bool RequestBattleEx(int id1, int id2, int stage, bool p2_com, int time_s, int hp1, int hp2, int form1, int form2);
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

namespace dbz3::pasarela::misiones {
namespace {

namespace T = dbz3::i18n;
namespace S = dbz3::ingame::style;

constexpr int kMainScreen = 5;
constexpr uint16_t kUp = 0x0001, kDown = 0x0002, kLeft = 0x0004, kRight = 0x0008, kStart = 0x0010, kLB = 0x0100,
                   kRB = 0x0200, kA = 0x1000, kB = 0x2000;
constexpr float kAutoSeg = 3.0f;   // prueba (DBZ3_TEST_MISIONES=...,auto): segundos por pantalla
// Mapas de B3 (entrada de data_cmn del escenario; EscenarioPorMapa da el registro). Medidos 2026-10-15
// (DBZ3_TEST_MISIONES): 46 BUDO, 58 MIND & TIME, 56 ISLAND, 68 TOWN, 64 ROCK, 62 PLANE, 52 GOKU, 60 NAMEC, 50 CELL,
// 54 KAIOU, 48 BUU BODY, 66 TOWN B, 44 R&R BASE.
constexpr int kMapaCamara = 58, kMapaPradera = 62, kMapaRocas = 64, kMapaCiudad = 68, kMapaKaio = 54;
// IDs de B3
constexpr int kGoku = 0, kGohanJoven = 3, kGohan = 4, kGoten = 6, kKrilin = 10, kVegeta = 7, kPiccolo = 11, kRaditz = 18, kNappa = 19, kMajinBu = 34,
              kSuperBu = 35, kSaibaman = 42;

int64_t NowMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

// ---- Contenido ----
enum Modo : int { kSagaB1, kTablero, kGotenks, kBuu, kIW, kModos };
const char* const kClaves[kModos] = {"saga_b1", "tablero", "gotenks", "buu", "iw"};

struct Texto {
  const char *es = "", *en = "";
  const char* operator()() const { return T::T(es, en); }
};
enum TipoPaso : int { kCharla, kCombate, kEleccion, kFinal, kInforme };
struct Paso {
  int tipo = kCharla;
  int quien = -1;          // charla/final: quien habla (su cara)
  bool derecha = false;    // charla: cara a la derecha (el rival)
  Texto texto;             // charla/final: lo que dice; combate: aviso de la tarjeta
  std::string texto_propio;   // en lugar de `texto` (charlas armadas con nombres)
  int jugador = 0, forma1 = 0, rival = 0, forma2 = 0, mapa = kMapaPradera, tiempo = 0, vida2 = 0;
  Reglas reglas;
  int desbloquea = -1;     // Bu: absorcion que gana al vencer (bit)
  int informe = -1;        // IW: mision (indice de g_iw) cuyo titulo y texto se ensenan
  int vida_minima = 0;     // IW, reto sin atacar: vida con la que hay que acabar
};

// --- Saga de B1: los combates de la historia de Budokai 1 de la ISO del jugador ---
// Hilo de la saga: los registros seguidos del mismo protagonista desde el primero contra Zarbon (B1 13): en B1 es la
// historia de Vegeta en Namek (Zarbon transformado, Recoome, Freezer y Freezer en su forma final).
constexpr int kZarbonB1 = 13, kDodoriaB1 = 14;
int LuchadorB1(int id_b1) {
  if (id_b1 == kZarbonB1 || id_b1 == kDodoriaB1) return IdPort("b1", std::to_string(id_b1));
  return B1aB3(id_b1);
}
std::vector<CombateB1> HiloSaga(const DatosB1& d) {
  std::vector<CombateB1> out;
  for (size_t i = 0; i < d.historia.size(); ++i) {
    if (d.historia[i].rival_b1 != kZarbonB1) continue;
    out.push_back(d.historia[i]);
    for (size_t j = i + 1; j < d.historia.size(); ++j) {
      const CombateB1& c = d.historia[j];
      if (c.jugador_b1 != out.back().jugador_b1 || c.capitulo != out.back().capitulo + 1) break;
      out.push_back(c);
    }
    break;
  }
  return out;
}
// Lo que dicen antes de cada combate (textos propios; las escenas de B1 no se copian).
struct Frases {
  int rival_b1, forma_b1;
  Texto jugador, rival;
};
const Frases kFrasesSaga[] = {
    {kZarbonB1, -1, {"Zarbon... Esta vez no me vas a pillar por sorpresa. ¡Enséñame esa cara de monstruo!",
                     "Zarbon... This time you won't catch me off guard. Show me that monster face!"},
     {"¿Has vuelto a por más, mono? Te mandaré al fondo del lago otra vez.",
      "Back for more, monkey? I'll send you to the bottom of the lake again."}},
    {12, -1, {"Las Fuerzas Especiales... ¡Apártate, payaso!", "The Special Squadron... Out of my way, clown!"},
     {"¡Recoome no ha perdido nunca! ¡Recoome... ultra fighting bomber!", "Recoome never loses! Recoome... ultra fighting bomber!"}},
    {15, 0, {"¡Freezer! Ahora soy mucho más fuerte. ¡Ha llegado tu hora!", "Frieza! I'm much stronger now. Your time has come!"},
     {"Qué pesado eres, Vegeta. Te recordaré cuál es tu sitio.", "You're so tiresome, Vegeta. Let me remind you of your place."}},
    {15, 1, {"¡Soy el príncipe de los Saiyans! ¡Ni tu forma final me detendrá!",
             "I'm the prince of all Saiyans! Not even your final form will stop me!"},
     {"Esta es mi forma final. Ya no tienes nada que hacer.", "This is my final form. You don't stand a chance."}}};

// --- ¡Fusion fallida! (Gotenks gordo, port de B2: origen b2 / XGXL) ---
int IdGotenksGordo() { return IdPort("b2", "XGXL"); }

// --- Absorciones de Super Bu ---
// Las absorciones de B3 son formas especiales de Super Bu (tipo 7, capsula 123 "Absorcion": modelos 84, 78, 79) que
// solo nacen en combate: pedidas como forma inicial el juego se cae al cargar (medido 2026-10-15). Por eso cada
// absorcion es un traje extra de Super Bu con el modelo del juego del Bu absorbido (traje.toml con "juego:N", mods de
// mod center hd/campanas/trajes_bu/): se elige en la seleccion como cualquier traje y el 1.o es el Bu de siempre.
struct Absorcion {
  const char* carpeta;   // mod del traje ("" = el traje normal)
  Texto nombre;
};
const Absorcion kAbsorciones[] = {{"", {"Súper Bu", "Super Buu"}},
                                  {"bu_absorbido_piccolo", {"Bu (Piccolo absorbido)", "Buu (Piccolo absorbed)"}},
                                  {"bu_absorbido_gotenks", {"Bu (Gotenks absorbido)", "Buu (Gotenks absorbed)"}},
                                  {"bu_absorbido_gohan", {"Bu (Gohan absorbido)", "Buu (Gohan absorbed)"}}};
constexpr int kNAbsorciones = int(std::size(kAbsorciones));
int TrajeBu(int k) { return k <= 0 ? 0 : TrajeExtra(kSuperBu, kAbsorciones[k].carpeta); }
constexpr int kFormaGotenSsj = 1, kFormaGohanDefinitivo = 3, kFormaGokuSsj3 = 4;   // tabla de formas (medida)

// --- Retos de Infinite World (ficha 02 C) ---
// Misiones de IW que son un combate con condicion (textos de DATA_USA de la copia del jugador; numero = entrada - 132):
// 0 "Training No.1": 30 ataques basicos en 180 s; 42 "Endure Gohan's attacks!": 60 s sin atacar con la vida por
// encima del nivel marcado (ponytail: el nivel de IW esta sin descifrar; aqui 1000 de 3000 con el COM en Novato).
const int kMisionesIW[] = {0, 42};
std::mutex g_iw_mu;
std::shared_ptr<const DatosIW> g_iw;
std::atomic<bool> g_iw_leyendo{false};
std::atomic<uint32_t> g_iw_gen{0};
std::shared_ptr<const DatosIW> IW() {
  std::lock_guard<std::mutex> lk(g_iw_mu);
  return g_iw;
}
void LeerIW() {   // en un hilo; una vez (o otra vez si fallo)
  if (IW() || g_iw_leyendo.exchange(true)) return;
  std::thread([] {
    auto d = std::make_shared<DatosIW>();
    std::string error;
    const auto ruta = BuscarIW();
    const bool ok = !ruta.empty() &&
                    LeerMisionesIW(ruta, std::vector<int>(std::begin(kMisionesIW), std::end(kMisionesIW)), *d, error);
    if (ok) {
      std::lock_guard<std::mutex> lk(g_iw_mu);
      g_iw = d;
      ++g_iw_gen;
    }
    int imagenes = 0;
    for (const auto& mi : d->misiones) imagenes += (mi.titulo ? 1 : 0) + (mi.texto ? 1 : 0);
    REXLOG_INFO("dbz3 misiones: Infinite World {}: {}", ruta.empty() ? std::string("no esta") : ruta.filename().string(),
                ok ? std::to_string(d->misiones.size()) + " misiones, " + std::to_string(imagenes) + " imagenes" : error);
    g_iw_leyendo = false;
  }).detach();
}

// --- Tablero (6 casillas originales, saga Saiyan al estilo de B2) ---
enum TipoNodo : int { kInicio, kEncuentro, kSemilla, kJefe };
struct Nodo {
  int tipo;
  float x, y;              // px del area 1200x900
  Texto nombre;
  int rival = -1, mapa = kMapaPradera;
  Texto antes;             // lo que dice el rival
};
const Nodo kNodos[] = {
    {kInicio, 170, 440, {"Kame House", "Kame House"}},
    {kEncuentro, 420, 280, {"Llanura", "Plains"}, kSaibaman, kMapaPradera,
     {"¡Kiii! (Los Saibaman salen de la tierra)", "Kiii! (The Saibamen spring from the ground)"}},
    {kSemilla, 430, 580, {"Torre de Karin", "Karin's Tower"}},
    {kEncuentro, 700, 420, {"Desfiladero", "Gorge"}, kNappa, kMapaRocas,
     {"¡Ja! ¿Tú eres Kakarotto? Esto va a ser divertido.", "Ha! You're Kakarot? This is going to be fun."}},
    {kSemilla, 690, 215, {"Templo de Kami", "Kami's Lookout"}},
    {kJefe, 980, 330, {"Campo de batalla", "Battlefield"}, kVegeta, kMapaRocas,
     {"Kakarotto, eres una vergüenza para los Saiyans. ¡Te enseñaré lo que es un guerrero de élite!",
      "Kakarot, you're a disgrace to the Saiyans. I'll show you what an elite warrior is!"}}};
constexpr int kNNodos = int(std::size(kNodos));
constexpr int kAristas[][2] = {{0, 1}, {0, 2}, {1, 3}, {2, 3}, {1, 4}, {4, 5}, {3, 5}};
constexpr int kEncuentrosParaJefe = (1 << 1) | (1 << 3);   // Saibaman y Nappa antes que Vegeta
std::vector<int> Vecinos(int n) {
  std::vector<int> v;
  for (const auto& e : kAristas) {
    if (e[0] == n) v.push_back(e[1]);
    if (e[1] == n) v.push_back(e[0]);
  }
  std::sort(v.begin(), v.end(), [](int a, int b) { return kNodos[a].x < kNodos[b].x || (kNodos[a].x == kNodos[b].x && kNodos[a].y < kNodos[b].y); });
  return v;
}

// ---- Estado (g_mu) ----
enum Fase : int { kNada = 0, kMenu, kPantalla, kLuchando, kPerdido, kTableroF };
const char* const kFaseNombre[] = {"nada", "menu", "pantalla", "combate", "perdido", "tablero"};

std::mutex g_mu;
int g_fase = kNada, g_modo = 0, g_cursor = 0, g_intentos = 0;
std::vector<Paso> g_pasos;
int g_paso = 0;
uint64_t g_cambio = 0;
std::atomic<int> g_fase_pub{kNada};
std::atomic<bool> g_auto{false};
// marcador del reto en el combate (IW): 0 nada, 1 golpes (g_hud_golpes de g_hud_meta), 2 aguantar sin atacar
std::atomic<int> g_hud_tipo{0}, g_hud_meta{0}, g_hud_golpes{0};
std::atomic<bool> g_panel{false};
// tablero
int g_nodo = 0, g_turno = 0, g_hecho = 0, g_vida = 0, g_destino = 0;
std::string g_mensaje;   // aviso del tablero (semilla, jefe cerrado...)
// Bu
int g_absorbidos = 1;    // bit por absorcion (la 0, Super Bu, siempre)
int g_forma_bu = 0;

void Publicar(int fase) {   // con g_mu
  if (fase != g_fase || fase == kPantalla) {
    std::string que;
    if (fase == kPantalla && g_paso < int(g_pasos.size())) {
      const Paso& p = g_pasos[size_t(g_paso)];
      que = p.tipo == kCharla ? " (charla)" : p.tipo == kCombate ? " (combate)" : p.tipo == kEleccion ? " (eleccion)"
            : p.tipo == kInforme ? " (informe)" : " (final)";
    }
    REXLOG_INFO("dbz3 misiones: {} paso {}/{}{}: {}", g_fase == kNada && fase == kMenu ? "menu" : kClaves[g_modo],
                g_paso + 1, g_pasos.size(), que, kFaseNombre[fase]);
  }
  g_fase = fase;
  g_fase_pub = fase;
  ++g_cambio;
}

// Disponible y por que no (para el menu).
bool Listo(int m, std::string* motivo) {
  auto no = [&](const char* es, const char* en) {
    if (motivo) *motivo = T::T(es, en);
    return false;
  };
  if (m == kSagaB1) {
    const auto d = DatosLeidos();
    if (!d) return no("Necesita tu copia de Dragon Ball Z Budokai (PS2): pon la ISO en la carpeta ps2_games.",
                      "Needs your copy of Dragon Ball Z Budokai (PS2): put the ISO in the ps2_games folder.");
    if (HiloSaga(*d).empty()) return no("Tu ISO de Budokai 1 no trae la historia de Namek.", "Your Budokai 1 ISO has no Namek story.");
    if (IdPort("b1", std::to_string(kZarbonB1)) < 0)
      return no("Importa a Zarbon de Budokai 1 con el Mod Kit (Importar personaje > Budokai 1).",
                "Import Zarbon from Budokai 1 with the Mod Kit (Import character > Budokai 1).");
  }
  if (m == kBuu) {
    for (int k = 1; k < kNAbsorciones; ++k) {
      if (TrajeBu(k) < 0)
        return no("Instala los trajes de Bu absorbido (Mod Kit: mods de mod center hd/campanas/trajes_bu).",
                  "Install the absorbed Buu costumes (Mod Kit: mods in mod center hd/campanas/trajes_bu).");
    }
  }
  if (m == kIW && !IW()) {
    LeerIW();
    return no("Necesita tu copia de Dragon Ball Z Infinite World (PS2) en la carpeta ps2_games (ISO o carpeta).",
              "Needs your copy of Dragon Ball Z Infinite World (PS2) in the ps2_games folder (ISO or folder).");
  }
  if (m == kGotenks && IdGotenksGordo() < 0)
    return no("Importa a Gotenks gordo de Budokai 2 con el Mod Kit (Importar personaje > Budokai 2).",
              "Import Fat Gotenks from Budokai 2 with the Mod Kit (Import character > Budokai 2).");
  if (motivo) motivo->clear();
  return true;
}

const Texto kTitulos[kModos] = {{"Saga de Vegeta en Namek (Budokai 1)", "Vegeta's Namek saga (Budokai 1)"},
                                {"Tablero de la saga Saiyan (Budokai 2)", "Saiyan saga board (Budokai 2)"},
                                {"¡Fusión fallida! (Gotenks gordo)", "Failed fusion! (Fat Gotenks)"},
                                {"Las absorciones de Súper Bu", "Super Buu's absorptions"},
                                {"Retos de Infinite World", "Infinite World challenges"}};
const Texto kAyudas[kModos] = {
    {"Los combates de la historia de Budokai 1 de tu copia del juego, con Zarbon importado de Budokai 1.",
     "The story battles from your copy of Budokai 1, with Zarbon imported from Budokai 1."},
    {"Muévete por las casillas como en Budokai 2: lucha en ellas, recupérate con las semillas y vence a Vegeta.",
     "Move across the spaces like in Budokai 2: fight on them, heal with senzu beans and beat Vegeta."},
    {"Una misión nueva: la fusión sale mal, pero la Tierra no puede esperar.",
     "A new mission: the fusion goes wrong, but the Earth can't wait."},
    {"Bu absorbe a quien vence. Antes de cada combate, elige con qué absorción luchar.",
     "Buu absorbs whoever he beats. Before each fight, choose which absorption to fight with."},
    {"Misiones de Infinite World con sus reglas: golpes contra el reloj y aguantar sin atacar.",
     "Infinite World missions with their rules: hits against the clock and enduring without attacking."}};

// Pasos lineales de cada modo (el tablero arma los suyos al llegar a una casilla).
std::vector<Paso> PasosDe(int m) {
  std::vector<Paso> p;
  auto charla = [&](int quien, bool der, Texto t) {
    Paso x;
    x.tipo = kCharla;
    x.quien = quien;
    x.derecha = der;
    x.texto = t;
    p.push_back(x);
  };
  auto combate = [&](int j, int f1, int r, int f2, int mapa, Texto aviso) {
    Paso x;
    x.tipo = kCombate;
    x.jugador = j;
    x.forma1 = f1;
    x.rival = r;
    x.forma2 = f2;
    x.mapa = mapa;
    x.texto = aviso;
    p.push_back(x);
    return &p.back();
  };
  auto final = [&](int quien, Texto t) {
    Paso x;
    x.tipo = kFinal;
    x.quien = quien;
    x.texto = t;
    p.push_back(x);
  };
  if (m == kSagaB1) {
    const auto d = DatosLeidos();
    if (!d) return p;
    for (const CombateB1& c : HiloSaga(*d)) {
      const int j = LuchadorB1(c.jugador_b1), r = LuchadorB1(c.rival_b1);
      for (const auto& f : kFrasesSaga) {
        if (f.rival_b1 != c.rival_b1 || (f.forma_b1 >= 0 && f.forma_b1 != c.forma)) continue;
        charla(j, false, f.jugador);
        charla(r, true, f.rival);
      }
      Paso* x = combate(j, FormaB3(j, c.forma_jugador), r, FormaB3(r, c.forma), 0, {});
      x->mapa = -1;   // el escenario de B1 (EscenarioB3 con su nombre)
      x->texto_propio = c.escenario_nombre;
      x->tiempo = c.tiempo > 0 ? (c.tiempo + 30) / 60 : c.tiempo == -1 ? -1 : 0;
      x->vida2 = c.vida_rival > 0 && c.vida_rival != 3000 ? c.vida_rival : 0;
      x->reglas.ki_rival = float(c.ki_rival) / 100.0f;
    }
    final(kVegeta, {"¡Kakarotto! Esto no ha terminado. El único que derrotará a Freezer seré yo...",
                    "Kakarot! This isn't over. I'm the only one who will defeat Frieza..."});
  } else if (m == kGotenks) {
    const int fat = IdGotenksGordo();
    charla(kPiccolo, true, {"¡Os habéis equivocado de postura! ¡Los dedos no se tocaban!",
                            "You got the pose wrong! Your fingers weren't touching!"});
    charla(fat, false, {"¡Uf, uf...! ¡Da igual! ¡Con este cuerpo también vamos a ganar a Bu!",
                        "Huff, huff...! Who cares! We'll beat Buu even with this body!"});
    combate(fat, 0, kMajinBu, 0, kMapaCiudad, {"¡Gotenks gordo contra Majin Bu en la ciudad!",
                                               "Fat Gotenks versus Majin Buu in the city!"});
    charla(kPiccolo, true, {"No me lo puedo creer... ¡Está aguantando! Pero Bu ha cambiado...",
                            "I can't believe it... He's holding on! But Buu has changed..."});
    charla(kSuperBu, true, {"Ji, ji. Bu quiere jugar con el gordito en la sala del tiempo.",
                            "Hee hee. Buu wants to play with the chubby one in the time chamber."});
    combate(fat, 0, kSuperBu, 0, kMapaCamara, {"¡Aguanta en la sala del espíritu y del tiempo!",
                                               "Hold on in the Hyperbolic Time Chamber!"});
    final(fat, {"¡Ja! ¡La fusión perfecta es para los flojos! ...Ahora la repetimos bien, ¿vale, Piccolo?",
                "Ha! Perfect fusions are for wimps! ...We'll do it properly now, okay, Piccolo?"});
  } else if (m == kIW) {
    Paso i0;
    i0.tipo = kInforme;
    i0.informe = 0;
    i0.texto = {"Entrenamiento n.º 1: ataques básicos. Da 30 golpes al rival con P y K antes de que pasen 180 segundos.",
                "Training No.1: basic attacks. Land 30 hits on your opponent with P and K before 180 seconds are up."};
    p.push_back(i0);
    Paso* x = combate(kGoku, 0, kKrilin, 0, kMapaPradera, {"Reto: 30 golpes en 180 s.", "Challenge: 30 hits in 180 s."});
    x->tiempo = 180;
    x->reglas.golpes = 30;
    x->reglas.nivel_com = 0;
    Paso i1;
    i1.tipo = kInforme;
    i1.informe = 1;
    i1.texto = {"¡Aguanta los ataques de Gohan! No ataques y acaba los 60 segundos con 1000 de vida o más.",
                "Endure Gohan's attacks! Don't attack and finish the 60 seconds with 1000 health or more."};
    p.push_back(i1);
    x = combate(kGoku, 0, kGohanJoven, 0, kMapaCamara, {"Reto: 60 s sin atacar, vida 1000 o más.",
                                                       "Challenge: 60 s without attacking, health 1000 or more."});
    x->tiempo = 60;
    x->reglas.sin_atacar = true;
    x->reglas.nivel_com = 0;
    x->vida_minima = 1000;
    final(kGoku, {"¡Retos superados! Así se entrena en Infinite World.",
                  "Challenges cleared! That's how you train in Infinite World."});
  } else if (m == kBuu) {
    Paso e;
    e.tipo = kEleccion;
    charla(kSuperBu, false, {"Bu tiene hambre... ¡Bu se va a comer a los más fuertes de la Tierra!",
                             "Buu is hungry... Buu is going to eat the strongest on Earth!"});
    charla(kPiccolo, true, {"No te dejaré pasar, monstruo. ¡Aquí se acaba todo!", "I won't let you through, monster. This ends here!"});
    combate(kSuperBu, 0, kPiccolo, 0, kMapaCamara, {"Vence a Piccolo y Bu lo absorberá.", "Beat Piccolo and Buu will absorb him."})
        ->desbloquea = 1;
    p.push_back(e);
    charla(kGoten, true, {"¡Trunks, la fusión! ...¡Demasiado tarde, ya está aquí!", "Trunks, the fusion! ...Too late, he's here!"});
    combate(kSuperBu, 0, kGoten, kFormaGotenSsj, kMapaPradera, {"Vence a Goten y Bu absorberá a Gotenks.",
                                                                "Beat Goten and Buu will absorb Gotenks."})
        ->desbloquea = 2;
    p.push_back(e);
    charla(kGohan, true, {"¡Bu! ¡Devuélveme a mis amigos ahora mismo!", "Buu! Give my friends back right now!"});
    combate(kSuperBu, 0, kGohan, kFormaGohanDefinitivo, kMapaRocas, {"Vence a Gohan y Bu lo absorberá.",
                                                                     "Beat Gohan and Buu will absorb him."})
        ->desbloquea = 3;
    p.push_back(e);
    charla(kGoku, true, {"Así que has absorbido a todos... Pues yo no me voy a dejar.",
                         "So you've absorbed everyone... Well, I won't let you have me."});
    combate(kSuperBu, 0, kGoku, kFormaGokuSsj3, kMapaKaio, {"El último: Goku en Súper Saiyan 3.", "The last one: Super Saiyan 3 Goku."});
    final(kSuperBu, {"¡Ja, ja! ¡Bu es el más fuerte! ...Bu tiene hambre otra vez.",
                     "Ha ha! Buu is the strongest! ...Buu is hungry again."});
  }
  return p;
}

// ---- Tablero ----
void GuardarTablero() {
  GuardarProgreso("tablero_nodo", g_nodo);
  GuardarProgreso("tablero_turno", g_turno);
  GuardarProgreso("tablero_hecho", g_hecho);
  GuardarProgreso("tablero_vida", g_vida);
}
void ReiniciarTablero() {   // con g_mu
  g_nodo = 0;
  g_turno = 0;
  g_hecho = 0;
  g_vida = 0;
  GuardarTablero();
}
void VolverAlTablero() {   // con g_mu
  g_pasos.clear();
  g_paso = 0;
  const auto v = Vecinos(g_nodo);
  g_destino = v.empty() ? g_nodo : v.front();
  Publicar(kTableroF);
}
// Llega a la casilla n: su suceso (combate, semilla o jefe).
void Llegar(int n) {   // con g_mu
  g_nodo = n;
  ++g_turno;
  g_mensaje.clear();
  const Nodo& d = kNodos[n];
  REXLOG_INFO("dbz3 misiones: tablero: turno {}, casilla {} ({}), hechas {:#x}", g_turno, n, d.nombre.es, g_hecho);
  GuardarTablero();
  if (g_hecho & (1 << n)) return VolverAlTablero();
  if (d.tipo == kSemilla) {
    g_hecho |= 1 << n;
    g_vida = 0;
    g_mensaje = T::T("¡Una semilla del ermitaño! Recuperas toda la vida.", "A senzu bean! Your health is fully restored.");
    GuardarTablero();
    return VolverAlTablero();
  }
  if (d.tipo == kJefe && (g_hecho & kEncuentrosParaJefe) != kEncuentrosParaJefe) {
    g_mensaje = T::T("Vegeta no se dignará a luchar hasta que venzas a los Saibaman y a Nappa.",
                     "Vegeta won't bother to fight until you beat the Saibamen and Nappa.");
    return VolverAlTablero();
  }
  if (d.tipo != kEncuentro && d.tipo != kJefe) return VolverAlTablero();
  g_pasos.clear();
  Paso c;
  c.tipo = kCharla;
  c.quien = d.rival;
  c.derecha = true;
  c.texto = d.antes;
  g_pasos.push_back(c);
  Paso x;
  x.tipo = kCombate;
  x.jugador = kGoku;
  x.rival = d.rival;
  x.mapa = d.mapa;
  x.texto = d.nombre;
  g_pasos.push_back(x);
  if (d.tipo == kJefe) {
    Paso f;
    f.tipo = kFinal;
    f.quien = kGoku;
    f.texto = {"¡Lo conseguimos! Los Saiyans se marchan... por ahora. Tablero completado.",
               "We did it! The Saiyans are leaving... for now. Board complete."};
    g_pasos.push_back(f);
  }
  g_paso = 0;
  g_intentos = 0;
  Publicar(kPantalla);
}

// ---- Avance ----
void Salir() {   // con g_mu: al menu de la pasarela (se guarda donde iba)
  if (g_modo != kTablero && g_fase != kMenu) GuardarProgreso(kClaves[g_modo], g_paso);
  g_pasos.clear();
  g_paso = 0;
  Publicar(kMenu);
}
void Terminar() {   // con g_mu: modo completado
  REXLOG_INFO("dbz3 misiones: {} completado", kClaves[g_modo]);
  if (g_modo == kTablero) ReiniciarTablero();
  else GuardarProgreso(kClaves[g_modo], 0);
  if (g_modo == kBuu) {
    g_absorbidos = 1;
    GuardarProgreso("buu_absorbidos", 1);
  }
  g_pasos.clear();
  g_paso = 0;
  Publicar(kMenu);
}
void Siguiente() {   // con g_mu
  ++g_paso;
  g_intentos = 0;
  if (g_paso >= int(g_pasos.size())) {
    if (g_modo == kTablero) return VolverAlTablero();
    return Terminar();
  }
  if (g_modo != kTablero) GuardarProgreso(kClaves[g_modo], g_paso);
  Publicar(kPantalla);
}

void Entrar(int m) {   // con g_mu: desde el menu
  g_modo = m;
  g_intentos = 0;
  g_mensaje.clear();
  if (m == kTablero) {
    g_nodo = std::clamp(LeerProgreso("tablero_nodo", 0), 0, kNNodos - 1);
    g_turno = LeerProgreso("tablero_turno", 0);
    g_hecho = LeerProgreso("tablero_hecho", 0);
    g_vida = LeerProgreso("tablero_vida", 0);
    REXLOG_INFO("dbz3 misiones: tablero: casilla {}, turno {}, hechas {:#x}, vida {}", g_nodo, g_turno, g_hecho, g_vida);
    return VolverAlTablero();
  }
  g_pasos = PasosDe(m);
  g_paso = std::clamp(LeerProgreso(kClaves[m], 0), 0, std::max(0, int(g_pasos.size()) - 1));
  // se sigue desde la charla de antes del combate pendiente
  while (g_paso > 0 && g_pasos[size_t(g_paso)].tipo != kCharla && g_pasos[size_t(g_paso - 1)].tipo == kCharla) --g_paso;
  if (m == kBuu) {
    g_absorbidos = LeerProgreso("buu_absorbidos", 1) | 1;
    g_forma_bu = 0;
  }
  REXLOG_INFO("dbz3 misiones: {} ({} pasos) desde el paso {}", kClaves[m], g_pasos.size(), g_paso + 1);
  Publicar(kPantalla);
}

bool Lanzar() {   // con g_mu
  const Paso& p = g_pasos[size_t(g_paso)];
  int escenario = 0;
  if (p.mapa < 0) {
    escenario = EscenarioB3(p.texto_propio);
  } else {
    escenario = EscenarioPorMapa(p.mapa);
  }
  const int forma1 = p.forma1;
  Reglas reglas = p.reglas;
  g_hud_golpes = 0;
  g_hud_tipo = p.reglas.golpes > 0 ? 1 : p.reglas.sin_atacar ? 2 : 0;
  g_hud_meta = p.reglas.golpes > 0 ? p.reglas.golpes : p.vida_minima;
  reglas.contador = &g_hud_golpes;
  if (g_modo == kBuu && p.jugador == kSuperBu) reglas.traje_jugador = std::max(0, TrajeBu(g_forma_bu));
  const int vida1 = g_modo == kTablero && g_vida > 0 ? -g_vida : 0;   // la vida que le quedo (tablero)
  if (!dbz3::hub::RequestBattleEx(p.jugador, p.rival, escenario, true, p.tiempo, vida1, p.vida2, forma1, p.forma2)) return false;
  REXLOG_INFO("dbz3 misiones: {} combate: id {} (forma {}, traje {}) contra id {} (forma {}), escenario {}, tiempo {}, "
              "vida {} / {}, ki del rival {:.1f}", kClaves[g_modo], p.jugador, forma1, reglas.traje_jugador, p.rival,
              p.forma2, escenario, p.tiempo, vida1, p.vida2, reglas.ki_rival);
  Publicar(kLuchando);
  const uint64_t cambio = g_cambio;
  // en el registro, cada combate con su nombre (el tablero repite pasos: tambien el turno)
  const std::string quien = "misiones: " + std::string(kClaves[g_modo]) +
                            (g_modo == kTablero ? " turno " + std::to_string(g_turno) : std::string()) + " paso " +
                            std::to_string(g_paso + 1);
  VigilarCombate(quien.c_str(), reglas,
                 [cambio](const Fin& f) {
    std::lock_guard<std::mutex> lk(g_mu);
    if (g_fase != kLuchando || g_cambio != cambio) return;
    REXLOG_INFO("dbz3 misiones: fin del combate ({} paso {}): gana el lado {}, vida {}", kClaves[g_modo], g_paso + 1,
                f.ganador + 1, f.vida[0]);
    const Paso& reto = g_pasos[size_t(g_paso)];
    bool gana = f.ganador == 0;
    if (reto.reglas.golpes > 0) gana = f.golpes >= reto.reglas.golpes;
    if (reto.reglas.sin_atacar) gana = !f.ataco && f.segundos == 0 && f.vida[0] >= reto.vida_minima;
    if (reto.reglas.golpes > 0 || reto.reglas.sin_atacar) {
      REXLOG_INFO("dbz3 misiones: reto {}: {} golpes, ataco {}, {} s de sobra, vida {} (minimo {})",
                  gana ? "cumplido" : "fallado", f.golpes, f.ataco, f.segundos, f.vida[0], reto.vida_minima);
    }
    if (!gana) {
      ++g_intentos;
      if (g_modo == kTablero) {   // fracaso: el tablero vuelve a empezar
        REXLOG_INFO("dbz3 misiones: tablero: fracaso en el turno {} (se vuelve al principio)", g_turno);
        ReiniciarTablero();
      }
      return Publicar(kPerdido);
    }
    const Paso& p = g_pasos[size_t(g_paso)];
    if (g_modo == kTablero) {
      g_hecho |= 1 << g_nodo;
      g_vida = std::max(1, f.vida[0]);
      GuardarTablero();
    }
    if (g_modo == kBuu && p.desbloquea > 0) {
      g_absorbidos |= 1 << p.desbloquea;
      GuardarProgreso("buu_absorbidos", g_absorbidos);
      REXLOG_INFO("dbz3 misiones: Bu absorbe: {} (absorciones {:#x})", kAbsorciones[p.desbloquea].nombre.es, g_absorbidos);
    }
    Siguiente();
  });
  return true;
}

void CreatePanel();

}  // namespace

// ---- API (HUB y pruebas) ----
bool Start() {
  DatosLeidos();   // la saga lee la ISO de B1 (si no estaba leida)
  LeerIW();
  if (!g_panel.exchange(true)) dbz3::launcher::DeferUi(&CreatePanel);
  std::lock_guard<std::mutex> lk(g_mu);
  if (g_fase != kNada) return false;
  g_cursor = 0;
  Publicar(kMenu);
  return true;
}
bool Busy() { return g_fase_pub.load() != kNada; }

namespace {

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
    if (fase == kLuchando && g_hud_tipo.load() && dbz3::ingame::GameScreen() == 25 &&
        (EstadoCombate() == 3 || EstadoCombate() == 4)) {
      Marcador(io);   // solo con la pelea en marcha (no sobre el menu del final)
    }
    const bool menu = dbz3::ingame::GameScreen() == kMainScreen;
    const bool show = fase != kNada && fase != kLuchando && menu && !dbz3::GameDimmed() && !dbz3::OverlayOpen();
    if (auto* in = Input(); in && show != blocked_) {
      if (show) in->AddUIInputBlocker(); else in->RemoveUIInputBlocker();
      blocked_ = show;
    }
    const float t = float(ImGui::GetTime()), dt = std::min(io.DeltaTime, 0.1f);
    alpha_ = std::clamp(alpha_ + (show ? dt * 4.0f : -dt * 8.0f), 0.0f, 1.0f);
    if (!show) {
      wait_release_ = true;
      if (alpha_ <= 0.0f) return;
    }
    uint16_t pressed = Pad();
    if (wait_release_) {
      if (g_pad.load() == 0 && alpha_ >= 1.0f) wait_release_ = false;
      pressed = 0;
    }
    if (!show || g_auto.load()) pressed = 0;   // prueba automatica: solo avanza el modo (teclas de mas no cuentan)
    std::unique_lock<std::mutex> lk(g_mu);
    if (g_cambio != visto_) {
      visto_ = g_cambio;
      cambio_at_ = t;
    }
    const auto a = S::GameArea(io);
    auto* dl = ImGui::GetForegroundDrawList();
    const int v0 = dl->VtxBuffer.Size;
    S::Backdrop(dl, a, "fondo_duelo");
    S::Sides(dl, a);
    const bool auto_on = g_auto.load() && t - cambio_at_ > kAutoSeg;
    if (g_fase == kMenu) {
      DrawMenu(dl, a, t, pressed, auto_on);
    } else if (g_fase == kTableroF) {
      DrawTablero(dl, a, t, pressed, auto_on);
    } else if (g_fase == kPerdido) {
      DrawCombate(dl, a, t, pressed, auto_on, true);
    } else if (g_paso < int(g_pasos.size())) {
      const Paso& p = g_pasos[size_t(g_paso)];
      if (p.tipo == kCharla) DrawCharla(dl, a, t, pressed, auto_on);
      else if (p.tipo == kCombate) DrawCombate(dl, a, t, pressed, auto_on, false);
      else if (p.tipo == kEleccion) DrawEleccion(dl, a, t, pressed, auto_on);
      else if (p.tipo == kInforme) DrawInforme(dl, a, t, pressed, auto_on);
      else DrawFinal(dl, a, t, pressed, auto_on);
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
  uint16_t Pad() {
    if (!g_pad_pending.exchange(true)) {
      dbz3::launcher::DeferUi([]() {
        uint16_t b = 0;
        if (auto* in = Input()) {
          rex::input::X_INPUT_STATE st = {};
          if (in->GetStateForUI(0, &st) == 0) {
            b = static_cast<uint16_t>(st.gamepad.buttons);
            const int lx = int16_t(st.gamepad.thumb_lx), ly = int16_t(st.gamepad.thumb_ly);
            if (ly > 16000) b |= kUp;
            if (ly < -16000) b |= kDown;
            if (lx < -16000) b |= kLeft;
            if (lx > 16000) b |= kRight;
          }
        }
        g_pad = b;
        g_pad_pending = false;
      });
    }
    const uint16_t b = g_pad.load(), pressed = uint16_t(b & ~last_);
    last_ = b;
    return pressed;
  }

  // Durante el combate de un reto de IW, bajo el reloj: "Golpes 12/30" o "Sin atacar - vida 1500 o más".
  static void Marcador(ImGuiIO& io) {
    const S::Area a = S::GameArea(io);
    char txt[96];
    if (g_hud_tipo.load() == 1) {
      std::snprintf(txt, sizeof(txt), T::T("Golpes %d/%d", "Hits %d/%d"), g_hud_golpes.load(), g_hud_meta.load());
    } else {
      std::snprintf(txt, sizeof(txt), T::T("Sin atacar - vida %d o más", "No attacking - health %d or more"),
                    g_hud_meta.load());
    }
    S::MenuText(ImGui::GetForegroundDrawList(), a.At(600, 172), 34 * a.s, txt, true, false);
  }

  void Ayuda(ImDrawList* dl, const S::Area& a, const std::vector<std::pair<const char*, const char*>>& items) {
    S::HelpLine(dl, a.At(600, 878), 34 * a.s, items);
  }
  void Bocadillo(ImDrawList* dl, const S::Area& a, float t, const char* texto, float cola) {
    const float in = Ease((t - cambio_at_) / 0.3f);
    S::Speech(dl, a.At(60, 640 + 30 * (1 - in)), a.At(1140, 850 + 30 * (1 - in)), a.s, texto, cola);
  }

  // Menu de la pasarela: los cuatro modos en el marco del juego; los que no estan listos, en gris con el motivo.
  void DrawMenu(ImDrawList* dl, const S::Area& a, float t, uint16_t pressed, bool auto_on) {
    S::Banner(dl, a, T::T("Pasarela", "Gateway"));
    const float y0 = 140, paso = 84;
    S::Frame(dl, a.At(150, y0), a.At(1050, y0 + 30 + paso * float(kModos)), a.s);
    std::string motivo[kModos];
    bool listo[kModos];
    for (int m = 0; m < kModos; ++m) listo[m] = Listo(m, &motivo[m]);
    if (pressed & kUp) g_cursor = (g_cursor + kModos - 1) % kModos;
    if (pressed & kDown) g_cursor = (g_cursor + 1) % kModos;
    for (int m = 0; m < kModos; ++m) {
      const float y = y0 + 15 + paso * (float(m) + 0.5f);
      if (m == g_cursor) S::Highlight(dl, a.At(165, y - paso * 0.5f + 6), a.At(1035, y + paso * 0.5f - 6), t);
      S::MenuText(dl, a.At(600, y), 40 * a.s, kTitulos[m](), m == g_cursor, !listo[m]);
    }
    S::Speech(dl, a.At(60, 640), a.At(1140, 840), a.s,
              listo[g_cursor] ? kAyudas[g_cursor]() : motivo[g_cursor].c_str(), 0.12f);
    Ayuda(dl, a, {{"A", T::T("Elegir", "Select")}, {"B", T::T("Volver", "Back")}});
    if (pressed & kB) {
      REXLOG_INFO("dbz3 misiones: fuera de la pasarela");
      return Publicar(kNada);
    }
    if ((pressed & (kA | kStart)) && listo[g_cursor]) Entrar(g_cursor);
    (void)auto_on;
  }

  void Cara(ImDrawList* dl, const S::Area& a, int id, bool derecha, bool on, float t) {
    if (id < 0) return;
    DibujarCara(dl, a, id, derecha ? 800.0f : 120.0f, 190.0f, 280.0f, on, NombreDe(id).c_str(), t, derecha);
  }

  void DrawCharla(ImDrawList* dl, const S::Area& a, float t, uint16_t pressed, bool auto_on) {
    const Paso& p = g_pasos[size_t(g_paso)];
    S::Banner(dl, a, kTitulos[g_modo]());
    Cara(dl, a, p.quien, p.derecha, true, t);
    Bocadillo(dl, a, t, p.texto_propio.empty() ? p.texto() : p.texto_propio.c_str(), p.derecha ? 0.78f : 0.22f);
    Ayuda(dl, a, {{"A", T::T("Siguiente", "Next")}, {"B", T::T("Salir", "Quit")}});
    if (auto_on) pressed |= kA;
    if (pressed & kB) return Salir();
    if (pressed & (kA | kStart)) Siguiente();
  }

  // Tarjeta del combate (y la de reintentar tras perder): las dos caras, "VS" y el aviso.
  void DrawCombate(ImDrawList* dl, const S::Area& a, float t, uint16_t pressed, bool auto_on, bool perdido) {
    const Paso& p = g_pasos[size_t(g_paso)];
    S::Banner(dl, a, kTitulos[g_modo]());
    int j = p.jugador;
    Cara(dl, a, j, false, !perdido, t);
    Cara(dl, a, p.rival, true, true, t);
    const float in = Ease((t - cambio_at_) / 0.35f);
    S::MenuText(dl, a.At(600, 330), (110 + 40 * (1 - in)) * a.s, "VS", true, false);
    std::string aviso = p.mapa < 0 ? std::string() : std::string(p.texto());
    if (g_modo == kBuu && p.jugador == kSuperBu) aviso = std::string(kAbsorciones[g_forma_bu].nombre()) + ". " + aviso;
    if (g_modo == kTablero && g_vida > 0) {
      char v[64];
      std::snprintf(v, sizeof(v), T::T(" Vida: %d.", " Health: %d."), g_vida);
      aviso += v;
    }
    if (perdido) {
      const bool tablero = g_modo == kTablero;
      dl->AddRectFilled(a.At(150, 540), a.At(1050, 625), IM_COL32(8, 30, 48, 215), 10 * a.s);
      S::MenuText(dl, a.At(600, 582), 38 * a.s, tablero ? T::T("¡Fracaso!", "Failure!") : T::T("¡Derrota!", "Defeat!"),
                  true, false);
      S::Speech(dl, a.At(60, 660), a.At(1140, 840), a.s,
                tablero ? T::T("Has caído en el tablero: vuelves a Kame House y las casillas se reinician.",
                               "You fell on the board: back to Kame House, and the spaces reset.")
                        : T::T("¿Lo intentas otra vez?", "Want to try again?"),
                -1);
      Ayuda(dl, a, {{"A", tablero ? T::T("Al tablero", "To the board") : T::T("Reintentar", "Retry")},
                    {"B", T::T("Salir", "Quit")}});
      if (auto_on) pressed |= kA;
      if (pressed & kB) return Salir();
      if (pressed & kA) {
        if (tablero) return VolverAlTablero();
        Publicar(kPantalla);
      }
      return;
    }
    if (!aviso.empty()) {
      dl->AddRectFilled(a.At(150, 540), a.At(1050, 625), IM_COL32(8, 30, 48, 215), 10 * a.s);
      S::MenuText(dl, a.At(600, 582), 32 * a.s, aviso.c_str(), true, false);
    }
    std::string texto = NombreDe(j) + T::T(" contra ", " versus ") + NombreDe(p.rival) + ".";
    if (g_intentos > 0) {
      char k[48];
      std::snprintf(k, sizeof(k), T::T(" Intento %d.", " Try %d."), g_intentos + 1);
      texto += k;
    }
    S::Speech(dl, a.At(60, 660), a.At(1140, 840), a.s, texto.c_str(), -1);
    Ayuda(dl, a, {{"A", T::T("¡A luchar!", "Fight!")}, {"B", T::T("Salir", "Quit")}});
    if (auto_on) pressed |= kA;
    if (pressed & kB) return Salir();
    if ((pressed & kA) && !Lanzar()) REXLOG_WARN("dbz3 misiones: el conector no acepto el combate (otro pendiente)");
  }

  // Imagenes de las misiones de IW (titulo y texto de la copia del jugador) como texturas.
  void TexturasIW() {
    const auto d = IW();
    if (!d || tex_iw_gen_ == g_iw_gen.load()) return;
    auto* drawer = dbz3::launcher::PreviewDrawer();
    if (!drawer) return;
    tex_iw_gen_ = g_iw_gen.load();
    tex_iw_.clear();
    for (size_t i = 0; i < d->misiones.size(); ++i) {
      for (int k = 0; k < 2; ++k) {
        const Imagen& img = k == 0 ? d->misiones[i].titulo : d->misiones[i].texto;
        if (!img) continue;
        Tex t;
        t.tex = drawer->CreateTexture(uint32_t(img.w), uint32_t(img.h), rex::ui::ImmediateTextureFilter::kLinear, false,
                                      img.rgba.data());
        t.w = float(img.w);
        t.h = float(img.h);
        if (t.tex) tex_iw_[int(i) * 2 + k] = std::move(t);
      }
    }
  }

  // Mision de IW: su titulo y su texto (imagenes del juego de origen, en ingles) en una franja oscura, y en el
  // bocadillo las reglas en el idioma del juego.
  void DrawInforme(ImDrawList* dl, const S::Area& a, float t, uint16_t pressed, bool auto_on) {
    const Paso& p = g_pasos[size_t(g_paso)];
    S::Banner(dl, a, kTitulos[g_modo]());
    TexturasIW();
    const float in = Ease((t - cambio_at_) / 0.35f);
    S::Frame(dl, a.At(230, 150 + 20 * (1 - in)), a.At(970, 600 + 20 * (1 - in)), a.s);
    dl->AddRectFilled(a.At(250, 170), a.At(950, 580), IM_COL32(16, 22, 48, 235), 8 * a.s);
    auto img = [&](int k, float cx, float cy, float alto) {
      auto it = tex_iw_.find(p.informe * 2 + k);
      if (it == tex_iw_.end()) return false;
      const Tex& x = it->second;
      const float w = x.w * alto / x.h;
      dl->AddImage(reinterpret_cast<ImTextureID>(x.tex.get()), a.At(cx - w * 0.5f, cy - alto * 0.5f),
                   a.At(cx + w * 0.5f, cy + alto * 0.5f));
      return true;
    };
    if (!img(0, 600, 225, 66)) S::MenuText(dl, a.At(600, 225), 44 * a.s, T::T("Misión", "Mission"), true, false);
    img(1, 600, 410, 280);
    Bocadillo(dl, a, t, p.texto(), 0.5f);
    Ayuda(dl, a, {{"A", T::T("Aceptar", "OK")}, {"B", T::T("Salir", "Quit")}});
    if (auto_on) pressed |= kA;
    if (pressed & kB) return Salir();
    if (pressed & (kA | kStart)) Siguiente();
  }

  // Bu: con que absorcion lucha el siguiente combate (las que tiene; las demas, en gris).
  void DrawEleccion(ImDrawList* dl, const S::Area& a, float t, uint16_t pressed, bool auto_on) {
    S::Banner(dl, a, kTitulos[g_modo]());
    Cara(dl, a, kSuperBu, false, true, t);
    const int n = kNAbsorciones;
    const float y0 = 190, paso = 90;
    S::Frame(dl, a.At(470, y0), a.At(1110, y0 + 30 + paso * float(n)), a.s);
    if (pressed & kUp) {
      for (int k = 1; k <= n; ++k) {
        const int c = (g_forma_bu + n - k) % n;
        if (g_absorbidos & (1 << c)) {
          g_forma_bu = c;
          break;
        }
      }
    }
    if (pressed & kDown) {
      for (int k = 1; k <= n; ++k) {
        const int c = (g_forma_bu + k) % n;
        if (g_absorbidos & (1 << c)) {
          g_forma_bu = c;
          break;
        }
      }
    }
    for (int k = 0; k < n; ++k) {
      const float y = y0 + 15 + paso * (float(k) + 0.5f);
      const bool tiene = (g_absorbidos & (1 << k)) != 0;
      if (k == g_forma_bu) S::Highlight(dl, a.At(485, y - paso * 0.5f + 6), a.At(1095, y + paso * 0.5f - 6), t);
      S::MenuText(dl, a.At(790, y), 38 * a.s, tiene ? kAbsorciones[k].nombre() : "???", k == g_forma_bu, !tiene);
    }
    Bocadillo(dl, a, t, T::T("¿Con qué absorción lucha Bu en el siguiente combate?",
                             "Which absorption does Buu fight with in the next battle?"), 0.22f);
    Ayuda(dl, a, {{"A", T::T("Elegir", "Select")}, {"B", T::T("Salir", "Quit")}});
    if (auto_on) {   // prueba: la absorcion mas nueva
      for (int k = n - 1; k >= 0; --k) {
        if (g_absorbidos & (1 << k)) {
          g_forma_bu = k;
          break;
        }
      }
      pressed |= kA;
    }
    if (pressed & kB) return Salir();
    if (pressed & (kA | kStart)) {
      REXLOG_INFO("dbz3 misiones: Bu elige {} (traje {})", kAbsorciones[g_forma_bu].nombre.es, TrajeBu(g_forma_bu));
      Siguiente();
    }
  }

  void DrawFinal(ImDrawList* dl, const S::Area& a, float t, uint16_t pressed, bool auto_on) {
    const Paso& p = g_pasos[size_t(g_paso)];
    S::Banner(dl, a, kTitulos[g_modo]());
    Cara(dl, a, p.quien, false, true, t);
    S::MenuText(dl, a.At(850, 330), 56 * a.s, T::T("¡Completado!", "Complete!"), true, false);
    Bocadillo(dl, a, t, p.texto(), 0.22f);
    Ayuda(dl, a, {{"A", T::T("Volver", "Back")}});
    if (auto_on) pressed |= kA;
    if (pressed & (kA | kStart | kB)) Terminar();
  }

  // Tablero: casillas unidas por caminos dorados; la ficha de Goku en la suya; izquierda/derecha elige la vecina a la
  // que ir y A mueve. Las casillas hechas salen en gris.
  void DrawTablero(ImDrawList* dl, const S::Area& a, float t, uint16_t pressed, bool auto_on) {
    S::Banner(dl, a, kTitulos[kTablero]());
    const std::vector<int> vec = Vecinos(g_nodo);
    int sel = int(std::find(vec.begin(), vec.end(), g_destino) - vec.begin());
    if (sel >= int(vec.size())) sel = 0;
    if (!vec.empty() && (pressed & (kRight | kDown | kRB))) sel = (sel + 1) % int(vec.size());
    if (!vec.empty() && (pressed & (kLeft | kUp | kLB))) sel = (sel + int(vec.size()) - 1) % int(vec.size());
    if (!vec.empty() && vec[size_t(sel)] != g_destino) g_mensaje.clear();   // otro camino: su descripcion
    if (!vec.empty()) g_destino = vec[size_t(sel)];
    // caminos
    for (const auto& e : kAristas) {
      const Nodo &n0 = kNodos[e[0]], &n1 = kNodos[e[1]];
      const bool activo = (e[0] == g_nodo && e[1] == g_destino) || (e[1] == g_nodo && e[0] == g_destino);
      dl->AddLine(a.At(n0.x, n0.y), a.At(n1.x, n1.y), IM_COL32(40, 20, 0, 230), 16 * a.s);
      const float pulso = activo ? 0.5f + 0.5f * std::sin(t * 6.0f) : 0.0f;
      dl->AddLine(a.At(n0.x, n0.y), a.At(n1.x, n1.y),
                  activo ? IM_COL32(255, 230, int(90 + 120 * pulso), 255) : IM_COL32(214, 160, 40, 255), 8 * a.s);
    }
    // casillas
    for (int i = 0; i < kNNodos; ++i) {
      const Nodo& n = kNodos[i];
      const bool hecho = (g_hecho & (1 << i)) != 0, destino = i == g_destino;
      const float w = 100;
      if (destino) S::Highlight(dl, a.At(n.x - w * 0.5f - 14, n.y - w * 0.5f - 14), a.At(n.x + w * 0.5f + 14, n.y + w * 0.5f + 14), t);
      if (n.rival >= 0) {
        DibujarCara(dl, a, n.rival, n.x - w * 0.5f, n.y - w * 0.5f, w, !hecho, nullptr, t, true);
      } else {
        S::Frame(dl, a.At(n.x - w * 0.5f, n.y - w * 0.5f), a.At(n.x + w * 0.5f, n.y + w * 0.5f), a.s);
        const char* icono = n.tipo == kSemilla ? T::T("Semilla", "Senzu") : T::T("Inicio", "Start");
        S::MenuText(dl, a.At(n.x, n.y), 28 * a.s, icono, !hecho, hecho);
      }
      S::MenuText(dl, a.At(n.x, n.y + w * 0.5f + 26), 28 * a.s, n.nombre(), destino, false);
    }
    // ficha de Goku sobre su casilla
    {
      const Nodo& n = kNodos[g_nodo];
      const float w = 78, bob = 6.0f * std::sin(t * 4.0f);
      DibujarCara(dl, a, kGoku, n.x - w * 0.5f - 46, n.y - w - 40 + bob, w, true, nullptr, t, false);
    }
    char estado[160];
    std::snprintf(estado, sizeof(estado), T::T("Turno %d - Vida %s", "Turn %d - Health %s"), g_turno + 1,
                  g_vida > 0 ? std::to_string(g_vida).c_str() : T::T("llena", "full"));
    S::MenuText(dl, a.At(985, 645), 30 * a.s, estado, true, false);
    std::string dice = g_mensaje;
    if (dice.empty()) {
      const Nodo& d = kNodos[g_destino];
      const bool hecho = (g_hecho & (1 << g_destino)) != 0;
      dice = std::string(d.nombre()) + ": ";
      if (hecho) dice += T::T("ya superada.", "already cleared.");
      else if (d.tipo == kEncuentro) dice += NombreDe(d.rival) + T::T(" te espera aquí.", " is waiting here.");
      else if (d.tipo == kSemilla) dice += T::T("aquí hay una semilla del ermitaño.", "there's a senzu bean here.");
      else if (d.tipo == kJefe) dice += NombreDe(d.rival) + T::T(", el jefe del tablero.", ", the board's boss.");
      else dice += T::T("donde empieza el viaje.", "where the journey begins.");
    }
    S::Speech(dl, a.At(60, 690), a.At(1140, 845), a.s, dice.c_str(), -1);
    Ayuda(dl, a, {{"LB", ""}, {"RB", T::T("Elegir camino", "Pick a path")}, {"A", T::T("Mover", "Move")},
                  {"B", T::T("Salir", "Quit")}});
    if (auto_on) {   // prueba: el camino mas corto al jefe por las casillas sin hacer
      g_destino = SiguienteAuto();
      pressed |= kA;
    }
    if (pressed & kB) return Salir();
    if ((pressed & (kA | kStart)) && !vec.empty()) Llegar(g_destino);
  }
  // Prueba: la vecina sin hacer (encuentro o semilla) con menos x; si no hay, hacia el jefe.
  static int SiguienteAuto() {
    const auto v = Vecinos(g_nodo);
    for (int pref : {kEncuentro, kSemilla}) {
      for (int n : v) {
        if (kNodos[n].tipo == pref && !(g_hecho & (1 << n))) return n;
      }
    }
    if ((g_hecho & kEncuentrosParaJefe) == kEncuentrosParaJefe) {
      for (int n : v) {
        if (kNodos[n].tipo == kJefe) return n;
      }
    }
    // hacia un encuentro pendiente: la vecina con mas x
    int mejor = v.empty() ? g_nodo : v.front();
    for (int n : v) {
      if (kNodos[n].x > kNodos[mejor].x) mejor = n;
    }
    return mejor;
  }

  struct Tex {
    std::unique_ptr<rex::ui::ImmediateTexture> tex;
    float w = 0, h = 0;
  };
  std::map<int, Tex> tex_iw_;
  uint32_t tex_iw_gen_ = ~0u;
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

void Abrir() {
  if (!Start()) REXLOG_WARN("dbz3 misiones: no se pudo abrir la pasarela");
}
bool Lista() { return true; }   // US y EU
const char* Por() {
  return T::T("Modos de Budokai 1 y 2: la saga de Namek, el tablero, Gotenks gordo y las absorciones de Bu.",
              "Budokai 1 and 2 modes: the Namek saga, the board, Fat Gotenks and Buu's absorptions.");
}
const bool g_hub = dbz3::hub::RegisterMode("pasarela", &Abrir, &Busy) && dbz3::hub::RegisterGate("pasarela", &Lista, &Por);

// Prueba: formas de cada personaje (tabla de combate 0x82329CF0 + 372 * ID: +208 cuantas, +212 + 20 * f: capsula,
// ID del modelo, indice, tipo) y escenarios (0x82323760 + 92 * i: nombre, +0x10 mapa) en el registro.
void LogDatos() {
  uint8_t* base = rex::Runtime::instance() ? rex::Runtime::instance()->virtual_membase() : nullptr;
  if (!base) return;
  for (int id : {0, 4, 6, 7, 9, 11, 19, 21, 27, 34, 35, 42, 22, 23}) {
    const uint32_t c = dbz3::GA(0x82329CF0u) + 372u * uint32_t(id);
    const uint32_t n = REX_LOAD_U32(c + 208);
    std::string s;
    for (uint32_t f = 0; f < n && f < 8; ++f) {
      const uint32_t p = c + 212 + 20 * f;
      s += " [" + std::to_string(f) + ": capsula " + std::to_string(REX_LOAD_U16(p)) + ", modelo " +
           std::to_string(REX_LOAD_U16(p + 2)) + ":" + std::to_string(REX_LOAD_U16(p + 4)) + ", tipo " +
           std::to_string(REX_LOAD_U8(p + 6)) + "]";
    }
    REXLOG_INFO("dbz3 misiones: prueba: formas del id {} ({}): {}", id, n, s);
  }
  // modelos (char96 0x8234ABB8 + 96 * ID: +4 trajes, +5 modelos por traje, +7 formas, +8 tabla de 12 B: fid)
  for (int id : {35, 84, 78, 79, 11, 64, 4, 90}) {
    const uint32_t c = dbz3::GA(0x8234ABB8u) + 96u * uint32_t(id);
    const uint32_t trajes = REX_LOAD_U8(c + 4), per = REX_LOAD_U8(c + 5), tabla = REX_LOAD_U32(c + 8);
    std::string s;
    for (uint32_t k = 0; tabla && k < trajes * per && k < 24; ++k) s += " " + std::to_string(REX_LOAD_U32(tabla + 12 * k));
    std::string idx;
    for (uint32_t f = 0; f < 8; ++f) idx += " " + std::to_string(REX_LOAD_U8(c + 16 + 8 * f + 2));
    REXLOG_INFO("dbz3 misiones: prueba: modelo {}: {} trajes x {} modelos, formas {} [{}], fids{}", id, trajes, per,
                REX_LOAD_U8(c + 7), idx, s);
  }
  for (uint32_t i = 0; i < 17; ++i) {
    std::string nombre;
    for (uint32_t k = 0; k < 16 && REX_LOAD_U8(dbz3::GA(0x82323760u) + 92 * i + k); ++k) nombre += char(REX_LOAD_U8(dbz3::GA(0x82323760u) + 92 * i + k));
    REXLOG_INFO("dbz3 misiones: prueba: escenario {} '{}' mapa {}", i, nombre, REX_LOAD_U32(dbz3::GA(0x82323760u) + 92 * i + 0x10));
  }
}

// PRUEBA: DBZ3_TEST_MISIONES="<modo>[,auto][,desde=N]" (modo 0 saga B1, 1 tablero, 2 Gotenks gordo, 3 Bu; "menu" =
// solo el menu) abre la pasarela cuando el menu principal lleva 3 s y entra en ese modo. "reset" pone su avance a 0.
const bool g_test = [] {
  const char* t = std::getenv("DBZ3_TEST_MISIONES");
  if (!t) return false;
  g_auto = std::strstr(t, "auto") != nullptr;
  if (std::strncmp(t, "tecnica", 7) == 0) {
    // "tecnica,K1,K2...": combates seguidos de Goku (quieto, 7000 de vida para que dure) contra Super Bu del COM con
    // la absorcion K (traje; 0 = el Bu de siempre) y sus capsulas de serie, IA en Avanzado, 60 s: el registro dice
    // cuando alguien gasta ki de golpe (tecnica) y la vida/ki de los dos cada segundo.
    std::vector<int> ks;
    for (const char* c = std::strchr(t, ','); c; c = std::strchr(c + 1, ',')) ks.push_back(std::atoi(c + 1));
    std::thread([ks] {
      for (int quieto = 0; quieto < 120;) {
        quieto = dbz3::ingame::GameScreen() == kMainScreen ? quieto + 1 : 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
      }
      for (size_t i = 0; i < ks.size(); ++i) {
        Reglas r;
        r.nivel_com = 2;
        r.traje_rival = std::max(0, TrajeBu(ks[i]));
        r.traza = true;
        const int caps[] = {3, 123, 124, 125};   // Absorcion (definitiva), Ill Flash, Ill Ball Attack
        std::copy(std::begin(caps), std::end(caps), r.capsulas_rival);
        const bool ok = dbz3::hub::RequestBattleEx(kGoku, kSuperBu, EscenarioPorMapa(kMapaCamara), true, 60, 7000, 0, 0, 0);
        REXLOG_INFO("dbz3 misiones: prueba: tecnica {}: {} (traje {}) {}", i + 1,
                    kAbsorciones[std::clamp(ks[i], 0, kNAbsorciones - 1)].nombre.es, r.traje_rival,
                    ok ? "pedido" : "no aceptado");
        std::atomic<bool> listo{false};
        VigilarCombate(("misiones: tecnica " + std::to_string(i + 1)).c_str(), r, [&listo](const Fin&) { listo = true; });
        while (!listo.load()) std::this_thread::sleep_for(std::chrono::milliseconds(50));
        for (int quieto = 0; quieto < 120;) {   // de vuelta en el menu principal
          quieto = dbz3::ingame::GameScreen() == kMainScreen ? quieto + 1 : 0;
          std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
      }
      REXLOG_INFO("dbz3 misiones: prueba: tecnicas acabadas");
    }).detach();
    return true;
  }
  const int modo = std::strncmp(t, "menu", 4) == 0 ? -1 : std::atoi(t);
  int desde = -1;
  if (const char* d = std::strstr(t, "desde=")) desde = std::atoi(d + 6);
  const bool reset = std::strstr(t, "reset") != nullptr;
  std::thread([modo, desde, reset] {
    for (int quieto = 0; quieto < 120;) {
      quieto = dbz3::ingame::GameScreen() == kMainScreen ? quieto + 1 : 0;
      std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    LeerIW();
    for (int i = 0; i < 200 && (!DatosLeidos() || g_iw_leyendo.load()); ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    LogDatos();
    if (modo >= 0 && modo < kModos) {
      if (reset) {
        GuardarProgreso(kClaves[modo], 0);
        if (modo == kTablero) {
          for (const char* k : {"tablero_nodo", "tablero_turno", "tablero_hecho", "tablero_vida"}) GuardarProgreso(k, 0);
        }
        if (modo == kBuu) GuardarProgreso("buu_absorbidos", 1);
      }
      if (desde >= 0) GuardarProgreso(kClaves[modo], desde);
    }
    const bool ok = Start();
    std::string motivo;
    if (modo >= 0 && modo < kModos) {
      std::lock_guard<std::mutex> lk(g_mu);
      const bool listo = Listo(modo, &motivo);
      REXLOG_INFO("dbz3 misiones: prueba: modo {} listo {} ({})", kClaves[modo], listo, motivo);
      g_cursor = modo;
      if (ok && listo) Entrar(modo);
    } else {   // solo el menu: lo que esta disponible y por que no (WL-20: falta un mod)
      std::lock_guard<std::mutex> lk(g_mu);
      for (int m = 0; m < kModos; ++m) {
        const bool listo = Listo(m, &motivo);
        REXLOG_INFO("dbz3 misiones: prueba: modo {} listo {} ({})", kClaves[m], listo, motivo);
      }
    }
    REXLOG_INFO("dbz3 misiones: prueba: {}", ok ? "abierta" : "no abre");
  }).detach();
  return true;
}();

}  // namespace
}  // namespace dbz3::pasarela::misiones
