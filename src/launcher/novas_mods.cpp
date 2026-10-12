// dbz3 - "Nova's Mods" catalog and downloads (see novas_mods.h).

#include "novas_mods.h"

#include "../mods.h"
#include "i18n.h"
#include "update_check.h"

#include <rex/filesystem.h>
#include <rex/logging.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>

#include <toml++/toml.hpp>

namespace dbz3::launcher {

namespace {

namespace fs = std::filesystem;

constexpr const wchar_t* kHost = L"api.github.com";
constexpr const char* kRepoPath = "/repos/novapowers0/DBZ3HD-Novas-Mods/contents/";

struct Entry {
  NovaMod mod;
  std::map<std::string, std::string> names, descs;  // by language code
};

std::atomic<NovaState> g_state{NovaState::kLoading};
std::atomic<bool> g_started{false}, g_inflight{false};
std::mutex g_mu;
std::vector<Entry> g_catalog;
std::set<std::string> g_busy;
std::string g_status;

fs::path ExeDir() { return ModsRoot().parent_path(); }

// novas_mods.key: the read-only token, hex of its bytes XOR 0x5A (keeps it out of
// plain-text searches; tools/novas_mods.py clave writes it).
std::string Token() {
  std::ifstream in(ExeDir() / "novas_mods.key");
  std::string hex, out;
  in >> hex;
  if (hex.rfind("github_pat_", 0) == 0) return hex;   // pegada tal cual (sin pasar por la herramienta)
  for (size_t i = 0; i + 1 < hex.size(); i += 2) {
    if (!std::isxdigit(uint8_t(hex[i])) || !std::isxdigit(uint8_t(hex[i + 1]))) return {};   // fichero roto: sin clave
    out += char(std::stoi(hex.substr(i, 2), nullptr, 16) ^ 0x5A);
  }
  return out;
}

std::string Get(const std::string& repo_file) {
  const std::string token = Token();
  if (token.empty()) return {};
  const std::string path = kRepoPath + repo_file;
  const std::wstring wpath(path.begin(), path.end());
  const std::string h = "Authorization: Bearer " + token +
                        "\r\nAccept: application/vnd.github.raw\r\nX-GitHub-Api-Version: 2022-11-28\r\n";
  const std::wstring wh(h.begin(), h.end());
  return HttpGet(kHost, wpath.c_str(), wh.c_str());
}

std::string Pick(const std::map<std::string, std::string>& m) {
  for (const char* k : {i18n::LangCode(), "en", "es"}) {
    const auto it = m.find(k);
    if (it != m.end() && !it->second.empty()) return it->second;
  }
  return {};
}

void Fetch() {
  if (g_inflight.exchange(true)) return;
  if (Token().empty()) {
    g_state = NovaState::kNoKey;
    g_inflight = false;
    return;
  }
  g_state = NovaState::kLoading;
  std::thread([] {
    std::vector<Entry> list;
    const std::string body = Get("catalogo.toml");
    try {
      const toml::table tbl = toml::parse(body);
      if (const toml::array* mods = tbl["mod"].as_array()) {
        for (const toml::node& n : *mods) {
          const toml::table* t = n.as_table();
          if (!t) continue;
          Entry e;
          e.mod.id = (*t)["id"].value_or(std::string());
          e.mod.version = (*t)["version"].value_or(std::string());
          e.mod.zip = (*t)["zip"].value_or(std::string());
          e.mod.tipo = (*t)["tipo"].value_or(std::string());
          e.mod.size = (*t)["size"].value_or(0LL);
          for (const auto& [k, v] : *t) {
            const std::string key(k.str());
            if (key.rfind("nombre_", 0) == 0) e.names[key.substr(7)] = v.value_or(std::string());
            if (key.rfind("descripcion_", 0) == 0) e.descs[key.substr(12)] = v.value_or(std::string());
          }
          // id = nombre de carpeta (solo letras, cifras, _ y -: ni rutas ni "C:"); zip sin ?, # ni ".."
          const auto plain = [](const std::string& s) {
            return std::all_of(s.begin(), s.end(), [](char c) { return std::isalnum(uint8_t(c)) || c == '_' || c == '-'; });
          };
          if (!e.mod.id.empty() && !e.mod.zip.empty() && plain(e.mod.id) &&
              e.mod.zip.find_first_of("?#\\") == std::string::npos && e.mod.zip.find("..") == std::string::npos) {
            list.push_back(std::move(e));
          }
        }
      }
    } catch (const std::exception& ex) {
      REXLOG_WARN("dbz3: Nova's Mods: catalogo ilegible ({})", ex.what());
    }
    {
      std::lock_guard<std::mutex> lock(g_mu);
      g_catalog = std::move(list);
    }
    g_state = body.empty() || g_catalog.empty() ? NovaState::kFailed : NovaState::kReady;
    g_inflight = false;
  }).detach();
}

std::string Stamp() {
  const std::time_t now = std::time(nullptr);
  std::tm tm{};
#ifdef _WIN32
  localtime_s(&tm, &now);
#else
  localtime_r(&now, &tm);
#endif
  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm);
  return buf;
}

void SetStatus(const std::string& s) {
  std::lock_guard<std::mutex> lock(g_mu);
  g_status = s;
}

}  // namespace

