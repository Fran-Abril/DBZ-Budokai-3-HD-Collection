// dbz3 - Dibujo del HUB (ver hub_panel.h): la fila "HUB" bajo la lista del menu principal y, dentro de su
// propio marco, la pantalla HUB (el juego sigue dibujando su fondo, su marco, su cinta, su bocadillo y su
// "BACK"; src/hub_menu.cpp le quita sus etiquetas, su franja y el texto de la cinta, y mueve sus
// animaciones de entrada y salida). Medidas de capturas 1600x900 del menu principal (area 4:3 desde x=200):
// interior del marco x 110..568 y 85..590, filas cada 83 px, franja de 80 px; bocadillo 30..790 x 650..858
// con el texto centrado en x 409, renglones cada 45 px; papel de la cinta del titulo x 760..1160, y 25..175.
// Piezas (res/ingame_ui, de assets_propios/generar.py): etq_<idioma>_<clave>, modo_<clave>, beta,
// hub/logo (logo "Dragon HUB" del autor del proyecto), titulo_hub y num_* (cifras de la pagina). Del juego
// (native_ui.cpp, sacadas del AFS del jugador): agua, marco, panel, flechas, botones, bocadillo, borde.
// De un pack opcional del jugador (mod center hd/packs_visuales/pack_hub.toml): hub/fondo y hub/vuelo_<n>
// (personaje que vuela hacia la camara al entrar; sin el, solo el fundido).
#include "hub_panel.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <imgui.h>
#include <toml++/toml.hpp>

#include <rex/runtime.h>
#include <rex/ui/imgui_dialog.h>

#include "../launcher/i18n.h"
#include "../launcher/settings.h"
#include "../launcher/ui_kit.h"
#include "game_style.h"

namespace dbz3 {
bool GameDimmed();
bool OverlayOpen();
}
namespace dbz3::modos {   // modos.cpp (docs/14_frentes/modos.md, API)
bool Record(int tipo, int* id, int* mejor, int* total, int* completados);
bool Guardado(int tipo, int* id, int* ganados);
std::string Nombre(int id);
}

