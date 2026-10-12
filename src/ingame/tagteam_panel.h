// dbz3 - Pantalla TAG TEAM dentro del juego (ver tagteam_panel.cpp).
#pragma once

#include <memory>
#include <string>
#include <vector>

#include <imgui.h>

#include <rex/ui/imgui_dialog.h>

namespace dbz3::ingame {

class TagTeamPanel : public rex::ui::ImGuiDialog {
 public:
  explicit TagTeamPanel(rex::ui::ImGuiDrawer* drawer);
  ~TagTeamPanel() override;

 protected:
  void OnDraw(ImGuiIO& io) override;

 private:
  uint16_t Pad();
  void DrawMenu(ImGuiIO& io, uint16_t pressed);
  void DrawPicking(ImGuiIO& io);
  bool blocked_ = false;
  bool shown_ = false;
  bool wait_release_ = false;   // la A que abre la pantalla no cuenta: se espera a soltarlo todo
  uint16_t last_buttons_ = 0;
  int row_ = 0;
};

void TagTeamOpen();
bool TagTeamWanted();
// Modo Tag Team activo (de la pantalla TAG TEAM al menu principal).
bool TagTeamMode();
// Mando `user` (0/1) en la rueda con el modo Tag Team: Y quita el ultimo miembro.
uint16_t TeamPickFilter(uint16_t real, int user);
// Personaje confirmado en la rueda (select_ext.cpp): true si era un miembro del equipo.
bool OnSelectPicked(int player, int id);
// Menu de capsulas del juego tras confirmar (select_ext.cpp): elegido o atras, opcion (0 Normal,
// 1 Custom) y la lista Custom de la casilla (7 IDs, 0xFFFF = hueco).
void OnCapsuleChoice(int player, bool chosen, int option, const uint16_t* custom);
// Forma inicial en la rueda (fuera del Tag Team y del online): LT/RT del mando `user` baja/sube la
// forma con la que empieza el lado que esta eligiendo. StartForm(side) = 0 normal.
void FormPickPad(int user, uint8_t lt, uint8_t rt);
int StartForm(int side);
// La elegida ahora (sin gastarla) y si cambio desde la ultima vez (la vista previa se rehace).
int ChosenForm(int side);
bool TakeFormDirty(int side);

}  // namespace dbz3::ingame
