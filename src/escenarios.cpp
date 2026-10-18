// dbz3 - ESCENARIOS NUEVOS (altas) y sonda de colision. Ver escenarios.h. US y EU (src/eu_map.h).
//
// Mapa del juego (US):
//   0x82323760  17 registros x 92 B (+0x00 nombre, +0x10 id_mapa, +0x14 id_SE, +0x18 variante,
//               +0x1C/+0x3C dos grupos de objetos de 8 palabras); 0x82323D80 temas (17 + 0).
//   sub_820C26E0(obj): carga el escenario obj+340 (s16): id_mapa -> AFS, G+0 (0x823A3D80) = indice.
//   sub_820C19B0(i):   id_SE del registro i (lo pide sub_821221F8 con el mismo obj+340).
//   Todo lo demas lee el registro con G+0. Por eso basta con traducir en esos dos ganchos el alta
//   (17+) al anfitrion y tener el registro del anfitrion con los datos del alta mientras este cargada.

#include "escenarios.h"

#include "clima.h"
#include "launcher/settings.h"
#include "mods.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#include <rex/hook.h>
#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/system/xmemory.h>

#include "generated/dbz3_init.h"
#include "eu_map.h"

// Modo opcional "zonas B de B2" (frente 07): al caer por el borde se pasa a la zona B del escenario
// (el mod de la zona B lo declara en su stage.toml con `zona_b = <entrada del escenario A>`).
REXCVAR_DEFINE_BOOL(dbz3_escenarios_zonas_b, false, "DBZ3/Mods",
                    "Al caer por el borde se pasa a la zona B del escenario (si su mod la trae; "
                    "stage.toml `zona_b`; desactivado = el ring out acaba el combate como siempre)");

