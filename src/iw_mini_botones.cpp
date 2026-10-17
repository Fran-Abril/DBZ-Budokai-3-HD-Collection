// dbz3 - Minijuegos de botones de Infinite World (frente R14, ronda 4, 2026-10-16). Ver docs/14_frentes/RONDA4_PULIDO.md
// ("R14"). Registrados en iw_minijuegos (RegistrarModulo): elegir su casilla del mapa de Dragon Mission los empieza.
//
// RE (ELF USA SLUS_218.42 con E:/DBZ3HD_trabajo/P4/iwdis.py; nombres del decomp EU iw_sles55347.c, codigo EU = USA +
// 0x330 en esta zona, datos EU = USA + 0x7B0). Todos usan la libreria QTE comun (EU 0x2EE6D0-0x2EF290: QTE_SetSequence
// copia N codigos de boton; codigo i = bit (1 << i) del mando de PS2, tabla 0x497E90 USA). Dos familias:
//  A) "Dos carriles" (EU QteMg_* 0x32E8D0-0x333080 = MSN_NB014; clones en NB013 0x3513D0 y NB006 0x376F00 USA):
//     - Por dificultad 0..5 dos secuencias de 50 (tabla S + d*0x70; carril 1 en +0x32): carril 0 = botones de la
//       derecha (12 triangulo, 13 circulo, 14 equis, 15 cuadrado), carril 1 = gatillos (8 L2, 9 R2, 10 L1, 11 R1).
//     - Separacion entre iconos en cuadros: S + 0x2A0 + d*0x190 + carril*200, 50 ints (QteMg_UpdatePlayerButtons:
//       al bajar a 0 sale el siguiente en x = -16 - velocidad y la cuenta vuelve a cargar la siguiente separacion).
//     - Los iconos van de izquierda a derecha a `velocidad` px por cuadro (pantalla de 640: carril 0 en y 366,
//       carril 1 en y 409): se aceptan pulsaciones con x > W/2 + 50, fallo si pasan de W/2 + 150, se quitan en 495.
//     - Vida 1000 los dos (QteMg_SetCharStatsByLevel). Acierto en el carril 0 = el rival pierde el ataque del jugador;
//       fallo en el carril 1 = el jugador pierde el ataque del rival (QteMg_ApplyHitDamage). Acaba al gastar las dos
//       secuencias o con una vida a 0.
//     - Nota (SkillEdit_ComputeStatPercent) = aciertos / iconos que llegaron a la zona x 100. Rango (QteMg_ClassifyResult
//       con los umbrales de s_MSN_NB0xx_Updata): 95-100 -> 5, 81-94 -> 4, 69-80 -> 3, 56-68 -> 2, 1-55 -> 1; con la vida
//       del jugador a 0 o nota 0 -> 0 (no superado). Puntos (QteMg_FinalizeResult) = tabla[rango] x multiplicador[d]
//       (0.6 0.8 1.0 1.3 1.6 2.0), redondeado a la centena (Math_RoundUp100).
//  B) "Rondas" (NB007 0x292BB0, NB012 0x3124F0 USA: secuencias por ronda tabla + ronda*10, longitud de +0xB8) y MSN019
//     (0x2EAA20-0x2EC6A0, sin QTE_SetSequence): RE en el documento, sin portar aun.
#include "iw_minijuegos.h"

#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/immediate_drawer.h>

#include <imgui.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <random>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "generated/dbz3_init.h"
#include "anim_iw.h"
#include "cine.h"
#include "combate.h"
#include "eu_map.h"
#include "ingame/game_style.h"
#include "launcher/launcher_state.h"
#include "pasarela.h"
#include "pasarela_iw.h"
#include "ps2_modelo.h"

namespace dbz3::hub {
bool RequestBattleEx(int id1, int id2, int stage, bool p2_com, int time_s, int hp1, int hp2, int form1, int form2);
}
namespace dbz3::net {
void InyectarPad(uint32_t usuario, int32_t botones);   // net/netplay_hooks.cpp (-1 = el mando de verdad)
}
namespace dbz3 {
bool GameDimmed();
bool OverlayOpen();
}  // namespace dbz3

