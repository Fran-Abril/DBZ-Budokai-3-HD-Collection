// dbz3 - In-game quick settings (see quick_settings.h).

#include "quick_settings.h"

#include <atomic>
#include <string>
#include <utility>
#include <vector>

#include <rex/cvar.h>
#include <rex/logging.h>

#include "../launcher/i18n.h"
#include "../launcher/settings.h"

namespace dbz3::ingame {
namespace {

using Item = rex::ui::QuickMenuItem;
using Choices = std::vector<std::pair<std::string, std::string>>;

// A video option changed from the menu: the quality preset becomes "manual",
// or "auto" would put the recommended values back on the next start.
std::atomic<bool> g_video_touched{false};

const char* const kVideoCvars[] = {"dbz3_resolution_scale", "dbz3_present_effect", "dbz3_fsr_render",
                                   "dbz3_fsr_sharpness",    "dbz3_cas_sharpness",  "dbz3_fxaa",
                                   "dbz3_anisotropic"};

Item Toggle(const char* label, const char* cvar, const char* help) {
  Item item;
  item.kind = Item::Kind::kToggle;
  item.label = label;
  item.cvar = cvar;
  item.help = help;
  return item;
}

Item Choice(const char* label, const char* cvar, Choices choices, const char* help) {
  Item item;
  item.kind = Item::Kind::kChoice;
  item.label = label;
  item.cvar = cvar;
  item.choices = std::move(choices);
  item.help = help;
  return item;
}

Item Number(const char* label, const char* cvar, double min, double max, double step, const char* help) {
  Item item;
  item.kind = Item::Kind::kNumber;
  item.label = label;
  item.cvar = cvar;
  item.min = min;
  item.max = max;
  item.step = step;
  item.help = help;
  return item;
}

// After any change: forward the dbz3_* values onto the SDK cvars (they apply
// live) and save them where the launcher keeps them.
// Language the menu texts were built in (the launcher can change it after startup).
int32_t g_menu_language = -1;

void ApplyAndSave() {
  if (g_video_touched.exchange(false) && dbz3::settings::QualityPreset() != "manual") {
    dbz3::settings::SetQualityPreset("manual");
  }
  rex::cvar::SetFlagByName("fullscreen", dbz3::settings::FullscreenMode() != "windowed" ? "true" : "false");
  dbz3::settings::ApplyRuntimeSettingsToSdk(true);
  dbz3::settings::SaveUserSettings();
}

}  // namespace

void ConfigureQuickMenu(rex::ui::QuickMenuConfig& menu, std::function<void()> open_full_settings) {
  namespace T = dbz3::i18n;
  g_menu_language = dbz3::settings::Language();
  T::SetLanguage(g_menu_language);

  // Once: the menu is rebuilt when the language changes (RefreshQuickMenuLanguage).
  static bool callbacks_registered = false;
  if (!callbacks_registered) {
    callbacks_registered = true;
    for (const char* name : kVideoCvars) {
      rex::cvar::RegisterChangeCallback(name, [](std::string_view, std::string_view) {
        if (rex::ui::QuickMenuDialog::IsOpen()) g_video_touched.store(true);
      });
    }
    // F3 and the menu's "Show FPS" are the same switch.
    rex::cvar::RegisterChangeCallback("debug_overlay", [](std::string_view, std::string_view value) {
      const bool on = value == "true" || value == "1";
      const std::string want = on ? "true" : "false";
      if (rex::cvar::GetFlagByName("dbz3_show_fps") != want) rex::cvar::SetFlagByName("dbz3_show_fps", want);
    });
  }

  menu.title = T::T("AJUSTES RAPIDOS", "QUICK SETTINGS");
  menu.subtitle = "DRAGON BALL Z  BUDOKAI 3  HD";
  menu.text_change = T::T("Cambiar", "Change");
  menu.text_close = T::T("Cerrar", "Close");
  // Y (mando) enciende/apaga el pack HD y cierra el menu: lo mismo que Burst Limit hace con
  // su camara libre. El cambio es en vivo (dbz3_texture_packs_on es kHotReload).
  menu.quick_toggle_cvar = "dbz3_texture_packs_on";
  menu.quick_toggle_label = T::T("Pack HD", "HD pack");
  menu.text_saved = T::T("Se guarda solo", "Saved automatically");
  menu.text_restart = T::T("REINICIO", "RESTART");
  menu.text_on = T::T("SI", "ON");
  menu.text_off = T::T("NO", "OFF");
  menu.on_changed = ApplyAndSave;

  // Render-resolution choice: every upscaler; sharpening: not DLSS (it has none).
  const std::vector<std::string> fsr = {"fsr", "fsr3", "dlss"};
  const std::vector<std::string> fsr_sharp = {"fsr", "fsr3"};

  auto& image = menu.sections.emplace_back();
  image.title = T::T("IMAGEN", "PICTURE");
  image.items.push_back(Choice(
      T::T("Resolucion interna", "Internal resolution"), "dbz3_resolution_scale",
      {{"1", "1x  (720p)"}, {"2", "2x  (1440p)"}, {"3", "3x  (4K)"}, {"4", "4x  (5K)"}},
      T::T("A cuanta resolucion dibuja el juego. Mas alto es mas nitido pero pide mas grafica.",
           "The resolution the game draws at. Higher is sharper but needs a faster GPU.")));
  image.items.push_back(Choice(
      T::T("Escalado", "Upscaler"), "dbz3_present_effect",
      {{"bilinear", T::T("Basico", "Basic")},
       {"cas", "AMD CAS"},
       {"fsr", "AMD FSR"},
       {"fsr3", "AMD FSR 3 (beta)"},
       {"dlss", "NVIDIA DLSS (beta)"}},
      T::T("Como se lleva la imagen al tamano de tu pantalla. FSR es el mas nitido; CAS solo "
           "afila. FSR 3 y DLSS suavizan los bordes usando los fotogramas anteriores (DLSS "
           "solo en graficas NVIDIA RTX con D3D12; si no, usa FSR 3).",
           "How the picture is brought to your screen size. FSR is the sharpest; CAS only "
           "sharpens. FSR 3 and DLSS smooth edges using previous frames (DLSS only on NVIDIA "
           "RTX cards with D3D12; otherwise it uses FSR 3).")));
  {
    Item& more = image.items.emplace_back(Choice(
        T::T("Mas FPS", "More FPS"), "dbz3_fsr_render",
        {{"native", T::T("Nativa", "Native")},
         {"quality", T::T("Calidad", "Quality")},
         {"balanced", T::T("Equilibrado", "Balanced")},
         {"performance", T::T("Rendimiento", "Performance")},
         {"ultra_performance", T::T("Ultra rendimiento", "Ultra performance")}},
        T::T("Dibuja por debajo de la resolucion interna y el escalado la recupera: mas FPS "
             "con una imagen algo mas suave. Solo hay escalas enteras: a 2x solo Rendimiento "
             "baja (a 1x); a 3x Calidad baja a 2x. Con 1x no cambia nada.",
             "Draws below the internal resolution and the upscaler brings it back: more FPS "
             "with a slightly softer picture. Only whole scales exist: at 2x only Performance "
             "goes lower (to 1x); at 3x Quality renders at 2x. At 1x nothing changes.")));
    more.shown_if_cvar = "dbz3_present_effect";
    more.shown_if_values = fsr;
  }
  {
    // FSR takes a sharpness reduction in stops: 0 = sharpest.
    Item& sharp = image.items.emplace_back(Number(
        T::T("Nitidez", "Sharpness"), "dbz3_fsr_sharpness", 0.0, 2.0, 0.2,
        T::T("Afilado despues de FSR.", "Sharpening after FSR.")));
    sharp.display_scale = -50.0;
    sharp.display_offset = 100.0;
    sharp.format = "%.0f%%";
    sharp.shown_if_cvar = "dbz3_present_effect";
    sharp.shown_if_values = fsr_sharp;
  }
  {
    Item& sharp = image.items.emplace_back(Number(
        T::T("Nitidez", "Sharpness"), "dbz3_cas_sharpness", 0.0, 1.0, 0.1,
        T::T("Afilado extra sobre AMD CAS.", "Extra sharpening on top of AMD CAS.")));
    sharp.display_scale = 100.0;
    sharp.format = "%.0f%%";
    sharp.shown_if_cvar = "dbz3_present_effect";
    sharp.shown_if_values = {"cas"};
  }
  image.items.push_back(Choice(
      T::T("Suavizado de bordes", "Edge smoothing"), "dbz3_fxaa",
      {{"none", T::T("No", "Off")}, {"fxaa", "FXAA"}, {"fxaa_extreme", T::T("FXAA fuerte", "FXAA (strong)")}},
      T::T("Suaviza los dientes de sierra. Muy barato; suaviza un poco la imagen.",
           "Smooths jagged edges. Very cheap; softens the picture a little.")));
  image.items.push_back(Choice(
      T::T("Filtrado de texturas", "Texture filtering"), "dbz3_anisotropic",
      {{"0", T::T("No", "Off")}, {"2", "2x"}, {"3", "4x"}, {"4", "8x"}, {"5", "16x"}},
      T::T("Mantiene nitido el suelo y lo que se ve de lado.",
           "Keeps the floor and anything seen at an angle sharp.")));
  image.items.push_back(Toggle(
      T::T("Pack HD de texturas", "HD texture pack"), "dbz3_texture_packs_on",
      T::T("Usa los packs de texturas HD instalados en mods. Apagado vuelve a las texturas "
           "originales al momento, sin reiniciar. Atajo: Y en el mando con este menu abierto.",
           "Uses the HD texture packs installed in mods. OFF switches back to the original "
           "textures right away, no restart. Shortcut: Y on the pad while this menu is open.")));
  {
    Item& rim = image.items.emplace_back(Number(
        T::T("Brillo HD", "HD shine"), "dbz3_hd_rim_light", 0.0, 1.0, 0.25,
        T::T("Halo brillante en el borde de los personajes que anadio el remaster HD. 0% lo "
             "quita (aspecto PS2); 100% es el original.",
             "Bright halo on the characters' edges added by the HD remaster. 0% removes it "
             "(PS2 look); 100% is the original.")));
    rim.display_scale = 100.0;
    rim.format = "%.0f%%";
  }

  auto& sound = menu.sections.emplace_back();
  sound.title = T::T("SONIDO Y MANDO", "SOUND & PAD");
  {
    Item& volume = sound.items.emplace_back(Number(T::T("Volumen", "Volume"), "dbz3_master_volume", 0.0,
                                                   1.0, 0.05, T::T("Volumen general del juego.",
                                                                   "Overall game volume.")));
    volume.display_scale = 100.0;
    volume.format = "%.0f%%";
  }
  sound.items.push_back(Toggle(T::T("Silenciar", "Mute"), "dbz3_mute",
                               T::T("Quita todo el sonido.", "Turns all sound off.")));
  sound.items.push_back(Toggle(T::T("Vibracion", "Vibration"), "dbz3_rumble",
                               T::T("Vibracion del mando.", "Controller vibration.")));
  sound.items.push_back(Choice(
      T::T("Botones de este menu", "Buttons for this menu"), "quick_menu_buttons",
      {{"back+start", "Back + Start"}, {"l3+r3", "L3 + R3"}, {"none", T::T("Solo teclado (F1)", "Keyboard only (F1)")}},
      T::T("Que botones del mando abren este menu. F1 en el teclado siempre funciona.",
           "Which controller buttons open this menu. F1 on the keyboard always works.")));

  auto& screen = menu.sections.emplace_back();
  screen.title = T::T("PANTALLA", "DISPLAY");
  screen.items.push_back(Choice(
      T::T("Modo de pantalla", "Screen mode"), "dbz3_fullscreen_mode",
      {{"windowed", T::T("Ventana", "Window")}, {"borderless", T::T("Pantalla completa", "Fullscreen")}},
      T::T("Ventana o pantalla completa sin bordes.", "Window or borderless fullscreen.")));
  screen.items.push_back(Toggle(
      T::T("Mostrar FPS", "Show FPS"), "dbz3_show_fps",
      T::T("FPS, tiempo de cada fotograma con grafica, resolucion y escalado en una esquina (F3).",
           "FPS, frame time with a graph, resolution and upscaler in a corner (F3).")));
  {
    Item& corner = screen.items.emplace_back(Choice(
        T::T("Esquina de los FPS", "FPS corner"), "debug_overlay_position",
        {{"top-left", T::T("Arriba izquierda", "Top left")},
         {"top-right", T::T("Arriba derecha", "Top right")},
         {"bottom-left", T::T("Abajo izquierda", "Bottom left")},
         {"bottom-right", T::T("Abajo derecha", "Bottom right")}},
        T::T("Donde se ve el panel de FPS.", "Where the FPS panel sits.")));
    corner.shown_if_cvar = "dbz3_show_fps";
    corner.shown_if_values = {"true"};
  }
  screen.items.push_back(Toggle(
      T::T("Oscurecer en segundo plano", "Dim in the background"), "dbz3_dim_unfocused",
      T::T("Oscurece el juego cuando cambias a otra ventana.",
           "Darkens the game while you are in another window.")));
  if (open_full_settings) {
    Item full;
    full.kind = Item::Kind::kAction;
    full.label = T::T("Todos los ajustes", "All settings");
    full.value_text = "F4";
    full.help = T::T("Abre el launcher completo encima del juego: mods, personajes, controles...",
                     "Opens the full launcher over the game: mods, characters, controls...");
    full.action = std::move(open_full_settings);
    screen.items.push_back(std::move(full));
  }
  REXLOG_INFO("dbz3: quick settings menu ready (F1 / {})", rex::cvar::GetFlagByName("quick_menu_buttons"));
}

void RefreshQuickMenuLanguage(rex::ui::QuickMenuConfig& menu, std::function<void()> open_full_settings) {
  if (dbz3::settings::Language() == g_menu_language) return;
  rex::ui::QuickMenuConfig fresh;
  ConfigureQuickMenu(fresh, std::move(open_full_settings));
  // Everything but the callbacks (can_open is the caller, still running).
  menu.title = std::move(fresh.title);
  menu.sections = std::move(fresh.sections);
  menu.quick_toggle_label = std::move(fresh.quick_toggle_label);
  menu.quick_toggle_cvar = std::move(fresh.quick_toggle_cvar);
  menu.subtitle = std::move(fresh.subtitle);
  menu.text_change = std::move(fresh.text_change);
  menu.text_close = std::move(fresh.text_close);
  menu.text_saved = std::move(fresh.text_saved);
  menu.text_restart = std::move(fresh.text_restart);
  menu.text_on = std::move(fresh.text_on);
  menu.text_off = std::move(fresh.text_off);
}

}  // namespace dbz3::ingame
