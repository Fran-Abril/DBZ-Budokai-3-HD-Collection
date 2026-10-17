// dbz3 - Dragon World de Budokai 2 (port con sus graficos originales; ver docs/14_frentes/PORT_EXACTO.md).
// El tablero de la Pasarela: pasarela_misiones.cpp solo llama a Entrar/Dibujar/Resultado (siempre con su g_mu).
// Todo se lee en marcha de la ISO de B2 del jugador (DATA_CMN.AFS con AfsPs2); nada de B2 va en la version.
//
// Fase k (0..8): tablero #YDW 387+k, mapa cenital 346+5k (piezas de 256x256 en filas de <ancho de textura>/256;
// la fase I son 5x3, la IX 5x5), fichas 342+5k (128x128 en parejas retrato + dorso con la Z; retrato = 2 * indice
// de la pieza; el impar, el dorso con la Z = peana), adornos 345+5k (sprites ya pintados, sus recortes en el #AMA
// 344+5k), interfaz 398 (comun: tramos verdes de vida, cifras, bolas, !!, ?!, radar, nubes de aturdido...), GAME OVER
// 404 #0; en el idioma del juego (DATA_UK/SP/FR/IT/DU.AFS, mismas entradas): 146+k sprites y textos de la fase
// ("Stage N", objetivo, Saibaman que asoma del agujero) y 155 (Goku de cuerpo entero, 64x88, el de la intro). Comprobado 2026-10-16 (P2) con E:/DBZ3HD_trabajo/P2/ (scan*.py, png/): la cabecera del #YDW da el tamano
// de la textura del mapa y cuadra con el numero de piezas de cada 346+5k, y las fichas de 342 son los 13 companeros
// en el orden de las piezas del #YDW + Raditz (indice 13) y Nappa (14).
// Formato #YDW: +0x0C ancho y alto visibles del mapa, +0x14 ancho y alto de su textura, +0x20 8 pares (cuantos, donde):
//   T1 casillas de 52 B: x, y (float, px del mapa), 4 vecinas (u16, 0 = ninguna), +0x10 tipo (2 Dende cura, 3 alfombra de
//      Mr. Popo que lleva a la casilla +0x14, 4 sitio al que lleva), +0x19 condicion del combate en esa casilla (1 empiezan
//      con media vida, 2 con 0 de ki, 3 la vida se va gastando, 4 el ki se va gastando, 5 sin bloqueo), +0x1C 0x0A =
//      casilla de combate (boton con el kanji), +0x1F escenario de B2; la 0 nula. Comprobado con los 9 mapas de la guia
//      de PAndrews (GameFAQs, "Maps and Charts"): mismas casillas, caminos, objetos, Dende, Popo y condiciones.
//   T2 piezas de 40 B: +0 personaje (ID de B2: los 25 primeros = orden de B1), +1 indice de ficha, +2 bando (0 companero
//      elegible, 4 del jugador, bit 2 rival), +6 barras, +0xB barras maximas, +0xC vida, +0x14 casilla (u16).
//   T7 adornos: punteros a {objeto (registro del #AMA 344+5k), tipo (2 fijo, 0 animado, 4 capa de fondo), x, y}.
//   T8 objetos: punteros a {u16 casilla, u8 enterrado, u8 a la vista, u8 tipo, u8 valor, u16 ?, i32 cuantas banderas,
//      banderas (como las de T4)}. ELF 0x27CD40: lo no enterrado sale al suelo si esta "a la vista" o si se cumplen sus
//      banderas (las capsulas de las fases III, VI y VIII); 0x27C090: cada busqueda saca el primer enterrado de la casilla
//      que aun queda y cumple sus banderas (la fase IX apila 5 o 6 por casilla: uno por busqueda).
//      Tipos (comprobados con los mapas de la guia): 0 sitio posible de Bola (ELF 0x27BD80: el juego pone
//      min(fase, 6) - Bolas que tienes + 1 Bolas, como mucho 3, en sitios al azar, cada una con un numero de Bola que aun
//      no tienes; "a la vista" = sobre el suelo, como la de la fase I), 1 daga (+valor % de ataque), 3 armadura (+valor % de defensa), 5 cebolla (mueve
//      2 casillas por turno), 7 capsula al azar, 8 zenie (valor x 100), 9 Radar del Dragon (a la vista), 6 Mr. Popo.
// #AMA (sprites): +8 cuantos, +0xC donde; registros de 0x40: rect x0,y0 (+0) x1,y1 (+0x10) respecto al ancla, +0x38/+0x3C
//   cuadros (0x30 B: +4 textura, +0x20 u0 v0 u1 v1 en px, inclusivos).
// Reglas (FAQ de GameFAQs, E:/DBZ3HD_trabajo/estudio_ports/b2_reglas.md): en tu turno cada personaje mueve 1 casilla
// (la cebolla da +1), buscar gasta el movimiento (una vez por casilla), tocar a un rival = combate 1 contra 1 (el que se
// movio contra el rival), ganar le quita una barra, la vida se conserva entre combates, Saibaman y Dr. Gero no se mueven.
#include "pasarela.h"
#include "ps2_modelo.h"

#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/ui/immediate_drawer.h>

#include <imgui.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <array>

#include "generated/dbz3_init.h"
#include "eu_map.h"
#include "ingame/game_style.h"
#include "launcher/i18n.h"
#include "launcher/launcher_state.h"

