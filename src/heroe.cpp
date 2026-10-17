// dbz3 - Modo Heroe (beta; ronda 3 frente G, ronda 4 frente R5): creacion de personaje al estilo Xenoverse /
// Ultimate Tenkaichi. Raza -> cuerpo (un luchador del juego), apariencia (sus trajes, tabla del select), estilo de
// lucha = el moveset de otro luchador (maestro o no). En combate el jugador ES el luchador del estilo (movimientos,
// tecnicas, capsulas, voz) con el MODELO del cuerpo: mientras dura la partida del heroe, el registro de combate del
// estilo (char372 +212 + 20*forma: +2 ID cuyo modelo se carga, +4 su forma en char96) apunta al modelo del cuerpo
// (Vestir/Quitar; es lo mismo que el cambio de modelo B3->B3 nativo validado, pero en memoria y solo en esa partida).
// Bucle de runs: sagas de 3 combates encadenadas; cada combate ganado da experiencia, el nivel sube vida y dano.
// Lo usa rpg.cpp (vista kHeroeP y el reto kHeroe) con su candado puesto. Guardado: claves heroe_* de rpg.toml.
#include <rex/logging.h>
#include <rex/runtime.h>
#include <toml++/toml.hpp>

#include <algorithm>
#include <cstring>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "launcher/i18n.h"
#include "modos.h"

namespace dbz3::pasarela {
int RangoMejora(const std::string& clave, int valor, int pe_por_paso, int pe_primera);   // pasarela.h
}

