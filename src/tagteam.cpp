// dbz3 - Tag Team: equipos de 2-3 personajes del juego, tal cual (sin personajes nuevos), con
// relevo como en Tenkaichi 3: el luchador que sale se descarga y entra el otro personaje entero
// (su modelo, sus golpes y tecnicas, su camara, sus transformaciones).
//
// Los miembros los elige cada lado en la rueda del juego (modo TAG TEAM, tagteam_panel.cpp;
// tagteam_slots.cpp guarda la lista y hace que el lado empiece con su primer miembro).
//   - SELECT = relevo al siguiente miembro vivo; SELECT + L = al otro (equipos de 3).
//   - vida por miembro: la del que sale se guarda y la del que entra se pone (X+6080).
//   - KO con compaÃ±eros vivos: el golpe deja al luchador "casi muerto" (X+8203, el clamp de
//     sub_8211A5A8 en vez del KO) y entra el siguiente. El ultimo cae de verdad.
// Cambio de personaje con las rutinas del juego (estudios 11-13 de docs/10_tag_team):
//   1. sub_821092C8 libera camara, idioma y los modelos/animaciones de las 8 formas;
//   2. ID nuevo (X+7420, X+7416 = su registro de 372 B; la seleccion X+7412 con su ID, forma 0
//      y sus capsulas por defecto) y sub_82109148 pide su camara, idioma, animaciones y modelos
//      (cargas asincronas);
//   3. Tick espera a que esten (camara +192 y modelo de la forma 0 con +36 == 0) y hace lo que
//      el juego al empezar: sub_8210FDE0 (golpes y camara: X+7896..), sub_82109638 (reinicio
//      del luchador) y la forma 0 (sub_8210E690 + sub_82115270). La vida se restaura despues.
#include <rex/hook.h>
#include <rex/logging.h>

#include <algorithm>
#include <cstdlib>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <vector>

#include "generated/dbz3_init.h"
#include "guest_region.h"
#include "roster_ext.h"
#include "tagteam.h"
#include "tagteam_round.h"
#include "form_selection_state.h"
#include "ingame/tagteam_panel.h"
namespace dbz3::campaigns { int PlayerStartForm(); int RivalSuit(); }
#include "net/netplay.h"

DBZ3_EXTERN_EU(dbz3eu_sub_820FB6D0);
DBZ3_EXTERN_EU(dbz3eu_sub_82115370);
DBZ3_EXTERN_EU(dbz3eu_sub_8210E790);
DBZ3_EXTERN_EU(dbz3eu_sub_821093C8);
DBZ3_EXTERN_EU(dbz3eu_sub_82100E60);
DBZ3_EXTERN_EU(dbz3eu_sub_82100EE8);
DBZ3_EXTERN_EU(dbz3eu_sub_82109248);
DBZ3_EXTERN_EU(dbz3eu_sub_8210FEE0);
DBZ3_EXTERN_EU(dbz3eu_sub_82109738);
DBZ3_EXTERN_EU(dbz3eu_sub_82113918);
DBZ3_EXTERN_EU(dbz3eu_sub_82113970);
DBZ3_EXTERN_EU(dbz3eu_sub_82112360);
DBZ3_EXTERN_EU(dbz3eu_sub_82115430);

