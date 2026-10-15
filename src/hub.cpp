// dbz3 - Conector nativo de combate para el HUB (modos propios lanzados con las piezas del juego).
//
// El juego arranca un combate desde la escena 24/25 con la configuracion global CFG (0x8245EFC0, 384 B; la
// copia sub_820FE668 a mgr+1680). Camino (docs/12_conectores/10_arranque_combate_nativo.md, s9), sin pilotar
// menus ni pulsar botones:
//   menu principal (espera 0x821D7C30) -> sub_82179408 (CFG por defecto) + nuestros datos -> se destruye el
//   menu (sub_8218BA00) -> la tarea de escenas pasa a la espera de la 24 (0x821D7400) -> se pide la 25
//   (sub_821D2778(25,0) + sub_821D27C8(10)) -> combate -> sub_821D3628 con resultado 2 -> menu principal.
// Solo US (las direcciones EU no estan medidas).
#include <rex/hook.h>
#include <rex/logging.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>

#include "generated/dbz3_init.h"
#include "guest_region.h"

namespace dbz3::hub {
namespace {
constexpr uint32_t kCfg = 0x8245EFC0, kReqCur = 0x8247A990, kSceneTask = 0x8247A9A0, kGame = 0x824BA110;

struct Request {
  int id[2] = {0, 7};
  int suit[2] = {0, 0};
  int pad[2] = {0, -1};   // 0 mando 1, 1 mando 2, -1 COM
  int stage = 0;
  int time_s = 0;         // 0 el de las opciones, > 0 segundos, < 0 sin tiempo (CFG+348 en s; doc 10 s2.2)
  int hp[2] = {0, 0};     // 0 la normal; > 0 vida inicial y maxima (rec+36/+40, 1000 por barra); < 0 solo la
                          // inicial (-hp), la maxima la de serie (vida que no se recupera: la barra no se acorta)
  int form[2] = {0, 0};   // forma inicial (rec+20; 0 la base)
};
Request g_req;
// 0 nada; 1 pedido (lo recoge la espera del menu principal); 3 combate nuestro en curso
std::atomic<int> g_phase{0};
}  // namespace

// Pide un combate desde el menu principal (cualquier hilo; lo lanza el hilo del juego en su siguiente tick).
// Con tiempo, vida y forma inicial por combate (Leyenda de Mr. Satan, retos): 0 = lo normal en cada uno.
bool RequestBattleEx(int id1, int id2, int stage, bool p2_com, int time_s, int hp1, int hp2, int form1, int form2) {
  if (dbz3::g_guest_eu || g_phase.load() != 0) return false;
  g_req = Request{};
  g_req.id[0] = id1;
  g_req.id[1] = id2;
  g_req.pad[1] = p2_com ? -1 : 1;
  g_req.stage = stage;
  g_req.time_s = time_s;
  g_req.hp[0] = hp1;
  g_req.hp[1] = hp2;
  g_req.form[0] = form1;
  g_req.form[1] = form2;
  g_phase = 1;
  return true;
}
bool RequestBattle(int id1, int id2, int stage, bool p2_com) {
  return RequestBattleEx(id1, id2, stage, p2_com, 0, 0, 0, 0, 0);
}
bool OwnBattle() { return g_phase.load() == 3; }
}  // namespace dbz3::hub

