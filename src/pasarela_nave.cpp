// dbz3 - Nave de Babidi de Budokai 2 (frente R2, ronda 4): las 4 pruebas de un jugador que dan Kili y los premios
// que se consiguen al juntar Kili. pasarela_misiones.cpp solo llama a Entrar/Dibujar/Resultado (con su g_mu) y pide
// los combates nativos (RequestBattleEx). Necesita la ISO de B2 del jugador (como el tablero).
// Reglas (b2_reglas.md s8; pagos comprobados en el ELF europeo, 0x265AB0: prueba 6..9 -> x25, x2 con (cuadros + 60) / 60,
// x4, x3; descompilado de Ghidra E:/DBZ3HD_trabajo/ghidra/salida/b2.c):
//   I   derrota a todos los enemigos que puedas hasta caer (la vida se conserva): 25 Kili por enemigo
//   II  aguanta con una sola barra de vida: 2 Kili por segundo
//   III golpea al enemigo durante 99 s: 4 Kili por golpe
//   IV  devuelve ondas de ki durante 99 s: 3 Kili por onda (b2.c: FUN_001e2610 -> FUN_001ebf40(luchador, 3) suma 1 en
//       +0x9bb8 cuando la onda da "devuelta" (5) en FUN_001f7fc0, guardia pulsada hace < 5 cuadros; FUN_001d41e0 lo da
//       como resultado y FUN_00265ab0 paga x3). En B3 es el mismo suceso: combate.cpp lo cuenta (sub_82122488 desde
//       sub_82119B38, OndasDevueltas) y Resultado lo toma de ahi. El rival de B2 en esta prueba no ataca (FUN_001d1c40 da
//       -2), vuelve a la guardia (FUN_001d1db0 da 1) y devuelve todas las ondas (FUN_001d1980 da 2): es el rally; aqui
//       combate.cpp OndasModo lo pone de muneco en guardia con Deflect-Back mientras dura el combate.
// El Kili se acumula (progreso "nave_kili") y cada umbral da su premio: tabla del ELF de la copia del jugador (SLES
// 0x278320 en la europea: 43 filas u16 ID + u16 Kili, crecientes, acaba en 0,0; se busca por sus dos primeras filas).
// 486-489 = Dabura, Majin Bu, Super Bu y Kid Bu (personajes); el resto, capsulas de B2 (nombres de la FAQ F3 por Kili).
// ponytail: rivales al azar entre los de B2 (los del original sin leer: tabla del ELF por encontrar); escenario fijo
// (B3 no tiene la nave); los premios se ensenan y avisan, todavia no entran en el inventario de capsulas; pantallas
// con la letra y el marco de B3 (los graficos de la nave en la ISO estan sin localizar).
#include "pasarela.h"

#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/runtime.h>

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <memory>

#include <rex/ui/immediate_drawer.h>

#include "generated/dbz3_init.h"
#include "eu_map.h"
#include "ingame/game_style.h"
#include "launcher/i18n.h"
#include "launcher/launcher_state.h"

namespace dbz3::pasarela::b2 {
bool Disponible();
}
namespace dbz3::combate {   // combate.cpp: ondas que el luchador del lado dado ha devuelto (prueba IV)
int OndasDevueltas(int lado);
void OndasCero();
void OndasModo(bool on);   // el rival pasa a ser el muneco en guardia que devuelve toda onda (prueba IV, como en B2)
}