namespace dbz3::escenarios {
namespace {

constexpr dbz3::GA kRecs = 0x82323760, kTemas = 0x82323D80;
constexpr uint32_t kRec = 92;
constexpr int kAnfitrion = 16;   // R&R BASE: grupos de objetos vacios y tema generico

// Ring out (medido, tagteam.cpp DBZ3_TEST_RINGOUT): el aviso mgr+35569 = hueco del ganador + 1 lo pone
// el juego al caer alguien del ring y sub_820FD678 (cada fotograma de la pelea) lo da como fin (motivo 4).
constexpr dbz3::GA kMgrPtr = 0x823D6CF4;
constexpr uint32_t kAvisoRing = 35569;
constexpr uint32_t kEstadoCombate = 35128;   // mgr+35128: 4 = pelea, 5 = fin (modos.cpp, tagteam.cpp)

struct Alta {
  std::string mod, nombre, titulo;   // nombre: el del registro (<= 15); titulo: el del rotulo
  uint32_t entrada = 0, id_se = 45, menu = 0;   // menu: #AZT con la bola, la vista y el rotulo (selector)
  int tema = 16;
  bool niebla = false;   // R8: niebla propia (campos de IW: la del #MCF), "RRGGBBAA,inicio,fin" en stage.toml
  uint32_t niebla_color = 0;
  float niebla_ini = 0, niebla_fin = 0;
  uint32_t zona_b = 0;   // frente 07: esta alta es la zona B del alta cuya `entrada` es este numero (0 = no)
  bool oculta = false;   // fuera del selector del Duelo (campos de IW: solo para sus minijuegos); `selector = false`
};
rex::memory::Memory* g_mem = nullptr;
std::vector<Alta> g_altas;
uint32_t g_visibles = 0;   // altas con casilla en el selector: las primeras de g_altas (las ocultas van al final)
int g_activa = -1;               // alta puesta en el registro anfitrion (-1 = ninguna)
uint8_t g_orig[kRec];            // registro original del anfitrion
uint32_t g_orig_tema = 0;
constexpr dbz3::GA kNieblas = 0x823232A0;   // entradas de niebla de 20 B (color, 2 floats, inicio, fin): clima.cpp
uint32_t g_niebla = 0;                       // entrada de niebla prestada mientras el alta esta puesta (0 = ninguna)
uint8_t g_orig_niebla[20];
uint32_t g_obj_combate = 0;                  // obj del escenario del combate (sub_820C26E0): para recargarlo

std::string Limpia(std::string s) {
  const auto h = s.find('#');
  if (h != std::string::npos && s.find('"') == std::string::npos) s.resize(h);
  while (!s.empty() && std::strchr(" \t\r", s.front())) s.erase(0, 1);
  while (!s.empty() && std::strchr(" \t\r", s.back())) s.pop_back();
  return s;
}

std::string Valor(const std::string& v) {
  std::string s = Limpia(v);
  if (!s.empty() && s.front() == '"') {
    const auto z = s.find('"', 1);
    return s.substr(1, z == std::string::npos ? std::string::npos : z - 1);
  }
  const auto h = s.find('#');
  return Limpia(h == std::string::npos ? s : s.substr(0, h));
}

void Restaurar(uint8_t* base) {
  if (g_activa < 0) return;
  std::memcpy(base + kRecs + kAnfitrion * kRec, g_orig, kRec);
  REX_STORE_U32(kTemas + 4 * kAnfitrion, g_orig_tema);
  if (g_niebla) std::memcpy(base + g_niebla, g_orig_niebla, sizeof(g_orig_niebla));
  g_niebla = 0;
  g_activa = -1;
}

void Activar(uint8_t* base, int v) {
  if (g_activa == v) return;
  Restaurar(base);
  const Alta& a = g_altas[size_t(v - kSerie)];
  const uint32_t rec = kRecs + kAnfitrion * kRec;
  std::memcpy(g_orig, base + rec, kRec);
  g_orig_tema = REX_LOAD_U32(kTemas + 4 * kAnfitrion);
  const uint32_t tema = REX_LOAD_U32(kTemas + 4 * uint32_t(a.tema));
  const uint32_t ni = REX_LOAD_U32(rec + 0x1C + 4);   // entrada de niebla del anfitrion (grupo 0)
  std::memset(base + rec, 0, kRec);                 // sin grupos de objetos (como R&R BASE)
  std::strncpy(reinterpret_cast<char*>(base + rec), a.nombre.c_str(), 15);
  REX_STORE_U32(rec + 0x10, a.entrada);
  REX_STORE_U32(rec + 0x14, a.id_se);
  REX_STORE_U32(kTemas + 4 * kAnfitrion, tema);
  if (a.niebla && ni < 64) {   // R8: la del alta en la entrada del anfitrion (se devuelve en Restaurar)
    g_niebla = kNieblas + ni * 20;
    std::memcpy(g_orig_niebla, base + g_niebla, sizeof(g_orig_niebla));
    uint32_t ini, fin;
    std::memcpy(&ini, &a.niebla_ini, 4);
    std::memcpy(&fin, &a.niebla_fin, 4);
    REX_STORE_U32(g_niebla, a.niebla_color);
    REX_STORE_U32(g_niebla + 12, ini);
    REX_STORE_U32(g_niebla + 16, fin);
    REX_STORE_U32(rec + 0x1C + 4, ni);   // variantes 0 y 1
    REX_STORE_U32(rec + 0x3C + 4, ni);
  }
  g_activa = v;
  REXLOG_INFO("dbz3 escenarios: alta {} '{}' (mod {}) en el registro {}: entrada {}, id_se {}, tema {}", v,
              a.nombre, a.mod, kAnfitrion, a.entrada, a.id_se, a.tema);
}

// Indice que debe ver el juego: el anfitrion para un alta; de serie tal cual (y el anfitrion vuelve a
// ser el original si estaba prestado).
int Traducir(uint8_t* base, int i) {
  if (i >= kSerie && i < Total()) {
    Activar(base, i);
    return kAnfitrion;
  }
  if (i >= 0 && i < kSerie) Restaurar(base);
  return i;
}

// Frente 07: obj del escenario del combate (lo da sub_820C26E0), para poder recargar el escenario al
// pasar de zona (como hace el juego original de B2 al caer por el borde: recarga el escenario sin tocar
// el combate, FUN_001b95b0 en b2.c).
void RecordarObj(uint32_t obj) { g_obj_combate = obj; }

// Registro del alta que es la zona B del alta v (su stage.toml lleva `zona_b = entrada de v`), o -1.
int ZonaBDe(int v) {
  if (v < kSerie || v >= Total()) return -1;
  const uint32_t entrada = g_altas[size_t(v - kSerie)].entrada;
  for (size_t k = 0; k < g_altas.size(); ++k)
    if (g_altas[k].zona_b == entrada) return kSerie + int(k);
  return -1;
}

// Ring out con zona B (dbz3_escenarios_zonas_b): el alta cargada tiene zona B -> se recarga el escenario
// del combate con el bin de la zona B (su objeto 0 es la otra zona; las camaras de su objeto 0 son las de
// la transicion de B2). Devuelve true si ha hecho el cambio (el llamador consume el aviso de ring out).
bool PasarZonaB(PPCContext& ctx, uint8_t* base) {
  const int zb = ZonaBDe(g_activa);
  if (zb < 0 || !g_obj_combate) return false;
  const int j = Traducir(base, zb);   // pone el alta de la zona B en el anfitrion
  REX_STORE_U16(g_obj_combate + 340, uint16_t(j));
  PPCContext c = ctx;
  c.r3.u64 = g_obj_combate;
  DBZ3_CALL_EU(sub_820C26E0)(c, base);   // carga del escenario (la regenerada: sin re-entrar en el hook)
  REX_STORE_U16(g_obj_combate + 340, uint16_t(zb));   // como el hook: el combate sigue pidiendo el alta
  REXLOG_INFO("dbz3 escenarios: ring out (aviso {}): zona B '{}' (registro {}, entrada {}) cargada en el combate",
              REX_LOAD_U8(REX_LOAD_U32(kMgrPtr) + kAvisoRing), g_altas[size_t(zb - kSerie)].nombre, zb,
              g_altas[size_t(zb - kSerie)].entrada);
  return true;
}

}  // namespace

int Total() { return kSerie + int(g_altas.size()); }
int Visibles() { return int(g_visibles); }

int PorMod(const std::string& mod) {
  for (size_t i = 0; i < g_altas.size(); ++i)
    if (g_altas[i].mod == mod) return kSerie + int(i);
  return -1;
}

int CampoIW(int campo) { return PorMod("Campo IW " + std::to_string(campo)); }   // altas de mod center hd/campos_iw.py

// Nombres oficiales: los rotulos del selector (data_<idioma>.afs 2043, textura 0, una fila por casilla),
// copiados a texto. Fila de cada registro: las variantes (ISLAND B, TOWN B, NAMEC B, KAIOU B y EARTH, que
// usa el mapa de ROCK) llevan el nombre de su mapa.
std::string Nombre(int registro, int idioma) {
  static const char* const kFilas[12][5] = {   // ingles, aleman, frances, espanol, italiano
      {"Red Ribbon Base", "Red Ribbon Basis", "Base Ruban rouge", "Base de la Red Ribbon", "Base Red Ribbon"},
      {"Grandpa Gohan's House", "Großvaters Haus", "Chez Grand-père", "Casa del abuelo", "Casa del nonno"},
      {"Inside Buu", "In Buu", "Entrailles de Buu", "Dentro de Bu", "Dentro Bu"},
      {"Archipelago", "Archipel", "Archipel", "Archipiélago", "Arcipelago"},
      {"Supreme Kai's World", "Kaioshins Welt", "Monde de Kaïo Shin", "Mundo de Kaiyoshin", "Mondo di Kaiohshin"},
      {"Hyperbolic Time Chamber", "Raum v. Geist/Zeit", "Salle Esprit / Temps", "Cámara de Tiempo Hiperbólico",
       "Stanza Tempo e Spirito"},
      {"Planet Namek", "Planet Namek", "Planète Namek", "Planeta Namek", "Pianeta Namek"},
      {"Mountains", "Felsenberg", "Montagnes rocheuses", "Montañas rocosas", "Montagna rocciosa"},
      {"World Tournament Stage", "Stufe Welt-Kampf", "Niveau Champ. du Monde", "Escenario de Torneo Mundial",
       "Livello Torneo mondiale"},
      {"Urban Area", "Stadt", "Zone urbaine", "Zona urbana", "Città"},
      {"Plains", "Ebenen", "Plaines", "Llanuras", "Pianure"},
      {"Cell Ring", "Cell-Ring", "Anneau de Cell", "Ring de Cell", "Ring di Cell"},
  };
  // BUDO, MIND&TIME, ISLAND, TOWN, ROCK, PLANE, GOKU, NAMEC, CELL, KAIOU, BUU BODY, ISLAND B, TOWN B, EARTH,
  // NAMEC B, KAIOU B, R&R BASE
  static const int kFila[kSerie] = {8, 5, 3, 9, 7, 10, 1, 6, 11, 4, 2, 3, 9, 7, 6, 4, 0};
  if (registro >= kSerie && registro < Total()) {
    const Alta& a = g_altas[size_t(registro - kSerie)];
    return a.titulo.empty() ? a.nombre : a.titulo;
  }
  if (registro < 0 || registro >= kSerie) return {};
  const int col = idioma == 3 ? 1 : idioma == 4 ? 2 : idioma == 5 ? 3 : idioma == 6 ? 4 : 0;   // 1, 2: ingles
  return kFilas[kFila[registro]][col];
}

std::string Nombre(int registro) { return Nombre(registro, dbz3::settings::Language()); }

void CargarDeMods(rex::memory::Memory* memory) {
  g_altas.clear();
  g_mem = memory;
  if (!memory) return;
  std::error_code ec;
  for (const auto& info : ListMods(false)) {
    if (!info.enabled) continue;
    const auto file = ModsRoot() / info.name / "stage.toml";
    if (!std::filesystem::is_regular_file(file, ec)) continue;
    std::ifstream in(file);
    std::string line;
    Alta* a = nullptr;
    while (std::getline(in, line)) {
      line = Limpia(line);
      if (line.empty() || line.front() == '#') continue;
      if (line.front() == '[') {
        a = nullptr;
        if (line == "[[escenario]]" || line == "[escenario]") a = &g_altas.emplace_back(Alta{info.name});
        continue;
      }
      const auto eq = line.find('=');
      if (!a || eq == std::string::npos) continue;
      const std::string k = Limpia(line.substr(0, eq)), v = Valor(line.substr(eq + 1));
      if (k == "nombre") a->nombre = v.substr(0, 15);
      else if (k == "titulo") a->titulo = v;
      else if (k == "entrada") a->entrada = uint32_t(std::strtoul(v.c_str(), nullptr, 10));
      else if (k == "id_se") a->id_se = uint32_t(std::strtoul(v.c_str(), nullptr, 10));
      else if (k == "tema") a->tema = std::atoi(v.c_str());
      else if (k == "menu") a->menu = uint32_t(std::strtoul(v.c_str(), nullptr, 10));
      else if (k == "zona_b") a->zona_b = uint32_t(std::strtoul(v.c_str(), nullptr, 10));
      else if (k == "selector") a->oculta = v == "false" || v == "no" || v == "0";
      else if (k == "niebla")
        a->niebla = std::sscanf(v.c_str(), "%x,%f,%f", &a->niebla_color, &a->niebla_ini, &a->niebla_fin) == 3;
    }
  }
  std::erase_if(g_altas, [](const Alta& a) {
    const bool mal = a.entrada < 5000 || a.tema < 0 || a.tema >= kSerie;
    if (mal) REXLOG_WARN("dbz3 escenarios [{}]: alta ignorada (entrada {} < 5000 o tema {} fuera de 0..16)", a.mod,
                         a.entrada, a.tema);
    return mal;
  });
  if (g_altas.size() > 23) g_altas.resize(23);
  // Campos de IW (campos_iw.py): llanuras para los minijuegos, no escenarios de Duelo. Las ocultas van al final
  // y las visibles caben en el carrusel de 24 (escenarios_sel_gen.inc kSelMax) = 12 del juego + 11 + el "?";
  // antes los 8 campos llenaban el carrusel y dejaban fuera las altas siguientes (Montanas nevadas de B2).
  for (auto& a : g_altas)
    if (a.mod.rfind("Campo IW", 0) == 0) a.oculta = true;
  std::stable_partition(g_altas.begin(), g_altas.end(), [](const Alta& a) { return !a.oculta; });
  g_visibles = uint32_t(std::count_if(g_altas.begin(), g_altas.end(), [](const Alta& a) { return !a.oculta; }));
  if (g_visibles > 11) {
    REXLOG_WARN("dbz3 escenarios: {} escenarios nuevos y el selector solo tiene sitio para 11", g_visibles);
    g_visibles = 11;
  }
  for (auto& a : g_altas)
    if (!a.menu) a.menu = a.entrada + 100;
  // las tablas viven en .data: escribibles para prestar el registro anfitrion
  if (auto* heap = memory->LookupHeap(kRecs))
    heap->Protect(kRecs & ~0xFFFu, 0x2000, rex::memory::kMemoryProtectRead | rex::memory::kMemoryProtectWrite);
  for (size_t k = 0; k < g_altas.size(); ++k)
    REXLOG_INFO("dbz3 escenarios: alta {} = '{}' / '{}' (mod {}, entrada {})", kSerie + int(k), g_altas[k].nombre,
                Nombre(kSerie + int(k), 5), g_altas[k].mod, g_altas[k].entrada);
  std::string lista;   // para pantallas propias y pruebas: los nombres en el idioma del jugador
  for (int r = 0; r < Total(); ++r) lista += (r ? ", " : "") + std::to_string(r) + " " + Nombre(r);
  REXLOG_INFO("dbz3 escenarios: nombres (idioma {}): {}", dbz3::settings::Language(), lista);
}

}  // namespace dbz3::escenarios

