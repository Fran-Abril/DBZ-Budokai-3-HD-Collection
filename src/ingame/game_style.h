// dbz3 - Dibujo con el aspecto de los menus de Budokai 3 (marco naranja, filas con letras
// turquesa perfiladas, cinta del titulo, bocadillo). Lo usan las pantallas propias que se
// meten dentro del juego (ONLINE en el Duelo, TAG TEAM en el menu principal).
// Piezas: ingame_ui/<nombre>.png, que roster_build.ui_native saca de los AFS del jugador (marco,
// bocadillo, botones, fondos, anfitriones) y genera (titulos con la letra del juego). Un mod puede
// cambiarlas (mods/<mod>/ui_ingame/). Sin ellas se dibujan sustitutos.
#pragma once

#include <imgui.h>

#include <string>
#include <utility>
#include <vector>

namespace dbz3::ingame::style {

// Area 4:3 donde dibuja el juego (centrada en la ventana) y escala respecto a 1600x900.
struct Area {
  ImVec2 p0, p1;
  float s = 1.0f;   // 1.0 = ventana de 900 px de alto
  ImVec2 At(float x, float y) const { return ImVec2(p0.x + x * s, p0.y + y * s); }   // en px de 1200x900
};
Area GameArea(const ImGuiIO& io);

// Imagen <nombre>.png: primero la de un mod activo (mods/*/ui_ingame/), si no la de serie
// (ingame_ui/). Sin ninguna, w = 0. Hilo de dibujo.
struct Pic {
  ImTextureID tex{};
  float w = 0, h = 0;
  explicit operator bool() const { return w > 0; }
};
Pic Img(const char* name);
// Recorte `src` (px de la textura: x0, y0, x1, y1; todo ceros = entera) en p0..p1.
void DrawPic(ImDrawList* dl, const Pic& p, ImVec2 p0, ImVec2 p1, ImVec4 src = ImVec4(0, 0, 0, 0),
             ImU32 tint = IM_COL32_WHITE, bool mirror = false);

void Frame(ImDrawList* dl, ImVec2 p0, ImVec2 p1, float s);                 // marco de las listas
void Highlight(ImDrawList* dl, ImVec2 p0, ImVec2 p1, float t);             // franja de la fila elegida
void MenuText(ImDrawList* dl, ImVec2 center, float size, const char* text, bool selected, bool disabled);
// Cinta del titulo arriba a la izquierda, como la del Duelo; `title` = imagen del titulo
// (ingame_ui) o, sin ella, `text` con la fuente.
void Banner(ImDrawList* dl, const Area& a, const char* text, const char* title = nullptr);
// Bocadillo; tail_x = donde sale el pico (0..1 del ancho, hacia arriba) o < 0 sin pico.
void Speech(ImDrawList* dl, ImVec2 p0, ImVec2 p1, float s, const char* text, float tail_x = 0.75f);
// Fondo del area 4:3: dibujo del juego `image` (ingame_ui) o degradado.
void Backdrop(ImDrawList* dl, const Area& a, const char* image = nullptr);
// Anfitrion que habla (como los Kaio del Duelo): esquina de arriba en (x, y) px de 1200x900, al
// tamano del juego; lo que sale del area se corta. Despues va Sides (los tapa, como en el juego).
void Host(ImDrawList* dl, const Area& a, const char* image, float x, float y, bool mirror);
// Tiras doradas de los lados del area (las de los menus del juego).
void Sides(ImDrawList* dl, const Area& a);
// Boton del mando con el dibujo del juego: "A", "B", "X", "Y", "LB", "LT", "RB", "RT", "BACK"
// (= SELECT), "START". pos = borde izquierdo, centro vertical. Devuelve el ancho usado.
float Glyph(ImDrawList* dl, const char* button, ImVec2 pos, float h);
// Linea de ayuda centrada: boton + texto, ... (como "BACK : Help" de los menus).
void HelpLine(ImDrawList* dl, ImVec2 center, float h, const std::vector<std::pair<const char*, const char*>>& items);
// Flechas izquierda/derecha del juego a los lados de p0..p1 (selector de formato).
void Arrows(ImDrawList* dl, ImVec2 p0, ImVec2 p1, float h, float t);

// Tira de un equipo sobre la rueda del select (Tenkaichi 3): `slots` casillas numeradas, las
// elegidas con la cara y el nombre. mirror = lado derecho (jugador 2). active = eligiendo.
void TeamStrip(ImDrawList* dl, ImVec2 p0, float s, bool mirror, int slots, const std::vector<std::string>& names,
               const std::vector<ImTextureID>& faces, bool active, bool done, float t);
// Ficha de un compañero del Tag Team bajo el marcador del juego (estilo FighterZ/Tenkaichi 3):
// cara (la del marcador del juego; sin ella, la inicial), nombre y barra de vida. mirror = lado del jugador 2 (crece hacia la izquierda).
// button = boton que lo hace entrar ("BACK" o "BACK+LB") o nullptr; switching = parpadeo del relevo.
void PartnerCard(ImDrawList* dl, ImVec2 p0, float s, bool mirror, const char* name, ImTextureID face, float hp,
                 bool down, const char* button, bool switching, float t);

}  // namespace dbz3::ingame::style
