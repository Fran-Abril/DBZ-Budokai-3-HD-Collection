// dbz3 - Menu HUB: la 8a fila "HUB" del menu principal y su pantalla con las entradas propias del port
// (Tag Team, Online, Combate rapido y los modos de otros frentes). La logica va en el hilo del juego
// dentro del propio menu principal (src/hub_menu.cpp: su mando, sus sonidos, su bocadillo, su marco, sus
// animaciones de entrada y salida); el dibujo de lo que el juego no tiene (etiquetas, franja, logo, texto
// de ayuda, fundido), en hub_panel.cpp. Solo US (la imagen EU no pasa por los ganchos).
// Plan: docs/12_conectores/11_menu_principal_nativo.md s9; estado: docs/14_frentes/hub.md.
#pragma once

#include <cstdint>

namespace dbz3::hub {

// Entradas del HUB, en orden. Las 8 primeras se ven siempre (en gris con "Muy pronto" hasta que su frente
// se registre); las de kStages en adelante solo cuando alguien las registra. Mas de 8 visibles: paginas
// (LB/RB o izquierda/derecha).
enum Entry {
  kTagTeam, kOnline, kQuick, kTower, kSurvival, kKumite, kRanking, kLab,
  kStages, kGateway, kMinigames, kSatan, kEntries
};
constexpr int kAlwaysShown = kStages;   // entradas que se ven aunque nadie las registre
constexpr int kPerPage = 8;             // filas por pagina (las 8 caben en el marco del menu)
// "tagteam", "online", "rapido", "torre", "supervivencia", "kumite", "ranking", "laboratorio",
// "escenarios", "pasarela", "minijuegos", "satan" (nombre de sus piezas: etq_<idioma>_<clave>.png,
// modo_<clave>.png en res/ingame_ui, que genera assets_propios/generar.py)
const char* EntryKey(int e);

// Otros frentes: dar vida a una entrada de EntryKey (las tres primeras ya la tienen). Se puede llamar
// desde un inicializador estatico de su .cpp, sin tocar este frente:
//   static const bool reg = dbz3::hub::RegisterMode("torre", &TorreOpen, &TorreBusy);
// open: hilo del juego, con el menu principal activo (puede llamar a RequestBattle o pedir pantallas).
// busy: mientras devuelva true (su pantalla propia abierta) el HUB ni se dibuja ni lee el mando; al
// volver a false, el HUB se abre otra vez. Sin busy, se espera a que el menu principal vuelva a nacer.
bool RegisterMode(const char* key, void (*open)(), bool (*busy)() = nullptr);
// Entrada que depende de algo del jugador (p. ej. su copia de otro juego): ready() false = en gris (se ve igual
// si esta registrada) y el bocadillo dice why() (texto ya traducido) en lugar de "Muy pronto". Cualquier hilo;
// las dos se llaman en cada fotograma: tienen que ser baratas.
//   static const bool g = dbz3::hub::RegisterGate("satan", &Disponible, &Motivo);
bool RegisterGate(const char* key, bool (*ready)(), const char* (*why)());
const char* EntryWhy(int e);   // why() de una entrada en gris (nullptr = sin explicacion propia)
bool EntryReady(int e);     // se puede elegir (registrada, o Tag Team/Online con su interruptor)
bool EntryVisible(int e);   // sale en la lista
// Entradas visibles en orden (out con sitio para kEntries); devuelve cuantas.
int VisibleEntries(int* out);

// La fila HUB existe en el menu principal (US, interruptor dbz3_hub_enabled). Mientras sea true,
// online_panel.cpp no dibuja su fila TAG TEAM ni cuenta las filas del menu principal (Tag Team esta
// dentro del HUB y sigue con X). Cualquier hilo.
bool RowActive();

// Fundido propio entre el menu principal y la pantalla HUB (como el del juego al entrar en un menu):
// el menu sale con su animacion, la pantalla se funde a negro, cambia el contenido, vuelve a entrar con
// su animacion de entrada y se funde desde negro.
enum class Fade : uint8_t { kNone, kOut, kIn };
constexpr int64_t kFadeOutMs = 520, kFadeHoldMs = 80, kFadeInMs = 520, kEntryMs = 600;

// Lo que dibuja hub_panel.cpp; lo publica el hilo del juego en cada fotograma del menu principal.
struct View {
  bool menu = false;       // menu principal en pantalla (dibujado hace menos de 0,2 s)
  int state = 0;           // +148: 2 entrando, 3 activo, 4 sale con B, 5 sale con A
  bool other = false;      // otra pantalla propia encima (TAG TEAM, ONLINE, piloto, la de un modo)
  bool row_focus = false;  // cursor en la fila HUB
  bool open = false;       // pantalla HUB
  bool leaving = false;    // el menu se va hacia un modo elegido en el HUB
  int cursor = 0;          // entrada elegida (Entry)
  Fade fade = Fade::kNone; // fundido en curso
  bool fade_to_hub = false;
  int64_t fade_t0 = 0;     // ms (ClockMs) del principio de la fase
  int rows = 6;            // filas nativas del menu (7 con Dragon Arena)
};
View Snapshot();
int64_t ClockMs();         // reloj de View::fade_t0 (steady_clock en ms)
// Hilo de la interfaz: crea el dialogo que dibuja todo (lo pide el hilo del juego la primera vez).
void CreatePanel();

}  // namespace dbz3::hub