// Carga del escenario del combate (obj+340 = indice de registro).
DBZ3_HOOK_EU(sub_820C26E0) {
  const uint32_t obj = ctx.r3.u32;
  const int i = int16_t(REX_LOAD_U16(obj + 340));
  dbz3::escenarios::RecordarObj(obj);   // frente 07: para recargar el escenario al pasar de zona
  const int j = dbz3::escenarios::Traducir(base, i);
  dbz3::clima::EscenarioPedido();
  if (j != i) REX_STORE_U16(obj + 340, uint16_t(j));
  orig(ctx, base);
  if (j != i) REX_STORE_U16(obj + 340, uint16_t(i));   // "otra vez" vuelve a pedir el alta
}

// id_SE del registro r3 (sub_821221F8, con el mismo indice del combate).
DBZ3_HOOK_EU(sub_820C19B0) {
  ctx.r3.u64 = uint32_t(dbz3::escenarios::Traducir(base, int(ctx.r3.s32)));
  orig(ctx, base);
}

// ---------------------------------------------------------------------------------------------------
// SONDA (DBZ3_SONDA_ESCENARIO=1): tras reubicar el #ZDD (sub_820CBC90) comprueba que los registros de
// colision rec20/rec30 de cada objeto (obj+0xAC/+0xB0/+0xB4/+0xB8) caen dentro de su modelo #AWO:
// campo 7 = el modelo, 4 = cabecera del bloque de colision [FFFFFFFF ...], 3 = [4, nodos, ...].
// Es la medida que separo el arreglo del cuelgue de B2 (punteros PS2 sin traducir, sub_820CB0A0).
// ---------------------------------------------------------------------------------------------------
DBZ3_HOOK_EU(sub_820CBC90) {
  const uint32_t zdd = ctx.r3.u32;
  orig(ctx, base);
  static const bool sonda = std::getenv("DBZ3_SONDA_ESCENARIO") != nullptr;
  if (!sonda) return;
  auto guest = [](uint32_t p) { return p >= 0x40000000u && p < 0x50000000u; };
  const uint32_t n = REX_LOAD_U32(zdd + 4), lista = REX_LOAD_U32(zdd + 32);
  int ok = 0, mal = 0;
  for (uint32_t k = 0; k < n && guest(lista); ++k) {
    const uint32_t ob = REX_LOAD_U32(lista + 4 * k);
    if (!guest(ob)) continue;
    const uint32_t arr[4][3] = {{0xAC, 0x94, 5}, {0xB0, 0x98, 8}, {0xB4, 0x9C, 5}, {0xB8, 0xA0, 8}};
    for (const auto& a : arr) {
      const uint32_t p = REX_LOAD_U32(ob + a[0]), c = REX_LOAD_U32(ob + a[1]);
      const uint32_t tam = a[2] == 5 ? 0x20 : 0x30, campos = a[2];
      for (uint32_t r = 0; r < c && guest(p); ++r) {
        const uint32_t e = p + r * tam;
        uint32_t f[8];
        for (uint32_t j = 0; j < campos; ++j) f[j] = REX_LOAD_U32(e + 4 * j);
        const uint32_t mdl = f[campos - 1], cab = f[campos - 4], f3 = f[campos - 5];
        bool bien = guest(mdl) && REX_LOAD_U8(mdl + 1) == 'A' && REX_LOAD_U8(mdl + 2) == 'W' &&
                    REX_LOAD_U8(mdl + 3) == 'O' && guest(cab) && guest(f3);
        for (uint32_t j = 0; bien && j < campos; ++j) bien = f[j] >= mdl && f[j] - mdl < 0x400000u;
        bien = bien && REX_LOAD_U32(cab) == 0xFFFFFFFFu && REX_LOAD_U32(f3) == 4;
        if (bien) {
          ++ok;
        } else if (mal++ < 4) {
          REXLOG_WARN("dbz3 sonda escenario: obj {} +0x{:X}[{}] MAL: {:08X} {:08X} {:08X} {:08X} {:08X} {:08X} {:08X} {:08X}",
                      k, a[0], r, f[0], f[1], f[2], f[3], f[4], campos > 5 ? f[5] : 0, campos > 6 ? f[6] : 0,
                      campos > 7 ? f[7] : 0);
        }
      }
    }
  }
  REXLOG_INFO("dbz3 sonda escenario: #ZDD 0x{:08X}: {} registros de colision dentro de su modelo, {} fuera", zdd, ok,
              mal);
}

