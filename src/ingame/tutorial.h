// dbz3 - Tutoriales optativos de los modos nuevos (frente F3, ronda 4, 2026-10-16): la primera vez que se entra en un
// modo sale un panel con el estilo del juego (marco, bocadillo y barra de botones de game_style.h), paso a paso:
// A siguiente, B cerrar (vuelve la proxima sesion), Y "No volver a mostrar". Lo visto se guarda en
// UserDataRoot()/ayudas.toml (clave = id, true); sin el archivo, todo "sin ver". Ajustes rapidos (F1): "Mostrar
// tutoriales" (cvar dbz3_tutoriales) y "Volver a ver tutoriales" (mueve ayudas.toml a respaldo/).
#pragma once


namespace dbz3::ingame::tutorial {

struct Paso {
  const char* es;
  const char* en;   // fr/de/it: res/ingame_ui/i18n_modos.toml (clave = este texto)
};

// Abre el tutorial del modo `id` (catalogo en tutorial.cpp: hub, retos, heroe, satan, dragon_world, nave_babidi,
// dragon_mission, fighters_road, iw_mini_carrera/vuelo/disparo/botones) si esta activada la opcion, no esta visto y no
// hay otro abierto. Se puede llamar cada cuadro. true = abierto (la pantalla de debajo espera). Hilo de la interfaz.
bool Mostrar(const char* id);
// El del minijuego de la casilla `clave` del mapa de IW (0x7D0..0x7E1): su tipo, con los controles la primera vez.
bool MostrarMinijuego(int clave);
// Hay un tutorial en pantalla: la pantalla de debajo no debe leer el mando. Cualquier hilo.
bool Abierto();
// Ya visto (guardado en ayudas.toml o cerrado en esta sesion).
bool Visto(const char* id);

}  // namespace dbz3::ingame::tutorial
