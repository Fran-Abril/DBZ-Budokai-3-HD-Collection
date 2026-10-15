// dbz3 - Frente EU (2026-10-15): la pareja EU de cada gancho, llamada y direccion US del juego.
//
// El exe dual lleva el codigo US (generated/, sub_X) y el EU (generated_eu/, dbz3eu_sub_Y). REX_HOOK_RAW(sub_X)
// solo cambia la funcion US: con el xex europeo nunca se ejecuta, y una direccion US lee otra cosa. Cambios en
// cada fichero (uno por linea, el cuerpo no cambia):
//   gancho:     REX_HOOK_RAW(sub_X) { ... __imp__sub_X(ctx, base); }  ->  DBZ3_HOOK_EU(sub_X) { ... orig(ctx, base); }
//   llamada:    sub_X(ctx, base)                                     ->  DBZ3_CALL_EU(sub_X)(ctx, base)
//   original:   __imp__sub_X(ctx, base) fuera de su gancho           ->  DBZ3_IMP_EU(sub_X)(ctx, base)
//   direccion:  constexpr uint32_t kX = 0x82...;                       ->  constexpr dbz3::GA kX = 0x82...;
//               REX_LOAD_U32(0x82...)                                 ->  REX_LOAD_U32(dbz3::GA(0x82...))
//               (campo de una estructura: dbz3::GA(base) + desplazamiento; los desplazamientos no cambian)
// La pareja sale de src/eu_map_gen.h (python tools/eu_map.py; evidencia de cada una en docs/14_frentes/eu_mapa.txt).
// Una funcion o direccion sin pareja verificada NO compila: se regenera el mapa (que la mide) o ese trozo se queda
// solo US con dbz3::g_guest_eu. En la build solo US todo vuelve a ser lo de antes (mismo codigo, la direccion US).
#pragma once

#include <cstdint>
#include <type_traits>

#include <rex/hook.h>

#include "eu_map_gen.h"
#include "guest_region.h"

namespace dbz3 {
namespace eu_map {
void SinParejaEU_ejecuta_tools_eu_map_py();   // sin definir ni constexpr: usarla en Of() rompe la compilacion

consteval uint32_t Of(uint32_t us) {
  for (const Par& p : kPares)
    if (p.us == us) return p.eu;
  SinParejaEU_ejecuta_tools_eu_map_py();
  return 0;
}
}  // namespace eu_map

// Direccion del juego valida en las dos imagenes: la pareja se busca al compilar; al usarla elige segun el xex.
// Un campo de una estructura (kCfg + 16) sigue siendo GA: los desplazamientos son iguales en US y EU.
struct GA {
  uint32_t us, eu;
  consteval GA(uint32_t a) : us(a), eu(eu_map::Of(a)) {}
  operator uint32_t() const { return g_guest_eu ? eu : us; }
  template <class T>
    requires std::is_integral_v<T>
  friend constexpr GA operator+(GA a, T d) {
    return GA(a.us + uint32_t(d), a.eu + uint32_t(d), 0);
  }
  template <class T>
    requires std::is_integral_v<T>
  friend constexpr GA operator-(GA a, T d) {
    return GA(a.us - uint32_t(d), a.eu - uint32_t(d), 0);
  }

 private:
  constexpr GA(uint32_t u, uint32_t e, int) : us(u), eu(e) {}
};
}  // namespace dbz3

#define DBZ3_HOOK_EU(us) DBZ3_HOOK_EU_(us, DBZ3EU_##us)
#define DBZ3_HOOK_EU_(us, eu) DBZ3_HOOK(us, eu)
#define DBZ3_CALL_EU(us) DBZ3_CALL_EU_(us, DBZ3EU_##us)
#define DBZ3_CALL_EU_(us, eu) DBZ3_CALL2(us, eu)
#define DBZ3_IMP_EU(us) DBZ3_IMP_EU_(us, DBZ3EU_##us)
#define DBZ3_IMP_EU_(us, eu) DBZ3_IMP_EU__(us, eu)   // un nivel mas: DBZ3EU_sub_X se expande antes de pegar __imp__
#define DBZ3_IMP_EU__(us, eu) DBZ3_CALL2(__imp__##us, __imp__##eu)