// ---------------------------------------------------------------------------------------------------
// SELECTOR DE ESCENARIOS DEL DUELO (mn_StageSel): una casilla nueva por alta (13 + k -> registro 17 + k).
// Datos ampliados y tablas por casilla propias: escenarios_sel_gen.inc (tools/gen_escenarios_sel.py).
// Imagenes de la casilla nueva: el #AZT `menu` del mod (0 circulo, 1 vista, 2 rotulo, 3 bola) se carga con
// los del selector y el juego lo registra al crearle las texturas. Lo de la casilla nueva se dibuja con ese
// #AZT puesto como banco de texturas (el del selector, D+4, queda igual): circulo y bola (sub_8219D520 /
// sub_8219D6E8), y la vista y el rotulo (sprites copiados de los nativos, tabla #ACA ampliada en D+0 +28;
// se crean en sub_8219CC48 y se dibujan en sub_8219E828). Sin altas todo es el codigo original.
// ---------------------------------------------------------------------------------------------------
// valores de partida (US); Tablas() los cambia por las tablas propias antes de que corra ninguna copia Sel_/SelEu_
uint32_t dbz3_esc_f94 = 0x82020F94, dbz3_esc_fb0 = 0x82020FB0, dbz3_esc_trans = 0x82373384, dbz3_esc_limite = 11;
void dbz3_esc_instalar(PPCContext& ctx, uint8_t* base, uint32_t d);
void dbz3_esc_banco(PPCContext& ctx, uint8_t* base, uint32_t d, bool nuevo);

