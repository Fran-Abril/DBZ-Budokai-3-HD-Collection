// dbz3 - Extension de ESCENARIOS (Budokai 2 -> Budokai 3 HD) declarada por mods.
//
// Un mod puede llevar mods/<mod>/stage.toml. Al arrancar (imagen del guest ya
// cargada, hilo del guest aun sin ejecutar) se aplican sus ordenes sobre la TABLA
// DE ESCENARIOS del juego.
//
// Mapa del juego (US; el .data va en la misma direccion en EU, se verifica por
// firma):
//   0x82323760  registros de escenario, 17 x 92 B:
//                 +0x00 nombre (16 B) | +0x10 id_mapa (u32, entrada data_cmn.afs)
//                 +0x14 id_SE (u32)    | +0x18 variante (0 normal / 1 "destruido")
//   0x82323D80  tabla paralela de punteros a funcion (tema), 17 + terminador 0.
//   0x82373384  12 pares u16 (casilla del menu 0..11 -> registro del escenario).
//   El indice de escenario del combate esta en obj+340 (s16); sub_820C26E0 lee
//   *((u32*)(0x82323760 + idx*92 + 0x10)) y carga esa entrada AFS (sub_82083968).
//
// stage.toml:
//   [parche]                 # banco de pruebas: redirige un registro EXISTENTE a
//   registro = 2             # otra entrada de data_cmn (pisa ese escenario).
//   entrada = 3990           # id_mapa nuevo (entrada data_cmn.afs)
//   id_se   = 57             # opcional: id de la entrada que trae el SE
//   nombre  = "MI ESCENARIO" # opcional: rotula el registro (<=15 chars)
//   tema    = 16             # opcional: usa la funcion de tema del registro N
//                            # (evita el cuelgue si la geometria no encaja)
//
#pragma once

namespace rex::memory {
class Memory;
}

namespace dbz3::stages {

// Lee cada mod activo con stage.toml y parchea la tabla de escenarios. Seguro de
// llamar una vez por arranque; no hace nada (y lo registra) en imagen desconocida.
void ApplyAtLaunch(rex::memory::Memory* memory);

}  // namespace dbz3::stages
