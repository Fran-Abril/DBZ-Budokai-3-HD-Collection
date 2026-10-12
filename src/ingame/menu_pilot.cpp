// dbz3 - Ver menu_pilot.h.
#include "menu_pilot.h"

#include <mutex>

#include <rex/logging.h>

#include "../guest_region.h"

bool dbz3_select_live();   // select_ext.cpp
int64_t dbz3_select_browse_ms(int k);
int64_t dbz3_select_now_ms();

namespace dbz3::ingame::pilot {
namespace {

// ticks (~60 por segundo): se mantiene como una pulsacion de teclado de prueba (150 ms)
constexpr int kHold = 9, kGap = 24, kTimeout = 900;

std::mutex g_mu;
std::vector<Step> g_steps;
size_t g_at = 0;
int g_phase = 0;          // ticks dentro del paso actual
int64_t g_step_ms = 0;    // cuando empezo el paso (reloj de select_ext)
uint16_t g_out = 0;       // botones que ve el juego mientras pilota
bool g_armed = false;     // los botones reales ya se soltaron (no cancelar por el A que lo lanzo)

void Next() {
  ++g_at;
  g_phase = 0;
  g_out = 0;
  g_step_ms = dbz3_select_now_ms();
}

void Stop(const char* why) {
  if (g_at < g_steps.size()) REXLOG_INFO("dbz3 piloto: parado en el paso {} ({})", g_at, why);
  g_steps.clear();
  g_at = 0;
  g_out = 0;
}

}  // namespace

Step Press(uint16_t buttons, int user) { return Step{Step::kPress, buttons, 0, user}; }
Step WaitScreen(int screen) { return Step{Step::kWaitScreen, 0, screen}; }
Step PressToLeave(uint16_t buttons, int screen) { return Step{Step::kPressToLeave, buttons, screen}; }
Step PressUntilBrowse(uint16_t buttons, int player, int user) {
  return Step{Step::kPressUntilBrowse, buttons, player, user};
}

Step Poke(uint32_t addr, uint8_t value) { return Step{Step::kPoke, value, int(addr)}; }

void Run(std::vector<Step> steps) {
  std::lock_guard<std::mutex> lock(g_mu);
  g_steps = std::move(steps);
  g_at = 0;
  g_phase = 0;
  g_out = 0;
  g_armed = false;
  g_step_ms = dbz3_select_now_ms();
}

void Cancel() {
  std::lock_guard<std::mutex> lock(g_mu);
  Stop("cancelado");
}

bool Busy() {
  std::lock_guard<std::mutex> lock(g_mu);
  return g_at < g_steps.size();
}

void Tick(uint8_t* base) {
  std::lock_guard<std::mutex> lock(g_mu);
  if (g_at >= g_steps.size()) return;
  const Step& s = g_steps[g_at];
  ++g_phase;
  switch (s.kind) {
    case Step::kPress:
      g_out = g_phase <= kHold ? s.buttons : 0;
      if (g_phase >= kHold + kGap) Next();
      break;
    case Step::kWaitScreen:
      g_out = 0;
      if (base[dbz3::GuestAddr(0x824BA110, 0x824BA100)] == s.value) {
        if (g_phase > 100) Next();  // tras su animacion de entrada (con 40 el Duelo perdia la A)
      } else if (g_phase > kTimeout) {
        Stop("la pantalla no llego");
      }
      break;
    case Step::kPressToLeave:
      // la rueda de personajes comparte pantalla con el Duelo (14): tambien cuenta como salir
      if (base[dbz3::GuestAddr(0x824BA110, 0x824BA100)] != s.value || dbz3_select_live()) {
        Next();
      } else {
        g_out = (g_phase % (kHold + kGap)) < kHold ? s.buttons : 0;
        if (g_phase > kTimeout) Stop("la pantalla no cambio");
      }
      break;
    case Step::kPoke:
      base[uint32_t(s.value)] = uint8_t(s.buttons);
      Next();
      break;
    case Step::kPressUntilBrowse:
      if (dbz3_select_browse_ms(s.value) > g_step_ms) {
        Next();
      } else {
        g_out = (g_phase % (kHold + kGap)) < kHold ? s.buttons : 0;
        if (g_phase > kTimeout) Stop("la rueda no volvio");
      }
      break;
  }
  if (g_at >= g_steps.size()) Stop("hecho");
}

int User() {
  std::lock_guard<std::mutex> lock(g_mu);
  return g_at < g_steps.size() ? g_steps[g_at].user : -1;
}

uint16_t Filter(uint16_t real, int user) {
  std::lock_guard<std::mutex> lock(g_mu);
  if (g_at >= g_steps.size()) return real;
  if (g_steps[g_at].user != user) return user == 0 ? 0 : real;   // el jugador 1 no toca mientras pilota
  if (!real) g_armed = true;
  // solo B o START cancelan (atras): A o la cruz pulsados con prisa durante los fundidos no
  // deben cortar el camino a medias (pasaba al abrir TAG TEAM la primera vez)
  if (g_armed && (real & 0x2010)) {   // el jugador toma el mando
    Stop("el jugador pulso atras");
    return real;
  }
  return g_out;
}

}  // namespace dbz3::ingame::pilot