namespace dbz3::heroe {
namespace {
namespace M = dbz3::modos;
namespace T = dbz3::i18n;   // textos nuevos: T::T(es, en)
using M::L;

struct Fisico {
  int id, trajes;   // trajes: los de la tabla del select (0x8245F150) si ya esta en memoria; si no, estos
};
struct Raza {
  const char *es, *en, *fr, *de, *it;
  Fisico cuerpos[4];
  int n;
};
const Raza kRazas[] = {
    {"Saiyan", "Saiyan", "Saiyan", "Saiyajin", "Saiyan", {{0, 3}, {7, 2}, {39, 2}, {8, 2}}, 4},
    {"Humano", "Human", "Humain", "Mensch", "Umano", {{10, 2}, {12, 2}, {13, 2}, {15, 2}}, 4},
    {"Namekiano", "Namekian", "Namek", "Namekianer", "Namecciano", {{11, 2}}, 1},
    {"Raza de Freezer", "Frieza Race", "Race de Freezer", "Freezers Volk", "Razza di Freezer", {{27, 2}, {38, 2}}, 2},
    {"Majin", "Majin", "Majin", "Majin", "Majin", {{34, 2}, {36, 2}}, 2},
    {"Androide", "Android", "Androïde", "Androide", "Androide", {{30, 2}, {29, 2}, {28, 2}, {33, 2}}, 4},
};
constexpr int kNumRazas = int(std::size(kRazas));

// Estilo de lucha: el moveset de ese luchador. Los 6 primeros son maestros (bonus por las Reglas + una capsula de
// habilidad); el resto, su moveset tal cual.
struct Estilo {
  int id;
  int dano = 100, recibido = 100;   // %
  int vida = 0;                     // vida extra sobre 3000
  int ki = 0;                       // Reglas::ki_drena (< 0: el ki se recarga solo)
  int hab = 0;                      // capsula de habilidad "para todos" (0 = ninguna)
  const char* hab_n = nullptr;
  const char* tec = nullptr;        // sus tecnicas (nombres del juego en ingles)
};
// ponytail: nombres de tecnicas escritos a mano (en B3 son imagenes de data_usi, no texto); solo los de los
// maestros, que ya estaban validados. Leer los rotulos #AZT del jugador si hace falta para todos.
const Estilo kEstilos[] = {
    {0, 110, 100, 500, 0, 378, "Rage!", "Kamehameha, Warp Kamehameha"},
    {7, 120, 110, 0, 0, 385, "Serious!!", "Galick Gun, Final Flash"},
    {11, 100, 85, 500, 0, 381, "Spirit!", "Makankosappo, Destructive Wave"},
    {10, 100, 100, 0, -3, 445, "Concentration", "Kienzan, Kamehameha"},
    {27, 115, 100, 0, -2, 384, "Serious!", "Death Beam, Second Form"},
    {33, 100, 90, 1000, 0, 382, "Spirit!!", "Kamehameha, #17 Absorption"},
    {4}, {8}, {12}, {13}, {18}, {19}, {20}, {30}, {29}, {28}, {34}, {36}, {37}, {38}, {39}, {40}, {14},
};
constexpr int kNumEstilos = int(std::size(kEstilos));
constexpr int kNumMaestros = 6;

// Sagas del bucle de runs: 3 combates cada una (rival; alternativo si el rival es el luchador del estilo, porque
// el registro de combate es por ID y el rival saldria con el cuerpo del heroe).
struct Paso {
  int rival, alt, vida, escenario;
  const char *es, *en, *fr, *de, *it;
};
struct SagaDef {
  const char *es, *en, *fr, *de, *it;
  Paso p[3];
};
const SagaDef kSagas[] = {
    {"Saga Saiyan", "Saiyan Saga", "Saga Saiyan", "Saiyajin-Saga", "Saga dei Saiyan",
     {{42, 43, 1500, 5, "Unos Saibaman atacan la ciudad del Oeste. ¡Demuestra quién eres!",
       "Saibamen are attacking West City. Show them who you are!",
       "Des Saibaman attaquent la Capitale de l'Ouest. Montre-leur qui tu es !",
       "Saibaman greifen die Westliche Hauptstadt an. Zeig ihnen, wer du bist!",
       "Dei Saibaman attaccano la Città dell'Ovest. Mostra loro chi sei!"},
      {19, 18, 3000, 4, "Nappa ha visto tu fuerza y viene a por ti. ¡No te rindas!",
       "Nappa saw your strength and is coming for you. Don't give up!",
       "Nappa a vu ta force et vient te chercher. N'abandonne pas !",
       "Nappa hat deine Stärke gesehen und kommt zu dir. Gib nicht auf!",
       "Nappa ha visto la tua forza e viene a cercarti. Non arrenderti!"},
      {7, 18, 3500, 6, "El príncipe Vegeta en persona. ¡Es el combate decisivo!",
       "Prince Vegeta himself. This is the decisive fight!", "Le prince Vegeta en personne. C'est le combat décisif !",
       "Prinz Vegeta höchstpersönlich. Das ist der entscheidende Kampf!",
       "Il principe Vegeta in persona. È lo scontro decisivo!"}}},
    {"Saga de Namek", "Namek Saga", "Saga de Namek", "Namek-Saga", "Saga di Namek",
     {{21, 20, 2500, 3, "Las Fuerzas Especiales llegan a Namek. Recoome quiere jugar contigo.",
       "The Ginyu Force lands on Namek. Recoome wants to play with you.",
       "Les Forces Spéciales arrivent sur Namek. Recoome veut jouer avec toi.",
       "Die Ginyu-Truppe landet auf Namek. Recoome will mit dir spielen.",
       "Le Forze Speciali arrivano su Namek. Recoome vuole giocare con te."},
      {20, 21, 3000, 3, "El capitán Ginyu en persona. ¡Que no te robe el cuerpo!",
       "Captain Ginyu himself. Don't let him steal your body!",
       "Le capitaine Ginyu en personne. Ne le laisse pas voler ton corps !",
       "Kommandant Ginyu persönlich. Lass dir nicht den Körper stehlen!",
       "Il capitano Ginyu in persona. Non farti rubare il corpo!"},
      {27, 38, 4000, 2, "Freezer, el emperador del universo. ¡Todo Namek depende de ti!",
       "Frieza, emperor of the universe. All of Namek depends on you!",
       "Freezer, l'empereur de l'univers. Tout Namek compte sur toi !",
       "Freezer, der Herrscher des Universums. Ganz Namek zählt auf dich!",
       "Freezer, l'imperatore dell'universo. Tutto Namek dipende da te!"}}},
    {"Saga de los Androides", "Android Saga", "Saga des Cyborgs", "Androiden-Saga", "Saga dei Cyborg",
     {{30, 29, 2500, 1, "La Androide 18 bloquea la carretera. Es rápida: ¡vigila su guardia!",
       "Android 18 blocks the road. She's fast: watch her guard!",
       "C-18 bloque la route. Elle est rapide : surveille sa garde !",
       "C18 blockiert die Straße. Sie ist schnell: achte auf ihre Deckung!",
       "C-18 blocca la strada. È veloce: attento alla sua guardia!"},
      {43, 42, 3000, 0, "Cell ha dejado a sus hijos en el ring. ¡Acaba con los Cell Jr.!",
       "Cell left his children in the ring. Take down the Cell Jr.!",
       "Cell a laissé ses enfants sur le ring. Bats les Cell Junior !",
       "Cell hat seine Kinder im Ring gelassen. Besiege die Cell Jr.!",
       "Cell ha lasciato i suoi figli sul ring. Sconfiggi i Cell Jr.!"},
      {33, 28, 4500, 0, "Cell perfecto te espera en su torneo. ¡Es el combate decisivo!",
       "Perfect Cell awaits you in his tournament. This is the decisive fight!",
       "Cell parfait t'attend dans son tournoi. C'est le combat décisif !",
       "Der perfekte Cell erwartet dich in seinem Turnier. Das ist der entscheidende Kampf!",
       "Cell perfetto ti aspetta nel suo torneo. È lo scontro decisivo!"}}},
    {"Saga de Bu", "Buu Saga", "Saga de Boo", "Boo-Saga", "Saga di Majin Bu",
     {{37, 35, 3000, 7, "Dabura, el rey de los demonios, quiere tu energía para Bu.",
       "Dabura, king of demons, wants your energy for Buu.",
       "Dabura, le roi des démons, veut ton énergie pour Boo.",
       "Dabura, der Dämonenkönig, will deine Energie für Boo.",
       "Dabura, il re dei demoni, vuole la tua energia per Bu."},
      {34, 36, 4000, 6, "Bu se ha despertado. ¡No dejes que te convierta en chocolate!",
       "Buu has awakened. Don't let him turn you into chocolate!",
       "Boo s'est réveillé. Ne le laisse pas te changer en chocolat !",
       "Boo ist erwacht. Lass dich nicht in Schokolade verwandeln!",
       "Bu si è risvegliato. Non lasciare che ti trasformi in cioccolato!"},
      {36, 35, 5000, 5, "Bu el puro, la forma más peligrosa. ¡Es el combate final!",
       "Kid Buu, the most dangerous form. This is the final fight!",
       "Boo originel, la forme la plus dangereuse. C'est le combat final !",
       "Kid Boo, die gefährlichste Form. Das ist der letzte Kampf!",
       "Bu puro, la forma più pericolosa. È lo scontro finale!"}}},
};
constexpr int kNumSagas = int(std::size(kSagas));
// Tercer rival de cada combate para cuando el rival y su alternativo chocan con el estilo y el cuerpo del heroe
// (p. ej. cuerpo Freezer 2 con el estilo de Freezer). Salen luchadores de la misma saga; al final, 42/43 (nunca son
// estilo ni cuerpo).
constexpr int kAlt2[kNumSagas][3] = {{43, 43, 19}, {43, 43, 20}, {28, 43, 30}, {43, 37, 43}};

int g_raza = 0, g_cuerpo = 0, g_traje = 0, g_estilo = 0;
constexpr int kNivelMax = 20, kExpNivel = 3;   // nivel = 1 + exp / 3, hasta el 20
int g_exp = 0, g_saga = 0;   // combates ganados (todas las partidas) y saga siguiente (crece sin tope: vueltas)

// ---- Memoria del guest (solo imagen US: firma "_charasel.cpp", como roster_ext.cpp) ----
constexpr uint32_t kChar372 = 0x82329CF0;
constexpr uint32_t kIdToSlot = 0x82020668;
constexpr uint32_t kCostumeCounts = 0x8245F150;
constexpr uint32_t kSignature = 0x82020700;
uint8_t* Base() {
  rex::Runtime* rt = rex::Runtime::instance();
  uint8_t* b = rt ? rt->virtual_membase() : nullptr;
  return b && std::memcmp(b + kSignature, "_charasel.cpp", 13) == 0 ? b : nullptr;
}
uint32_t Be32(const uint8_t* p) { return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3]; }

void W32(uint8_t* p, uint32_t v) { p[0] = uint8_t(v >> 24), p[1] = uint8_t(v >> 16), p[2] = uint8_t(v >> 8), p[3] = uint8_t(v); }
void W16(uint8_t* p, uint32_t v) { p[0] = uint8_t(v >> 8), p[1] = uint8_t(v); }
// Cara del marcador (como tagteam.cpp): filas de 16 B (ID, mascara de trajes, fichero de caras, ficha; fin = ID -1).
// Al empezar el combate el juego busca la fila del jugador (sub_820E6D70) y carga su hoja de caras y su retrato.
constexpr uint32_t kFaceTable = 0x82324468;
uint32_t FilaCara(uint8_t* b, uint32_t id, uint32_t traje) {
  uint32_t any = 0;
  for (uint32_t row = kFaceTable; row < kFaceTable + 16 * 256; row += 16) {
    const uint32_t rid = Be32(b + row);
    if (rid == 0xFFFFFFFFu) break;
    if (rid != id) continue;
    if (Be32(b + row + 4) & (1u << (traje & 31))) return row;
    if (!any) any = row;
  }
  return any;
}

struct Puesto {   // lo original del estilo mientras lleva el cuerpo del heroe
  int estilo = -1, cuerpo = -1, traje = -1;
  uint8_t c372[372];
  std::vector<std::pair<uint32_t, uint32_t>> caras;   // fila de la tabla de caras -> su fichero original
} g_puesto;

void Quitar() {
  if (g_puesto.estilo < 0) return;
  if (uint8_t* b = Base()) {
    std::memcpy(b + kChar372 + uint32_t(g_puesto.estilo) * 372, g_puesto.c372, 372);
    for (const auto& [row, fid] : g_puesto.caras) W32(b + row + 8, fid);
    REXLOG_INFO("dbz3 heroe: estilo {} vuelve a su modelo", g_puesto.estilo);
  }
  g_puesto.caras.clear();
  g_puesto.estilo = g_puesto.cuerpo = g_puesto.traje = -1;
}

// El luchador `estilo` carga en combate el modelo de `cuerpo` (su forma base) y su cara en el marcador.
// Transformaciones: el cuerpo no tiene modelo (ni cara, ni aura) para las formas del estilo, asi que con otro cuerpo
// las formas 1+ piden una capsula que el estilo no puede llevar (char372 +212+20f = capsula que exige la forma):
// el heroe pelea en su forma base, sin transformaciones a medias. Con su propio cuerpo, todo como siempre.
// ponytail: la voz y los gritos siguen siendo los del estilo: el banco de gritos (char372 +0x24, lang_*) va por
// indices del moveset (los ports reconstruyen el banco del donante con gritos.py); el del cuerpo tiene otro orden
// y otro numero (55/56) y podria pedir un sonido que no existe. Altura: ver Altura().
void Vestir(int estilo, int cuerpo, int traje) {
  if (estilo == cuerpo || estilo < 0 || estilo >= 44 || cuerpo < 0 || cuerpo >= 44) return Quitar();
  if (g_puesto.estilo == estilo && g_puesto.cuerpo == cuerpo && g_puesto.traje == traje) return;
  Quitar();
  uint8_t* b = Base();
  if (!b) return;
  uint8_t* r = b + kChar372 + uint32_t(estilo) * 372;
  const uint8_t* rc = b + kChar372 + uint32_t(cuerpo) * 372;
  if (!Be32(r) || !Be32(rc)) return;   // sin registro de combate (recortados)
  std::memcpy(g_puesto.c372, r, 372);
  const uint32_t nf = std::clamp<uint32_t>(Be32(r + 0xD0), 1, 8);
  const uint32_t tapon = estilo == 27 ? 6 : 96;   // capsula de otro (Kamehameha de Goku / Second Form de Freezer)
  for (uint32_t f = 0; f < nf; ++f) {
    std::memcpy(r + 212 + 20 * f + 2, rc + 212 + 2, 4);   // ID del modelo + su forma: la base del cuerpo
    if (f > 0) W16(r + 212 + 20 * f, tapon);
  }
  if (const uint32_t fila = FilaCara(b, uint32_t(cuerpo), uint32_t(traje))) {
    const uint32_t cara = Be32(b + fila + 8);
    for (uint32_t row = kFaceTable; row < kFaceTable + 16 * 256 && Be32(b + row) != 0xFFFFFFFFu; row += 16) {
      if (Be32(b + row) != uint32_t(estilo)) continue;
      g_puesto.caras.emplace_back(row, Be32(b + row + 8));
      W32(b + row + 8, cara);
    }
  }
  g_puesto.estilo = estilo;
  g_puesto.cuerpo = cuerpo;
  g_puesto.traje = traje;
  REXLOG_INFO("dbz3 heroe: estilo {} ({}) con el cuerpo {} ({}) traje {}, {} formas (1+ bloqueadas), {} filas de cara",
              estilo, M::Nombre(estilo), cuerpo, M::Nombre(cuerpo), traje + 1, nf, g_puesto.caras.size());
}

const Fisico& CuerpoAct() { return kRazas[g_raza].cuerpos[g_cuerpo]; }
// Trajes del cuerpo: la tabla del select por casilla (la que llena el juego, con los trajes de mods), o los fijos.
int TrajesDe(const Fisico& c) {
  if (uint8_t* b = Base()) {
    const uint32_t slot = uint32_t(b[kIdToSlot + c.id * 2]) << 8 | b[kIdToSlot + c.id * 2 + 1];
    if (slot < 38 && b[kCostumeCounts + slot] >= 1 && b[kCostumeCounts + slot] <= 8) return b[kCostumeCounts + slot];
  }
  return c.trajes;
}
int Ciclo(int v, int d, int n) { return (v + d + n) % n; }
void Ajustar() {
  g_raza = std::clamp(g_raza, 0, kNumRazas - 1);
  g_cuerpo = std::clamp(g_cuerpo, 0, kRazas[g_raza].n - 1);
  g_traje = std::clamp(g_traje, 0, TrajesDe(CuerpoAct()) - 1);
  g_estilo = std::clamp(g_estilo, 0, kNumEstilos - 1);
}
const Estilo& EstiloDeId(int id) {
  for (const Estilo& e : kEstilos) {
    if (e.id == id) return e;
  }
  static const Estilo nada{-1};
  return nada;
}
bool EsMaestroId(int id) {
  for (int i = 0; i < kNumMaestros; ++i) {
    if (kEstilos[i].id == id) return true;
  }
  return false;
}
std::string Pct(int v) { return (v > 0 ? "+" : "") + std::to_string(v) + "%"; }
const Paso& PasoDe(int saga, int k) { return kSagas[std::max(saga, 0) % kNumSagas].p[std::clamp(k, 0, 2)]; }
}  // namespace