namespace dbz3::tag {
// TAG_F3_RECIPE_V1
bool ReadMemberCapsules(uint8_t*, int, int, std::array<uint16_t, 8>&);
namespace {

constexpr uint32_t kHp = 6080, kHpMax = 6084, kId = 7420, kRec372 = 7416, kSel = 7412;
constexpr uint32_t kNearDeath = 8203, kBody = 1616, kForm = 2980, kNextForm = 2984, kModel0 = 7608, kAnm0 = 7672;
constexpr uint32_t kCamRoot = 7896, kCam = 7916, kLangFlag = 7920, kLang = 7924;
constexpr uint32_t kLoad = 7;            // hueco de forma donde se carga el que entra
// hueco donde se carga la forma con la que salio el que entra (vuelve transformado, como en Tenkaichi 3)
constexpr uint32_t kSpare = 6;
constexpr uint32_t kChar96 = 0x8234ABB8, kChar372 = 0x82329CF0;
// Efectos de las tecnicas por jugador (estudio 18, solo US): fichero por ID en kTechFid; bloque
// del jugador k en *kTechBlock + 1152k, ranura A en +80 (+100 = procesada).
constexpr uint32_t kTechFid = 0x82333208, kTechBlock = 0x82333994;
// Cara del marcador (estudios 17 y 19, solo US): tabla kFaceTable (filas de 16 B: ID, mascara de
// trajes, fichero de caras, retrato animado; fin = ID -1). Al empezar el combate el juego carga la
// hoja de caras de cada jugador (las 6 formas), la apunta en B+4376 (B = *kHudBase) y crea su
// retrato animado en B+4428 (sub_821B62D0 + sub_821B6368); al terminar (sub_820E7058) suelta sus
// hojas (0x823CE010) y los retratos.
constexpr uint32_t kFaceTable = 0x82324468, kHudBase = 0x82324064, kHudFaces = 4376, kHudPortrait = 4428;
// Aura (solo US): fichero por ID en kAuraFid (-1 = se queda la que habia, como al empezar); el
// juego la carga al empezar en el bloque de 420 B del jugador (G = *kAuraBase + 420k) y la suelta
// al final para los dos (sub_820B8E40).
constexpr uint32_t kAuraFid = 0x82322950, kAuraBase = 0x82322AEC;
constexpr uint32_t kLoadTimeout = 600;
constexpr uint32_t kSwapMin = 24;       // ticks del destello antes de cambiar   // ticks (~10 s) esperando las cargas

enum class Phase { kIdle, kLoading };

struct Team {
  std::vector<int> ids;            // personajes del equipo
  bool round_owned = false, round_blocked = false;
  uint32_t generation = 0, round_form = 0;
  int round_suit = 0;
  std::string round_identity;
  std::array<uint16_t, 8> round_caps{};
  std::array<bool, 5> visited{};
  round::RestoreTicket restore_ticket;
  round::PositionCache position;
  int active = 0;                  // miembro en combate
  std::array<float, 5> hp{};       // vida guardada de cada miembro
  std::array<bool, 5> down{};      // caido
  Phase phase = Phase::kIdle;
  int target = -1;                 // miembro que entra
  uint32_t since = 0;              // tick en que empezo la carga
  int retries = 0;                 // cargas pedidas de nuevo por no terminar
  uint32_t new_cam = 0;            // ficheros pedidos para el que entra (aun no son del luchador)
  uint32_t new_lang = 0;
  uint32_t rec = 0;                // registro de 372 B provisional (forma kLoad = su forma 0)
  uint32_t new_tech = 0;           // efectos de sus tecnicas (BSP, 0x82333208[ID]); US
  uint32_t new_face = 0;           // nodo de su hoja de caras del marcador (si no estaba ya); US
  uint32_t new_portrait = 0;       // su retrato animado (sub_821B62D0: carga sus ficheros); US
  uint32_t new_aura = 0;           // nodo de su aura (si es otra que la del que sale); US
  int pending = -1;                // relevo pedido cuando no se podia (miembro que entra)
  uint32_t pending_until = 0;      // tick limite (0 = espera lo que haga falta: KO)
  uint32_t ko_at = 0;              // tick en que cayo el activo
  bool seen25 = false;             // el caido ya ha empezado su caida (accion 25)
  uint32_t done_at = 0;            // tick del ultimo relevo terminado (enfriamiento SELECT)
  bool zero_logged = false;
  uint32_t refused_at = 0;         // tick del ultimo SELECT rechazado (HUD)
  // deslizamiento del relevo (como la llegada del compañero en Budokai 2, guion de Ten Shin Han:
  // aparece a un lado y se desliza a su sitio en ~12 fotogramas): 1 = sale, 2 = entra
  uint8_t slide = 0;
  uint32_t slide_at = 0;
  float slide_a[3]{}, slide_b[3]{}, home[3]{};   // x, altura, z
  // efectos de tecnica de cada miembro (la ranura A ya procesada, 544 B, estudio 18): la del que sale
  // se guarda aqui y no se suelta hasta el final del combate. Soltarla en el relevo dejaba efectos
  // vivos (choque de poderes, el rayo en pantalla) leyendo memoria liberada: cierre del juego
  // (2026-10-12, sub_82150C00 leyendo 0x4)
  std::array<std::array<uint8_t, 544>, 5> tech{};
  std::array<bool, 5> tech_ok{};
  int tech_owner = -1;             // miembro cuyos efectos estan en la ranura del juego
  // transformacion en curso (forma actual X+2980 / destino X+2984): SELECT a mitad de la subida de
  // forma dejaba la camara lejos y el luchador sin dibujar (jugador, 2026-10-12): el relevo espera
  uint32_t last_form = 0xFFFFFFFF;
  uint32_t form_at = 0;            // tick del ultimo cambio de forma (o mientras actual != destino)
  std::array<uint8_t, 5> form{};   // forma con la que salio cada miembro (vuelve con ella)
  uint32_t spare = 0;              // forma del que entra pedida en el hueco kSpare (0 = ninguna)
  uint32_t restore = 0, restore_at = 0;   // forma a la que vuelve el que entro, y desde que tick
  // ring out con compañero vivo (cuenta como KO): desde que tick lo pide el juego, y donde aterriza
  // el que entra (dentro del ring; el caido quedo fuera)
  uint32_t ring_since = 0, ring_last = 0, ring_at = 0;   // ring_at: tick del ultimo ring out convertido
  bool ring = false, ring_giveup = false;                // giveup: el relevo no cargo, el ring out sigue
  float ring_pos[3]{};
  uint32_t pose_at = 0, pose_until = 0;   // pose de entrada (carga de ki del juego) al aterrizar
};

std::mutex g_mu;
std::array<Team, 2> g_team;                   // por lado (0 = P1)
std::array<uint16_t, 4> g_prev_buttons{};
// En combate: desde el reinicio de luchadores del inicio del combate (sub_82109638) hasta que
// vuelve un menu del juego (OnScreen). Fuera de el, los bloques X son de un combate viejo.
std::atomic<bool> g_in_battle{false};
std::atomic<uint32_t> g_frame{0};             // ticks del bucle principal
std::atomic<uint32_t> g_fall_x{0};            // luchador tumbado por KO que aun tiene que salir despedido
std::array<HudSide, 2> g_hud{};               // con g_mu
bool g_in_swap = false;                       // nuestro sub_82109638 (no es un combate nuevo)
std::array<std::atomic<bool>, 2> g_com{};     // lado del ordenador
// Pose de entrada (2026-10-14): al aterrizar, el que entra hace ~0,5 s la carga de ki del propio juego
// (Y+B mantenidos, acciones 20/21). Las acciones del luchador solo nacen de su entrada (X+6048 es el nodo
// de su lista de movimientos): un humano la "pulsa" en su mando (TagPad); el COM no lee mando (su hueco
// X+5028 = -1 lo lleva a sub_820F8470, la IA), asi que desde que baja hasta acabar la pose su hueco apunta
// al mando 4 y el juego lee un mando 4 virtual (conectado solo entonces) con Y+B; despues vuelve a -1.
// Con el mando 4 desconectado de verdad el juego pausaba ("Player 2's controller has been disconnected").
constexpr uint32_t kPoseTicks = 30, kPoseWindow = 120, kPoseSlotOff = 5028, kPoseRec = 0x8238B780 + 184 * 3;
constexpr int16_t kPoseSlot = 3;
std::array<uint32_t, 2> g_pose_com{};           // luchador del COM con el hueco cambiado (con g_mu)
std::array<std::atomic<bool>, 4> g_pose_pad{};  // mando (hueco) que pulsa la pose este tick
std::array<uint32_t, 2> g_pose_ticks{};
std::array<int, 2> g_pose_slot{-1, -1};      // mando que pulsa la pose de cada lado

void PoseEnd(uint8_t* base, int p) {
  if (const uint32_t x = g_pose_com[size_t(p)]) {
    if (int16_t(REX_LOAD_U16(x + kPoseSlotOff)) == kPoseSlot) REX_STORE_U16(x + kPoseSlotOff, 0xFFFF);
    for (const uint32_t o : {12u, 14u, 16u, 88u, 92u}) REX_STORE_U32(kPoseRec + o, 0);
    g_pose_com[size_t(p)] = 0;
  }
  if (const int slot = g_pose_slot[size_t(p)]; slot >= 0) g_pose_pad[size_t(slot)] = false;
  g_pose_slot[size_t(p)] = -1;
  g_pose_ticks[size_t(p)] = 0;
}
std::array<std::map<int, uint32_t>, 2> g_face;  // por lado: ID -> hoja de caras (datos !AZT) del combate
// ultimo tick en que el juego uso un banco de efectos de la ranura de tecnica del lado (sub_82092C58):
// mientras un efecto suyo sigue vivo (rayo en pantalla, choque de poderes) el relevo espera, porque
// esos efectos buscan sus datos en la ranura del lado y tras el cambio ya serian los del que entra
std::array<std::atomic<uint32_t>, 2> g_tech_seen{};// ticks sin efectos; espera maxima (~10 s). Con 8 ticks un efecto del que salia (sus llamadas al
// banco no son seguidas) sobrevivia al relevo, buscaba su id en el banco del que entra y el juego se
// paraba (2026-10-12: trampa en sub_822F8988 via sub_82096FF0, 20-30 ms despues del relevo)
constexpr uint32_t kFxQuiet = 90, kFxWaitMax = 600;
std::vector<uint32_t> g_face_res;             // hojas que cargamos nosotros (se sueltan con las del juego)
constexpr uint32_t kComCheck = 90;            // ticks entre decisiones del ordenador

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

using GuestFn = void (*)(PPCContext&, uint8_t*);
uint32_t Call(PPCContext& ctx, uint8_t* base, GuestFn fn, uint32_t r3, uint32_t r4, uint32_t r5 = 0) {
  PPCContext c = ctx;
  c.r3.u64 = r3;
  c.r4.u64 = r4;
  c.r5.u64 = r5;
  fn(c, base);
  return c.r3.u32;
}
#define TAG_CALL(us, eu, a, b) Call(ctx, base, DBZ3_CALL2(us, eu), (a), uint32_t(b))

// Bloque X del jugador p (0 = P1, 1 = P2).
uint32_t Fighter(uint8_t* base, int p) {
  const uint32_t cfg = REX_LOAD_U32(dbz3::GuestAddr(0x823D6CF4, 0x823D6CF4));
  return cfg ? cfg + 2096 + 8240 * uint32_t(p) : 0;
}

int SideOf(uint8_t* base, uint32_t x) {
  for (int p = 0; p < 2; ++p) {
    if (x && Fighter(base, p) == x) return p;
  }
  return -1;
}

// Equipo del luchador X (nullptr si su lado no lleva equipo). Con g_mu tomado.
Team* TeamOf(uint8_t* base, uint32_t x) {
  const int p = SideOf(base, x);
  if (p < 0) return nullptr;
  Team& t = g_team[size_t(p)];
  return t.ids.size() >= 2 ? &t : nullptr;
}

// Se puede relevar con el luchador propio libre: de pie (accion 0), en guardia (1) o andando (2),
// medido el 2026-10-11. Golpes (15), recibir (25), modo hiper (43/44), Dragon Rush y definitivas no:
// cambiar a mitad de esas escenas rompe el juego.
bool Free(uint8_t* base, uint32_t x) { return x && REX_LOAD_U16(x + 3244) <= 2; }
// Pelea en marcha: 0x823F39A0 vale 1 solo entre el FIGHT! y el final del asalto (0 en la
// presentacion, el READY?, el KO/ganador y el menu final; medido 2026-10-11 con volcados de un
// Duelo, US). En la EU no esta medido: se deja pasar.
// Estado del combate del juego (US): mgr = *0x823D6CF4, mgr+35128 = 1-3 presentacion/READY, 4 pelea,
// 5 combate acabado (KO o tiempo; mgr+35188 = perdedor). Medido 2026-10-12: es fiable donde
// 0x823F39A0 no (en un COM vs COM se quedo a 0 todo el combate; en otro, a 1 sobre el menu final).
uint32_t BattleState(uint8_t* base) {
  const uint32_t mgr = dbz3::g_guest_eu ? 0 : REX_LOAD_U32(0x823D6CF4);
  return mgr ? REX_LOAD_U32(mgr + 35128) : 0;
}
bool Fighting(uint8_t* base) {
  if (const uint32_t st = BattleState(base)) return st == 4;
  return dbz3::g_guest_eu || REX_LOAD_U32(0x823F39A0) != 0;
}
// Presentacion / READY? (antes del FIGHT!): sin relevos ni SELECT
bool PreFight(uint8_t* base) {
  const uint32_t st = BattleState(base);
  return st != 0 && st < 4;
}
// Combate acabado (KO final o tiempo; el menu "Battle Again?"): la bandera de pelea estuvo a 1 en este
// combate y ha vuelto a 0. Sin relevos ni fichas del equipo desde ahi (seguian sobre el menu final,
// 2026-10-12). Si un escenario no la pone nunca a 1, no se bloquea nada.
std::atomic<bool> g_fight_seen{false};
bool Over(uint8_t* base) {
  if (dbz3::g_guest_eu) return false;
  if (const uint32_t st = BattleState(base)) return st >= 5;
  return g_fight_seen.load() && REX_LOAD_U32(0x823F39A0) == 0;
}

// Solo para el ordenador (COM): los dos luchadores libres.
bool BothFree(uint8_t* base) { return Free(base, Fighter(base, 0)) && Free(base, Fighter(base, 1)); }

constexpr uint32_t kKoMin = 90, kCooldown = 90, kSelectBuffer = 120, kRefusedShow = 30;   // ticks
// tras un cambio de forma el relevo espera esto (la subida de forma sigue animandose tras el cambio)
constexpr uint32_t kFormQuiet = 120;
// Tras un KO: el caido ya ha aterrizado (acabo la accion 25, o esta en pie / levantandose) y han
// pasado 90 ticks desde la caida. Ticks y no reloj de pared: online el relevo tiene que caer en el
// mismo tick en los dos PCs.
bool KoLanded(uint8_t* base, uint32_t x, const Team& t, uint32_t now) {
  if (now - t.ko_at < kKoMin) return false;
  const uint16_t a = REX_LOAD_U16(x + 3244);
  // 19 (tumbado) tambien, y a los 5 s de la caida en cualquier caso (un ring out no pasa por la 25 y el
  // relevo no llegaba nunca)
  return (t.seen25 && a != 25) || a == 18 || a == 19 || a == 35 || a == 36 || a <= 2 || now - t.ko_at > 300;
}
// El rival en modo hiper (43/44) no se releva a mitad de la escena.
bool RivalHyper(uint8_t* base, int p) {
  const uint32_t o = Fighter(base, p ^ 1);
  const uint16_t a = o ? REX_LOAD_U16(o + 3244) : 0;
  return a == 43 || a == 44;
}
// Enfriamiento de SELECT: 90 ticks desde el ultimo relevo terminado.
bool Cooled(const Team& t, uint32_t now) { return !t.done_at || now - t.done_at >= kCooldown; }

int Alive(const Team& t, int skip) {
  int n = 0;
  for (int k = 0; k < int(t.ids.size()); ++k) n += k != skip && !t.down[size_t(k)];
  return n;
}

// Miembro que entra: el siguiente vivo, o el que va despues de ese (`second`, SELECT + L).
int Pick(const Team& t, bool second) {
  const int n = int(t.ids.size());
  int found = 0;
  for (int s = 1; s < n; ++s) {
    const int k = (t.active + s) % n;
    if (t.down[size_t(k)]) continue;
    if (!second || found++ == 1) return k;
  }
  return -1;
}

// Fila de la tabla de caras para el ID y el traje (como sub_820E6D70); 0 si no hay.
uint32_t FaceRow(uint8_t* base, uint32_t id, uint32_t costume) {
  uint32_t any = 0;
  for (uint32_t row = kFaceTable; row < kFaceTable + 16 * 256; row += 16) {
    const uint32_t rid = REX_LOAD_U32(row);
    if (rid == 0xFFFFFFFFu) break;
    if (rid != id) continue;
    if (REX_LOAD_U32(row + 4) & (1u << (costume & 31))) return row;
    if (!any) any = row;
  }
  return any;
}

// Marcador del que entra: su hoja de caras (cargada en StartSwap o de un relevo anterior) en B+4376
// y su retrato animado (creado en StartSwap, sus ficheros ya cargados) en B+4428, como
// sub_820EB090 al empezar el combate.
void SwapFace(PPCContext& ctx, uint8_t* base, uint32_t x, Team& t, uint32_t id) {
  const int side = SideOf(base, x);
  const uint32_t hud = REX_LOAD_U32(kHudBase), sel = REX_LOAD_U32(x + kSel);
  if (side < 0 || !hud) return;
  auto& faces = g_face[size_t(side)];
  if (t.new_face) {
    const uint32_t res = REX_LOAD_U32(t.new_face + 192);
    if (res && res != 0xFFFFFFFFu) {
      REX_STORE_U32(t.new_face + 192, 0);   // la hoja se queda; el nodo se suelta (igual que el juego)
      faces[int(id)] = REX_LOAD_U32(res + 208);
      g_face_res.push_back(res);
      Call(ctx, base, sub_820800A8, faces[int(id)], 0);
    }
    Call(ctx, base, sub_82083D08, t.new_face, 0);
    t.new_face = 0;
  }
  const uint32_t obj = t.new_portrait;
  t.new_portrait = 0;
  const auto it = faces.find(int(id));
  if (it == faces.end() || !it->second || !obj) {
    if (obj) Call(ctx, base, sub_821B63F8, obj, 0);
    return;
  }
  REX_STORE_U32(hud + kHudFaces + 4 * uint32_t(side), it->second);
  const uint32_t old = REX_LOAD_U32(hud + kHudPortrait + 4 * uint32_t(side));
  if (old) Call(ctx, base, sub_821B63F8, old, 0);
  REX_STORE_U32(hud + kHudPortrait + 4 * uint32_t(side), obj);
  Call(ctx, base, sub_821B6368, obj, 0);   // lee los ficheros de su retrato: ya cargados
}

// Aura del que entra en el bloque del jugador `side`: fuera la vieja (lo de sub_820B8E40 para un
// jugador), la nueva procesada (sub_820B8820), los punteros a su arbol que deja sub_820B9B40
// (+404/+408) y sus efectos rehechos (sub_820B9E70 suelta los viejos).
void SwapAura(PPCContext& ctx, uint8_t* base, int side, uint32_t node) {
  const uint32_t g = REX_LOAD_U32(kAuraBase);
  if (!g || side < 0) {
    Call(ctx, base, sub_82083D08, node, 0);
    return;
  }
  const uint32_t a = g + 420 * uint32_t(side);
  if (REX_LOAD_U32(a + 4)) {
    Call(ctx, base, sub_82083A68, REX_LOAD_U32(a), 0);
    REX_STORE_U32(a + 4, 0);
  } else if (REX_LOAD_U32(a)) {
    Call(ctx, base, sub_82083D08, REX_LOAD_U32(a), 0);
  }
  for (uint32_t i = 0; i < 48; ++i) {
    const uint32_t p = REX_LOAD_U32(a + 212 + 4 * i);
    if (p) Call(ctx, base, sub_82089C10, p, 0);
    REX_STORE_U32(a + 212 + 4 * i, 0);
  }
  REX_STORE_U32(a, node);
  Call(ctx, base, sub_820B8820, uint32_t(side), 0);
  const uint32_t n1 = REX_LOAD_U32(a + 12);
  REX_STORE_U32(a + 404, REX_LOAD_U32(a + 4 * (n1 + 5)));
  REX_STORE_U32(a + 408, REX_LOAD_U32(a + 4 * (n1 + 63)));
  Call(ctx, base, sub_820B9E70, uint32_t(side), 0);
}

// Empieza el relevo al miembro k. El que esta en pantalla no se toca (el juego lee su modelo y
// su forma cada fotograma): el que entra se pide con el cargador del juego (sub_82109148) en el
// hueco de forma kLoad, con un registro provisional cuya forma kLoad es su forma normal; su
// camara e idioma se guardan aparte. FinishSwap lo cambia todo en un mismo tick.
// Relevo como en Tenkaichi 3 (estudio 10: el que sale sube a toda velocidad y el que entra llega
// volando y aterriza): el que sale salta hacia atras y arriba, acelerando; el que entra baja desde
// ahi frenando y aterriza en el sitio del que salio. Poco desplazamiento en el suelo: 100 unidades
// de lado sacaban al luchador del escenario (2026-10-12).
constexpr float kExitBack = 15.0f, kExitRise = 60.0f;    // unidades del escenario
// ticks de la salida (~0,2 s) y de la llegada (~0,3 s); DBZ3_TAG_SLOW=N los alarga N veces (solo para
// ver el salto en capturas de prueba)
uint32_t SlideTicks(int kind) {
  static const uint32_t slow = std::getenv("DBZ3_TAG_SLOW") ? std::max(1, std::atoi(std::getenv("DBZ3_TAG_SLOW"))) : 1;
  return (kind == 2 ? 18u : 12u) * slow;
}
// Direccion "hacia fuera": del rival al luchador, en el suelo (X, Z en X+64 / X+72).
void AwayDir(uint8_t* base, uint32_t x, float* dx, float* dz) {
  const int side = SideOf(base, x);
  const uint32_t o = side < 0 ? 0 : Fighter(base, side ^ 1);
  float vx = 1.0f, vz = 0.0f;
  if (o) {
    vx = LoadF(base, x + 64) - LoadF(base, o + 64);
    vz = LoadF(base, x + 72) - LoadF(base, o + 72);
  }
  const float len = std::sqrt(vx * vx + vz * vz);
  if (len < 0.01f) { vx = 1.0f; vz = 0.0f; } else { vx /= len; vz /= len; }
  *dx = vx;
  *dz = vz;
}
// a y b = (x, altura, z), como X+64 / X+68 / X+72
void StartSlide(Team& t, uint8_t kind, const float* a, const float* b) {
  t.slide = kind;
  t.slide_at = g_frame.load();
  std::copy(a, a + 3, t.slide_a);
  std::copy(b, b + 3, t.slide_b);
}
// Un tick del salto: en el suelo de a a b con arranque y frenada suaves; en altura la salida acelera
// (despega) y la llegada frena (aterriza).
void StepSlide(uint8_t* base, uint32_t x, Team& t, uint32_t now) {
  if (!t.slide) return;
  float f = float(now - t.slide_at) / float(SlideTicks(t.slide));
  if (f >= 1.0f) f = 1.0f;
  const float e = f * f * (3.0f - 2.0f * f);
  const float h = t.slide == 1 ? f * f : 1.0f - (1.0f - f) * (1.0f - f);
  float p[3];
  for (int i = 0; i < 3; ++i) p[i] = t.slide_a[i] + (t.slide_b[i] - t.slide_a[i]) * (i == 1 ? h : e);
  // la posicion esta tres veces (X+48, X+64 y X+80; medido con volcados el 2026-10-11): con solo
  // X+64 el juego la recalcula de las otras en el fotograma siguiente
  for (const uint32_t o : {48u, 64u, 80u}) {
    for (uint32_t i = 0; i < 3; ++i) StoreF(base, x + o + 4 * i, p[i]);
  }
  if (f >= 1.0f && t.slide == 2) t.slide = 0;   // la salida se queda arriba hasta el cambio
}

// Insertar tras Team: el propietario guest prepara y fija una sola carga.
// US/offline opt-in. EU/online conservan la ruta anterior, sin preferencias F2.
bool RoundEnabled() {
  const char* flag = std::getenv("DBZ3_TAG_F3");
  return flag && std::strcmp(flag, "1") == 0 && !dbz3::g_guest_eu &&
      dbz3::net::Session::Get().link() == dbz3::net::Link::kIdle &&
      dbz3::campaigns::PlayerStartForm() < 0 && dbz3::campaigns::RivalSuit() < 0 &&
      dbz3::ingame::TagTeamMode() && dbz3::ingame::FormSelectorEnabled();
}
bool PlanMember(uint8_t* base, uint32_t x, Team& t, int k) {
  const int side = SideOf(base, x), id = t.ids[size_t(k)];
  // El relevo solo sustituye la tecnica A: rechazar una variacion B no resuelta.
  const int old_id = t.ids[size_t(t.active)];
  if (!dbz3::roster::FormSelectionRange(0x82333404u + 4u * uint32_t(id), 4, false) ||
      !dbz3::roster::FormSelectionRange(0x82333404u + 4u * uint32_t(old_id), 4, false)) return false;
  if (REX_LOAD_U32(0x82333404u + 4u * uint32_t(id)) !=
      REX_LOAD_U32(0x82333404u + 4u * uint32_t(old_id))) {
    REXLOG_WARN("dbz3 tag F3: tecnica B distinta, relevo bloqueado id {}", id);
    return false;
  }
  const int suit = dbz3::forms::MemberSuit(side, id, 0); // nunca el traje del saliente
  auto snapshot = dbz3::roster::ReadForms(id, suit);
  if (!ReadMemberCapsules(base, side, id, t.round_caps)) return false;
  snapshot.capsules.assign(t.round_caps.begin() + 1, t.round_caps.begin() + 1 + t.round_caps[0]);
  const int want = round::OwnRequest(t.visited[size_t(k)], t.form[size_t(k)],
                                   dbz3::forms::MemberRequest(side, id));
  const auto result = dbz3::forms::Resolve(snapshot, want);
  dbz3::forms::Notice(side, snapshot, result); // UI F2 nativa ya existente
  if (!result.valid) return false;
  t.round_suit = suit;
  t.round_identity = snapshot.identity;
  t.round_form = uint32_t(result.applied); // NO result.model_index
  if (result.reason != dbz3::forms::Reason::kOk)
    REXLOG_WARN("dbz3 tag F3: id {} pedida {} base aplicada; motivo {} capsula {}", id, want,
                int(result.reason), result.capsule);
  return true;
}

// Validar los rangos ANTES de Loaded, incluyendo datos de nodos +192.
bool RoundNodesReady(uint8_t* base, uint32_t x, const Team& t) {
  if (!dbz3::roster::FormSelectionRange(x, 8240, true)) return false;
  auto node = [&](uint32_t n) {
    if (!n || !dbz3::roster::FormSelectionRange(n, 196, false)) return false;
    const uint32_t data = REX_LOAD_U32(n + 192);
    return !REX_LOAD_U8(n + 1) && data && data != 0xFFFFFFFFu &&
           dbz3::roster::FormSelectionRange(data, 4, false);
  };
  auto model = [&](uint32_t m) {
    return m && dbz3::roster::FormSelectionRange(m, 40, false) && !REX_LOAD_U32(m + 36);
  };
  if (!node(t.new_cam)) return false;
  for (uint32_t n : {t.new_lang, t.new_tech, t.new_face, t.new_aura}) if (n && !node(n)) return false;
  for (uint32_t slot : {kLoad, kSpare}) {
    if (slot == kSpare && !t.spare) continue;
    if (!model(REX_LOAD_U32(x + kModel0 + 4*slot))) return false;
    const uint32_t a = REX_LOAD_U32(x + kAnm0 + 28*slot + 20);
    if (a && !node(a)) return false;
  }
  if (t.new_portrait) {
    if (!dbz3::roster::FormSelectionRange(t.new_portrait, 16, false)) return false;
    for (uint32_t offset : {8u, 12u}) {
      const uint32_t part = REX_LOAD_U32(t.new_portrait + offset);
      if (!part) continue;
      if (!dbz3::roster::FormSelectionRange(part, 20, false)) return false;
      const uint32_t n = REX_LOAD_U32(part + 16);
      if (n && !node(n)) return false;
    }
  }
  return true;
}

void StartSwap(PPCContext& ctx, uint8_t* base, uint32_t x, Team& t, int k) {
  if (k < 0 || k >= int(t.ids.size()) || k == t.active || t.phase != Phase::kIdle ||
      t.down[size_t(k)]) return;
  if (const int side = SideOf(base, x); side >= 0) PoseEnd(base, side);
  t.pose_at = 0;
  if (t.round_owned && (t.round_blocked || t.restore || !Fighting(base))) return;
  const uint32_t id = uint32_t(t.ids[size_t(k)]);
  const uint32_t sel = REX_LOAD_U32(x + kSel);
  if (!sel) return;
  if (t.round_owned) {
    if (!dbz3::roster::FormSelectionRange(x, 8240, true) ||
        !dbz3::roster::FormSelectionRange(sel, 80, true)) return;
    auto occupied = [&](uint32_t slot) {
      return REX_LOAD_U32(x + kModel0 + 4*slot) || REX_LOAD_U32(x + kAnm0 + 28*slot) ||
          REX_LOAD_U32(x + kAnm0 + 28*slot + 20) || REX_LOAD_U32(x + kAnm0 + 28*slot + 24);
    };
    if (!round::ScratchFree(REX_LOAD_U32(x + kForm), occupied(kSpare), occupied(kLoad)) ||
        !PlanMember(base, x, t, k)) {
      t.refused_at = g_frame.load();
      REXLOG_WARN("dbz3 tag F3: carga rechazada antes de tocar recursos id {}", id);
      return;
    }
  }
  if (!t.rec) t.rec = dbz3::roster::GuestAlloc(372);
  if (!t.rec) return;
  if (t.round_owned && !dbz3::roster::FormSelectionRange(t.rec, 372, true)) return;
  t.hp[size_t(t.active)] = t.down[size_t(t.active)] ? 0.0f : LoadF(base, x + kHp);
  // (si aun no se habia transformado de vuelta, sale con la forma que tenia pendiente)
  t.form[size_t(t.active)] = t.down[size_t(t.active)] ? 0
                             : uint8_t(std::min<uint32_t>(t.restore ? t.restore : REX_LOAD_U32(x + kForm), 7));
  if (t.round_owned) {
    t.form[size_t(t.active)] = t.down[size_t(t.active)] ? 0 : uint8_t(REX_LOAD_U32(x + kForm));
    t.visited[size_t(t.active)] = true;
  }
  t.restore = 0;
  if (t.spare) {   // la forma pedida en un intento anterior (carga que no llego) se suelta
    TAG_CALL(sub_82100D60, dbz3eu_sub_82100E60, x, kSpare);
    TAG_CALL(sub_82100DE8, dbz3eu_sub_82100EE8, x, kSpare);
    t.spare = 0;
  }
  TAG_CALL(sub_82100D60, dbz3eu_sub_82100E60, x, kLoad);       // hueco libre
  TAG_CALL(sub_82100DE8, dbz3eu_sub_82100EE8, x, kLoad);
  const uint32_t src = kChar372 + 372 * id;
  std::memcpy(base + t.rec, base + src, 372);
  std::memcpy(base + t.rec + 4 * kLoad, base + src, 4);                       // su moveset
  std::memcpy(base + t.rec + 212 + 20 * kLoad, base + src + 212, 20);         // su modelo
  // vuelve con la forma con la que salio: su modelo y animaciones de esa forma f en el hueco kSpare
  // (sub_82109148 pide sel+22 y luego sel+20; hueco libre: el que esta en pantalla no lo usa). En
  // FinishSwap pasan al hueco f y se transforma con la rutina del juego (sub_82115330), como con P+K+G
  const uint32_t f = t.round_owned ? t.round_form : t.form[size_t(k)];
  const bool spare_free = !REX_LOAD_U32(x + kModel0 + 4 * kSpare) && !REX_LOAD_U32(x + kAnm0 + 28 * kSpare) &&
                          !REX_LOAD_U32(x + kAnm0 + 28 * kSpare + 20) && !REX_LOAD_U32(x + kAnm0 + 28 * kSpare + 24);
  t.spare = (f > 0 && f <= (t.round_owned ? 7u : kSpare) && spare_free) ? f : 0;
  if (t.spare) {
    std::memcpy(base + t.rec + 4 * kSpare, base + src + 4 * f, 4);
    std::memcpy(base + t.rec + 212 + 20 * kSpare, base + src + 212 + 20 * f, 20);
    REX_STORE_U8(x + 7436 + kSpare, REX_LOAD_U8(x + 7436));
  }
  const uint32_t keep[] = {REX_LOAD_U32(x + kRec372), REX_LOAD_U32(x + kId), REX_LOAD_U32(x + kCam),
                           REX_LOAD_U32(x + kLang), REX_LOAD_U32(x + 7928)};
  const uint8_t suit6 = REX_LOAD_U8(x + 7436 + kSpare), suit7 = REX_LOAD_U8(x + 7436 + kLoad);
  if (t.round_owned) REX_STORE_U8(x + 7436 + kSpare, uint8_t(t.round_suit));
  const uint16_t sel20 = REX_LOAD_U16(sel + 20), sel22 = REX_LOAD_U16(sel + 22);
  REX_STORE_U8(x + 7436 + kLoad, REX_LOAD_U8(x + 7436));      // mismo traje
  if (t.round_owned) REX_STORE_U8(x + 7436 + kLoad, uint8_t(t.round_suit));
  REX_STORE_U32(x + kRec372, t.rec);
  REX_STORE_U32(x + kId, id);
  REX_STORE_U32(x + kCam, 0);
  REX_STORE_U32(x + kLang, 0);
  REX_STORE_U16(sel + 20, uint16_t(kLoad));
  REX_STORE_U16(sel + 22, uint16_t(t.spare ? kSpare : kLoad));
  TAG_CALL(sub_82109148, dbz3eu_sub_82109248, x, sel);         // pide sus ficheros
  t.new_cam = REX_LOAD_U32(x + kCam);
  t.new_lang = REX_LOAD_U32(x + kLang);
  t.new_tech = 0;
  if (!dbz3::g_guest_eu) {
    const uint32_t fid = REX_LOAD_U32(kTechFid + 4 * id);
    if (fid != 0xFFFFFFFFu && !t.tech_ok[size_t(k)] && t.tech_owner != k) {   // si ya los tiene guardados, no se cargan
      const uint32_t h = Call(ctx, base, sub_82083968, fid, 0, 1);
      t.new_tech = h != 0xFFFFFFFFu ? h : 0;
    }
    // su hoja de caras del marcador (la del que sale se apunta la primera vez: es la del juego)
    const int side = SideOf(base, x);
    const uint32_t hud = REX_LOAD_U32(kHudBase);
    auto& faces = g_face[size_t(side)];
    if (hud && !faces.count(int(keep[1]))) faces[int(keep[1])] = REX_LOAD_U32(hud + kHudFaces + 4 * uint32_t(side));
    const uint32_t row = FaceRow(base, id, t.round_owned ? uint32_t(t.round_suit) : REX_LOAD_U16(sel + 18));
    t.new_face = 0;
    if (row && !faces.count(int(id))) {
      const uint32_t h = Call(ctx, base, sub_82083968, REX_LOAD_U32(row + 8), 0, 1);
      t.new_face = h != 0xFFFFFFFFu ? h : 0;
    }
    if (t.new_portrait) Call(ctx, base, sub_821B63F8, t.new_portrait, 0);
    t.new_portrait = row ? Call(ctx, base, sub_821B62D0, REX_LOAD_U32(row + 12), 0) : 0;
    // su aura, si es otra (sin aura propia, -1, se queda la que habia)
    const uint32_t afid = REX_LOAD_U32(kAuraFid + 4 * id);
    t.new_aura = 0;
    if (afid != 0xFFFFFFFFu && afid != REX_LOAD_U32(kAuraFid + 4 * keep[1])) {
      const uint32_t h = Call(ctx, base, sub_82083968, afid, 0, 1);
      t.new_aura = h != 0xFFFFFFFFu ? h : 0;
    }
  }
  REX_STORE_U32(x + kRec372, keep[0]);                        // el de pantalla sigue igual
  REX_STORE_U32(x + kId, keep[1]);
  REX_STORE_U32(x + kCam, keep[2]);
  REX_STORE_U32(x + kLang, keep[3]);
  REX_STORE_U32(x + 7928, keep[4]);
  REX_STORE_U16(sel + 20, sel20);
  REX_STORE_U16(sel + 22, sel22);
  if (t.round_owned) {
    REX_STORE_U8(x + 7436 + kSpare, suit6);
    REX_STORE_U8(x + 7436 + kLoad, suit7);
  }
  t.phase = Phase::kLoading;
  t.target = k;
  t.since = g_frame.load();
  for (uint32_t i = 0; i < 3; ++i) t.home[i] = LoadF(base, x + 64 + 4 * i);
  if (!t.down[size_t(t.active)]) {   // el que sale salta hacia atras y arriba (el caido se queda tumbado)
    float dx, dz;
    AwayDir(base, x, &dx, &dz);
    const float out[3] = {t.home[0] + dx * kExitBack, t.home[1] + kExitRise, t.home[2] + dz * kExitBack};
    StartSlide(t, 1, t.home, out);
    // (2026-10-14: una accion nativa para la pose de salida no se puede pedir aqui: en reposo X+6048
    // es nulo y sub_82103908 lo necesita; haria falta el reproductor de movimientos, sub_82136328)
  } else {
    t.slide = 0;
  }
  REXLOG_INFO("dbz3 tag: luchador {:08X} relevo {} -> {} (id {}, camara {:08X}, sale en forma {}, entra en forma {})", x,
              t.active, k, id, t.new_cam, t.form[size_t(t.active)], t.spare);
}

// Ficheros del que entra cargados: en este tick sale el de pantalla (sub_821092C8 libera su
// camara, idioma y formas), entra el nuevo (su modelo y moveset al hueco de la forma 0, su ID,
// registro y capsulas) y se termina como al empezar un combate.
void FinishSwap(PPCContext& ctx, uint8_t* base, uint32_t x, Team& t) {
  const int k = t.target;
  const bool ko = t.down[size_t(t.active)];   // entra por un KO (el que sale esta en el suelo)
  const uint8_t near_death = REX_LOAD_U8(x + kNearDeath);
  const uint32_t id = uint32_t(t.ids[size_t(k)]);
  const uint32_t model = REX_LOAD_U32(x + kModel0 + 4 * kLoad);
  uint8_t anm[28];
  std::memcpy(anm, base + x + kAnm0 + 28 * kLoad, 28);
  REX_STORE_U32(x + kModel0 + 4 * kLoad, 0);
  std::memset(base + x + kAnm0 + 28 * kLoad, 0, 28);
  // su forma de vuelta (hueco kSpare) tambien se aparta de la limpieza del que sale
  const uint32_t spare = t.spare, smodel = REX_LOAD_U32(x + kModel0 + 4 * kSpare);
  uint8_t sanm[28];
  std::memcpy(sanm, base + x + kAnm0 + 28 * kSpare, 28);
  if (spare) {
    REX_STORE_U32(x + kModel0 + 4 * kSpare, 0);
    std::memset(base + x + kAnm0 + 28 * kSpare, 0, 28);
  }
  t.spare = 0;
  TAG_CALL(sub_821092C8, dbz3eu_sub_821093C8, x, 0);           // fuera el que sale
  REX_STORE_U32(x + kModel0, model);
  std::memcpy(base + x + kAnm0, anm, 28);
  REX_STORE_U32(x + kCamRoot, 0);
  REX_STORE_U32(x + kCam, t.new_cam);
  REX_STORE_U32(x + kLangFlag, 0);
  REX_STORE_U32(x + kLang, t.new_lang);
  REX_STORE_U32(x + kId, id);
  REX_STORE_U32(x + kRec372, kChar372 + 372 * id);
  REX_STORE_U32(x + kForm, 0);
  REX_STORE_U32(x + kNextForm, 0);
  const uint32_t sel = REX_LOAD_U32(x + kSel);
  if (sel) {
    REX_STORE_U16(sel + 16, uint16_t(id));
    REX_STORE_U16(sel + 20, 0);
    REX_STORE_U16(sel + 22, 0);
    if (t.round_owned) {
      REX_STORE_U16(sel + 18, uint16_t(t.round_suit));
      for (uint32_t c = 0; c < 8; ++c) REX_STORE_U16(sel + 52 + 2*c, t.round_caps[c]);
      // sub_82113870 resuelve 7436 por modelo/traje; no sobrescribir sus clamps.
    } else PutCapsules(base, sel, SideOf(base, x), int(id));
  }
  // lo que sub_82109BA0 prepara por luchador al fijar su ID (capsulas y tecnicas)
  TAG_CALL(sub_82113818, dbz3eu_sub_82113918, x, 0);
  TAG_CALL(sub_82113870, dbz3eu_sub_82113970, x, 0);
  TAG_CALL(sub_82112260, dbz3eu_sub_82112360, x, 0);
  TAG_CALL(sub_8210FDE0, dbz3eu_sub_8210FEE0, x, 0);           // golpes y camara (X+7896..)
  // X+5028/5030 (sub_82110040 al cargar) siguen con lo del inicio del combate
  // el reinicio lo pone en la salida del asalto (+-15) y con su lado de salida (X+8185): el que
  // entra sigue donde estaba el que sale y mirando igual
  uint8_t pos[16];
  std::memcpy(pos, base + x + 64, sizeof(pos));
  const uint8_t side_flag = REX_LOAD_U8(x + 8185);
  // el que entra no hereda el "caido" del que salio (X+6248 lo pone la accion 25; X+8189 es el control)
  REX_STORE_U32(x + 6248, 0);
  REX_STORE_U8(x + 8189, 0);
  g_in_swap = true;
  TAG_CALL(sub_82109638, dbz3eu_sub_82109738, x, 0);           // reinicio del luchador
  g_in_swap = false;
  TAG_CALL(sub_8210E690, dbz3eu_sub_8210E790, x, 0);
  TAG_CALL(sub_82115270, dbz3eu_sub_82115370, x, 0);           // el cuerpo pasa al modelo nuevo
  // su forma de vuelta al hueco de esa forma, ya pasado el reinicio (con el modelo puesto antes, el
  // hueco salia vacio al aterrizar, 2026-10-12)
  if (spare) {
    REX_STORE_U32(x + kModel0 + 4 * spare, smodel);
    std::memcpy(base + x + kAnm0 + 28 * spare, sanm, 28);
  }
  std::memcpy(base + x + 64, pos, sizeof(pos));
  REX_STORE_U8(x + 8185, side_flag);
  REXLOG_INFO("dbz3 tag: posicion del relevo {:.2f} {:.2f} {:.2f} {:.2f}", LoadF(base, x + 64), LoadF(base, x + 68),
              LoadF(base, x + 72), LoadF(base, x + 76));
  // efectos de sus tecnicas, solo en la ranura A del jugador (estudio 18): los del que sale se guardan
  // (no se sueltan hasta el final del combate, ver Team::tech) y entran los del nuevo: los guardados de
  // un relevo anterior o los recien cargados, procesados con sub_82126490
  if (!dbz3::g_guest_eu && (t.new_tech || t.tech_ok[size_t(k)]) && REX_LOAD_U32(kTechBlock) && SideOf(base, x) >= 0) {
    const int side = SideOf(base, x);
    const uint32_t slot = REX_LOAD_U32(kTechBlock) + 1152 * uint32_t(side) + 80;
    if (t.tech_owner >= 0) {
      std::memcpy(t.tech[size_t(t.tech_owner)].data(), base + slot, 544);
      t.tech_ok[size_t(t.tech_owner)] = true;
    }
    if (t.tech_ok[size_t(k)]) {
      std::memcpy(base + slot, t.tech[size_t(k)].data(), 544);
      t.tech_ok[size_t(k)] = false;
      if (t.new_tech) Call(ctx, base, sub_82083D08, t.new_tech, 0);
    } else {
      std::memset(base + slot, 0, 544);
      REX_STORE_U32(slot, t.new_tech);
      Call(ctx, base, sub_82126490, uint32_t(side), 0);
    }
    t.tech_owner = k;
    t.new_tech = 0;
    // reenlace de los punteros de tecnica en uso (S1 0x82333990, S2 0x82333998) con la ranura nueva,
    // como al empezar el combate (sub_82125888 / sub_8212AF48: r4 = -1, la ranura que toca); si no,
    // seguian en la del que salio (Kamehameha feo, efecto 34 buscado en el banco viejo)
    Call(ctx, base, sub_82125808, uint32_t(side), 0xFFFFFFFFu);
    Call(ctx, base, sub_8212AEC0, uint32_t(side), 0xFFFFFFFFu);
    // la ranura B (segunda tabla 0x82333404, solo algunos personajes) se queda como estaba: se anota
    REXLOG_INFO("dbz3 tag: tecnicas del lado {}: ranura A del miembro {}; ranura B {:08X} (estado {}, tabla B {:08X})", side + 1, k,
                REX_LOAD_U32(slot + 544), REX_LOAD_U16(0x823D76EE + 1152 * uint32_t(side)), REX_LOAD_U32(0x82333404 + 4 * id));
  }
  if (!dbz3::g_guest_eu) SwapFace(ctx, base, x, t, id);   // cara y formas del marcador
  if (t.new_aura) {
    SwapAura(ctx, base, SideOf(base, x), t.new_aura);
    t.new_aura = 0;
  }
  // llegada: el que entra baja desde atras y arriba y aterriza en el sitio del que salio. Tras un KO
  // igual, como en Tenkaichi 3 (el companero llega volando cuando el derrotado ya esta en el suelo), y
  // ademas con el destello de transformacion del juego (como Freezer al volver como Mecha Freezer,
  // abajo). Si cayo mientras salia de un relevo pedido, aterriza donde estaba (t.home).
  {
    float dx, dz;
    AwayDir(base, x, &dx, &dz);
    float home[3];
    for (uint32_t i = 0; i < 3; ++i)
      home[i] = t.ring ? t.ring_pos[i] : t.slide == 1 ? t.home[i] : LoadF(base, x + 64 + 4 * i);
    t.ring = false;   // tras un ring out, dentro del ring (no donde quedo el caido)
    t.ring_at = 0;    // el ring out ya se resolvio: los KO y relevos siguientes van por su camino normal
    const float from[3] = {home[0] + dx * kExitBack, home[1] + kExitRise, home[2] + dz * kExitBack};
    StartSlide(t, 2, from, home);
    StepSlide(base, x, t, g_frame.load());
  }
  // en guardia: sin movimiento en curso ni accion (como el rival en reposo; el reinicio lo
  // dejaba en la pose de antes de empezar el asalto)
  REX_STORE_U32(x + 6048, 0);
  REX_STORE_U16(x + 3244, 0);
  REX_STORE_U16(x + 3246, 0);
  REX_STORE_U16(x + 3252, 0);
  if (ko && !spare) TAG_CALL(sub_820FB5D0, dbz3eu_sub_820FB6D0, x + kBody, 0);   // destello (forma 0)
  t.restore = spare;                    // se transforma al aterrizar (Tick)
  t.restore_at = g_frame.load() + SlideTicks(2) + 6;
  static const bool no_pose = std::getenv("DBZ3_TAG_NO_POSE") != nullptr;
  t.pose_at = (spare || no_pose || dbz3::g_guest_eu) ? 0 : t.restore_at;
  t.pose_until = t.pose_at + kPoseWindow;   // tope para empezar; la pose dura kPoseTicks
  // el COM pasa ya al mando 4 (sin nada pulsado): baja y aterriza quieto (su IA empezaba a moverse al
  // aterrizar y nunca quedaba libre para la pose)
  if (const int side = SideOf(base, x); t.pose_at && side >= 0 && int16_t(REX_LOAD_U16(x + kPoseSlotOff)) == -1 &&
                                        !dbz3::net::Session::Get().active()) {
    for (const uint32_t o : {12u, 14u, 16u, 88u, 92u}) REX_STORE_U32(kPoseRec + o, 0);
    REX_STORE_U16(x + kPoseSlotOff, uint16_t(kPoseSlot));
    g_pose_com[size_t(side)] = x;
  }
  if (t.round_owned) {
    t.visited[size_t(k)] = true;
    t.restore_ticket.Begin(t.generation, int(id), int(spare), g_frame.load());
  }
  t.active = k;
  t.phase = Phase::kIdle;
  t.retries = 0;
  t.done_at = g_frame.load();
  const float mx = LoadF(base, x + kHpMax);
  StoreF(base, x + kHp, std::min(t.hp[size_t(k)], mx > 0 ? mx : t.hp[size_t(k)]));
  REX_STORE_U8(x + kNearDeath, near_death);   // en Practica vale 1 (vida infinita)
  REXLOG_INFO("dbz3 tag: luchador {:08X} en combate: id {} ({} ticks de carga)", x, id, g_frame.load() - t.since);
}

bool Loaded(uint8_t* base, uint32_t x, const Team& t) {
  // nodo de sub_82083968: +1 = 1 mientras se lee (medido 2026-10-11: ~16 ticks)
  auto ready = [&](uint32_t node) { return node && !REX_LOAD_U8(node + 1); };
  const uint32_t model = REX_LOAD_U32(x + kModel0 + 4 * kLoad);
  const uint32_t anm = REX_LOAD_U32(x + kAnm0 + 28 * kLoad + 20);
  auto loaded = [&](uint32_t node) { return ready(node) && REX_LOAD_U32(node + 192) + 1 > 1; };   // +192 ni 0 ni -1
  // retrato: sus dos partes (+12, +8) con su nodo en +16 (sub_821B6050 lee el +192 del nodo)
  auto part = [&](uint32_t so) { return !so || !REX_LOAD_U32(so + 16) || loaded(REX_LOAD_U32(so + 16)); };
  const bool face = (!t.new_face || loaded(t.new_face)) && (!t.new_aura || loaded(t.new_aura)) &&
                    (!t.new_portrait || (part(REX_LOAD_U32(t.new_portrait + 12)) && part(REX_LOAD_U32(t.new_portrait + 8))));
  // la forma con la que vuelve (hueco kSpare), si se pidio
  const uint32_t smodel = REX_LOAD_U32(x + kModel0 + 4 * kSpare), sanm = REX_LOAD_U32(x + kAnm0 + 28 * kSpare + 20);
  const bool spare = !t.spare || (smodel && !REX_LOAD_U32(smodel + 36) && (!sanm || ready(sanm)));
  return ready(t.new_cam) && (!t.new_lang || ready(t.new_lang)) && (!t.new_tech || ready(t.new_tech)) && face &&
         model &&
         !REX_LOAD_U32(model + 36) &&
         (!anm || ready(anm)) && spare;
}

}  // namespace

std::atomic<uint16_t> g_net_local{0};                      // relevos cargados en este PC (bits del mando)
std::atomic<uint16_t> g_net_applied[2] = {};               // los mismos bits en los mandos aplicados
uint16_t NetReadyBits() { return g_net_local.load(); }
void NetApplied(uint32_t user, uint16_t buttons) {
  if (user < 2) g_net_applied[user] = uint16_t(buttons & 0x0C00);
}

// sub_82103908 (cambiar la accion del luchador) lee X+6048: sin ese contexto (luchador a medio
// cargar o recien relevado) el juego se cerraba. Todas las llamadas pasan por aqui.
static bool ActionCtxOk(uint8_t* base, uint32_t x) {
  const uint32_t action_ctx = REX_LOAD_U32(x + 6048);
  return action_ctx != 0 && dbz3::roster::FormSelectionRange(action_ctx, 4, false);
}

// Cada tick del juego: termina los relevos cuyos ficheros ya estan y copia el marcador.
void Tick(PPCContext& ctx, uint8_t* base) {
  const uint32_t now = ++g_frame;
  if (const uint32_t fx = g_fall_x.exchange(0)) {
    static const int act = std::getenv("DBZ3_KO_ACTION") ? std::atoi(std::getenv("DBZ3_KO_ACTION")) : 25;
    if (act >= 0 && !dbz3::g_guest_eu && REX_LOAD_U16(fx + 3244) != uint16_t(act) && ActionCtxOk(base, fx)) {   // accion medida en la US
      REXLOG_INFO("dbz3 tag: KO de {:08X}: sale despedido (accion {})", fx, act);
      Call(ctx, base, sub_82103908, fx, uint32_t(act));
    }
  }
  // Prueba de campanas (DBZ3_TEST_KO=1): el rival (jugador 2) se queda sin vida a los ~2 s de combate, para
  // recorrer una historia entera sin jugarla.
  static const bool test_ko = std::getenv("DBZ3_TEST_KO") != nullptr;
  static uint32_t battle_frames = 0;
  if (!g_in_battle.load()) {
    battle_frames = 0;
    if (g_pose_com[0] || g_pose_com[1] || g_pose_slot[0] >= 0 || g_pose_slot[1] >= 0) {   // nada pulsado en los menus
      std::lock_guard<std::mutex> lock(g_mu);
      PoseEnd(base, 0);
      PoseEnd(base, 1);
    }
    return;
  }
  // el jugador no pierde vida y, desde los 2 s, el rival se queda a 0 hasta que un golpe lo tumba
  {  // sonda (DBZ3_ACT_PROBE, US): estado del combate (mgr+35128) y modo (mgr+2018), mgr = *0x823D6CF4
    static const bool probe = std::getenv("DBZ3_ACT_PROBE") != nullptr;
    static uint32_t last_state = 0xFFFFFFFF, last_mode = 0xFFFFFFFF;
    const uint32_t mgr = (probe && !dbz3::g_guest_eu) ? REX_LOAD_U32(0x823D6CF4) : 0;
    if (mgr) {
      const uint32_t st = REX_LOAD_U32(mgr + 35128), mode = REX_LOAD_U16(mgr + 2018);
      if (st != last_state || mode != last_mode)
        REXLOG_INFO("dbz3 stateprobe: estado {} modo {} perdedor {:08X} pelea {}", st, mode, REX_LOAD_U32(mgr + 35188),
                    REX_LOAD_U32(0x823F39A0));
      last_state = st;
      last_mode = mode;
    }
  }
  // PRUEBA (DBZ3_TEST_RINGOUT=1): a los 5 s de pelea (no de presentacion: antes saltaba en la intro)
  // el rival aparece fuera del ring (para medir el ring out)
  static const bool test_ro = std::getenv("DBZ3_TEST_RINGOUT") != nullptr;
  if (test_ro && battle_frames == 300 && !dbz3::g_guest_eu) {
    // como el juego al caer fuera del ring (medido 2026-10-14): aviso pendiente mgr+35569 = hueco del
    // GANADOR + 1 (el manejador del modo, sub_820FD678, lo da como "motivo 4"). 1 = gana el jugador 1, cae
    // el rival. Mover al rival no siempre lo provocaba: no todos los escenarios tienen ring.
    if (const uint32_t mgr = REX_LOAD_U32(0x823D6CF4)) {
      REX_STORE_U8(mgr + 35569, 1);
      REXLOG_INFO("dbz3 tag: prueba: rival fuera del ring");
    }
  }
  if (test_ro && BattleState(base) == 4) ++battle_frames;
  if (test_ko) {
    if (const uint32_t x1 = Fighter(base, 0)) StoreF(base, x1 + kHp, LoadF(base, x1 + kHpMax));
    if (++battle_frames >= 120) {
      if (const uint32_t x2 = Fighter(base, 1)) {
        // a 1 (barra vacia): el golpe que lo tumba pasa por sub_8211A5A8, como un KO de verdad
        if (LoadF(base, x2 + kHp) > 1.0f) StoreF(base, x2 + kHp, 1.0f);
        if (battle_frames == 120) REXLOG_INFO("dbz3 tag: prueba: el rival cae");
      }
    }
  }
  if (!dbz3::g_guest_eu && REX_LOAD_U32(0x823F39A0) != 0) g_fight_seen = true;  // DIAGNOSTICO (fin por tiempo): cambios de la bandera de pelea y de la accion de cada luchador
  if (static const bool fdiag = std::getenv("DBZ3_TAG_FLAGDIAG") != nullptr; fdiag && !dbz3::g_guest_eu) {
    static uint32_t last_flag = 0xFFFFFFFFu;
    const uint32_t fl = REX_LOAD_U32(0x823F39A0);
    if (fl != last_flag) {
      const uint32_t a = Fighter(base, 0), b = Fighter(base, 1);
      REXLOG_INFO("dbz3 tag: bandera de pelea {:08X} -> {:08X} (acciones {} {}, 39A4 {:08X} 39A8 {:08X})", last_flag, fl,
                  a ? REX_LOAD_U16(a + 3244) : 0, b ? REX_LOAD_U16(b + 3244) : 0, REX_LOAD_U32(0x823F39A4),
                  REX_LOAD_U32(0x823F39A8));
      last_flag = fl;
    }
    static uint32_t tick = 0;
    if (++tick % 60 == 0 && g_in_battle.load()) {   // cada segundo: acciones, pantalla y zona de la bandera
      const uint32_t a = Fighter(base, 0), b = Fighter(base, 1);
      char zone[160] = {};
      for (uint32_t i = 0; i < 16; ++i)
        std::snprintf(zone + 9 * i, sizeof(zone) - 9 * i, "%08X ", REX_LOAD_U32(0x823F3980 + 4 * i));
      REXLOG_INFO("dbz3 tag: estado acciones {} {} pantalla {} zona {}", a ? REX_LOAD_U16(a + 3244) : 0,
                  b ? REX_LOAD_U16(b + 3244) : 0, base[dbz3::GuestAddr(0x824BA110, 0x824BA100)], zone);
    }
  }
  const bool over = Over(base);
  std::lock_guard<std::mutex> lock(g_mu);
  for (int p = 0; p < 2; ++p) {
    const uint32_t x = Fighter(base, p);
    Team* tp = TeamOf(base, x);
    g_hud[size_t(p)] = HudSide{};
    if (!tp || over) {
      PoseEnd(base, p);
      continue;
    }
    Team& t = *tp;
    // posicion no valida (NaN): tras algunas caidas por KO quedaba asi y golpearle cerraba el juego
    // (2026-10-12, logs 960/962). Vuelve a la ultima buena de ese lado.
    {
      static std::array<std::array<float, 3>, 2> good{};
      const float px = LoadF(base, x + 64), py = LoadF(base, x + 68), pz = LoadF(base, x + 72);
      if (t.round_owned) {
        if (!t.position.Remember({px, py, pz})) {
          if (!t.position.valid) {
            if (!t.round_blocked) REXLOG_WARN("dbz3 tag F3: posicion invalida sin muestra de esta ronda; bloqueo");
            t.round_blocked = true;
            continue; // no teletransportar al origen ni al escenario anterior
          }
          for (const uint32_t o : {48u, 64u, 80u})
            for (uint32_t i = 0; i < 3; ++i) StoreF(base, x + o + 4*i, t.position.value[i]);
          REXLOG_WARN("dbz3 tag F3: posicion restaurada desde esta ronda");
        }
      } else if (std::isfinite(px) && std::isfinite(py) && std::isfinite(pz)) {
        good[size_t(p)] = {px, py, pz};
      } else {
        for (const uint32_t o : {48u, 64u, 80u})
          for (uint32_t i = 0; i < 3; ++i) StoreF(base, x + o + 4 * i, good[size_t(p)][i]);
        REXLOG_WARN("dbz3 tag: luchador {:08X} con posicion no valida: vuelve a {:.1f} {:.1f} {:.1f}", x,
                    good[size_t(p)][0], good[size_t(p)][1], good[size_t(p)][2]);
      }
    }
    if (!t.down[size_t(t.active)] && LoadF(base, x + kHp) <= 0.0f && Alive(t, t.active) > 0 && !t.zero_logged) {
      t.zero_logged = true;   // diagnostico: KO que no paso por sub_8211A5A8
      REXLOG_WARN("dbz3 tag: luchador {:08X} a vida 0 sin pasar por el golpe (accion {})", x, REX_LOAD_U16(x + 3244));
    }
    if (t.down[size_t(t.active)]) {
      if (REX_LOAD_U16(x + 3244) == 25) t.seen25 = true;               // ha empezado la caida
      if (LoadF(base, x + kHp) < 1.0f) StoreF(base, x + kHp, 1.0f);    // caido: vida 1, no termina el combate
    }
    {
      const uint32_t form = REX_LOAD_U32(x + kForm);
      if (form != t.last_form || REX_LOAD_U32(x + kNextForm) != form) t.form_at = now;
      t.last_form = form;
    }
    // vuelve con su forma: ya aterrizado, libre y con su estado de combate rehecho (X+6048; justo tras
    // el reinicio es nulo y las rutinas de accion cerraban el juego), la transformacion del juego (lo que
    // hace P+K+G tras cargar el modelo: destino X+2984, sub_82115330), con el modelo en su hueco
    if (t.round_owned && t.restore) {
      const int id = t.ids[size_t(t.active)];
      auto snapshot = dbz3::roster::ReadForms(id, t.round_suit);
      snapshot.capsules.assign(t.round_caps.begin() + 1, t.round_caps.begin() + 1 + t.round_caps[0]);
      const uint32_t model = REX_LOAD_U32(x + kModel0 + 4 * t.restore);
      const bool model_ready = model && dbz3::roster::FormSelectionRange(model, 40, false) &&
                               !REX_LOAD_U32(model + 36);
      const uint32_t context = REX_LOAD_U32(x + 6048);
      const bool quiet = now - t.form_at >= kFormQuiet && REX_LOAD_U32(x + kForm) == REX_LOAD_U32(x + kNextForm);
      round::RestoreObservation observation{t.generation, int(REX_LOAD_U32(x + kId)),
          int(REX_LOAD_U32(x + kForm)), int(REX_LOAD_U32(x + kNextForm)),
          snapshot.identity == t.round_identity &&
              dbz3::forms::Availability(snapshot, int(t.restore)) == dbz3::forms::Reason::kOk,
          model_ready, context && dbz3::roster::FormSelectionRange(context, 28, false),
          Free(base, x), !t.slide, Fighting(base), quiet};
      const auto step = t.restore_ticket.Poll(observation, now);
      if (step == round::RestoreStep::kIssue) {
        REX_STORE_U32(x + kNextForm, t.restore);
        TAG_CALL(sub_82115330, dbz3eu_sub_82115430, x, 0);
        REXLOG_INFO("dbz3 tag F3: restauracion PEDIDA id {} forma {}", id, t.restore);
      } else if (step == round::RestoreStep::kConfirmed) {
        t.form[size_t(t.active)] = uint8_t(t.restore);
        t.restore = 0;
        REXLOG_INFO("dbz3 tag F3: restauracion OBSERVADA id {}", id);
      } else if ((step == round::RestoreStep::kBlocked || step == round::RestoreStep::kStale) && !t.round_blocked) {
        t.round_blocked = true;
        REXLOG_WARN("dbz3 tag F3: restauracion bloqueada id {}; terminar combate por menu nativo", id);
      }
    }
    if (!t.round_owned && t.restore && t.phase == Phase::kIdle && now >= t.restore_at && !t.slide && Free(base, x) &&
        REX_LOAD_U32(x + 6048)) {
      if (REX_LOAD_U32(x + kModel0 + 4 * t.restore) && REX_LOAD_U32(x + kForm) == 0) {
        REX_STORE_U32(x + kNextForm, t.restore);
        TAG_CALL(sub_82115330, dbz3eu_sub_82115430, x, 0);
        REXLOG_INFO("dbz3 tag: luchador {:08X} vuelve en la forma {}", x, t.restore);
      } else {
        REXLOG_WARN("dbz3 tag: luchador {:08X} no vuelve en la forma {} (modelo {:08X}, forma {})", x, t.restore,
                    REX_LOAD_U32(x + kModel0 + 4 * t.restore), REX_LOAD_U32(x + kForm));
      }
      t.restore = 0;
    }
    // PRUEBA (DBZ3_TAG_AUTOSWAP=N): cada N ticks cada lado pide un relevo como un SELECT (combates COM
    // contra COM largos con muchos relevos, rayos y choques de poderes, sin nadie al mando)
    static const uint32_t autoswap = std::getenv("DBZ3_TAG_AUTOSWAP") ? uint32_t(std::atoi(std::getenv("DBZ3_TAG_AUTOSWAP"))) : 0;
    if (autoswap && t.phase == Phase::kIdle && t.pending < 0 && now % autoswap == uint32_t(p) * autoswap / 2) {
      t.pending = Pick(t, false);
      t.pending_until = now + kSelectBuffer;
    }
    const bool form_quiet = now - t.form_at >= kFormQuiet;
    if (t.phase == Phase::kIdle && t.pending >= 0) {
      if (!form_quiet && t.pending_until) t.pending_until = std::max(t.pending_until, now + kSelectBuffer);   // espera a la forma
      if (t.pending_until && now > t.pending_until) {   // SELECT viejo (0x823F39A0 no vale en todos los escenarios)
        t.pending = -1;
      } else if (t.down[size_t(t.active)]) {
        // KO: sin Fighting() (con vida 1 la pelea no se corta) y sin esperar al rival libre; la
        // carga empieza ya, mientras cae, y el cambio espera a que haya aterrizado (abajo)
        if (!RivalHyper(base, p)) {
          const int k = t.down[size_t(t.pending)] ? Pick(t, false) : t.pending;
          t.pending = -1;
          StartSwap(ctx, base, x, t, k);
        }
      } else if (Free(base, x) && form_quiet && Cooled(t, now) && !PreFight(base)) {   // SELECT: solo el propio libre
        const int k = t.pending;
        t.pending = -1;
        StartSwap(ctx, base, x, t, k);
      }
    }
    // online: el relevo acaba en el mismo tick en los dos PCs. Cada PC avisa en su mando (bit
    // 0x0400 lado 1, 0x0800 lado 2; libres en XInput) de que ya tiene cargado al que entra, y se
    // termina cuando los dos mandos aplicados (iguales en los dos PCs) lo dicen
    StepSlide(base, x, t, now);   // deslizamiento de salida / llegada del relevo
    const uint16_t bit = p ? 0x0800 : 0x0400;
    const bool online = dbz3::net::Session::Get().active();
    // pose de entrada: aterrizado y libre; termina a su tiempo, al caer o si hace otra cosa (un golpe)
    if (t.pose_at && (online || t.phase != Phase::kIdle || t.down[size_t(t.active)] || now >= t.pose_until ||
                      g_pose_ticks[size_t(p)] > kPoseTicks)) {
      PoseEnd(base, p);
      t.pose_at = 0;
    } else if (t.pose_at && now >= t.pose_at && !t.slide) {
      const uint16_t act = REX_LOAD_U16(x + 3244);
      const int16_t slot = int16_t(REX_LOAD_U16(x + kPoseSlotOff));
      if (!g_pose_ticks[size_t(p)]) {
        if (Free(base, x)) {          // empieza (el COM ya esta en el mando 4 desde FinishSwap)
          if (slot == -1) {
            REX_STORE_U16(x + kPoseSlotOff, uint16_t(kPoseSlot));
            g_pose_com[size_t(p)] = x;
          } else if (slot >= 0 && slot < 4 && slot != kPoseSlot) {
            g_pose_pad[size_t(slot)] = true;
            g_pose_slot[size_t(p)] = slot;
          }
          g_pose_ticks[size_t(p)] = 1;
          REXLOG_INFO("dbz3 tag: pose de entrada de {:08X} ({})", x, g_pose_com[size_t(p)] ? "COM" : "mando");
        }
      } else if (act > 2 && act != 20 && act != 21) {   // golpeado o haciendo otra cosa
        PoseEnd(base, p);
        t.pose_at = 0;
      } else {
        ++g_pose_ticks[size_t(p)];
      }
    }
    if (t.phase != Phase::kLoading) g_net_local &= uint16_t(~bit);
    if (t.phase == Phase::kLoading && !t.round_blocked) {
      // PRUEBA (DBZ3_TEST_STALL=1: la primera carga no llega; =2: ninguna): el modelo que entra se pierde
      static const char* stall = std::getenv("DBZ3_TEST_STALL");
      if (!t.round_owned && stall && (stall[0] == '2' || t.retries == 0)) REX_STORE_U32(x + kModel0 + 4 * kLoad, 0);
      bool loaded = !t.round_blocked && now - t.since >= kSwapMin &&
          (!t.round_owned || RoundNodesReady(base, x, t)) && Loaded(base, x, t);
      if (t.round_owned && stall && (stall[0] == '2' || t.retries == 0)) loaded = false; // no perder el puntero
      if (loaded && t.round_owned) {
        std::array<uint16_t, 8> caps{};
        const int id = t.ids[size_t(t.target)];
        auto snapshot = dbz3::roster::ReadForms(id, t.round_suit);
        snapshot.capsules.assign(t.round_caps.begin()+1, t.round_caps.begin()+1+t.round_caps[0]);
        loaded = ReadMemberCapsules(base, p, id, caps) && caps == t.round_caps &&
            snapshot.identity == t.round_identity &&
            dbz3::forms::Availability(snapshot, int(t.round_form)) == dbz3::forms::Reason::kOk;
      }
      if (online) {
        if (loaded) g_net_local |= bit;
        loaded = loaded && (g_net_applied[0].load() & bit) && (g_net_applied[1].load() & bit);
      }
      // apartado ya (deslizamiento de salida terminado) y sin golpe (15), modo hiper (43/44) ni
      // definitiva/Dragon Rush (>= 46) en curso: se le pone de pie con la propia accion del juego y el
      // cambio llega en el tick siguiente con el luchador libre. Cambiar a mitad de una reaccion a
      // un golpe cerro el juego (2026-10-11, lr 82097084: un objeto del que salio sin terminar).
      const uint16_t act = REX_LOAD_U16(x + 3244);
      // KO: el caido no se ve levantarse (parecia inmortal). Tras aterrizar y 90 ticks en el suelo,
      // tumbado (18/19) o levantandose (35/36) se le pone de pie con la accion del juego y en el
      // tick siguiente ya esta el relevo
      if (t.down[size_t(t.active)]) {
        if (!KoLanded(base, x, t, now)) {
          loaded = false;
        } else if (loaded && ActionCtxOk(base, x) && (act == 18 || act == 19 || act == 35 || act == 36) && !dbz3::g_guest_eu) {
          Call(ctx, base, sub_82103908, x, 0);
        }
      }
      if (!t.down[size_t(t.active)] && !form_quiet) loaded = false;   // transformandose: se espera
      if (loaded && !t.round_owned && !Free(base, x) && t.slide == 1 && now - t.slide_at >= SlideTicks(1) && act != 15 && act != 43 &&
          act != 44 && act < 46 && !dbz3::g_guest_eu && ActionCtxOk(base, x)) {
        Call(ctx, base, sub_82103908, x, 0);
      }
      // efectos del que sale aun vivos: se espera (como mucho kFxWaitMax desde que empezo la carga)
      // (tambien los del rival: una tecnica suya pegada al que sale hizo caer el cambio de cuerpo,
      // sub_82115270 -> sub_82155858 -> sub_821527C0 leyendo 0x3, 2026-10-12)
      const uint32_t fx = std::max(g_tech_seen[0].load(), g_tech_seen[1].load());
      if (loaded && now - fx < kFxQuiet && !online &&
          (t.round_owned || now - t.since < kFxWaitMax)) loaded = false;
      if (t.round_owned) loaded = round::CommitReady(loaded, false, false, false, Free(base, x),
          t.down[size_t(t.active)] || form_quiet, now - fx >= kFxQuiet);
      // tras un ring out convertido el caido queda tumbado (18/19) sin contexto de accion (X+6048 nulo): no
      // se le puede levantar (sub_82103908 lo necesita) y el juego vaciaba el hueco del que entra antes de
      // que se levantara solo (medido 2026-10-14). Sin accion en curso no hay nada a medias: se cambia ya.
      const bool ring_lying = t.ring_at && t.down[size_t(t.active)] && !REX_LOAD_U32(x + 6048) &&
                              (act == 18 || act == 19);
      if (loaded && (Free(base, x) || ring_lying)) {
        FinishSwap(ctx, base, x, t);
        g_net_local &= uint16_t(~bit);
      } else if (!loaded && !online &&
                 (now - t.since > kLoadTimeout ||
                  // ring out a mitad de un relevo pedido: el reinicio del caido (RingOutReset) vacia el modelo
                  // que se estaba cargando; se pide otra vez ya (antes: 10 s de espera y el asalto perdido)
                  (t.ring_at && t.retries == 0 && now - t.ring_at > 30 && !REX_LOAD_U32(x + kModel0 + 4 * kLoad)))) {
        const uint32_t cam = t.new_cam, model = REX_LOAD_U32(x + kModel0 + 4 * kLoad);
        REXLOG_WARN("dbz3 tag: luchador {:08X}: la carga no termina (camara {:08X} +192 {:08X}, modelo {:08X} +36 {:08X})",
                    x, cam, cam ? REX_LOAD_U32(cam + 192) : 0, model, model ? REX_LOAD_U32(model + 36) : 0);
        // tras un ring out convertido en relevo: si el que entra no carga, el asalto termina como el ring
        // out que era (aviso del juego mgr+35569 = hueco del ganador + 1); si no, el caido se quedaba fuera
        // de juego para siempre
        if (t.ring_at && !t.ring_giveup && t.retries >= 1) {   // tras un reintento
          t.ring_giveup = true;
          t.ring_at = 0;
          t.ring = false;
          if (const uint32_t mgr = dbz3::g_guest_eu ? 0 : REX_LOAD_U32(0x823D6CF4)) {
            REX_STORE_U8(mgr + 35569, uint8_t((p ^ 1) + 1));
            // sin relevo pendiente ni carga en curso: si no, el tick siguiente lo volvia a pedir
            t.phase = Phase::kIdle;
            t.pending = -1;
            t.target = -1;
            REXLOG_WARN("dbz3 tag: el relevo tras el ring out no carga: el asalto termina por ring out");
            continue;
          }
        }
        // se quedaba asi el resto del combate, con el que sale suspendido en el aire (2026-10-12: el modelo
        // del que entra no llego tras un modo hiper a mitad de carga). Se pide otra vez y, si tampoco
        // llega, se cancela el relevo y el que salio vuelve a su sitio.
        if (t.round_owned) {
          if (!t.round_blocked) REXLOG_WARN("dbz3 tag F3: timeout; recursos retenidos, salir por menu nativo");
          t.round_blocked = true;
          g_net_local &= uint16_t(~bit);
          continue;
        }
        const float home[3] = {t.home[0], t.home[1], t.home[2]};
        const int k = t.target, tries = t.retries;
        t.phase = Phase::kIdle;
        if (tries < 1) StartSwap(ctx, base, x, t, k);
        if (t.phase == Phase::kLoading) {   // pedido otra vez
          std::copy(home, home + 3, t.home);
          if (t.slide == 1) {   // ya estaba fuera: se queda donde esta
            float cur[3];
            for (uint32_t i = 0; i < 3; ++i) cur[i] = LoadF(base, x + 64 + 4 * i);
            StartSlide(t, 1, cur, cur);
          }
          t.retries = tries + 1;
        } else {                            // cancelado (o StartSwap no pudo empezar)
          if (t.slide == 1) {               // el que salio vuelve a su sitio (un caido sigue tumbado)
            float cur[3];
            for (uint32_t i = 0; i < 3; ++i) cur[i] = LoadF(base, x + 64 + 4 * i);
            StartSlide(t, 2, cur, home);
          }
          if (t.spare) {
            TAG_CALL(sub_82100D60, dbz3eu_sub_82100E60, x, kSpare);
            TAG_CALL(sub_82100DE8, dbz3eu_sub_82100EE8, x, kSpare);
            t.spare = 0;
          }
          t.target = -1;
          t.retries = 0;
          t.done_at = now;
          // KO: el relevo se vuelve a pedir (si no, el caido se quedaba tumbado con vida 1 para siempre)
          if (t.down[size_t(t.active)]) {
            t.pending = Pick(t, false);
            t.pending_until = 0;
          }
          REXLOG_WARN("dbz3 tag: luchador {:08X}: relevo cancelado", x);
        }
        t.since = now;
      }
    }
    // el ordenador se releva con poca vida si un compañero esta mejor (como en Tenkaichi 3)
    if (g_com[size_t(p)].load() && t.phase == Phase::kIdle && now % kComCheck == 0) {
      const float mx = std::max(LoadF(base, x + kHpMax), 1.0f);
      const int k = Pick(t, false);
      if (k >= 0 && LoadF(base, x + kHp) / mx < 0.3f && t.hp[size_t(k)] / mx > 0.5f && BothFree(base) && Fighting(base)) {
        StartSwap(ctx, base, x, t, k);
      }
    }
    HudSide& h = g_hud[size_t(p)];
    const float mx = std::max(LoadF(base, x + kHpMax), 1.0f);
    h.n = int(t.ids.size());
    h.active = t.active;
    h.switching = t.phase != Phase::kIdle;
    h.refused = t.refused_at && now - t.refused_at < kRefusedShow;
    for (int k = 0; k < h.n && k < 5; ++k) {
      h.ids[k] = t.ids[size_t(k)];
      h.hp[k] = std::clamp((k == t.active && !h.switching ? LoadF(base, x + kHp) : t.hp[size_t(k)]) / mx, 0.0f, 1.0f);
      h.down[k] = t.down[size_t(k)];
    }
  }
}

bool InBattle() { return g_in_battle.load(); }

void SetComSide(int side, bool com) {
  if (side == 0 || side == 1) g_com[size_t(side)] = com;
}

HudSide Hud(int player) {
  std::lock_guard<std::mutex> lock(g_mu);
  return g_in_battle.load() && player >= 0 && player < 2 ? g_hud[size_t(player)] : HudSide{};
}

// Pantalla de menu del juego (byte 0x824BA110): 5 menu principal, 14 menu de Duelo.
void OnScreen(int screen) {
  if (screen == 5 || screen == 14) {
    g_in_battle = false;
    g_net_local = 0;
    g_net_applied[0] = 0; g_net_applied[1] = 0;
    g_prev_buttons = {}; g_fall_x = 0;
    for (auto& com : g_com) com = false;
    std::lock_guard<std::mutex> lock(g_mu);
    for (auto& t : g_team) { ++t.generation; t.restore = 0; t.pending = -1; }
  }
}

// Mando del usuario (0/1) leido este tick (hook de XamInputGetState): SELECT = relevo, con L
// (LB o gatillo) al tercero. Devuelve true si hay que ocultar SELECT al juego.
// Y+B (XInput 0x8000 | 0x2000) que pulsa la pose de entrada en ese mando (solo sin online)
uint16_t PoseButtons(uint32_t user) { return user < 4 && g_pose_pad[user].load() ? 0xA000 : 0; }

// Mando 4 virtual del COM en pose: true mientras algun lado lo usa; *buttons = Y+B si su pose ya empezo
bool PoseVirtualPad(uint16_t* buttons) {
  std::lock_guard<std::mutex> lock(g_mu);
  for (int p = 0; p < 2; ++p) {
    if (g_pose_com[size_t(p)]) {
      *buttons = g_pose_ticks[size_t(p)] ? 0xA000 : 0;
      return true;
    }
  }
  return false;
}

bool OnPad(PPCContext& ctx, uint8_t* base, uint32_t user, uint16_t buttons, uint8_t lt) {
  if (user > 1) return false;
  constexpr uint16_t kBack = 0x0020, kStart = 0x0010, kLb = 0x0100;
  const bool pressed = (buttons & kBack) && !(g_prev_buttons[user] & kBack) && !(buttons & kStart);
  g_prev_buttons[user] = buttons;
  if (!g_in_battle.load()) return false;      // solo en combate (no un bloque viejo en los menus)
  const uint32_t x = Fighter(base, int(user));
  std::lock_guard<std::mutex> lock(g_mu);
  Team* t = x ? TeamOf(base, x) : nullptr;
  if (!t) return false;
  if (pressed && !Over(base) && !PreFight(base)) {
    const uint32_t now = g_frame.load();
    const int k = Pick(*t, (buttons & kLb) || lt > 64);
    // se apunta aunque el luchador este a mitad de un golpe o en el aire: el relevo llega en cuanto
    // quede libre dentro del margen (antes solo valia pulsar quieto y casi siempre parpadeaba)
    if (k >= 0 && t->phase == Phase::kIdle && Cooled(*t, now)) {
      t->pending = k;
      t->pending_until = now + kSelectBuffer;
    } else {
      t->refused_at = now;                      // HUD: el compañero parpadea
      REXLOG_INFO("dbz3 tag: SELECT rechazado (compañero {}, pelea {}, accion {}, fase {}, enfriando {})", k,
                  Fighting(base), REX_LOAD_U16(x + 3244), int(t->phase), !Cooled(*t, now));
    }
  }
  return (buttons & kBack) && !(buttons & kStart);
}

}  // namespace dbz3::tag

