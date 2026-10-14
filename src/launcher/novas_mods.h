// dbz3 - "Nova's Mods": official mods downloaded from the project's private mods
// repository (GitHub contents API, read-only key in <exe>/novas_mods.key).
//
// The repository holds catalogo.toml ([[mod]] id, version, zip, size, tipo,
// nombre_<lang>, descripcion_<lang>, and optional autor, enlaces = ["https://..."],
// partes = [asset ids]) and the zips. A zip over GitHub's file limits (the HD texture
// pack) is uploaded as release assets in parts that are downloaded and joined in order.
// A download replaces the installed copy, which is moved to <exe>/mods_respaldo first
// (never deleted).

#pragma once

#include <string>
#include <vector>

namespace dbz3::launcher {

struct NovaMod {
  std::string id;       // folder under mods/
  std::string version;
  std::string zip;      // path inside the repository
  std::string tipo;     // personaje, campana, ...
  std::string name;     // in the launcher language (falls back to English)
  std::string description;
  long long size = 0;
  std::string autor;                  // credit shown on the card (optional)
  std::vector<std::string> enlaces;   // author's https:// pages, clickable on the card
  std::vector<long long> partes;      // release asset ids of a split zip (empty = `zip`)
};

enum class NovaState { kNoKey, kLoading, kReady, kFailed };

// Fetches the catalog once (background thread). Safe every frame.
void StartNovaCatalog();
void RefreshNovaCatalog();
NovaState GetNovaState();
std::vector<NovaMod> NovaCatalog();

// Installed version of a catalog mod ("" = not installed).
std::string NovaInstalledVersion(const std::string& id);

// Downloads and installs (or updates) one mod on a background thread.
void InstallNovaMod(const NovaMod& mod);
bool NovaBusy(const std::string& id);
// Last result line for the status bar ("" = nothing new).
std::string NovaStatus();

}  // namespace dbz3::launcher