void Leer(const toml::table& t) {
  g_raza = int(t["heroe_raza"].value_or(int64_t(0)));
  g_cuerpo = int(t["heroe_cuerpo"].value_or(int64_t(0)));
  g_traje = int(t["heroe_traje"].value_or(int64_t(0)));
  g_estilo = int(t["heroe_estilo"].value_or(t["heroe_maestro"].value_or(int64_t(0))));   // antes: maestro (0-5)
  g_exp = std::max(0, int(t["heroe_exp"].value_or(int64_t(0))));
  g_saga = std::max(0, int(t["heroe_saga"].value_or(int64_t(0))));
  Ajustar();
}
void Escribir(std::ostream& o) {
  o << "heroe_raza = " << g_raza << "\nheroe_cuerpo = " << g_cuerpo << "\nheroe_traje = " << g_traje
    << "\nheroe_estilo = " << g_estilo << "\nheroe_maestro = " << std::min(g_estilo, kNumMaestros - 1)
    << "\nheroe_exp = " << g_exp << "\nheroe_saga = " << g_saga << "\n";
}

int Cuerpo() { return CuerpoAct().id; }
int Traje() { return g_traje; }
int EstiloId() { return kEstilos[g_estilo].id; }
int Saga() { return g_saga; }
int Nivel() { return std::min(kNivelMax, 1 + g_exp / kExpNivel); }
int g_nivel_antes = 0;   // nivel antes de la ultima saga (pantalla final: subida de nivel)
int NivelAntes() { return g_nivel_antes ? g_nivel_antes : Nivel(); }
int Vida(int estilo) { return 3000 + std::max(EstiloDeId(estilo).vida, 0) + 100 * (Nivel() - 1); }
// Partida guardada: extra = 1000 + cuerpo (antes de la ronda 4: el indice del maestro y jugador = cuerpo).
int CuerpoDe(int extra, int jugador) { return extra >= 1000 ? extra - 1000 : jugador; }

