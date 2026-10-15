// dbz3 - Frente Ports: guarda de la ranura de tecnicas (BSP) con mas de 64 recursos (2026-10-15).
//
// sub_82126490 (procesar la ranura de tecnicas j del lado k; ranura = *0x82333994 + 1152k + 544j + 80) apunta
// cada recurso del BSP (hijos de [0], hijos de [1] y hijos de la raiz desde el 2, en ese orden) en
// ranura+32+4i y, si el recurso es un #AWO, crea su instancia en ranura+288+4i. Hay sitio para 64 (los BSP del
// juego llegan a 52). El de Vegeta GT de Infinite World tiene 65: el recurso 64 (#CWV) cae en ranura+288, la
// instancia 0. Durante el combate no pasa nada (sub_820CF580 busca en ranura+32+4*indice y encuentra el #CWV),
// pero al acabar sub_821266A8 libera las 64 "instancias" con sub_82089C10 y libera ese bloque del fichero:
// lectura de 0x10 y el juego se cierra (D:\DBZ3HD\_pruebas\20261015-015446_modos_test_vegeta_gt_sin_modos,
// lr 0x82089C34, r31 = el #CWV en +0xA1700 del BSP).
// Arreglo: antes de liberar, las entradas desbordadas (punteros a datos del BSP, no instancias) se ponen a 0.
// Solo US (la base de las ranuras en la EU no esta medida). roster_build.py avisa de los BSP de mas de 64.
#include <rex/hook.h>
#include <rex/logging.h>

#include <cstdint>

#include "generated/dbz3_init.h"
#include "guest_region.h"

namespace {
constexpr uint32_t kSlotsBase = 0x82333994, kCap = 64;
}  // namespace

REX_HOOK_RAW(sub_821266A8) {
  if (!dbz3::g_guest_eu) {
    const uint32_t base0 = REX_LOAD_U32(kSlotsBase);
    const uint32_t k = ctx.r3.u32, j = ctx.r4.u32;
    if (base0 && k < 2 && j < 2) {
      const uint32_t slot = base0 + 1152 * k + 544 * j + 80;
      const uint32_t bin = REX_LOAD_U32(slot + 4);
      if (bin) {
        const uint32_t n01 = REX_LOAD_U32(slot + 16);   // recursos de [0] y [1]
        const uint32_t ntop = REX_LOAD_U32(bin + 16);  // hijos de la raiz
        const uint32_t total = n01 + (ntop > 2 ? ntop - 2 : 0);
        if (total > kCap && total < 2 * kCap) {
          for (uint32_t g = kCap; g < total; ++g) REX_STORE_U32(slot + 32 + 4 * g, 0);
          REXLOG_WARN("dbz3 ports: tecnicas del lado {} con {} recursos (el juego admite {}): {} sin liberar "
                      "(eran datos del fichero, no instancias)", k, total, kCap, total - kCap);
        }
      }
    }
  }
  __imp__sub_821266A8(ctx, base);
}
