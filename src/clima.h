// dbz3 - CLIMA (ficha 10): el cielo del escenario se oscurece durante el combate. Ver clima.cpp. Solo US.
#pragma once

#include <cstdint>

namespace dbz3::clima {

// Encendido: opcion dbz3_clima, regla de reto (Activar) o DBZ3_CLIMA en pruebas. Apagado por defecto.
bool Activo();
void Activar(bool on);   // regla de reto: true durante el reto, false al salir
// Gancho de escenarios.cpp: se pide el escenario de un combate (sub_820C26E0): todo vuelve a empezar.
void EscenarioPedido();

}  // namespace dbz3::clima