// Reinicio del luchador al empezar un combate/ronda: el lado empieza con su primer miembro.
DBZ3_HOOK(sub_82109638, dbz3eu_sub_82109738) {
  bool new_battle = false;
  const uint32_t x = ctx.r3.u32;
  if (!dbz3::tag::g_in_swap) {
    std::lock_guard<std::mutex> lock(dbz3::tag::g_mu);
    const int p = dbz3::tag::SideOf(base, x);
    if (p >= 0) {
      dbz3::tag::Team& t = dbz3::tag::g_team[size_t(p)];
      const uint32_t generation = t.generation + 1;
      t = dbz3::tag::Team{};
      t.generation = generation;
      t.round_owned = dbz3::tag::RoundEnabled();
      t.ids = dbz3::tag::DynamicTeam(p);
      if (t.round_owned && !dbz3::tag::round::ValidTeam(t.ids)) t.ids.clear();
      // empieza con el primero (tagteam_slots.cpp pone su ID en la seleccion)
      const int now = int(REX_LOAD_U32(x + dbz3::tag::kId));
      for (size_t k = 0; k < t.ids.size(); ++k) {
        if (t.ids[k] == now) {
          t.active = int(k);
          break;
        }
      }
      // ponytail: en un asalto nuevo del mismo combate los efectos guardados se pierden sin soltar (unos
      // nodos del juego); soltarlos aqui si algun dia hay combates de varios asaltos con equipos
      t.tech_owner = t.active;   // la ranura del juego tiene los del que empieza
      for (auto& h : t.hp) h = 3000.0f;
      if (t.ids.size() >= 2) REXLOG_INFO("dbz3 tag: lado {}: equipo de {} (en combate el id {})", p + 1, t.ids.size(), now);
    }
    if (!dbz3::tag::g_in_battle.exchange(true)) {
      REXLOG_INFO("dbz3 tag: empieza un combate");
      new_battle = true;
      dbz3::tag::g_fight_seen = false;
      dbz3::tag::g_net_local = 0;
      dbz3::tag::g_net_applied[0] = 0; dbz3::tag::g_net_applied[1] = 0;
      dbz3::tag::g_prev_buttons = {}; dbz3::tag::g_fall_x = 0;
      for (auto& f : dbz3::tag::g_face) f.clear();   // las hojas del combate anterior ya no valen
    }
  }
  if (new_battle) {   // online: los dos PCs empiezan el combate en el mismo tick y con el mismo RNG
    auto& net = dbz3::net::Session::Get();
    net.BattleBarrier();          // fuera de g_mu: espera al otro PC
    net.ReseedBattle(base);
  }
  orig(ctx, base);
  if (!dbz3::tag::g_in_swap) {
    std::lock_guard<std::mutex> lock(dbz3::tag::g_mu);
    if (dbz3::tag::Team* t = dbz3::tag::TeamOf(base, x)) {
      const float mx = dbz3::tag::LoadF(base, x + dbz3::tag::kHpMax);
      for (auto& h : t->hp) h = mx > 0 ? mx : 3000.0f;
    }
  }
}