namespace dbz3::rpg {
void Premio(int zeni);   // Zenie al monedero del HUB (modos.h)
}
namespace dbz3::pasarela::nave {
int CapsulaDeB2(int id_b2);   // pasarela_nave.cpp
}
namespace dbz3::pasarela::b2 {
namespace {

namespace T = dbz3::i18n;
namespace S = dbz3::ingame::style;

constexpr uint16_t kUp = 0x0001, kDown = 0x0002, kLeft = 0x0004, kRight = 0x0008, kStart = 0x0010, kLB = 0x0100,
                   kRB = 0x0200, kA = 0x1000, kB = 0x2000, kX = 0x4000, kY = 0x8000;
constexpr int kFases = 9;
constexpr int kGoku = 0, kSaibaman = 22, kGero = 25;   // IDs de B2

struct Texto {
  const char *es, *en;
  const char* operator()() const { return T::T(es, en); }
};
const Texto kSagas[kFases] = {{"Fase I: saga Saiyan", "Stage I: Saiyan saga"},
                              {"Fase II: Freezer en la Tierra", "Stage II: Frieza on Earth"},
                              {"Fase III: Namek", "Stage III: Namek"},
                              {"Fase IV: los androides y Cell", "Stage IV: the androids and Cell"},
                              {"Fase V: la resurrección de Bu", "Stage V: Buu's revival"},
                              {"Fase VI: Babidi y Majin Bu", "Stage VI: Babidi and Majin Buu"},
                              {"Fase VII: la sala del tiempo", "Stage VII: the time chamber"},
                              {"Fase VIII: la ciudad", "Stage VIII: the city"},
                              {"Fase IX: Kid Bu", "Stage IX: Kid Buu"}};
// Escenario de B3 (entrada de data_cmn de su mapa) para los combates de cada fase.
const int kMapaB3[kFases] = {64, 62, 60, 50, 64, 68, 58, 68, 54};

// ID de B3 de un personaje de B2: los 25 primeros siguen el orden de B1; los nuevos, por sus fichas (342+5k).
int B2aB3(int c) {
  switch (c) {
    case 25: return 32;   // Dr. Gero
    case 26: return 6;    // Goten
    case 27: return 9;    // Trunks nino
    case 28: return 4;    // Gohan
    case 29: return 34;   // Majin Bu
    case 30: return 35;   // Super Bu
    case 31: return 36;   // Kid Bu
    case 32: return 37;   // Dabura
    case 33: return 15;   // Videl
    case 35: return 16;   // Kaio-shin
    case 47: case 48: case 50: case 51: return 35;   // ponytail: formas de Super Bu de las fases VII-VIII, sin separar
  }
  return c >= 0 && c < 25 ? B1aB3(c) : -1;
}

struct Casilla {
  float x = 0, y = 0;
  int vec[4] = {};
  int tipo = 0, destino = 0, condicion = 0;   // ver T1 arriba
  bool lucha = false;
};
struct Objeto {
  int casilla = 0, tipo = 0, valor = 0;
  bool visible = false, enterrado = false, hecho = false;
  std::vector<int> banderas;   // las que pide (T8 +8); vacia = ninguna
};
struct Ficha {
  int c = 0, idx = 0, bando = 0, barras = 1, barras_max = 1, vida_max = 3000, casilla = 0;
  int vida = 0;                   // la que le queda (0 = llena)
  int movs = 0, movs_turno = 1;
  bool fuera = false, jugador = false, rival = false, oculta = false;
  int aturdido = 0;               // turnos rivales que no se mueve (nube con remolino); vencerlo asi quita 2 barras
  int ataque = 0, defensa = 0;    // % de la daga y la armadura recogidas (valen hasta acabar la fase)
  int n = 0;                      // indice de la pieza en el #YDW (el que usan los scripts)
  int mov = -1, mov_p = 0;        // tipo de movimiento (T2 +3/+4, o el que pone la orden 50) y su dato
  int desde = 0;
  float t_mov = -10;              // animacion del ultimo paso
};
struct Datos {
  int fase = -1;
  float ancho = 0, alto = 0;
  int cols = 1;
  std::vector<Casilla> casillas;  // [0] nula
  std::vector<Objeto> objetos;
  std::vector<Ficha> piezas;
  // Scripts de la fase (#YDW T3 y T6, interprete del ELF 0x295F20): T3 = disparadores {pieza, otra, turno, script, ?}
  // (-1, 0, N: al empezar el turno N); T6 = scripts: u32 cuantas + ordenes de 24 B {espera, op, 4 datos}.
  std::vector<std::array<int, 5>> t3;
  // T4 (ELF 0x27F510): contactos {pieza A (0 = cualquiera), pieza B, casilla (0 = cualquiera), script, ?, ?, cuantas banderas,
  // banderas...}: vale si los personajes de A y B son los de las dos fichas que se tocan; numero > 0 = encendida,
  // < 0 = apagada (bitfield del ELF 0x699260). T5 (0x27F3A0): {fila de T4 del ultimo contacto, suceso (5 = cualquiera),
  // script, cuantas banderas, banderas...}: lo que pasa tras ese combate.
  std::vector<std::vector<int>> t4, t5;
  std::vector<std::vector<std::array<int, 6>>> guiones;
  std::vector<Imagen> mapa, fichas, iconos, adornos, loc;
  std::vector<Imagen> shen_frases, shen_marco;   // Shenron: frases (DATA_<idioma> 537) y ventana (CMN 850)
  std::vector<Imagen> deseos;   // deseos de Shenron (DATA_<idioma> 540, 539, 538: los carga dw_shenglong.c, ELF 0x37A600)
  Imagen game_over, goku, hoyo, cabezas;   // Goku de cuerpo entero (155), agujero y cabezas de Saibaman (146 #13/#16)
  struct Sprite {
    int tex = -1;
    float u0 = 0, v0 = 0, u1 = 0, v1 = 0, x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    float px = 0, py = 0;   // sitio de reposo (ultimo fotograma de la lista A del #AMA, pantalla de 640x448)
    float ex = 1, ey = 1;   // su escala (la caja del menu va a 0,95 x 0,8)
    std::vector<std::array<float, 7>> claves;   // lista A: {cuadro, x, y, alfa, escala x, escala y, giro en grados}
  };
  std::vector<Sprite> sprites;             // #AMA 344+5k: primer cuadro de cada objeto
  std::vector<Sprite> textos;              // #AMA 343+5k: ventanas de mensaje de la fase (texturas de 146+k)
  std::vector<Sprite> shen_ama;            // Shenron: disposicion de su ventana (CMN 849)
  // Marcador y ventanas del tablero (ELF 0x2350C0: fijos CMN 402 = #AMA y DATA_<idioma> 188 = sus texturas, los
  // uRam00698dc8/db0 de las tareas D_TURN, D_RADAR, D_RADARBASE, D_CLEAR, D_DBGET, DBALL y del menu DWU_MMAIN/DWU_YN*):
  // 0-2 barra de madera, 3-5 "MUNDO DRAGON", 6-15 Bolas, 16-24 radar, 38-50 rombo del turno y cifras, 51-74 ventana de
  // mensaje (8 piezas: entrar 0x33+3k, reposo +1, salir +2), 87-89 flecha, punto y brillo del radar, 97-120 menu y Si/No.
  std::vector<Imagen> hud;
  std::vector<Sprite> hud_ama;
  struct Adorno {
    int obj = 0;
    float x = 0, y = 0;
  };
  std::vector<Adorno> colocados;           // T7 (tipos 0 y 2)
  int stage = -1, objetivo = -1;           // en loc: "Stage N" (240x72) y el objetivo (primer 560x128)
};

enum Estado : int { kPrepara, kJuego, kGanada, kPerdida, kDeseo };

Datos g_d;
int g_estado = kPrepara, g_comp = 0, g_sel = 0, g_dest = -1, g_turno = 1, g_bolas = 0, g_zenie = 0, g_jefe = -1;
std::vector<std::array<int, 2>> g_bolas_fase;   // Bolas puestas en esta fase: {casilla, numero de Bola 0..6}
int g_bolas_ini = 0;                           // las que tenias al empezar la fase
std::vector<Ficha> g_f;
std::vector<Objeto> g_obj;
std::vector<bool> g_buscada;
std::vector<int> g_cand;            // piezas elegibles como companero
std::string g_msg;
int g_msg_icono = -1;
float g_msg_t = -10, g_ahora = 0;
// Ventanas de mensaje de los scripts (orden 38 {a, b}: las animaciones a y b del #AMA 343+5k con las texturas de
// 146+k, como la ventana del ELF 0x26F280/0x26EFC0). Las que reposan fuera de la pantalla son las de salida: no se
// ponen. Se quitan con A o a los 3 s (ponytail: el original espera a su ventana; el tiempo es medido a ojo).
std::vector<std::array<int, 2>> g_mensajes;
// Banderas de la fase (ELF 0x699260; ordenes 1 encender, 2 apagar, 3 cambiar) que piden los disparadores T4/T5.
std::vector<uint32_t> g_banderas(64, 0);
bool Bandera(int n) { return n >= 0 && n < 2048 && (g_banderas[size_t(n) >> 5] >> (n & 31) & 1u); }
bool Condiciones(const std::vector<int>& r, size_t desde) {   // todas las banderas de r[desde..] como se piden
  for (size_t k = desde; k < r.size(); ++k)
    if (r[k] > 0 ? !Bandera(r[k]) : Bandera(-r[k])) return false;
  return true;
}
// Objeto sobre el suelo (ELF 0x27CD40) o enterrado que aun puede salir al buscar (0x27C090).
bool EnSuelo(const Objeto& o) { return !o.hecho && !o.enterrado && (o.visible || Condiciones(o.banderas, 0)); }
bool Escondido(const Objeto& o) { return !o.hecho && o.enterrado && Condiciones(o.banderas, 0); }
int BolaEn(int casilla) {
  for (size_t i = 0; i < g_bolas_fase.size(); ++i)
    if (g_bolas_fase[i][0] == casilla) return int(i);
  return -1;
}
float g_mensaje_t = -10;
int g_pend_j = -1, g_pend_r = -1;   // combate pendiente (fichas de g_f)
float g_pend_t = 0;
bool g_luchando = false;
// Radar del Dragon recogido (bit 1 de 0x6992DC: D_RADAR solo dibuja con el), marcador de abajo escondido con L1 (bit 8,
// 0x2700E0) y menu del tablero (triangulo/Start, ELF 0x2A35F0: "Salir" y "Reiniciar Mundo Dragon", cada uno con su Si/No
// de 0x2A2C90; 0 cerrado, 1 menu, 2 Si/No).
bool g_radar = false, g_hud_oculto = false;
// Cuando entro o salio cada parte del marcador (entradas y salidas animadas de DibHud) y cuando se dibujo por ultima vez
float g_hud_t = -10, g_radar_t = -10, g_rombo_t = -10, g_hud_visto = -10;
bool g_radar_visto = false;
float g_bola_t = -10;   // D_DBGET: la Bola recien cogida (g_bola_n, 0..6) entra en la barra
int g_bola_n = -1;
int g_menu = 0, g_menu_sel = 0, g_yn_sel = 1;
float g_menu_t = 0;
int g_marca = -1, g_marca_icono = 0;   // "?!" o "!!" sobre una casilla
float g_marca_t = -10;
float g_cam_x = 0, g_cam_y = 0, g_auto_t = 0;

void DarCapsulaB3(int c);   // abajo (inventario de capsulas de la partida)
void Aviso(const std::string& m, int icono = -1) {
  g_msg = m;
  g_msg_icono = icono;
  g_msg_t = g_ahora;
  REXLOG_INFO("dbz3 tablero b2: {}", m);
}
std::string Nombre(const Ficha& f) {
  const int id = B2aB3(f.c);
  return id >= 0 ? NombreDe(id) : "#" + std::to_string(f.c);
}

// ---- Lectura de la ISO ----
bool Cargar(int fase) {
  if (g_d.fase == fase) return true;
  const auto iso = BuscarJuego("budokai 2");
  AfsPs2 cmn;
  if (iso.empty() || !cmn.Abrir(iso, "DATA_CMN.AFS")) {
    REXLOG_WARN("dbz3 tablero b2: sin ISO de Budokai 2 ({})", iso.string());
    return false;
  }
  const std::vector<uint8_t> y = cmn.Entrada(387 + size_t(fase));
  auto u32 = [&](size_t o) -> uint32_t {
    return o + 4 <= y.size() ? y[o] | y[o + 1] << 8 | y[o + 2] << 16 | uint32_t(y[o + 3]) << 24 : 0;
  };
  auto u16 = [&](size_t o) -> int { return o + 2 <= y.size() ? y[o] | y[o + 1] << 8 : 0; };
  auto f32 = [&](size_t o) {
    float v = 0;
    if (o + 4 <= y.size()) std::memcpy(&v, &y[o], 4);
    return v;
  };
  const uint32_t n1 = u32(0x20), o1 = u32(0x24), n2 = u32(0x28), o2 = u32(0x2C), n8 = u32(0x58), o8 = u32(0x5C);
  if (y.size() < 0x70 || std::memcmp(y.data(), "#YDW", 4) != 0 || n1 < 3 || n1 > 128 || n2 > 128 ||
      size_t(o1) + 52 * n1 > y.size() || size_t(o2) + 40 * n2 > y.size() || size_t(o8) + 4 * n8 > y.size()) {
    REXLOG_WARN("dbz3 tablero b2: #YDW {} no valido", 387 + fase);
    return false;
  }
  Datos d;
  d.fase = fase;
  d.ancho = float(u32(0x0C));
  d.alto = float(u32(0x10));
  d.cols = std::max(1, int(u32(0x14) / 256));
  for (uint32_t k = 0; k < n1; ++k) {
    const size_t o = o1 + 52 * size_t(k);
    Casilla c;
    c.x = f32(o);
    c.y = f32(o + 4);
    for (int j = 0; j < 4; ++j) c.vec[j] = u16(o + 8 + 2 * j) < int(n1) ? u16(o + 8 + 2 * j) : 0;
    c.tipo = int(u32(o + 0x10));
    c.destino = int(u32(o + 0x14)) < int(n1) ? int(u32(o + 0x14)) : 0;
    c.condicion = y[o + 0x19];
    c.lucha = y[o + 0x1C] == 0x0A;
    d.casillas.push_back(c);
  }
  for (uint32_t p = 1; p < n2; ++p) {
    const size_t o = o2 + 40 * size_t(p);
    Ficha f;
    f.c = y[o];
    f.idx = y[o + 1];
    f.bando = y[o + 2];
    f.barras_max = std::max(1, int(y[o + 0xB]));
    f.barras = std::clamp(int(y[o + 6]), 1, f.barras_max);
    f.vida_max = u16(o + 0xC) > 0 ? u16(o + 0xC) : 3000;
    f.casilla = u16(o + 0x14);
    f.n = int(p);
    // +3/+4: tipo de movimiento inicial y su dato (ELF 0x2791A0 los copia a +0x1C/+0x1D, los mismos que cambia la orden 50)
    f.mov = y[o + 3] <= 8 ? y[o + 3] : 0;
    f.mov_p = y[o + 4];
    if (f.casilla <= 0 || f.casilla >= int(n1)) continue;
    f.desde = f.casilla;
    d.piezas.push_back(f);
  }
  for (uint32_t i = 0; i < n8; ++i) {
    const size_t p = u32(o8 + 4 * size_t(i));
    if (p + 12 > y.size()) continue;
    Objeto ob;
    ob.casilla = u16(p);
    ob.enterrado = y[p + 2] != 0;
    ob.visible = y[p + 3] != 0;
    ob.tipo = y[p + 4];
    ob.valor = y[p + 5];
    for (uint32_t b = 0; b < u32(p + 8) && b < 16 && p + 16 + 4 * size_t(b) <= y.size(); ++b)
      ob.banderas.push_back(int32_t(u32(p + 12 + 4 * size_t(b))));
    if (ob.casilla <= 0 || ob.casilla >= int(n1)) continue;
    d.objetos.push_back(ob);
  }
  {   // scripts (T3 disparadores y T6 ordenes)
    const uint32_t n3 = u32(0x30), o3 = u32(0x34), n6 = u32(0x48), o6 = u32(0x4C);
    for (uint32_t i = 0; i < n3 && i < 4096; ++i) {
      const size_t q = u32(o3 + 4 * size_t(i));
      std::array<int, 5> r{};
      for (int k = 0; k < 5 && q + 20 <= y.size(); ++k) r[size_t(k)] = int32_t(u32(q + 4 * size_t(k)));
      d.t3.push_back(r);
    }
    const uint32_t n4 = u32(0x38), o4 = u32(0x3C);
    for (uint32_t i = 0; i < n4 && i < 4096; ++i) {
      const size_t q = u32(o4 + 4 * size_t(i));
      if (q + 28 > y.size()) continue;
      std::vector<int> r;
      for (int k = 0; k < 7; ++k) r.push_back(int32_t(u32(q + 4 * size_t(k))));
      for (int k = 0; k < r[6] && k < 16 && q + 28 + 4 * size_t(k) + 4 <= y.size(); ++k)
        r.push_back(int32_t(u32(q + 28 + 4 * size_t(k))));
      d.t4.push_back(std::move(r));
    }
    const uint32_t n5 = u32(0x40), o5 = u32(0x44);
    for (uint32_t i = 0; i < n5 && i < 4096; ++i) {
      const size_t q = u32(o5 + 4 * size_t(i));
      if (q + 16 > y.size()) continue;
      std::vector<int> r;
      for (int k = 0; k < 4; ++k) r.push_back(int32_t(u32(q + 4 * size_t(k))));
      for (int k = 0; k < r[3] && k < 16 && q + 16 + 4 * size_t(k) + 4 <= y.size(); ++k)
        r.push_back(int32_t(u32(q + 16 + 4 * size_t(k))));
      d.t5.push_back(std::move(r));
    }
    for (uint32_t i = 0; i < n6 && i < 4096; ++i) {
      const size_t q = u32(o6 + 4 * size_t(i));
      std::vector<std::array<int, 6>> g;
      const uint32_t cuantas = q + 4 <= y.size() ? u32(q) : 0;
      for (uint32_t c = 0; c < cuantas && c < 256 && q + 4 + 24 * (size_t(c) + 1) <= y.size(); ++c) {
        std::array<int, 6> o{};
        for (int k = 0; k < 6; ++k) o[size_t(k)] = int32_t(u32(q + 4 + 24 * size_t(c) + 4 * size_t(k)));
        g.push_back(o);
      }
      d.guiones.push_back(std::move(g));
    }
  }
  const uint32_t n7 = u32(0x50), o7 = u32(0x54);
  for (uint32_t i = 0; i < n7 && size_t(o7) + 4 * (size_t(i) + 1) <= y.size(); ++i) {
    const size_t p = u32(o7 + 4 * size_t(i));
    if (p + 16 > y.size() || (u32(p + 4) != 0 && u32(p + 4) != 2)) continue;   // 4 = capa de fondo: sin usar
    d.colocados.push_back({int(u32(p)), f32(p + 8), f32(p + 12)});
  }
  auto amt = [&](AfsPs2& afs, size_t k, std::vector<Imagen>& out) {
    const std::vector<uint8_t> e = afs.Entrada(k);
    for (int j = 0; j < std::min(CuantasAmt(e), 64); ++j) out.push_back(TexturaAmt(e, j));
  };
  amt(cmn, 346 + 5 * size_t(fase), d.mapa);
  amt(cmn, 342 + 5 * size_t(fase), d.fichas);
  amt(cmn, 345 + 5 * size_t(fase), d.adornos);
  // #AMA: 344+5k recortes de los adornos; 343+5k ventanas de mensaje de la fase (orden 38 de los scripts)
  auto leer_ama = [&](size_t k, std::vector<Datos::Sprite>& out) {
    const std::vector<uint8_t> am = cmn.Entrada(k);
    auto a32 = [&](size_t o) -> uint32_t {
      return o + 4 <= am.size() ? am[o] | am[o + 1] << 8 | am[o + 2] << 16 | uint32_t(am[o + 3]) << 24 : 0;
    };
    auto af = [&](size_t o) {
      float v = 0;
      if (o + 4 <= am.size()) std::memcpy(&v, &am[o], 4);
      return v;
    };
    if (am.size() < 0x10 || std::memcmp(am.data(), "#AMA", 4) != 0) return;
    for (uint32_t i = 0; i < a32(8) && size_t(a32(12)) + 0x40 * (size_t(i) + 1) <= am.size(); ++i) {
      const size_t o = a32(12) + 0x40 * size_t(i), q = a32(o + 0x3C);
      Datos::Sprite sp{-1, 0, 0, 0, 0, af(o), af(o + 4), af(o + 0x10) + 1, af(o + 0x14) + 1};
      if (a32(o + 0x38) > 0 && q + 0x30 <= am.size()) {
        sp.tex = int(a32(q + 4));
        sp.u0 = af(q + 0x20);
        sp.v0 = af(q + 0x24);
        sp.u1 = af(q + 0x28) + 1;
        sp.v1 = af(q + 0x2C) + 1;
      }
      const uint32_t na = a32(o + 0x30), oa = a32(o + 0x34);
      if (na > 0 && na < 256 && oa + 0x20 * size_t(na) <= am.size()) {
        sp.px = af(oa + 0x20 * (size_t(na) - 1) + 0x10);
        sp.py = af(oa + 0x20 * (size_t(na) - 1) + 0x14);
        sp.ex = af(oa + 0x20 * (size_t(na) - 1) + 4);
        sp.ey = af(oa + 0x20 * (size_t(na) - 1) + 8);
        for (uint32_t c = 0; c < na; ++c) {
          const size_t q2 = oa + 0x20 * size_t(c);
          sp.claves.push_back({float(a32(q2)), af(q2 + 0x10), af(q2 + 0x14), af(q2 + 0x1C), af(q2 + 4), af(q2 + 8), af(q2 + 0xC)});
        }
      }
      out.push_back(sp);
    }
  };
  leer_ama(344 + 5 * size_t(fase), d.sprites);
  leer_ama(343 + 5 * size_t(fase), d.textos);
  const std::string lc = T::LangCode();
  AfsPs2 loc;
  if (!loc.Abrir(iso, lc == "es" ? "DATA_SP.AFS" : lc == "fr" ? "DATA_FR.AFS" : lc == "it" ? "DATA_IT.AFS"
                                   : lc == "de" ? "DATA_DU.AFS" : "DATA_UK.AFS"))
    loc.Abrir(iso, "DATA_UK.AFS");
  amt(loc, 146 + size_t(fase), d.loc);
  // Deseos de Shenron, en el orden de sus archivos: 540 (#0 "Portal de la nave de Babidi", trajes de Bulma), 539
  // (fusion y Pothala) y 538 ("Avance" de cada personaje).
  amt(loc, 540, d.deseos);
  amt(loc, 539, d.deseos);
  amt(loc, 538, d.deseos);
  amt(loc, 537, d.shen_frases);   // "Te concedere un deseo.", "Si, eso es aceptable.", ..., "Ahora debo irme..."
  amt(cmn, 850, d.shen_marco);
  leer_ama(849, d.shen_ama);
  for (size_t j = 0; j < d.loc.size(); ++j) {
    if (d.loc[j].w == 240 && d.loc[j].h == 72) d.stage = int(j);
    if (d.loc[j].w == 560 && d.loc[j].h == 128 && d.objetivo < 0) d.objetivo = int(j);
  }
  if (g_d.iconos.empty()) {
    amt(loc, 188, d.hud);
    leer_ama(402, d.hud_ama);
    amt(cmn, 398, d.iconos);
    std::vector<Imagen> v;
    amt(cmn, 404, v);
    if (!v.empty()) d.game_over = std::move(v[0]);
    v.clear();
    amt(loc, 146, v);   // el agujero y las cabezas verdes de la fase I valen para todas
    if (v.size() > 16 && v[13].w == 96 && v[13].h == 48 && v[16].w == 192 && v[16].h == 64) {
      d.hoyo = std::move(v[13]);
      d.cabezas = std::move(v[16]);
    }
    v.clear();
    amt(loc, 155, v);   // ponytail: se decodifica la entrada entera (una vez) para quedarse con el Goku de 64x88
    for (Imagen& im : v) {
      if (im.w == 64 && im.h == 88) d.goku = std::move(im);
    }
  } else {
    d.iconos = std::move(g_d.iconos);
    d.hud = std::move(g_d.hud);
    d.hud_ama = std::move(g_d.hud_ama);
    d.game_over = std::move(g_d.game_over);
    d.goku = std::move(g_d.goku);
    d.hoyo = std::move(g_d.hoyo);
    d.cabezas = std::move(g_d.cabezas);
  }
  REXLOG_INFO("dbz3 tablero b2: fase {} leida de {}: mapa {}x{} ({} piezas en {} columnas), {} casillas, {} piezas, "
              "{} objetos, {} fichas, {} iconos, {} adornos ({} sprites), {} del idioma {}, Goku {}", fase + 1,
              iso.filename().string(), d.ancho, d.alto, d.mapa.size(), d.cols, d.casillas.size() - 1, d.piezas.size(),
              d.objetos.size(), d.fichas.size(), d.iconos.size(), d.colocados.size(), d.sprites.size(), d.loc.size(), lc,
              d.goku ? "si" : "no");
  if (d.mapa.empty() || d.iconos.size() < 30) {
    if (g_d.iconos.empty()) {   // devuelve lo prestado a la fase en curso
      g_d.iconos = std::move(d.iconos);
      g_d.hud = std::move(d.hud);
      g_d.hud_ama = std::move(d.hud_ama);
      g_d.game_over = std::move(d.game_over);
      g_d.goku = std::move(d.goku);
      g_d.hoyo = std::move(d.hoyo);
      g_d.cabezas = std::move(d.cabezas);
    }
    return false;
  }
  g_d = std::move(d);
  return true;
}

// ---- Reglas ----
bool Activa(const Ficha& f) { return !f.fuera && !f.oculta; }
int FichaEn(int casilla, bool rival) {
  for (size_t i = 0; i < g_f.size(); ++i) {
    if (Activa(g_f[i]) && g_f[i].casilla == casilla && (rival ? g_f[i].rival : g_f[i].jugador)) return int(i);
  }
  return -1;
}
bool QuedanMovs() {
  for (const Ficha& f : g_f) {
    if (Activa(f) && f.jugador && f.movs > 0) return true;
  }
  return false;
}
void SiguienteSel() {
  for (size_t k = 1; k <= g_f.size(); ++k) {
    const size_t i = (size_t(g_sel) + k) % g_f.size();
    if (Activa(g_f[i]) && g_f[i].jugador && g_f[i].movs > 0) {
      g_sel = int(i);
      g_dest = -1;
      return;
    }
  }
}

void Preparar() {
  g_estado = kPrepara;
  g_cand.clear();
  for (size_t i = 0; i < g_d.piezas.size(); ++i) {
    if (g_d.piezas[i].bando == 0 && g_d.piezas[i].c != kSaibaman && B2aB3(g_d.piezas[i].c) >= 0) g_cand.push_back(int(i));
  }
  g_comp = g_cand.empty() ? 0 : std::clamp(g_comp, 0, int(g_cand.size()) - 1);
  g_pend_j = g_pend_r = -1;
  g_luchando = false;
}

void Guion(int k, int prof);
void Empezar() {
  g_f = g_d.piezas;
  g_obj = g_d.objetos;
  g_buscada.assign(g_d.casillas.size(), false);
  g_jefe = -1;
  for (size_t i = 0; i < g_f.size(); ++i) {
    Ficha& f = g_f[i];
    f.oculta = f.bando == 0 && f.c == kSaibaman;   // ponytail: Saibaman enterrado, sale al buscar en su casilla
    f.rival = (f.bando & 2) != 0;
    f.jugador = f.bando == 4 && !f.rival;
    if (f.bando == 0 && !f.oculta) f.fuera = true;
    if (f.rival && !f.oculta && (g_jefe < 0 || f.barras_max >= g_f[size_t(g_jefe)].barras_max)) g_jefe = int(i);
  }
  if (!g_cand.empty()) {   // ponytail: un companero por fase (la IV lleva dos en el original)
    Ficha& c = g_f[size_t(g_cand[size_t(g_comp)])];
    c.fuera = false;
    c.jugador = true;
  }
  g_sel = 0;
  for (size_t i = 0; i < g_f.size(); ++i) {
    Ficha& f = g_f[i];
    if (f.jugador) f.movs = f.movs_turno = 1;
    if (f.jugador && f.c == kGoku && f.bando == 4) g_sel = int(i);
  }
  {   // Bolas de la fase (ELF 0x27BD80)
    std::vector<int> sitios, nums;
    for (const Objeto& o : g_obj)
      if (o.tipo == 0) sitios.push_back(o.casilla);
    g_bolas &= 0x7F;
    g_bolas_ini = g_bolas;
    for (int b = 0; b < 7; ++b)
      if (!(g_bolas >> b & 1)) nums.push_back(b);
    const int tiene = 7 - int(nums.size()), n = tiene >= 7 ? 0 : std::clamp(std::min(g_d.fase, 6) - tiene + 1, 0, 3);
    std::minstd_rand rnd(unsigned(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::shuffle(sitios.begin(), sitios.end(), rnd);
    std::shuffle(nums.begin(), nums.end(), rnd);
    g_bolas_fase.clear();
    for (size_t k = 0; k < size_t(n) && k < sitios.size() && k < nums.size(); ++k) g_bolas_fase.push_back({sitios[k], nums[k]});
    for (const auto& b : g_bolas_fase) REXLOG_INFO("dbz3 tablero b2: Bola {} en la casilla {}", b[1] + 1, b[0]);
  }
  g_turno = 1;
  g_radar = false;
  // PRUEBA: DBZ3_TEST_B2_RADAR=1 empieza con el Radar del Dragon; DBZ3_TEST_B2_BOLAS=N (7 bits) con esas Bolas en la barra
  if (std::getenv("DBZ3_TEST_B2_RADAR")) g_radar = true;
  if (const char* b = std::getenv("DBZ3_TEST_B2_BOLAS")) g_bolas |= std::atoi(b) & 0x7F;
  g_menu = 0;
  g_mensajes.clear();
  std::fill(g_banderas.begin(), g_banderas.end(), 0u);
  for (const auto& t : g_d.t3)   // disparadores del turno 1 (al empezar la fase)
    if (t[0] == -1 && t[1] == 0 && t[2] == 1) Guion(t[3], 0);
  g_dest = -1;
  g_estado = kJuego;
  g_msg.clear();
  const Casilla& c = g_d.casillas[size_t(g_f[size_t(g_sel)].casilla)];
  g_cam_x = c.x;
  g_cam_y = c.y;
  REXLOG_INFO("dbz3 tablero b2: fase {} empieza: {} con {}, jefe {}", g_d.fase + 1, Nombre(g_f[size_t(g_sel)]),
              g_cand.empty() ? std::string("nadie") : Nombre(g_d.piezas[size_t(g_cand[size_t(g_comp)])]),
              g_jefe >= 0 ? Nombre(g_f[size_t(g_jefe)]) : std::string("ninguno"));
}

void Guion(int k, int prof);
// Disparadores de contacto: T3 {pieza, pieza, 0, script} y T4 por personaje con banderas (el primero que vale).
int g_suceso = -1;   // fila de T4 del ultimo contacto (iGpffff8d8c del ELF)
int PersonajeDe(int n) {   // personaje (+0 del registro de T2) de la pieza n
  for (const Ficha& f : g_d.piezas)
    if (f.n == n) return f.c;
  return -1;
}
void SucesosDeContacto(const Ficha& a, const Ficha& b) {
  for (const auto& t : g_d.t3)
    if (t[2] == 0 && ((t[0] == a.n && t[1] == b.n) || (t[0] == b.n && t[1] == a.n))) Guion(t[3], 0);
  g_suceso = -1;
  for (size_t i = 1; i < g_d.t4.size(); ++i) {   // como el ELF, desde la fila 1
    const auto& r = g_d.t4[i];
    const int ca = r[0] ? PersonajeDe(r[0]) : -1, cb = r[1] ? PersonajeDe(r[1]) : -1;
    bool vale;
    if (ca < 0 && cb < 0) vale = false;
    else if (cb < 0) vale = ca == a.c || ca == b.c;
    else if (ca < 0) vale = cb == a.c || cb == b.c;
    else vale = (ca == a.c && cb == b.c) || (ca == b.c && cb == a.c);
    // r[2]: casilla del contacto (+0x0A de la pieza que se mueve, su destino); 0 = cualquiera
    if (!vale || (r[2] != 0 && r[2] != a.casilla && r[2] != b.casilla) || !Condiciones(r, 7)) continue;
    g_suceso = int(i);
    REXLOG_INFO("dbz3 tablero b2: contacto {} y {}: suceso {} (script {})", Nombre(a), Nombre(b), i, r[3]);
    Guion(r[3], 0);
    break;
  }
}
// Tras el combate del contacto: T5 de esa fila con el suceso (3 gana el jugador, 4 pierde: sin confirmar; 5 cualquiera).
void SucesosDeCombate(bool gana) {
  if (g_suceso < 0) return;
  for (const auto& r : g_d.t5) {
    if (r[0] != g_suceso || (r[1] != 5 && r[1] != (gana ? 3 : 4)) || !Condiciones(r, 4)) continue;
    REXLOG_INFO("dbz3 tablero b2: tras el suceso {}: script {}", g_suceso, r[2]);
    Guion(r[2], 0);
    break;
  }
  g_suceso = -1;
}
void Contacto(int j, int r) {
  SucesosDeContacto(g_f[size_t(j)], g_f[size_t(r)]);
  g_pend_j = j;
  g_pend_r = r;
  g_pend_t = g_ahora;
  g_marca = g_f[size_t(r)].casilla;
  g_marca_icono = 8;   // "!!"
  g_marca_t = g_ahora;
  if (B2aB3(g_f[size_t(r)].c) < 0 || B2aB3(g_f[size_t(j)].c) < 0) {
    g_pend_j = g_pend_r = -1;
    Aviso(Nombre(g_f[size_t(r)]) + T::T(" no puede luchar aquí (personaje solo de Budokai 2).",
                                         " can't fight here (Budokai 2-only character)."));
    return;
  }
  Aviso(Nombre(g_f[size_t(j)]) + T::T(" contra ", " versus ") + Nombre(g_f[size_t(r)]) + "!", 8);
}

std::vector<int> Distancias(int c);
void Coger(Ficha& f, Objeto& o) {
  o.hecho = true;
  char t[160];
  switch (o.tipo) {
    case 0: {
      const int b = BolaEn(o.casilla);
      if (b < 0) return Aviso(T::T("No hay nada.", "Nothing here."));
      g_bola_n = g_bolas_fase[size_t(b)][1];
      g_bolas |= 1 << g_bolas_fase[size_t(b)][1];
      g_bolas_fase.erase(g_bolas_fase.begin() + b);
      GuardarProgreso("tablero_b2_bolas", g_bolas);
      g_bola_t = g_ahora;   // D_DBGET: sin letrero; la Bola entra en la barra del marcador
      REXLOG_INFO("dbz3 tablero b2: ¡Una Bola de Dragón! (bolas {:#x})", g_bolas);
      return;
    }
    case 9: {   // el radar dice hacia donde esta la Bola mas cercana por los caminos (ELF 0x27CC30)
      g_radar = true;   // el radar del marcador (D_RADAR) se enciende con sus flechas hacia las Bolas
      g_hud_oculto = false;
      if (g_bolas_fase.empty()) return Aviso(T::T("Radar del Dragón: no queda Bola en esta fase.", "Dragon Radar: no Ball left here."), 26);
      const std::vector<int> dist = Distancias(f.casilla);
      int cerca = g_bolas_fase[0][0];
      for (const auto& bo : g_bolas_fase)
        if (dist[size_t(bo[0])] < dist[size_t(cerca)]) cerca = bo[0];
      const Casilla &a = g_d.casillas[size_t(f.casilla)], &b = g_d.casillas[size_t(cerca)];
      const float dx = b.x - a.x, dy = b.y - a.y;
      const char* dir = std::abs(dx) > 2 * std::abs(dy) ? (dx > 0 ? T::T("la derecha", "the right") : T::T("la izquierda", "the left"))
                        : std::abs(dy) > 2 * std::abs(dx) ? (dy > 0 ? T::T("abajo", "down") : T::T("arriba", "up"))
                        : dy < 0 ? (dx > 0 ? T::T("arriba a la derecha", "up and right") : T::T("arriba a la izquierda", "up and left"))
                                 : (dx > 0 ? T::T("abajo a la derecha", "down and right") : T::T("abajo a la izquierda", "down and left"));
      std::snprintf(t, sizeof(t), T::T("Radar del Dragón: la Bola está hacia %s.", "Dragon Radar: the Ball is %s."), dir);
      return Aviso(t, 26);
    }
    case 8:   // al monedero del HUB (antes se sumaba a g_zenie y se perdia: auditoria8/C)
      g_zenie += o.valor * 100;
      dbz3::rpg::Premio(o.valor * 100);
      std::snprintf(t, sizeof(t), T::T("%d zenie.", "%d zenie."), o.valor * 100);
      return Aviso(t, 25);
    case 5:
      f.movs_turno = 2;
      return Aviso(Nombre(f) + T::T(": cebolla, mueve 2 casillas por turno.", ": onion, moves 2 spaces per turn."), 24);
    case 1:   // daga: +valor % al dano que hace en sus combates (Reglas::ataque). ponytail: que duren toda la fase y
              // se sumen es lo simple; el efecto exacto esta en el ELF 0x26FE80 sin descifrar
      f.ataque += o.valor;
      std::snprintf(t, sizeof(t), T::T("Daga: +%d %% de ataque.", "Dagger: +%d%% attack."), o.valor);
      return Aviso(t, 20);
    case 3:   // armadura: se anula valor % del dano que recibe (Reglas::defensa)
      f.defensa += o.valor;
      std::snprintf(t, sizeof(t), T::T("Armadura: +%d %% de defensa.", "Armor: +%d%% defense."), o.valor);
      return Aviso(t, 21);
    case 7: {   // la capsula de B3 equivalente (o una al azar de las que tienen equivalente); antes solo avisaba
      int c = dbz3::pasarela::nave::CapsulaDeB2(o.valor);
      if (!c) {
        static const int kHay[] = {180, 179, 443, 380, 187, 440, 381, 194, 182, 181, 398, 213, 458, 382, 188, 214, 416,
                                   445, 426, 383, 215, 193};
        c = dbz3::pasarela::nave::CapsulaDeB2(kHay[std::rand() % int(std::size(kHay))]);
      }
      DarCapsulaB3(c);
      REXLOG_INFO("dbz3 tablero b2: cápsula al azar: n.º {}", c);   // el número va al registro: al jugador no le dice nada
      return Aviso(T::T("Cápsula al azar: ya está en tu inventario.", "Random capsule: now in your inventory."), 22);
    }
    case 6:
      return Aviso(T::T("Mr. Popo.", "Mr. Popo."), 16);
  }
  std::snprintf(t, sizeof(t), T::T("Objeto de tipo %d.", "Item of type %d."), o.tipo);
  Aviso(t);
}

// Casillas que hay de `c` a cada casilla por los caminos, sin mirar las fichas (la tabla n x n de bytes del original,
// iGpffff8d24 en el ELF europeo; 255 = sin camino).
std::vector<int> Distancias(int c) {
  std::vector<int> d(g_d.casillas.size(), 255);
  std::vector<int> cola{c};
  d[size_t(c)] = 0;
  for (size_t q = 0; q < cola.size(); ++q) {
    for (int v : g_d.casillas[size_t(cola[q])].vec) {
      if (v <= 0 || d[size_t(v)] != 255) continue;
      d[size_t(v)] = d[size_t(cola[q])] + 1;
      cola.push_back(v);
    }
  }
  return d;
}
// Paso de un rival como el tipo 4 del original (ELF 0x273F40 + 0x273180 + 0x276B80): elige la ficha del jugador mas
// cercana por los caminos (la primera con menos casillas), va a la vecina (en el orden 0..3 de la casilla, la primera)
// que deja menos casillas hasta ella y, si en esa vecina hay otro rival, se queda. -1 = no se mueve.
int PasoA(int desde, int meta);
int PasoHacia(int desde) {
  const std::vector<int> dd = Distancias(desde);
  int meta = -1, mejor = 255;
  for (const Ficha& f : g_f) {
    if (Activa(f) && f.jugador && dd[size_t(f.casilla)] < mejor) {
      mejor = dd[size_t(f.casilla)];
      meta = f.casilla;
    }
  }
  return PasoA(desde, meta);
}
// Paso de `desde` hacia la casilla `meta` (ELF 0x273180): la primera vecina con menos casillas hasta ella; si en esa
// vecina hay otro rival, se queda (0x276B80). -1 = no se mueve.
int PasoA(int desde, int meta) {
  if (meta <= 0 || meta == desde) return -1;
  const std::vector<int> dm = Distancias(meta);
  int paso = -1, menor = 255;
  for (int v : g_d.casillas[size_t(desde)].vec) {
    if (v > 0 && dm[size_t(v)] < menor) {
      menor = dm[size_t(v)];
      paso = v;
    }
  }
  if (paso > 0 && FichaEn(paso, true) >= 0 && FichaEn(paso, false) < 0) return -1;
  return paso;
}

// Turno de los rivales. En el original cada pieza tiene un tipo de movimiento (pieza +0x1C, tabla 0x378FE0: 0 quieto,
// 2 ir a una casilla, 3 al azar, 4 perseguir a la mas cercana, 5 seguir a una pieza, 6 perseguir a la de menos +1, 7/8
// ir a un objetivo o perseguir) que ponen los scripts de cada fase (orden de script 0x293EE0 -> 0x276D80). ponytail:
// hasta descifrar esos scripts, todos los que se mueven usan el tipo 4, cada dos turnos (FAQ); Saibaman y Dr. Gero
// quietos (tipo 0).
// Script k de la fase: de sus ordenes se portea la 50 (tipo de movimiento de una pieza: pieza, tipo, dato; ELF 0x293EE0)
// y la 44 (salta a otro script, 0x294450). ponytail: las demas (dialogos, camara, apariciones, combates, objetos) siguen
// sin portear; su lista y lo que llama cada una: E:/DBZ3HD_trabajo/R2/scr_ops.txt.
void Guion(int k, int prof) {
  if (k < 0 || k >= int(g_d.guiones.size()) || prof > 8) return;
  for (const auto& o : g_d.guiones[size_t(k)]) {
    if (o[1] == 44) return Guion(o[2], prof + 1);   // salta (no vuelve)
    if (o[1] >= 1 && o[1] <= 3 && o[2] >= 0 && o[2] < 2048) {   // banderas
      uint32_t& w = g_banderas[size_t(o[2]) >> 5];
      const uint32_t b = 1u << (o[2] & 31);
      w = o[1] == 1 ? (w | b) : o[1] == 2 ? (w & ~b) : (w ^ b);
      continue;
    }
    // 63/64 {pieza, casilla}: la pieza va andando a esa casilla (ELF 0x277540; -1 = la suya). 58-62 animan el modelo 3D de
    // la pieza (58-60 animacion +0x1C, 61/62 la giran hacia un angulo +0x24 en pasos de 0x800/0x10000) y 65/66 la llevan
    // a un punto del mapa: ponytail, las piezas aqui son retratos; se portaran con los modelos.
    if ((o[1] == 63 || o[1] == 64) && o[3] > 0 && o[3] < int(g_d.casillas.size())) {
      for (Ficha& f : g_f) {
        if (f.n != o[2]) continue;
        f.desde = f.casilla;
        f.casilla = o[3];
        f.t_mov = g_ahora;
        REXLOG_INFO("dbz3 tablero b2: script {}: {} anda a la casilla {}", k, Nombre(f), o[3]);
      }
      continue;
    }
    if (o[1] == 55 || o[1] == 57 || o[1] == 72 || o[1] == 73) {   // colocar pieza / cambiar su ficha (ELF 0x27A370,
      // 0x27A3F0, 0x276B30, 0x276AD0): 55 {pieza, casilla}, 57 {pieza, otra} a la casilla de la otra, 72 {pieza, ficha},
      // 73 {pieza, otra}: la ficha de la otra
      Ficha* pa = nullptr;
      const Ficha* pb = nullptr;
      for (Ficha& f : g_f) {
        if (f.n == o[2]) pa = &f;
        if (f.n == o[3]) pb = &f;
      }
      if (!pa) continue;
      if (o[1] == 55 && o[3] > 0 && o[3] < int(g_d.casillas.size())) pa->casilla = pa->desde = o[3];
      if (o[1] == 57 && pb) pa->casilla = pa->desde = pb->casilla;
      if (o[1] == 72) pa->idx = o[3];
      if (o[1] == 73 && pb) pa->idx = pb->idx;
      REXLOG_INFO("dbz3 tablero b2: script {}: orden {} ({}, {}) a {}", k, o[1], o[2], o[3], Nombre(*pa));
      continue;
    }
    if (o[1] == 41 || o[1] == 42) {   // pantalla de suceso del original (escena o combate de historia)
      REXLOG_INFO("dbz3 tablero b2: script {}: suceso {} ({}) sin portear", k, o[1], o[2]);   // ponytail: falta su escena
      continue;
    }
    if (o[1] == 38) {
      auto dentro = [](int k) {
        return k >= 0 && k < int(g_d.textos.size()) && g_d.textos[size_t(k)].tex >= 0 && g_d.textos[size_t(k)].py > 0 &&
               g_d.textos[size_t(k)].py < 448;
      };
      if (dentro(o[2]) || dentro(o[3])) {
        g_mensajes.push_back({o[2], o[3]});
        REXLOG_INFO("dbz3 tablero b2: script {}: mensaje {} y {}", k, o[2], o[3]);
      }
      continue;
    }
    if (o[1] != 50) continue;
    for (Ficha& f : g_f) {
      if (f.n != o[2]) continue;
      f.mov = o[3];
      f.mov_p = o[4];
      REXLOG_INFO("dbz3 tablero b2: script {}: {} pasa al movimiento {} ({})", k, Nombre(f), f.mov, f.mov_p);
    }
  }
}
// Paso de un rival segun su tipo de movimiento (ELF 0x378FE0): 0 quieto, 2 a la casilla mov_p, 3 al azar, 4 (y los
// demas, ponytail) persigue a la ficha del jugador mas cercana.
int PasoRival(const Ficha& r) {
  if (r.mov == 0) return -1;
  if (r.mov == 2) return PasoA(r.casilla, r.mov_p);
  if (r.mov == 3) {
    const int v = g_d.casillas[size_t(r.casilla)].vec[size_t(std::chrono::steady_clock::now().time_since_epoch().count() & 3)];
    return v > 0 && FichaEn(v, true) < 0 ? v : -1;
  }
  return PasoHacia(r.casilla);
}

void TurnoRival() {
  ++g_turno;
  REXLOG_INFO("dbz3 tablero b2: turno {}", g_turno);
  for (const auto& t : g_d.t3)   // disparadores del turno (-1, 0, turno, script)
    if (t[0] == -1 && t[1] == 0 && t[2] == g_turno) Guion(t[3], 0);
  {
    for (size_t i = 0; i < g_f.size(); ++i) {
      Ficha& r = g_f[i];
      if (!Activa(r) || !r.rival || r.aturdido > 0) continue;
      // -1 ya no sale (T2 da el tipo de cada pieza); se deja por si un #YDW no lo trae
      if (r.mov < 0 && (g_turno % 2 == 0 || r.c == kSaibaman || r.c == kGero)) continue;
      const int p = PasoRival(r);
      if (p <= 0) continue;
      if (const int j = FichaEn(p, false); j >= 0) {
        Contacto(j, int(i));
        break;
      }
      r.desde = r.casilla;
      r.t_mov = g_ahora;
      r.casilla = p;
      REXLOG_INFO("dbz3 tablero b2: {} a la casilla {}", Nombre(r), p);
    }
  }
  for (Ficha& f : g_f) {
    if (f.jugador) f.movs = f.movs_turno;
    if (f.rival && f.aturdido > 0) --f.aturdido;
  }
  for (size_t i = 0; i < g_f.size(); ++i) {
    if (Activa(g_f[i]) && g_f[i].jugador && g_f[i].c == kGoku) g_sel = int(i);
  }
  g_dest = -1;
}
void FinDeAccion() {
  if (g_pend_j >= 0 || g_estado != kJuego) return;
  if (!QuedanMovs()) TurnoRival();
  else if (g_f[size_t(g_sel)].movs <= 0) SiguienteSel();
}

void Mover(int i, int dest) {
  Ficha& f = g_f[size_t(i)];
  --f.movs;
  g_dest = -1;
  if (const int r = FichaEn(dest, true); r >= 0) return Contacto(i, r);
  f.desde = f.casilla;
  f.t_mov = g_ahora;
  f.casilla = dest;
  g_dest = -1;
  REXLOG_INFO("dbz3 tablero b2: turno {}: {} a la casilla {}", g_turno, Nombre(f), dest);
  const Casilla& c = g_d.casillas[size_t(dest)];
  if (c.tipo == 2) {   // Dende: vida llena y una barra mas (FAQ: "+1")
    f.vida = 0;
    f.barras = std::min(f.barras_max, f.barras + 1);
    Aviso(T::T("Dende te cura.", "Dende heals you."));
  } else if (c.tipo == 3 && c.destino > 0 && FichaEn(c.destino, true) < 0) {   // alfombra de Mr. Popo
    f.desde = c.destino;   // ponytail: aparece alli (sin vuelo)
    f.casilla = c.destino;
    Aviso(T::T("Mr. Popo te lleva en su alfombra.", "Mr. Popo carries you on his carpet."), 16);
    REXLOG_INFO("dbz3 tablero b2: Mr. Popo lleva a {} a la casilla {}", Nombre(f), c.destino);
  }
  for (Objeto& o : g_obj) {
    if (EnSuelo(o) && o.casilla == f.casilla) Coger(f, o);
  }
  FinDeAccion();
}

void Buscar(int i) {
  Ficha& f = g_f[size_t(i)];
  if (g_buscada[size_t(f.casilla)]) return Aviso(T::T("Aquí ya has buscado.", "You already searched here."));
  --f.movs;
  g_marca = f.casilla;
  g_marca_icono = 7;   // "?!"
  g_marca_t = g_ahora;
  REXLOG_INFO("dbz3 tablero b2: turno {}: {} busca en la casilla {}", g_turno, Nombre(f), f.casilla);
  for (size_t k = 0; k < g_f.size(); ++k) {
    Ficha& s = g_f[k];
    if (s.oculta && !s.fuera && s.casilla == f.casilla) {
      s.oculta = false;
      s.rival = true;
      return Contacto(i, int(k));
    }
  }
  // una busqueda = el primer enterrado que queda (ELF 0x27C090); sin mas, la casilla queda buscada (en gris)
  auto queda = [&](const Objeto& o) { return o.casilla == f.casilla && Escondido(o); };
  if (auto it = std::find_if(g_obj.begin(), g_obj.end(), queda); it != g_obj.end()) Coger(f, *it);
  else Aviso(T::T("No hay nada.", "Nothing here."));
  g_buscada[size_t(f.casilla)] = std::none_of(g_obj.begin(), g_obj.end(), queda);
  FinDeAccion();
}

// Prueba (auto): la ficha elegida del jugador va primero a la Bola de la fase (y la busca si esta enterrada), despues al
// jefe (por su casilla) y, si no se puede llegar, al rival mas cercano que pueda luchar en B3 (los que solo son de B2 se
// saltan).
void Auto() {
  if (g_estado == kPrepara) return Empezar();
  if (g_estado != kJuego || g_pend_j >= 0) return;
  Ficha& f = g_f[size_t(g_sel)];
  if (!f.jugador || f.movs <= 0 || B2aB3(f.c) < 0) {
    f.movs = 0;
    return FinDeAccion();
  }
  const bool bola = !g_bolas_fase.empty();
  if (BolaEn(f.casilla) >= 0 && !g_buscada[size_t(f.casilla)]) return Buscar(g_sel);
  const bool jefe = g_jefe >= 0 && Activa(g_f[size_t(g_jefe)]) && B2aB3(g_f[size_t(g_jefe)].c) >= 0;
  // primer paso del camino mas corto hasta la primera casilla que cumple `meta` (-1 si no hay)
  auto paso = [&](auto meta) {
    const size_t n = g_d.casillas.size();
    std::vector<int> padre(n, -1);
    std::vector<int> cola{f.casilla};
    padre[size_t(f.casilla)] = f.casilla;
    for (size_t q = 0; q < cola.size(); ++q) {
      const int c = cola[q];
      if (c != f.casilla && meta(c)) {
        int p = c;
        while (padre[size_t(p)] != f.casilla) p = padre[size_t(p)];
        return p;
      }
      for (int v : g_d.casillas[size_t(c)].vec) {
        if (v <= 0 || padre[size_t(v)] >= 0) continue;
        // un rival que solo es de B2 (Babidi en la fase V, inalcanzable como en el original) tapa su casilla
        if (const int r = FichaEn(v, true); r >= 0 && B2aB3(g_f[size_t(r)].c) < 0) continue;
        padre[size_t(v)] = c;
        cola.push_back(v);
      }
    }
    return -1;
  };
  int p = -1;
  if (bola) p = paso([](int c) { return BolaEn(c) >= 0 && !g_buscada[size_t(c)]; });
  if (p < 0 && jefe) p = paso([](int c) { return c == g_f[size_t(g_jefe)].casilla; });
  if (p < 0) p = paso([](int c) {
    const int r = FichaEn(c, true);
    return r >= 0 && B2aB3(g_f[size_t(r)].c) >= 0;
  });
  if (p > 0) return Mover(g_sel, p);
  f.movs = 0;
  FinDeAccion();
}

// ---- Dibujo ----
struct Tex {
  std::unique_ptr<rex::ui::ImmediateTexture> t;
  float w = 0, h = 0;
  ImTextureID id() const { return reinterpret_cast<ImTextureID>(t.get()); }
  explicit operator bool() const { return t != nullptr; }
};
std::vector<Tex> g_tmapa, g_tfichas, g_ticonos, g_tadornos, g_tloc, g_thud;
Tex g_tgo, g_tgoku, g_thoyo, g_tcabezas;
// Goku del tablero en 3D, como en B2: los archivos fijos del tablero (ELF 0x2350C0, tabla 0x377C80) son CMN 335 (#AMO de
// 21 huesos) + 334 (sus 26 animaciones), 337 + 338 (Goku, #AMO de 44 huesos y 21 variantes; con la bandera 4 de
// 0x6992DC, 339 + 340) y 336 (las 19 animaciones de Goku). Los demas personajes del tablero de B2 son fichas (los
// retratos de 342+5k), como aqui. Animacion de espera y de andar: DBZ3_TEST_GOKU_ANIM="espera,andar" (indices de 336).
ps2::Modelo g_goku3d;
std::vector<ps2::Tex> g_goku3d_tx;
ps2::Anim g_goku_espera, g_goku_andar;
float g_goku_alto = 0, g_goku_giro = 0;
bool g_goku3d_cargado = false;
// Ficha del tablero en 3D (CMN 335: #AMO de 21 huesos y 3 partes; su textura 0 es el retrato y la 1 el dorso con la Z de
// cada pieza, de 342+5k: 2 * indice y 2 * indice + 1) con sus 26 animaciones (CMN 334). Las texturas de las fichas de la
// fase como ps2::Tex (las mismas que g_tfichas). ponytail: la espera es la animacion 0 (72 cuadros), sin confirmar.
ps2::Modelo g_ficha3d;
ps2::Anim g_ficha_espera;
float g_ficha_alto = 0;
std::vector<ps2::Tex> g_ps2_fichas;
int g_ps2_fichas_fase = -1;
void CargarGoku3D() {
  if (g_goku3d_cargado) return;
  g_goku3d_cargado = true;
  AfsPs2 cmn;
  if (!cmn.Abrir(BuscarJuego("budokai 2"), "DATA_CMN.AFS")) return;
  g_goku3d = ps2::CargarSuelto(cmn.Entrada(337), cmn.Entrada(338), true, true);   // con colores: el pelo negro
  if (!g_goku3d.esq) return;
  int ie = 0, ia = 4;   // reposo (99 cuadros) y carrera (36), en absoluto (auditoria10/B: la 5 es una guardia)
  if (const char* v = std::getenv("DBZ3_TEST_GOKU_ANIM")) std::sscanf(v, "%d,%d", &ie, &ia);
  const std::vector<uint8_t> amm = cmn.Entrada(336);
  g_goku_espera = ps2::LeerAnim(amm, uint32_t(ie), g_goku3d.esq->padre.size());
  g_goku_andar = ps2::LeerAnim(amm, uint32_t(ia), g_goku3d.esq->padre.size());
  g_goku_espera.relativa = g_goku_andar.relativa = false;   // en relativo salia roto (un brazo arriba, de lado)
  for (const auto& tr : g_goku3d.malla.tris)
    for (const auto& v : tr.p) g_goku_alto = std::max(g_goku_alto, v.y);
  g_goku3d_tx = ps2::Texturas(g_goku3d);
  REXLOG_INFO("dbz3 tablero b2: Goku 3D: {} triangulos, alto {:.1f}, espera {} ({} cuadros), andar {} ({} cuadros)",
              g_goku3d.malla.tris.size(), g_goku_alto, ie, g_goku_espera.cuadros, ia, g_goku_andar.cuadros);
}
void CargarFicha3D() {
  static bool hecho = false;
  if (!hecho) {
    hecho = true;
    AfsPs2 cmn;
    if (cmn.Abrir(BuscarJuego("budokai 2"), "DATA_CMN.AFS")) {
      g_ficha3d = ps2::CargarSuelto(cmn.Entrada(335), {}, true, true);   // B2: la carta opaca con el alfa del retrato
      if (g_ficha3d.esq) {
        const int ie = std::getenv("DBZ3_TEST_FICHA_ANIM") ? std::atoi(std::getenv("DBZ3_TEST_FICHA_ANIM")) : 0;
        g_ficha_espera = ps2::LeerAnim(cmn.Entrada(334), uint32_t(ie), g_ficha3d.esq->padre.size());
        g_ficha_espera.relativa = true;
      }
      for (const auto& tr : g_ficha3d.malla.tris)
        for (const auto& v : tr.p) g_ficha_alto = std::max(g_ficha_alto, v.y);
    }
    REXLOG_INFO("dbz3 tablero b2: ficha 3D: {} triangulos, alto {:.1f}, espera {} cuadros", g_ficha3d.malla.tris.size(),
                g_ficha_alto, g_ficha_espera.cuadros);
  }
  if (g_ps2_fichas_fase != g_d.fase) {   // las texturas de las fichas de esta fase
    g_ps2_fichas_fase = g_d.fase;
    g_ps2_fichas.clear();
    for (const Imagen& im : g_d.fichas) g_ps2_fichas.push_back(ps2::Crear(im));
  }
}
// Camara de una figura del tablero: casi de frente, 20 grados desde arriba, el pie en `pie` y `alto_px` de alto.
ps2::Camara CamaraFigura(ImVec2 pie, float alto_modelo, float alto_px) {
  ps2::Camara cam;
  cam.fov = 0.785f;
  cam.c = pie;
  cam.h = ImVec2(450, 450);
  const float f = 450.0f / std::tan(cam.fov * 0.5f), d = alto_modelo * f / std::max(alto_px, 1.0f);
  const float th = 0.35f, cth = std::cos(th), sth = std::sin(th);
  cam.m[0] = 1;
  cam.m[5] = cth, cam.m[9] = -sth;
  cam.m[6] = sth, cam.m[10] = cth;
  cam.m[15] = 1;
  cam.m[14] = -d;
  cam.ok = true;
  return cam;
}
// Ficha 3D de la pieza `idx` (su retrato y su dorso) de pie en `pie`.
bool DibFicha3D(ImDrawList* dl, int idx, ImVec2 pie, float alto_px, float t) {
  CargarFicha3D();
  const size_t r = size_t(idx) * 2, dorso = r + 1;
  if (!g_ficha3d.malla || g_ficha_alto <= 0 || dorso >= g_ps2_fichas.size() || !g_ps2_fichas[r] || !g_ps2_fichas[dorso])
    return false;
  static ps2::Malla pose;
  const bool anima = g_ficha3d.esq && bool(g_ficha_espera);
  if (anima) ps2::Posar(*g_ficha3d.esq, g_ficha_espera, t, pose);
  ps2::Malla m = anima ? pose : g_ficha3d.malla;
  float ymin = 1e9f;
  for (auto& tr : m.tris) {
    tr.tex = tr.tex == 0 ? int(r) : int(dorso);
    for (const auto& v : tr.p) ymin = std::min(ymin, v.y);
  }
  const ps2::Camara cam = CamaraFigura(pie, g_ficha_alto, alto_px);
  ps2::Escena esc;
  esc.Empezar();
  ps2::Escena::Toon toon;   // cel shading y borde de P4 (la carta y la peana traen normales)
  toon.borde = g_ficha_alto * 0.009f;
  esc.Poner(cam, m, g_ps2_fichas, {0, -ymin, 0}, 0.0f, 1.0f, toon);
  esc.Pintar(dl);
  return true;
}
// Goku 3D de pie en `pie` (pantalla), de `alto_px` de alto, mirando hacia (dx, dy) del mapa si anda.
bool DibGoku3D(ImDrawList* dl, ImVec2 pie, float alto_px, float t, bool anda, float dx, float dy) {
  CargarGoku3D();
  if (!g_goku3d.malla || g_goku_alto <= 0) return false;
  if (anda && (dx != 0 || dy != 0)) g_goku_giro = std::atan2(dx, dy);
  const ps2::Anim& an = anda ? g_goku_andar : g_goku_espera;
  static ps2::Malla pose;
  const bool anima = bool(an);
  if (anima) ps2::Posar(*g_goku3d.esq, an, t, pose);
  // Posar deja la y del modelo: el pie mas bajo al suelo en cada cuadro (con la del primero, al pasar de esperar a
  // andar flotaba o se hundia; auditoria7/G)
  float ymin = 1e9f;
  for (const auto& tr : (anima ? pose : g_goku3d.malla).tris)
    for (const auto& v : tr.p) ymin = std::min(ymin, v.y);
  ps2::Camara cam;   // casi de frente y un poco desde arriba (20 grados), el pie de Goku en `pie`
  cam.fov = 0.785f;
  cam.c = pie;
  cam.h = ImVec2(450, 450);
  const float f = 450.0f / std::tan(cam.fov * 0.5f), d = g_goku_alto * f / std::max(alto_px, 1.0f);
  const float th = 0.35f, cth = std::cos(th), sth = std::sin(th);
  cam.m[0] = 1;
  cam.m[5] = cth, cam.m[9] = -sth;
  cam.m[6] = sth, cam.m[10] = cth;
  cam.m[15] = 1;
  cam.m[14] = -d;
  cam.ok = true;
  ps2::Escena esc;
  esc.Empezar();
  ps2::Escena::Toon toon;   // cel shading y borde negro de P4 (las partes de Goku traen normales, vtype & 1)
  toon.borde = g_goku_alto * 0.009f;
  esc.Poner(cam, anima ? pose : g_goku3d.malla, g_goku3d_tx, {0, -ymin, 0}, g_goku_giro, 1.0f, toon);
  esc.Pintar(dl);
  return true;
}
std::vector<Tex> g_tdeseos, g_tshen_frases, g_tshen_marco;
int g_deseo = 0;
// Shenron en 3D: el modelo de B2 (CMN 327: #AMO hijo 0 con su #AMT hijo 1; el 328 es el sitio, las Bolas y la luz)
// pintado con el lector de modelos de PS2 de P4 (ps2_modelo.h) con una camara fija que lo encuadra entero.
ps2::Modelo g_shen_mod;
std::vector<ps2::Tex> g_shen_tx;
ps2::Tex g_shen_nubes;   // nubes del cielo de la escena del deseo (CMN 328 hijo 1, textura 0)
ps2::Modelo g_shen_sitio;   // la escena del deseo (CMN 328 hijo 0 + #AMT hijo 1), en las coordenadas de Shenron
std::vector<ps2::Tex> g_shen_sitio_tx;
bool g_shen_cargado = false;
int g_shen = 0;   // 0 "Te concedere un deseo", 1 la lista, 2 "Si, eso es aceptable", 3 "Ahora debo irme..."
// Deseo -> capsula de B3 (por nombre: SkillList.dson). Orden de la lista: 540 #0-5 (portal, trajes de Bulma 2-6), 539 #0-4
// (Fusion <Tiencha>, Pothala <Vegetto>(Vegeta), Pothala <Gokutan>(Goku), Pothala <Gokutan>(Mr. Satan), Pothala
// <Kibitoshin>), 538 #0-28 ("Avance" de cada personaje). Solo Pothala <Vegetto>(Vegeta) tiene equivalente: la 192
// (Potara <Vegito> de Vegeta). Sin equivalente en B3: Tiencha, Gokutan, Kibitoshin, los trajes de Bulma (no es
// luchadora en B3) y los 29 "Avance" (B3 no tiene esas capsulas).
int DeseoB3(int k) { return k == 7 ? 192 : 0; }
void DarCapsulaB3(int c) {   // inventario de capsulas de la partida (u8 por ID en 0x824BA110 + 0x2AFE9; hasta 9)
  uint8_t* base = rex::Runtime::instance() ? rex::Runtime::instance()->virtual_membase() : nullptr;
  if (!c || !base) return;
  constexpr dbz3::GA kPartida = 0x824BA110;
  const auto a = kPartida + 0x2AFE9 + c;
  const uint8_t n = REX_LOAD_U8(a);
  if (n < 9) REX_STORE_U8(a, uint8_t(n + 1));
  REXLOG_INFO("dbz3 tablero b2: deseo: capsula {} de B3 ({} -> {})", c, n, std::min(9, n + 1));
}
int g_tfase = -1;

void Texturas() {
  if (g_tfase == g_d.fase) return;
  auto* dr = dbz3::launcher::PreviewDrawer();
  if (!dr) return;
  auto crear = [&](const Imagen& im) {
    Tex x;
    if (!im) return x;
    x.t = dr->CreateTexture(uint32_t(im.w), uint32_t(im.h), rex::ui::ImmediateTextureFilter::kLinear, false,
                            im.rgba.data());
    x.w = float(im.w);
    x.h = float(im.h);
    return x;
  };
  auto lista = [&](const std::vector<Imagen>& v, std::vector<Tex>& out) {
    out.clear();
    for (const Imagen& im : v) out.push_back(crear(im));
  };
  lista(g_d.mapa, g_tmapa);
  lista(g_d.fichas, g_tfichas);
  lista(g_d.adornos, g_tadornos);
  lista(g_d.loc, g_tloc);
  if (g_ticonos.empty()) {
    lista(g_d.hud, g_thud);
    lista(g_d.iconos, g_ticonos);
    g_tgo = crear(g_d.game_over);
    g_tgoku = crear(g_d.goku);
    g_thoyo = crear(g_d.hoyo);
    g_tcabezas = crear(g_d.cabezas);
    lista(g_d.deseos, g_tdeseos);
    lista(g_d.shen_frases, g_tshen_frases);
    lista(g_d.shen_marco, g_tshen_marco);
  }
  g_tfase = g_d.fase;
}
const Tex* Icono(int k) { return k >= 0 && k < int(g_ticonos.size()) && g_ticonos[size_t(k)] ? &g_ticonos[size_t(k)] : nullptr; }
// Recorte (px de la textura) de un icono en p0..p1.
void DibIcono(ImDrawList* dl, int k, ImVec2 p0, ImVec2 p1, ImVec4 src = ImVec4(0, 0, 0, 0), ImU32 col = IM_COL32_WHITE) {
  const Tex* x = Icono(k);
  if (!x) return;
  if (src.z <= 0) src = ImVec4(0, 0, x->w, x->h);
  dl->AddImage(x->id(), p0, p1, ImVec2(src.x / x->w, src.y / x->h), ImVec2(src.z / x->w, src.w / x->h), col);
}

// Pantalla de 640x448 del original dentro del area: origen y escala.
struct Lienzo {
  ImVec2 o;
  float s = 1;
  ImVec2 P(float x, float y) const { return ImVec2(o.x + x * s, o.y + y * s); }
};
Lienzo LienzoDe(const S::Area& a) {
  Lienzo L;
  L.s = std::min(1200.0f / 640.0f, 900.0f / 448.0f) * a.s;
  L.o = ImVec2(a.p0.x + (1200.0f * a.s - 640.0f * L.s) * 0.5f, a.p0.y + (900.0f * a.s - 448.0f * L.s) * 0.5f);
  return L;
}
// Animacion k de un #AMA del original (texturas tx) en su sitio: c = cuadro de su lista A a 60 por segundo (< 0 = el de
// reposo; posicion lineal entre claves), (dx, dy) = desplazamiento, giro = radianes alrededor de su ancla.
// Pose de la animacion de un sprite en el cuadro c (< 0 = reposo): posicion, escala y alfa de la lista A, lineales entre
// claves; pasado el ultimo cuadro, el reposo (ultima clave).
struct Pose {
  float px, py, ex, ey, al;
};
Pose PoseDe(const Datos::Sprite& sp, float c) {
  Pose o{sp.px, sp.py, sp.ex, sp.ey, 1.0f};
  for (size_t i = 1; c >= 0 && i < sp.claves.size(); ++i) {
    const auto &k0 = sp.claves[i - 1], &k1 = sp.claves[i];
    if (c >= k1[0]) continue;
    const float f = k1[0] > k0[0] ? std::clamp((c - k0[0]) / (k1[0] - k0[0]), 0.0f, 1.0f) : 1.0f;
    o.px = k0[1] + (k1[1] - k0[1]) * f;
    o.py = k0[2] + (k1[2] - k0[2]) * f;
    o.al = k0[3] + (k1[3] - k0[3]) * f;
    o.ex = k0[4] + (k1[4] - k0[4]) * f;
    o.ey = k0[5] + (k1[5] - k0[5]) * f;
    break;
  }
  return o;
}
void DibSp(ImDrawList* dl, const std::vector<Datos::Sprite>& v, const std::vector<Tex>& tx, int k, const Lienzo& L,
           float c = -1, ImU32 col = IM_COL32_WHITE, float dx = 0, float dy = 0, float giro = 0) {
  if (k < 0 || k >= int(v.size())) return;
  const Datos::Sprite& sp = v[size_t(k)];
  if (sp.tex < 0 || sp.tex >= int(tx.size()) || !tx[size_t(sp.tex)]) return;
  const Tex& x = tx[size_t(sp.tex)];
  const Pose po = PoseDe(sp, c);
  float px = po.px + dx, py = po.py + dy;
  if (po.al < 1.0f)   // alfa de la clave (0..1) sobre el del color
    col = (col & 0x00FFFFFFu) | (ImU32(float(col >> 24) * std::clamp(po.al, 0.0f, 1.0f)) << 24);
  const ImVec2 uv0(sp.u0 / x.w, sp.v0 / x.h), uv1(sp.u1 / x.w, sp.v1 / x.h);
  const float x0 = sp.x0 * po.ex, x1 = sp.x1 * po.ex, y0 = sp.y0 * po.ey, y1 = sp.y1 * po.ey;
  if (giro == 0) return dl->AddImage(x.id(), L.P(px + x0, py + y0), L.P(px + x1, py + y1), uv0, uv1, col);
  const float cs = std::cos(giro), sn = std::sin(giro);
  auto r = [&](float lx, float ly) { return L.P(px + lx * cs - ly * sn, py + lx * sn + ly * cs); };
  dl->AddImageQuad(x.id(), r(x0, y0), r(x1, y0), r(x1, y1), r(x0, y1), uv0, ImVec2(uv1.x, uv0.y), uv1, ImVec2(uv0.x, uv1.y), col);
}
void DibHudSp(ImDrawList* dl, int k, const Lienzo& L, float c = -1, ImU32 col = IM_COL32_WHITE, float dx = 0, float dy = 0,
              float giro = 0) {
  DibSp(dl, g_d.hud_ama, g_thud, k, L, c, col, dx, dy, giro);
}

// Camara del original: casi cenital (el mapa ya viene pintado en vista inclinada; en las capturas apenas hay
// perspectiva); 1 px del mapa = 1 px de una pantalla de 480 de alto. Punto (mx, my) del mapa a pantalla.
struct Cam {
  float cx = 0, cy = 0, d = 900, f = 1800, sn = 0.97f, cs = 0.24f;
  ImVec2 c;
  bool P(float mx, float my, ImVec2& out, float* esc = nullptr) const {
    const float dx = mx - cx, dy = my - cy, z = d - dy * cs;
    if (z < 80) return false;
    out = ImVec2(c.x + f * dx / z, c.y + f * dy * sn / z);
    if (esc) *esc = f / z;
    return true;
  }
};

// Retrato hexagonal de una ficha (342+5k: 2 * indice), Saibaman (398 #10, 3 cuadros) o Goku (sin ficha en B2:
// hexagono azul como los de los companeros con su cara de B3; ponytail, en el original seguramente es su modelo 3D).
void DibFicha(ImDrawList* dl, const Ficha& f, ImVec2 base, float w, float t, ImU32 col) {
  if (f.c == kSaibaman) {
    const int k = int(t * 5) % 3;
    const float h = w * 80.0f / 104.0f;
    DibIcono(dl, 10, ImVec2(base.x - w * 0.5f, base.y - h), ImVec2(base.x + w * 0.5f, base.y),
             ImVec4(104.0f * float(k), 0, 104.0f * float(k + 1), 80), col);
    return;
  }
  const size_t r = size_t(f.idx) * 2;
  if (!(f.c == kGoku && f.bando == 4) && r < g_tfichas.size() && g_tfichas[r]) {
    dl->AddImage(g_tfichas[r].id(), ImVec2(base.x - w * 0.5f, base.y - w), ImVec2(base.x + w * 0.5f, base.y), ImVec2(0, 0),
                 ImVec2(1, 1), col);
    return;
  }
  const ImVec2 c(base.x, base.y - w * 0.43f);
  const float R = w * 0.40f;
  ImVec2 v[6];
  for (int k = 0; k < 6; ++k) {
    const float a = 3.14159265f / 3.0f * float(k);
    v[k] = ImVec2(c.x + R * std::cos(a), c.y + R * std::sin(a) * 0.95f);
  }
  dl->AddConvexPolyFilled(v, 6, IM_COL32(70, 150, 230, (col >> 24) & 0xFF));
  if (ImTextureID cara = CaraDe(B2aB3(f.c))) {
    dl->PushTexture(ImTextureRef(cara));
    const ImDrawIdx b = ImDrawIdx(dl->_VtxCurrentIdx);
    dl->PrimReserve(18, 7);
    auto uv = [&](ImVec2 p) { return ImVec2(0.5f + (p.x - c.x) / (2.3f * R), 0.5f + (p.y - c.y) / (2.3f * R)); };
    dl->PrimWriteVtx(c, uv(c), col);
    for (int k = 0; k < 6; ++k) dl->PrimWriteVtx(v[k], uv(v[k]), col);
    for (int k = 0; k < 6; ++k) {
      dl->PrimWriteIdx(b);
      dl->PrimWriteIdx(ImDrawIdx(b + 1 + k));
      dl->PrimWriteIdx(ImDrawIdx(b + 1 + (k + 1) % 6));
    }
    dl->PopTexture();
  }
  dl->AddPolyline(v, 6, IM_COL32(40, 50, 220, (col >> 24) & 0xFF), ImDrawFlags_Closed, std::max(2.0f, w * 0.05f));
}

// Tramos de vida (398 #9: 16x16 verde claro y oscuro) apilados en vertical junto a la ficha: uno por barra.
void Barras(ImDrawList* dl, const Ficha& f, ImVec2 base, float e) {
  for (int k = 0; k < f.barras_max; ++k) {
    const float y1 = base.y - float(k) * 9 * e;
    DibIcono(dl, 9, ImVec2(base.x, y1 - 8 * e), ImVec2(base.x + 9 * e, y1),
             k < f.barras ? ImVec4(0, 0, 16, 16) : ImVec4(16, 0, 32, 16));
  }
}
// Cifras azules del original (398 #6: "123450"); las que no tiene (6-9), con las de 398 #29.
void CifrasAzules(ImDrawList* dl, int n, ImVec2 c, float h) {
  const std::string s = std::to_string(std::max(0, n));
  float x = c.x - float(s.size()) * h * 0.4f;
  for (char ch : s) {
    const int d = ch - '0';
    if (d <= 5) {
      const int celda = d == 0 ? 5 : d - 1;
      DibIcono(dl, 6, ImVec2(x - h * 0.1f, c.y - h * 0.5f), ImVec2(x + h * 0.9f, c.y + h * 0.5f),
               ImVec4(32.0f * float(celda), 0, 32.0f * float(celda + 1), 32));
    } else {
      DibIcono(dl, 29, ImVec2(x + h * 0.1f, c.y - h * 0.6f), ImVec2(x + h * 0.85f, c.y + h * 0.4f),
               ImVec4(24.0f * float(d), 0, 24.0f * float(d + 1), 32), IM_COL32(120, 160, 255, 255));
    }
    x += h * 0.8f;
  }
}
// Cifras: 398 #28 (0-9 de 24x24).
void Cifras(ImDrawList* dl, int n, ImVec2 p, float h) {
  const std::string s = std::to_string(std::max(0, n));
  for (size_t i = 0; i < s.size(); ++i) {
    const int d = s[i] - '0';
    DibIcono(dl, 28, ImVec2(p.x + float(i) * h * 0.85f, p.y), ImVec2(p.x + float(i) * h * 0.85f + h, p.y + h),
             ImVec4(24.0f * float(d), 0, 24.0f * float(d + 1), 24));
  }
}

// Una pieza sobre el tablero; devuelve la y de pantalla de su parte de arriba.
//   Saibaman: la cabeza verde que asoma del agujero (idioma 146 #13 y #16, 3 cuadros).
//   Goku: de cuerpo entero (idioma 155, el sprite de 64x88 de la intro). Los demas personajes del original son modelos 3D;
//   ponytail: aqui, su retrato hexagonal de pie sobre la peana (el dorso con la Z de su ficha, plano en el suelo).
float DibPieza(ImDrawList* dl, const Cam& k, const Ficha& f, ImVec2 pos, float t, ImU32 col) {
  ImVec2 p;
  float e = 1;
  if (!k.P(pos.x, pos.y, p, &e)) return -1;
  float arriba = p.y - 68 * e;
  if (f.c == kSaibaman && g_thoyo && g_tcabezas) {
    dl->AddImage(g_thoyo.id(), ImVec2(p.x - 36 * e, p.y - 18 * e), ImVec2(p.x + 36 * e, p.y + 18 * e));
    const int n = int(t * 3) % 3;
    dl->AddImage(g_tcabezas.id(), ImVec2(p.x - 28 * e, p.y - 48 * e), ImVec2(p.x + 28 * e, p.y + 8 * e),
                 ImVec2(float(n) / 3, 0), ImVec2(float(n + 1) / 3, 1), col);
    arriba = p.y - 48 * e;
  } else if (f.c == kGoku && f.bando == 4 && (g_tgoku || g_goku3d.malla)) {
    dl->AddEllipseFilled(ImVec2(p.x, p.y), ImVec2(22 * e, 7 * e), IM_COL32(0, 0, 0, 90));
    // en 3D (su modelo del tablero de B2, andando durante el paso); sin el, el sprite de la intro
    const Casilla &c0 = g_d.casillas[size_t(f.desde)], &c1 = g_d.casillas[size_t(f.casilla)];
    const bool anda = g_ahora - f.t_mov < 0.35f && f.desde != f.casilla;
    if (!DibGoku3D(dl, p, 80 * e, t, anda, c1.x - c0.x, c1.y - c0.y) && g_tgoku)
      dl->AddImage(g_tgoku.id(), ImVec2(p.x - 29 * e, p.y - 80 * e), ImVec2(p.x + 29 * e, p.y), ImVec2(0, 0), ImVec2(1, 1), col);
    arriba = p.y - 80 * e;
  } else if (f.c != kSaibaman && DibFicha3D(dl, f.idx, p, 96 * e, t + float(f.n) * 0.37f)) {
    arriba = p.y - 96 * e;   // la ficha 3D del original (CMN 335) con el retrato y el dorso de su pieza
  } else {
    const size_t dorso = size_t(f.idx) * 2 + 1;
    ImVec2 v[7];
    bool ok = f.c != kSaibaman && dorso < g_tfichas.size() && g_tfichas[dorso] && k.P(pos.x, pos.y, v[0]);
    for (int i = 0; ok && i < 6; ++i) {
      const float an = 3.14159265f / 3.0f * float(i);
      ok = k.P(pos.x + 30 * std::cos(an), pos.y + 22 * std::sin(an), v[i + 1]);
    }
    if (ok) {   // el hexagono de la Z (centro 0.41, radio 0.36 de la textura 128x128)
      dl->PushTexture(ImTextureRef(g_tfichas[dorso].id()));
      const ImDrawIdx b = ImDrawIdx(dl->_VtxCurrentIdx);
      dl->PrimReserve(18, 7);
      dl->PrimWriteVtx(v[0], ImVec2(0.41f, 0.41f), col);
      for (int i = 0; i < 6; ++i) {
        const float an = 3.14159265f / 3.0f * float(i);
        dl->PrimWriteVtx(v[i + 1], ImVec2(0.41f + 0.36f * std::cos(an), 0.41f + 0.36f * std::sin(an)), col);
      }
      for (int i = 0; i < 6; ++i) {
        dl->PrimWriteIdx(b);
        dl->PrimWriteIdx(ImDrawIdx(b + 1 + i));
        dl->PrimWriteIdx(ImDrawIdx(b + 1 + (i + 1) % 6));
      }
      dl->PopTexture();
    }
    DibFicha(dl, f, ImVec2(p.x, p.y - 6 * e), 62 * e, t, col);
  }
  Barras(dl, f, ImVec2(p.x - 40 * e, p.y), e);
  if (f.rival && f.aturdido > 0) {   // aturdido: la nube de pensar (398 #3, la derecha) con el remolino (398 #4)
    const ImVec2 c(p.x + 34 * e, arriba - 18 * e);
    DibIcono(dl, 3, ImVec2(c.x - 28 * e, c.y - 24 * e), ImVec2(c.x + 28 * e, c.y + 24 * e), ImVec4(56, 0, 112, 48));
    const int n = int(t * 4) % 2;
    DibIcono(dl, 4, ImVec2(c.x - 14 * e, c.y - 16 * e), ImVec2(c.x + 14 * e, c.y + 12 * e),
             ImVec4(32.0f * float(n), 0, 32.0f * float(n + 1), 32));
  }
  return arriba;
}

ImVec2 PosFicha(const Ficha& f) {
  const Casilla &a = g_d.casillas[size_t(f.desde)], &b = g_d.casillas[size_t(f.casilla)];
  float k = std::clamp((g_ahora - f.t_mov) / 0.35f, 0.0f, 1.0f);
  k = 1.0f - (1.0f - k) * (1.0f - k);
  return ImVec2(a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k);
}

void DibTablero(ImDrawList* dl, float t, bool prepara) {
  const ImVec2 W = ImGui::GetIO().DisplaySize;
  // camara: sigue a la ficha elegida (o al rival del combate pendiente) sin salirse mucho del mapa
  ImVec2 obj(g_d.ancho * 0.5f, g_d.alto * 0.5f);
  if (!prepara && g_sel < int(g_f.size())) obj = PosFicha(g_f[size_t(g_pend_r >= 0 ? g_pend_r : g_sel)]);
  Cam k;
  k.c = ImVec2(W.x * 0.5f, W.y * 0.5f);
  k.f = k.d * W.y / 480.0f;
  // distancia (px del mapa) del centro al borde de abajo de la pantalla y medio ancho visible ahi
  const float abajo = 0.5f * W.y * k.d / (k.f * k.sn + 0.5f * W.y * k.cs);
  const float medio = 0.5f * W.x * (k.d - abajo * k.cs) / k.f;
  obj.x = g_d.ancho > 2 * medio ? std::clamp(obj.x, medio, g_d.ancho - medio) : g_d.ancho * 0.5f;
  obj.y = std::min(obj.y, g_d.alto - abajo);
  const float dt = std::clamp(ImGui::GetIO().DeltaTime, 0.0f, 0.1f);
  g_cam_x += (obj.x - g_cam_x) * std::min(1.0f, dt * 4.0f);
  g_cam_y += (obj.y - g_cam_y) * std::min(1.0f, dt * 4.0f);
  k.cx = g_cam_x;
  k.cy = g_cam_y;
  dl->AddRectFilledMultiColor(ImVec2(0, 0), W, IM_COL32(18, 36, 80, 255), IM_COL32(18, 36, 80, 255),
                              IM_COL32(6, 14, 34, 255), IM_COL32(6, 14, 34, 255));
  // terreno: las piezas del mapa en 4x4 trozos (la perspectiva de AddImageQuad es por triangulo). Con la camara
  // inclinada se ve mas ancho arriba que abajo: alrededor del mapa van copias en espejo (izquierda, derecha y detras)
  // para que no asomen esquinas vacias; los bordes casan porque el espejo repite el mismo pixel.
  constexpr int kTrozos = 4;
  for (int oy = -1; oy <= 0; ++oy)
  for (int ox = -1; ox <= 1; ++ox)
  for (size_t j = 0; j < g_tmapa.size(); ++j) {
    if (!g_tmapa[j]) continue;
    const float x0 = 256.0f * float(int(j) % g_d.cols), y0 = 256.0f * float(int(j) / g_d.cols);
    const float pw = std::min(256.0f, g_d.ancho - x0), ph = std::min(256.0f, g_d.alto - y0);
    if (pw <= 0 || ph <= 0) continue;
    auto espejo = [&](float mx, float my, ImVec2& out) {
      mx = ox < 0 ? -mx : ox > 0 ? 2 * g_d.ancho - mx : mx;
      my = oy < 0 ? -my : my;
      return k.P(mx, my, out);
    };
    for (int a = 0; a < kTrozos; ++a) {
      for (int b = 0; b < kTrozos; ++b) {
        const float u0 = pw * float(a) / kTrozos, u1 = pw * float(a + 1) / kTrozos;
        const float v0 = ph * float(b) / kTrozos, v1 = ph * float(b + 1) / kTrozos;
        ImVec2 p[4];
        if (!espejo(x0 + u0, y0 + v0, p[0]) || !espejo(x0 + u1, y0 + v0, p[1]) || !espejo(x0 + u1, y0 + v1, p[2]) ||
            !espejo(x0 + u0, y0 + v1, p[3]))
          continue;
        if (std::max({p[0].y, p[1].y, p[2].y, p[3].y}) < 0 || std::min({p[0].y, p[1].y, p[2].y, p[3].y}) > W.y ||
            std::max({p[0].x, p[1].x, p[2].x, p[3].x}) < 0 || std::min({p[0].x, p[1].x, p[2].x, p[3].x}) > W.x)
          continue;
        dl->AddImageQuad(g_tmapa[j].id(), p[0], p[1], p[2], p[3], ImVec2(u0 / 256, v0 / 256), ImVec2(u1 / 256, v0 / 256),
                         ImVec2(u1 / 256, v1 / 256), ImVec2(u0 / 256, v1 / 256));
      }
    }
  }
  // adornos de la fase (345+5k con su recorte del #AMA 344+5k) en sus sitios de la tabla 7; bajo los caminos
  for (const auto& ad : g_d.colocados) {
    if (ad.obj < 0 || ad.obj >= int(g_d.sprites.size())) continue;
    const Datos::Sprite& sp = g_d.sprites[size_t(ad.obj)];
    if (sp.tex < 0 || sp.tex >= int(g_tadornos.size()) || !g_tadornos[size_t(sp.tex)]) continue;
    ImVec2 p;
    float e = 1;
    if (!k.P(ad.x, ad.y, p, &e)) continue;
    const Tex& x = g_tadornos[size_t(sp.tex)];
    dl->AddImage(x.id(), ImVec2(p.x + sp.x0 * e, p.y + sp.y0 * e), ImVec2(p.x + sp.x1 * e, p.y + sp.y1 * e),
                 ImVec2(sp.u0 / x.w, sp.v0 / x.h), ImVec2(sp.u1 / x.w, sp.v1 / x.h));
  }
  const int sel_c = prepara || g_sel >= int(g_f.size()) ? -1 : g_f[size_t(g_sel)].casilla;
  // caminos: lineas finas verde claro, semitransparentes
  for (size_t i = 1; i < g_d.casillas.size(); ++i) {
    for (int v : g_d.casillas[i].vec) {
      if (v <= int(i)) continue;
      ImVec2 a, b;
      float ea = 1, eb = 1;
      if (!k.P(g_d.casillas[i].x, g_d.casillas[i].y, a, &ea) || !k.P(g_d.casillas[size_t(v)].x, g_d.casillas[size_t(v)].y, b, &eb))
        continue;
      const bool activo = (int(i) == sel_c && v == g_dest) || (v == sel_c && int(i) == g_dest);
      const float pulso = activo ? 0.5f + 0.5f * std::sin(t * 7.0f) : 0.0f;
      dl->AddLine(a, b, activo ? IM_COL32(255, 255, 170, int(160 + 90 * pulso)) : IM_COL32(150, 255, 200, 150),
                  3.0f * (ea + eb) * 0.5f);
    }
  }
  // casillas (398): disco azul pequeno (#0), Dende (#11), Mr. Popo en su alfombra (#16, #18), combate (#14 con el icono de
  // su condicion). Ya buscadas: en gris.
  for (size_t i = 1; i < g_d.casillas.size(); ++i) {
    const Casilla& c = g_d.casillas[i];
    ImVec2 p;
    float e = 1;
    if (!k.P(c.x, c.y, p, &e)) continue;
    const bool buscada = !prepara && i < g_buscada.size() && g_buscada[i];
    const ImU32 col = buscada ? IM_COL32(140, 140, 140, 255) : IM_COL32_WHITE;
    if (c.tipo == 2) {   // Dende: el disco grande con aro dorado
      DibIcono(dl, 11, ImVec2(p.x - 24 * e, p.y - 16 * e), ImVec2(p.x + 24 * e, p.y + 16 * e), ImVec4(0, 0, 0, 0), col);
    } else if (c.tipo == 3) {   // Mr. Popo sobre su alfombra (4 cuadros de 104x56)
      const int n = int(t * 6) % 4;
      DibIcono(dl, 18, ImVec2(p.x - 52 * e, p.y - 28 * e), ImVec2(p.x + 52 * e, p.y + 28 * e), ImVec4(0, 56.0f * float(n), 104, 56.0f * float(n + 1)), col);
      DibIcono(dl, 16, ImVec2(p.x - 30 * e, p.y - 84 * e), ImVec2(p.x + 30 * e, p.y - 6 * e), ImVec4(0, 0, 0, 0), col);
    } else if (c.lucha || c.condicion) {   // casilla de combate: boton morado, kanji y en el hexagono su condicion
      DibIcono(dl, 14, ImVec2(p.x - 24 * e, p.y - 10 * e), ImVec2(p.x + 48 * e, p.y + 70 * e), ImVec4(0, 0, 0, 0), col);
      // 1 media vida (corazon a medias), 2 ki a 0 (el 0 con llama), 3 vida que baja (corazon gris), 4 ki que baja
      // (llama), 5 sin bloqueo (la X roja): corazones 398 #15 (32x32, 8 niveles) y 398 #19 (filas de 32x32)
      const ImVec4 src[6] = {ImVec4(0, 0, 0, 0), ImVec4(96, 0, 128, 32), ImVec4(0, 64, 32, 96), ImVec4(96, 32, 128, 64),
                             ImVec4(128, 64, 160, 96), ImVec4(160, 32, 192, 64)};
      if (c.condicion >= 1 && c.condicion <= 5)
        DibIcono(dl, c.condicion == 1 || c.condicion == 3 ? 15 : 19, ImVec2(p.x + 10 * e, p.y + 20 * e),
                 ImVec2(p.x + 38 * e, p.y + 48 * e), src[c.condicion], col);
    } else {
      DibIcono(dl, 0, ImVec2(p.x - 12 * e, p.y - 12 * e), ImVec2(p.x + 12 * e, p.y + 12 * e), ImVec4(0, 0, 0, 0), col);
    }
  }
  // objetos a la vista y piezas, de atras adelante
  struct Cosa {
    float y;
    int ficha, objeto;
  };
  std::vector<Cosa> cosas;
  if (!prepara) {
    for (size_t i = 0; i < g_obj.size(); ++i) {
      if (EnSuelo(g_obj[i])) cosas.push_back({g_d.casillas[size_t(g_obj[i].casilla)].y - 1, -1, int(i)});
    }
    for (size_t i = 0; i < g_f.size(); ++i) {
      if (Activa(g_f[i])) cosas.push_back({PosFicha(g_f[i]).y, int(i), -1});
    }
  } else {
    for (size_t i = 0; i < g_d.piezas.size(); ++i) {
      const Ficha& f = g_d.piezas[i];
      if (f.bando != 0) cosas.push_back({g_d.casillas[size_t(f.casilla)].y, -2 - int(i), -1});
    }
  }
  std::sort(cosas.begin(), cosas.end(), [](const Cosa& a, const Cosa& b) { return a.y < b.y; });
  auto casilla_de = [&](const Cosa& q) {
    return q.objeto >= 0 ? -1 : q.ficha <= -2 ? g_d.piezas[size_t(-2 - q.ficha)].casilla : g_f[size_t(q.ficha)].casilla;
  };
  std::vector<int> cuantas(g_d.casillas.size(), 0);
  for (const Cosa& q : cosas) {
    if (casilla_de(q) >= 0) ++cuantas[size_t(casilla_de(q))];
  }
  std::vector<int> vistas(g_d.casillas.size(), 0);
  for (const Cosa& q : cosas) {
    if (q.objeto >= 0) {   // de pie junto a la casilla, a su tamano del original (398)
      const Objeto& o = g_obj[size_t(q.objeto)];
      const Casilla& c = g_d.casillas[size_t(o.casilla)];
      ImVec2 p;
      float e = 1;
      if (!k.P(c.x + 6, c.y, p, &e)) continue;
      if (o.tipo == 0) {
        const int b = BolaEn(o.casilla);
        if (b < 0) continue;
        const int n = g_bolas_fase[size_t(b)][1];
        DibIcono(dl, 2, ImVec2(p.x - 18 * e, p.y - 40 * e), ImVec2(p.x + 18 * e, p.y - 4 * e), ImVec4(40.0f * float(n), 0, 40.0f * float(n + 1), 40));
        continue;
      }
      const int ic = o.tipo == 9 ? 26 : o.tipo == 1 ? 20 : o.tipo == 3 ? 21 : o.tipo == 8 ? 25 : o.tipo == 7 ? 22 : o.tipo == 6 ? 16 : o.tipo == 5 ? 24 : 17;
      const Tex* x = Icono(ic);
      if (!x) continue;
      const float w = (ic == 20 || ic == 21) ? x->w * 0.5f : x->w, h = x->h;   // daga y armadura: la primera de dos
      DibIcono(dl, ic, ImVec2(p.x - w * 0.5f * e, p.y - h * e), ImVec2(p.x + w * 0.5f * e, p.y), ImVec4(0, 0, w, h));
      continue;
    }
    const bool en_prep = q.ficha <= -2;
    const Ficha& f = en_prep ? g_d.piezas[size_t(-2 - q.ficha)] : g_f[size_t(q.ficha)];
    ImVec2 pos = en_prep ? ImVec2(g_d.casillas[size_t(f.casilla)].x, g_d.casillas[size_t(f.casilla)].y) : PosFicha(f);
    const int n_aqui = cuantas[size_t(f.casilla)], k_aqui = vistas[size_t(f.casilla)]++;
    pos.x += (float(k_aqui) - float(n_aqui - 1) * 0.5f) * 40.0f;
    const bool hecha = !en_prep && f.jugador && f.movs <= 0;
    const float arriba = DibPieza(dl, k, f, pos, t, hecha ? IM_COL32(170, 170, 180, 255) : IM_COL32_WHITE);
    ImVec2 p;
    float e = 1;
    if (!en_prep && q.ficha == g_sel && arriba >= 0 && k.P(pos.x, pos.y, p, &e)) {   // la elegida: sus movimientos en azul
      CifrasAzules(dl, f.movs, ImVec2(p.x + 34 * e, arriba + 6 * e + 3 * e * std::sin(t * 4.0f)), 26 * e);
    }
  }
  // destino elegido con la cruceta: la flecha roja animada (398 #19, fila de arriba)
  if (!prepara && g_dest > 0) {
    ImVec2 p;
    float e = 1;
    if (k.P(g_d.casillas[size_t(g_dest)].x, g_d.casillas[size_t(g_dest)].y, p, &e)) {
      const int n = int(t * 8) % 4;
      const float s = 34 * e, bob = 5 * e * std::sin(t * 6.0f);
      DibIcono(dl, 19, ImVec2(p.x - s * 0.5f, p.y - s * 1.5f + bob), ImVec2(p.x + s * 0.5f, p.y - s * 0.5f + bob),
               ImVec4(32.0f * float(n), 0, 32.0f * float(n + 1), 32));
    }
  }
  // "!!" del contacto o "?!" de la busqueda (398 #8 / #7)
  if (!prepara && g_marca > 0 && t - g_marca_t < 1.2f) {
    ImVec2 p;
    float e = 1;
    if (k.P(g_d.casillas[size_t(g_marca)].x, g_d.casillas[size_t(g_marca)].y, p, &e)) {
      const float s = 46 * e * (1.0f + 0.3f * std::max(0.0f, 0.2f - (t - g_marca_t)) * 5.0f);
      DibIcono(dl, g_marca_icono, ImVec2(p.x - s * 0.5f + 30 * e, p.y - 66 * e - s * 1.4f), ImVec2(p.x + s * 0.5f + 30 * e, p.y - 66 * e - s * 0.4f));
    }
  }
}

// Marcador del original (CMN 402 + DATA_<idioma> 188), en la pantalla de 640x448:
//   D_TURN (ELF 0x26E830/0x26E6F0): rombo (anim 39) con el turno (hasta 99): cifra d = anim 41+d, puesta en el sitio de la
//   41 (una cifra) o de la 42 (unidades) y la 43 (decenas).
//   D_RADARBASE / DBALL: barra de madera (1), "MUNDO DRAGON" (4) y cada Bola conseguida en su hueco (Bola 1 = 8, las
//   demas 9+n). D_DBGET: la recien cogida entra con las claves de la 7 (20 cuadros).
//   D_RADAR (0x26F940, solo con el Radar del Dragon recogido): radar (17), brillo (89) y, por cada Bola de la fase, la
//   flecha (87) girada hacia ella si esta a mas de 300 px del mapa o el punto (88) a 1/10 de la distancia.
//   L1 esconde y saca la barra y el radar (0x2700E0).
//   Entradas y salidas (60 cuadros por segundo, claves del #AMA): barra 0 entra (20) / 1 reposo / 2 sale (15) y "MUNDO
//   DRAGON" 3 / 4 / 5 (D_RADARBASE, 0x26F5D0; los huecos de las Bolas van con la barra), radar 16 entra (20) / 17 / 18 sale
//   (15) y el rombo del turno, 38 al llegar al tablero (10: de nada a 1,3 y a 1; D_TURN 0x26EA10). Todo entra al empezar
//   el tablero o volver de un combate y al sacar el marcador con L1. El radar roto (24-37, "BOOM": estado 5 de D_RADAR) no
//   lo pide ninguna parte del juego (ninguna escritura de ese estado en el ELF europeo): sin portar.
// Sin los archivos (otra ISO), el marcador de antes con colores propios.
// PRUEBA: DBZ3_TEST_B2_LENTO=N alarga N veces las entradas y salidas del marcador y del menu para verlas en una captura
float Lento() {
  static const float l = std::getenv("DBZ3_TEST_B2_LENTO") ? std::max(1.0f, float(std::atof(std::getenv("DBZ3_TEST_B2_LENTO")))) : 1.0f;
  return l;
}
void DibHud(ImDrawList* dl, const S::Area& a, float t) {
  if (g_f.empty()) return;
  if (g_d.hud_ama.size() > 89 && g_thud.size() > 23) {
    const Lienzo L = LienzoDe(a);
    const float lento = Lento();
    if (t - g_hud_visto > 0.4f) {   // el tablero empieza o vuelve de un combate: todo entra (el marcador escondido sigue asi)
      g_rombo_t = t;
      if (!g_hud_oculto) g_hud_t = g_radar_t = t;
    }
    g_hud_visto = t;
    if (g_radar && !g_radar_visto) g_radar_t = t;   // recien cogido el Radar del Dragon
    g_radar_visto = g_radar;
    const float ch = (t - g_hud_t) * 60.0f / lento, cr = (t - g_radar_t) * 60.0f / lento;
    if (!g_hud_oculto || ch < 15) {
      // barra y titulo: entrando, en reposo o saliendo; las Bolas se mueven con la barra (posicion de ahora - reposo)
      const int ab = g_hud_oculto ? 2 : ch < 20 ? 0 : 1;
      const float cb = ab == 1 ? -1.0f : ch;
      DibHudSp(dl, ab, L, cb);
      DibHudSp(dl, ab + 3, L, cb);
      const float bx = PoseDe(g_d.hud_ama[size_t(ab)], cb).px - g_d.hud_ama[1].px;
      for (int n = 0; n < 7; ++n) {
        if (!(g_bolas >> n & 1)) continue;
        const int k = n == 0 ? 8 : 9 + n;
        const float c = (t - g_bola_t) * 60.0f;
        if (n == g_bola_n && c < 20) {   // entra desde la izquierda (claves de la 7, llevadas a su hueco)
          const Datos::Sprite& e = g_d.hud_ama[7];
          DibHudSp(dl, k, L, -1, IM_COL32_WHITE, bx + PoseDe(e, c).px - e.px, 0);
          continue;
        }
        DibHudSp(dl, k, L, -1, IM_COL32_WHITE, bx, 0);
      }
      if (g_radar && g_sel < int(g_f.size())) {
        const float rc = g_hud_oculto ? ch : cr;   // 18 sale, 16 entra
        if (g_hud_oculto || cr < 20) {
          DibHudSp(dl, g_hud_oculto ? 18 : 16, L, rc);
        } else {
          DibHudSp(dl, 17, L);
          DibHudSp(dl, 89, L, std::fmod(t * 60.0f, 120.0f));
          const Datos::Sprite& centro = g_d.hud_ama[89];
          const Casilla& yo = g_d.casillas[size_t(g_f[size_t(g_sel)].casilla)];
          for (size_t i = 0; i < g_bolas_fase.size() && i < 3; ++i) {
            const Casilla& b = g_d.casillas[size_t(g_bolas_fase[i][0])];
            const float dx = b.x - yo.x, dy = b.y - yo.y;
            if (dx * dx + dy * dy > 90000.0f)
              DibHudSp(dl, 87, L, -1, IM_COL32_WHITE, centro.px - g_d.hud_ama[87].px, centro.py - g_d.hud_ama[87].py,
                       std::atan2(dx, -dy));
            else
              DibHudSp(dl, 88, L, -1, IM_COL32_WHITE, centro.px - g_d.hud_ama[88].px + dx / 10, centro.py - g_d.hud_ama[88].py + dy / 10);
          }
        }
      }
    }
    const float cd = (t - g_rombo_t) * 60.0f / lento;
    DibHudSp(dl, cd < 10 ? 38 : 39, L, cd < 10 ? cd : -1.0f);
    const int n = std::clamp(g_turno, 0, 99);
    auto cifra = [&](int d, int sitio) {
      const Datos::Sprite &s = g_d.hud_ama[size_t(41 + d)], &p = g_d.hud_ama[size_t(sitio)];
      DibHudSp(dl, 41 + d, L, -1, IM_COL32_WHITE, p.px - s.px, p.py - s.py);
    };
    if (n < 10) cifra(n, 41);
    else {
      cifra(n % 10, 42);
      cifra(n / 10, 43);
    }
    return;
  }
  const ImVec2 c = a.At(1110, 84);
  const float r = 54 * a.s;
  const ImVec2 q[4] = {ImVec2(c.x, c.y - r), ImVec2(c.x + r, c.y), ImVec2(c.x, c.y + r), ImVec2(c.x - r, c.y)};
  dl->AddQuadFilled(q[0], q[1], q[2], q[3], IM_COL32(238, 228, 206, 245));
  dl->AddQuad(q[0], q[1], q[2], q[3], IM_COL32(160, 36, 30, 255), 9 * a.s);
  dl->AddQuad(q[0], q[1], q[2], q[3], IM_COL32(90, 16, 12, 255), 2 * a.s);
  CifrasAzules(dl, g_turno, c, 46 * a.s);
  dl->AddRectFilled(a.At(70, 800), a.At(560, 858), IM_COL32(150, 96, 48, 240), 26 * a.s);
  dl->AddRect(a.At(70, 800), a.At(560, 858), IM_COL32(70, 40, 16, 255), 26 * a.s, 0, 4 * a.s);
  for (int b = 0; b < 7; ++b) {
    const bool tiene = (g_bolas >> b) & 1;
    const float x = 175 + 52.0f * float(b);
    dl->AddCircleFilled(a.At(x, 829), 21 * a.s, IM_COL32(90, 56, 26, 255));
    if (tiene) DibIcono(dl, 2, a.At(x - 21, 808), a.At(x + 21, 850), ImVec4(40.0f * float(b), 0, 40.0f * float(b + 1), 40));
  }
  DibIcono(dl, 26, a.At(16, 720), a.At(116, 860));
  S::MenuText(dl, a.At(320, 880), 30 * a.s, T::T("MUNDO DRAGÓN", "DRAGON WORLD"), true, false);
}

void DibAviso(ImDrawList* dl, const S::Area& a, float t) {
  if (g_msg.empty() || t - g_msg_t > 4.0f) return;
  dl->AddRectFilled(a.At(250, 702), a.At(950, 770), IM_COL32(10, 24, 70, 215), 12 * a.s);
  dl->AddRect(a.At(250, 702), a.At(950, 770), IM_COL32(120, 170, 255, 220), 12 * a.s, 0, 2 * a.s);
  float x = 600;
  if (g_msg_icono >= 0) {
    if (g_msg_icono == 2) {
      const int n = g_d.fase % 7;
      DibIcono(dl, 2, a.At(266, 714), a.At(310, 758), ImVec4(40.0f * float(n), 0, 40.0f * float(n + 1), 40));
    } else {
      DibIcono(dl, g_msg_icono, a.At(266, 714), a.At(310, 758));
    }
    x = 620;
  }
  S::MenuText(dl, a.At(x, 736), 30 * a.s, g_msg.c_str(), true, false, 600 * a.s);
}

}  // namespace

// ---- API (pasarela_misiones.cpp, con su g_mu) ----
bool Disponible() {
  // una vez encontrada, se queda; mientras falte se vuelve a mirar cada 2 s (el jugador la puede elegir con el juego
  // abierto: ElegirIsoPs2)
  static std::atomic<bool> hay{false};
  static std::atomic<int64_t> visto{-1000000};
  if (hay) return true;
  const int64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::steady_clock::now().time_since_epoch()).count();
  if (ms - visto.load() < 2000) return false;
  visto = ms;
  hay = !BuscarJuego("budokai 2").empty();
  return hay;
}

void Entrar() {
  g_bolas = LeerProgreso("tablero_b2_bolas", 0);
  const int fase = std::clamp(LeerProgreso("tablero_b2_fase", 0), 0, kFases - 1);
  if (g_luchando) {   // se salio desde la tarjeta del combate
    g_luchando = false;
    g_pend_j = g_pend_r = -1;
  }
  if (g_estado == kJuego && g_d.fase == fase) return;   // sigue la partida de esta sesion
  if (Cargar(fase)) Preparar();
  if (std::getenv("DBZ3_TEST_B2_DESEO")) {   // PRUEBA: la pantalla de los deseos sin jugar las 9 fases
    g_estado = kDeseo;
    g_deseo = 0;
    g_shen = 0;
  }
}

void Resultado(int ganador, int vida) {
  g_luchando = false;
  const int pj = g_pend_j, pr = g_pend_r;
  g_pend_j = g_pend_r = -1;
  if (pj >= 0 && pr >= 0 && ganador >= 0) SucesosDeCombate(ganador == 0);
  if (pj < 0 || pr < 0 || ganador < 0) return FinDeAccion();
  Ficha &j = g_f[size_t(pj)], &r = g_f[size_t(pr)];
  REXLOG_INFO("dbz3 tablero b2: combate {} contra {}: gana {}, vida {}", Nombre(j), Nombre(r), ganador == 0 ? "el jugador" : "el rival", vida);
  if (ganador == 0) {
    j.vida = vida > 0 && vida < j.vida_max ? vida : 0;
    // FAQ: si estaba aturdido, el golpe quita 2 barras. ponytail: perder aturde al rival hasta el siguiente turno rival
    r.barras -= r.aturdido > 0 ? 2 : 1;
    r.aturdido = 1;
    if (r.barras <= 0) {
      r.fuera = true;
      if (pr == g_jefe) {
        g_estado = kGanada;
        GuardarProgreso("tablero_b2_fase", std::min(g_d.fase + 1, kFases - 1));
        REXLOG_INFO("dbz3 tablero b2: fase {} completada (bolas {:#x})", g_d.fase + 1, g_bolas);
        return Aviso(Nombre(r) + T::T(" ha caído. ¡Fase completada!", " is down. Stage complete!"));
      }
      Aviso(Nombre(r) + T::T(" ha caído.", " is down."));
    } else {
      Aviso(Nombre(r) + T::T(" pierde una barra y queda aturdido.", " loses a bar and is stunned."), 4);
    }
  } else {   // ponytail (sin confirmar en la FAQ): perder quita una barra al tuyo, que vuelve con la vida llena
    j.vida = 0;
    if (--j.barras <= 0) {
      j.fuera = true;
      bool queda = false;
      for (const Ficha& f : g_f) queda |= Activa(f) && f.jugador && f.c == kGoku;
      if (!queda) {
        g_estado = kPerdida;
        REXLOG_INFO("dbz3 tablero b2: fase {} perdida", g_d.fase + 1);
        return Aviso(T::T("Goku ha caído.", "Goku is down."));
      }
      Aviso(Nombre(j) + T::T(" queda fuera del tablero.", " is out of the board."));
    } else {
      Aviso(Nombre(j) + T::T(" pierde una barra.", " loses a bar."), 15);
    }
  }
  if (g_sel >= 0 && !Activa(g_f[size_t(g_sel)])) SiguienteSel();
  FinDeAccion();
}

// 0 nada, 1 salir, 2 combate: out = {jugador, rival (IDs de B3), mapa de B3, vida que le queda al jugador, vida del
// rival (< 0 = solo la inicial), condicion de la casilla (1-5, ver T1)}.
int Dibujar(ImDrawList* dl, const S::Area& a, float t, uint16_t pressed, bool auto_on, int out[8]) {
  g_ahora = t;
  if (g_d.fase < 0) {
    S::MenuText(dl, a.At(600, 400), 40 * a.s, T::T("No se pudo leer el tablero de tu Budokai 2.",
                                                   "Couldn't read the board from your Budokai 2."), true, false);
    S::MenuText(dl, a.At(600, 470), 30 * a.s, T::T("Comprueba que has elegido bien la ISO de Budokai 2.",
                                                   "Check that you picked the right Budokai 2 ISO."), false, true, 1000 * a.s);
    S::HelpLine(dl, a.At(600, 878), 34 * a.s, {{"B", T::T("Salir", "Quit")}});
    return (pressed & kB) ? 1 : 0;
  }
  Texturas();
  if (auto_on && t - g_auto_t > 1.4f) {
    g_auto_t = t;
    if (g_estado == kGanada || g_estado == kPerdida) pressed |= kA;
    else Auto();
  }
  const bool prepara = g_estado == kPrepara;
  DibTablero(dl, t, prepara);
  if (!prepara && !g_mensajes.empty()) {   // ventana de mensaje del script, en la pantalla de 640x448 del original
    if (g_mensaje_t < 0) g_mensaje_t = t;
    const Lienzo L = LienzoDe(a);
    // entrada del original: las claves de la lista A a 60 cuadros por segundo (posicion lineal entre claves).
    // D_CLEAR (ELF 0x26F280 -> 0x26EFC0 -> 0x26ED50): primero las 8 piezas de la ventana del CMN 402 (entrar = 0x33+3k:
    // "Borrar cond.", que tapa la nube, la nube de la cabecera, los 4 lados y el fondo azul; la 8.a, sin textura, oscurece
    // la pantalla: ponytail, aqui un velo negro al 40 %) y encima los dos textos de la fase (343+5k con 146+k).
    const float c = (t - g_mensaje_t) * 60.0f;
    if (g_d.hud_ama.size() > 74) {
      dl->AddRectFilled(ImVec2(0, 0), ImGui::GetIO().DisplaySize, IM_COL32(0, 0, 0, int(100 * std::min(1.0f, c / 20.0f))));
      for (int p = 0; p < 7; ++p) DibHudSp(dl, 0x33 + 3 * p, L, c);
    }
    for (int k : g_mensajes.front()) DibSp(dl, g_d.textos, g_tloc, k, L, c);
    if ((pressed & kA) || t - g_mensaje_t > 3.0f) {
      g_mensajes.erase(g_mensajes.begin());
      g_mensaje_t = -10;
      pressed &= uint16_t(~kA);
    }
  }
  if (prepara) {
    // eleccion de fase (arriba/abajo) y de companero (izquierda/derecha) con las fichas del original
    dl->AddRectFilled(a.At(90, 200), a.At(1110, 660), IM_COL32(8, 18, 54, 225), 16 * a.s);
    dl->AddRect(a.At(90, 200), a.At(1110, 660), IM_COL32(120, 170, 255, 230), 16 * a.s, 0, 3 * a.s);
    // aviso de que el modo aun se esta haciendo
    S::MenuText(dl, a.At(600, 170), 30 * a.s, T::T("EXPERIMENTAL: en construcción", "EXPERIMENTAL: work in progress"), true, false,
                1000 * a.s);
    auto img = [&](int j, float cy, float esc) {
      if (j < 0 || j >= int(g_tloc.size()) || !g_tloc[size_t(j)]) return false;
      const Tex& x = g_tloc[size_t(j)];
      dl->AddImage(x.id(), a.At(600 - x.w * esc * 0.5f, cy - x.h * esc * 0.5f), a.At(600 + x.w * esc * 0.5f, cy + x.h * esc * 0.5f));
      return true;
    };
    if (!img(g_d.stage, 248, 1.2f)) S::MenuText(dl, a.At(600, 248), 44 * a.s, kSagas[g_d.fase](), true, false, 900 * a.s);
    img(g_d.objetivo, 338, 1.0f);
    S::MenuText(dl, a.At(600, 410), 38 * a.s, T::T("Elige al compañero de Goku", "Pick Goku's partner"), true, false);
    const int n = int(g_cand.size());
    for (int i = 0; i < n; ++i) {
      const float w = i == g_comp ? 118.0f : 70.0f, x = 600 + (float(i) - float(g_comp)) * 76.0f +
                                                         (i > g_comp ? 26.0f : i < g_comp ? -26.0f : 0.0f);
      if (x < 170 || x > 1030) continue;   // margen con el borde del panel (90..1110)
      DibFicha(dl, g_d.piezas[size_t(g_cand[size_t(i)])], a.At(x, i == g_comp ? 565.0f : 540.0f), w * a.s, t,
               i == g_comp ? IM_COL32_WHITE : IM_COL32(150, 150, 170, 230));
    }
    if (n > 0) S::MenuText(dl, a.At(600, 618), 36 * a.s, Nombre(g_d.piezas[size_t(g_cand[size_t(g_comp)])]).c_str(), true, false);
    S::HelpLine(dl, a.At(600, 878), 34 * a.s, {{"LB", ""}, {"RB", T::T("Compañero", "Partner")}, {"A", T::T("Empezar", "Start")},
                                               {"Y", T::T("Salir", "Quit")}});
    if (pressed & kY) return 1;
    if (n > 0 && (pressed & (kLeft | kLB))) g_comp = (g_comp + n - 1) % n;
    if (n > 0 && (pressed & (kRight | kRB))) g_comp = (g_comp + 1) % n;
    // Fases en orden, como B2: el Mundo Dragon carga la fase de su partida guardada (ELF 0x2350C0 con 0x6992DA, copiado
    // de la partida 0x693A96 y limitado a 0-8) y no tiene pantalla para elegir otra. Antes, arriba/abajo saltaban a
    // cualquiera de las 9; ya no. Para volver a la I: menu del tablero > "¿Reiniciar Mundo Dragon?".
    if (pressed & (kA | kStart)) Empezar();
    return 0;
  }
  DibHud(dl, a, t);
  DibAviso(dl, a, t);
  if (g_estado == kGanada || g_estado == kPerdida) {
    const bool gana = g_estado == kGanada;
    dl->AddRectFilled(ImVec2(0, 0), ImGui::GetIO().DisplaySize, IM_COL32(0, 0, 0, 120));
    if (!gana && g_tgo) {   // "GAME OVER" del original (404 #0)
      const float w = 860, h = w * g_tgo.h / g_tgo.w;
      dl->AddImage(g_tgo.id(), a.At(600 - w * 0.5f, 380 - h * 0.5f), a.At(600 + w * 0.5f, 380 + h * 0.5f));
    } else {
      S::MenuText(dl, a.At(600, 380), 64 * a.s, gana ? T::T("¡Fase completada!", "Stage complete!") : "GAME OVER", true, false);
    }
    if (gana) {   // las Bolas conseguidas en esta fase
      float x = 560;
      for (int n = 0; n < 7; ++n) {
        if (!((g_bolas & ~g_bolas_ini) >> n & 1)) continue;
        DibIcono(dl, 2, a.At(x, 440), a.At(x + 80, 520), ImVec4(40.0f * float(n), 0, 40.0f * float(n + 1), 40));
        x += 90;
      }
    }
    S::HelpLine(dl, a.At(600, 878), 34 * a.s, {{"A", gana ? T::T("Siguiente fase", "Next stage") : T::T("Reintentar", "Retry")},
                                               {"Y", T::T("Salir", "Quit")}});
    if (pressed & kY) return 1;
    if (pressed & (kA | kStart)) {
      if (gana && g_d.fase == kFases - 1 && (g_bolas & 0x7F) == 0x7F) {   // las 7 Bolas tras la fase IX: Shenron
        g_estado = kDeseo;
        g_deseo = 0;
        g_shen = 0;
        REXLOG_INFO("dbz3 tablero b2: Shenron: {} deseos", g_tdeseos.size());
        return 0;
      }
      const int f = gana ? std::min(g_d.fase + 1, kFases - 1) : g_d.fase;
      if (Cargar(f)) Preparar();
    }
    return 0;
  }
  if (g_estado == kDeseo) {   // Shenron (dw_shenglong.c): su ventana (CMN 850 en el sitio del #AMA 849) con sus frases
    // (537) y la lista de deseos en las 3 filas del #AMA (284, 320, 356), delante de Shenron en 3D (modelo de B2, CMN 327).
    // ponytail: sin sus efectos #SPX (329/331/332) ni el modelo del sitio (328 hijo 0): de fondo, sus nubes en 2D.
    if (!g_shen_cargado) {   // una vez: Shenron (CMN 327: #AMO 0 + #AMT 1) con su animacion (CMN 328 hijo 3, #AMM de 8
      // huesos: 290, 240 y 400 cuadros; la 2, la de 400, en bucle) y el cielo de la escena del deseo (CMN 328 hijo 1, #AMT
      // del sitio: nubes y suelo)
      g_shen_cargado = true;
      AfsPs2 cmn;
      if (cmn.Abrir(BuscarJuego("budokai 2"), "DATA_CMN.AFS")) {
        const std::vector<uint8_t> a327 = cmn.Entrada(327), a328 = cmn.Entrada(328);
        g_shen_mod = ps2::CargarSuelto(HijoAmb(a327, 0), HijoAmb(a327, 1), true);
        // Las claves de B2 van sobre el eje de reposo (Anim::relativa; sustituyendo, el cuerpo quedaba en columna: medido
        // 2026-10-16, RONDA4_PULIDO R2 octava vuelta). La 2 (400 cuadros) en bucle. PRUEBA: DBZ3_TEST_SHEN_ANIM=N (-1 quieto)
        const int na = std::getenv("DBZ3_TEST_SHEN_ANIM") ? std::atoi(std::getenv("DBZ3_TEST_SHEN_ANIM")) : 2;
        if (g_shen_mod.esq && na >= 0) {
          g_shen_mod.anim = ps2::LeerAnim(HijoAmb(a328, 3), uint32_t(na), g_shen_mod.esq->padre.size());
          g_shen_mod.anim.relativa = true;
        }
        if (g_shen_mod.esq) {   // instrumento: listas de pesos sin peso (con Posar iban al origen) y huesos fuera de rango
          const auto& e = *g_shen_mod.esq;
          size_t sin_peso = 0, fuera = 0, verts = 0, usados = 0;
          for (const auto& l : e.pesos) {
            float sw = 0;
            for (const auto& q : l) {
              sw += q.w;
              fuera += q.hueso >= e.padre.size();
            }
            sin_peso += sw <= 1e-6f;
          }
          for (int32_t lp : e.pesos_de) {
            ++verts;
            usados += lp >= 0 && e.pesos[size_t(lp)].size() > 0;
          }
          REXLOG_INFO("dbz3 tablero b2: Shenron 3D: {} listas de pesos ({} sin peso, {} pesos con hueso fuera), {} de {} "
                      "vertices pesados", e.pesos.size(), sin_peso, fuera, usados, verts);
        }
        g_shen_tx = ps2::Texturas(g_shen_mod);
        const std::vector<uint8_t> sitio = HijoAmb(a328, 1);
        if (CuantasAmt(sitio) > 0) g_shen_nubes = ps2::Crear(TexturaAmt(sitio, 0));
        // el sitio sin centrar y movido como Shenron (Posar le quita a el su centro x/z): quedan en el mismo sitio que en B2
        if (ps2::LeerAmo(HijoAmb(a328, 0), 0, g_shen_sitio.malla, nullptr, true) && g_shen_mod.esq) {   // con sus colores
          for (auto& tr : g_shen_sitio.malla.tris)
            for (auto& v : tr.p) v = {v.x - g_shen_mod.esq->centro.x, v.y, v.z - g_shen_mod.esq->centro.z};
          for (int j = 0, n = CuantasAmt(sitio); j < n; ++j) g_shen_sitio.tex.push_back(TexturaAmt(sitio, j));
          g_shen_sitio_tx = ps2::Texturas(g_shen_sitio);
        }
      }
      REXLOG_INFO("dbz3 tablero b2: Shenron 3D: {} triangulos, {} texturas, animacion de {} cuadros, nubes {}, sitio {} "
                  "triangulos", g_shen_mod.malla.tris.size(), g_shen_tx.size(), g_shen_mod.anim.cuadros, bool(g_shen_nubes),
                  g_shen_sitio.malla.tris.size());
    }
    if (g_shen_nubes) {   // cielo oscuro con las nubes del original, repetidas por toda la ventana
      const ImVec2 ds = ImGui::GetIO().DisplaySize;
      const float lado = 256 * a.s * 1.6f, desp = std::fmod(t * 6.0f, 1.0f);
      dl->AddRectFilled(ImVec2(0, 0), ds, IM_COL32(4, 6, 18, 255));
      dl->AddImage(g_shen_nubes.id(), ImVec2(0, 0), ds, ImVec2(desp, 0), ImVec2(desp + ds.x / lado, ds.y / lado),
                   IM_COL32(70, 80, 120, 255));
    }
    if (g_shen_mod.malla) {   // encuadre (una vez): del 2 % al 98 % de los vertices; Shenron llena la escena por encima de la
      // ventana (alto ~ 520 de los 900 del area), sin salirse
      static float cy = 0, dist = 0;
      if (dist <= 0) {
        std::vector<float> ys, xs;
        ps2::Malla p0;
        if (g_shen_mod.esq && g_shen_mod.anim) ps2::Posar(*g_shen_mod.esq, g_shen_mod.anim, 0.0f, p0);
        for (const auto& tr : (p0 ? p0 : g_shen_mod.malla).tris)
          for (const auto& v : tr.p) {
            ys.push_back(v.y);
            xs.push_back(std::max(std::abs(v.x), std::abs(v.z)));
          }
        auto q = [](std::vector<float>& v, float f) {
          const size_t k = size_t(f * float(v.size() - 1));
          std::nth_element(v.begin(), v.begin() + std::ptrdiff_t(k), v.end());
          return v[k];
        };
        const float y0 = q(ys, 0.02f), y1 = q(ys, 0.98f), xr = q(xs, 0.98f), tg = std::tan(0.785f * 0.5f);
        cy = (y0 + y1) * 0.5f;
        // alto (y1 - y0) en 480 px del area de 900 (f = 450 / tg) y ancho (2 xr) en 1000 px
        dist = std::max((y1 - y0) * 450.0f / (480.0f * tg), 2.0f * xr * 450.0f / (1000.0f * tg));
        REXLOG_INFO("dbz3 tablero b2: Shenron 3D: alto {:.1f}..{:.1f}, radio {:.1f}, distancia {:.1f}", y0, y1, xr, dist);
      }
      ps2::Camara cam;
      cam.fov = 0.785f;
      cam.c = ImVec2(a.p0.x + 600 * a.s, a.p0.y + 260 * a.s);   // Shenron de y 20 a 500 del area, sobre la ventana
      cam.h = ImVec2(600 * a.s, 450 * a.s);
      cam.m[0] = cam.m[5] = cam.m[10] = cam.m[15] = 1;
      cam.m[13] = -cy;
      cam.m[14] = -dist;
      cam.ok = true;
      static ps2::Malla pose;
      const bool anima = g_shen_mod.esq && g_shen_mod.anim;
      if (anima) ps2::Posar(*g_shen_mod.esq, g_shen_mod.anim, t, pose);
      ps2::Escena esc;
      esc.Empezar();
      if (g_shen_sitio.malla) esc.Poner(cam, g_shen_sitio.malla, g_shen_sitio_tx, {0, 0, 0}, 0.0f, 1.0f);
      // sin toon: con el toon de P4 (Toon{}, borde 0.15) Shenron animado salia roto (triangulos enormes por toda la
      // pantalla; captura D:/DBZ3HD/_pruebas/20261016-074244_escenarios_test3_r2_deseo). Avisado a P4 en RONDA4_PULIDO.
      // PRUEBA (P4): DBZ3_TEST_SHEN_TOON=1 lo pinta con el toon de ps2_modelo (normales basura de B2 -> sin normal)
      static const char* toon_prueba = std::getenv("DBZ3_TEST_SHEN_TOON");   // "2": sin borde
      ps2::Escena::Toon toon_p;
      if (toon_prueba && toon_prueba[0] == '2') toon_p.borde = 0;
      if (toon_prueba)
        esc.Poner(cam, anima ? pose : g_shen_mod.malla, g_shen_tx, {0, 0, 0}, 0.0f, 1.0f, toon_p);
      else
        esc.Poner(cam, anima ? pose : g_shen_mod.malla, g_shen_tx, {0, 0, 0}, 0.0f, 1.0f);
      esc.Pintar(dl);
    }
    const float s = std::min(1200.0f / 640.0f, 900.0f / 448.0f) * a.s;
    const ImVec2 o0(a.p0.x + (1200.0f * a.s - 640.0f * s) * 0.5f, a.p0.y + (900.0f * a.s - 448.0f * s) * 0.5f);
    auto P = [&](float x, float y) { return ImVec2(o0.x + x * s, o0.y + y * s); };
    if (!g_d.shen_ama.empty() && !g_tshen_marco.empty() && g_tshen_marco[0]) {
      const Datos::Sprite& m = g_d.shen_ama[0];
      dl->AddImage(g_tshen_marco[0].id(), P(m.px + m.x0, m.py + m.y0 * 1.1f), P(m.px + m.x1, m.py + m.y1 * 1.1f));
    }
    auto texto = [&](const Tex& x, float cy, float alto, ImU32 col) {   // imagen centrada en x 320, como mucho 344 de ancho
      const float esc = std::min(alto / x.h, 344.0f / x.w);
      dl->AddImage(x.id(), P(320 - x.w * esc * 0.5f, cy - x.h * esc * 0.5f), P(320 + x.w * esc * 0.5f, cy + x.h * esc * 0.5f),
                   ImVec2(0, 0), ImVec2(1, 1), col);
    };
    const int n = int(g_tdeseos.size());
    if (g_shen == 1 && n > 0) {
      if (pressed & kUp) g_deseo = (g_deseo + n - 1) % n;
      if (pressed & kDown) g_deseo = (g_deseo + 1) % n;
      const float filas[3] = {284, 320, 356};
      for (int k = -1; k <= 1; ++k) {
        const int i = (g_deseo + k + n) % n;
        if (g_tdeseos[size_t(i)]) texto(g_tdeseos[size_t(i)], filas[k + 1], 26, k == 0 ? IM_COL32(255, 230, 90, 255) : IM_COL32(170, 170, 190, 220));
      }
    } else {
      const int f = g_shen == 0 ? 0 : g_shen == 2 ? 1 : 5;
      if (f < int(g_tshen_frases.size()) && g_tshen_frases[size_t(f)]) texto(g_tshen_frases[size_t(f)], 320, 100, IM_COL32_WHITE);
    }
    S::HelpLine(dl, a.At(600, 878), 38 * a.s, {{"A", g_shen == 1 ? T::T("Pedir", "Wish") : T::T("Seguir", "Continue")},
                                               {"Y", T::T("Salir", "Quit")}});
    if (auto_on) pressed |= kA;
    if (pressed & kY) return 1;
    if (pressed & (kA | kStart)) {
      if (g_shen == 1) {
        if (g_deseo == 0) {
          GuardarProgreso("tablero_b2_nave", 1);
          REXLOG_INFO("dbz3 tablero b2: deseo: portal de la nave de Babidi (Nave abierta)");
        } else if (DeseoB3(g_deseo)) {
          DarCapsulaB3(DeseoB3(g_deseo));
        } else {   // ponytail: sin equivalente en B3 (lista arriba); ahora el jugador lo ve
          REXLOG_INFO("dbz3 tablero b2: deseo {} sin equivalente en B3", g_deseo);
          Aviso(T::T("Este deseo no tiene equivalente en Budokai 3.", "This wish has no equivalent in Budokai 3."));
        }
      }
      if (g_shen < 3) {
        ++g_shen;
        return 0;
      }
      g_shen = 0;
      g_bolas = 0;   // las Bolas se dispersan y se vuelve a la fase I
      GuardarProgreso("tablero_b2_bolas", 0);
      GuardarProgreso("tablero_b2_fase", 0);
      if (Cargar(0)) Preparar();
    }
    return 0;
  }

  // combate pendiente: el "!!" se ve un momento y se pide el combate
  if (g_pend_j >= 0 && !g_luchando) {
    if (t - g_pend_t < 1.0f) return 0;
    const Ficha &j = g_f[size_t(g_pend_j)], &r = g_f[size_t(g_pend_r)];
    out[0] = B2aB3(j.c);
    out[1] = B2aB3(r.c);
    out[2] = kMapaB3[g_d.fase];
    out[3] = j.vida;
    out[4] = r.vida_max != 3000 ? r.vida_max : 0;
    out[5] = g_d.casillas[size_t(r.casilla)].condicion;
    out[6] = j.ataque;
    out[7] = j.defensa;
    // PRUEBA: DBZ3_TEST_B2_CONDICION=N fuerza la condicion N (1-5) en todos los combates del tablero
    static const int forzada = std::getenv("DBZ3_TEST_B2_CONDICION") ? std::atoi(std::getenv("DBZ3_TEST_B2_CONDICION")) : 0;
    if (forzada > 0) out[5] = forzada;
    if (out[5] == 1) {   // empiezan con media vida (la maxima, la de siempre)
      out[3] = std::min(j.vida > 0 ? j.vida : j.vida_max, j.vida_max / 2);
      out[4] = -(r.vida_max / 2);
    }
    g_luchando = true;
    REXLOG_INFO("dbz3 tablero b2: combate: {} (id {}) contra {} (id {}), turno {}, condicion {}", Nombre(j), out[0],
                Nombre(r), out[1], g_turno, out[5]);
    return 2;
  }
  if (g_pend_j >= 0) return 0;
  if (g_menu > 0) {   // menu del tablero (DWU_MMAIN, ELF 0x2A35F0) y su Si/No (DWU_YN*, 0x2A2C90), piezas del CMN 402
    const Lienzo L = LienzoDe(a);
    const float c = (t - g_menu_t) * 60.0f / Lento();
    const ImU32 apagado = IM_COL32(110, 110, 130, 255);
    dl->AddRectFilled(ImVec2(0, 0), ImGui::GetIO().DisplaySize, IM_COL32(0, 0, 0, 90));
    if (g_menu == 1 && c < 10) {   // la caja se abre (97: su alto de 0 a 0,8 en 10 cuadros)
      DibHudSp(dl, 97, L, c);
    } else if (g_menu == 2 && c < 10) {   // y la del Si/No (110: su ancho de 0 a 1)
      DibHudSp(dl, 110, L, c);
    } else if (g_menu == 1) {   // caja (98) con "Salir" (106) y "¿Reiniciar Mundo Dragon?" (107)
      DibHudSp(dl, 98, L);
      for (int i = 0; i < 2; ++i) DibHudSp(dl, 106 + i, L, -1, i == g_menu_sel ? IM_COL32_WHITE : apagado);
    } else {   // caja (111), pregunta (113 + mensaje: salir / empezar de nuevo), "Si" (116), "No" (117) y la flecha (118)
      DibHudSp(dl, 111, L);
      DibHudSp(dl, 113 + g_menu_sel, L);
      for (int i = 0; i < 2; ++i) DibHudSp(dl, 116 + i, L, -1, i == g_yn_sel ? IM_COL32_WHITE : apagado);
      if (g_d.hud_ama.size() > 118) {   // ponytail: la flecha a la izquierda de la opcion (su ancla exacta, sin leer)
        const Datos::Sprite &op = g_d.hud_ama[size_t(116 + g_yn_sel)], &fl = g_d.hud_ama[118];
        DibHudSp(dl, 118, L, std::fmod(c, 15.0f), IM_COL32_WHITE, op.px + op.x0 - 18 - fl.px, op.py - fl.py);
      }
    }
    S::HelpLine(dl, a.At(600, 878), 38 * a.s, {{"A", T::T("Aceptar", "OK")}, {"Y", T::T("Volver", "Back")}});
    if (pressed & (kUp | kDown)) {
      (g_menu == 1 ? g_menu_sel : g_yn_sel) ^= 1;
    } else if (pressed & kA) {
      if (g_menu == 1) {
        g_menu = 2;
        g_yn_sel = 1;   // el original empieza en "No" (+0x44 = 1)
        g_menu_t = t;
      } else if (g_yn_sel == 1) {
        g_menu = 1;
      } else if (g_menu_sel == 0) {
        REXLOG_INFO("dbz3 tablero b2: menu: salir del Mundo Dragon");
        g_menu = 0;
        return 1;
      } else {   // empezar de nuevo: el original borra la partida del Mundo Dragon (0x693A1C, 0x9C bytes)
        REXLOG_INFO("dbz3 tablero b2: menu: reiniciar el Mundo Dragon");
        g_menu = 0;
        g_bolas = 0;
        GuardarProgreso("tablero_b2_bolas", 0);
        GuardarProgreso("tablero_b2_fase", 0);
        if (Cargar(0)) Preparar();
      }
    } else if (pressed & (kY | kStart | kB)) {
      g_menu = g_menu == 2 ? 1 : 0;
    }
    return 0;
  }
  // botones de B2 (ELF, control del tablero): cruz mueve, cuadrado busca, circulo (B) pasa, triangulo o Start (Y) abren el
  // menu, L1 esconde o saca el marcador de abajo. RB cambia de ficha (propio).
  S::HelpLine(dl, a.At(870, 878), 38 * a.s, {{"A", T::T("Mover", "Move")}, {"X", T::T("Buscar", "Search")},
                                             {"B", T::T("Pasar", "Pass")}, {"LB", T::T("Radar", "Radar")},
                                             {"RB", T::T("Personaje", "Character")}, {"Y", T::T("Menú", "Menu")}});
  if (pressed & (kY | kStart)) {
    g_menu = 1;
    g_menu_sel = 0;
    g_menu_t = t;
    REXLOG_INFO("dbz3 tablero b2: menu abierto");
    return 0;
  }
  Ficha& f = g_f[size_t(g_sel)];
  if (pressed & kLB) {
    g_hud_oculto = !g_hud_oculto;
    g_hud_t = g_radar_t = t;   // sale o entra con su animacion
  }
  if (pressed & kRB) SiguienteSel();
  if (pressed & (kUp | kDown | kLeft | kRight)) {   // la vecina en esa direccion (en el mapa)
    const float dx = (pressed & kRight) ? 1.0f : (pressed & kLeft) ? -1.0f : 0.0f;
    const float dy = (pressed & kDown) ? 1.0f : (pressed & kUp) ? -1.0f : 0.0f;
    const Casilla& c = g_d.casillas[size_t(f.casilla)];
    float mejor = 0.3f;
    for (int v : c.vec) {
      if (v <= 0) continue;
      const float vx = g_d.casillas[size_t(v)].x - c.x, vy = g_d.casillas[size_t(v)].y - c.y, l = std::hypot(vx, vy);
      const float p = l > 0 ? (vx * dx + vy * dy) / l : 0;
      if (p > mejor) {
        mejor = p;
        g_dest = v;
      }
    }
  }
  if ((pressed & (kA | kStart)) && g_dest > 0 && f.movs > 0) Mover(g_sel, g_dest);
  else if ((pressed & kX) && f.movs > 0) Buscar(g_sel);
  else if (pressed & kB) {
    f.movs = 0;
    FinDeAccion();
  }
  return 0;
}

}  // namespace dbz3::pasarela::b2
