// dbz3 - Cinematicas en el motor de B3 (frente R3, 2026-10-16): una escena por pasos dentro del combate pedido
// (hub::RequestBattleEx), con los modelos y el escenario de B3. Al empezar la pelea (estado 4) los dos luchadores se
// quedan quietos (o hacen la pose de los botones del paso, la del propio personaje), la camara va por planos
// (general, luchador 1, luchador 2) con bandas de cine, y abajo sale el subtitulo (retrato + imagen del juego de
// origen o texto propio) con su voz. A la salta; al acabar la camara vuelve a la del combate y se pelea.
// Para la Leyenda de Mr. Satan (pasarela.cpp) y despues IW y B2. Ver docs/16_pasarela_modos/01_leyenda_mr_satan.md.
#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "pasarela.h"

namespace dbz3::cinematica {

enum Plano : int { kGeneral = 0, kLado1 = 1, kLado2 = 2 };

struct Paso {
  float segundos = 3.0f;
  int plano = kGeneral;
  uint16_t botones[2] = {0, 0};   // botones de Xbox (XInput: X golpe, Y patada, B ki, A guardia) de cada lado
  int anim[2] = {-1, -1};         // pose del propio personaje (codigo X+2928, kPose*) de cada lado; -1 su guardia
  int cara = -1;                  // retrato del subtitulo: ID de B3 (pasarela::CaraDe); -1 ninguno
  int cara_alt = -1;              // si `cara` no tiene imagen (el heroe sin su icono propio todavia): esta
  pasarela::Imagen texto;         // subtitulo: imagen del juego de origen (charla 669 de B1...)
  std::string linea;              // o texto propio (si no hay imagen)
  std::vector<int> voces;         // entradas de ADX_JP.AFS de B1 (pasarela::OirVozB1) que suenan con el paso
  // Escena de MOTOR de IW (tablas del ELF de la copia del jugador, volcadas con
  // studies 3/ENTREGAS/04_iw_escenas_motor/generar.py): duracion real de la linea en cuadros de 60 Hz
  // (tabla de clips del banco; 0 = usar segundos) y camara del clip de la linea, por cuadro.
  // {cuadro, x, y, z} para ojo/objetivo; {cuadro, radianes} para fov/roll. Vacio = camara de plano.
  int cuadros = 0;
  std::vector<std::array<float, 4>> cam_ojo, cam_obj;
  std::vector<std::array<float, 2>> cam_fov, cam_roll;
  // s9b: true = ojo/objetivo van en el marco de los dos luchadores (origen = punto medio, x hacia el lado 2, y arriba,
  // z = perpendicular a su izquierda) en vez de en coordenadas del mundo: asi una toma vale en cualquier escenario.
  bool rel = false;
};
// Y+B mantenidos (XInput, mando virtual del lado; como la pose del relevo del Tag Team, acciones 20/21). ponytail:
// medido 2026-10-16 al empezar la pelea: da la accion 15 unos 40 cuadros y vuelve a 0, sin carga visible. Sin usar.
constexpr uint16_t kCargarKi = 0xA000;
// Poses medidas con la sonda DBZ3_CINE_TRAZA (accion 15 + codigo; las del propio personaje): entrada del lado 1 y del
// lado 2 (estado 2 del combate) y victoria (estados 6-8).
constexpr int kPoseEntrada1 = 0x281, kPoseEntrada2 = 0x280, kPoseVictoria = 0x2A0;

struct Escena {
  std::vector<Paso> pasos;
  std::filesystem::path iso_voces;   // ISO de B1 de las voces
  float volumen = 1.0f;
  float distancia = 0;   // > 0: los dos a esta distancia (unidades de B3) durante la escena, con su punto medio
};

// La escena se reproduce al empezar la pelea del combate siguiente (cualquier hilo).
void Empezar(Escena e);
bool Activa();   // pedida o en curso
void Cancelar();
// Los ganchos ya llaman (Fotograma corrio alguna vez): sin ellos la escena no se ve y el modo sigue con sus pantallas.
bool Conectada();

// Ganchos (hilo del juego), una llamada cada uno: al empezar el dibujo (clima.cpp, sub_820C2058, como
// iwmini::Fotograma) y tras leer cada mando (combate.cpp, sub_8208FD58 despues de orig, como iwmini::Mando).
void Fotograma(uint8_t* base, uint32_t estado);
void Mando(uint8_t* base, uint32_t usuario);

}  // namespace dbz3::cinematica
