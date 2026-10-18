// dbz3 - CINE (ficha 22): intro de combate de 10 s en tiempo real con camara propia y salto. Ver cine.cpp. Solo US.
#pragma once

#include <cstdint>

namespace dbz3::cine {

// Intro del combate siguiente (una vez): para retos y campanas. La opcion dbz3_cine_intro la pone en todos.
void Pedir();
bool EnCurso();
// Mando del luchador (X+5028, 16 bits; -1 = COM) como era antes de la intro (la intro deja al COM en guardia con
// otro mando mientras dura): para saber si un lado es el ordenador tambien durante la intro.
int16_t Mando(uint8_t* base, int lado);
// Camara de la pelea (mgr+0x190 ojo, +0x1A0 objetivo, +0x60 matriz de vista): leer la del juego / poner una en
// este fotograma (llamar al empezar el dibujo). Ojo: el juego parte de su ojo anterior; para volver a la suya,
// fundirse con ella (no alejarla en cada fotograma: se va lejos).
bool LeerCamara(uint8_t* base, float ojo[3], float obj[3]);
// giro: balanceo en radianes alrededor del eje de vision (0 = arriba del mundo; el vuelo de IW lo usa).
void Camara(uint8_t* base, const float ojo[3], const float obj[3], float giro = 0.0f);

// Ganchos (clima.cpp): combate nuevo (sub_820C26E0) y cada fotograma de escenario con el estado del combate
// (mgr+35128) y los fotogramas en estado 4.
void NuevoCombate();
void Fotograma(uint8_t* base, uint32_t estado, uint32_t frames_pelea);

}  // namespace dbz3::cine
