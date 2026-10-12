// dbz3 - Launcher visual kit: fonts, icon glyphs and the shared widgets (cards,
// section titles, setting rows, status pills, help markers) that give every tab
// the same look. Used by launcher_state.cpp.

#pragma once

#include <imgui.h>

// Segoe MDL2 Assets glyphs (merged into the launcher fonts; Windows 10+). On a
// system without the icon font they render as nothing, so the labels next to
// them still read fine.
#define ICON_VIDEO "\xEE\x9F\xB4"
#define ICON_UPSCALE "\xEE\x9D\x80"
#define ICON_AUDIO "\xEE\x9D\xA7"
#define ICON_INPUT "\xEE\x9F\xBC"
#define ICON_MODS "\xEE\xAA\x86"
#define ICON_NATIVE "\xEE\x9D\x8E"
#define ICON_SWAP "\xEE\xA2\xAB"
#define ICON_TEXTURES "\xEE\x9D\xB1"
#define ICON_CHARS "\xEE\xA3\xBA"
#define ICON_DEV "\xEE\xB1\xBA"
#define ICON_PLAY "\xEE\x9D\xA8"
#define ICON_FOLDER "\xEE\xA2\xB7"
#define ICON_DISC "\xEE\xA5\x98"
#define ICON_REFRESH "\xEE\x9C\xAC"
#define ICON_SAVE "\xEE\x9D\x8E"
#define ICON_REPAIR "\xEE\xA4\x8F"
#define ICON_RESET "\xEE\x9E\xA7"
#define ICON_UPDATE "\xEE\xA2\x95"
#define ICON_INFO "\xEE\xA5\x86"
#define ICON_OK "\xEE\x9C\xBE"
#define ICON_WARN "\xEE\x9E\xBA"
#define ICON_ERROR "\xEE\xA8\xB9"
#define ICON_DOWNLOAD "\xEE\xA2\x96"
#define ICON_GLOBE "\xEE\x9D\xB4"
#define ICON_SEARCH "\xEE\x9C\xA1"

namespace dbz3::launcher::ui {

// Palette (one meaning, one color).
inline constexpr ImVec4 kAccent(0.96f, 0.55f, 0.11f, 1.0f);      // DBZ orange
inline constexpr ImVec4 kAccentDim(0.62f, 0.35f, 0.07f, 1.0f);
inline constexpr ImVec4 kAccentSoft(0.96f, 0.55f, 0.11f, 0.16f);
inline constexpr ImVec4 kBlue(0.27f, 0.56f, 0.98f, 1.0f);        // DBZ blue
inline constexpr ImVec4 kBg(0.067f, 0.075f, 0.094f, 1.0f);
inline constexpr ImVec4 kCard(0.102f, 0.114f, 0.141f, 1.0f);
inline constexpr ImVec4 kCardHeader(0.125f, 0.137f, 0.169f, 1.0f);
inline constexpr ImVec4 kFrame(0.149f, 0.165f, 0.204f, 1.0f);
inline constexpr ImVec4 kFrameHover(0.196f, 0.212f, 0.259f, 1.0f);
inline constexpr ImVec4 kFrameActive(0.239f, 0.255f, 0.306f, 1.0f);
inline constexpr ImVec4 kLine(0.196f, 0.212f, 0.255f, 1.0f);
inline constexpr ImVec4 kText(0.93f, 0.94f, 0.96f, 1.0f);
inline constexpr ImVec4 kTextDim(0.56f, 0.59f, 0.65f, 1.0f);
inline constexpr ImVec4 kOk(0.36f, 0.82f, 0.48f, 1.0f);
inline constexpr ImVec4 kWarn(1.00f, 0.74f, 0.30f, 1.0f);
inline constexpr ImVec4 kError(1.00f, 0.42f, 0.36f, 1.0f);

struct Fonts {
  ImFont* body = nullptr;   // Segoe UI, normal text
  ImFont* bold = nullptr;   // Segoe UI Semibold: labels, buttons, tabs
  ImFont* h2 = nullptr;     // card titles
  ImFont* title = nullptr;  // header title
  ImFont* banner = nullptr; // Comic Sans Bold: the in-game name banner style
  ImFont* sm = nullptr;     // help lines, chips
  ImFont* menu = nullptr;   // Arial Rounded Bold, large: rows of the in-game menus (Duelo, Tag Team)
  ImFont* speech = nullptr; // Segoe UI Semibold, large: the game's speech-bubble text
};

// Loads the launcher fonts into the shared atlas (called from the app's
// OnConfigureFonts, before the atlas texture is built). Missing system fonts
// leave the pointers null and the launcher keeps the SDK's default font.
void LoadFonts(ImFontAtlas* atlas);
const Fonts& GetFonts();

// RAII font push that tolerates a missing font.
class FontScope {
 public:
  explicit FontScope(ImFont* f) : pushed_(f != nullptr) {
    if (pushed_) ImGui::PushFont(f);
  }
  ~FontScope() {
    if (pushed_) ImGui::PopFont();
  }
  FontScope(const FontScope&) = delete;
  FontScope& operator=(const FontScope&) = delete;

 private:
  bool pushed_;
};

// A rounded panel with a title row (icon + title + optional one-line subtitle).
// Pair with EndCard(). height 0 = fit the content.
void BeginCard(const char* id, const char* icon, const char* title,
               const char* subtitle = nullptr, float height = 0.0f);
void EndCard();

// Section title inside a card: accent bar + semibold text.
void SectionTitle(const char* title);

// Setting row: the label (and an optional dim help line under it) on the left,
// then the cursor moves to the right column with the item width set, ready for
// a control drawn with a "##id" label.
void RowLabel(const char* label, const char* help = nullptr);

// Help marker: an (i) right after the previous item; hovering it or the item
// shows `text`. Replaces the old hover-only tooltips so users know help exists.
void Tip(const char* text);

// Colored pill with an optional icon (status chips in the header/footer).
void Pill(const char* icon, const char* text, ImVec4 color);

// Icon + text button (the icon in the accent color unless disabled).
bool IconButton(const char* icon, const char* label, ImVec2 size = ImVec2(0, 0));

// Dim wrapped paragraph in the small font.
void Hint(const char* text);

// Live preview of a select-wheel name banner, drawn the way roster_build.py
// makes it (white Comic Sans Bold, thick black outline, squeezed to fit the
// width), so the name can be checked while it is being typed. Returns false
// when the banner font is missing.
bool NameBanner(const char* text, float height, float max_width);

}  // namespace dbz3::launcher::ui