void StartNovaCatalog() {
  if (!g_started.exchange(true)) Fetch();
}

void RefreshNovaCatalog() { Fetch(); }

NovaState GetNovaState() { return g_state.load(); }

std::vector<NovaMod> NovaCatalog() {
  std::lock_guard<std::mutex> lock(g_mu);
  std::vector<NovaMod> out;
  for (const Entry& e : g_catalog) {
    NovaMod m = e.mod;
    m.name = Pick(e.names);
    if (m.name.empty()) m.name = m.id;
    m.description = Pick(e.descs);
    out.push_back(std::move(m));
  }
  return out;
}

std::string NovaInstalledVersion(const std::string& id) {
  std::error_code ec;
  for (const std::string& dir : {id, id + ".disabled"}) {
    if (fs::is_directory(ModsRoot() / dir, ec)) {
      const std::string v = GetModManifestValue(id, "version");
      return v.empty() ? "?" : v;
    }
  }
  return {};
}

bool NovaBusy(const std::string& id) {
  std::lock_guard<std::mutex> lock(g_mu);
  return g_busy.count(id) > 0;
}

std::string NovaStatus() {
  std::lock_guard<std::mutex> lock(g_mu);
  return g_status;
}

void InstallNovaMod(const NovaMod& mod) {
  {
    std::lock_guard<std::mutex> lock(g_mu);
    if (!g_busy.insert(mod.id).second) return;
    g_status = std::string(i18n::T("Descargando ", "Downloading ")) + mod.name + "...";
  }
  std::thread([mod] {
    const std::string data = Get(mod.zip);
    std::error_code ec;
    std::string error;
    // un zip empieza por "PK": una respuesta de error de GitHub no se instala
    if (data.size() < 4 || data.compare(0, 2, "PK") != 0 || (mod.size > 0 && (long long)data.size() != mod.size)) {
      error = i18n::T("no se pudo descargar (revisa tu conexion)", "download failed (check your connection)");
    } else {
      const fs::path tmp_dir = ExeDir() / "novas_tmp";
      fs::create_directories(tmp_dir, ec);
      const fs::path zip = tmp_dir / (mod.id + ".zip");
      {
        std::ofstream out(zip, std::ios::binary);
        out.write(data.data(), std::streamsize(data.size()));
      }
      // la copia instalada pasa a mods_respaldo (nunca se borra) y se respeta si estaba desactivada
      bool was_disabled = false;
      std::vector<std::pair<fs::path, fs::path>> moved;   // (copia apartada, sitio original)
      for (const std::string& dir : {mod.id, mod.id + ".disabled"}) {
        const fs::path old = ModsRoot() / dir;
        if (!fs::is_directory(old, ec)) continue;
        was_disabled = was_disabled || dir != mod.id || fs::exists(old / ".disabled", ec);
        const fs::path bak = ExeDir() / "mods_respaldo" / (dir + "_" + Stamp());
        fs::create_directories(bak.parent_path(), ec);
        fs::rename(old, bak, ec);
        if (ec) error = i18n::T("no se pudo apartar la version instalada", "could not move the installed version aside");
        else moved.emplace_back(bak, old);
      }
      std::string name;
      if (error.empty() && !InstallModFromZip(rex::path_to_utf8(zip), name, error)) {
        error = i18n::T("el zip no se pudo instalar", "the zip could not be installed");
      }
      if (!error.empty()) {   // fallo: lo copiado a medias se aparta y la version que habia vuelve a su sitio
        for (const auto& [bak, old] : moved) {
          if (fs::exists(old, ec)) fs::rename(old, ExeDir() / "mods_respaldo" / (old.filename().string() + "_fallido_" + Stamp()), ec);
          fs::rename(bak, old, ec);
          if (ec) REXLOG_WARN("dbz3: Nova's Mods: no se pudo devolver {} (sigue en mods_respaldo)", rex::path_to_utf8(old));
        }
      }
      fs::remove(zip, ec);
      if (error.empty() && was_disabled) SetModEnabled(name, false);
    }
    REXLOG_INFO("dbz3: Nova's Mods: {} {} -> {}", mod.id, mod.version, error.empty() ? "instalado" : error);
    std::lock_guard<std::mutex> lock(g_mu);
    g_busy.erase(mod.id);
    g_status = error.empty() ? mod.name + i18n::T(": instalado. Se aplica al pulsar JUGAR.", ": installed. It applies when you press PLAY.")
                             : mod.name + ": " + error;
  }).detach();
}

}  // namespace dbz3::launcher