namespace {
#include "escenarios_sel_gen.inc"
#if defined(DBZ3_DUAL_REGION)
#include "escenarios_sel_gen_eu.inc"   // las mismas copias del codigo EU (tools/gen_escenarios_sel.py)
#define SEL(fn) DBZ3_CALL2(Sel_##fn, SelEu_##fn)
#else
#define SEL(fn) Sel_##fn
#endif

using dbz3::escenarios::g_altas;
using dbz3::escenarios::g_visibles;
constexpr uint32_t kCasillasSerie = 13;   // 0..11 escenarios, 12 = "?"
constexpr dbz3::GA kGlobalSel = 0x8245F25C;   // estructura del selector: +20 tarea (+48 = datos)

bool Ext() { return !g_altas.empty() && dbz3::escenarios::g_mem; }

uint32_t Alloc(uint32_t n) {
  const uint32_t a = dbz3::escenarios::g_mem->SystemHeapAlloc(n, 0x20);
  if (a) std::memset(dbz3::escenarios::g_mem->TranslateVirtual<uint8_t*>(a), 0, n);
  return a;
}

uint32_t Llamar(PPCContext& ctx, uint8_t* base, void (*fn)(PPCContext&, uint8_t*), uint32_t r3, uint32_t r4 = 0,
                uint32_t r5 = 0) {
  PPCContext c = ctx;
  c.r3.u64 = r3;
  c.r4.u64 = r4;
  c.r5.u64 = r5;
  fn(c, base);
  return c.r3.u32;
}

std::vector<uint32_t> g_cargas;       // nodo de lectura AFS del #AZT de cada alta (sub_82083968)
std::vector<uint32_t> g_azt;          // #AZT de cada alta ya en memoria y registrado (0 = no esta)
uint32_t g_datos = 0, g_tabla_nat = 0;

// #AZT de la casilla c (0 si es del juego o su alta no tiene imagenes).
uint32_t AztDe(uint32_t c) {
  return c >= kCasillasSerie && c - kCasillasSerie < g_azt.size() ? g_azt[c - kCasillasSerie] : 0;
}

void Tablas(uint8_t* base) {
  static bool hechas = false;
  if (hechas) return;
  hechas = true;
  const uint32_t f94 = Alloc(2 * kSelMax), fb0 = Alloc(2 * kSelMax), tr = Alloc(4 * kSelMax);
  for (uint32_t c = 0; c < kSelMax; ++c) {
    const uint32_t src = c < kCasillasSerie ? c : 12;   // de relleno: las del "?"
    REX_STORE_U8(f94 + 2 * c, REX_LOAD_U8(dbz3::GA(0x82020F94) + 2 * src));
    REX_STORE_U8(f94 + 2 * c + 1, REX_LOAD_U8(dbz3::GA(0x82020F94) + 2 * src + 1));
    REX_STORE_U8(fb0 + 2 * c, REX_LOAD_U8(dbz3::GA(0x82020FB0) + 2 * src));
    REX_STORE_U8(fb0 + 2 * c + 1, REX_LOAD_U8(dbz3::GA(0x82020FB0) + 2 * src + 1));
  }
  for (uint32_t c = 0; c < 12; ++c) REX_STORE_U32(tr + 4 * c, REX_LOAD_U32(dbz3::GA(0x82373384) + 4 * c));
  for (uint32_t k = 0; k < g_visibles; ++k) {
    const uint16_t r = uint16_t(dbz3::escenarios::kSerie + k);
    REX_STORE_U16(tr + 4 * (kCasillasSerie + k), r);
    REX_STORE_U16(tr + 4 * (kCasillasSerie + k) + 2, r);
  }
  dbz3_esc_f94 = f94;
  dbz3_esc_fb0 = fb0;
  dbz3_esc_trans = tr;
  dbz3_esc_limite = 12 + g_visibles;
}

// Copia de un sprite del #ACA (hasta el siguiente de la tabla) con sus punteros internos rebasados y su
// bloque de textura [tex, 3, 1.0, 0, x0, y0, x1, y1] apuntando a `tex` con el rectangulo logico dado.
uint32_t CopiaSprite(uint8_t* base, uint32_t tabla, uint32_t n, uint32_t i, uint32_t tex, float w, float h,
                     uint32_t indice) {
  const uint32_t src = REX_LOAD_U32(tabla + 4 * i);
  uint32_t tam = 0x200;
  for (uint32_t j = 0; j < n; ++j) {
    const uint32_t q = REX_LOAD_U32(tabla + 4 * j);
    if (q > src && q - src < tam) tam = q - src;
  }
  const uint32_t dst = Alloc(tam);
  if (!dst) return 0;
  uint32_t bloque = 0;
  for (uint32_t o = 0; o + 4 <= tam; o += 4) {
    uint32_t v = REX_LOAD_U32(src + o);
    if (v >= src && v < src + tam) v = v - src + dst;
    REX_STORE_U32(dst + o, v);
    if (!bloque && o >= 16 && o + 32 <= tam && REX_LOAD_U32(src + o + 4) == 3 &&
        REX_LOAD_U32(src + o + 8) == 0x3F800000u && REX_LOAD_U32(src + o) < 0x100)
      bloque = o;
  }
  if (!bloque) return 0;
  auto f = [](float x) {
    uint32_t u;
    std::memcpy(&u, &x, 4);
    return u;
  };
  REX_STORE_U32(dst + bloque, tex);
  REX_STORE_U32(dst + bloque + 16, 0);
  REX_STORE_U32(dst + bloque + 20, 0);
  REX_STORE_U32(dst + bloque + 24, f(w));
  REX_STORE_U32(dst + bloque + 28, f(h));
  REX_STORE_U32(dst + 0x14, indice);
  return dst;
}

// Pone el banco de texturas `banco` como lo hace el juego (sub_8219CBB8: globales del menu + sub_82092C58).
void PonerBanco(PPCContext& ctx, uint8_t* base, uint32_t banco) {
  REX_STORE_U32(dbz3::GA(0x8239592C), 0);
  REX_STORE_U32(dbz3::GA(0x82395928), 28672);
  REX_STORE_U32(dbz3::GA(0x82395930), banco);
  Llamar(ctx, base, DBZ3_CALL_EU(sub_82092C58), banco, 28672);
  REX_STORE_U32(dbz3::GA(0x82389430), 0);
}

}  // namespace

