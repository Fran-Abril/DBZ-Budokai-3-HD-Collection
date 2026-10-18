// dbz3 - Mods opcionales: lector comun (ver mods_opcionales.h). Solo lee ficheros del jugador (mods/ y su ISO).
#include "mods_opcionales.h"

#include "launcher/settings.h"
#include "mods.h"
#include "pasarela.h"

#include <rex/filesystem.h>
#include <rex/logging.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>

namespace dbz3::opcional {
namespace {

bool EndsWith(const std::string& s, const std::string& suf) {
  return s.size() >= suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
}

// id logico por convencion: "opc_b2_musica_combate" -> "b2.musica_combate" (juego.resto). El toml puede
// fijarlo con [opcional] id = "..."; si no, se usa la carpeta tal cual cuando no sigue la convencion.
std::string IdDeCarpeta(const std::string& carpeta) {
  std::string s = carpeta;
  if (s.rfind("opc_", 0) == 0) s.erase(0, 4);
  const size_t guion = s.find('_');
  if (guion != std::string::npos && guion > 0) s = s.substr(0, guion) + "." + s.substr(guion + 1);
  return s;
}

double AhoraMs() {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

std::mutex g_mu;
std::shared_ptr<const std::vector<Mod>> g_cache;
double g_cache_ms = -1e18;

toml::table LeerToml(const std::filesystem::path& p) {
  try {
    return toml::parse_file(rex::path_to_utf8(p));
  } catch (const std::exception& ex) {
    REXLOG_WARN("dbz3 opcional: {} no se pudo leer: {}", rex::path_to_utf8(p), ex.what());
    return {};
  }
}

std::string CarpetasManifest(const std::filesystem::path& dir, const std::string& clave) {
  std::ifstream in(dir / "manifest.txt");
  std::string line;
  while (std::getline(in, line)) {
    while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
    const size_t eq = line.find('=');
    if (eq == std::string::npos || line.compare(0, eq, clave) != 0 || line.size() <= eq) continue;
    std::string v = line.substr(eq + 1);
    while (!v.empty() && (v.front() == ' ' || v.front() == '\t')) v.erase(v.begin());
    return v;
  }
  return {};
}

// Mod con ese id logico (o nombre de carpeta); `keep` mantiene vivo el vector de la cache mientras se usa.
const Mod* Encontrar(const std::string& id, std::shared_ptr<const std::vector<Mod>>& keep) {
  keep = Listar();
  for (const Mod& m : *keep) {
    if (m.id == id || m.carpeta == id) return &m;
  }
  return nullptr;
}

// Nodo del opcional.toml: `clave` (admite puntos) en la raiz y, si no esta, dentro de [opcional].
const toml::node* Nodo(const Mod& m, const std::string& clave) {
  auto navegar = [&](const toml::table& raiz) -> const toml::node* {
    const toml::node* n = &raiz;
    size_t p = 0;
    while (p <= clave.size()) {
      const size_t d = clave.find('.', p);
      const std::string k = clave.substr(p, d == std::string::npos ? std::string::npos : d - p);
      const toml::table* t = n ? n->as_table() : nullptr;
      if (!t) return nullptr;
      n = t->get(k);
      if (d == std::string::npos) return n;
      p = d + 1;
    }
    return nullptr;
  };
  if (const toml::node* n = navegar(m.datos)) return n;
  if (const toml::table* op = m.datos.get("opcional") ? m.datos.get("opcional")->as_table() : nullptr) return navegar(*op);
  return nullptr;
}

// ---- ISO9660 de solo lectura (extraer un fichero de la copia del jugador; nada se reparte) ----
constexpr uint32_t kSector = 2048;

uint32_t U32le(const std::vector<uint8_t>& b, size_t o) {
  return o + 4 <= b.size() ? uint32_t(b[o]) | uint32_t(b[o + 1]) << 8 | uint32_t(b[o + 2]) << 16 | uint32_t(b[o + 3]) << 24
                           : 0;
}
std::string Mayus(std::string s) {
  for (char& c : s) c = char(std::toupper(static_cast<unsigned char>(c)));
  return s;
}
std::vector<uint8_t> LeerIso(std::ifstream& f, uint64_t off, uint32_t n) {
  std::vector<uint8_t> b(n);
  f.clear();
  f.seekg(std::streamoff(off));
  f.read(reinterpret_cast<char*>(b.data()), std::streamsize(n));
  b.resize(size_t(std::max<std::streamsize>(0, f.gcount())));
  return b;
}
// Baja por el arbol comparando la ruta en mayusculas; `quiero` = "USR/MOVIE_EU.SFD".
bool IsoBuscar(std::ifstream& f, uint32_t lba, uint32_t tam, const std::string& quiero, const std::string& prefijo,
               int prof, uint32_t& out_lba, uint32_t& out_tam) {
  if (prof > 8 || tam > (64u << 20)) return false;
  const std::vector<uint8_t> d = LeerIso(f, uint64_t(lba) * kSector, (tam + kSector - 1) / kSector * kSector);
  for (size_t o = 0; o + 33 <= d.size();) {
    const uint8_t len = d[o];
    if (len == 0) {   // el resto del sector va vacio
      o = (o / kSector + 1) * kSector;
      continue;
    }
    if (o + len > d.size()) break;
    const uint32_t elba = U32le(d, o + 2), etam = U32le(d, o + 10);
    const bool dir = (d[o + 25] & 2) != 0;
    std::string n(reinterpret_cast<const char*>(&d[o + 33]), std::min<size_t>(d[o + 32], d.size() - o - 33));
    o += len;
    if (n.empty() || n[0] == '\0' || n[0] == '\1') continue;
    if (const size_t s = n.find(';'); s != std::string::npos) n.resize(s);
    const std::string full = prefijo.empty() ? Mayus(n) : prefijo + "/" + Mayus(n);
    if (full == quiero) {
      if (dir) return false;
      out_lba = elba;
      out_tam = etam;
      return true;
    }
    if (dir && quiero.rfind(full + "/", 0) == 0 &&
        IsoBuscar(f, elba, etam, quiero, full, prof + 1, out_lba, out_tam))
      return true;
  }
  return false;
}
bool ExtraerDeIso(const std::filesystem::path& iso, const std::string& ruta, const std::filesystem::path& destino) {
  std::ifstream f(iso, std::ios::binary);
  if (!f) return false;
  const std::vector<uint8_t> pvd = LeerIso(f, uint64_t(16) * kSector, kSector);
  if (pvd.size() < 2048 || std::memcmp(pvd.data() + 1, "CD001", 5) != 0) return false;
  const uint32_t raiz_lba = U32le(pvd, 156 + 2), raiz_tam = U32le(pvd, 156 + 10);
  std::string quiero = Mayus(ruta);
  for (char& c : quiero)
    if (c == '\\') c = '/';
  uint32_t lba = 0, tam = 0;
  if (!IsoBuscar(f, raiz_lba, raiz_tam, quiero, "", 0, lba, tam) || !tam) return false;
  std::filesystem::path parcial = destino;
  parcial += ".parcial";   // y al final un rename: nunca queda un fichero a medias con el nombre bueno
  {
    std::ofstream out(parcial, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    for (uint64_t off = 0; off < tam;) {
      const uint32_t n = uint32_t(std::min<uint64_t>(1u << 20, tam - off));
      const std::vector<uint8_t> b = LeerIso(f, uint64_t(lba) * kSector + off, n);
      if (b.empty()) return false;
      out.write(reinterpret_cast<const char*>(b.data()), std::streamsize(b.size()));
      off += b.size();
    }
    if (!out.good()) return false;
  }
  std::error_code ec;
  std::filesystem::rename(parcial, destino, ec);
  REXLOG_INFO("dbz3 opcional: extraido {} de la ISO a {} ({} bytes)", ruta, rex::path_to_utf8(destino), tam);
  return !ec;
}

// PRUEBAS (tools/pruebas.py): DBZ3_TEST_OPCIONAL="id1,id2" los da por activos sin tocar mods/.
bool Forzado(const std::string& id) {
  const char* t = std::getenv("DBZ3_TEST_OPCIONAL");
  if (!t) return false;
  const std::string s = "," + std::string(t) + ",";
  return s.find("," + id + ",") != std::string::npos;
}

}  // namespace

std::shared_ptr<const std::vector<Mod>> Listar() {
  const double ms = AhoraMs();
  {
    std::lock_guard<std::mutex> lk(g_mu);
    if (g_cache && ms - g_cache_ms < 2000) return g_cache;
  }
  auto out = std::make_shared<std::vector<Mod>>();
  std::error_code ec;
  for (auto [it, iec] = std::pair{std::filesystem::directory_iterator(ModsRoot(), ec), std::error_code{}};
       !iec && it != std::filesystem::directory_iterator(); it.increment(iec)) {
    const std::string carpeta = rex::path_to_utf8(it->path().filename());
    if (carpeta.empty() || carpeta[0] == '.' || !it->is_directory(ec)) continue;
    const std::filesystem::path toml = it->path() / "opcional.toml";
    if (!std::filesystem::exists(toml, ec)) continue;   // no es un mod opcional
    Mod m;
    m.carpeta = carpeta;
    m.datos = LeerToml(toml);
    m.textos = LeerToml(it->path() / "textos.toml");
    m.activo = !EndsWith(carpeta, ".disabled") && !std::filesystem::exists(it->path() / ".disabled", ec);
    m.id = IdDeCarpeta(carpeta);
    if (const toml::table* op = m.datos.get("opcional") ? m.datos.get("opcional")->as_table() : nullptr)
      if (const toml::node* id = op->get("id"))
        if (const toml::value<std::string>* s = id->as_string()) m.id = s->get();
    out->push_back(std::move(m));
  }
  std::sort(out->begin(), out->end(), [](const Mod& a, const Mod& b) { return a.carpeta < b.carpeta; });
  {
    std::lock_guard<std::mutex> lk(g_mu);
    g_cache = out;
    g_cache_ms = ms;
  }
  return out;
}

void Refrescar() {
  std::lock_guard<std::mutex> lk(g_mu);
  g_cache_ms = -1e18;
}

bool Activo(const std::string& id) {
  if (Forzado(id)) return true;
  std::shared_ptr<const std::vector<Mod>> keep;
  const Mod* m = Encontrar(id, keep);
  return m && m->activo;
}

bool Desactivado(const std::string& id) {
  if (Forzado(id)) return false;
  std::shared_ptr<const std::vector<Mod>> keep;
  const Mod* m = Encontrar(id, keep);
  return m && !m->activo;
}

long long Entero(const std::string& id, const std::string& clave, long long defecto) {
  std::shared_ptr<const std::vector<Mod>> keep;
  const Mod* m = Encontrar(id, keep);
  const toml::node* n = m ? Nodo(*m, clave) : nullptr;
  if (const toml::value<int64_t>* v = n ? n->as_integer() : nullptr) return v->get();
  return defecto;
}

std::string Texto(const std::string& id, const std::string& clave, const std::string& defecto) {
  std::shared_ptr<const std::vector<Mod>> keep;
  const Mod* m = Encontrar(id, keep);
  const toml::node* n = m ? Nodo(*m, clave) : nullptr;
  if (const toml::value<std::string>* v = n ? n->as_string() : nullptr) return v->get();
  return defecto;
}

std::vector<int> Lista(const std::string& id, const std::string& clave) {
  std::vector<int> out;
  std::shared_ptr<const std::vector<Mod>> keep;
  const Mod* m = Encontrar(id, keep);
  const toml::node* n = m ? Nodo(*m, clave) : nullptr;
  const toml::array* a = n ? n->as_array() : nullptr;
  if (!a) return out;
  for (const toml::node& e : *a)
    if (const toml::value<int64_t>* v = e.as_integer()) out.push_back(int(v->get()));
  return out;
}

std::vector<std::string> Claves(const std::string& id, const std::string& tabla) {
  std::vector<std::string> out;
  std::shared_ptr<const std::vector<Mod>> keep;
  const Mod* m = Encontrar(id, keep);
  const toml::node* n = m ? Nodo(*m, tabla) : nullptr;
  const toml::table* t = n ? n->as_table() : nullptr;
  if (!t) return out;
  for (auto&& [k, v] : *t) {
    (void)v;
    out.push_back(std::string(k.str()));
  }
  return out;
}

std::string Nombre(const Mod& m, const std::string& idioma) {
  const toml::node* n = m.textos.get("nombre_" + idioma);
  if (const toml::value<std::string>* v = n ? n->as_string() : nullptr) return v->get();
  std::string v = CarpetasManifest(ModsRoot() / m.carpeta, "name");
  return v.empty() ? m.carpeta : v;
}

std::string Descripcion(const Mod& m, const std::string& idioma) {
  const toml::node* n = m.textos.get("descripcion_" + idioma);
  if (const toml::value<std::string>* v = n ? n->as_string() : nullptr) return v->get();
  return CarpetasManifest(ModsRoot() / m.carpeta, "description");
}

std::filesystem::path ArchivoDeJuegoPs2(const std::string& juego, const std::string& ruta) {
  const std::string clave = juego == "b2" ? "budokai 2" : juego == "iw" ? "infinite world" : juego;
  const std::filesystem::path j = pasarela::BuscarJuego(clave);
  if (j.empty()) {
    REXLOG_WARN("dbz3 opcional: sin copia de {} en ps2_games (falta la ISO para {})", juego, ruta);
    return {};
  }
  std::error_code ec;
  if (std::filesystem::is_directory(j, ec)) {   // carpeta extraida: el fichero tal cual
    const std::filesystem::path p = j / std::filesystem::path(ruta);
    if (std::filesystem::is_regular_file(p, ec)) return p;
    REXLOG_WARN("dbz3 opcional: {} no esta en la carpeta {}", ruta, rex::path_to_utf8(j));
    return {};
  }
  // ISO: extraer a la cache (una vez; la ISO no se toca)
  const std::filesystem::path cache = dbz3::settings::UserDataRoot() / "cache_ps2";
  const std::filesystem::path destino = cache / std::filesystem::path(juego) / std::filesystem::path(ruta).filename();
  if (std::filesystem::is_regular_file(destino, ec)) return destino;
  std::filesystem::create_directories(cache, ec);
  if (!ExtraerDeIso(j, ruta, destino)) {
    REXLOG_WARN("dbz3 opcional: no se pudo extraer {} de {}", ruta, rex::path_to_utf8(j));
    return {};
  }
  return std::filesystem::is_regular_file(destino, ec) ? destino : std::filesystem::path();
}

}  // namespace dbz3::opcional