// DaÃ±o: si el golpe mataria al activo y queda alguien, "casi muerto" + relevo forzado. Durante
// la carga del que entra tampoco cae nadie.
DBZ3_HOOK(sub_8211A5A8, dbz3eu_sub_8211A6A8) {
  const uint32_t x = ctx.r3.u32;
  const int32_t delta = ctx.r4.s32;
  bool protect = false, ko = false;
  uint8_t old_nd = 0;
  {
    std::lock_guard<std::mutex> lock(dbz3::tag::g_mu);
    dbz3::tag::Team* t = dbz3::tag::TeamOf(base, x);
    if (t && delta > 0 && t->down[size_t(t->active)]) {   // (tambien con el relevo cargando: un segundo KO)
      ctx.r4.s64 = 0;                                    // ya cayo: no mas golpes letales hasta el relevo
    } else if (t && delta > 0) {
      const bool lethal = dbz3::tag::LoadF(base, x + dbz3::tag::kHp) - float(delta) <= 0.0f;
      const bool loading = t->phase != dbz3::tag::Phase::kIdle;
      if ((lethal || loading) && dbz3::tag::Alive(*t, t->active) > 0) {
        old_nd = REX_LOAD_U8(x + dbz3::tag::kNearDeath);
        REX_STORE_U8(x + dbz3::tag::kNearDeath, 1);   // sub_8211A5A8: vida = f30, sin KO
        protect = true;
        // tambien con el relevo cargando: antes el golpe letal se perdia y parecia invencible hasta
        // que entraba el compañero (2026-10-12); ahora cae y el que entra aparece donde cayo
        ko = lethal;
      }
    }
  }
  orig(ctx, base);
  if (!protect) return;
  REX_STORE_U8(x + dbz3::tag::kNearDeath, old_nd);
  if (!ko) return;
  std::lock_guard<std::mutex> lock(dbz3::tag::g_mu);
  dbz3::tag::Team* t = dbz3::tag::TeamOf(base, x);
  if (!t) return;
  t->down[size_t(t->active)] = true;
  if (t->slide == 1) t->slide = 0;   // salia de un relevo: deja de sostenerlo en el aire y cae
  // KO con compañero vivo: en el tick siguiente (tras la reaccion al golpe que pone el juego) sale
  // despedido con la accion 25 del juego (la del golpe fuerte) y el relevo espera a que se levante
  dbz3::tag::g_fall_x = x;
  // con la vida de kNearDeath (f30, casi 0) el juego da el KO al caer al suelo y termina el combate
  // (medido: la pelea, 0x823F39A0, pasaba a 0): 1 punto de vida mientras cae y entra el compañero
  if (dbz3::tag::LoadF(base, x + dbz3::tag::kHp) < 1.0f) dbz3::tag::StoreF(base, x + dbz3::tag::kHp, 1.0f);
  t->ko_at = dbz3::tag::g_frame.load();
  t->seen25 = false;
  // KO: entra el siguiente cuando el caido haya aterrizado (KoLanded)
  t->pending = dbz3::tag::Pick(*t, false);
  t->pending_until = 0;
}