namespace dbz3::hub {
namespace {

namespace T = dbz3::i18n;
namespace S = dbz3::ingame::style;

struct Text {
  const char *es, *en;
};
// nombre (sin su pieza etq_*) y lo que dice el bocadillo, por entrada
constexpr Text kNames[kEntries] = {{"Tag Team", "Tag Team"},        {"Online", "Online"},
                                   {"Combate rápido", "Quick Battle"}, {"Torre", "Tower"},
                                   {"Supervivencia", "Survival"},     {"Kumite", "Kumite"},
                                   {"Ranking", "Ranking"},            {"Laboratorio", "Laboratory"},
                                   {"Escenarios", "Stages"},          {"Pasarela", "Gateway"},
                                   {"Minijuegos", "Minigames"},       {"Leyenda de Mr. Satán", "Legend of Hercule"}};
// con la entrada lista (registrada)
constexpr Text kHelp[kEntries] = {
    {"Equipos de 2 o 3 luchadores. Pulsa SELECT en el combate para relevar al compañero.",
     "Teams of 2 or 3 fighters. Press SELECT in battle to switch partners."},
    {"Juega contra otra persona por internet desde el menú de Duelo.",
     "Play against another person over the internet from the Duel menu."},
    {"Un combate al momento contra el ordenador, con luchadores y escenario al azar.",
     "A fight right away against the computer, with random fighters and stage."},
    {"Sube piso a piso contra rivales cada vez más fuertes con tu último luchador del Duelo.",
     "Climb floor by floor against ever stronger rivals with your last Duel fighter."},
    {"Oleadas seguidas: la vida que te queda pasa a la siguiente y recuperas un poco entre medias.",
     "Back-to-back waves: your remaining health carries over and you recover a little in between."},
    {"Jefes con mucha vida, uno tras otro. Tu vida también se arrastra.",
     "Bosses with lots of health, one after another. Your health carries over too."},
    {"Tus mejores marcas en los modos del HUB.", "Your best records in the HUB modes."},
    {"Pruebas y experimentos con los combates.", "Battle tests and experiments."},
    {"Explora los escenarios y elige dónde luchar.", "Browse the stages and pick where to fight."},
    {"Trae luchadores de Budokai 1, Budokai 2 e Infinite World.",
     "Bring fighters over from Budokai 1, Budokai 2 and Infinite World."},
    {"Pruebas cortas para jugar entre combate y combate.", "Short challenges to play between fights."},
    {"La historia del campeón del mundo, a su manera.", "The world champion's story, his way."}};
constexpr Text kSoon = {"Muy pronto.", "Coming soon."};
constexpr Text kRowHelp = {"Tag Team, Online, combate rápido, retos y los modos nuevos de esta versión.",
                           "Tag Team, Online, quick battle, challenges and this version's new modes."};

// geometria (px del area 4:3 a 900 de alto)
constexpr float kInX0 = 110, kInX1 = 568;                  // interior del marco del menu
constexpr float kRow0 = 118, kPitch = 61, kStripe = 58;    // 8 filas en el sitio de las 6 del juego
constexpr float kIconX = 120, kLabelX = 180, kLabelK = 0.6f, kLabelMax = 372;
constexpr float kBoxX0 = 160, kBoxY0 = 606, kBoxX1 = 520, kBoxY1 = 650;   // la fila HUB (bajo el marco)
constexpr float kLogoX = 962, kLogoY = 94, kLogoW = 400;   // logo sobre el papel de la cinta
constexpr ImU32 kDim = IM_COL32(148, 148, 148, 255), kOff = IM_COL32(90, 90, 110, 200);
constexpr int kMaxFlyers = 8;                              // hub/vuelo_1..8 del pack

float Ease(float t) {
  t = std::clamp(t, 0.0f, 1.0f);
  return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}
float Clamp01(float t) { return std::clamp(t, 0.0f, 1.0f); }
ImU32 Alpha(ImU32 c, float a) { return (c & 0x00FFFFFF) | (ImU32(float(c >> 24) * Clamp01(a)) << 24); }
void FadeVerts(ImDrawList* dl, int from, float a) {
  for (int i = from; i < dl->VtxBuffer.Size; ++i) dl->VtxBuffer[i].col = Alpha(dl->VtxBuffer[i].col, a);
}

// Etiqueta de una entrada: la pieza con la letra del menu (etq_<idioma>_<clave>, 2x del atlas del juego)
// con el borde izquierdo en x y centrada en y; sin ella, la fuente. Si pasa de max_w (px del area), se
// encoge. Devuelve el borde derecho.
float Label(ImDrawList* dl, const S::Area& a, const char* key, const Text& name, float x, float y, float k,
            ImU32 tint, bool selected, float max_w = 1e9f) {
  std::string pic = std::string("etq_") + T::LangCode() + "_" + key;
  S::Pic p = S::Img(pic.c_str());
  if (!p) p = S::Img((std::string("etq_en_") + key).c_str());
  if (p) {
    const float kk = std::min(k, max_w / p.w);
    // alineadas por arriba (las piezas con g/p son mas altas: centradas bailaban); encogida, centrada
    const float h = p.h * kk * a.s, w = p.w * kk * a.s;
    const float top = a.At(0, y).y - 94 * k * a.s * 0.5f + (k - kk) * 47 * a.s;
    const ImVec2 p0(a.At(x, 0).x, top);
    S::DrawPic(dl, p, p0, ImVec2(p0.x + w, p0.y + h), ImVec4(0, 0, 0, 0), tint);
    return x + p.w * kk;
  }
  const char* text = T::T(name.es, name.en);
  ImFont* font = dbz3::launcher::ui::GetFonts().menu ? dbz3::launcher::ui::GetFonts().menu : ImGui::GetFont();
  float size = 52 * k / 0.66f * a.s;
  float w = font->CalcTextSizeA(size, FLT_MAX, 0, text).x;
  if (w > max_w * a.s) {
    size *= max_w * a.s / w;
    w = max_w * a.s;
  }
  const int v0 = dl->VtxBuffer.Size;
  S::MenuText(dl, ImVec2(a.At(x, 0).x + w * 0.5f, a.At(0, y).y), size, text, selected, false);
  for (int i = v0; i < dl->VtxBuffer.Size; ++i) {   // mismo tinte que las piezas
    ImU32& c = dl->VtxBuffer[i].col;
    const auto mul = [&](int sh) { return ImU32(((c >> sh) & 0xFF) * ((tint >> sh) & 0xFF) / 255) << sh; };
    c = mul(0) | mul(8) | mul(16) | mul(24);
  }
  return x + w / a.s;
}

// Texto en el bocadillo del menu principal (el del juego queda vacio): letra redonda gris oscuro; `title`
// (opcional) en un renglon propio en naranja oscuro, como los nombres del juego.
void BubbleText(ImDrawList* dl, const S::Area& a, const std::string& title, const char* text, float alpha) {
  if (alpha <= 0.01f) return;
  ImFont* font = dbz3::launcher::ui::GetFonts().menu;
  if (!font) font = ImGui::GetFont();
  float size = 31 * a.s, step = 45 * a.s;
  const float wrap = 600 * a.s;
  struct Line {
    const char *s, *e;
    bool title;
  };
  std::vector<Line> lines;
  auto split = [&](const char* s, bool is_title) {
    while (*s) {
      const char* e = font->CalcWordWrapPosition(size, s, s + std::strlen(s), wrap);
      if (e == s) e = s + 1;
      lines.push_back({s, e, is_title});
      s = e;
      while (*s == ' ' || *s == '\n') ++s;
    }
  };
  for (int k = 0; k < 4; ++k) {   // caben 4 renglones; con mas, letra y renglon mas pequenos
    lines.clear();
    if (!title.empty()) split(title.c_str(), true);
    split(text, false);
    if (lines.size() <= 4) break;
    size *= 0.88f;
    step *= 0.84f;
  }
  float y = a.At(0, 754).y - step * float(lines.size()) * 0.5f;
  for (const Line& l : lines) {
    const ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0, l.s, l.e);
    const ImU32 col = l.title ? IM_COL32(176, 70, 14, int(255 * alpha)) : IM_COL32(52, 52, 52, int(255 * alpha));
    dl->AddText(font, size, ImVec2(a.At(409, 0).x - ts.x * 0.5f, y + (step - ts.y) * 0.5f), col, l.s, l.e);
    y += step;
  }
}

// Reto puesto en user_data/dbz3/modos/<torre|supervivencia|kumite>.toml (modos_reto.py /
// retos_predefinidos.py): su nombre, su descripcion y cuantos combates tiene. Se relee como mucho cada 2 s.
struct Reto {
  std::string name, desc;
  int fights = 0;
  bool file = false;
  std::filesystem::file_time_type mtime{};
  double checked = -10;
};
const Reto& RetoOf(int e, double t) {
  static Reto cache[3];
  Reto& r = cache[e - kTower];
  if (t - r.checked < 2.0) return r;
  r.checked = t;
  const auto path = dbz3::settings::UserDataRoot() / "modos" / (std::string(EntryKey(e)) + ".toml");
  std::error_code ec;
  const auto mt = std::filesystem::last_write_time(path, ec);
  if (ec) {
    r = Reto{};
    r.checked = t;
    return r;
  }
  if (r.file && mt == r.mtime) return r;
  r = Reto{};
  r.checked = t;
  r.file = true;
  r.mtime = mt;
  try {
    std::ifstream in(path, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    const toml::table tb = toml::parse(ss.str());
    const bool es = std::strcmp(T::LangCode(), "es") == 0;
    auto str = [&](const char* k1, const char* k2) {
      std::string v = tb[es ? k1 : k2].value_or(std::string());
      if (v.empty()) v = tb[es ? k2 : k1].value_or(std::string());
      return v;
    };
    r.name = str("nombre", "name");
    r.desc = str("descripcion", "description");
    if (const toml::array* arr = tb["combate"].as_array()) r.fights = int(arr->size());
  } catch (const std::exception&) {
    r.name.clear();   // fichero roto: el juego usa el de serie
    r.file = false;
  }
  return r;
}

// "Récord: piso 5/8 con Goku - Guardado: piso 3" (modos.cpp; tipo 0 torre, 1 supervivencia, 2 kumite).
std::string Marks(int tipo) {
  static const Text kWord[3] = {{"piso", "floor"}, {"oleada", "wave"}, {"jefe", "boss"}};
  const char* w = T::T(kWord[tipo].es, kWord[tipo].en);
  int id = 0, best = 0, total = 0, done = 0, won = 0;
  std::string s = dbz3::modos::Record(tipo, &id, &best, &total, &done)
                      ? std::string(T::T("Récord: ", "Record: ")) + w + " " + std::to_string(best) + "/" +
                            std::to_string(total) + T::T(" con ", " with ") + dbz3::modos::Nombre(id)
                      : std::string(T::T("Sin récord", "No record"));
  if (dbz3::modos::Guardado(tipo, &id, &won)) {
    s += std::string(T::T(" - Guardado: ", " - Saved: ")) + w + " " + std::to_string(won + 1);
  }
  return s;
}

class HubPanel : public rex::ui::ImGuiDialog {
 public:
  explicit HubPanel(rex::ui::ImGuiDrawer* drawer) : ImGuiDialog(drawer) {}

 protected:
  void OnDraw(ImGuiIO& io) override {
    if (dbz3::GameDimmed() || dbz3::OverlayOpen()) return;
    const View v = Snapshot();
    const float t = float(ImGui::GetTime()), dt = std::min(io.DeltaTime, 0.1f);
    const bool shown = v.menu && !v.other;
    const float el = float(ClockMs() - v.fade_t0);
    // fundido a negro de toda la ventana (como el del juego entre sus pantallas)
    float black = 0;
    if (v.menu && v.fade == Fade::kOut) black = Clamp01(el / float(kFadeOutMs));
    if (v.menu && v.fade == Fade::kIn) black = 1.0f - Clamp01(el / float(kFadeInMs));
    if (v.fade == Fade::kOut && v.fade_to_hub && v.fade_t0 != fly_t0_) {   // un vuelo nuevo: el siguiente del pack
      fly_t0_ = v.fade_t0;
      fly_ = NextFlyer();
    }
    // la fila HUB: con el menu activo (entra al acabar de entrar el juego, sale con el)
    const bool row = shown && v.state == 3 && !v.leaving && v.fade == Fade::kNone;
    row_a_ = std::clamp(row_a_ + (row ? dt * 4.0f : -dt * 6.0f), 0.0f, 1.0f);
    // pantalla HUB: las filas entran escalonadas al abrirse y se van con el menu al salir
    const bool closing = v.fade == Fade::kOut && !v.fade_to_hub;
    const bool screen = shown && (v.open || v.leaving);
    if (screen && !was_open_) opened_at_ = t;
    was_open_ = screen;
    if (v.open && !closing) screen_a_ = 1.0f;
    else screen_a_ = screen ? std::max(0.0f, screen_a_ - dt * (closing ? 6.0f : 4.0f)) : 0.0f;
    auto* dl = ImGui::GetForegroundDrawList();
    has_bg_ = false;   // lo pone DrawScreen si el pack trae fondo
    if (shown) {
      const S::Area a = S::GameArea(io);
      // el bocadillo: su texto entra al cambiar (el del juego se funde a la vez)
      const int say = v.open && !closing ? v.cursor : (v.row_focus && v.fade == Fade::kNone ? kEntries : -1);
      if (say != said_) {
        said_ = say;
        said_at_ = t;
      }
      if (screen_a_ > 0.0f) DrawScreen(dl, a, v, t, dt);
      if (row_a_ > 0.0f) DrawRow(dl, a, v, t);
      if (say >= 0 && (row_a_ > 0.0f || screen_a_ > 0.0f)) DrawHelp(dl, a, say, t, std::max(row_a_, screen_a_));
    }
    if (black > 0.0f) dl->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(0, 0, 0, int(255 * black)));
    if (v.menu && v.fade != Fade::kNone && v.fade_to_hub) DrawFlyer(dl, io, v);
  }

 private:
  // Recuadro de la fila HUB bajo la lista (donde estaba el de TAG TEAM): marco del juego, agua si es la
  // elegida y la etiqueta con la letra del menu. En la pantalla HUB con varias paginas: LB < 1/2 > RB.
  void DrawRow(ImDrawList* dl, const S::Area& a, const View& v, float t) {
    const int v0 = dl->VtxBuffer.Size;
    const bool lit = v.row_focus || v.open;
    const ImVec2 r0 = a.At(kBoxX0, kBoxY0), r1 = a.At(kBoxX1, kBoxY1);
    // sin el parche de online_panel su recuadro TAG TEAM sigue debajo: se tapa entero
    dl->AddRectFilled(r0, r1, IM_COL32(8, 40, 56, 235));
    S::Frame(dl, r0, r1, a.s);
    if (lit) S::Highlight(dl, r0, r1, t);
    int vis[kEntries];
    const int n = VisibleEntries(vis);
    const int pages = (n + kPerPage - 1) / kPerPage;
    if (v.open && pages > 1) {
      int pos = 0;
      while (pos < n && vis[pos] != v.cursor) ++pos;
      DrawPager(dl, a, std::min(pos, n - 1) / kPerPage, pages, t);
    } else {
      const S::Pic p = S::Img((std::string("etq_") + T::LangCode() + "_hub").c_str());
      const float k = 0.66f, w = p ? p.w * k : 120;
      Label(dl, a, "hub", {"HUB", "HUB"}, (kBoxX0 + kBoxX1 - w) * 0.5f, (kBoxY0 + kBoxY1) * 0.5f, k,
            lit ? IM_COL32_WHITE : kDim, lit);
    }
    FadeVerts(dl, v0, row_a_);
  }

  // Pagina: botones LB/RB del juego a los lados, flechas del juego y "1/2" con cifras doradas (num_*).
  void DrawPager(ImDrawList* dl, const S::Area& a, int page, int pages, float t) {
    const float cy = (kBoxY0 + kBoxY1) * 0.5f, gh = 30 * a.s;
    S::Glyph(dl, "LB", a.At(kBoxX0 + 14, cy), gh);
    const float rb = S::Glyph(nullptr, "RB", ImVec2(0, 0), gh);
    S::Glyph(dl, "RB", ImVec2(a.At(kBoxX1 - 14, cy).x - rb, a.At(0, cy).y), gh);
    const std::string txt = std::to_string(page + 1) + "/" + std::to_string(pages);
    float w = 0;
    std::vector<std::pair<S::Pic, float>> pics;
    const float dh = 34;   // alto de las cifras (px del area)
    for (char c : txt) {
      const S::Pic p = S::Img(c == '/' ? "retos/num_barra" : (std::string("retos/num_") + c).c_str());
      if (!p) {
        pics.clear();
        break;
      }
      const float pw = p.w * dh / p.h;
      pics.emplace_back(p, pw);
      w += pw - 4;
    }
    const float x0 = (kBoxX0 + kBoxX1) * 0.5f - w * 0.5f;
    if (!pics.empty()) {
      float x = x0;
      for (const auto& [p, pw] : pics) {
        S::DrawPic(dl, p, a.At(x, cy - dh * 0.5f), a.At(x + pw, cy + dh * 0.5f));
        x += pw - 4;
      }
    } else {
      S::MenuText(dl, a.At((kBoxX0 + kBoxX1) * 0.5f, cy), 34 * a.s, txt.c_str(), true, false);
      w = 60;
    }
    S::Arrows(dl, a.At(x0 - 14, cy - 13), a.At(x0 + w + 14, cy + 13), 26 * a.s, t);
  }

  void DrawHelp(ImDrawList* dl, const S::Area& a, int say, float t, float alpha) {
    alpha *= Ease((t - said_at_ - 0.12f) / 0.25f);
    std::string title;
    std::string body;
    if (say >= kEntries) {
      body = T::T(kRowHelp.es, kRowHelp.en);
    } else {
      body = T::T(kHelp[say].es, kHelp[say].en);
      if (!EntryReady(say)) {
        if (const char* why = EntryWhy(say)) {   // gris por algo del jugador (RegisterGate): su explicacion
          body = why;
        } else {
          body += " ";
          body += T::T(kSoon.es, kSoon.en);
        }
      } else if (say == kTower || say == kSurvival || say == kKumite) {   // el reto puesto
        const Reto& r = RetoOf(say, t);
        if (!r.name.empty()) {
          title = std::string(T::T("Reto: ", "Challenge: ")) + r.name;
          if (r.fights > 0) title += " (" + std::to_string(r.fights) + ")";
          if (!r.desc.empty()) body = r.desc;
        } else {
          title = T::T("Reto: el de serie", "Challenge: built-in");
        }
        body += "\n" + Marks(say - kTower);
      }
    }
    if (has_bg_) {   // con el fondo del pack, el bocadillo del juego queda debajo: el nuestro
      const std::string all = title.empty() ? body : title + "\n" + body;
      const int v0 = dl->VtxBuffer.Size;
      S::Speech(dl, a.At(30, 650), a.At(790, 858), a.s, all.c_str(), 0.88f);
      FadeVerts(dl, v0, alpha);
      return;
    }
    BubbleText(dl, a, title, body.c_str(), alpha);
  }

  void DrawScreen(ImDrawList* dl, const S::Area& a, const View& v, float t, float dt) {
    const int v0 = dl->VtxBuffer.Size;
    // pack opcional del jugador: su fondo encima del menu (y entonces nuestro marco y las tiras doradas)
    if (const S::Pic bg = S::Img("hub/fondo")) {
      has_bg_ = true;
      const float sw = ImGui::GetIO().DisplaySize.x, sh = ImGui::GetIO().DisplaySize.y;
      const float k = std::max(sw / bg.w, sh / bg.h);
      const ImVec2 c(sw * 0.5f, sh * 0.5f), half(bg.w * k * 0.5f, bg.h * k * 0.5f);
      S::DrawPic(dl, bg, ImVec2(c.x - half.x, c.y - half.y), ImVec2(c.x + half.x, c.y + half.y));
      S::Sides(dl, a);
      S::Frame(dl, a.At(kInX0, 85), a.At(kInX1, 590), a.s);
    }
    DrawTitle(dl, a, t);
    // pagina del cursor y sus filas
    int vis[kEntries];
    const int n = VisibleEntries(vis);
    int pos = 0;
    while (pos < n && vis[pos] != v.cursor) ++pos;
    pos = std::min(pos, std::max(0, n - 1));
    const int page = pos / kPerPage;
    if (page != page_) {
      if (page_ >= 0 && t - opened_at_ > 0.05f) {
        page_dir_ = page > page_ ? 1.0f : -1.0f;
        if ((page_ == 0 && page > 1) || (page == 0 && page_ > 1)) page_dir_ = -page_dir_;   // vuelta
        page_at_ = t;
      }
      page_ = page;
    }
    const float since = std::max(opened_at_, page_at_);
    const float dir = page_at_ > opened_at_ ? page_dir_ : -1.0f;
    // franja de agua del juego bajo la entrada elegida (se desliza al moverse)
    const float target = kRow0 + kPitch * float(pos % kPerPage);
    if (stripe_y_ < 0 || t - since < 0.01f) stripe_y_ = target;
    stripe_y_ += (target - stripe_y_) * std::min(1.0f, dt * 22.0f);
    if (v.open) S::Highlight(dl, a.At(kInX0, stripe_y_ - kStripe * 0.5f), a.At(kInX1, stripe_y_ + kStripe * 0.5f), t);
    for (int i = page * kPerPage; i < std::min(n, page * kPerPage + kPerPage); ++i) {
      const int e = vis[i], slot = i % kPerPage;
      const float y = kRow0 + kPitch * float(slot);
      const float in = Ease((t - since - 0.05f * float(slot)) / 0.25f);   // entran una tras otra
      const float dx = 40.0f * dir * (1.0f - in);
      const int e0 = dl->VtxBuffer.Size;
      const bool sel = e == v.cursor, ready = EntryReady(e);
      const ImU32 tint = !ready ? kOff : sel ? IM_COL32_WHITE : kDim;
      if (const S::Pic ic = S::Img((std::string("modo_") + EntryKey(e)).c_str())) {
        const float s = (sel ? 52.0f + 3.0f * std::sin(t * 5.0f) : 46.0f) * a.s;
        const ImVec2 c = a.At(kIconX + 23 + dx, y);
        S::DrawPic(dl, ic, ImVec2(c.x - s * 0.5f, c.y - s * 0.5f), ImVec2(c.x + s * 0.5f, c.y + s * 0.5f),
                   ImVec4(0, 0, 0, 0), ready ? (sel ? IM_COL32_WHITE : kDim) : kOff);
      }
      const bool beta = e == kTagTeam || e == kOnline;   // siempre como beta (Tag Team y Online)
      const float right = Label(dl, a, EntryKey(e), kNames[e], kLabelX + dx, y, kLabelK, tint, sel,
                                kLabelMax - (beta ? 70.0f : 0.0f));
      if (beta) {
        if (const S::Pic b = S::Img("beta")) {
          const float h = 26 * a.s, w = b.w * h / b.h;
          const ImVec2 p0 = a.At(right + 6, y - 22);
          S::DrawPic(dl, b, p0, ImVec2(p0.x + w, p0.y + h), ImVec4(0, 0, 0, 0), sel ? IM_COL32_WHITE : kDim);
        }
      }
      FadeVerts(dl, e0, in);
    }
    FadeVerts(dl, v0, screen_a_);
  }

  // Titulo sobre la cinta nativa: el logo "Dragon HUB" (hub/logo, 1024 o 512 de ancho segun el tamano en
  // pantalla) o, sin el, "HUB" con la letra de los titulos. Entra como un sello cuando la cinta ya llego.
  void DrawTitle(ImDrawList* dl, const S::Area& a, float t) {
    const float u = Clamp01((t - opened_at_ - 0.22f) / 0.30f);
    if (u <= 0.0f) return;
    const float pop = 1.0f + 0.28f * (1.0f - Ease(u)) - 0.04f * std::sin(Clamp01(u * 1.4f) * 3.14159f);
    const float w = kLogoW * a.s;
    S::Pic p = S::Img(w > 600 ? "hub/logo" : "hub/logo_512");
    if (!p) p = S::Img("hub/logo");
    const int v0 = dl->VtxBuffer.Size;
    if (p) {
      const float h = p.h * w / p.w;
      const ImVec2 c = a.At(kLogoX, kLogoY);
      S::DrawPic(dl, p, ImVec2(c.x - w * 0.5f * pop, c.y - h * 0.5f * pop), ImVec2(c.x + w * 0.5f * pop, c.y + h * 0.5f * pop));
    } else if (const S::Pic ti = S::Img("titulo_hub")) {
      const float h = 118 * a.s * pop, tw = ti.w * h / ti.h;
      const ImVec2 c = a.At(965, 90);
      S::DrawPic(dl, ti, ImVec2(c.x - tw * 0.5f, c.y - h * 0.5f), ImVec2(c.x + tw * 0.5f, c.y + h * 0.5f));
    }
    FadeVerts(dl, v0, Ease(u * 1.6f));
  }

  // Pack opcional: el personaje vuela hacia la camara durante el fundido de entrada (crece cada vez mas
  // deprisa, la cara siempre cerca del centro, con dos estelas) y pasa de largo al volver la luz.
  void DrawFlyer(ImDrawList* dl, const ImGuiIO& io, const View& v) {
    if (fly_ <= 0) return;
    const S::Pic p = S::Img(("hub/vuelo_" + std::to_string(fly_)).c_str());
    if (!p) return;
    const float total = float(kFadeOutMs + kFadeHoldMs) + 320.0f;
    const float ms = float(ClockMs() - v.fade_t0) + (v.fade == Fade::kIn ? float(kFadeOutMs + kFadeHoldMs) : 0.0f);
    const float u = ms / total;
    if (u >= 1.0f) return;
    const float sw = io.DisplaySize.x, sh = io.DisplaySize.y;
    const float alpha = Clamp01(u / 0.12f) * (1.0f - Clamp01((u - 0.78f) / 0.22f));
    const float h0 = sh * 0.14f * std::exp(3.4f * std::pow(u, 1.5f));
    const float trail[3][2] = {{0.70f, 0.10f}, {0.84f, 0.22f}, {1.0f, 1.0f}};   // escala, alfa
    for (const auto& tr : trail) {
      const float h = h0 * tr[0], w = p.w * h / p.h;
      const ImVec2 c(sw * 0.5f, sh * 0.48f + h * 0.22f);
      S::DrawPic(dl, p, ImVec2(c.x - w * 0.5f, c.y - h * 0.5f), ImVec2(c.x + w * 0.5f, c.y + h * 0.5f), ImVec4(0, 0, 0, 0),
                 IM_COL32(255, 255, 255, int(255 * alpha * tr[1])));
    }
  }

  // El siguiente hub/vuelo_<n> que tenga el pack (se turnan); 0 sin ninguno.
  int NextFlyer() {
    for (int k = 1; k <= kMaxFlyers; ++k) {
      const int n = (fly_last_ % kMaxFlyers) + 1;
      fly_last_ = n;
      if (S::Img(("hub/vuelo_" + std::to_string(n)).c_str())) return n;
    }
    return 0;
  }

  float row_a_ = 0, screen_a_ = 0, stripe_y_ = -1, opened_at_ = 0, said_at_ = 0, page_at_ = -10, page_dir_ = 1;
  bool was_open_ = false, has_bg_ = false;
  int said_ = -2, page_ = -1, fly_ = 0, fly_last_ = 0;
  int64_t fly_t0_ = 0;
};

}  // namespace

void CreatePanel() {
  static HubPanel* panel = nullptr;   // vive lo que el juego (el dibujante no borra sus dialogos)
  auto* rt = rex::Runtime::instance();
  if (!panel && rt && rt->imgui_drawer()) panel = new HubPanel(rt->imgui_drawer());
}

}  // namespace dbz3::hub
