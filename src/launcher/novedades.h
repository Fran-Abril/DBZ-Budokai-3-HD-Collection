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

inline constexpr NewsItem kNews151[] = {
    {"Budokai 2 (experimental)", "Budokai 2 (experimental)",
     "El Mundo Dragón arranca con su intro y las charlas del original; los Saibaman brotan del suelo y suena su música.",
     "Dragon World starts with its intro and the original dialogues; Saibamen sprout from the ground and its music plays."},
    {nullptr, nullptr, "Saibaman de colores: cada combate del tablero contra uno sale de un color.",
     "Coloured Saibamen: each board battle against one has a different colour."},
    {nullptr, nullptr,
     "Tablero más fiel: flecha roja, columna de vida, retratos, sonidos, música y escenarios de Budokai 2 en cada casilla.",
     "More faithful board: red arrow, life column, portraits, sounds, music and Budokai 2 stages on every tile."},
    {nullptr, nullptr, "Nave de Babidi (beta): pantallas y pruebas como en el original.",
     "Babidi's ship (beta): screens and trials as in the original."},
    {"Infinite World (experimental)", "Infinite World (experimental)",
     "El entrenamiento de Goku y Gohan ya no se descoloca, y las misiones dicen qué personaje falta.",
     "The Goku and Gohan training no longer glitches, and missions tell you which character is missing."},
    {nullptr, nullptr, "Pack Infinite World en Nova's Mods: marca los personajes y se crean con tu copia del juego.",
     "Infinite World pack in Nova's Mods: tick the characters and they are built from your game copy."},
    {nullptr, nullptr, "Mapa con bordes, música por zona y animaciones más suaves.",
     "Map with borders, music per area and smoother animations."},
    {"Modo Héroe (beta)", "Hero mode (beta)", "Cambia el moveset de tu héroe dentro del juego.",
     "Change your hero's moveset inside the game."},
    {"Mods", "Mods",
     "Janemba y las Fuerzas Ginyu en Nova's Mods con sus propias voces, y huecos de personaje que se conservan entre versiones.",
     "Janemba and the Ginyu Force in Nova's Mods with their own voices, and character slots kept between versions."},
    {"Launcher", "Launcher",
     "Arreglado: en algunos equipos no cargaba Nova's Mods ni la comprobación de versión.",
     "Fixed: on some PCs Nova's Mods and the update check did not load."},
    {nullptr, nullptr, "Nuevos idiomas: portugués (Brasil) y japonés.",
     "New languages: Portuguese (Brazil) and Japanese."},
    {nullptr, nullptr, "Arranca antes tras actualizar: los retratos de los personajes ya no se vuelven a dibujar.",
     "Starts faster after updating: character portraits are no longer redrawn."},
};

inline constexpr NewsItem kNews150[] = {
    {"Dragon HUB", "Dragon HUB",
     "Placas de rango, fondos y anfitriones nuevos, y marcas BETA / EXPERIMENTAL en lo que está en construcción.",
     "Rank plates, new backgrounds and hosts, and BETA / EXPERIMENTAL labels on what is still being built."},
    {"Tag Team (beta)", "Tag Team (beta)", "Relevos con teletransporte, también entre luchadores del ordenador.",
     "Teleport relays, also between computer fighters."},
    {"Mods", "Mods", "Los mods ya funcionan jugando directamente desde la ISO.",
     "Mods now work when playing straight from the ISO."},
    {"Modos nuevos", "New modes",
     "Misión 100, Modo Héroe (beta), Leyenda de Mr. Satán (beta) y las campañas de Infinite World y Budokai 2 "
     "(experimental): elige tu ISO de PS2 en la pestaña Juego.",
     "Mission 100, Hero mode (beta), Mr. Satan legend (beta) and the Infinite World and Budokai 2 campaigns "
     "(experimental): pick your PS2 ISO in the Game tab."},
    {"Launcher", "Launcher", "Lanzador y menú F1 renovados, y todo traducido a cinco idiomas.",
     "Renewed launcher and F1 menu, and everything translated into five languages."},
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
    {"1.5.1", kNews151, int(sizeof(kNews151) / sizeof(kNews151[0]))},
    {"1.5.0", kNews150, int(sizeof(kNews150) / sizeof(kNews150[0]))},
    {"1.4.5", kNews145, int(sizeof(kNews145) / sizeof(kNews145[0]))},
};

}  // namespace dbz3::launcher
