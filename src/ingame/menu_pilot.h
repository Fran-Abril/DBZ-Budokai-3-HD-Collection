// dbz3 - Piloto de menus: pulsa botones del mando 1 por el jugador para llevar al juego por
// sus propios menus (p. ej. de TAG TEAM a Duelo > 1P VS COM y su seleccion de personajes),
// sin tocar su logica. Las esperas de pantalla no dependen de los tiempos de carga. Fuera del
// online; se cancela si el jugador pulsa cualquier boton.
#pragma once

#include <cstdint>
#include <vector>

namespace dbz3::ingame::pilot {

struct Step {
  enum Kind { kPress, kWaitScreen, kPressToLeave, kPressUntilBrowse } kind;
  uint16_t buttons = 0;   // kPress / kPressToLeave: botones (XInput)
  int value = 0;          // kWaitScreen: pantalla esperada; kPressToLeave: pantalla de la que salir
  int user = 0;           // mando que pulsa (0 = jugador 1, 1 = jugador 2)
};

Step Press(uint16_t buttons, int user = 0);
Step WaitScreen(int screen);
// Pulsa hasta que el juego deja esa pantalla (las entradas de menu ignoran botones un rato).
Step PressToLeave(uint16_t buttons, int screen);
// Pulsa hasta que el jugador `player` de la rueda vuelve a estar eligiendo (tras confirmar, el
// juego abre su menu de capsulas con una animacion que se come las pulsaciones).
Step PressUntilBrowse(uint16_t buttons, int player, int user);

void Run(std::vector<Step> steps);
void Cancel();
bool Busy();

// Un tick del juego (bucle principal): avanza los pasos.
void Tick(uint8_t* base);
// Mando `user` leido por el juego: devuelve los botones a usar (los del piloto o los reales).
uint16_t Filter(uint16_t real, int user = 0);
// Mando al que pulsa el paso actual (-1 si no pilota).
int User();

}  // namespace dbz3::ingame::pilot
