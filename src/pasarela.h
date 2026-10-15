// dbz3 - Pasarela de modos (src/pasarela.cpp) y lectura de la ISO de Budokai 1 del jugador (src/pasarela_b1.cpp).
// Nada del juego de origen va en la version: todo se lee en tiempo de ejecucion de la copia del jugador.
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace dbz3::pasarela {

struct Imagen {
  int w = 0, h = 0;
  std::vector<uint8_t> rgba;
  explicit operator bool() const { return w > 0; }
};

// Un combate de "La leyenda de Mr. Satan" (registro de 128 B del ELF de B1; ver docs/16_pasarela_modos/).
struct CombateB1 {
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

struct DatosB1 {
  std::string iso, elf, afs;      // de donde salio (para el registro)
  uint32_t tabla = 0;             // offset de la tabla en el ELF
  std::vector<CombateB1> combates;
  std::string idioma;             // "es", "en"... de las imagenes
  std::vector<Imagen> charlas;    // banco de charlas: 2 por combate desde el 2.o (charla de Mr. Satan, aviso)
  Imagen logo, fondo, satan;      // pantalla del modo: logo, fondo azul y dibujo de Mr. Satan
};

// ISO de Budokai 1 en ps2_games (junto a dbz3.exe; DBZ3_PS2_GAMES la sustituye en las pruebas); vacio si no hay.
std::filesystem::path BuscarIsoB1();
// Lee la ISO (sin extraerla): tabla de combates del ELF y, en el idioma `lang` (es/en/fr/de/it), las imagenes.
bool LeerLeyendaSatan(const std::filesystem::path& iso, const std::string& lang, DatosB1& out, std::string& error);

// Para el HUB (parche en docs/14_frentes/pasarela.md): disponible = hay ISO de Budokai 1 y se pudo leer.
bool Disponible();
const char* Motivo();
bool Start();
bool Busy();

}  // namespace dbz3::pasarela