// Espera del menu principal (G+0 = 5): aqui se lanza el combate pedido.
REX_HOOK_RAW(sub_821D7C30) {
  using namespace dbz3::hub;
  // PRUEBA (DBZ3_TEST_BATTLE="id1,id2,escenario"): a los 3 s en el menu principal, un combate 1P VS COM
  if (static const char* tb = std::getenv("DBZ3_TEST_BATTLE"); tb && !dbz3::g_guest_eu) {
    static uint32_t ticks = 0;
    static int done = 0;
    ticks = REX_LOAD_U8(kGame) == 5 ? ticks + 1 : 0;
    if (done < 2 && ticks == 180 && g_phase.load() == 0) {   // dos seguidos: ida y vuelta al menu
      int a = 0, b = 7, st = 0, t = 0, h1 = 0, h2 = 0, f1 = 0, f2 = 0;   // ",tiempo,vida1,vida2,forma1,forma2" opcional
      std::sscanf(tb, "%d,%d,%d,%d,%d,%d,%d,%d", &a, &b, &st, &t, &h1, &h2, &f1, &f2);
      if (RequestBattleEx(a, b, st, true, t, h1, h2, f1, f2)) ++done;
    }
  }
  if (!dbz3::g_guest_eu && g_phase.load() == 1 && int32_t(REX_LOAD_U32(kReqCur)) == -1) {
    const PPCContext saved = ctx;
    sub_82179408(ctx, base);   // CFG por defecto (modo 5, tiempo de las opciones)
    for (uint32_t k = 0; k < 2; ++k) {
      const uint32_t rec = kCfg + 16 + 80 * k;
      REX_STORE_U16(rec + 16, uint16_t(g_req.id[k]));
      REX_STORE_U16(rec + 18, uint16_t(g_req.suit[k]));
      REX_STORE_U16(rec + 26, uint16_t(int16_t(g_req.pad[k])));
      if (g_req.form[k] > 0) REX_STORE_U16(rec + 20, uint16_t(g_req.form[k]));
      if (g_req.hp[k] != 0) REX_STORE_U32(rec + 36, uint32_t(g_req.hp[k] > 0 ? g_req.hp[k] : -g_req.hp[k]));
      if (g_req.hp[k] > 0) REX_STORE_U32(rec + 40, uint32_t(g_req.hp[k]));
    }
    REX_STORE_U16(kCfg + 340, uint16_t(g_req.stage));
    // Modo 1 (no el 5 que deja sub_82179408): el combate empieza con la presentacion nativa del Duelo, vuelo por el
    // escenario y entrada de los luchadores (START la salta). Medido 2026-10-15: estados 1 (vuelo, 14 s), 2 (entradas,
    // 6 s), 3 READY, 4 pelea; la vuelta por "Return to Main Menu" da resultado 2. Con el 5 iba directo al READY.
    // (CFG+352 != -1 en el modo 5 no sirve: es un indice y cascaba.) DBZ3_HUB_MODE=5 para probar sin presentacion.
    const char* mode = std::getenv("DBZ3_HUB_MODE");
    REX_STORE_U16(kCfg + 338, uint16_t(mode ? std::atoi(mode) : 1));
    if (g_req.time_s) REX_STORE_U32(kCfg + 348, g_req.time_s > 0 ? uint32_t(g_req.time_s) : 0xFFFFFFFFu);
    REX_STORE_U8(kGame + 1, 6);   // G+1 = 6: la vuelta de la escena 25
    ctx = saved;
    sub_8218BA00(ctx, base);      // fuera el menu principal (lo que hace su confirmacion)
    REX_STORE_U32(REX_LOAD_U32(kSceneTask) + 28, 0x821D7400);   // la tarea de escenas: espera de la 24
    ctx = saved;
    ctx.r3.u64 = 25;
    ctx.r4.u64 = 0;
    sub_821D2778(ctx, base);
    ctx = saved;
    ctx.r3.u64 = 10;
    sub_821D27C8(ctx, base);      // "salida terminada", como sub_82177300 (Duelo)
    ctx = saved;
    g_phase = 3;
    REXLOG_INFO("dbz3 hub: combate nativo: id {} contra id {} (escenario {}, mandos {} / {}, tiempo {}, vida {} / {}, "
                "forma {} / {})", g_req.id[0], g_req.id[1], g_req.stage, g_req.pad[0], g_req.pad[1], g_req.time_s,
                g_req.hp[0], g_req.hp[1], g_req.form[0], g_req.form[1]);
    return;
  }
  __imp__sub_821D7C30(ctx, base);
}

// sub_8216BF78 destruye la interfaz de la escena 24 (byte 0x82456430): en nuestro camino nunca se creo.
REX_HOOK_RAW(sub_8216BF78) {
  if (dbz3::hub::g_phase.load() == 3 && REX_LOAD_U8(0x824BA110) == 25) return;
  __imp__sub_8216BF78(ctx, base);
}

// Fin del combate: con uno nuestro, resultado 2 = volver al menu principal (sin "otra vez" ni menus del Duelo).
REX_HOOK_RAW(sub_821D3628) {
  if (dbz3::hub::g_phase.load() == 3) {
    // "Otra vez" no pasa por aqui (revancha dentro de la escena); "Elegir personajes" y "Menu principal"
    // dan 4 (medido 2026-10-15), que con G+1 = 6 lleva a la escena 24 (la seleccion del Duelo, que en nuestro
    // camino no existe: se quedaba ahi). Todo resultado que no sea 2 se lleva a 2 = menu principal (el HUB
    // decide despues a donde volver).
    REXLOG_INFO("dbz3 hub: fin del combate nativo (resultado del juego {}): al menu principal", ctx.r3.s32);
    ctx.r3.u64 = 2;
    dbz3::hub::g_phase = 0;
  }
  __imp__sub_821D3628(ctx, base);
}