// Fin del combate (US): el juego suelta sus hojas de caras y los retratos del marcador (el retrato
// que pusimos en el relevo incluido); despues soltamos las hojas que cargamos nosotros.
// Sonda (DBZ3_ACT_PROBE=1, solo US): cada accion que el combate pone a un luchador (r3 = X, r4 =
// accion), para encontrar la de la caida del KO y usarla en el relevo.
// Sonda (DBZ3_ACT_PROBE=1, solo US): avisos de fin de combate (sub_82100FA8, r3 = op; el KO por
// golpe llama con op 0 desde sub_8211A5A8): para encontrar el del ring out y el de tiempo agotado.
REX_HOOK_RAW(sub_82100FA8) {
  static const bool probe = std::getenv("DBZ3_ACT_PROBE") != nullptr;
  if (probe) {
    const uint32_t r4 = ctx.r4.u32;
    REXLOG_INFO("dbz3 endprobe: op={} lr={:08X} r4={:08X} [{:08X} {:08X}] pelea={}", ctx.r3.u32, uint32_t(ctx.lr), r4,
                r4 ? REX_LOAD_U32(r4) : 0, r4 ? REX_LOAD_U32(r4 + 8) : 0, REX_LOAD_U32(0x823F39A0));
  }
  __imp__sub_82100FA8(ctx, base);
}

