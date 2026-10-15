// dbz3 - Pasarela de modos (src/pasarela.cpp: Leyenda de Mr. Satan; src/pasarela_misiones.cpp: saga de B1, tablero,
// misiones de B2) y lectura de la ISO de Budokai 1 del jugador (src/pasarela_b1.cpp).
// Nada del juego de origen va en la version: todo se lee en tiempo de ejecucion de la copia del jugador.
#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <imgui.h>

#include "ingame/game_style.h"

namespace dbz3::pasarela {

struct Imagen {
  int w = 0, h = 0;
  std::vector<uint8_t> rgba;
  explicit operator bool() const { return w > 0; }
};

// Un combate de B1 (registro de 128 B del ELF; ver docs/16_pasarela_modos/): los de "La leyenda de Mr. Satan" y los
// de la historia (misma estructura, modo 0, +0x28 capitulo).
struct CombateB1 {
  int jugador_b1 = 21;    // +0x20 personaje del lado 1
  int forma_jugador = 0;  // +0x22
  int ki_jugador = 300;   // +0x2C (0xFFFF = el que le quedo)
  int capitulo = -1;      // +0x28 (historia: orden dentro de la saga del protagonista)
  int modo = 0;           // +0x70 (3 Mr. Satan, 0 historia)
  int rival_b1 = 0;       // ID de personaje de B1 (registro +0x50)
  int rival = 0;          // ID de B3 equivalente
  int variante = 0;       // +0x51 (sin confirmar)
  int forma = 0;          // +0x52 forma con la que empieza (Goku 4 = SSJ, Gohan 1 = SSJ, Cell 2 = perfecto)
  int cantidad = 1;       // +0x56 rivales seguidos (Cell Jr: 5)
  int vida_rival = 3000;  // +0x5A
  int ki_rival = 300;     // +0x5C
  bool arrastra_vida = false;   // vida del jugador 0xFFFF en +0x2A: no se recupera (Krilin, Gohan)
  int vida_jugador = 3000;
  int escenario_b1 = 0;   // +0x71 (14 = CHG_CELL RING)
  std::string escenario_nombre;   // nombre del registro de escenario de B1
  int musica = 0;         // +0x72 (hipotesis)
  int tiempo = -2;        // +0x74 en cuadros (5940 = 99 s; -1 sin limite; -2 el de las opciones)
};

// Piezas de la pantalla de resultado del modo (entrada 668 del AFS de cada idioma) y del titulo (666).
struct ResultadoB1 {
  Imagen tablon;     // 668 #15: tablon con el logo y "NEXT BATTLE"
  Imagen retadores;  // 668 #16: "Challenger 1st".."11th" (3 x 4 de 136 x 72)
  Imagen cruz;       // 668 #17: aspa roja
  Imagen total;      // 668 #18: Mr. Satan y "Total ... Pts."
  Imagen ahora;      // 668 #19: "Now!! ... Pts."
  Imagen tienes;     // 668 #21: "You have ... Pts."
  Imagen cifras;     // 668 #22: 0-9 (5 x 2 de 48 x 64)
  Imagen chibis;     // 668 #24: caras de los 11 retadores (7 x 3 de 72 x 72; despues, sus siluetas)
  Imagen letrero;    // 666 #6: letrero de madera del titulo
  Imagen dificultad[3];   // 666 #7-9: Novato, Experto, Avanzado
};

struct DatosB1 {
  std::string iso, elf, afs;      // de donde salio (para el registro)
  uint32_t tabla = 0;             // offset de la tabla en el ELF
  std::vector<CombateB1> combates;
  std::vector<CombateB1> historia;   // combates del modo historia (los registros de antes de la tabla del modo)
  std::string idioma;             // "es", "en"... de las imagenes
  std::vector<Imagen> charlas;    // banco de charlas: 2 por combate desde el 2.o (charla de Mr. Satan, aviso)
  Imagen logo, fondo, satan;      // pantalla del modo: logo, fondo azul y dibujo de Mr. Satan
  ResultadoB1 res;
};

// ISO de Budokai 1 en ps2_games (junto a dbz3.exe; DBZ3_PS2_GAMES la sustituye en las pruebas); vacio si no hay.
std::filesystem::path BuscarIsoB1();
// Lee la ISO (sin extraerla): tabla de combates del ELF y, en el idioma `lang` (es/en/fr/de/it), las imagenes.
bool LeerLeyendaSatan(const std::filesystem::path& iso, const std::string& lang, DatosB1& out, std::string& error);
// ID de B3 que hace de un personaje de B1 (el donante del importador; Zarbon/Dodoria: Cooler/Nappa).
int B1aB3(int id_b1);

// Infinite World (ficha 02 C): titulo y texto de una mision (imagenes de DATA_USA.AFS de la copia del jugador).
struct MisionIW {
  int numero = 0;          // 0 = "Training No.1"; la entrada de DATA_USA es 132 + numero
  Imagen titulo, texto;
};
struct DatosIW {
  std::string origen;
  std::vector<MisionIW> misiones;   // en el orden pedido
};
// Carpeta extraida o ISO de Infinite World en ps2_games (vacio si no hay).
std::filesystem::path BuscarIW();
bool LeerMisionesIW(const std::filesystem::path& iw, const std::vector<int>& numeros, DatosIW& out, std::string& error);

// Para el HUB (parche en docs/14_frentes/pasarela.md): disponible = hay ISO de Budokai 1 y se pudo leer.
bool Disponible();
const char* Motivo();
bool Start();
bool Busy();

// ---- Piezas comunes de la pasarela (pasarela.cpp) ----

// Datos de la ISO de B1 ya leidos (nullptr = sin ISO o sin leer); pide la lectura si hace falta.
std::shared_ptr<const DatosB1> DatosLeidos();

// Avance: user_data/dbz3/pasarela/progreso.toml (una clave por modo; se reescribe entero conservando las demas).
int LeerProgreso(const std::string& clave, int defecto = 0);
void GuardarProgreso(const std::string& clave, int valor);

// Personaje de un mod instalado (mods/_roster/roster.toml + personaje.toml del mod) por su origen:
// fuente "b1"/"b2"/"iw"... y clave del importador ("13" Zarbon de B1, "XGXL" Fat Gotenks de B2). -1 = no esta.
int IdPort(const std::string& fuente, const std::string& clave);
// Traje en el juego (rec+18) del traje extra de la carpeta de mod `carpeta` (traje.toml) del personaje `id`; -1 si no
// esta instalado.
int TrajeExtra(int id, const std::string& carpeta);
// Nombre para mostrar (los de B3 en el idioma del juego; los de mods, el de su autor).
std::string NombreDe(int id);

// Reglas del combate en curso que se aplican desde fuera del juego (memoria del luchador; sin ganchos).
struct Reglas {
  int nivel_com = -1;         // nivel de la IA del COM (mgr+35573: 0 Novato, 1 Experto, 2 Avanzado); -1 el de serie
  float ki_rival = 0;         // ki inicial del rival en barras de B1 (300 = 3); 0 = el de serie
  float armadura_rival = 0;   // parte del dano al rival que se anula (0.5 = la mitad): Androide 16
  bool solo_ki = false;       // el jugador solo hace dano con ki (golpes al 25 %): Mr. Satan herido
  float ki_lento = 0;         // parte del ki que gana el jugador que se pierde (0.5): Piccolo
  int traje_jugador = 0;      // traje del jugador (rec+18; los extra de traje.toml van tras los del juego)
  int traje_rival = 0;
  bool traza = false;         // pruebas: vida y ki de los dos cada segundo en el registro
  bool capsulas = true;       // cada lado con la lista Normal de capsulas de su personaje (tecnicas, como el Duelo)
  int capsulas_rival[8] = {}; // pruebas: [cuantas, 7 IDs] en lugar de la lista Normal del rival
  int golpes = 0;             // reto: golpes al rival para cumplirlo (al llegar, el combate acaba a favor)
  bool sin_atacar = false;    // reto: si el jugador quita vida al rival, falla (el combate acaba en contra)
  std::atomic<int>* contador = nullptr;   // golpes al rival en vivo (marcador del reto en pantalla)
};
// Fin del combate pedido (lado 0 = jugador).
struct Fin {
  int ganador = -1;           // -1 = no acabo (salio desde la pausa o no empezo)
  int vida[2] = {0, 0}, vida_max[2] = {0, 0};
  int segundos = -1;          // tiempo que sobraba; -1 sin limite
  bool empezo = false;
  int golpes = 0;             // golpes que quitaron vida al rival
  bool ataco = false;         // el jugador quito vida al rival (reto sin atacar)
};
// Vigila el combate pedido con dbz3::hub::RequestBattleEx (hilo propio): aplica las reglas mientras se pelea y llama
// a `fin` cuando el conector suelta el combate. `quien` = prefijo del registro ("pasarela", "misiones").
void VigilarCombate(const char* quien, const Reglas& reglas, std::function<void(const Fin&)> fin);
// Estado del combate vigilado (mgr+35128: 1 vuelo, 2 entradas, 3 READY, 4 pelea, 5-7 final); 0 = ninguno.
int EstadoCombate();
// Escenario de B3 (indice del registro de escenarios) cuyo mapa es `mapa` (entrada de data_cmn): 50 Juegos de Cell,
// 60 Namek, 46 Templo de Kami... (stage_ext.h). 0 si no hay.
int EscenarioPorMapa(int mapa);
// Escenario de B3 para el nombre de un escenario de B1 ("CHG_NAMEC" -> Namek).
int EscenarioB3(const std::string& nombre_b1);
// Forma inicial de B3 (rec+20) del personaje `id` de B3 para la forma `forma_b1` de su registro de B1.
int FormaB3(int id, int forma_b1);

// Dibujo (hilo de la interfaz): cara del marcador (del juego o de su mod) en un marco del juego, con el nombre debajo.
ImTextureID CaraDe(int id);
void DibujarCara(ImDrawList* dl, const ingame::style::Area& a, int id, float x0, float y0, float w, bool on,
                 const char* nombre, float t, bool espejo);

}  // namespace dbz3::pasarela
