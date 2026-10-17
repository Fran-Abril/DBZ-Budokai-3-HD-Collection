// dbz3 - Piezas del motor de vuelo de Infinite World (src/iw_mini_vuelo.cpp, frente R12) para otros minijuegos que
// usan el mismo esquema (R13: el 3, ladrillos de Kaio, con su #MSS de DATA_CMN 890). Todo leido del ELF USA / decomp EU;
// ver docs/14_frentes/RONDA4_PULIDO.md "R12". Solo US (como iw_minijuegos).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dbz3::iwvuelo {

// Objeto de una lista del #MSF: u32 (flags en los 16 bits bajos, tipo en los altos), x, z (12 B). Tipos del vuelo:
// 0 meta, 0x100 punto de control, 0x201/0x101/1 monedas (100/500/1000), 2 velocidad, 3 sin R1, 8/0x108 congela el
// medidor, 10 al azar. r = radio de recogida (cabecera +0x60 + 4 (tipo & 0xF)), en el plano (d^2 < r^2).
struct Objeto {
  int tipo = 0, flags = 0;
  float x = 0, z = 0, r = 1;
};

// #MSF (DATA_CMN 891 hijos 23 y 24): cabecera + el bloque de una dificultad (0..5; tabla de 6 desplazamientos en +0xA8).
struct Msf {
  // cabecera: hover (+0x14), empuje de los estados 3/5/6 (+0x18..), velocidad minima del frenazo (+0x24), velocidad del
  // frenazo (+0x28), freno por cuadro de 3/5/6 (+0x34..), giro por estado en 1/65536 de vuelta por cuadro (+0x44 hover,
  // +0x4C.. 3/5/6), duracion de los objetos 2 y 3 (+0x58/+0x5C), radios por tipo (+0x60..)
  float hover = 16, empuje[3] = {10, 34, 56}, vmin = 28, vfreno = 10, freno[3] = {0.98f, 0.98f, 0.98f};
  int giro0 = 192, giro[3] = {96, 96, 96};
  float dur2 = 180, dur3 = 180, radio[16] = {};
  // bloque: salida {estado, giro u16, x, z}, meta, listas L1 (los n_control primeros: puntos de control) y L2,
  // medidor inicial (+0x14) y su subida por cuadro (+0x18 base, +0x1C estado 5, +0x20 estado 6), cuadros de congelado
  // (+0x24) y de reaparicion (+0x28), puntos de moneda (+0x2C), rangos en segundos (+0x38; el 5.o es el limite),
  // puntos por rango (+0x4C) y al fallar (+0x60)
  int salida_estado = 0, salida_giro = 0;
  float sx = 0, sz = 0;
  Objeto meta;
  std::vector<Objeto> lista;
  int n_control = 0;
  float medidor0 = 0, sube[3] = {}, congela = 300, reaparece = 120;
  int puntos_moneda[3] = {1000, 500, 100}, rango_s[5] = {}, puntos_rango[5] = {}, puntos_fallo = 100;
};
bool LeerMsf(const std::vector<uint8_t>& msf, int dificultad, Msf& out, std::string& error);

// Tabla de bloques por dificultad de un guion de minijuego (#MSF: 6 desplazamientos en +0xA8; #MSS de 890: en +0x2C):
// los `n` u32 desde `en` (desplazamientos desde el principio del archivo; 0 si se salen).
std::vector<uint32_t> Bloques(const std::vector<uint8_t>& guion, uint32_t en, int n = 6);

// Eje de la palanca como Viewer_ReadControlAxes (EU 0x33E8B0): s8 con zona muerta 64 -> (v -+ 64) / 64 (0..~1).
float EjeIW(int v);
// Diferencia en un mundo que da la vuelta (Node_WrapPositionInGrid): resultado en [-periodo/2, periodo/2).
float Envolver(float d, float periodo);

// Camara del vuelo (CameraFocusWhileFlying EU 0x339AA0: distancia 52.2 por detras y cabeceo -0x800 = -11,25 grados,
// girada con el personaje): ojo y objetivo para `pos` (pies) y `yaw` (u16, 1/65536 de vuelta, delante = (sin, cos)).
// `alto`: altura del objetivo sobre los pies. Ponerla con dbz3::cine::Camara(base, ojo, obj) en el hilo del juego.
void CamaraVuelo(const float pos[3], int yaw, float ojo[3], float obj[3], float alto = 8.0f, float dist = 52.2f,
                 int cabeceo = -0x800);
// Punto delante de la camara del combate como la flecha del vuelo (World_CharacterLoader USA 0x33B1E8: matriz de la
// camara por (0, -9, -45) de 0x482AA0): `adelante` por delante del ojo y `abajo` por debajo. false sin camara.
bool DelanteDeCamara(uint8_t* base, float adelante, float abajo, float out[3]);
// Proyeccion con la camara del combate (vista mgr+0x60, fov mgr+0x1C4; ps2::CamaraCombate): punto del mundo -> pixel
// de una pantalla ancho x alto y pixeles por unidad a esa distancia. false si queda detras.
bool Proyectar(uint8_t* base, float ancho, float alto, const float p[3], float& x, float& y, float& px_por_unidad);

// Personaje de IW que el jugador lleva en el mapa de Dragon Mission (en IW: tabla 0x47D450 [DM+0x374] -> descriptor de la
// mision +0x10). El vuelo elige con el: 2 (Gohan nino) en el 7 y 9 (Trunks nino) en el 17 cambian de personaje. -1 = no
// se sabe (IW: el de cualquier otro). Lo llama pasarela_misiones.cpp antes de cada minijuego.
void PersonajeDelMapa(int id_iw);

}  // namespace dbz3::iwvuelo