// Ring out del activo con compañero vivo: cuenta como un KO. sub_820FD408 es la entrada del estado 5
// del combate (la llama la tabla de estados desde sub_820FFFC8 cuando el manejador del modo devuelve
// un perdedor, ya puesto en mgr+35188; medido 2026-10-14). No se deja pasar al estado 5: el caido
// vuelve dentro del ring (hacia el centro desde el rival, a su altura) y el compañero entra volando
// como tras un KO, aterrizando ahi. Si el juego insiste 10 s, o era el ultimo, el ring out sigue.
static bool RingOutToSwap(uint8_t* base, uint32_t x) {
  namespace tg = dbz3::tag;
  std::lock_guard<std::mutex> lock(tg::g_mu);
  tg::Team* t = tg::TeamOf(base, x);
  if (!t || t->ring_giveup) return false;
  const uint32_t now = tg::g_frame.load();
  if (now - t->ring_last > 2) t->ring_since = now;   // primera peticion de esta salida
  t->ring_last = now;
  if (now - t->ring_since > 600) return false;        // ponytail: tope de 10 s; despues, ring out normal
  if (!t->down[size_t(t->active)] && tg::Alive(*t, t->active) <= 0) return false;   // el ultimo: ring out de verdad
  // (la secuencia de caida del juego se cierra despues, en RingOutReset, fuera de g_mu)
  if (t->down[size_t(t->active)]) return true;         // ya contado: sigue el relevo
  // sitio dentro del ring: a 60 del centro del escenario (0, 0), del lado contrario al rival, a su altura
  // (con "60 desde el rival hacia el centro" quedaba fuera si el rival estaba en el borde, 2026-10-14)
  const int side = tg::SideOf(base, x);
  const uint32_t o = side < 0 ? 0 : tg::Fighter(base, side ^ 1);
  const float rx = o ? tg::LoadF(base, o + 64) : 0.0f, rz = o ? tg::LoadF(base, o + 72) : 0.0f;
  const float ry = o ? tg::LoadF(base, o + 68) : tg::LoadF(base, x + 68);
  float dx = -rx, dz = -rz;
  const float len = std::sqrt(dx * dx + dz * dz);
  if (len < 1.0f) { dx = 1.0f; dz = 0.0f; } else { dx /= len; dz /= len; }
  const float p[3] = {dx * 60.0f, ry, dz * 60.0f};
  for (const uint32_t off : {48u, 64u, 80u})
    for (uint32_t i = 0; i < 3; ++i) tg::StoreF(base, x + off + 4 * i, p[i]);
  std::copy(p, p + 3, t->ring_pos);
  t->ring = true;
  t->ring_at = now;
  t->down[size_t(t->active)] = true;
  if (t->slide == 1) t->slide = 0;
  t->ko_at = now;
  t->seen25 = false;
  // con un relevo ya cargando (SELECT), ese entra; si no, el siguiente vivo
  t->pending = t->phase == tg::Phase::kIdle ? tg::Pick(*t, false) : -1;
  t->pending_until = 0;
  REXLOG_INFO("dbz3 tag: ring out de {:08X} con compañero vivo: cuenta como KO, entra el miembro {} en {:.1f} {:.1f} {:.1f}",
              x, t->pending, p[0], p[1], p[2]);
  return true;
}