namespace dbz3::pasarela::nave {
namespace {

namespace T = dbz3::i18n;
namespace S = dbz3::ingame::style;
namespace M = dbz3::ingame::style::mode;

constexpr uint16_t kUp = 0x0001, kDown = 0x0002, kLeft = 0x0004, kRight = 0x0008, kStart = 0x0010, kA = 0x1000,
                   kB = 0x2000;

struct Prueba {
  const char *es, *en, *ayuda_es, *ayuda_en;
  int pago, tiempo;
};
const Prueba kPruebas[4] = {
    {"Prueba I", "Trial I", "Derrota a todos los enemigos que puedas hasta caer. 25 Kili por enemigo.",
     "Defeat as many enemies as you can until you fall. 25 Kili per enemy.", 25, 0},
    {"Prueba II", "Trial II", "Aguanta con una sola barra de vida. 2 Kili por segundo.",
     "Survive with a single health bar. 2 Kili per second.", 2, 99},
    {"Prueba III", "Trial III", "Golpea al enemigo durante 99 segundos. 4 Kili por golpe.",
     "Hit the enemy for 99 seconds. 4 Kili per hit.", 4, 99},
    {"Prueba IV", "Trial IV", "Devuelve ondas de ki durante 99 segundos. 3 Kili por onda.",
     "Deflect ki waves for 99 seconds. 3 Kili per wave.", 3, 99}};
// Nombres de los premios por su Kili (FAQ F3, version inglesa). ponytail: las imagenes con sus nombres en la ISO estan
// sin localizar.
struct Nombre {
  int kili;
  const char* nombre;
};
const Nombre kNombres[] = {
    {100, "Dabura"}, {320, "Hell Blitz"}, {430, "Evil Tornado"}, {540, "Demonic Will"}, {800, "Yakon"},
    {965, "Demon Realm Guard"}, {1200, "Majin Buu"}, {1420, "Innocence Cannon"}, {1530, "Hell Blow Shoot"},
    {1695, "Babidi's Scope"}, {1805, "Merry Swing"}, {2025, "Mage Guard"}, {2190, "Enjoy Browning"},
    {2400, "Super Buu"}, {2620, "Ill Flash"}, {2730, "Hate Ray Cannon"}, {2840, "Innocence Dance"},
    {2950, "Badness Throw"}, {3000, "Super Saiyan (Goku)"}, {3060, "Hate Dancing"}, {3170, "Vice Browning"},
    {3390, "Vice Break"}, {3600, "Kid Buu"}, {4040, "Hell Blade Rush"}, {4260, "Evil Blast"},
    {4480, "Majin Buu's Restoration"}, {4700, "Vanishing Ball"}, {4975, "Loyalty to Babidi"}, {5085, "Vanishing Drop"},
    {5360, "Babidi's Guard"}, {5580, "Assault Rain"}, {5745, "Innocence Strike"}, {6185, "Angry Explosion"},
    {6405, "Kamehameha"}, {6790, "Dabura Cookie"}, {6955, "Crazy Smash"}, {7175, "Human Candy"},
    {7505, "Toxic Chocolate"}, {7670, "Mad Kill Spike"}, {8000, "Babidi's Guard"}, {8110, "Savage Rush"},
    {8660, "Warp Kamehameha"}, {10000, "Absorption"}};
struct Premio {
  int id = 0, kili = 0;
  std::string nombre;
};

// ELF de B2 (SLES_* de la raiz de la ISO o de la carpeta extraida). ponytail: copia de LeerElf de pasarela_iw.cpp
// (frente R1); pasarla a pasarela.h cuando ese archivo quede libre.
uint32_t U32(const std::vector<uint8_t>& b, size_t o) {
  return uint32_t(b[o]) | uint32_t(b[o + 1]) << 8 | uint32_t(b[o + 2]) << 16 | uint32_t(b[o + 3]) << 24;
}
std::vector<uint8_t> Leer(std::ifstream& f, uint64_t off, uint32_t n) {
  std::vector<uint8_t> b(n);
  f.clear();
  f.seekg(std::streamoff(off));
  f.read(reinterpret_cast<char*>(b.data()), std::streamsize(n));
  b.resize(size_t(std::max<std::streamsize>(0, f.gcount())));
  return b;
}
std::vector<uint8_t> LeerElf(const std::filesystem::path& juego) {
  auto es_elf = [](std::string n) {
    for (auto& c : n) c = char(std::toupper(static_cast<unsigned char>(c)));
    return n.size() >= 11 && (n.rfind("SLUS_", 0) == 0 || n.rfind("SLES_", 0) == 0) && n[8] == '.';
  };
  std::error_code ec;
  if (std::filesystem::is_directory(juego, ec)) {
    for (const auto& e : std::filesystem::directory_iterator(juego, ec)) {
      if (!es_elf(e.path().filename().string())) continue;
      std::ifstream f(e.path(), std::ios::binary);
      return std::vector<uint8_t>(std::istreambuf_iterator<char>(f), {});
    }
    return {};
  }
  std::ifstream f(juego, std::ios::binary);
  const std::vector<uint8_t> pvd = Leer(f, 16 * 2048, 2048);
  if (pvd.size() < 2048 || std::memcmp(pvd.data() + 1, "CD001", 5) != 0) return {};
  const std::vector<uint8_t> raiz = Leer(f, uint64_t(U32(pvd, 156 + 2)) * 2048, std::min<uint32_t>(U32(pvd, 156 + 10), 1 << 20));
  for (size_t o = 0; o + 33 < raiz.size();) {
    const uint8_t len = raiz[o];
    if (len == 0) {
      o = (o / 2048 + 1) * 2048;
      continue;
    }
    const uint8_t nl = raiz[o + 32];
    if (o + 33 + nl <= raiz.size() && es_elf(std::string(reinterpret_cast<const char*>(&raiz[o + 33]), nl)))
      return Leer(f, uint64_t(U32(raiz, o + 2)) * 2048, std::min<uint32_t>(U32(raiz, o + 10), 64 << 20));
    o += len;
  }
  return {};
}
// Tabla de premios del ELF: se busca por sus dos primeras filas (486, 100), (180, 320).
std::vector<Premio> LeerPremios() {
  std::vector<Premio> out;
  const std::vector<uint8_t> elf = LeerElf(BuscarJuego("budokai 2"));
  const uint8_t firma[8] = {0xE6, 0x01, 100, 0, 0xB4, 0, 0x40, 0x01};
  const auto it = std::search(elf.begin(), elf.end(), std::begin(firma), std::end(firma));
  for (size_t o = size_t(it - elf.begin()); it != elf.end() && o + 4 <= elf.size() && out.size() < 64; o += 4) {
    Premio p;
    p.id = elf[o] | elf[o + 1] << 8;
    p.kili = elf[o + 2] | elf[o + 3] << 8;
    if (!p.kili) break;
    char n[48];
    std::snprintf(n, sizeof(n), "#%d", p.id);
    p.nombre = n;
    for (const Nombre& k : kNombres)
      if (k.kili == p.kili) p.nombre = k.nombre;
    out.push_back(p);
  }
  REXLOG_INFO("dbz3 nave: {} premios de Kili en el ELF ({} B){}", out.size(), elf.size(),
              out.empty() ? "" : (", el ultimo " + std::to_string(out.back().kili)).c_str());
  return out;
}
const std::vector<Premio>& Premios() {
  static const std::vector<Premio> p = LeerPremios();   // ponytail: se lee una vez por sesion
  return p;
}
// Luchadores de B2 que tiene B3 (IDs de B3, los de pasarela_b2.cpp B2aB3).
const int kLuchadores[] = {0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 19, 20, 21, 27, 28, 29, 30, 32,
                           33, 34, 35, 36, 37};
constexpr int kN = int(sizeof(kLuchadores) / sizeof(kLuchadores[0]));

// Pasos del original (textos de Babidi de DATA_<idioma> 1132): prueba -> tu luchador -> tu oponente ("Entonces,
// quien va a pelear? Ahora escoge a tu oponente", #9) -> el sitio ("Ahora, escoge el sitio donde quieras luchar",
// #12) -> combate -> resultado. El jugador elige rival y escenario (no hay tabla de rivales: lo dicen esos textos).
enum Estado { kPrueba, kJugador, kRival, kSitio, kSigue, kResultado };
int g_estado = kPrueba, g_prueba = 0, g_luch = 0, g_riv = 1, g_sitio = 0, g_kili = 0;
int g_vida = 0, g_vencidos = 0, g_ganado = 0, g_antes = 0, g_cuenta = 0;
std::vector<const Premio*> g_nuevos;   // premios de la ultima prueba
// "¡Nuevo!" solo si de verdad lo es: la primera vez que la barra pasa por su umbral (vuelta 0, progreso "nave_vueltas")
// y con capsula de B3 que dar (los personajes 486-489 y las 16 sin equivalente no dan nada nuevo).
std::vector<bool> g_de_verdad;
float g_auto_t = 0;

// Escenarios de B3 (entrada de data_cmn de su mapa) que se pueden elegir. ponytail: los de B2 no estan en B3.
struct Sitio {
  int mapa;
  const char *es, *en;
};
const Sitio kSitios[] = {{68, "Ciudad", "City"},          {64, "Zona rocosa", "Rocky area"}, {62, "Llanura", "Plains"},
                         {60, "Namek", "Namek"},          {50, "Juegos de Cell", "Cell Games"},
                         {56, "Isla", "Island"},          {58, "Sala del Tiempo", "Time Chamber"},
                         {46, "Torneo", "Tournament"},    {54, "Planeta Kaio", "Kaio's planet"}};
constexpr int kNSitios = int(sizeof(kSitios) / sizeof(kSitios[0]));

void Pedir(int out[6]) {
  combate::OndasCero();
  combate::OndasModo(g_prueba == 3);
  const int j = kLuchadores[g_luch], r = kLuchadores[g_riv];
  out[0] = j;
  out[1] = r;
  out[2] = kSitios[g_sitio].mapa;
  out[3] = g_prueba == 1 ? 1000 : g_prueba == 0 ? g_vida : 0;   // II: una barra (1000 por barra); I: la que quedo
  out[4] = kPruebas[g_prueba].tiempo;
  out[5] = g_prueba;
  REXLOG_INFO("dbz3 nave: prueba {}: {} contra {} en {} (vida {}, tiempo {})", g_prueba + 1, NombreDe(j), NombreDe(r),
              kSitios[g_sitio].es, out[3], out[4]);
}

// El Kili llena la barra de la nave (hasta 10000) y cada umbral que pasa da su premio; al pasar de 10000 la barra da la
// vuelta y los premios se vuelven a ganar (ELF 0x262DD0; el original solo da la vuelta si quedan capsulas por tener:
// ponytail, sin inventario de capsulas de B2 aqui, siempre la da).
// Capsula de B3 equivalente a cada premio de B2 (por nombre: modding resources discord/research/SkillList.dson y
// docs/07_ports/.../b3_caps.json; E:/DBZ3HD_trabajo/R2/capsulas/tabla.md). 0 = sin equivalente en B3: 186 Evil Tornado,
// 183 Hell Blow Shoot, 192 Merry Swing, 190 Enjoy Browning, 184 Hate Ray Cannon, 191 Innocence Dance, 198 Badness
// Throw, 185 Hate Dancing, 197 Vice Browning, 195 Vice Break, 219 Vanishing Drop, 196 Assault Rain, 189 Innocence
// Strike, 216 Crazy Smash, 217 Mad Kill Spike, 218 Savage Rush; y los personajes 486-489 (Dabura y los Bu, que B3 ya
// tiene). Probables (mismo juego de IDs): 398->395, 214->127, 383->343, 215->128.
int CapsulaB3(int id_b2) {
  static const int k[][2] = {{180, 130}, {179, 129}, {443, 421}, {380, 340}, {187, 120}, {440, 413}, {381, 341},
                             {194, 124}, {2, 2},     {182, 132}, {181, 131}, {398, 395}, {213, 126}, {458, 436},
                             {382, 342}, {188, 122}, {214, 127}, {416, 393}, {445, 423}, {426, 377}, {383, 343},
                             {215, 128}, {193, 123}};
  for (const auto& e : k)
    if (e[0] == id_b2) return e[1];
  return 0;
}
// Premio al inventario de capsulas de la partida de B3 (u8 por ID en 0x824BA110 + 0x2AFE9, docs/03_formatos/
// CAPSULAS_B3.md). ponytail: una copia mas, hasta 9; el juego la guarda en su siguiente guardado.
void DarCapsula(const Premio& p) {
  const int c = CapsulaB3(p.id);
  uint8_t* base = rex::Runtime::instance() ? rex::Runtime::instance()->virtual_membase() : nullptr;
  if (!c || !base) {
    REXLOG_INFO("dbz3 nave: premio {} sin capsula de B3", p.nombre);
    return;
  }
  constexpr dbz3::GA kPartida = 0x824BA110;   // US; la EU la traduce GA (eu_map)
  const auto a = kPartida + 0x2AFE9 + c;
  const uint8_t n = REX_LOAD_U8(a);
  if (n < 9) REX_STORE_U8(a, uint8_t(n + 1));
  REXLOG_INFO("dbz3 nave: premio {} -> capsula {} de B3 ({} -> {})", p.nombre, c, n, std::min(9, n + 1));
}

void Cobrar() {
  g_antes = g_kili;
  g_nuevos.clear();
  g_de_verdad.clear();
  int k = g_kili, resto = g_ganado, vueltas = LeerProgreso("nave_vueltas", 0);
  while (resto > 0) {
    if (k >= 10000) {
      k -= 10000;
      ++vueltas;
    }
    const int paso = std::min(resto, 10000 - k);
    for (const Premio& p : Premios())
      if (p.kili > k && p.kili <= k + paso) {
        g_nuevos.push_back(&p);
        g_de_verdad.push_back(vueltas == 0 && CapsulaB3(p.id) != 0);   // primera vuelta de la barra y algo que dar en B3
      }
    k += paso;
    resto -= paso;
  }
  g_kili = k;
  GuardarProgreso("nave_kili", g_kili);
  GuardarProgreso("nave_vueltas", vueltas);
  g_estado = kResultado;
  REXLOG_INFO("dbz3 nave: prueba {}: {} Kili (barra {} -> {})", g_prueba + 1, g_ganado, g_antes, g_kili);
  for (const Premio* p : g_nuevos) {
    REXLOG_INFO("dbz3 nave: premio: {} (ID {}, {} Kili)", p->nombre, p->id, p->kili);
    DarCapsula(*p);
  }
}

// ---- Graficos del original. El modo carga 3 archivos (tabla del ELF 0x378350, 0x2659F20 los pide de 3 en 3):
// DATA_<idioma> 1131 (35 piezas: marco de madera, titulo, Babidi, bocadillos, "Fase 1-4", flechas, la nave, panel del
// Kili, cifras, "ganadas/segs./veces/Kili/Hits", "Selecc. Person.", "Seleccion Fase"), DATA_CMN 905 (#AMA: la
// disposicion, 125 animaciones) y DATA_<idioma> 1132 (las 14 frases de Babidi). Cada animacion del #AMA (registro de
// 0x40 en +0x10: rectangulo local +0x00/+0x10, lista A de fotogramas de 0x20 B {tiempo, escala x, escala y, giro, x,
// y, z, alfa}, lista B de cuadros de 0x30 B {+4 pieza, +0x20 u0 v0 u1 v1 inclusivos}) se dibuja en su sitio de reposo
// (ultimo fotograma de A) con su primer cuadro, en la pantalla de 640x448 del original. Que animaciones salen en cada
// pantalla: las que ponen las funciones del modo (0x2621F0, 0x262640, 0x262830; ELF europeo).
struct Tex {
  std::unique_ptr<rex::ui::ImmediateTexture> t;
  float w = 0, h = 0;
};
struct Anim {
  float x0 = 0, y0 = 0, x1 = 0, y1 = 0, px = 0, py = 0, sx = 1, sy = 1;
  int pieza = -1;
  float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
  std::vector<std::array<float, 6>> claves;   // lista A: {cuadro, escala x, escala y, x, y, alfa}
};
float g_cuadro = 1e9f;   // cuadros (60 por segundo) desde que cambio la pantalla: entrada de cada pieza
std::vector<Tex> g_pz, g_frases;
std::vector<Anim> g_anim;
bool g_cargado = false;

std::vector<Anim> LeerAma(const std::vector<uint8_t>& d) {
  std::vector<Anim> out;
  auto u32 = [&](size_t o) -> uint32_t {
    return o + 4 <= d.size() ? uint32_t(d[o]) | uint32_t(d[o + 1]) << 8 | uint32_t(d[o + 2]) << 16 | uint32_t(d[o + 3]) << 24 : 0;
  };
  auto f32 = [&](size_t o) {
    const uint32_t u = u32(o);
    float f;
    std::memcpy(&f, &u, 4);
    return f;
  };
  if (d.size() < 16 || std::memcmp(d.data(), "#AMA", 4) != 0) return out;
  const uint32_t n = u32(8), base = u32(12);
  for (uint32_t i = 0; i < n && i < 1024 && base + 0x40 * (size_t(i) + 1) <= d.size(); ++i) {
    const size_t o = base + 0x40 * size_t(i);
    Anim a;
    a.x0 = f32(o);
    a.y0 = f32(o + 4);
    a.x1 = f32(o + 0x10);
    a.y1 = f32(o + 0x14);
    const uint32_t na = u32(o + 0x30), oa = u32(o + 0x34), nb = u32(o + 0x38), ob = u32(o + 0x3C);
    for (uint32_t c = 0; c < na && c < 64 && oa + 0x20 * (size_t(c) + 1) <= d.size(); ++c) {
      const size_t q = oa + 0x20 * size_t(c);
      a.claves.push_back({float(u32(q)), f32(q + 4), f32(q + 8), f32(q + 0x10), f32(q + 0x14), f32(q + 0x1C)});
    }
    if (na > 0 && oa + 0x20 * size_t(na) <= d.size()) {
      const size_t k = oa + 0x20 * (size_t(na) - 1);
      a.sx = f32(k + 4);
      a.sy = f32(k + 8);
      a.px = f32(k + 0x10);
      a.py = f32(k + 0x14);
    }
    if (nb > 0 && ob + 0x30 <= d.size()) {
      a.pieza = int(u32(ob + 4));
      a.u0 = f32(ob + 0x20);
      a.v0 = f32(ob + 0x24);
      a.u1 = f32(ob + 0x28) + 1;
      a.v1 = f32(ob + 0x2C) + 1;
    }
    out.push_back(a);
  }
  return out;
}

void Cargar() {
  if (g_cargado) return;
  g_cargado = true;
  auto* dr = dbz3::launcher::PreviewDrawer();
  const auto iso = BuscarJuego("budokai 2");
  AfsPs2 loc, cmn;
  const std::string lc = T::LangCode();
  if (!dr || iso.empty() || !cmn.Abrir(iso, "DATA_CMN.AFS") ||
      (!loc.Abrir(iso, lc == "es" ? "DATA_SP.AFS" : lc == "fr" ? "DATA_FR.AFS" : lc == "it" ? "DATA_IT.AFS"
                                     : lc == "de" ? "DATA_DU.AFS" : "DATA_UK.AFS") && !loc.Abrir(iso, "DATA_UK.AFS"))) {
    REXLOG_WARN("dbz3 nave: sin graficos de la nave (ISO {})", iso.string());
    return;
  }
  auto leer = [&](size_t k, std::vector<Tex>& out) {
    const std::vector<uint8_t> e = loc.Entrada(k);
    for (int j = 0; j < std::min(CuantasAmt(e), 64); ++j) {
      const Imagen im = TexturaAmt(e, j);
      Tex x;
      if (im) {
        x.t = dr->CreateTexture(uint32_t(im.w), uint32_t(im.h), rex::ui::ImmediateTextureFilter::kLinear, false,
                                im.rgba.data());
        x.w = float(im.w);
        x.h = float(im.h);
      }
      out.push_back(std::move(x));
    }
  };
  leer(1131, g_pz);
  leer(1132, g_frases);
  g_anim = LeerAma(cmn.Entrada(905));
  REXLOG_INFO("dbz3 nave: graficos de la ISO: {} piezas, {} frases, {} animaciones", g_pz.size(), g_frases.size(),
              g_anim.size());
}

// Pantalla del original (640x448) dentro del area: escala y origen.
struct Lienzo {
  float s;
  ImVec2 o;
  ImVec2 P(float x, float y) const { return ImVec2(o.x + x * s, o.y + y * s); }
};
Lienzo LienzoDe(const S::Area& a) {
  const float s = std::min(1200.0f / 640.0f, 900.0f / 448.0f) * a.s;
  return {s, ImVec2(a.p0.x + (1200.0f * a.s - 640.0f * s) * 0.5f, a.p0.y + (900.0f * a.s - 448.0f * s) * 0.5f)};
}
// Animacion k del #AMA en su sitio (o en (x, y) si se da), con su escala de reposo.
void Dib(ImDrawList* dl, const Lienzo& L, int k, ImU32 col = IM_COL32_WHITE, float x = -1, float y = -1) {
  if (k < 0 || k >= int(g_anim.size())) return;
  const Anim& a = g_anim[size_t(k)];
  if (a.pieza < 0 || a.pieza >= int(g_pz.size()) || !g_pz[size_t(a.pieza)].t) return;
  const Tex& p = g_pz[size_t(a.pieza)];
  float px = x >= 0 ? x : a.px, py = y >= 0 ? y : a.py, sx = a.sx, sy = a.sy;
  for (size_t i = 1; x < 0 && i < a.claves.size(); ++i) {   // entrada: claves de la lista A (lineal entre claves)
    const auto &k0 = a.claves[i - 1], &k1 = a.claves[i];
    if (g_cuadro >= k1[0]) continue;
    const float f = k1[0] > k0[0] ? std::clamp((g_cuadro - k0[0]) / (k1[0] - k0[0]), 0.0f, 1.0f) : 1.0f;
    sx = k0[1] + (k1[1] - k0[1]) * f;
    sy = k0[2] + (k1[2] - k0[2]) * f;
    px = k0[3] + (k1[3] - k0[3]) * f;
    py = k0[4] + (k1[4] - k0[4]) * f;
    break;
  }
  dl->AddImage(reinterpret_cast<ImTextureID>(p.t.get()), L.P(px + a.x0 * sx, py + a.y0 * sy),
               L.P(px + (a.x1 + 1) * sx, py + (a.y1 + 1) * sy), ImVec2(a.u0 / p.w, a.v0 / p.h),
               ImVec2(a.u1 / p.w, a.v1 / p.h), col);
}
// Numero con las cifras del #AMA: `primera` = animacion del 0 (las 10 seguidas); la ultima cifra en el sitio de la
// animacion del 0 y las demas a su izquierda (cifras de 40 px).
void Numero(ImDrawList* dl, const Lienzo& L, int primera, int n) {
  if (primera < 0 || primera + 9 >= int(g_anim.size())) return;
  const Anim& c0 = g_anim[size_t(primera)];
  const std::string s = std::to_string(std::max(0, n));
  for (size_t i = 0; i < s.size(); ++i)
    Dib(dl, L, primera + (s[i] - '0'), IM_COL32_WHITE, c0.px - 40.0f * c0.sx * float(s.size() - 1 - i), c0.py);
}

// Fondo: los lados de la ventana con la madera del marco del original (pieza 0) un poco oscurecida, como hacen las
// remasterizaciones con las pantallas 4:3; dentro, el morado, la nave (71), el marco (4-7) y la cinta con el titulo
// (13, 16).
void Fondo(ImDrawList* dl, const S::Area& a, const Lienzo& L) {
  const ImVec2 ds = ImGui::GetIO().DisplaySize;
  dl->AddRectFilled(ImVec2(0, 0), ds, IM_COL32(0, 0, 0, 255));
  if (!g_pz.empty() && g_pz[0].t && L.s > 0) {
    const float w = 40 * L.s, h = 312 * L.s, x0 = L.o.x, x1 = L.o.x + 640 * L.s;
    const ImTextureID id = reinterpret_cast<ImTextureID>(g_pz[0].t.get());
    for (float y = 0; y < ds.y; y += h) {
      for (float x = x0 - w; x > -w; x -= w) dl->AddImage(id, ImVec2(x, y), ImVec2(x + w, y + h), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(120, 120, 120, 255));
      for (float x = x1; x < ds.x; x += w) dl->AddImage(id, ImVec2(x, y), ImVec2(x + w, y + h), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(120, 120, 120, 255));
    }
  }
  (void)a;
  dl->AddRectFilled(L.P(0, 0), L.P(640, 448), IM_COL32(34, 22, 52, 255));
  Dib(dl, L, 71, IM_COL32(255, 255, 255, 90));
  for (int k = 4; k <= 7; ++k) Dib(dl, L, k);
  Dib(dl, L, 13);   // la cinta; el titulo (16) o la cabecera de cada pantalla (119, 122) van en el mismo sitio
  // aviso de que el modo aun se esta haciendo (en la franja de papel de debajo del marco, sin pisar nada)
  S::MenuText(dl, L.P(320, 430), 16 * L.s, T::T("EXPERIMENTAL: en construcción", "EXPERIMENTAL: work in progress"), true, false,
              420 * L.s);
}
// Babidi (37) a la derecha con su bocadillo (19) y la frase k de 1132 dentro, como en el original.
void Babidi(ImDrawList* dl, const Lienzo& L, int frase) {
  Dib(dl, L, 37);
  Dib(dl, L, 19);
  if (frase >= 0 && frase < int(g_frases.size()) && g_frases[size_t(frase)].t && 19 < int(g_anim.size())) {
    const Anim& b = g_anim[19];
    const Tex& f = g_frases[size_t(frase)];
    const float cx = b.px + (b.x0 + b.x1) * 0.5f - 6, cy = b.py + (b.y0 + b.y1) * 0.5f;
    const float esc = std::min(400.0f / f.w, 120.0f / f.h);
    dl->AddImage(reinterpret_cast<ImTextureID>(f.t.get()), L.P(cx - f.w * esc * 0.5f, cy - f.h * esc * 0.5f),
                 L.P(cx + f.w * esc * 0.5f, cy + f.h * esc * 0.5f), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(20, 20, 30, 255));
  }
}

}  // namespace

bool Disponible() { return b2::Disponible(); }
// Se abre con el deseo "Portal de la nave de Babidi" de Shenron en el Dragon World (pasarela_b2.cpp, como en B2).
bool Abierta() { return LeerProgreso("tablero_b2_nave", 0) != 0; }

void Entrar() {
  combate::OndasModo(false);
  g_kili = LeerProgreso("nave_kili", 0);
  Premios();
  Cargar();
  g_estado = kPrueba;
  REXLOG_INFO("dbz3 nave: entrar ({} Kili)", g_kili);
}

// 0 nada, 1 salir, 2 combate pedido en out = {jugador, rival, mapa, vida inicial, tiempo, prueba}.
int Dibujar(ImDrawList* dl, const S::Area& a, float t, uint16_t pressed, bool auto_on, int out[6]) {
  static int antes = -1;
  static float desde = 0;
  if (g_estado != antes) {   // pantalla nueva: las piezas entran con sus claves
    antes = g_estado;
    desde = t;
  }
  g_cuadro = (t - desde) * 60.0f;
  const Lienzo L = LienzoDe(a);
  Fondo(dl, a, L);
  if (auto_on && t - g_auto_t > 2.5f) {
    g_auto_t = t;
    pressed |= kA;
  }
  if (g_estado == kSigue) {   // prueba I: el siguiente enemigo sin pasar por el menu (el siguiente de la lista)
    g_riv = (g_riv + 1) % kN;
    if (g_riv == g_luch) g_riv = (g_riv + 1) % kN;
    Pedir(out);
    g_estado = kPrueba;
    return 2;
  }
  if (g_estado == kResultado) {   // panel (77) con la bola (80) y su aguja (81), franja (78); cifras y palabras del #AMA
    Dib(dl, L, 16);
    Dib(dl, L, 77);
    Dib(dl, L, 78);
    Dib(dl, L, 80);
    Dib(dl, L, 81);
    Numero(dl, L, 82, g_cuenta);
    Dib(dl, L, g_prueba == 0 ? 92 : g_prueba == 1 ? 93 : g_prueba == 2 ? 94 : 95);   // ganadas / segs. / Hits / veces
    Numero(dl, L, 96, g_ganado);
    Dib(dl, L, 106);   // Kili
    Numero(dl, L, 107, g_kili);
    Dib(dl, L, 117);   // Kili (total de la barra)
    float y = 352;
    for (size_t i = 0; i < g_nuevos.size(); ++i) {   // ponytail: los nombres con la letra de B3 (imagen sin localizar)
      const Premio* p = g_nuevos[i];
      if (y > 400) break;
      char l[96];
      if (i < g_de_verdad.size() && g_de_verdad[i])
        std::snprintf(l, sizeof(l), T::T("¡Nuevo! %s (%d Kili)", "New! %s (%d Kili)"), p->nombre.c_str(), p->kili);
      else
        std::snprintf(l, sizeof(l), "%s (%d Kili)", p->nombre.c_str(), p->kili);
      S::MenuText(dl, L.P(320, y), 15 * L.s, l, true, false);
      y += 22;
    }
    M::Help(dl, a, {{"A", T::T("Seguir", "Continue")}});
    if (pressed & (kA | kB | kStart)) g_estado = kPrueba;
    if (auto_on && (pressed & kA)) return 1;   // prueba: un combate y fuera
    return 0;
  }
  if (g_estado == kPrueba) {   // "Seleccion Fase" (122), "Fase 1-4" (46/50/54/58), el indicador (62 + 65..69) y Babidi
    if (pressed & kUp) g_prueba = (g_prueba + 3) % 4;
    if (pressed & kDown) g_prueba = (g_prueba + 1) % 4;
    if (auto_on) g_prueba = 2;
    Dib(dl, L, 122);
    Dib(dl, L, 46 + 4 * g_prueba);   // solo la elegida: las 4 van en el mismo sitio, cada una con su linea a su casilla
    Dib(dl, L, 62);
    Dib(dl, L, 66 + g_prueba);
    Babidi(dl, L, 1 + g_prueba);
    {   // lo que hay que hacer y lo que paga, con nuestra letra: Babidi lo dice con su frase, pero sin las cifras del Kili
      const char* d = T::T(kPruebas[g_prueba].ayuda_es, kPruebas[g_prueba].ayuda_en);
      const char* corte = std::strstr(d, ". ");
      const std::string l1 = corte ? std::string(d, corte + 1) : std::string(d), l2 = corte ? std::string(corte + 2) : std::string();
      S::MenuText(dl, L.P(198, 205), 17 * L.s, l1.c_str(), true, false, 322 * L.s);
      if (!l2.empty()) S::MenuText(dl, L.P(198, 225), 17 * L.s, l2.c_str(), true, false, 322 * L.s);
      if (g_prueba == 3)   // como se juega (B2: el rival solo devuelve; la guardia pulsada < 5 cuadros antes del impacto devuelve)
        S::MenuText(dl, L.P(198, 245), 15 * L.s, T::T("Dispara; devuelve la onda con la guardia justo al llegar.", "Fire; guard right as the wave arrives to send it back."),
                    false, false, 322 * L.s);
    }
    M::Help(dl, a, {{"A", T::T("Elegir", "Select")}, {"B", T::T("Salir", "Quit")}});
    if (pressed & kB) return 1;
    if (pressed & (kA | kStart)) g_estado = kJugador;
    return 0;
  }
  // luchador, oponente y sitio: "Selecc. Person." (119), el recuadro (25) con la cara de B3, 1P/2P (29/35) y las
  // flechas (40/43); el sitio, con su nombre
  int* cur = g_estado == kJugador ? &g_luch : g_estado == kRival ? &g_riv : &g_sitio;
  const int n = g_estado == kSitio ? kNSitios : kN;
  if (pressed & (kLeft | kUp)) *cur = (*cur + n - 1) % n;
  if (pressed & (kRight | kDown)) *cur = (*cur + 1) % n;
  Dib(dl, L, g_estado == kSitio ? 16 : 119);   // el sitio no tiene cabecera propia: el titulo de la nave
  Dib(dl, L, 25);
  if (g_estado == kSitio) {
    S::MenuText(dl, L.P(320, 172), 22 * L.s, T::T(kSitios[g_sitio].es, kSitios[g_sitio].en), true, false, 150 * L.s);
  } else {
    const int id = kLuchadores[*cur];
    if (ImTextureID tex = CaraDe(id)) dl->AddImage(tex, L.P(268, 150), L.P(372, 236));
    S::MenuText(dl, L.P(320, 252), 15 * L.s, NombreDe(id).c_str(), true, false, 150 * L.s);
    Dib(dl, L, g_estado == kJugador ? 29 : 35);   // 1P / 2P
  }
  Dib(dl, L, 40);
  Dib(dl, L, 43);
  Babidi(dl, L, g_estado == kJugador ? 0 : g_estado == kRival ? 9 : 12);
  M::Help(dl, a, {{"A", T::T("Elegir", "Select")}, {"B", T::T("Volver", "Back")}});
  if (pressed & kB) g_estado = g_estado == kJugador ? kPrueba : g_estado - 1;
  if (pressed & (kA | kStart)) {
    if (g_estado == kRival && g_riv == g_luch) g_riv = (g_riv + 1) % kN;
    if (g_estado != kSitio) {
      ++g_estado;
      return 0;
    }
    g_vida = 0;
    g_vencidos = 0;
    Pedir(out);
    g_estado = kPrueba;
    return 2;
  }
  return 0;
}

// Fin del combate de la prueba (lado 0 = jugador): segundos que sobraban (-1 sin limite) y golpes que quitaron vida.
void Resultado(int ganador, int vida, int segundos, int golpes) {
  const Prueba& p = kPruebas[g_prueba];
  combate::OndasModo(false);
  REXLOG_INFO("dbz3 nave: fin: gana el lado {}, vida {}, {} s de sobra, {} golpes, {} ondas devueltas", ganador + 1, vida,
              segundos, golpes, combate::OndasDevueltas(0));
  if (ganador < 0) {   // salio desde la pausa: sin Kili
    g_estado = kPrueba;
    return;
  }
  if (g_prueba == 0) {
    if (ganador == 0) {   // sigue con la vida que le queda
      ++g_vencidos;
      g_vida = std::max(1, vida);
      g_estado = kSigue;
      return;
    }
    g_cuenta = g_vencidos;
  } else if (g_prueba == 1) {
    g_cuenta = segundos >= 0 ? p.tiempo - segundos : 0;
  } else if (g_prueba == 2) {
    g_cuenta = golpes;
  } else {
    g_cuenta = combate::OndasDevueltas(0);   // IV: ondas de ki devueltas (combate.cpp, sub_82122488)
  }
  g_ganado = p.pago * g_cuenta;
  if (g_cuenta > 0) RangoMejora("rango_nave_" + std::to_string(g_prueba), 1, 25);   // escalafon: cada prueba, una vez
  Cobrar();
}

// Para el tablero de B2 (cápsula al azar de sus casillas): la de B3 equivalente a una de B2, 0 si no hay.
int CapsulaDeB2(int id_b2) { return CapsulaB3(id_b2); }

}  // namespace dbz3::pasarela::nave