namespace dbz3::iwmini {
namespace {

namespace P = dbz3::pasarela;
namespace S = dbz3::ingame::style;

constexpr dbz3::GA kMgrPtr = 0x823D6CF4, kPads = 0x8238B780;   // iw_minijuegos.cpp (P4)
constexpr uint32_t kX0 = 2096, kXPaso = 8240, kVida = 6080, kVidaMax = 6084, kFin = 35569;
// Registro del mando 0 (+12, botones mantenidos) con los bits del mando de PS2, medido con el teclado de pruebas: X del
// mando de Xbox -> 0x8000 cuadrado, Y -> 0x1000 triangulo, A -> 0x4000 equis, B -> 0x2000 circulo, LB -> 0x0400 L1,
// RB -> 0x0800 R1, LT -> 0x0100 L2, RT -> 0x0200 R2: los mismos bits que los codigos de boton de IW (1 << codigo).
constexpr uint32_t kPadBotones = 12;
// Prueba (auto): el boton de Xbox que da cada codigo de IW, para pulsarlo de verdad (net::InyectarPad). L2/R2 son
// gatillos (el mando virtual no los lleva): esos se dan por pulsados dentro del minijuego.
int32_t XboxDe(int boton) {
  switch (boton) {
    case 15: return 0x4000;   // X
    case 12: return 0x8000;   // Y
    case 14: return 0x1000;   // A
    case 13: return 0x2000;   // B
    case 10: return 0x0100;   // LB
    case 11: return 0x0200;   // RB
    default: return 0;
  }
}

// Altura a la que luchan NB013 y NB014: la cadera de sus poses de pelea esta a y = 38-41 (la guardia de B3, a 8): en el aire.
constexpr float kAire = 38;
constexpr float kAlto0 = -6;   // camara al empezar (NB013/NB014): los dos de pie en el suelo, antes de que el salto los suba

// ---- Poses de IW (R14, 2026-10-17) ----
// Los dos luchadores no usan las poses de B3 sino las de IW (anim_iw.cpp CodigoQte: cada uno trae las suyas en su #BSK de
// DATA_CMN). Todo del ELF: QteMg_PlayHitPoses (EU 0x330410; en NB013 Mg_SetClashStepPoses 0x352D40; en NB006
// Mg_SetRandomHitPose 0x378DD0) elige las poses en una tabla de 4 filas (USA 0x4804D0, 0x483000 y 0x485600, iguales: [+0]
// 4/7/10/13, [+4] 6/9/12/15, [+8] 5/8/11/14, [+0xC] 3/6/9/12, [+0x10] 5/8/11/14, [+0x14] 4/7/10/13) con un contador que
// da la vuelta de 0 a 3 (+0x2F4; NB006 lo saca al azar, Memory() & 3) y que sube con cada golpe:
//   carril 0 (el jugador pega): jugador [+0]; rival, si acierta [+0x10] (herido) y si no [+0x14] (esquiva);
//   carril 1 (el rival pega): rival [+0xC]; jugador, si acierta [+8] (esquiva) y si no [+4] (herido).
// Cada numero es un estado de pose (Char_SetPoseState / _P2: tablas USA 0x499820 y 0x499950 en NB014, 0x49A5B0 y 0x499F30
// en NB013, 0x49B370 y 0x49AE00 en NB006) que pone una animacion del #BSK; la tabla de abajo ya trae los codigos.
// Tambien: intro (pose 0), READY (jugador 2 / rival 1), START (jugador 3 / rival 2: la espera de pelea, en bucle),
// resultado (jugador 0x12 / rival 0x11 si gana, 0x13 / 0x12 si no) y final del jugador (0x14 / 0x15). Scripts de lectura
// en E:\DBZ3HD_trabajo\R14\poses.
struct Poses {
  int cmn[2];                           // DATA_CMN del jugador y del rival
  uint16_t ini[2], listo[2], pelea[2];  // pose 0, READY, START
  uint16_t j_ataca[4], j_esquiva[4], j_herido[4];
  uint16_t r_ataca[4], r_herido[4], r_esquiva[4];
  uint16_t gana[2], pierde[2];          // [jugador, rival]
  uint16_t fin_bien, fin_mal;           // el jugador al acabar el resultado
  bool azar;                            // NB006: el contador sale al azar. sep: las poses de IW van en el sitio (raiz < 6 u salvo
                                        // las finales) y en los videos (mosaico 230 s) los dos van a un par de cuerpos. ponytail: sin leer
  float aire, dist, sep;                // altura a la que luchan (la cadera de sus poses), distancia de la camara y entre los dos
  // Camaras de IW (clips del #AMC de DATA_CMN 915, ver "Camaras de IW"; -1 = la propia). READY (dur_intro cuadros, los del
  // clip), juego (plano fijo), resultado (gana / pierde) y final del jugador (rango > 0 / 0). ancla = {x, z del centro del par
  // y eje jugador -> rival} en el mundo de IW (de las raices de sus poses: docs/14_frentes/iw_camaras_minijuegos.md).
  int cam_ini, cam_juego, cam_gana, cam_pierde, cam_fin_bien, cam_fin_mal, dur_intro;
  float ancla[5];   // + separacion de los objetos con la camara de IW (la del par de IW: 11-18 u entre las caderas)
};
constexpr Poses kPosesNB014 = {{922, 920},
                               {0x5C8, 0x5CD}, {0x578, 0x579}, {0x57A, 0x57B},
                               {0x582, 0x585, 0x588, 0x58B}, {0x598, 0x59B, 0x59E, 0x5A1}, {0x597, 0x59A, 0x59D, 0x5A0},
                               {0x596, 0x599, 0x59C, 0x59F}, {0x583, 0x586, 0x589, 0x58C}, {0x584, 0x587, 0x58A, 0x58D},
                               {0x5BE, 0x5BF}, {0x5C0, 0x5C1}, 0x5C9, 0x5CA, false, kAire, 62, 26,
                               0x1A4, 0x1A5, 0x1AE, 0x1AF, 0x4B1, 0x4B2, 230, {1, 0, 1, 0, 17}};   // QteMg_* (0x32E8D0-0x333080 EU)
constexpr Poses kPosesNB013 = {{931, 928},
                               {0x564, 0x569}, {0x578, 0x579}, {0x57A, 0x57B},
                               {0x582, 0x585, 0x588, 0x58B}, {0x598, 0x59B, 0x59E, 0x5A1}, {0x597, 0x59A, 0x59D, 0x5A0},
                               {0x596, 0x599, 0x59C, 0x59F}, {0x583, 0x586, 0x589, 0x58C}, {0x584, 0x587, 0x58A, 0x58D},
                               {0x5BE, 0x5BF}, {0x5C0, 0x5C1}, 0x565, 0x566, false, kAire, 62, 26,
                               0x186, 0x187, 0x190, 0x191, 0x47F, 0x480, 230, {1, 0, 1, 0, 17}};   // Mg_UpdateIntroPhase / Mg_StartRoundSequence
constexpr Poses kPosesNB006 = {{921, 925},
                               {0x2AD, 0x2A8}, {0x258, 0x259}, {0x258, 0x259},
                               {0x262, 0x265, 0x268, 0x26B}, {0x278, 0x27B, 0x27E, 0x281}, {0x277, 0x27A, 0x27D, 0x280},
                               {0x276, 0x279, 0x27C, 0x27F}, {0x263, 0x266, 0x269, 0x26C}, {0x264, 0x267, 0x26A, 0x26D},
                               {0x289, 0x28A}, {0x287, 0x288}, 0x2A9, 0x2AA, true, 14, 50, 15,   // NB006 pelea en el suelo (cadera a y = 4-10)
                               0x96, 0x97, 0x322, 0x323, -1, -1, 100, {-15, -0.5f, 0, 1, 7}};   // Mg_UpdateIntroSequence / EnterResultPhase
// (el final del rango 0 de NB006 es 0x2AA para los dos y el del resto 0x2A9: Mg_StartResultAnim; aqui el jugador)


// Familia A por indice del #DMD (constantes del codigo de cada modulo, ELF USA; tablas leidas de la copia del jugador).
struct Modulo2 {
  int indice, clave;
  const char* nombre;
  uint32_t seq, puntos;          // S (secuencias) y tabla de puntos por rango (+0x20: multiplicadores)
  float velocidad[6];            // +0x35C/+0x360 (QteMg_SetSpeedByLevel)
  int ataque_jugador[6], ataque_rival[6];   // +0x228C / +0x435C (QteMg_SetCharStatsByLevel)
  // Personajes (ID de IW = ID de B3 hasta el 43) y formas del bloque de partida que rellena el modulo al empezar
  // (Mg_AllocContexts: +0x10/+0x12 personajes, +0x14/+0x16 formas) y escenario de IW de su w_Stage_BeginLoad
  // (0 BUDO, 1 MIND & TIME, 4 ROCK; pasarela::MapaIW). NB014 no escribe +0x10: queda el del mapa (Goku).
  int jugador, rival, forma_jugador, forma_rival, escenario_iw;
  const Poses* poses;   // R14: poses de IW de sus dos luchadores
};
constexpr Modulo2 kModulos[] = {
    {5, 0x7D4, "MSN_NB006", 0x484720, 0x485670, {2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.7f}, {15, 15, 10, 10, 5, 5},
     {35, 45, 55, 65, 75, 85}, 2, 11, 0, 0, 4, &kPosesNB006},
    {12, 0x7DB, "MSN_NB013", 0x483060, 0x483F40, {2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.7f}, {20, 20, 15, 15, 10, 10},
     {25, 35, 45, 55, 65, 75}, 7, 8, 1, 0, 1, &kPosesNB013},
    {13, 0x7DC, "MSN_NB014", 0x480530, 0x481170, {1.7f, 2.1f, 2.6f, 3.2f, 3.8f, 4.4f}, {50, 35, 25, 20, 15, 10},
     {15, 25, 35, 45, 55, 65}, 0, 3, 0, 0, 1, &kPosesNB014},
};
constexpr int kUmbral[6][2] = {{0, 0}, {1, 55}, {56, 68}, {69, 80}, {81, 94}, {95, 100}};   // +0x310..+0x338

// Pantalla de IW (640 x 448) y medidas del codigo
constexpr float kW = 640, kH = 448, kCarrilY[2] = {366, 409}, kIcono = 32;
constexpr float kAcepta = kW / 2 + 50, kFalla = kW / 2 + 150, kQuita = 495;
// ponytail: el objetivo (objetos 1 y 2 de la UI del modulo) no esta leido; se pinta entre aceptar y fallar.
constexpr float kObjetivo = (kAcepta + kFalla) / 2;

struct Icono {
  int boton = 0;
  float x = 0;
  bool vivo = false, resuelto = false, contado = false, pedido = false;
};
struct Carril {
  uint8_t seq[50] = {};
  int hueco[50] = {};
  Icono ic[50];
  int n = 50;   // iconos del carril: hasta la primera separacion negativa (-256 en el carril 1 de NB006)
  int sale = 0, actual = 0, cuenta = 0, aciertos = 0, llegaron = 0;
};
struct Juego {
  const Modulo2* m = nullptr;
  int clave = 0, fila = 0, dif = 2;
  Carril c[2];
  float vel = 2, vida[2] = {1000, 1000};
  int fase = 0, t = 0, cuadros = 0, rango = 0, puntos = 100, nota = 0;   // fase 0 espera, 1 READY, 2 juego, 3 fin
  int aviso_t = 0;
  const Poses* poses = nullptr;   // R14
  int k = 0;                      // contador de las filas de poses (QteMg_PlayHitPoses: +0x2F4)
  bool aviso_bien = false, auto_ = false, fallos = false;   // prueba: fallos = deja pasar 1 de cada 5 del carril 1
  float pts[6] = {}, mult[6] = {};
  uint16_t antes = 0, pendiente = 0;
  int32_t inyecta = 0;   // prueba: botones de Xbox que se pulsan ahora
  int inyecta_t = 0;
  P::Imagen ready, start, hit, miss, clear, failed;   // DATA_USA 5/1 t08 t00 t07 t02 t09 t10
  P::Imagen objetivo;                                 // DATA_USA 0/1 t10: aro rojo y amarillo con la flecha
  P::Imagen aro, relleno, barra;                       // DATA_USA 6/0 t00, t01, t15 (HUD de P4)
};

int Azar();   // mas abajo
std::mutex g_mu;
std::shared_ptr<Juego> g_j;
std::atomic<bool> g_corre{false};
std::atomic<uint16_t> g_raw12{0};   // botones (+12) tal como los leyo el juego (MandoBotones)
std::atomic<bool> g_raw_ok{false};
std::atomic<float> g_cam_alto{14}, g_cam_dist{62};
std::atomic<bool> g_cam_reset{true};
std::atomic<float> g_cam_lado{1};   // 1: la camara a un lado; -1: al otro (la pose final, de frente)   // lo que pide el panel a la camara (PasoBotones la acerca poco a poco)

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

// Tablas del ELF de IW del jugador (little endian; direccion = desplazamiento - 0x100000 + 0x80).
bool LeerElf(Juego& j, std::string& error) {
  const std::filesystem::path iw = P::BuscarIW();
  std::ifstream f(iw / "SLUS_218.42", std::ios::binary);
  if (!f) {
    error = "sin SLUS_218.42 de Infinite World (USA)";
    return false;
  }
  std::vector<uint8_t> e((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  auto off = [&](uint32_t a) -> const uint8_t* {
    const size_t o = size_t(a) - 0x100000 + 0x80;
    return o + 0x400 <= e.size() ? e.data() + o : nullptr;
  };
  const Modulo2& m = *j.m;
  for (int l = 0; l < 2; ++l) {
    const uint8_t* s = off(m.seq + uint32_t(j.dif) * 0x70 + uint32_t(l) * 0x32);
    const uint8_t* h = off(m.seq + 0x2A0 + uint32_t(j.dif) * 0x190 + uint32_t(l) * 200);
    if (!s || !h) {
      error = "ELF corto";
      return false;
    }
    for (int i = 0; i < 50; ++i) {
      j.c[l].seq[i] = s[i];
      std::memcpy(&j.c[l].hueco[i], h + 4 * i, 4);
      const bool ok = l == 0 ? (s[i] >= 12 && s[i] <= 15) : (s[i] >= 8 && s[i] <= 11);
      if (j.c[l].hueco[i] < 0 && i > 0 && j.c[l].n == 50) j.c[l].n = i;   // ponytail: fin del carril; IW no la mira
      if (!ok || (j.c[l].hueco[i] < 0 && j.c[l].n == 50) || j.c[l].hueco[i] > 2000) {
        error = "las tablas del ELF no son las de IW USA";
        return false;
      }
    }
  }
  const uint8_t* p = off(m.puntos);
  if (!p) return false;
  for (int k = 0; k < 6; ++k) {
    int32_t v;
    std::memcpy(&v, p + 4 * k, 4);
    j.pts[k] = float(v);
    std::memcpy(&j.mult[k], p + 0x20 + 4 * k, 4);
  }
  return true;
}

// Carga (hilo del mapa): ELF y piezas de la UI de IW (DATA_USA de la copia del jugador).
bool Cargar(Juego& j, std::string& error) {
  if (!LeerElf(j, error)) return false;
  P::AfsPs2 usa;
  if (!usa.Abrir(P::BuscarIW(), "DATA_USA.AFS")) {
    error = "sin DATA_USA.AFS";
    return false;
  }
  j.objetivo = P::TexturaAmt(P::HijoAmb(usa.Entrada(0), 1), 10);
  const std::vector<uint8_t> t51 = P::HijoAmb(usa.Entrada(5), 1);
  j.start = P::TexturaAmt(t51, 0);   // "START!"
  j.miss = P::TexturaAmt(t51, 2);    // "MISS!"
  j.hit = P::TexturaAmt(t51, 7);     // "HIT!"
  j.ready = P::TexturaAmt(t51, 8);   // "READY?"
  j.clear = P::TexturaAmt(t51, 9);    // "CLEAR!"
  j.failed = P::TexturaAmt(t51, 10);  // "FAILED!"
  const std::vector<uint8_t> hud = P::HijoAmb(usa.Entrada(6), 0);
  if (P::CuantasAmt(hud) > 38) {
    j.aro = P::TexturaAmt(hud, 0);
    j.relleno = P::TexturaAmt(hud, 1);
    j.barra = P::TexturaAmt(hud, 15);
  }
  return true;
}

// Boton de B3 HD para cada codigo de IW (la HD cambio los del mando de PS2 por los de la 360): triangulo Y, circulo B,
// equis A, cuadrado X, L1 LB, R1 RB, L2 LT, R2 RT. Recortes de la tira LB LT RB RT Y B A X de data_usi 2062 (los de
// style::Glyph, la ayuda de botones de nuestros menus).
constexpr const char* kNombre[16] = {"", "", "", "", "", "", "", "", "LT", "RT", "LB", "RB", "Y", "B", "A", "X"};
ImVec4 Glifo(int boton) {
  switch (boton) {
    case 12: return {249, 2, 284, 46};   // Y
    case 13: return {297, 2, 332, 46};   // B
    case 14: return {346, 2, 381, 46};   // A
    case 15: return {393, 2, 428, 46};   // X
    case 10: return {4, 2, 56, 46};      // LB
    case 11: return {126, 2, 178, 46};   // RB
    case 8: return {74, 2, 108, 46};     // LT
    default: return {194, 2, 233, 46};   // 9 RT
  }
}

uint32_t AliasPose(const Poses& p, int rol, uint32_t codigo) {
  if (!codigo) return 0;
  const uint32_t cmn = codigo >= 0x57A && codigo <= 0x57D ? 915u : uint32_t(p.cmn[rol]);   // las de espera son de 915
  return dbz3::anim::CodigoQte(cmn, codigo);
}
// El luchador del rol (0 jugador, 1 rival) del combate en curso (0 si no hay).
uint32_t Luchador(int rol) {
  uint8_t* base = Base();
  const uint32_t mgr = base ? REX_LOAD_U32(kMgrPtr) : 0;
  return mgr ? mgr + kX0 + kXPaso * uint32_t(rol) : 0;
}
// Una pose una vez y luego la espera de pelea en bucle; `espera` = true: solo la espera (bucle).
void Poner(const Poses& p, int rol, uint32_t codigo, bool bucle, uint32_t luego = 0) {
  const uint32_t x = Luchador(rol), a = AliasPose(p, rol, codigo);
  if (!x || !a) return;
  if (bucle) dbz3::anim::Bucle(x, a);
  else dbz3::anim::Reproducir(x, a, false, 0, luego ? AliasPose(p, rol, luego) : 0);
}

// ---- Camaras de IW (familias A y B) ----
// UiAnim_Start(0, 0, clip, ...) del ELF no es una animacion de interfaz: reproduce un clip del #AMC de camara del minijuego
// (Camera_FinishMotionLoads: el hijo 2 del #AMB general de DATA_CMN 915, +0x7EC) con ojo / objetivo / giro / fov a 60 Hz
// (formato en docs/03_formatos/CAMARA_ACC.md), y el ultimo clip se queda en su ultimo cuadro. Los clips estan en el mundo de
// IW con el luchador en el origen mirando a +Z (MgPly: +0x40/+0x44/+0x48 = 0): aqui se suman a la posicion del luchador
// (con el rival-pivote delante, a +Z, para que mire igual) y el fov se escribe en mgr+0x1C4 (cine.cpp).
struct ClipCam {
  int nf = 0;
  std::vector<std::array<float, 4>> ojo, obj;   // {cuadro, x, y, z}
  std::vector<std::array<float, 2>> fov;        // {cuadro, radianes}
};
std::vector<uint8_t> g_amc;
std::map<int, ClipCam> g_clips;
std::atomic<int> g_cb_clip{-1}, g_cb_frame{0};   // clip en curso y cuadros desde que empezo (hilo de la interfaz -> del juego)
std::atomic<bool> g_cb_quieto{false};
std::atomic<const Poses*> g_poses_a{nullptr};   // las poses (y camaras) del minijuego de la familia A en curso            // se queda en su primer cuadro (UiAnim_Start con play = 0)

void PrecargarClips(const std::vector<int>& ids) {
  if (g_amc.empty()) {
    P::AfsPs2 cmn;
    if (!cmn.Abrir(P::BuscarIW(), "DATA_CMN.AFS")) return;
    g_amc = P::HijoAmb(cmn.Entrada(915), 2);
  }
  auto u32 = [&](size_t o) {
    uint32_t v = 0;
    if (o + 4 <= g_amc.size()) std::memcpy(&v, g_amc.data() + o, 4);
    return v;
  };
  auto f32 = [&](size_t o) {
    const uint32_t v = u32(o);
    float f;
    std::memcpy(&f, &v, 4);
    return f;
  };
  if (g_amc.size() < 0x20 || std::memcmp(g_amc.data(), "#AMC", 4) != 0) {
    g_amc.clear();
    return;
  }
  const uint32_t n = u32(0x10), tab = u32(0x14);
  for (int id : ids) {
    if (id < 0 || uint32_t(id) >= n || g_clips.count(id)) continue;
    ClipCam c;
    c.nf = int(u32(tab + 16 * size_t(id) + 8));
    const uint32_t off = u32(tab + 16 * size_t(id) + 12);
    for (int k = 0; k < 4; ++k) {
      const uint32_t t = u32(off + 4 * size_t(k)), nk = u32(t + 8);
      if (nk == 0 || nk > 5000) continue;
      for (uint32_t i = 0; i < nk; ++i) {
        if (k < 2) {
          const size_t o = t + 12 + 16 * size_t(i);
          (k == 0 ? c.ojo : c.obj).push_back({float(u32(o)), f32(o + 4), f32(o + 8), f32(o + 12)});
        } else if (k == 3) {
          const size_t o = t + 12 + 8 * size_t(i);
          c.fov.push_back({float(u32(o)), f32(o + 4)});
        }
      }
    }
    if (c.nf > 0 && !c.ojo.empty() && !c.obj.empty()) g_clips[id] = std::move(c);
  }
  REXLOG_INFO("dbz3 r14: camaras de IW: {} clips (AMC de DATA_CMN 915, {} clips en total)", g_clips.size(), n);
}
// Valor de una pista de claves {cuadro, v...} en el cuadro f (lineal, como el juego).
template <size_t N>
void Muestra(const std::vector<std::array<float, N>>& k, float f, float* out) {
  size_t i = 0;
  while (i + 1 < k.size() && k[i + 1][0] <= f) ++i;
  const size_t j = std::min(i + 1, k.size() - 1);
  const float d = k[j][0] - k[i][0], t = d > 0 ? std::clamp((f - k[i][0]) / d, 0.0f, 1.0f) : 0.0f;
  for (size_t c = 1; c < N; ++c) out[c - 1] = k[i][c] + (k[j][c] - k[i][c]) * t;
}
void CamB(int clip, bool quieto = false) {   // hilo de la interfaz: este clip desde el cuadro 0
  if (clip < 0 || !g_clips.count(clip)) return;
  g_cb_clip = clip;
  g_cb_frame = 0;
  g_cb_quieto = quieto;
  REXLOG_INFO("dbz3 r14: camara de IW: clip 0x{:X} ({} cuadros{})", clip, g_clips[clip].nf, quieto ? ", quieto" : "");
}

// ---- Sonidos de golpe ----
// El codigo de IW de estos minijuegos no tiene un efecto de impacto propio: sus voces (Hit_PlayHealthTierSound,
// Minigame_PlayEventVoice, Audio_PlayCharacterVoice) salen por el sistema de ids del IOP de la PS2, sin traducir a las ADX de
// la copia. Suenan los equivalentes de B3, por el mismo gestor de efectos que usa su combate (sub_820922A0; ids medidos con
// DBZ3_TRAZA_SE en un combate normal: banco 2 id 2 = impacto, banco 4 ids 42/43/50 = grito de ataque del luchador 1 y
// 19/20/25 = su grito al ser golpeado). Se piden desde el hilo de la interfaz con un retardo (el puno llega unos cuadros
// despues de empezar la pose) y los reproduce el hilo del juego (SeBotones, gancho de iw_minijuegos.cpp).
constexpr int kSeImpacto[2] = {2, 2}, kSeAtaque[3] = {42, 43, 50}, kSeHerido[3] = {19, 20, 25};
struct SePend {
  int t = 0, banco = 0, id = 0;
};
SePend g_cola_se[8];
std::atomic<int> g_se_listo{-1};   // banco * 1000 + id, para el hilo del juego
void PedirSe(int cuadros, int banco, int id) {
  for (SePend& e : g_cola_se)
    if (e.t <= 0) {
      e = SePend{std::max(cuadros, 1), banco, id};
      return;
    }
}
void CuadroSe() {   // un cuadro de 60 Hz (hilo de la interfaz)
  for (SePend& e : g_cola_se)
    if (e.t > 0 && --e.t == 0) {
      g_se_listo = e.banco * 1000 + e.id;
      REXLOG_INFO("dbz3 r14: sonido banco {} id {}", e.banco, e.id);
    }
}
// Solo suenan las voces del banco del jugador (el del rival no esta cargado): su grito al atacar y al ser golpeado.
void SeGolpe(int atacante, bool conecta) {   // atacante 0 = el jugador; conecta: el golpe da (si no, lo esquivan)
  if (atacante == 0) PedirSe(2, 4, kSeAtaque[Azar() % 3]);
  if (!conecta) return;
  PedirSe(12, 2, kSeImpacto[0]);
  if (atacante == 1) PedirSe(14, 4, kSeHerido[Azar() % 3]);
}


// QteMg_PlayHitPoses(carril, acierto): las dos poses del golpe; despues cada uno vuelve a su espera de pelea.
void PosesGolpe(Juego& j, int carril, bool acierto) {
  const Poses& p = *j.poses;
  const int k = j.k;
  if (carril == 0) {
    Poner(p, 0, p.j_ataca[k], false, p.pelea[0]);
    Poner(p, 1, acierto ? p.r_herido[k] : p.r_esquiva[k], false, p.pelea[1]);
    SeGolpe(0, acierto);
  } else {
    Poner(p, 1, p.r_ataca[k], false, p.pelea[1]);
    Poner(p, 0, acierto ? p.j_esquiva[k] : p.j_herido[k], false, p.pelea[0]);
    SeGolpe(1, !acierto);
  }
  j.k = p.azar ? Azar() & 3 : (k + 1) & 3;
}

// Fallo del carril 1: el rival golpea (vida del jugador; la pose la pone PosesGolpe).
void Golpe(Juego& j) { j.vida[0] -= float(j.m->ataque_rival[j.dif]); }

// Un cuadro de 60 Hz de la logica de IW (QteMg_UpdatePlayerButtons / ProcessPlayerInput / ApplyHitDamage).
void Paso(Juego& j, uint16_t pulsado) {
  ++j.cuadros;
  if (j.aviso_t > 0) --j.aviso_t;
  for (int l = 0; l < 2; ++l) {
    Carril& c = j.c[l];
    if (--c.cuenta <= 0 && c.sale < c.n) {   // sale el siguiente icono
      c.ic[c.sale] = Icono{c.seq[c.sale], -16 - j.vel, true};
      ++c.sale;
      c.cuenta = c.sale < c.n ? c.hueco[c.sale] : 0;
    }
    for (int i = 0; i < c.sale; ++i) {
      Icono& k = c.ic[i];
      if (!k.vivo) continue;
      k.x += j.vel;
      if (!k.resuelto && !k.contado && std::fabs(k.x - kObjetivo) < kIcono) {   // entra en el objetivo
        k.contado = true;
        ++c.llegaron;
      }
      if (!k.resuelto && k.x > kFalla) {   // se paso: fallo
        k.resuelto = true;
        if (!k.contado) {
          k.contado = true;
          ++c.llegaron;
        }
        PosesGolpe(j, l, false);   // QteMg_UpdatePlayerButtons: PlayHitPoses(carril, 0) al pasarse
        if (l == 1) Golpe(j);
        j.aviso_t = 30;
        j.aviso_bien = false;
      }
      if (k.x >= kQuita) k.vivo = false;
    }
    while (c.actual < c.sale && c.ic[c.actual].resuelto) ++c.actual;
    // prueba (auto): pulsa el boton correcto cuando el icono esta en el objetivo
    uint16_t p = pulsado & (l == 0 ? 0xF000 : 0x0F00);
    if (j.auto_ && c.actual < c.sale && c.ic[c.actual].x >= kObjetivo - 4 && !c.ic[c.actual].pedido) {
      Icono& k = c.ic[c.actual];
      k.pedido = true;
      if (j.fallos && l == 1 && c.actual % 5 == 2) continue;   // prueba: fallo a proposito (el rival golpea)
      if (const int32_t x = XboxDe(k.boton)) {   // pulsado de verdad: llega por el mando como el del jugador
        j.inyecta |= x;
        j.inyecta_t = 4;
      } else {
        p = uint16_t(1u << k.boton);
      }
    }
    if (p && c.actual < c.sale && c.ic[c.actual].x > kAcepta) {
      Icono& k = c.ic[c.actual];
      k.resuelto = true;
      if (!k.contado) {
        k.contado = true;
        ++c.llegaron;
      }
      j.aviso_t = 30;
      if (p & (1u << k.boton)) {
        ++c.aciertos;
        k.vivo = false;
        if (l == 0) j.vida[1] -= float(j.m->ataque_jugador[j.dif]);
        PosesGolpe(j, l, true);
        j.aviso_bien = true;
      } else {
        PosesGolpe(j, l, false);   // boton malo: PlayHitPoses(carril, 0)
        if (l == 1) Golpe(j);
        j.aviso_bien = false;
      }
    }
  }
  const bool gastado = j.c[0].actual >= j.c[0].n && j.c[1].actual >= j.c[1].n;
  if (gastado || j.vida[0] <= 0 || j.vida[1] <= 0) {   // QteMg_ClassifyResult + FinalizeResult
    const int ac = j.c[0].aciertos + j.c[1].aciertos, ll = j.c[0].llegaron + j.c[1].llegaron;
    j.nota = ll > 0 ? int(float(ac) / float(ll) * 100.0f) : 0;
    j.rango = 0;
    if (j.vida[0] > 0)
      for (int r = 5; r >= 1; --r)
        if (j.nota >= kUmbral[r][0]) {
          j.rango = r;
          break;
        }
    const float v = j.rango == 0 ? 100.0f : j.pts[j.rango] * j.mult[j.dif];
    j.puntos = (int(v) + 99) / 100 * 100;
    j.fase = 3;
    j.t = 0;
    {   // QteMg_StartResultPhase: poses de victoria o derrota (la del jugador al acabar sale en el panel)
      const Poses& p = *j.poses;
      Poner(p, 0, j.rango > 0 ? p.gana[0] : p.pierde[0], false);
      Poner(p, 1, j.rango > 0 ? p.gana[1] : p.pierde[1], false);
    }
    REXLOG_INFO("dbz3 r14: fin: aciertos {}/{} (carril 0 {}/{}, carril 1 {}/{}), vida {} / {}, nota {}, rango {}, "
                "puntos {}",
                ac, ll, j.c[0].aciertos, j.c[0].llegaron, j.c[1].aciertos, j.c[1].llegaron, int(j.vida[0]),
                int(j.vida[1]), j.nota, j.rango, j.puntos);
  }
}

class Panel : public rex::ui::ImGuiDialog {
 public:
  explicit Panel(rex::ui::ImGuiDrawer* drawer) : ImGuiDialog(drawer) {}

 protected:
  void OnDraw(ImGuiIO& io) override {
    if (!g_corre.load()) return;
    std::shared_ptr<Juego> j;
    {
      std::lock_guard<std::mutex> lk(g_mu);
      j = g_j;
    }
    if (!j) return;
    uint8_t* base = Base();
    // el final de B3 (estados 5-8: su RING OUT, la pose del ganador y su menu azul "Luchar otra vez?") corre a la vez que el
    // resultado (fase 3) y se tapa con negro: sin esto el menu de B3 salia ~1,7 s entre el resultado y el mapa
    const int est = P::EstadoCombate();
    if (!base || (est != 4 && !(j->fase == 3 && est >= 5 && est <= 8))) return;
    const uint32_t mgr = REX_LOAD_U32(kMgrPtr);
    if (!mgr) return;
    if (j.get() != hecho_) Preparar(*j);
    // los dos luchadores de B3 no caen: la vida es la del minijuego
    for (uint32_t x : {mgr + kX0, mgr + kX0 + kXPaso}) StoreF(base, x + kVida, LoadF(base, x + kVidaMax));
    // camara: al ras del suelo en la presentacion, a la altura del combate aereo al empezar, y mas abierta en el resultado
    // (las poses de victoria y derrota se van lejos: raiz de hasta 96 u) y despues mas cerca para la pose final
    {
      const float aire = j->poses->aire, d = j->poses->dist;
      g_cam_alto = j->fase == 0 ? (aire > 20 ? kAlto0 : aire) : j->fase == 1 ? (aire > 20 ? kAlto0 + 0.5f * (aire - kAlto0) : aire) : j->fase == 2 ? aire
                   : j->t < 100 ? (aire > 20 ? aire + 8 : aire - 4) : std::min(18.0f, aire);
      g_cam_dist = j->fase == 3 ? (j->t < 100 ? d * 1.5f : d * 0.65f) : d;
    }
    g_cam_lado = j->fase == 3 && j->t >= 100 ? -1.0f : 1.0f;
    if (const char* e = std::getenv("DBZ3_R14_CAMD")) g_cam_dist = float(std::atof(e));   // prueba
    const bool pausa = dbz3::GameDimmed() || dbz3::OverlayOpen();
    // el mando que ve el minijuego (MandoBotones guarda lo que leyo el juego antes de quitarle los botones a B3)
    const uint16_t ahora = g_raw_ok.load() ? g_raw12.load() : REX_LOAD_U16(kPads + kPadBotones);
    j->pendiente = uint16_t(j->pendiente | (ahora & ~j->antes));   // hasta el siguiente cuadro de la logica
    if (ahora != j->antes && sondas_ < 40) {   // registro: los 40 primeros cambios del mando (sonda de teclas)
      ++sondas_;
      REXLOG_INFO("dbz3 r14: mando +12 {:04X}", ahora);
    }
    j->antes = ahora;
    acum_ += pausa ? 0.0f : std::min(io.DeltaTime, 0.1f);
    while (acum_ >= 1.0f / 60) {   // logica a 60 cuadros por segundo, como en la PS2
      acum_ -= 1.0f / 60;
      ++j->t;
      CuadroSe();
      if (g_cb_clip.load() >= 0 && !g_cb_quieto.load()) ++g_cb_frame;
      if (j->fase == 0 && j->t == 1) CamB(j->poses->cam_ini, true);   // el plano de salida: el primer cuadro del clip
      if (j->fase <= 1) {
        static const float sep_env = std::getenv("DBZ3_R14_SEP") ? float(std::atof(std::getenv("DBZ3_R14_SEP"))) : 0.0f;   // prueba
        const bool cam_iw = j->poses->cam_ini >= 0 && g_clips.count(j->poses->cam_ini);
        Juntar(base, mgr, sep_env > 0 ? sep_env : cam_iw ? j->poses->ancla[4] : j->poses->sep);
        j->pendiente = 0;   // lo pulsado antes de empezar no cuenta
      }
      if (j->fase == 0 && j->t >= 30) {
        j->fase = 1;
        j->t = 0;
        REXLOG_INFO("dbz3 r14: en juego ({} dificultad {})", j->m->nombre, j->dif);
        CamB(j->poses->cam_ini);   // READY?: el clip entero (UpdateCountdownPhase)
        // READY?: jugador pose 2 / rival pose 1 (QteMg_UpdateCountdownPhase); pasan a la espera al acabar
        Poner(*j->poses, 0, j->poses->listo[0], false, j->poses->pelea[0]);
        Poner(*j->poses, 1, j->poses->listo[1], false, j->poses->pelea[1]);
      } else if (j->fase == 1 && j->t >= (j->poses->cam_ini >= 0 && g_clips.count(j->poses->cam_ini) ? j->poses->dur_intro : 60)) {
        j->fase = 2;
        j->t = 0;
        CamB(j->poses->cam_juego, true);   // el plano de juego (en bucle: ya quieto)
        Poner(*j->poses, 0, j->poses->pelea[0], true);   // START!: la espera de pelea (jugador 3 / rival 2)
        Poner(*j->poses, 1, j->poses->pelea[1], true);
      } else if (j->fase == 2 && std::getenv("DBZ3_R14_POSES")) {
        // PRUEBA (DBZ3_R14_POSES=1): en vez de jugar, cada 90 cuadros la pose siguiente de cada luchador (la lista del
        // modulo) y una rafaga de capturas (dbz3_shot_NNN.bmp, una cada 45 pintados) para verlas
        const Poses& p = *j->poses;
        std::vector<uint16_t> lj{p.ini[0], p.listo[0], p.pelea[0]}, lr{p.ini[1], p.listo[1], p.pelea[1]};
        for (int q = 0; q < 4; ++q) {
          lj.push_back(p.j_ataca[q]), lr.push_back(p.r_ataca[q]);
          lj.push_back(p.j_esquiva[q]), lr.push_back(p.r_esquiva[q]);
          lj.push_back(p.j_herido[q]), lr.push_back(p.r_herido[q]);
        }
        lj.insert(lj.end(), {p.gana[0], p.pierde[0], p.fin_bien, p.fin_mal});
        lr.insert(lr.end(), {p.gana[1], p.pierde[1]});
        if (j->t == 1) std::ofstream("dbz3_shot.req") << int(lj.size() * 2) << " 45";
        if ((j->t - 1) % 90 == 0) {
          const size_t n = size_t((j->t - 1) / 90);
          if (n < lj.size()) {
            REXLOG_INFO("dbz3 r14: prueba de poses {}: jugador 0x{:X} rival 0x{:X}", n, lj[n], lr[n % lr.size()]);
            Poner(p, 0, lj[n], false, p.pelea[0]);
            Poner(p, 1, lr[n % lr.size()], false, p.pelea[1]);
          } else if (n == lj.size()) {
            REXLOG_INFO("dbz3 r14: prueba de poses acabada");
          }
        }
      } else if (j->fase == 2 && j->t == 60 && std::getenv("DBZ3_R14_SONDA")) {   // SONDA: el rumbo de los luchadores en X
        const uint32_t xa = mgr + kX0, xb = xa + kXPaso;
        const float ax = LoadF(base, xa + 64), az = LoadF(base, xa + 72), bx = LoadF(base, xb + 64), bz = LoadF(base, xb + 72);
        REXLOG_INFO("dbz3 r14: sonda: A ({:.2f}, {:.2f}, {:.2f}) B ({:.2f}, {:.2f}, {:.2f})", ax, LoadF(base, xa + 68), az, bx,
                    LoadF(base, xb + 68), bz);
        for (uint32_t o = 0; o < 0x400; o += 4) {
          const float v = LoadF(base, xa + o), w = LoadF(base, xb + o);
          if (std::fabs(v) > 0.2f && std::fabs(v) <= 1.0001f && std::fabs(v - std::round(v)) > 0.001f)
            REXLOG_INFO("dbz3 r14: sonda: A+{} = {:.4f}  B+{} = {:.4f}", o, v, o, w);
        }
        Paso(*j, j->pendiente);
        j->pendiente = 0;
      } else if (j->fase == 2) {
        Paso(*j, j->pendiente);
        j->pendiente = 0;   // una pulsacion vale para un solo cuadro de la logica
        if (j->auto_) {
          dbz3::net::InyectarPad(0, j->inyecta_t > 0 ? j->inyecta : 0);
          if (j->inyecta_t > 0 && --j->inyecta_t == 0) j->inyecta = 0;
        }
      } else if (j->fase == 3) {
        if (j->t == 1) {
          if (std::getenv("DBZ3_IWMINI_RAFAGA_FIN")) std::ofstream("dbz3_shot.req") << "40 15";   // prueba
          if (j->auto_) dbz3::net::InyectarPad(0, -1);
          if (!fin_) {   // el combate de B3 acaba ya (como el ring out de P4): su final (~11 s) corre bajo el resultado
            fin_ = true;
            REX_STORE_U8(mgr + kFin, uint8_t(j->rango > 0 ? 1 : 2));
          }
          CamB(j->rango > 0 ? j->poses->cam_gana : j->poses->cam_pierde);   // QteMg_StartResultPhase
        }
        if (j->t == 100) {   // QteMg_FinalizeResult: la pose final del jugador
          Poner(*j->poses, 0, j->rango > 0 ? j->poses->fin_bien : j->poses->fin_mal, false);
          CamB(j->rango > 0 ? j->poses->cam_fin_bien : j->poses->cam_fin_mal);
        }
      }
    }
    Dibujar(io, *j);
  }

 private:
  void Preparar(const Juego& j) {
    hecho_ = &j;
    fin_ = false;
    acum_ = 0;
    g_cam_reset = true;
    Poner(*j.poses, 0, j.poses->ini[0], true);   // pose 0 de la presentacion (los dos quietos)
    Poner(*j.poses, 1, j.poses->ini[1], true);
    objetivo_ = ps2::Crear(j.objetivo);
    failed_ = ps2::Crear(j.failed);
    ready_ = ps2::Crear(j.ready);
    start_ = ps2::Crear(j.start);
    hit_ = ps2::Crear(j.hit);
    miss_ = ps2::Crear(j.miss);
    clear_ = ps2::Crear(j.clear);
    aro_ = ps2::Crear(j.aro);
    relleno_ = ps2::Crear(j.relleno);
    barra_ = ps2::Crear(j.barra);
  }
  static void Pieza(ImDrawList* dl, const ps2::Tex& t, ImVec2 c, float k) {
    if (t) dl->AddImage(t.id(), ImVec2(c.x - t.w * k / 2, c.y - t.h * k / 2), ImVec2(c.x + t.w * k / 2, c.y + t.h * k / 2));
  }
  void Dibujar(ImGuiIO& io, const Juego& j) {
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    const S::Area a = S::GameArea(io);
    auto at = [&](float x, float y) { return a.At(x * 1200 / kW, y * 900 / kH); };   // pantalla de IW -> 1200x900
    const float k = a.s * 900 / kH;
    // Vida de los dos (UiFade 0 y 1 del modulo). ponytail: la pieza propia del modulo no esta leida; van las del HUD
    // de los minijuegos de IW (6/0: aro con la cara, marco t15 y relleno amarillo t01, medidas de P4) arriba, el
    // jugador a la izquierda y el rival a la derecha (en espejo).
    for (int i = 0; i < 2; ++i) {
      const float v = std::clamp(j.vida[i] / 1000.0f, 0.0f, 1.0f), kk = 1.6f * a.s;
      auto x = [&](float px, float w) { return i == 0 ? px : 1200 - px - w; };   // px de 1200x900
      const ImVec2 b0 = a.At(x(150, 240 * 1.6f), 92);
      if (barra_) fg->AddImage(barra_.id(), b0, ImVec2(b0.x + 240 * kk, b0.y + 40 * kk), ImVec2(i ? 1.0f : 0.0f, 0),
                               ImVec2(i ? 0.0f : 1.0f, 1));
      if (relleno_ && v > 0) {
        const float x0 = i == 0 ? b0.x + 22 * kk : b0.x + (218 - 200 * v) * kk;
        fg->AddImage(relleno_.id(), ImVec2(x0, b0.y + 12 * kk), ImVec2(x0 + 200 * v * kk, b0.y + 28 * kk), ImVec2(0, 0),
                     ImVec2(v, 1));
      }
      {   // F1: cara del personaje que juega (y del rival) en su forma (iwmini::PintarCara)
        const ImVec2 c0 = a.At(x(28, 64 * 1.6f), 30);
        dbz3::iwmini::PintarCara(fg, c0.x, c0.y, 64 * 1.6f * a.s, i ? j.m->rival : j.m->jugador,
                                 i ? j.m->forma_rival : j.m->forma_jugador, i != 0);
      }
      if (aro_) fg->AddImage(aro_.id(), a.At(x(6, 88 * 1.6f), 8), a.At(x(6, 88 * 1.6f) + 88 * 1.6f, 8 + 88 * 1.6f),
                             ImVec2(i ? 1.0f : 0.0f, 0), ImVec2(i ? 0.0f : 1.0f, 1));
      if (!barra_) {   // sin el HUD de IW: barra lisa
        const ImVec2 p1(b0.x + 240 * kk, b0.y + 20 * kk);
        fg->AddRectFilled(b0, p1, IM_COL32(20, 20, 40, 200));
        fg->AddRectFilled(b0, ImVec2(b0.x + (p1.x - b0.x) * v, p1.y), IM_COL32(255, 210, 40, 255));
      }
    }
    if (j.fase == 1) Pieza(fg, ready_, at(kW / 2, 200), k);
    if (j.fase == 2 && j.t < 60 && j.cuadros < 60) Pieza(fg, start_, at(kW / 2, 200), k);
    if (j.fase >= 2) {
      for (int l = 0; l < 2; ++l) {
        const float y = kCarrilY[l] + kIcono / 2;
        fg->AddRectFilled(at(0, y - 18), at(kW, y + 18), IM_COL32(0, 0, 0, 110));
        if (objetivo_) {   // el aro de IW (72 x 56; el centro del aro en (38, 28))
          const ImVec2 c = at(kObjetivo, y);
          fg->AddImage(objetivo_.id(), ImVec2(c.x - 38 * k, c.y - 28 * k), ImVec2(c.x + 34 * k, c.y + 28 * k));
        } else {
          fg->AddCircle(at(kObjetivo, y), 20 * k, IM_COL32(255, 230, 80, 255), 24, 3 * a.s);
        }
        const S::Pic pic = S::Img("botones");
        for (int i = 0; i < j.c[l].sale; ++i) {
          const Icono& ic = j.c[l].ic[i];
          if (!ic.vivo) continue;
          const ImU32 col = ic.resuelto ? IM_COL32(120, 120, 120, 200) : IM_COL32(255, 255, 255, 255);
          const ImVec2 c = at(ic.x + kIcono / 2, kCarrilY[l] + kIcono / 2);
          if (!pic) {   // sin la pieza de la HD: el nombre del boton
            S::Glyph(fg, kNombre[ic.boton & 15], ImVec2(c.x - kIcono * k / 2, c.y), kIcono * k);
            continue;
          }
          const ImVec4 r = Glifo(ic.boton);
          const float h = kIcono * k * 1.1f, w = (r.z - r.x) * h / (r.w - r.y);
          S::DrawPic(fg, pic, ImVec2(c.x - w / 2, c.y - h / 2), ImVec2(c.x + w / 2, c.y + h / 2), r, col);
        }
      }
      if (j.aviso_t > 0) Pieza(fg, j.aviso_bien ? hit_ : miss_, at(kObjetivo, 330), k * 0.8f);
    }
    if (j.fase == 3) {
      const float t = std::min(1.0f, j.t / 30.0f);
      fg->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(0, 0, 0, int(160 * t)));
      // CLEAR del juego (como P4); la nota, el rango y los puntos los ensena el mapa (PantallaDM::Resultado)
      Pieza(fg, j.rango > 0 ? clear_ : failed_, at(kW / 2, 200), k * 1.5f * (0.8f + 0.2f * t));
      // QteMg_UpdateResultPhase: 228 cuadros; despues a negro (tapa lo que quede del final de B3 hasta el mapa)
      if (const float neg = std::clamp((j.t - 228) / 36.0f, 0.0f, 1.0f); neg > 0)
        fg->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(0, 0, 0, int(255 * neg)));
    }
  }
  // El rival se pone a `sep` delante del jugador (en la linea que los une).
  static void Juntar(uint8_t* base, uint32_t mgr, float sep) {
    const uint32_t x0 = mgr + kX0, x1 = x0 + kXPaso;
    const float ax = LoadF(base, x0 + 64), az = LoadF(base, x0 + 72), bx = LoadF(base, x1 + 64), bz = LoadF(base, x1 + 72);
    const float dx = bx - ax, dz = bz - az, l = std::hypot(dx, dz);
    if (l < 1e-3f || std::fabs(l - sep) < 0.5f) return;
    for (uint32_t o : {48u, 64u, 80u}) {   // la posicion esta tres veces (tagteam.cpp StepSlide)
      StoreF(base, x1 + o, ax + dx / l * sep);
      StoreF(base, x1 + o + 8, az + dz / l * sep);
    }
  }
  const Juego* hecho_ = nullptr;
  int sondas_ = 0;
  bool fin_ = false;
  float acum_ = 0;
  ps2::Tex objetivo_, ready_, start_, hit_, miss_, clear_, failed_, aro_, relleno_, barra_;
};

void CreatePanel() {
  static Panel* panel = nullptr;   // vive lo que el juego
  auto* rt = rex::Runtime::instance();
  if (!panel && rt && rt->imgui_drawer()) panel = new Panel(rt->imgui_drawer());
}

bool Empezar(int clave, int fila, int dificultad) {
  const Modulo2* m = nullptr;
  for (const Modulo2& x : kModulos)
    if (x.clave == clave) m = &x;
  if (!m) return false;
  auto j = std::make_shared<Juego>();
  j->m = m;
  j->clave = clave;
  j->fila = fila;
  j->dif = std::clamp(dificultad, 0, 5);
  j->vel = m->velocidad[j->dif];
  j->poses = m->poses;
  g_poses_a = m->poses;
  g_cb_clip = -1;
  if (m->poses->cam_ini >= 0 && !std::getenv("DBZ3_IWMINI_CAMARA_B3")) {   // DBZ3_IWMINI_CAMARA_B3: la camara propia
    const Poses& q = *m->poses;
    PrecargarClips({q.cam_ini, q.cam_juego, q.cam_gana, q.cam_pierde, q.cam_fin_bien, q.cam_fin_mal});
  }
  const char* t = std::getenv("DBZ3_TEST_IWMINI");
  j->auto_ = t && std::strstr(t, "auto");
  j->fallos = t && std::strstr(t, "fallos");
  std::string error;
  if (!Cargar(*j, error)) {
    REXLOG_WARN("dbz3 r14: {}: {}", m->nombre, error);
    return false;
  }
  j->c[0].cuenta = j->c[0].hueco[0];
  j->c[1].cuenta = j->c[1].hueco[0];
  REXLOG_INFO("dbz3 r14: {} (clave 0x{:X}, dificultad {}): carril 0 empieza {} {} {}, carril 1 {} {} {}; velocidad {}",
              m->nombre, clave, j->dif, j->c[0].seq[0], j->c[0].seq[1], j->c[0].seq[2], j->c[1].seq[0], j->c[1].seq[1],
              j->c[1].seq[2], j->vel);
  {
    std::lock_guard<std::mutex> lk(g_mu);
    g_j = j;
  }
  dbz3::launcher::DeferUi(&CreatePanel);
  const int escenario = std::max(P::EscenarioPorMapa(P::MapaIW(m->escenario_iw)), 0);
  if (!dbz3::hub::RequestBattleEx(m->jugador, m->rival, escenario, true, -1, 0, 0, m->forma_jugador, m->forma_rival)) {
    REXLOG_WARN("dbz3 r14: el conector no acepto el combate");
    return false;
  }
  dbz3::combate::SetMuneco(1);       // el rival quieto, sin IA (le pegan los aciertos del minijuego)
  dbz3::combate::OcultarHud(true);   // sin marcador de B3
  g_corre = true;
  P::Reglas r;
  r.capsulas = false;
  P::VigilarCombate("r14", r, [j](const P::Fin&) {
    g_corre = false;
    g_cb_clip = -1;
    if (j->auto_) dbz3::net::InyectarPad(0, -1);
    dbz3::combate::SetMuneco(-1);
    dbz3::combate::OcultarHud(false);
    Resultado res;
    res.clave = j->clave;
    res.fila = j->fila;
    res.superado = j->fase == 3 && j->rango > 0;
    res.centesimas = j->cuadros * 100 / 60;
    res.puntos = j->fase == 3 ? j->puntos : 0;
    res.rango = std::max(j->rango - 1, 0);   // IW 1..5 -> 0 C .. 4 Z
    res.monedas = j->c[0].aciertos + j->c[1].aciertos;
    REXLOG_INFO("dbz3 r14: resultado {} superado {} rango {} puntos {}", j->m->nombre, res.superado, res.rango,
                res.puntos);
    Terminar(res);
  });
  return true;
}

// ---- Familia B, "10 rondas" (11 MSN_NB012; la lista 1-10 de la captura 88) ----
// RE (decomp EU, nombres Mg_*; USA = EU - 0x7B0 en los datos):
//  - 10 rondas: orden barajado de 20 filas (Mg_ShuffleRoundOrder: 5 de las filas 0-9 y 5 de las 10-19). Cada fila son
//    10 botones (tabla S + d*0xD0 + fila*10) y 10 separaciones en cuadros (tabla H + d*0x320 + fila*0x28).
//  - Botones por ronda +0xBC y en la ultima +0xC0 (Mg_SetHitThresholdsByLevel); velocidad +0x104 px por cuadro
//    (Mg_InitParamsByLevel). Un solo carril en y 395 (Mg_UpdateScrollingIcons); el icono sale al acabar la cuenta y
//    la cuenta carga la separacion de ese icono; se quita en W + 56.
//  - Juicio por solape con el objetivo (Mg_JudgeIconOverlap): dentro del todo 0, lo que sobresale < 25 % del icono 1,
//    < 50 % 2, si no 3. Al pulsar el boton bueno con el icono a la derecha de objetivo - 80 (Mg_UpdateQteInput) la ronda
//    suma +0x108[juicio] / botones de la ronda (100, 70, 50; 3 = 0; division entera) (Mg_ApplyQteJudgeScore); el boton
//    malo marca el icono como fallado.
//  - Total = suma de las 10 rondas (Mg_ComputeTotalAndRank, umbrales +0xCC..+0xF4): 901-1000 -> 5, 801-900 -> 4,
//    751-800 -> 3, 601-750 -> 2, 301-600 -> 1, si no 0. Puntos (Mg_FinishAndWriteResults) = tabla[rango] x
//    multiplicador[d] redondeado a la centena; rango 0 -> 100.
//  - Personaje: el del mapa si es uno de 7, 12, 10, 13 (tabla 0x47F310 USA; Mg_ValidateCharSettings), si no 7 (Vegeta);
//    escenario de IW 4 (ROCK), 12 con 10 o 13 (Mg_InitStageByChar). Un solo luchador: el otro es el pivote invisible.
// NB007 (6, Goku hacia Namek) es la otra variante ("teclear", MiniGame_* del decomp EU 0x2928C0-0x295C10): la fila de
// N botones esta quieta y se teclea en orden lo antes posible (MiniGame_CheckInputStep: el boton malo no avanza); el
// tiempo de la QTE de cada ronda (QTE_StartTimer / QTE_GetTimer, segundos) va a la lista. Rango por la suma de los 10
// tiempos (MiniGame_CalcRank, umbrales de s_MSN_NB007_Update): <= 25 -> 5, <= 35 -> 4, <= 40 -> 3, <= 45 -> 2,
// <= 55 -> 1, si no 0; y 0 si las 10 rondas pasan de 7 s (+0xF8). Botones por ronda {3,5,6,8,9,9} y la ultima
// {4,6,7,9,10,10} (Misc_SetParamTripleByType). Goku solo (MiniGame_AllocState pone +0x10 = 0).
// MSN019 (18, Videl) es la tercera ("barrido", decomp EU 0x2E92A0-0x2EC8B0, todo constantes de Minigame_InitConfigTable):
// 10 filas; en cada una N objetos (4 5 7 8 9 10 por dificultad) en huecos de una barra centrada (ancho 0,5 .. 0,9 de la
// pantalla, con algo de azar: Results_RespawnItem) con un boton al azar de la mascara de la dificultad (F000 botones,
// FC00 + L1 R1, FF00 + L2 R2; Results_ItemTypeToMask). Un cursor rebota de punta a punta (Bounce, 2,0 .. 5,2 px por
// cuadro) y hay que pulsar el boton de cada objeto mientras lo pisa. La fila acaba al cogerlos todos o al pasar de 7
// rebotes; nota por rebotes (Results_StateEvaluate): <= 2 -> 10, <= 4 -> 5, <= 7 -> 3, si no 0. Total de las 10 filas
// (max. 100) -> rango por 60/70/80/90/100 (GetRankIndexByThresholds) y premio {6500 .. 7700} x multiplicador.
enum Tipo { kDesplazar, kTeclear, kBarrido };
// R14 familia B: las poses de IW del luchador (maquinas de estados MgPly / MgPlyB del ELF: MgPly_SetPoseState 0x311DF0 y
// MgPlyB_ChangeState 0x309940 en NB012 y NB007; MgPly_ChangeState 0x308BD0 en MSN019). Los tres comparten el esquema:
// 0 intro (READY), 3 espera en bucle, 4 acierto, 5-6-7 fallo en cadena (se cae, se queda, se levanta), 8 final bueno, 1 y 2
// resultado (NB012: Mg_FinishAndWriteResults; NB007: MiniGame_ShowResult). MSN019: estados 0/2/3 = espera por tramo de
// filas (0-2, 3-5, 6-9; Results_TriggerReactionAnim), 1 = fila completa, 6 = fila fallada, 4 = intro, 5 = final malo, 7 = bueno.
// Los codigos salen del ELF (los `sw ..., 0xB60` de cada Enter); las de pose compartida de NB007 son de 915.
struct PosesB {
  int cmn;
  uint16_t intro, espera[3], entra[3], golpe[3], mal[3], gana, pierde, fin[2];
  int dur_golpe, dur_mal[2];   // cuadros (de las propias animaciones)
  // Camaras de IW (UiAnim_Start = reproducir un clip del #AMC de DATA_CMN 915, hijo 2): -1 = la de B3. Estatica al empezar,
  // la del READY (dur_intro cuadros), una por ronda (2 cuadros: un plano fijo; ronda r >= 1 usa cam_ronda[r - 1]) y las
  // del final (el ultimo clip se queda en su ultimo cuadro, como el juego).
  int cam_ini, cam_intro, cam_ronda[9], cam_gana, cam_pierde, dur_intro;
};
constexpr PosesB kPosesNB012 = {931, 0x500, {0x4B0, 0x4B0, 0x4B0}, {0, 0, 0}, {0x4BA, 0x4BA, 0x4BA}, {0x4B1, 0x4B2, 0x4B3},
                                0x4C4, 0x4B1, {0x501, 0x502}, 198, {25, 39},
                                // Mg_UpdateGameIntro 0x14A / 0x44C (Vegeta; 0x44F Ten, 0x452 Krilin, 0x455 Yamcha), Mg_StartNextRound
                                // (tabla 0x47E290 EU), Mg_CheckLastRoundEnd 0x154, Mg_StartResultAnimByChar 0x44E
                                0x14A, 0x44C, {0x14B, 0x14C, 0x14D, 0x14E, 0x14F, 0x150, 0x151, 0x152, 0x153}, 0x154, 0x44E, 180};
constexpr PosesB kPosesNB007 = {922, 0x30C, {0x2BC, 0x2BC, 0x2BC}, {0, 0, 0}, {0x2C6, 0x2C6, 0x2C6}, {0x2BD, 0x2BE, 0x2BF},
                                0x2D0, 0x2BD, {0x30D, 0x30E}, 130, {25, 40},
                                // MiniGame_UpdateCountdown 0xB4 / 0x352, MiniGame_StartNextRound (tabla USA 0x472F00), 0xBE, ShowResult 0x354
                                0xB4, 0x352, {0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD}, 0xBE, 0x354, 180};
constexpr PosesB kPosesMSN019 = {930, 0x7BC, {0x76C, 0x76E, 0x770}, {0, 0x76D, 0x76F}, {0x776, 0x777, 0x778}, {0x780, 0, 0},
                                 0x7BC, 0x7BC, {0x7BC, 0x7BC}, 45, {0, 0},   // el final es el estado 4 (Results_BeginOutcome / FinishOutcome)
                                 -1, -1, {-1, -1, -1, -1, -1, -1, -1, -1, -1}, 0x5AA, 0x5AA, 60};
// MSN019: el clip de resultado de Videl (Results_StateFinishOutcome: tabla USA 0x497DA0 = 0x5AA..0x5AF, el de Videl el primero;
// plano fijo alrededor de ella en el origen). La camara de juego (DATA_CMN 879, ranura 1) sigue sin portar: ver docs.
struct ModuloB {
  int indice, clave;
  const char* nombre;
  int tipo;
  bool teclear;
  uint32_t seq, huecos, puntos;
  float velocidad[6];
  int por_ronda[6], ultima[6];
  int jugador, escenario_iw;
  const PosesB* poses;
};
constexpr ModuloB kModulosB[] = {
    {11, 0x7DA, "MSN_NB012", kDesplazar, false, 0x47DB10, 0x47DFF0, 0x47F2D0, {1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f},
     {3, 4, 5, 6, 7, 9}, {4, 5, 6, 7, 8, 10}, 7, 4, &kPosesNB012},
    // ponytail: la escena de IW es el interior de la nave (DATA_CMN 880, modelo de IW); va el escenario 0 de IW (BUDO)
    {6, 0x7D5, "MSN_NB007", kTeclear, true, 0x472F30, 0, 0x473680, {0, 0, 0, 0, 0, 0}, {3, 5, 6, 8, 9, 9}, {4, 6, 7, 9, 10, 10}, 0,
     0, &kPosesNB007},
    // ponytail: el escenario de MSN019 (Stage_BeginLoad del modulo) sin leer; va el 5 de IW (PLANE). IW no tiene cara de
    // Videl en 6/0: va la nativa de B3 (PintarCara)
    {18, 0x7E1, "MSN019", kBarrido, false, 0, 0, 0, {2.0f, 3.5f, 4.0f, 4.5f, 5.0f, 5.2f}, {4, 5, 7, 8, 9, 10},
     {4, 5, 7, 8, 9, 10}, 15, 5, &kPosesMSN019},
};
constexpr uint16_t kMascaraBarrido[6] = {0xF000, 0xF000, 0xF000, 0xFC00, 0xFC00, 0xFF00};
constexpr float kAnchoBarrido[6] = {0.5f, 0.6f, 0.75f, 0.8f, 0.85f, 0.9f};
constexpr int kTipoBoton[8] = {15, 12, 13, 14, 10, 11, 8, 9};   // tipo de objeto -> codigo de boton (0x8000 = 1 << 15...)
constexpr float kUmbralT[6] = {1e9f, 55.0f, 45.0f, 40.0f, 35.0f, 25.0f};   // NB007: total de segundos <= umbral
constexpr float kYB = 395, kObjB = kW / 2 + 60;   // ponytail: x del objetivo (objeto 0x20 de la UI) sin leer
constexpr float kObjAncho = 48;                    // ponytail: ancho de su caja sin leer (el icono mide 32)
constexpr int kJuicioPts[3] = {100, 70, 50};       // +0x108, +0x10C, +0x110
constexpr int kUmbralB[6] = {0, 301, 601, 751, 801, 901};

// Azar del juego (Memory() de IW) para barajar las rondas y colocar los objetos: semilla del reloj.
int Azar() {
  static std::mt19937 g(uint32_t(std::chrono::steady_clock::now().time_since_epoch().count()));
  return int(g() & 0x7FFFFFFF);
}

struct JuegoB {
  const ModuloB* m = nullptr;
  const PosesB* pb = nullptr;
  int clave = 0, fila = 0, dif = 2;
  uint8_t seq[20][10] = {};
  int hueco[20][10] = {};
  int orden[10] = {};
  float pts[6] = {}, mult[6] = {};
  float vel = 2;
  int ronda = 0, n = 0, sale = 0, cuenta = 0, actual = -1;
  Icono ic[10];
  bool pulsado[10] = {};
  float nota[10] = {};
  int fase = 0, t = 0, cuadros = 0, rango = 0, puntos = 100, total = 0;
  int juicio = -1, juicio_t = 0;
  bool auto_ = false;
  uint16_t antes = 0, pendiente = 0;
  int32_t inyecta = 0;
  int inyecta_t = 0;
  float cursor = 0, sentido = 1;   // barrido: x del cursor y hacia donde va
  int rebotes = 0;
  float b0 = 0, b1 = 0;             // barrido: extremos de la barra
  P::Imagen objetivo, ready, start, clear, failed, aro;
  P::Imagen palabra[4];   // DATA_USA 4/0: t15 GREAT!, t08 GOOD!, t09 NORMAL, t10 BAD (juicio 0..3)
  bool fallos = false, mal_hecho = false;   // prueba: el piloto falla una vez en la ronda 2 (poses del fallo)
  int pose_t = 0, cola_t = 0, cola_k = 0, nivel = -1, variante = 0;   // poses de IW (PosesB): cuadros, cadena del fallo, tramo
};
std::shared_ptr<JuegoB> g_jb;
std::atomic<bool> g_corre_b{false};

// ---- poses de IW de la familia B ----
uint32_t AliasB(const PosesB& p, uint32_t codigo) {
  const uint32_t cmn = codigo >= 0x2BC && codigo <= 0x2D0 ? 915u : uint32_t(p.cmn);   // las compartidas de NB007 son de 915
  return codigo ? dbz3::anim::CodigoQte(cmn, codigo) : 0;
}
// Una pose una vez y luego `luego` en bucle; `bucle`: solo en bucle.
void PonerB(const PosesB& p, uint32_t codigo, bool bucle, uint32_t luego = 0) {
  const uint32_t x = Luchador(0), a = AliasB(p, codigo);
  if (!x || !a) return;
  if (bucle) dbz3::anim::Bucle(x, a);
  else dbz3::anim::Reproducir(x, a, false, 0, AliasB(p, luego));
}
enum EventoB { kEvIntro, kEvNivel, kEvGolpe, kEvMal, kEvFin };
void EventoPose(JuegoB& j, int ev) {
  if (!j.pb) return;
  const PosesB& p = *j.pb;
  const int n = j.m->tipo == kBarrido ? (j.ronda < 3 ? 0 : j.ronda < 6 ? 1 : 2) : 0;
  switch (ev) {
    case kEvIntro:   // READY: la pose 0 y luego la espera
      PonerB(p, p.intro, false, p.espera[0]);
      j.pose_t = 90;
      break;
    case kEvNivel:   // MSN019: sube el tramo de filas (Results_TriggerReactionAnim: filas 0, 3 y 6)
      if (n == j.nivel) break;
      j.nivel = n;
      if (p.entra[n]) PonerB(p, p.entra[n], false, p.espera[n]);
      else PonerB(p, p.espera[n], true);
      break;
    case kEvGolpe:   // acierto (la fila entera en MSN019); mientras dura la pose no se repite
      if (j.pose_t > 0) break;
      PonerB(p, p.golpe[j.variante++ % 3], false, p.espera[n]);
      j.pose_t = p.dur_golpe;
      if (j.m->tipo != kBarrido) SeGolpe(0, true);   // NB012 y NB007 pelean (MSN019 vuela: sin golpes)
      break;
    case kEvMal:     // fallo: cae (5), se queda (6) y se levanta (7); luego la espera
      PonerB(p, p.mal[0], false, p.espera[n]);
      if (j.m->tipo != kBarrido) SeGolpe(1, true);   // el fallo: le dan
      j.pose_t = p.dur_mal[0] + p.dur_mal[1] + 40;
      j.cola_k = p.mal[1] ? 1 : 0;
      j.cola_t = j.cola_k ? p.dur_mal[0] : 0;
      break;
    case kEvFin:     // resultado: el final (8) y luego el 1 o el 2 en bucle
      j.cola_t = j.cola_k = 0;
      PonerB(p, j.rango > 0 ? p.gana : p.pierde, false, p.fin[j.rango > 0 ? 0 : 1]);
      break;
  }
}
void PrecargarCamaras(const PosesB& p) {
  std::vector<int> ids = {p.cam_ini, p.cam_intro, p.cam_gana, p.cam_pierde};
  ids.insert(ids.end(), std::begin(p.cam_ronda), std::end(p.cam_ronda));
  PrecargarClips(ids);
}
// Un cuadro de 60 Hz: la cadena del fallo y el temporizador de la pose.
void CuadroPose(JuegoB& j) {
  CuadroSe();
  if (g_cb_clip.load() >= 0 && !g_cb_quieto.load()) ++g_cb_frame;
  if (!j.pb) return;
  if (j.pose_t > 0) --j.pose_t;
  if (j.cola_t > 0 && --j.cola_t == 0 && j.cola_k) {
    const PosesB& p = *j.pb;
    const int n = j.m->tipo == kBarrido ? (j.ronda < 3 ? 0 : j.ronda < 6 ? 1 : 2) : 0;
    PonerB(p, p.mal[j.cola_k], false, p.espera[n]);
    if (j.cola_k == 1) {
      j.cola_k = 2;
      j.cola_t = p.dur_mal[1];
    } else {
      j.cola_k = 0;
    }
  }
}

bool CargarB(JuegoB& j, std::string& error) {
  const ModuloB& m0 = *j.m;
  if (m0.tipo == kBarrido) {   // todo constantes del codigo; los pts del premio tambien
    constexpr float kPremio[6] = {100, 6500, 6800, 7100, 7400, 7700}, kMult[6] = {0.6f, 0.8f, 1.0f, 1.3f, 1.6f, 2.0f};
    for (int k = 0; k < 6; ++k) j.pts[k] = kPremio[k], j.mult[k] = kMult[k];
  }
  std::ifstream f(P::BuscarIW() / "SLUS_218.42", std::ios::binary);
  if (!f) {
    error = "sin SLUS_218.42 de Infinite World (USA)";
    return false;
  }
  std::vector<uint8_t> e((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  auto off = [&](uint32_t a) -> const uint8_t* {
    const size_t o = size_t(a) - 0x100000 + 0x80;
    return o + 0x400 <= e.size() ? e.data() + o : nullptr;
  };
  const ModuloB& m = *j.m;
  for (int r = 0; r < 20 && m.tipo != kBarrido; ++r) {
    const uint8_t* s = off(m.seq + uint32_t(j.dif) * 0xD0 + uint32_t(r) * 10);
    const uint8_t* h = m.teclear ? s : off(m.huecos + uint32_t(j.dif) * 0x320 + uint32_t(r) * 0x28);
    if (!s || !h) return false;
    for (int i = 0; i < 10; ++i) {
      j.seq[r][i] = s[i];
      if (!m.teclear) std::memcpy(&j.hueco[r][i], h + 4 * i, 4);
      // NB007 lleva tambien la cruceta (4 arriba, 5 derecha, 6 abajo, 7 izquierda, desde la dificultad 1)
      if (s[i] < (m.teclear ? 4 : 8) || s[i] > 15 || j.hueco[r][i] < 0 || j.hueco[r][i] > 3000) {
        error = "las tablas del ELF no son las de IW USA";
        return false;
      }
    }
  }
  const uint8_t* p = m.tipo == kBarrido ? nullptr : off(m.puntos);
  if (!p && m.tipo != kBarrido) return false;
  for (int k = 0; p && k < 6; ++k) {
    int32_t v;
    std::memcpy(&v, p + 4 * k, 4);
    j.pts[k] = float(v);
    std::memcpy(&j.mult[k], p + 0x20 + 4 * k, 4);
  }
  P::AfsPs2 usa;
  if (!usa.Abrir(P::BuscarIW(), "DATA_USA.AFS")) {
    error = "sin DATA_USA.AFS";
    return false;
  }
  j.objetivo = P::TexturaAmt(P::HijoAmb(usa.Entrada(0), 1), 10);
  const std::vector<uint8_t> t51 = P::HijoAmb(usa.Entrada(5), 1), t40 = P::HijoAmb(usa.Entrada(4), 0);
  j.start = P::TexturaAmt(t51, 0);
  j.ready = P::TexturaAmt(t51, 8);
  j.clear = P::TexturaAmt(t51, 9);
  j.failed = P::TexturaAmt(t51, 10);
  constexpr int kPalabra[4] = {15, 8, 9, 10};
  for (int k = 0; k < 4; ++k) j.palabra[k] = P::TexturaAmt(t40, kPalabra[k]);
  const std::vector<uint8_t> hud = P::HijoAmb(usa.Entrada(6), 0);
  if (P::CuantasAmt(hud) > 38) {
    j.aro = P::TexturaAmt(hud, 0);
  }
  // Mg_ShuffleRoundOrder: 0-9 barajadas (rondas 1-5) y 10-19 barajadas (rondas 6-10)
  int a[10], b[10];
  for (int i = 0; i < 10; ++i) a[i] = i, b[i] = 10 + i;
  for (int i = 0; i < 10; ++i) {
    std::swap(a[i], a[Azar() % 10]);
    std::swap(b[i], b[Azar() % 10]);
  }
  for (int i = 0; i < 5; ++i) j.orden[i] = a[i], j.orden[5 + i] = b[i];
  return true;
}

void EmpezarRonda(JuegoB& j) {
  j.n = j.ronda == 9 ? j.m->ultima[j.dif] : j.m->por_ronda[j.dif];
  j.sale = 0;
  j.cuenta = 0;
  j.actual = -1;
  for (int i = 0; i < 10; ++i) {
    j.ic[i] = Icono{};
    j.pulsado[i] = false;
  }
  if (j.m->tipo != kBarrido) return;
  // Results_RespawnItem: hueco i de la barra (ancho/N) con un poco de azar (4 pasos, sin los 2 ultimos) y boton al azar
  // de la mascara de la dificultad
  const float medio = kAnchoBarrido[j.dif] * kW / 2, hueco = 2 * medio / j.n;
  j.b0 = kW / 2 - medio;
  j.b1 = kW / 2 + medio;
  for (int i = 0; i < j.n; ++i) {
    int tipo = 0;
    do tipo = Azar() % 8;
    while (!((1u << kTipoBoton[tipo]) & kMascaraBarrido[j.dif]));
    j.ic[i] = Icono{kTipoBoton[tipo], j.b0 + i * hueco + hueco / 4 * float(Azar() % 2), true};
  }
  j.sale = j.n;
  j.cursor = j.b0;
  j.sentido = 1;
  j.rebotes = 0;
}

// Juicio del icono k contra el objetivo (Mg_JudgeIconOverlap).
int Juicio(float x) {
  const float i0 = x, i1 = x + kIcono, o0 = kObjB - kObjAncho / 2, o1 = kObjB + kObjAncho / 2;
  if (i0 >= o0 && i1 <= o1) return 0;
  const int pct = int((i0 < o0 ? o0 - i0 : i1 - o1) / kIcono * 100);
  return pct < 25 ? 1 : pct < 50 ? 2 : 3;
}

// NB007: la fila quieta; se teclea en orden y cuenta el tiempo de la ronda.
void PasoTeclear(JuegoB& j, uint16_t pulsado) {
  const int fila = j.orden[j.ronda];
  if (j.sale == 0) {   // la fila entera de una vez (MiniGame_UpdateSlotIcons)
    for (int i = 0; i < j.n; ++i) j.ic[i] = Icono{j.seq[fila][i], 0, true};
    j.sale = j.n;
    j.actual = 0;
  }
  j.nota[j.ronda] += 1.0f / 60;   // QTE_Update: segundos
  uint16_t p = pulsado & 0xFFF0;
  if (j.auto_ && j.actual < j.n && j.inyecta_t == 0 && j.t % 12 == 6) {   // prueba: una pulsacion cada 12 cuadros
    int bt = j.ic[j.actual].boton;
    if (j.fallos && j.ronda == 1 && !j.mal_hecho) {   // prueba: un boton malo
      j.mal_hecho = true;
      bt = bt == 15 ? 14 : 15;
    }
    if (const int32_t x = XboxDe(bt)) {
      j.inyecta = x;
      j.inyecta_t = 4;
    } else {
      p = uint16_t(1u << bt);
    }
  }
  if (p && j.actual < j.n) {
    if (p & (1u << j.ic[j.actual].boton)) {
      j.ic[j.actual].resuelto = true;
      ++j.actual;
      j.juicio = -1;
      EventoPose(j, kEvGolpe);
    } else {
      j.juicio = 3;   // error: el boton malo no avanza
      j.juicio_t = 30;
      EventoPose(j, kEvMal);
    }
  }
  if (j.actual < j.n) return;
  REXLOG_INFO("dbz3 r14: ronda {} (fila {}): {:.2f} s", j.ronda + 1, fila, j.nota[j.ronda]);
  if (++j.ronda < 10) {
    EmpezarRonda(j);
    j.fase = 1;
    j.t = 0;
    return;
  }
  float total = 0;
  int lentas = 0;
  for (float v : j.nota) {
    total += v;
    lentas += v >= 7.0f;
  }
  j.total = int(total * 100);   // centesimas
  j.rango = 0;
  if (lentas < 10)
    for (int r = 5; r >= 1; --r)
      if (total <= kUmbralT[r]) {
        j.rango = r;
        break;
      }
  const float v = j.rango == 0 ? 100.0f : j.pts[j.rango] * j.mult[j.dif];
  j.puntos = (int(v) + 99) / 100 * 100;
  j.fase = 3;
  j.t = 0;
  REXLOG_INFO("dbz3 r14: fin: total {:.2f} s, rango {}, puntos {}", total, j.rango, j.puntos);
}

void PasoBarrido(JuegoB& j, uint16_t pulsado) {
  j.cursor += j.sentido * j.vel;   // Bounce_UpdateMove
  if (j.cursor >= j.b1 || j.cursor <= j.b0) {
    j.cursor = std::clamp(j.cursor, j.b0, j.b1);
    j.sentido = -j.sentido;
    ++j.rebotes;
  }
  int bajo = -1;   // el objeto que pisa el cursor
  for (int i = 0; i < j.n; ++i) {
    if (j.ic[i].vivo && !j.ic[i].resuelto && std::fabs(j.ic[i].x + kIcono / 2 - j.cursor) < kIcono / 2) bajo = i;
    else j.ic[i].pedido = false;   // prueba: si se le escapo, lo vuelve a pedir en la pasada siguiente
  }
  uint16_t p = pulsado & 0xFF00;
  if (j.auto_ && bajo >= 0 && !j.ic[bajo].pedido && !(j.fallos && j.ronda == 1)) {   // fallos: la fila 2 no se coge
    j.ic[bajo].pedido = true;
    if (const int32_t x = XboxDe(j.ic[bajo].boton)) {
      j.inyecta = x;
      j.inyecta_t = 4;
    } else {
      p = uint16_t(1u << j.ic[bajo].boton);
    }
  }
  if (p && bajo >= 0 && (p & (1u << j.ic[bajo].boton))) {
    j.ic[bajo].resuelto = true;
    j.juicio = -1;
  }
  bool quedan = false;
  for (int i = 0; i < j.n; ++i) quedan |= !j.ic[i].resuelto;
  if (quedan && j.rebotes <= 7) return;
  const int juicio = j.rebotes <= 2 ? 0 : j.rebotes <= 4 ? 1 : j.rebotes <= 7 ? 2 : 3;
  constexpr int kPts[4] = {10, 5, 3, 0};
  j.nota[j.ronda] = float(kPts[juicio]);
  j.juicio = juicio;
  j.juicio_t = 40;
  EventoPose(j, quedan ? kEvMal : kEvGolpe);   // fila completa (estado 1) o fallada (estado 6)
  REXLOG_INFO("dbz3 r14: fila {}: {} rebotes, {} puntos", j.ronda + 1, j.rebotes, kPts[juicio]);
  if (++j.ronda < 10) {
    EmpezarRonda(j);
    j.fase = 1;
    j.t = 0;
    return;
  }
  j.total = 0;
  for (float v : j.nota) j.total += int(v);
  constexpr int kUmbral19[6] = {0, 60, 70, 80, 90, 100};   // (umbral[k], umbral[k+1]] -> indice k (0..4)
  int indice = -1;
  for (int k = 0; k < 5; ++k)
    if (j.total > kUmbral19[k] && j.total <= kUmbral19[k + 1]) indice = k;
  j.rango = indice + 1;   // como los otros: 0 = no superado, 1..5
  const int v = indice < 0 ? 100 : int(j.pts[indice + 1] * j.mult[j.dif]);
  j.puntos = indice < 0 ? 100 : (v % 100 < 50 ? v / 100 : v / 100 + 1) * 100;   // Results_ComputeTotalsAndRewards
  j.fase = 3;
  j.t = 0;
  REXLOG_INFO("dbz3 r14: fin: total {} rango {} puntos {}", j.total, j.rango, j.puntos);
}

void PasoB(JuegoB& j, uint16_t pulsado) {
  ++j.cuadros;
  if (j.juicio_t > 0) --j.juicio_t;
  if (j.m->tipo == kBarrido) {
    PasoBarrido(j, pulsado);
    return;
  }
  if (j.m->teclear) {
    PasoTeclear(j, pulsado);
    return;
  }
  const int fila = j.orden[j.ronda];
  if (--j.cuenta <= 0 && j.sale < j.n) {   // Mg_UpdateScrollingIcons
    j.ic[j.sale] = Icono{j.seq[fila][j.sale], -16 - j.vel, true};
    j.cuenta = j.hueco[fila][j.sale];
    ++j.sale;
  }
  bool quedan = false;
  float mejor = 1e9f;
  j.actual = -1;
  for (int i = 0; i < j.sale; ++i) {
    Icono& k = j.ic[i];
    if (!k.vivo) continue;
    k.x += j.vel;
    if (k.x >= kW + 56) {
      k.vivo = false;
      continue;
    }
    quedan = true;
    const float d = std::fabs(k.x + kIcono / 2 - kObjB);
    if (!k.resuelto && d < mejor) {   // el mas cercano al objetivo sin juzgar
      mejor = d;
      j.actual = i;
    }
  }
  uint16_t p = pulsado & 0xFF00;
  if (j.auto_ && j.actual >= 0 && !j.ic[j.actual].pedido && Juicio(j.ic[j.actual].x) == 0 &&
      j.ic[j.actual].x + kIcono / 2 >= kObjB - 2) {
    Icono& k = j.ic[j.actual];
    k.pedido = true;
    int bt = k.boton;
    if (j.fallos && j.ronda == 1 && !j.mal_hecho) {   // prueba: un boton malo
      j.mal_hecho = true;
      bt = bt == 15 ? 14 : 15;
    }
    if (const int32_t x = XboxDe(bt)) {
      j.inyecta = x;
      j.inyecta_t = 4;
    } else {
      p = uint16_t(1u << bt);
    }
  }
  if (p && j.actual >= 0 && j.ic[j.actual].x >= kObjB - kObjAncho / 2 - 80) {   // Mg_UpdateQteInput
    Icono& k = j.ic[j.actual];
    k.resuelto = true;
    if (p & (1u << k.boton)) {
      j.juicio = Juicio(k.x);
      if (j.juicio < 3) j.nota[j.ronda] += float(kJuicioPts[j.juicio] / j.n);
      k.vivo = false;
      EventoPose(j, kEvGolpe);
    } else {
      j.juicio = 3;   // boton malo: fallado
      EventoPose(j, kEvMal);
    }
    j.juicio_t = 30;
  }
  if (!quedan && j.sale >= j.n) {   // fin de la ronda (Mg_StoreRoundScore)
    REXLOG_INFO("dbz3 r14: ronda {} (fila {}): {}", j.ronda + 1, fila, int(j.nota[j.ronda]));
    if (++j.ronda < 10) {
      EmpezarRonda(j);
      j.fase = 1;   // presentacion de la ronda siguiente
      j.t = 0;
      return;
    }
    j.total = 0;
    for (float v : j.nota) j.total += int(v);
    j.rango = 0;
    for (int r = 5; r >= 1; --r)
      if (j.total >= kUmbralB[r]) {
        j.rango = r;
        break;
      }
    const float v = j.rango == 0 ? 100.0f : j.pts[j.rango] * j.mult[j.dif];
    j.puntos = (int(v) + 99) / 100 * 100;
    j.fase = 3;
    j.t = 0;
    REXLOG_INFO("dbz3 r14: fin: total {} rango {} puntos {}", j.total, j.rango, j.puntos);
  }
}

// Flecha de la cruceta (codigo 4 arriba, 5 derecha, 6 abajo, 7 izquierda): la de la izquierda de "flechas" (5,8-37,42)
// girada. Sin la pieza, un triangulo.
void Flecha(ImDrawList* dl, int boton, ImVec2 c, float r, ImU32 col) {
  constexpr float kAng[4] = {1.5707963f, 3.1415927f, -1.5707963f, 0.0f};   // apuntando a la izquierda = 0
  const float a = kAng[(boton - 4) & 3], ca = std::cos(a), sa = std::sin(a);
  auto rot = [&](float x, float y) { return ImVec2(c.x + x * ca - y * sa, c.y + x * sa + y * ca); };
  const S::Pic p = S::Img("flechas");
  if (!p) {
    dl->AddTriangleFilled(rot(-r, 0), rot(r, -r), rot(r, r), col);
    return;
  }
  const ImVec2 uv0(5 / p.w, 8 / p.h), uv1(37 / p.w, 42 / p.h);
  dl->AddImageQuad(p.tex, rot(-r, -r), rot(r, -r), rot(r, r), rot(-r, r), uv0, ImVec2(uv1.x, uv0.y), uv1,
                   ImVec2(uv0.x, uv1.y), col);
}

void PiezaB(ImDrawList* dl, const ps2::Tex& t, ImVec2 c, float k) {
  if (t) dl->AddImage(t.id(), ImVec2(c.x - t.w * k / 2, c.y - t.h * k / 2), ImVec2(c.x + t.w * k / 2, c.y + t.h * k / 2));
}

class PanelB : public rex::ui::ImGuiDialog {
 public:
  explicit PanelB(rex::ui::ImGuiDrawer* drawer) : ImGuiDialog(drawer) {}

 protected:
  void OnDraw(ImGuiIO& io) override {
    if (!g_corre_b.load()) return;
    std::shared_ptr<JuegoB> j;
    {
      std::lock_guard<std::mutex> lk(g_mu);
      j = g_jb;
    }
    if (!j) return;
    uint8_t* base = Base();
    const int est = P::EstadoCombate();   // el final de B3 (5-8) corre bajo el resultado y se tapa con negro (como PanelA)
    if (!base || (est != 4 && !(j->fase == 3 && est >= 5 && est <= 8))) return;
    const uint32_t mgr = REX_LOAD_U32(kMgrPtr);
    if (!mgr) return;
    if (j.get() != hecho_) Preparar(*j);
    const uint32_t x0 = mgr + kX0, x1 = x0 + kXPaso;
    for (uint32_t x : {x0, x1}) StoreF(base, x + kVida, LoadF(base, x + kVidaMax));
    REX_STORE_U8(x1 + 3261, 1);   // el pivote sin cuerpo (como P4: sub_82109D90, tagteam.cpp Veil)
    REX_STORE_U8(x1 + 4925, 1);
    const bool pausa = dbz3::GameDimmed() || dbz3::OverlayOpen();
    // botones (+12) y, para la cruceta de NB007, la direccion de la palanca de +108 (0x10 arriba, 0x20 derecha, 0x40
    // abajo, 0x80 izquierda, medido por P4: los mismos bits que los codigos 4..7 de IW). La cruceta de la HD llega como
    // palanca en los minijuegos (net/netplay_hooks.cpp); +12 no lleva direcciones (sonda de R14).
    const uint16_t ahora = uint16_t(REX_LOAD_U16(kPads + kPadBotones) | (REX_LOAD_U16(kPads + 108) & 0x00F0));
    j->pendiente = uint16_t(j->pendiente | (ahora & ~j->antes));
    j->antes = ahora;
    acum_ += pausa ? 0.0f : std::min(io.DeltaTime, 0.1f);
    while (acum_ >= 1.0f / 60) {
      acum_ -= 1.0f / 60;
      ++j->t;
      CuadroPose(*j);
      if (j->pb) {   // poses de IW: espera, READY y (MSN019) el tramo de filas; y sus camaras
        if (j->fase == 0 && j->t == 1) {
          CamB(j->pb->cam_ini);
          if (j->m->tipo == kBarrido) EventoPose(*j, kEvNivel);
          else PonerB(*j->pb, j->pb->espera[0], true);
        } else if (j->fase == 1 && j->ronda == 0 && j->t == 1) {
          CamB(j->pb->cam_intro);
          EventoPose(*j, kEvIntro);
        } else if (j->fase == 1 && j->ronda > 0 && j->ronda <= 9 && j->t == 1) {
          CamB(j->pb->cam_ronda[j->ronda - 1]);
        } else if (j->fase == 2 && j->t == 1 && j->m->tipo == kBarrido && j->ronda > 0) {
          EventoPose(*j, kEvNivel);
        }
      }
      if (j->fase <= 1) j->pendiente = 0;
      if (j->fase == 0 && j->t >= 30) {
        j->fase = 1;
        j->t = 0;
        REXLOG_INFO("dbz3 r14: en juego ({} dificultad {})", j->m->nombre, j->dif);
      } else if (j->fase == 1 && j->t >= ((j->ronda == 0 && j->pb) ? j->pb->dur_intro : 60)) {   // READY? (la primera) o el numero de ronda
        j->fase = 2;
        j->t = 0;
      } else if (j->fase == 2) {
        PasoB(*j, j->pendiente);
        j->pendiente = 0;
        if (j->auto_) {
          dbz3::net::InyectarPad(0, j->inyecta_t > 0 ? j->inyecta : 0);
          if (j->inyecta_t > 0 && --j->inyecta_t == 0) j->inyecta = 0;
        }
      } else if (j->fase == 3 && j->t == 1) {
        if (std::getenv("DBZ3_IWMINI_RAFAGA_FIN")) std::ofstream("dbz3_shot.req") << "40 15";   // prueba
        if (j->auto_) dbz3::net::InyectarPad(0, -1);
        if (j->pb) CamB(j->rango > 0 ? j->pb->cam_gana : j->pb->cam_pierde);
        EventoPose(*j, kEvFin);
        if (!fin_) {
          fin_ = true;
          REX_STORE_U8(mgr + kFin, uint8_t(j->rango > 0 ? 1 : 2));
        }
      }
    }
    Dibujar(io, *j);
  }

 private:
  void Preparar(const JuegoB& j) {
    hecho_ = &j;
    fin_ = false;
    acum_ = 0;
    objetivo_ = ps2::Crear(j.objetivo);
    ready_ = ps2::Crear(j.ready);
    start_ = ps2::Crear(j.start);
    clear_ = ps2::Crear(j.clear);
    failed_ = ps2::Crear(j.failed);
    aro_ = ps2::Crear(j.aro);
    for (int k = 0; k < 4; ++k) palabra_[k] = ps2::Crear(j.palabra[k]);
  }
  void Dibujar(ImGuiIO& io, const JuegoB& j) {
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    const S::Area a = S::GameArea(io);
    auto at = [&](float x, float y) { return a.At(x * 1200 / kW, y * 900 / kH); };
    const float k = a.s * 900 / kH;
    // cara del jugador (aro del HUD de IW 6/0)
    {   // F1: cara del que juega (forma 0, la del combate)
      const ImVec2 c0 = a.At(28, 30);
      dbz3::iwmini::PintarCara(fg, c0.x, c0.y, 64 * 1.6f * a.s, j.m->jugador, 0);
    }
    if (aro_) fg->AddImage(aro_.id(), a.At(6, 8), a.At(6 + 88 * 1.6f, 8 + 88 * 1.6f));
    // lista de las 10 rondas (captura 88). ponytail: sus piezas (cifras y marco de 4/0) sin colocar; letra del menu
    for (int r = 0; r < 10; ++r) {
      char txt[32];
      if ((r < j.ronda || (r == j.ronda && j.fase == 3)) && j.m->teclear)
        std::snprintf(txt, sizeof txt, "%2d  %5.2f\"", r + 1, j.nota[r]);
      else if (r < j.ronda || (r == j.ronda && j.fase == 3))
        std::snprintf(txt, sizeof txt, "%2d   %3d", r + 1, int(j.nota[r]));
      else
        std::snprintf(txt, sizeof txt, "%2d   ---", r + 1);
      S::MenuText(fg, a.At(1060, 150 + 44 * r), 34 * a.s, txt, r == j.ronda && j.fase < 3, r > j.ronda);
    }
    if (j.fase == 1) {
      if (j.ronda == 0) PiezaB(fg, ready_, at(kW / 2, 200), k);   // las demas: la fila de la lista se ilumina
    }
    if (j.fase == 2 && j.t < 60) PiezaB(fg, start_, at(kW / 2, 200), k);
    if (j.fase == 2) {
      const float y = kYB + kIcono / 2;
      fg->AddRectFilled(at(0, y - 18), at(kW, y + 18), IM_COL32(0, 0, 0, 110));
      if (j.m->tipo == kBarrido) {   // la barra y el cursor (objeto 0): el aro de IW
        fg->AddRectFilled(at(j.b0, y - 4), at(j.b1, y + 4), IM_COL32(255, 255, 255, 120));
        if (objetivo_) {
          const ImVec2 c = at(j.cursor, y);
          fg->AddImage(objetivo_.id(), ImVec2(c.x - 38 * k, c.y - 28 * k), ImVec2(c.x + 34 * k, c.y + 28 * k));
        }
      }
      if (objetivo_ && j.m->tipo == kDesplazar) {
        const ImVec2 c = at(kObjB, y);
        fg->AddImage(objetivo_.id(), ImVec2(c.x - 38 * k, c.y - 28 * k), ImVec2(c.x + 34 * k, c.y + 28 * k));
      }
      const S::Pic pic = S::Img("botones");
      for (int i = 0; i < j.sale; ++i) {
        const Icono& ic = j.ic[i];
        if (!ic.vivo) continue;
        if (j.m->tipo == kBarrido && ic.resuelto) continue;   // cogido: se va
        const ImU32 col = ic.resuelto ? IM_COL32(120, 120, 120, 200) : IM_COL32(255, 255, 255, 255);
        // NB007: la fila quieta centrada (48 px de paso) y el aro en el que toca
        const float x = j.m->teclear ? kW / 2 + (i - (j.n - 1) / 2.0f) * 48 : ic.x + kIcono / 2;
        const ImVec2 c = at(x, y);
        if (j.m->teclear && i == j.actual && objetivo_)
          fg->AddImage(objetivo_.id(), ImVec2(c.x - 38 * k, c.y - 28 * k), ImVec2(c.x + 34 * k, c.y + 28 * k));
        if (!pic) {
          S::Glyph(fg, kNombre[ic.boton & 15], ImVec2(c.x - kIcono * k / 2, c.y), kIcono * k);
          continue;
        }
        if (ic.boton < 8) {   // cruceta: la flecha de los menus de B3 HD ("flechas", data_usi 2062) girada
          Flecha(fg, ic.boton, c, kIcono * k * 0.5f, col);
          continue;
        }
        const ImVec4 r = Glifo(ic.boton);
        const float h = kIcono * k * 1.1f, w = (r.z - r.x) * h / (r.w - r.y);
        S::DrawPic(fg, pic, ImVec2(c.x - w / 2, c.y - h / 2), ImVec2(c.x + w / 2, c.y + h / 2), r, col);
      }
      if (j.juicio_t > 0 && j.juicio >= 0) PiezaB(fg, palabra_[j.juicio], at(j.m->teclear ? kW / 2 : kObjB, kYB - 40), k);
    }
    if (j.fase == 3) {
      const float t = std::min(1.0f, j.t / 30.0f);
      fg->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(0, 0, 0, int(160 * t)));
      PiezaB(fg, j.rango > 0 ? clear_ : failed_, at(kW / 2, 200), k * 1.5f * (0.8f + 0.2f * t));
      if (const float neg = std::clamp((j.t - 228) / 36.0f, 0.0f, 1.0f); neg > 0)   // a negro hasta el mapa
        fg->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(0, 0, 0, int(255 * neg)));
    }
  }
  const JuegoB* hecho_ = nullptr;
  bool fin_ = false;
  float acum_ = 0;
  ps2::Tex objetivo_, ready_, start_, clear_, failed_, aro_, palabra_[4];
};

void CreatePanelB() {
  static PanelB* panel = nullptr;
  auto* rt = rex::Runtime::instance();
  if (!panel && rt && rt->imgui_drawer()) panel = new PanelB(rt->imgui_drawer());
}

bool EmpezarRondas(int clave, int fila, int dificultad) {
  const ModuloB* m = nullptr;
  for (const ModuloB& x : kModulosB)
    if (x.clave == clave) m = &x;
  if (!m) return false;
  auto j = std::make_shared<JuegoB>();
  j->m = m;
  j->clave = clave;
  j->fila = fila;
  j->dif = std::clamp(dificultad, 0, 5);
  j->vel = m->velocidad[j->dif];
  j->pb = m->poses;
  const char* t = std::getenv("DBZ3_TEST_IWMINI");
  j->auto_ = t && std::strstr(t, "auto");
  j->fallos = t && std::strstr(t, "fallos");
  std::string error;
  if (!CargarB(*j, error)) {
    REXLOG_WARN("dbz3 r14: {}: {}", m->nombre, error);
    return false;
  }
  EmpezarRonda(*j);
  g_cb_clip = -1;
  if (m->poses && !std::getenv("DBZ3_IWMINI_CAMARA_B3")) PrecargarCamaras(*m->poses);   // (DBZ3_IWMINI_CAMARA_B3: sin ellas)
  REXLOG_INFO("dbz3 r14: {} (clave 0x{:X}, dificultad {}): rondas {} {} {} {} {} {} {} {} {} {}, {} botones, velocidad {}",
              m->nombre, clave, j->dif, j->orden[0], j->orden[1], j->orden[2], j->orden[3], j->orden[4], j->orden[5],
              j->orden[6], j->orden[7], j->orden[8], j->orden[9], j->n, j->vel);
  {
    std::lock_guard<std::mutex> lk(g_mu);
    g_jb = j;
  }
  dbz3::launcher::DeferUi(&CreatePanelB);
  const int escenario = std::max(P::EscenarioPorMapa(P::MapaIW(m->escenario_iw)), 0);
  if (!dbz3::hub::RequestBattleEx(m->jugador, 18, escenario, true, -1, 0, 0, 0, 0)) {   // 18: pivote invisible (P4)
    REXLOG_WARN("dbz3 r14: el conector no acepto el combate");
    return false;
  }
  dbz3::combate::SetMuneco(1);
  dbz3::combate::OcultarHud(true);
  g_corre_b = true;
  P::Reglas r;
  r.capsulas = false;
  P::VigilarCombate("r14", r, [j](const P::Fin&) {
    g_corre_b = false;
    g_cb_clip = -1;
    if (j->auto_) dbz3::net::InyectarPad(0, -1);
    dbz3::combate::SetMuneco(-1);
    dbz3::combate::OcultarHud(false);
    Resultado res;
    res.clave = j->clave;
    res.fila = j->fila;
    res.superado = j->fase == 3 && j->rango > 0;
    res.centesimas = j->cuadros * 100 / 60;
    res.puntos = j->fase == 3 ? j->puntos : 0;
    res.rango = std::max(j->rango - 1, 0);
    res.monedas = j->total;
    REXLOG_INFO("dbz3 r14: resultado {} superado {} rango {} puntos {}", j->m->nombre, res.superado, res.rango,
                res.puntos);
    Terminar(res);
  });
  return true;
}

const bool g_registro = [] {
  for (const Modulo2& m : kModulos) RegistrarModulo(m.clave, &Empezar);
  for (const ModuloB& m : kModulosB) RegistrarModulo(m.clave, &EmpezarRondas);
  return true;
}();

}  // namespace

// Gancho del mando (iw_minijuegos.cpp Mando, antes y despues de leer el mando 0): los botones los lleva el minijuego (los
// ve por g_raw12) y el luchador de B3 no los ve (ni golpea ni anda: solo START y SELECT); las poses son las de IW. Antes
// de cada lectura se le devuelve al juego el registro como lo dejo (como MandoVuelo, iw_mini_vuelo.cpp).
// Hilo del juego: el sonido de golpe que toca (iw_minijuegos.cpp, gancho de sub_82114910).
bool SeBotones(int* banco, int* id) {
  const int v = g_se_listo.exchange(-1);
  if (v < 0) return false;
  *banco = v / 1000;
  *id = v % 1000;
  return true;
}

void MandoBotones(uint8_t* base, uint32_t usuario, bool leido) {
  struct Dejado {
    bool hay = false;
    uint16_t b12 = 0, b14 = 0, d108 = 0, d110 = 0;
    uint8_t p26 = 0, p27 = 0;
    uint32_t b16 = 0, cuenta[16] = {};
  };
  static Dejado g;
  if (usuario != 0) return;
  const uint32_t r = kPads;
  if (!leido) {
    if (g.hay) {
      REX_STORE_U16(r + 12, g.b12);
      REX_STORE_U16(r + 14, g.b14);
      REX_STORE_U16(r + 108, g.d108);
      REX_STORE_U16(r + 110, g.d110);
      REX_STORE_U8(r + 26, g.p26);
      REX_STORE_U8(r + 27, g.p27);
      REX_STORE_U32(r + 16, g.b16);
      for (uint32_t k = 0; k < 16; ++k) REX_STORE_U32(r + 0x28 + 4 * k, g.cuenta[k]);
      g.hay = false;
    }
    return;
  }
  if (!g_corre.load()) {
    g_raw_ok = false;
    return;
  }
  g.hay = true;
  g.b12 = REX_LOAD_U16(r + 12);
  g.b14 = REX_LOAD_U16(r + 14);
  g.d108 = REX_LOAD_U16(r + 108);
  g.d110 = REX_LOAD_U16(r + 110);
  g.p26 = REX_LOAD_U8(r + 26);
  g.p27 = REX_LOAD_U8(r + 27);
  g.b16 = REX_LOAD_U32(r + 16);
  for (uint32_t k = 0; k < 16; ++k) g.cuenta[k] = REX_LOAD_U32(r + 0x28 + 4 * k);
  g_raw12 = g.b12;
  g_raw_ok = true;
  constexpr uint16_t kDeja = 0x0009;   // SELECT 0x01 y START 0x08 (la pausa); el resto son golpes, ki, guardia...
  REX_STORE_U16(r + 12, uint16_t(g.b12 & kDeja));
  REX_STORE_U16(r + 14, uint16_t(g.b14 & kDeja));
  REX_STORE_U16(r + 108, 0);
  REX_STORE_U16(r + 110, 0);
  REX_STORE_U8(r + 26, 0);
  REX_STORE_U8(r + 27, 0);
  REX_STORE_U32(r + 16, g.b16 & (uint32_t(kDeja) << 16 | kDeja));
  for (uint32_t k = 0; k < 16; ++k)   // los botones quitados, "sueltos" (+92 = -1, como P4 con el circulo)
    if (!(kDeja >> k & 1) && int32_t(g.cuenta[k]) > 0) REX_STORE_U32(r + 0x28 + 4 * k, uint32_t(-1));
}

// Hilo del juego (Fotograma de iw_minijuegos.cpp, al empezar el dibujo, cuando los luchadores y la camara estan puestos): la
// camara de los botones. IW lleva una propia (UiAnim 0x4B0/0x1A4, sin leer): de lado, con los dos luchadores a los lados
// de la pantalla (el jugador a la izquierda) y a la altura a la que luchan en el aire (kAire).
// Familia B: la camara de IW del clip en curso (CamB) alrededor del luchador, que mira a +Z como en IW.
static void PasoCamaraB(uint8_t* base) {
  const int id = g_cb_clip.load();
  const auto it = id >= 0 ? g_clips.find(id) : g_clips.end();
  if (it == g_clips.end()) return;
  const uint32_t mgr = REX_LOAD_U32(kMgrPtr);
  if (!mgr) return;
  const uint32_t st = REX_LOAD_U32(mgr + 35128);
  if (st < 3 || st > 8) return;
  const ClipCam& c = it->second;
  const float f = std::min(float(g_cb_frame.load()), float(c.nf - 1));
  const uint32_t x0 = mgr + kX0, x1 = x0 + kXPaso;
  const float px = LoadF(base, x0 + 64), py = LoadF(base, x0 + 68), pz = LoadF(base, x0 + 72);
  for (uint32_t o : {48u, 64u, 80u}) {   // el rival-pivote a +Z: el luchador mira donde miraba en IW (tagteam.cpp StepSlide)
    StoreF(base, x1 + o, px);
    StoreF(base, x1 + o + 8, pz + 30.0f);
  }
  float e[3], t[3], fov = 0.6f;
  Muestra(c.ojo, f, e);
  Muestra(c.obj, f, t);
  if (!c.fov.empty()) Muestra(c.fov, f, &fov);
  const float ojo[3] = {px + e[0], py + e[1], pz + e[2]}, obj[3] = {px + t[0], py + t[1], pz + t[2]};
  dbz3::cine::Camara(base, ojo, obj);
  StoreF(base, mgr + 0x1C4, std::clamp(fov, 0.2f, 1.3f));
}

// Familia A: la camara de IW del clip en curso, anclada a los dos luchadores. En IW los dos objetos estan en el origen y su
// separacion sale de la raiz de las poses; el clip se hizo para ese par (centro y eje jugador -> rival en `ancla`). Se lleva
// al mundo de B3 con el centro = punto medio de los objetos, el eje = jugador -> rival y el suelo = y del jugador.
static bool PasoCamaraA(uint8_t* base, uint32_t mgr, const Poses& p) {
  const int id = g_cb_clip.load();
  const auto it = id >= 0 ? g_clips.find(id) : g_clips.end();
  if (it == g_clips.end()) return false;
  const ClipCam& c = it->second;
  const float f = std::min(float(g_cb_frame.load()), float(c.nf - 1));
  const uint32_t x0 = mgr + kX0, x1 = x0 + kXPaso;
  const float ax = LoadF(base, x0 + 64), ay = LoadF(base, x0 + 68), az = LoadF(base, x0 + 72), bx = LoadF(base, x1 + 64),
              bz = LoadF(base, x1 + 72);
  float ux = bx - ax, uz = bz - az;
  const float l = std::hypot(ux, uz);
  if (l < 1e-4f) return false;
  ux /= l, uz /= l;
  static const float rot = std::getenv("DBZ3_R14_ROT") ? float(std::atof(std::getenv("DBZ3_R14_ROT"))) * 3.14159265f / 180 : 0.0f;
  const float mx = (ax + bx) / 2, mz = (az + bz) / 2;
  // giro alrededor de Y que lleva el eje de IW al eje del par (el angulo de un vector, de +X hacia +Z, baja lo que sube el giro)
  const float th = std::atan2(p.ancla[3], p.ancla[2]) - std::atan2(uz, ux) + rot, ct = std::cos(th), st = std::sin(th);
  auto mundo = [&](const float v[3], float out[3]) {
    const float dx = v[0] - p.ancla[0], dz = v[2] - p.ancla[1];
    out[0] = mx + dx * ct + dz * st;
    out[1] = ay + v[1];
    out[2] = mz - dx * st + dz * ct;
  };
  float e[3], t[3], ojo[3], obj[3], fov = 0.6f;
  Muestra(c.ojo, f, e);
  Muestra(c.obj, f, t);
  if (!c.fov.empty()) Muestra(c.fov, f, &fov);
  mundo(e, ojo);
  mundo(t, obj);
  dbz3::cine::Camara(base, ojo, obj);
  StoreF(base, mgr + 0x1C4, std::clamp(fov, 0.2f, 1.3f));
  return true;
}

void PasoBotones(uint8_t* base) {
  if (g_corre_b.load()) {
    PasoCamaraB(base);
    return;
  }
  if (!g_corre.load()) return;
  const uint32_t mgr = REX_LOAD_U32(kMgrPtr);
  if (!mgr) return;
  const uint32_t st = REX_LOAD_U32(mgr + 35128);
  if (st < 3 || st > 8) return;   // el final de B3 (5-8) tambien: la camara del resultado se mantiene
  if (const Poses* pp = g_poses_a.load(); pp && g_cb_clip.load() >= 0 && PasoCamaraA(base, mgr, *pp)) return;
  const uint32_t x0 = mgr + kX0, x1 = x0 + kXPaso;
  const float ax = LoadF(base, x0 + 64), az = LoadF(base, x0 + 72), bx = LoadF(base, x1 + 64), bz = LoadF(base, x1 + 72);
  float dx = bx - ax, dz = bz - az;
  const float l = std::hypot(dx, dz);
  if (l < 1e-3f) return;
  dx /= l, dz /= l;
  // lado de la camara: f = -s mirando al punto medio; la derecha de la pantalla (f x arriba) debe ir de A a B
  float sx = -dz, sz = dx;
  const float fx = -sx, fz = -sz;
  if ((-fz) * dx + fx * dz < 0) sx = -sx, sz = -sz;   // f x (0,1,0) = (-fz, 0, fx)
  const bool foco = g_cam_lado.load() < 0;   // la pose final: el jugador solo
  const float mx = foco ? ax : (ax + bx) / 2, mz = foco ? az : (az + bz) / 2, y = LoadF(base, x0 + 68);
  static float alto = 14, dist = 62;
  if (g_cam_reset.exchange(false)) alto = g_cam_alto.load(), dist = g_cam_dist.load();
  alto += (g_cam_alto.load() - alto) * 0.06f;
  dist += (g_cam_dist.load() - dist) * 0.06f;
  const float lado = g_cam_lado.load();
  const float ojo[3] = {mx + sx * dist * lado, y + alto + 6, mz + sz * dist * lado}, obj[3] = {mx, y + alto, mz};
  dbz3::cine::Camara(base, ojo, obj);
}
}  // namespace dbz3::iwmini