// Cierra la secuencia de "cayo fuera" del juego en el caido, como hace el propio juego al acabarla
// (sub_820FBB38): sub_8211F650 (el aviso de ring out) le pone X+8184 = FF, y con eso sub_8211C098 lo
// sigue dando por fuera cada fotograma, aunque vuelva al centro y tras el reinicio del relevo, y el que
// entraba no llegaba a cargar (medido 2026-10-14). X+8184 = 2 y sub_82116808; su estado de caida
// (X+6300 = -1 reevaluar, X+8164, X+6852) a cero y el aviso mgr+35569 ya consumido, a 0.
static void RingOutReset(PPCContext& ctx, uint8_t* base, uint32_t mgr, uint32_t x) {
  if (REX_LOAD_U8(x + 8184) == 0xFF) {
    REX_STORE_U8(x + 8184, 2);
    dbz3::tag::Call(ctx, base, sub_82116808, x, 0);
  }
  REX_STORE_U32(x + 6300, 0xFFFFFFFFu);
  REX_STORE_U8(x + 8164, 0);
  REX_STORE_U32(x + 6852, 0);
  REX_STORE_U8(mgr + 35569, 0);
}

// + sonda (DBZ3_ACT_PROBE=1, US): quien la llama y que hay ya puesto (perdedor, pelea).
REX_HOOK_RAW(sub_820FD408) {
  const uint32_t mgr = dbz3::g_guest_eu ? 0 : REX_LOAD_U32(0x823D6CF4);
  // mgr+35136 = hueco (0/1; -1 = ninguno) del GANADOR; el perdedor es el otro, mgr+2096+8240*(1-hueco)
  // (medido 2026-10-14 con capturas: cayo fuera el jugador 1 y salio 1 = el rival como WINNER; mgr+35188
  // es el de un asalto anterior). Motivo mgr+35132: 0/2 caido, 4 aviso de ring out, 1 los dos, 3 tiempo.
  const int32_t slot = mgr ? int32_t(REX_LOAD_U32(mgr + 35136)) : -1;
  const uint32_t reason = mgr ? REX_LOAD_U32(mgr + 35132) : 1;
  const uint32_t loser = (slot == 0 || slot == 1) ? mgr + 2096u + 8240u * uint32_t(1 - slot) : 0;
  static const bool probe = std::getenv("DBZ3_ACT_PROBE") != nullptr;
  if (probe && mgr) {
    REXLOG_INFO("dbz3 ringprobe: lr={:08X} estado={} motivo={} hueco={} perdedor={:08X} x={:.1f} z={:.1f}", uint32_t(ctx.lr),
                REX_LOAD_U32(mgr + 35128), REX_LOAD_U32(mgr + 35132), int32_t(REX_LOAD_U32(mgr + 35136)), loser,
                loser ? dbz3::tag::LoadF(base, loser + 64) : 0.0f, loser ? dbz3::tag::LoadF(base, loser + 72) : 0.0f);
    // estado de "sigue en pie" de cada luchador (sub_8211C098: X+6300 retenido, X+6852 que prueba salto,
    // X+8153/8154, contador X+6296, X+8184 puerta, vida X+6080)
    for (const uint32_t f : {mgr + 2096u, mgr + 10336u}) {
      REXLOG_INFO("dbz3 ringprobe:   X={:08X} 6300={} 6852={} 8153={} 8154={} 6296={} 8184={:02X} vida={:.0f}", f,
                  int32_t(REX_LOAD_U32(f + 6300)), REX_LOAD_U32(f + 6852), REX_LOAD_U8(f + 8153), REX_LOAD_U8(f + 8154),
                  int32_t(REX_LOAD_U32(f + 6296)), REX_LOAD_U8(f + 8184), dbz3::tag::LoadF(base, f + 6080));
    }
  }
  if (mgr && REX_LOAD_U32(mgr + 35128) == 4 && loser && (reason == 0 || reason == 2 || reason == 4) &&
      RingOutToSwap(base, loser)) {
    RingOutReset(ctx, base, mgr, loser);
    return;
  }
  __imp__sub_820FD408(ctx, base);
}

