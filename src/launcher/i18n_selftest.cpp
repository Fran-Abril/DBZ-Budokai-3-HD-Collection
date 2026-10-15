// Comprobacion de i18n::T: tildes del espanol y tabla IT/DE/FR. No entra en el build del juego.
//   clang++ -std=c++20 -finput-charset=UTF-8 -o i18n_selftest.exe src/launcher/i18n_selftest.cpp && i18n_selftest
#include "i18n.cpp"

#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
  using dbz3::i18n::T;
  dbz3::i18n::SetLanguage(5);
  struct Case { const char* in; const char* want; } cases[] = {
      {"Reparar instalacion", "Reparar instalación"},
      {"Carpeta extraida", "Carpeta extraída"},
      {"Restablecer todos los ajustes? Se perderan tus teclas y se desactivaran los mods.",
       "¿Restablecer todos los ajustes? Se perderán tus teclas y se desactivarán los mods."},
      {"Si, restablecer", "Sí, restablecer"},
      {"Ya esta en el equipo", "Ya está en el equipo"},
      {"Quitar esta capsula del personaje?", "¿Quitar esta cápsula del personaje?"},
      {"Tu partida se sustituira por esta. Antes se hace una copia. Continuar?",
       "Tu partida se sustituirá por esta. Antes se hace una copia. ¿Continuar?"},
      {"MENU PRINCIPAL", "MENÚ PRINCIPAL"},
      {"Ultima version", "Última versión"},
      {"HD Collection - ReXGlue", "HD Collection - ReXGlue"},
      {"dbz3_region y la region", "dbz3_region y la región"},
      {"%d mod(s) con problemas: pulsa la pestana", "%d mod(s) con problemas: pulsa la pestaña"},
      {"Goku (nino)", "Goku (niño)"},
      {"Selector de transformacion (beta)", "Selector de transformación (beta)"},
      {"SELECT cambia al compañero.", "SELECT cambia al compañero."},
      {"esta partida", "esta partida"},
      {"El audio esta silenciado.", "El audio está silenciado."},
      {"estas en modo disco", "estás en modo disco"},
      {"Espanol", "Español"},
      {"Tamano no valido en ", "Tamaño no válido en "},
  };
  int bad = 0;
  for (const auto& c : cases) {
    const char* got = T(c.in, "en");
    if (std::strcmp(got, c.want) != 0) {
      std::printf("MAL: '%s' -> '%s' (esperado '%s')\n", c.in, got, c.want);
      ++bad;
    }
  }
  // mismo puntero en la segunda llamada (cache) y el texto sin cambios devuelve el original
  const char* plain = "Video";
  assert(T(plain, "Video") == plain);
  assert(T("Reparar instalacion", "x") == T("Reparar instalacion", "y"));
  dbz3::i18n::SetLanguage(6);
  assert(std::strcmp(T("Instalar", "Install"), "Installa") == 0);
  assert(std::strcmp(T("no existe en la tabla", "fallback"), "fallback") == 0);
  std::printf(bad ? "%d fallos\n" : "OK\n", bad);
  return bad != 0;
}