// sub_8219F350 (regenerado), con los recursos ya en D: texturas de las altas y sprites nuevos.
void dbz3_esc_instalar(PPCContext& ctx, uint8_t* base, uint32_t d) {
  if (!Ext() || !d) return;
  Tablas(base);
  const size_t na = g_visibles;
  g_azt.assign(na, 0);
  for (size_t k = 0; k < na && k < g_cargas.size(); ++k) {
    const uint32_t h = g_cargas[k];
    if (!h || REX_LOAD_U16(h + 4) != 3) continue;
    const uint32_t datos = REX_LOAD_U32(h + 212);
    REX_STORE_U32(h + 212, 0);
    Llamar(ctx, base, DBZ3_CALL_EU(sub_82083D08), h, 0);
    g_cargas[k] = 0;
    if (!datos || REX_LOAD_U8(datos + 1) != 'A' || REX_LOAD_U8(datos + 2) != 'Z' || REX_LOAD_U32(datos + 16) < 4) {
      REXLOG_WARN("dbz3 escenarios: el #AZT del selector de '{}' (entrada {}) no vale", g_altas[k].nombre,
                  g_altas[k].menu);
      continue;
    }
    Llamar(ctx, base, DBZ3_CALL_EU(sub_820800A8), datos);   // crea y registra las texturas
    g_azt[k] = datos;
  }
  const uint32_t aca = REX_LOAD_U32(d + 0);
  if (!aca) return;
  // sprites: tabla nativa + 2 por alta (vista y rotulo, copias de los de la casilla 0, texturas 1 y 2)
  const uint32_t ns = REX_LOAD_U32(aca + 24), tn = REX_LOAD_U32(aca + 28);
  const uint32_t t = Alloc(4 * (ns + 2 * uint32_t(na)));
  if (!t || ns + 2 * na > 255) return;
  for (uint32_t i = 0; i < ns; ++i) REX_STORE_U32(t + 4 * i, REX_LOAD_U32(tn + 4 * i));
  const uint32_t vista0 = REX_LOAD_U8(dbz3::GA(0x82020FB0)), rotulo0 = REX_LOAD_U8(dbz3::GA(0x82020FB1));
  for (size_t k = 0; k < na; ++k) {
    const uint32_t c = kCasillasSerie + uint32_t(k), sv_i = ns + 2 * uint32_t(k);
    const uint32_t sv = g_azt[k] ? CopiaSprite(base, tn, ns, vista0, 1, 440.0f, 320.0f, sv_i) : 0;
    const uint32_t sr = g_azt[k] ? CopiaSprite(base, tn, ns, rotulo0, 2, 192.0f, 32.0f, sv_i + 1) : 0;   // fila 0 del atlas
    REX_STORE_U32(t + 4 * sv_i, sv ? sv : REX_LOAD_U32(tn + 4 * vista0));
    REX_STORE_U32(t + 4 * (sv_i + 1), sr ? sr : REX_LOAD_U32(tn + 4 * rotulo0));
    if (g_azt[k]) {   // circulo y bola: texturas 0 y 3 de su #AZT; sin el, las del "?" (no se cuelga)
      REX_STORE_U8(dbz3_esc_f94 + 2 * c, 0);
      REX_STORE_U8(dbz3_esc_f94 + 2 * c + 1, 3);
    }
    REX_STORE_U8(dbz3_esc_fb0 + 2 * c, uint8_t(sv_i));
    REX_STORE_U8(dbz3_esc_fb0 + 2 * c + 1, uint8_t(sv_i + 1));
  }
  g_datos = d;
  g_tabla_nat = tn;
  REX_STORE_U32(aca + 28, t);
  REXLOG_INFO("dbz3 escenarios: selector con {} casillas nuevas (sprites {}+)", na, ns);
}