// Carrusel de estilos (rpg.cpp): el luchador del estilo elegido +- d y si es maestro.
int EstiloVecino(int d) { return kEstilos[Ciclo(g_estilo, d, kNumEstilos)].id; }
bool EsMaestro(int id) { return EsMaestroId(id); }
// Pone el estilo de una partida guardada como el elegido (el carrusel y LB/RB parten de el). Un ID que ya no es estilo
// (partidas de antes de la ronda 4) deja el que estaba.
void ElegirEstilo(int id) {
  for (int i = 0; i < kNumEstilos; ++i) {
    if (kEstilos[i].id == id) g_estilo = i;
  }
}

// Filas de la pantalla de creacion: 0 raza, 1 cuerpo, 2 apariencia, 3 estilo (4 = empezar, en rpg.cpp).
void Mover(int fila, int dir) {
  switch (fila) {
    case 0: g_raza = Ciclo(g_raza, dir, kNumRazas), g_cuerpo = g_traje = 0; break;
    case 1: g_cuerpo = Ciclo(g_cuerpo, dir, kRazas[g_raza].n), g_traje = 0; break;
    case 2: g_traje = Ciclo(g_traje, dir, TrajesDe(CuerpoAct())); break;
    case 3: g_estilo = Ciclo(g_estilo, dir, kNumEstilos); break;
    default: break;
  }
  Ajustar();
}
std::string Fila(int fila) {
  const Raza& r = kRazas[g_raza];
  switch (fila) {
    case 0: return std::string(L("Raza: ", "Race: ", "Race : ", "Volk: ", "Razza: ")) + L(r.es, r.en, r.fr, r.de, r.it);
    case 1: return std::string(L("Cuerpo: ", "Body: ", "Corps : ", "Körper: ", "Corpo: ")) + M::Nombre(Cuerpo());
    case 2:
      return std::string(L("Apariencia: traje ", "Look: costume ", "Apparence : tenue ", "Aussehen: Kostüm ",
                           "Aspetto: costume ")) + std::to_string(g_traje + 1) + " / " +
             std::to_string(TrajesDe(CuerpoAct()));
    case 3:
      return std::string(L("Estilo: ", "Style: ", "Style : ", "Stil: ", "Stile: ")) + M::Nombre(EstiloId()) +
             (g_estilo < kNumMaestros ? L(" (maestro)", " (master)", " (maître)", " (Meister)", " (maestro)") : "");
    default: return L("¡Empezar la misión!", "Start the mission!", "Commencer la mission !", "Mission starten!",
                      "Inizia la missione!");
  }
}
// Nivel y experiencia en una linea: "Nivel 2  XP 1/3" (+ lo que se apunta al acabar la saga en curso).
std::string LineaNivel(int pendientes) {
  std::string s = std::string(L("Nivel ", "Level ", "Niveau ", "Stufe ", "Livello ")) + std::to_string(Nivel());
  if (Nivel() >= kNivelMax) return s + "  (" + T::T("máximo", "max") + ")";
  s += "  XP " + std::to_string(g_exp - kExpNivel * (Nivel() - 1)) + "/" + std::to_string(kExpNivel);
  if (pendientes > 0) s += "  (+" + std::to_string(pendientes) + " " + T::T("al acabar la saga", "when the saga ends") + ")";
  return s;
}
const char* NombreSaga(int saga) {
  const SagaDef& sg = kSagas[std::max(saga, 0) % kNumSagas];
  return L(sg.es, sg.en, sg.fr, sg.de, sg.it);
}
// El estilo en dos lineas: bonus ("Piccolo (maestro): vida 3500, recibido -15%") y tecnicas.
std::string DetalleEstilo(int id) {
  const Estilo& x = EstiloDeId(id);
  std::string s = M::Nombre(id) + (EsMaestroId(id) ? L(" (maestro)", " (master)", " (maître)", " (Meister)", " (maestro)") : "") +
                  ": " + L("vida ", "health ", "vie ", "Leben ", "vita ") + std::to_string(Vida(id));
  if (x.dano != 100) s += std::string(", ") + L("daño ", "damage ", "dégâts ", "Schaden ", "danni ") + Pct(x.dano - 100);
  if (x.recibido != 100) {
    s += std::string(", ") + L("recibido ", "taken ", "subis ", "erlitten ", "subiti ") + Pct(x.recibido - 100);
  }
  if (x.ki < 0) s += std::string(", ") + L("el ki se recarga solo", "ki recharges", "ki auto", "Ki lädt sich auf", "ki si ricarica");
  s += std::string("\n") + L("Técnicas: ", "Techniques: ", "Techniques : ", "Techniken: ", "Tecniche: ");
  if (x.tec) {
    s += std::string(x.tec) + ", " + x.hab_n;
  } else {
    s += std::string(L("todo el moveset de ", "the whole moveset of ", "tout le style de ", "der ganze Stil von ",
                       "tutto lo stile di ")) + M::Nombre(id) +
         L(" (sus cápsulas)", " (his capsules)", " (ses capsules)", " (seine Kapseln)", " (le sue capsule)");
  }
  return s;
}
// Vista previa del estilo elegido en la creacion (bocadillo): bonus, tecnicas, nivel y la saga siguiente.
std::string Detalle() {
  return DetalleEstilo(EstiloId()) + "\n" + LineaNivel(0) + "  -  " + NombreSaga(g_saga);
}

