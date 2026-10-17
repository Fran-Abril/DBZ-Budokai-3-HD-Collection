// dbz3 - In-game quick settings (F1 / Back + Start): the SDK's controller menu
// (ported from DBZ Burst Limit Recompiled) filled with the launcher's own
// settings, so both stay in sync and everything is saved to dbz3_user.toml.

#pragma once

#include <functional>

#include <rex/ui/overlay/quick_menu.h>

namespace dbz3::ingame {

// open_full_settings: opens the full launcher dialog (the F4 settings).
void ConfigureQuickMenu(rex::ui::QuickMenuConfig& menu, std::function<void()> open_full_settings);

// Rebuilds the texts if the language changed since (the launcher sets it before Play).
void RefreshQuickMenuLanguage(rex::ui::QuickMenuConfig& menu, std::function<void()> open_full_settings);

// Tutoriales de los modos (tutorial.cpp, frente F3): "Volver a ver tutoriales" mueve <datos>/ayudas.toml a
// <datos>/respaldo/ (nunca se borra) y sube la generacion, que hace releer lo visto. Cualquier hilo.
void ReiniciarAyudas();
int AyudasGeneracion();

}  // namespace dbz3::ingame