// sub_8219E828 (regenerado): banco de la casilla nueva alrededor de la vista grande y el rotulo.
void dbz3_esc_banco(PPCContext& ctx, uint8_t* base, uint32_t d, bool nuevo) {
  if (!Ext() || !d) return;
  const uint32_t z = AztDe(REX_LOAD_U16(d + 604));
  if (z) PonerBanco(ctx, base, nuevo ? z : REX_LOAD_U32(d + 4));
}

// Cargador del selector: los #AZT de las altas se piden ANTES que los del juego (la cola de lectura va en
// orden, asi estan cuando sub_8219F350 ve el ultimo del juego).
DBZ3_HOOK_EU(sub_8219C7B0) {
  if (Ext()) {
    g_cargas.clear();
    for (uint32_t k = 0; k < g_visibles; ++k)
      g_cargas.push_back(Llamar(ctx, base, DBZ3_CALL_EU(sub_82083968), g_altas[k].menu, 0, 0));
  }
  orig(ctx, base);
}

// Fin del selector: el juego libera lo suyo (con su tabla de sprites); luego los #AZT de las altas.
DBZ3_HOOK_EU(sub_8219E108) {
  if (Ext() && g_datos) {
    const uint32_t g = REX_LOAD_U32(kGlobalSel), tarea = g ? REX_LOAD_U32(g + 20) : 0;
    const uint32_t d = tarea ? REX_LOAD_U32(tarea + 48) : 0;
    if (d == g_datos)
      if (const uint32_t aca = REX_LOAD_U32(d + 0)) REX_STORE_U32(aca + 28, g_tabla_nat);
    g_datos = 0;
  }
  orig(ctx, base);
  if (Ext()) {
    for (uint32_t z : g_azt)
      if (z) Llamar(ctx, base, DBZ3_CALL_EU(sub_82089C10), z);
    g_azt.clear();
    for (uint32_t h : g_cargas)   // pedidos que no llegaron a usarse
      if (h) Llamar(ctx, base, DBZ3_CALL_EU(sub_82083D08), h, 0);
    g_cargas.clear();
  }
}

// Lista de casillas: las del juego (bits 0..11 de la mascara), las altas y el "?" (bit 12) al final, que
// es lo que espera la eleccion al azar (toma una de las n-1 primeras).
DBZ3_HOOK_EU(sub_8219E268) {
  if (!Ext()) {
    orig(ctx, base);
    return;
  }
  Tablas(base);
  const uint32_t d = ctx.r3.u32, mascara = ctx.r4.u32 & 0xFFFF;
  REX_STORE_U16(d + 576, uint16_t(mascara));
  REX_STORE_U16(d + 574, uint16_t(mascara));
  for (uint32_t i = 0; i < kSelMax; ++i) REX_STORE_U16(d + kSelLista + 2 * i, 0xFFFF);
  uint32_t n = 0;
  auto poner = [&](uint32_t c) {   // (REX_STORE_U16 es una macro: nada de n++ dentro)
    if (n >= kSelMax) return;
    REX_STORE_U16(d + kSelLista + 2 * n, uint16_t(c));
    ++n;
  };
  for (uint32_t b = 0; b < 12; ++b)
    if (mascara >> b & 1) poner(b);
  for (uint32_t k = 0; k < g_visibles; ++k) poner(kCasillasSerie + k);
  if (mascara >> 12 & 1) poner(12);
  REX_STORE_U8(d + 609, uint8_t(n));
  REX_STORE_U32(d + 632, n ? 65536u / (n * 12) : 0);
  if (REX_LOAD_U8(d + 610) == 2) Llamar(ctx, base, DBZ3_CALL_EU(sub_8219CAE8), d);
}

