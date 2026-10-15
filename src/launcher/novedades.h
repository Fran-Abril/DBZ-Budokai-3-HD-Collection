// dbz3 - "Novedades" del launcher: lo nuevo de cada version, en palabras de jugador (ES/EN).
//
// Al publicar una version con cambios visibles, anade un bloque ARRIBA con su numero ("1.4.6"): el
// launcher muestra el bloque mas nuevo que no supere la version instalada y lo abre solo una vez tras
// actualizar (dbz3_news_seen). Pocas lineas, sin jerga; Tag Team y Online siempre como beta.
#pragma once

namespace dbz3::launcher {

struct NewsItem {
  const char* section_es;   // nullptr = misma seccion que la linea anterior
  const char* section_en;
  const char* es;
  const char* en;
};

struct NewsBlock {
  const char* version;
  const NewsItem* items;
  int count;
};

inline constexpr NewsItem kNews145[] = {
    {"Tag Team (beta)", "Tag Team (beta)",
     "Si te sacan del ring y te queda compañero, entra él y el combate sigue.",
     "Ring out with a partner still standing: they come in and the fight goes on."},
    {nullptr, nullptr, "El que entra llega volando y carga ki con la pose del propio juego.",
     "The incoming fighter flies in and powers up with the game's own ki pose."},
    {"Online (beta)", "Online (beta)",
     "Tener mods distintos ya no corta la conexión: el panel dice qué falta y lo instala desde Nova's Mods.",
     "Different mods no longer drop the connection: the panel shows what is missing and installs it from "
     "Nova's Mods."},
    {nullptr, nullptr, "Vuelve la apertura automática del puerto del router (UPnP).",
     "Automatic router port opening (UPnP) is back."},
    {"Texturas HD", "HD textures",
     "Los packs de texturas HD cargan sin tirones y se aplican nada más pulsar JUGAR.",
     "HD texture packs load without hitches and apply as soon as you press PLAY."},
    {nullptr, nullptr, "El pack HD de SSGPrinceVegeta se instala con un clic en Nova's Mods.",
     "SSGPrinceVegeta's HD pack installs in one click from Nova's Mods."},
    {"Launcher", "Launcher",
     "Nueva pestaña Juego: Tag Team, Online y los demás modos en un solo sitio, con arranque rápido.",
     "New Game tab: Tag Team, Online and the other modes in one place, plus a quick start option."},
    {nullptr, nullptr, "Textos en español con sus tildes y eñes. Pulsa un mod para ver su ficha.",
     "Spanish texts with proper accents. Click a mod to see its info card."},
    {"Kit de modding", "Modding kit",
     "Ports oficiales con un clic: Goku GT, Vegeta GT, Pan, Baby, Janemba, Trunks espada y Gohan del Futuro.",
     "Official ports in one click: Goku GT, Vegeta GT, Pan, Baby, Janemba, sword Trunks and Future Gohan."},
};

// Del mas nuevo al mas antiguo.
inline constexpr NewsBlock kNewsBlocks[] = {
    {"1.4.5", kNews145, int(sizeof(kNews145) / sizeof(kNews145[0]))},
};

}  // namespace dbz3::launcher