REX_HOOK_RAW(sub_82103908) {
  static const bool probe = std::getenv("DBZ3_ACT_PROBE") != nullptr;
  if (probe) {
    const uint32_t x = ctx.r3.u32;
    REXLOG_INFO("dbz3 actprobe: X={:08X} accion={} antes={} vida={:.0f}", x, ctx.r4.u32,
                REX_LOAD_U16(x + 3244), dbz3::tag::LoadF(base, x + dbz3::tag::kHp));
  }
  __imp__sub_82103908(ctx, base);
}

// Fin del combate (sub_82126A10 suelta las ranuras de tecnica de los dos jugadores): despues se
// sueltan igual los efectos guardados de los miembros que no estaban en combate (Team::tech).
REX_HOOK_RAW(sub_82126A10) {
  __imp__sub_82126A10(ctx, base);
  if (dbz3::g_guest_eu) return;
  std::lock_guard<std::mutex> lock(dbz3::tag::g_mu);
  int freed = 0;
  for (int p = 0; p < 2; ++p) {
    auto& t = dbz3::tag::g_team[size_t(p)];
    const uint32_t g2 = REX_LOAD_U32(dbz3::tag::kTechBlock);
    if (!g2) break;
    const uint32_t slot = g2 + 1152 * uint32_t(p) + 80;
    for (size_t k = 0; k < t.tech_ok.size(); ++k) {
      if (!t.tech_ok[k]) continue;
      std::memcpy(base + slot, t.tech[k].data(), 544);
      dbz3::tag::Call(ctx, base, sub_821266A8, uint32_t(p), 0);
      std::memset(base + slot, 0, 544);
      t.tech_ok[k] = false;
      ++freed;
    }
    t.tech_owner = -1;
  }
  if (freed) REXLOG_INFO("dbz3 tag: {} efectos de tecnica guardados sueltos al acabar el combate", freed);
}

REX_HOOK_RAW(sub_820E7058) {
  __imp__sub_820E7058(ctx, base);
  std::lock_guard<std::mutex> lock(dbz3::tag::g_mu);
  for (const uint32_t res : dbz3::tag::g_face_res) dbz3::tag::Call(ctx, base, sub_82083A68, res, 0);
  if (!dbz3::tag::g_face_res.empty()) REXLOG_INFO("dbz3 tag: {} hojas de caras del relevo sueltas", dbz3::tag::g_face_res.size());
  dbz3::tag::g_face_res.clear();
  for (auto& f : dbz3::tag::g_face) f.clear();
}

// Banco de efectos (sub_82092C58(objeto, tipo) pone el banco global 0x8238CE10, y su tabla en +20,
// justo antes de usar sus efectos): si es de la ranura de tecnica de un lado con equipo, un efecto
// suyo sigue vivo y su relevo espera (g_tech_seen).
REX_HOOK_RAW(sub_82092C58) {
  // banco de la ranura de tecnica de un lado con equipo (su arbol, su objeto o un hijo): efecto vivo
  if (!dbz3::g_guest_eu && dbz3::tag::g_in_battle.load()) {
    const uint32_t g2 = REX_LOAD_U32(dbz3::tag::kTechBlock), bank = ctx.r3.u32;
    for (int p = 0; g2 && p < 2; ++p) {
      if (dbz3::tag::g_team[size_t(p)].ids.size() < 2) continue;
      const uint32_t s = g2 + 1152 * uint32_t(p) + 80;
      bool mine = bank == REX_LOAD_U32(s) || bank == REX_LOAD_U32(s + 4);
      for (uint32_t i = 0; i < 64 && !mine; ++i) mine = bank == REX_LOAD_U32(s + 32 + 4 * i);
      if (mine) {
        static std::array<uint32_t, 2> last_log{};
        const uint32_t now = dbz3::tag::g_frame.load();
        if (now - last_log[size_t(p)] > 120) {
          last_log[size_t(p)] = now;
          REXLOG_INFO("dbz3 tag: efecto de tecnica del lado {} en uso (banco {:08X}, desde {:08X})", p + 1, bank, uint32_t(ctx.lr));
        }
        dbz3::tag::g_tech_seen[size_t(p)] = now;
      }
    }
  }
  __imp__sub_82092C58(ctx, base);
}