// Circulo (sub_8219D6E8) y bola (sub_8219D520) de la casilla r4 y sprites de la casilla actual
// (sub_8219CC48): con el #AZT de la casilla nueva en D+4 durante la llamada.
void ConBanco(Dbz3GuestFn F, PPCContext& ctx, uint8_t* base, uint32_t c) {
  const uint32_t d = ctx.r3.u32, z = AztDe(c), nat = REX_LOAD_U32(d + 4);
  if (z) REX_STORE_U32(d + 4, z);
  F(ctx, base);
  if (z) {
    REX_STORE_U32(d + 4, nat);
    // F deja de banco global (0x8238CE10) el ultimo que puso, aqui el del alta: el del juego, como lo dejaria
    // el juego. Los sprites de otras tareas piden sus texturas al banco global (sub_82092C80 -> sub_82096FF0:
    // la 37 al pasar a la casilla, medido 2026-10-15); en el #AZT del alta solo hay 4 (y su indice va relleno
    // hasta 64 por si acaso: tools/escenarios_convertir.py azt_menu)
    if (REX_LOAD_U32(dbz3::GA(0x8238CE10)) == z) PonerBanco(ctx, base, nat);
  }
}

DBZ3_HOOK_EU(sub_8219D520) {
  if (Ext())
    ConBanco(SEL(8219D520), ctx, base, ctx.r4.u32 & 0xFFFF);
  else
    orig(ctx, base);
}
DBZ3_HOOK_EU(sub_8219D6E8) {
  if (Ext())
    ConBanco(SEL(8219D6E8), ctx, base, ctx.r4.u32 & 0xFFFF);
  else
    orig(ctx, base);
}
DBZ3_HOOK_EU(sub_8219E540) {   // entrada r4 del carrusel
  if (Ext())
    ConBanco(SEL(8219E540), ctx, base, REX_LOAD_U16(ctx.r3.u32 + kSelCarrusel + 8 + 12 * (ctx.r4.u32 & 0xFFFF)));
  else
    orig(ctx, base);
}
DBZ3_HOOK_EU(sub_8219CC48) {
  if (Ext())
    ConBanco(SEL(8219CC48), ctx, base, REX_LOAD_U16(ctx.r3.u32 + 604));
  else
    orig(ctx, base);
}

#define DBZ3_SEL(fn)              \
  DBZ3_HOOK_EU(sub_##fn) {        \
    if (Ext())                    \
      SEL(fn)(ctx, base);         \
    else                          \
      orig(ctx, base);            \
  }
DBZ3_SEL(8219F4F8)
DBZ3_SEL(8219C870)
DBZ3_SEL(8219CAE8)
DBZ3_SEL(8219ECC8)
DBZ3_SEL(8219E6C0)
DBZ3_SEL(8219F2A8)
DBZ3_SEL(8219F350)
DBZ3_SEL(8219E828)
#undef DBZ3_SEL

// ---------------------------------------------------------------------------------------------------
// RING OUT CON ZONA B (frente 07, mod opcional dbz3_escenarios_zonas_b). En el original de B2, al caer un
// luchador por el borde el guion del escenario llama a FUN_001b95b0 (opcode 0x80 'call_ext', indice 0x40):
// recarga el escenario con la otra variante (la zona B) sin tocar el combate. En B3 ese aviso lo pone el
// juego en mgr+35569 (hueco del ganador + 1) y lo consumen las funciones de fin del combate (motivo 4).
// El gancho va en sub_82114910: comprueba mgr+35569 y los estados de combate (3/4/5/8) al entrar; con la
// cvar activada y una zona B en el alta cargada, el aviso se consume (el combate sigue) y se recarga el
// escenario del combate con el bin de la zona B (cuyo objeto 0 es la otra zona y cuyas camaras del objeto
// 0 son las de la transicion de B2). Con la cvar en false (por defecto) el ring out acaba el combate como
// siempre. [H] que sub_82114910 corra en el fotograma del ring out: lee el aviso y los estados de pelea
// (generated/dbz3_recomp.42.cpp:8277-8318) y la llama el flujo del combate (sub_8210E700).
// ---------------------------------------------------------------------------------------------------
// El gancho de sub_82114910 es uno solo (iw_minijuegos.cpp, tope de distancia de B3): lo llama al entrar.
namespace dbz3::escenarios {
void ComprobarZonaB(PPCContext& ctx, uint8_t* base);
}
void dbz3::escenarios::ComprobarZonaB(PPCContext& ctx, uint8_t* base) {
  if (REXCVAR_GET(dbz3_escenarios_zonas_b)) {
    const uint32_t mgr = REX_LOAD_U32(dbz3::escenarios::kMgrPtr);
    if (mgr && REX_LOAD_U32(mgr + dbz3::escenarios::kEstadoCombate) == 4 &&
        REX_LOAD_U8(mgr + dbz3::escenarios::kAvisoRing) && dbz3::escenarios::PasarZonaB(ctx, base))
      REX_STORE_U8(mgr + dbz3::escenarios::kAvisoRing, 0);   // el aviso se consume: el combate sigue
  }
}
