// dbz3 - Tag Team en combate (ver tagteam.cpp).
#pragma once

#include <cstdint>
#include <vector>

namespace dbz3::tag {

bool InBattle();

// Lo que dibuja el marcador de los compañeros (tagteam_panel.cpp), copiado en cada tick.
struct HudSide {
  int n = 0;              // miembros (0 = este jugador no lleva un equipo)
  int active = 0;         // miembro en combate
  int ids[5] = {};        // ID de personaje de cada miembro
  float hp[5] = {};       // vida 0..1
  bool down[5] = {};      // caido
  bool switching = false;
  bool refused = false;   // SELECT rechazado hace poco (el HUD parpadea)
};
HudSide Hud(int player);

// Tag Team al vuelo (tagteam_slots.cpp): miembros elegidos en la rueda por lado (0 = 1P).
// Con 2 o mas, el combate siguiente usa la ranura de ese lado con esos miembros.
// remember: el equipo del lado 1 pasa a ser "mi ultimo equipo" (el que se lleva al online); el online no
void SetDynamicTeam(int side, const std::vector<int>& members, bool remember = true);
void ClearDynamicTeams();
std::vector<int> DynamicTeam(int side);
// Capsulas "Custom" de un miembro, elegidas en el menu del juego al confirmarlo (n + 7 IDs, como
// char96 +80); nullptr = las suyas por defecto ("Normal").
void SetMemberCapsules(int side, int id, const uint16_t* caps);
// Capsulas del miembro `id` en el registro de seleccion de 80 B (+52 n, +54 7 IDs): las elegidas o las
// suyas por defecto. Online siempre las de por defecto (los dos PCs tienen que ver lo mismo).
void PutCapsules(uint8_t* base, uint32_t rec, int side, int id);
// Ultimo equipo elegido por 1P en TAG TEAM: online, cada PC manda el suyo (netplay kTeam).
std::vector<int> MyLastTeam();
// Lado que lleva el ordenador (relevo propio cuando le queda poca vida).
void SetComSide(int side, bool com);
// Inicio del combate (roster_ext.cpp, hook de sub_820FE668): rellena las ranuras y cambia el ID.
void BeforeBattleSetup(uint8_t* base, uint32_t sel);
// Online: relevos ya cargados en este PC (bits 0x0400/0x0800 que viajan en el mando) y los de los
// mandos aplicados; el relevo acaba cuando los dos PCs lo tienen (mismo tick en los dos).
uint16_t NetReadyBits();
void NetApplied(uint32_t user, uint16_t buttons);

}  // namespace dbz3::tag