// Reglas del combate (bonus del estilo si es maestro + nivel) y el modelo del cuerpo puesto al estilo.
M::Reglas Reglas(int estilo, int extra, int traje) {
  Vestir(estilo, CuerpoDe(extra, estilo), traje);
  const Estilo& x = EstiloDeId(estilo);
  M::Reglas g;
  g.dano_hecho = (x.id >= 0 ? x.dano : 100) + 2 * (Nivel() - 1);
  g.dano_recibido = x.id >= 0 ? x.recibido : 100;
  g.ki_drena = x.id >= 0 ? x.ki : 0;
  if (x.hab) {
    g.ncaps = 1;
    g.caps[0] = x.hab;   // se suma a las suyas
  }
  return g;
}

// Combate k de la saga. El rival sale distinto del luchador del estilo (el registro de combate es por ID: saldria con
// el cuerpo del heroe) y del cuerpo (seria un espejo): el primero de rival / alternativo / tercero / 42 / 43 que sirva.
void Mision(int saga, int k, int estilo, int cuerpo, int* rival, int* vida, int* escenario) {
  const Paso& p = PasoDe(saga, k);
  const int cand[] = {p.rival, p.alt, kAlt2[std::max(saga, 0) % kNumSagas][std::clamp(k, 0, 2)], 42, 43};
  *rival = p.alt;
  for (const int c : cand) {
    if (c != estilo && c != cuerpo) {
      *rival = c;
      break;
    }
  }
  *vida = p.vida + 500 * (std::max(saga, 0) / kNumSagas);   // cada vuelta a las sagas, rivales mas duros
  *escenario = p.escenario;
}
// Texto del combate k: la historia del rival de la saga; si por el estilo o el cuerpo salio otro (`rival`), no se
// nombra a uno que no va a salir.
std::string MisionTexto(int saga, int k, int rival) {
  const SagaDef& sg = kSagas[std::max(saga, 0) % kNumSagas];
  const Paso& p = PasoDe(saga, k);
  return std::string(L(sg.es, sg.en, sg.fr, sg.de, sg.it)) + ": " +
         (rival == p.rival ? L(p.es, p.en, p.fr, p.de, p.it)
                           : T::T("¡Un rival inesperado se cruza en tu camino!", "An unexpected rival crosses your path!"));
}

// Fin de una partida (no guardada a medias): experiencia por combate ganado; si se completo, la saga siguiente.
void FinRun(int ganados, bool victoria) {
  const int antes = g_nivel_antes = Nivel();
  if (victoria)   // escalafon: saga completada (15 + nivel), una vez por saga
    dbz3::pasarela::RangoMejora("rango_heroe_saga_" + std::to_string(g_saga), 1, 15 + Nivel(), 0);
  g_exp += std::max(ganados, 0);
  if (victoria) ++g_saga;
  REXLOG_INFO("dbz3 heroe: fin de saga ({} ganados, {}): exp {}, nivel {} -> {}, saga siguiente {}", ganados,
              victoria ? "completada" : "perdida", g_exp, antes, Nivel(), g_saga);
}

// Fuera de la partida del heroe, el estilo vuelve a su modelo (rpg.cpp, Sincronizar).
void Sync(bool activo) {
  if (!activo) Quitar();
}

}  // namespace dbz3::heroe
