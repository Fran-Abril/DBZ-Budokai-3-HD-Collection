// dbz3 - Extension de ESCENARIOS (ver stage_ext.h).
//
// Banco de pruebas: redirige un registro de la tabla de escenarios a una entrada
// de data_cmn (propia o nueva) y, opcionalmente, le cambia el nombre y el TEMA
// (para que la funcion de tema encaje con la geometria cargada). Confirma en el
// juego el id_mapa -> AFS antes de reubicar las tablas para el hueco nuevo.

#include "stage_ext.h"

#include "escenarios.h"
#include "mods.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/system/xmemory.h>


namespace dbz3::stages {
namespace {

// Tabla de registros de escenario (misma direccion US y EU: .data).
constexpr uint32_t kStageTable = 0x82323760;
constexpr uint32_t kStageRec = 92;
constexpr uint32_t kStageCount = 17;
// Tabla paralela de punteros a funcion de tema (17 + terminador 0).
constexpr uint32_t kThemeTable = 0x82323D80;

struct Rec {
  std::string name;
  uint32_t id_map = 0;
  uint32_t id_se = 0;
  uint32_t variant = 0;
};

class Guest {
 public:
  explicit Guest(rex::memory::Memory* m) : m_(m) {}
  uint8_t* P(uint32_t a) const { return m_->TranslateVirtual<uint8_t*>(a); }
  uint32_t U32(uint32_t a) const {
    const uint8_t* p = P(a);
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
  }
  void Protect(uint32_t a, uint32_t n) {
    if (auto* heap = m_->LookupHeap(a)) {
      const uint32_t lo = a & ~0xFFFu, hi = (a + n + 0xFFF) & ~0xFFFu;
      if (hi > lo)
        heap->Protect(lo, hi - lo, rex::memory::kMemoryProtectRead | rex::memory::kMemoryProtectWrite);
    }
  }
  void W32(uint32_t a, uint32_t v) {
    uint8_t b[4] = {uint8_t(v >> 24), uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v)};
    Protect(a, 4);
    std::memcpy(P(a), b, 4);
  }
  void WBytes(uint32_t a, const void* src, uint32_t n) {
    Protect(a, n);
    std::memcpy(P(a), src, n);
  }

 private:
  rex::memory::Memory* m_;
};

std::string Trim(std::string s) {
  while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r')) s.erase(0, 1);
  while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) s.pop_back();
  return s;
}

std::string Unquote(std::string s) {
  if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\'')))
    return s.substr(1, s.size() - 2);
  return s;
}

}  // namespace

void ApplyAtLaunch(rex::memory::Memory* memory) {
  if (!memory) return;
  Guest g(memory);

  // Firma: el primer registro es "BUDO" en las dos imagenes.
  if (std::memcmp(g.P(kStageTable), "BUDO", 4) != 0) {
    REXLOG_INFO("dbz3 escenarios: tabla de escenarios no reconocida; sin cambios");
    return;
  }

  std::error_code ec;
  for (const auto& info : ListMods(false)) {
    if (!info.enabled) continue;
    const auto file = ModsRoot() / info.name / "stage.toml";
    if (!std::filesystem::is_regular_file(file, ec)) continue;

    std::ifstream in(file);
    std::string line;
    bool in_parche = false;
    long registro = -1, entrada = -1, id_se = -1, tema = -1;
    std::string nombre;
    while (std::getline(in, line)) {
      const auto hash = line.find('#');
      if (hash != std::string::npos) line.resize(hash);
      line = Trim(line);
      if (line.empty()) continue;
      if (line.front() == '[') {
        in_parche = line.find("[parche]") != std::string::npos;
        continue;
      }
      const auto eq = line.find('=');
      if (eq == std::string::npos || !in_parche) continue;
      const std::string k = Trim(line.substr(0, eq));
      const std::string v = Unquote(Trim(line.substr(eq + 1)));
      if (k == "registro") registro = std::strtol(v.c_str(), nullptr, 10);
      else if (k == "entrada") entrada = std::strtol(v.c_str(), nullptr, 10);
      else if (k == "id_se") id_se = std::strtol(v.c_str(), nullptr, 10);
      else if (k == "tema") tema = std::strtol(v.c_str(), nullptr, 10);
      else if (k == "nombre") nombre = v;
    }

    if (registro < 0 || registro >= int(kStageCount)) continue;
    const uint32_t rec = kStageTable + uint32_t(registro) * kStageRec;

    if (entrada > 0) {
      const uint32_t old = g.U32(rec + 0x10);
      g.W32(rec + 0x10, uint32_t(entrada));
      REXLOG_INFO("dbz3 escenarios [{}]: registro {} id_mapa {} -> {}", info.name, registro, old, entrada);
    }
    if (id_se > 0) {
      const uint32_t old = g.U32(rec + 0x14);
      g.W32(rec + 0x14, uint32_t(id_se));
      REXLOG_INFO("dbz3 escenarios [{}]: registro {} id_se {} -> {}", info.name, registro, old, id_se);
    }
    if (!nombre.empty()) {
      char buf[16] = {};
      std::strncpy(buf, nombre.c_str(), 15);
      g.WBytes(rec + 0x00, buf, 16);
      REXLOG_INFO("dbz3 escenarios [{}]: registro {} nombre -> '{}'", info.name, registro, nombre);
    }
    if (tema >= 0 && tema < int(kStageCount)) {
      const uint32_t fn = g.U32(kThemeTable + uint32_t(tema) * 4);
      g.W32(kThemeTable + uint32_t(registro) * 4, fn);
      REXLOG_INFO("dbz3 escenarios [{}]: registro {} tema -> fn[{}] = 0x{:08X}", info.name, registro, tema, fn);
    }
  }
  dbz3::escenarios::CargarDeMods(memory);   // altas: casillas nuevas ([[escenario]], escenarios.cpp)
}

}  // namespace dbz3::stages
