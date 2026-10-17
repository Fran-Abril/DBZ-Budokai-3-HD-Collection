// dbz3 - Pasarela de modos: lectura de la ISO de Budokai 1 (PS2) del jugador, en tiempo de ejecucion y sin extraerla.
// Misma logica que "mod center hd/pasarela/pasarela.py" (referencia reproducible; docs/16_pasarela_modos/):
//   - ISO 9660: SYSTEM.CNF (BOOT2) -> ELF de arranque; /USR/DATA_<idioma>.AFS.
//   - ELF: 11 registros de 128 B del modo de Mr. Satan (firma: personaje 21 en +0x20, modo 3 en +0x70, float 1.0
//     en +0x0C de los 11); nombres de escenario (registros de 0x98 B, "CELL RING").
//   - Historia: los registros de 128 B de antes de esa tabla (misma estructura, modo 0/6/7, +0x28 capitulo; europea:
//     43 desde 0x35CA40), hacia atras mientras sean validos.
//   - AFS: banco de charlas = #AMT de 22 imagenes tras #AMA, #AMT(10), #AMA, #AMT(26) (europea: 665-669); el #AMT(26)
//     es el resultado (tablon, "Challenger Nth", puntos, cifras, caras) y el #AMT(10) el titulo (dificultades).
//   - #AMT PS2: entradas de 0x30 (+0x08 PSM 0x14/0x13, +0x10 ancho/alto u16, +0x14 datos, +0x18 tamano, +0x24 CLUT,
//     +0x28 tamano); cabecera GIF de 32 B delante de datos y CLUT; 4 bpp lineal (nibble bajo = pixel par); CLUT RGBA
//     con alfa 0..0x80; la de 256 en orden CSM1.
// Nada de esto se guarda en disco ni va en la version publicada.
#include "pasarela.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <mutex>
#include <system_error>
#include <thread>

#include <rex/logging.h>
#include <rex/platform.h>

#include "launcher/settings.h"
#include "launcher/i18n.h"
#include "launcher/launcher_state.h"

#if REX_PLATFORM_WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

namespace dbz3::pasarela {
namespace {

constexpr uint32_t kSector = 2048;
constexpr int kSatan = 21, kModoSatan = 3, kRec = 128, kCombates = 11;
// B1 -> B3 (importar.B1_DONOR)
constexpr int kB1aB3[] = {0, 2, 3, 7, 10, 8, 11, 12, 13, 18, 19, 20, 21, 38, 19, 27, 28, 29, 30, 32, 33, 14, 42, 43, 5};

std::string Lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return char(std::tolower(c)); });
  return s;
}
uint32_t U32(const std::vector<uint8_t>& b, size_t o) {
  return o + 4 <= b.size() ? uint32_t(b[o]) | uint32_t(b[o + 1]) << 8 | uint32_t(b[o + 2]) << 16 | uint32_t(b[o + 3]) << 24
                           : 0;
}
uint16_t U16(const std::vector<uint8_t>& b, size_t o) {
  return o + 2 <= b.size() ? uint16_t(b[o] | b[o + 1] << 8) : 0;
}

// ISO 9660 de solo lectura (rutas en minusculas, sin ";1").
class Iso {
 public:
  bool Open(const std::filesystem::path& p) {
    f_.open(p, std::ios::binary);
    if (!f_) return false;
    std::vector<uint8_t> pvd = Read(uint64_t(16) * kSector, kSector);
    if (pvd.size() < 190 || std::memcmp(&pvd[1], "CD001", 5) != 0) return false;
    Walk(U32(pvd, 156 + 2), U32(pvd, 156 + 10), "", 0);
    return !files_.empty();
  }
  // ruta sin distinguir mayusculas; `name` puede ser solo el nombre del fichero
  std::string Find(const std::string& name) const {
    const std::string n = Lower(name[0] == '/' ? name : "/" + name);
    if (files_.count(n)) return n;
    for (const auto& [p, v] : files_) {
      if (p.size() >= n.size() && p.compare(p.size() - n.size(), n.size(), n) == 0) return p;
    }
    return {};
  }
  std::pair<uint32_t, uint32_t> Info(const std::string& p) const {
    auto it = files_.find(p);
    return it == files_.end() ? std::pair<uint32_t, uint32_t>{0, 0} : it->second;
  }
  std::vector<uint8_t> ReadFile(const std::string& p, uint64_t off, uint32_t n) {
    const auto [lba, size] = Info(p);
    if (!lba || off >= size) return {};
    return Read(uint64_t(lba) * kSector + off, uint32_t(std::min<uint64_t>(n, size - off)));
  }
  std::vector<uint8_t> Read(uint64_t off, uint32_t n) {
    std::vector<uint8_t> b(n);
    f_.clear();
    f_.seekg(std::streamoff(off));
    f_.read(reinterpret_cast<char*>(b.data()), std::streamsize(n));
    b.resize(size_t(std::max<std::streamsize>(0, f_.gcount())));
    return b;
  }

 private:
  void Walk(uint32_t lba, uint32_t size, const std::string& prefix, int depth) {
    if (depth > 8 || size > (16u << 20)) return;
    const std::vector<uint8_t> d = Read(uint64_t(lba) * kSector, (size + kSector - 1) / kSector * kSector);
    for (size_t o = 0; o < d.size();) {
      const uint8_t len = d[o];
      if (len == 0) {
        o = (o / kSector + 1) * kSector;
        continue;
      }
      if (o + 33 > d.size()) break;
      const uint32_t elba = U32(d, o + 2), esize = U32(d, o + 10);
      const uint8_t flags = d[o + 25], nlen = d[o + 32];
      std::string name(reinterpret_cast<const char*>(&d[o + 33]), std::min<size_t>(nlen, d.size() - o - 33));
      o += len;
      if (name.empty() || name[0] == '\0' || name[0] == '\1') continue;
      if (const size_t semi = name.find(';'); semi != std::string::npos) name.resize(semi);
      const std::string full = prefix + "/" + Lower(name);
      if (flags & 2) Walk(elba, esize, full, depth + 1);
      else files_[full] = {elba, esize};
    }
  }
  std::ifstream f_;
  std::map<std::string, std::pair<uint32_t, uint32_t>> files_;
};

std::string BootElf(Iso& iso) {
  const std::string cnf_path = iso.Find("system.cnf");
  const std::vector<uint8_t> cnf = iso.ReadFile(cnf_path, 0, 512);
  const std::string text(cnf.begin(), cnf.end());
  for (size_t p = 0; (p = Lower(text).find("boot2", p)) != std::string::npos; ++p) {
    size_t e = text.find('=', p);
    if (e == std::string::npos) break;
    std::string name = text.substr(e + 1, text.find_first_of("\r\n", e) - e - 1);
    if (const size_t c = Lower(name).find("cdrom0:"); c != std::string::npos) name = name.substr(c + 7);
    name.erase(0, name.find_first_not_of(" \\/"));
    if (const size_t semi = name.find(';'); semi != std::string::npos) name.resize(semi);
    while (!name.empty() && name.back() == ' ') name.pop_back();
    return iso.Find(name);
  }
  return {};
}

CombateB1 Registro(const std::vector<uint8_t>& elf, size_t o, const std::vector<std::string>& esc) {
  CombateB1 c;
  c.jugador_b1 = elf[o + 0x20];
  c.forma_jugador = elf[o + 0x22];
  c.vida_jugador = U16(elf, o + 0x2A);
  c.ki_jugador = U16(elf, o + 0x2C);
  c.capitulo = int16_t(U16(elf, o + 0x28));
  c.modo = elf[o + 0x70];
  c.rival_b1 = elf[o + 0x50];
  c.rival = B1aB3(c.rival_b1);
  c.variante = elf[o + 0x51];
  c.forma = elf[o + 0x52];
  c.cantidad = std::max(1, int(U16(elf, o + 0x56)));
  c.arrastra_vida = c.vida_jugador == 0xFFFF;
  c.vida_rival = U16(elf, o + 0x5A);
  c.ki_rival = U16(elf, o + 0x5C);
  c.escenario_b1 = elf[o + 0x71];
  if (c.escenario_b1 < int(esc.size())) c.escenario_nombre = esc[size_t(c.escenario_b1)];
  c.musica = elf[o + 0x72];
  c.tiempo = int32_t(U32(elf, o + 0x74));
  return c;
}

int TablaSatan(const std::vector<uint8_t>& elf) {
  for (size_t p = 0; p + size_t(kRec) * kCombates <= elf.size(); p += 16) {
    bool ok = true;
    for (int k = 0; k < kCombates && ok; ++k) {
      const size_t o = p + size_t(kRec) * k;
      ok = elf[o + 0x20] == kSatan && elf[o + 0x70] == kModoSatan && elf[o + 0x0F] == 0x3F;
    }
    if (ok) return int(p);
  }
  return -1;
}

std::vector<std::string> Escenarios(const std::vector<uint8_t>& elf) {
  auto nombre = [&](size_t i) {
    std::string n;
    for (size_t k = i; k < i + 16 && k < elf.size() && elf[k]; ++k) {
      if (elf[k] < 32 || elf[k] >= 127) return std::string();
      n += char(elf[k]);
    }
    return n;
  };
  static const char kCell[] = "CELL RING";
  auto it = std::search(elf.begin(), elf.end(), kCell, kCell + sizeof(kCell));   // con el 0 final
  if (it == elf.end()) return {};
  size_t i = size_t(it - elf.begin());
  while (i >= 0x98 && nombre(i - 0x98).size() >= 3) i -= 0x98;
  std::vector<std::string> out;
  for (std::string n; (n = nombre(i)).size() >= 2; i += 0x98) out.push_back(n);
  return out;
}

struct Afs {
  Iso* iso = nullptr;
  std::string path;
  std::vector<std::pair<uint32_t, uint32_t>> tab;
  bool Open(Iso& i, const std::string& p) {
    iso = &i;
    path = p;
    const std::vector<uint8_t> h = i.ReadFile(p, 0, 8);
    if (h.size() < 8 || std::memcmp(h.data(), "AFS", 3) != 0) return false;
    const uint32_t n = std::min<uint32_t>(U32(h, 4), 65536);
    const std::vector<uint8_t> t = i.ReadFile(p, 8, 8 * n);
    for (uint32_t k = 0; k + 1 <= n && 8 * k + 8 <= t.size(); ++k) tab.push_back({U32(t, 8 * k), U32(t, 8 * k + 4)});
    return !tab.empty();
  }
  std::vector<uint8_t> Entry(size_t k, uint32_t n = 0xFFFFFFFF) {
    if (k >= tab.size() || !tab[k].second) return {};
    return iso->ReadFile(path, tab[k].first, std::min(n, tab[k].second));
  }
};

int AmtN(const std::vector<uint8_t>& h) {
  return h.size() >= 0x14 && std::memcmp(h.data(), "#AMT", 4) == 0 ? int(U32(h, 0x10)) : -1;
}
bool EsAma(const std::vector<uint8_t>& h) { return h.size() >= 4 && std::memcmp(h.data(), "#AMA", 4) == 0; }

int BancoCharlas(Afs& afs) {
  auto ok = [&](size_t k) {
    if (k < 4 || k >= afs.tab.size()) return false;
    return EsAma(afs.Entry(k - 4, 0x20)) && AmtN(afs.Entry(k - 3, 0x20)) == 10 && EsAma(afs.Entry(k - 2, 0x20)) &&
           AmtN(afs.Entry(k - 1, 0x20)) == 26 && AmtN(afs.Entry(k, 0x20)) == 22;
  };
  if (ok(669)) return 669;
  for (size_t k = 4; k < afs.tab.size(); ++k) {
    if (AmtN(afs.Entry(k, 0x20)) == 22 && ok(k)) return int(k);
  }
  return -1;
}

Imagen Textura(const std::vector<uint8_t>& amt, int j) {
  Imagen img;
  const uint32_t n = U32(amt, 0x10), idx = U32(amt, 0x14);
  if (j < 0 || uint32_t(j) >= n) return img;
  const uint32_t e = U32(amt, idx + 4 * uint32_t(j));
  if (!e || e + 0x30 > amt.size()) return img;
  const uint32_t psm = U32(amt, e + 0x08), w = U16(amt, e + 0x10), h = U16(amt, e + 0x12);
  const uint32_t data = U32(amt, e + 0x14) + 0x20, dsize = U32(amt, e + 0x18);
  const uint32_t clut = U32(amt, e + 0x24) + 0x20, csize = U32(amt, e + 0x28);
  const uint32_t nc = std::min<uint32_t>(csize / 4, 256);
  if (!w || !h || w > 2048 || h > 2048 || (psm != 0x14 && psm != 0x13) || clut + 4 * nc > amt.size()) return img;
  uint8_t pal[256][4] = {};
  for (uint32_t i = 0; i < nc; ++i) {
    uint32_t s = i;
    if (nc == 256) s = (i & ~0x18u) | ((i & 0x08u) << 1) | ((i & 0x10u) >> 1);   // CSM1
    const uint8_t* c = &amt[clut + 4 * s];
    pal[i][0] = c[0];
    pal[i][1] = c[1];
    pal[i][2] = c[2];
    pal[i][3] = uint8_t(std::min(255, c[3] * 2));
  }
  img.w = int(w);
  img.h = int(h);
  img.rgba.assign(size_t(w) * h * 4, 0);
  for (uint32_t i = 0; i < w * h; ++i) {
    uint32_t v = 0;
    if (psm == 0x14) {
      const uint32_t o = data + (i >> 1);
      if ((i >> 1) < dsize && o < amt.size()) v = (i & 1) ? amt[o] >> 4 : amt[o] & 0xF;
    } else if (i < dsize && data + i < amt.size()) {
      v = amt[data + i];
    }
    std::memcpy(&img.rgba[size_t(i) * 4], pal[v & 0xFF], 4);
  }
  return img;
}

const char* SufijoAfs(const std::string& lang) {
  if (lang == "es") return "data_sp.afs";
  if (lang == "fr") return "data_fr.afs";
  if (lang == "de") return "data_ge.afs";
  if (lang == "it") return "data_it.afs";
  return "data_en.afs";
}

}  // namespace

int B1aB3(int id_b1) { return id_b1 >= 0 && id_b1 < int(std::size(kB1aB3)) ? kB1aB3[id_b1] : 0; }

namespace T = dbz3::i18n;

namespace {
// ps2_games/<nombre>.ruta -> su ISO (o carpeta); vacia si ya no existe. Otro archivo: el mismo.
std::filesystem::path Destino(const std::filesystem::path& e) {
  std::string n = Lower(e.filename().string());
  if (n.size() < 5 || n.compare(n.size() - 5, 5, ".ruta") != 0) return e;
  std::ifstream f(e, std::ios::binary);
  std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
  std::error_code ec;
  const std::filesystem::path p = std::filesystem::u8path(s);
  return !s.empty() && std::filesystem::exists(p, ec) ? p : std::filesystem::path();
}
bool EsRuta(const std::string& n) { return n.size() > 5 && n.compare(n.size() - 5, 5, ".ruta") == 0; }
}  // namespace

std::filesystem::path BuscarIsoB1() {
  std::vector<std::filesystem::path> dirs;   // DBZ3_PS2_GAMES (pruebas) sustituye a la carpeta junto al juego
  if (const char* e = std::getenv("DBZ3_PS2_GAMES")) dirs.emplace_back(e);
  else dirs.push_back(dbz3::settings::UserSettingsPath().parent_path() / "ps2_games");
  std::error_code ec;
  for (const auto& d : dirs) {
    std::vector<std::filesystem::path> isos;
    for (const auto& e : std::filesystem::directory_iterator(d, ec)) {
      const std::string n = Lower(e.path().filename().string());
      // como iso.find_game("b1"): "... Budokai (Europe) ...", "(USA)"...; ni Shin Budokai ni Budokai 2/3
      if (e.is_regular_file(ec) && ((n.size() > 4 && n.compare(n.size() - 4, 4, ".iso") == 0) || EsRuta(n)) &&
          (n.find("budokai (") != std::string::npos || n.find("budokai 1") != std::string::npos) &&
          n.find("shin") == std::string::npos) {
        if (const auto p = Destino(e.path()); !p.empty()) isos.push_back(p);
      }
    }
    std::sort(isos.begin(), isos.end());
    if (!isos.empty()) return isos.front();
  }
  return {};
}

bool LeerLeyendaSatan(const std::filesystem::path& path, const std::string& lang, DatosB1& out, std::string& error) {
  Iso iso;
  if (!iso.Open(path)) {
    error = "no es una ISO legible: " + path.string();
    return false;
  }
  out = DatosB1{};
  out.iso = path.filename().string();
  out.ruta = path;
  out.elf = BootElf(iso);
  const std::vector<uint8_t> elf = iso.ReadFile(out.elf, 0, iso.Info(out.elf).second);
  const int tabla = TablaSatan(elf);
  if (out.elf.empty() || tabla < 0) {
    error = "sin la tabla de la leyenda de Mr. Satan en " + (out.elf.empty() ? std::string("(sin ELF)") : out.elf);
    return false;
  }
  out.tabla = uint32_t(tabla);
  const std::vector<std::string> esc = Escenarios(elf);
  for (int k = 0; k < kCombates; ++k) out.combates.push_back(Registro(elf, size_t(tabla) + size_t(kRec) * k, esc));
  // historia: hacia atras desde la tabla del modo mientras el registro sea valido (1.0 en +0x0C, personajes de B1,
  // modo de historia)
  for (size_t o = size_t(tabla); o >= size_t(kRec);) {
    o -= size_t(kRec);
    const bool ok = elf[o + 0x0F] == 0x3F && elf[o + 0x20] < std::size(kB1aB3) && elf[o + 0x50] < std::size(kB1aB3) &&
                    (elf[o + 0x70] == 0 || elf[o + 0x70] == 6 || elf[o + 0x70] == 7);
    if (!ok) break;
    out.historia.insert(out.historia.begin(), Registro(elf, o, esc));
  }
  // imagenes en el idioma del juego (o el ingles, o el primer DATA_*.AFS)
  std::string afs_path = iso.Find(SufijoAfs(lang));
  if (afs_path.empty()) afs_path = iso.Find("data_en.afs");
  Afs afs;
  if (afs_path.empty() || !afs.Open(iso, afs_path)) return true;   // combates sin imagenes: el modo usa textos propios
  const int banco = BancoCharlas(afs);
  if (banco < 0) return true;
  out.afs = afs_path + " #" + std::to_string(banco);
  out.idioma = lang;
  const std::vector<uint8_t> charlas = afs.Entry(size_t(banco));
  for (int j = 0; j < 22; ++j) out.charlas.push_back(Textura(charlas, j));
  const std::vector<uint8_t> pantalla = afs.Entry(size_t(banco - 3));
  out.fondo = Textura(pantalla, 0);
  out.satan = Textura(pantalla, 1);
  out.logo = Textura(pantalla, 2);
  out.res.letrero = Textura(pantalla, 6);
  out.res.pulsa = Textura(pantalla, 5);
  for (int k = 0; k < 3; ++k) out.res.dificultad[k] = Textura(pantalla, 7 + k);
  out.res.barra = Textura(pantalla, 3);
  const std::vector<uint8_t> res = afs.Entry(size_t(banco - 1));
  out.res.letras[0] = Textura(res, 3);
  out.res.letras[1] = Textura(res, 4);
  out.res.tienes_peq = Textura(res, 20);
  out.res.cifras_peq = Textura(res, 23);
  out.res.clave = Textura(res, 25);
  out.res.tablon = Textura(res, 15);
  out.res.retadores = Textura(res, 16);
  out.res.cruz = Textura(res, 17);
  out.res.total = Textura(res, 18);
  out.res.ahora = Textura(res, 19);
  out.res.tienes = Textura(res, 21);
  out.res.cifras = Textura(res, 22);
  out.res.chibis = Textura(res, 24);
  out.res.estilo = Textura(res, 6);
  out.res.cara_satan = Textura(res, 7);
  for (int k = 0; k < 6; ++k) out.res.consejos[k] = Textura(res, k == 0 ? 8 : 9 + k);
  return true;
}

// ---- Voces de B1 (ADX_JP.AFS): ADX de CRI (4 bits, mono o estereo; como "mod center hd/gritos.py" adx_decode) ----
namespace {
std::vector<int16_t> Adx(const std::vector<uint8_t>& b, int& hz) {
  if (b.size() < 32 || b[0] != 0x80) return {};
  auto be32 = [&](size_t o) { return uint32_t(b[o]) << 24 | uint32_t(b[o + 1]) << 16 | uint32_t(b[o + 2]) << 8 | b[o + 3]; };
  const size_t off = size_t(b[2] << 8 | b[3]) + 4, bs = b[5], ch = b[7];
  hz = int(be32(8));
  const uint32_t total = be32(12), cut = uint32_t(b[16] << 8 | b[17]);
  if (hz <= 0 || ch < 1 || ch > 2 || bs < 3) return {};
  const double z = std::cos(2 * 3.14159265358979 * cut / hz), a = std::sqrt(2.0) - z, bb = std::sqrt(2.0) - 1;
  const double c = (a - std::sqrt((a + bb) * (a - bb))) / bb;
  const int c1 = int(std::floor(c * 8192)), c2 = int(std::floor(c * c * -4096));
  const size_t spb = (bs - 2) * 2;
  std::vector<int16_t> out;
  out.reserve(total);
  int hist[2][2] = {};
  std::vector<int> s(spb * 2);
  for (size_t o = off; out.size() < total && o + bs * ch <= b.size(); o += bs * ch) {
    for (size_t k = 0; k < ch; ++k) {
      const uint8_t* fr = &b[o + k * bs];
      const int scale = (fr[0] << 8 | fr[1]) + 1;
      int& h1 = hist[k][0];
      int& h2 = hist[k][1];
      for (size_t i = 0; i < spb; ++i) {
        int nib = (i & 1) ? fr[2 + i / 2] & 15 : fr[2 + i / 2] >> 4;
        if (nib >= 8) nib -= 16;
        const int v = std::clamp(nib * scale + ((c1 * h1 + c2 * h2) >> 12), -32768, 32767);
        s[k * spb + i] = v;
        h2 = h1;
        h1 = v;
      }
    }
    for (size_t i = 0; i < spb && out.size() < total; ++i) {
      out.push_back(int16_t(ch == 2 ? (s[i] + s[spb + i]) / 2 : s[i]));
    }
  }
  return out;
}

std::mutex g_voz_mu;
std::vector<uint8_t> g_wav;          // lo que suena (PlaySound lo lee mientras suena)
std::atomic<uint32_t> g_voz_turno{0};
}  // namespace

void CallarVozB1() {
  ++g_voz_turno;
#if REX_PLATFORM_WIN32
  std::lock_guard<std::mutex> lk(g_voz_mu);
  PlaySoundW(nullptr, nullptr, 0);
#endif
}

// ponytail: suena por winmm (PlaySound), aparte de la mezcla del juego: sigue el volumen general y el silencio del
// lanzador al empezar, no si cambian a mitad; el SDK no tiene entrada de PCM propio (haria falta mezclar en su
// callback de SDL). Solo Windows.
void OirVozB1(const std::filesystem::path& iso_path, std::vector<int> entradas, float volumen) {
  const uint32_t turno = ++g_voz_turno;
  std::thread([iso_path, entradas = std::move(entradas), volumen, turno] {
    Iso iso;
    Afs afs;
    const std::string p = iso.Open(iso_path) ? iso.Find("adx_jp.afs") : std::string();
    if (p.empty() || !afs.Open(iso, p)) return;
    std::vector<int16_t> pcm;
    int hz = 0, marca = -1;
    for (const int e : entradas) {
      if (e < 0) {   // -ms: la siguiente entra a esos ms del principio (los cuadros de B1), no tras medio segundo
        marca = -e;
        continue;
      }
      int h = 0;
      std::vector<int16_t> v = Adx(afs.Entry(size_t(e)), h);
      if (v.empty() || (hz && h != hz)) continue;
      hz = h;
      if (marca >= 0) pcm.resize(std::max(pcm.size(), size_t(int64_t(marca) * hz / 1000)), 0);
      else if (!pcm.empty()) pcm.insert(pcm.end(), size_t(hz) / 2, 0);   // medio segundo entre una y otra
      marca = -1;
      pcm.insert(pcm.end(), v.begin(), v.end());
    }
    std::string lista;
    for (const int e : entradas) lista += " " + std::to_string(e);
    REXLOG_INFO("dbz3 pasarela: voz de B1 (ADX_JP.AFS{}): {:.1f} s a {} Hz{}", lista, hz ? double(pcm.size()) / hz : 0.0,
                hz, volumen > 0 ? "" : " (sin sonido: silencio o volumen 0)");
    if (pcm.empty() || volumen <= 0 || turno != g_voz_turno.load()) return;
    for (int16_t& x : pcm) x = int16_t(std::clamp(int(float(x) * volumen), -32768, 32767));
    // WAV PCM 16 bits mono
    const uint32_t datos = uint32_t(pcm.size() * 2);
    std::vector<uint8_t> w(44 + datos);
    auto u32 = [&](size_t o, uint32_t v) { std::memcpy(&w[o], &v, 4); };
    auto u16 = [&](size_t o, uint16_t v) { std::memcpy(&w[o], &v, 2); };
    std::memcpy(&w[0], "RIFF", 4);
    u32(4, 36 + datos);
    std::memcpy(&w[8], "WAVEfmt ", 8);
    u32(16, 16);
    u16(20, 1);
    u16(22, 1);
    u32(24, uint32_t(hz));
    u32(28, uint32_t(hz) * 2);
    u16(32, 2);
    u16(34, 16);
    std::memcpy(&w[36], "data", 4);
    u32(40, datos);
    std::memcpy(&w[44], pcm.data(), datos);
#if REX_PLATFORM_WIN32
    std::lock_guard<std::mutex> lk(g_voz_mu);
    if (turno != g_voz_turno.load()) return;
    PlaySoundW(nullptr, nullptr, 0);   // suelta g_wav antes de cambiarlo
    g_wav = std::move(w);
    PlaySoundW(reinterpret_cast<LPCWSTR>(g_wav.data()), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
#endif
  }).detach();
}

// F5: un ADX de un AFS de la copia del jugador (IW: USR/ADX_CMN.AFS...) por su propio waveOut, para que varios se
// mezclen (filas del resultado y su musica) sin cortar la voz de PlaySound. CallarAdxPs2 corta los que suenan.
namespace {
std::atomic<uint32_t> g_adx_gen{0};
}
void OirAdxPs2(const std::filesystem::path& juego, const std::string& afs, int entrada, float volumen) {
  if (volumen <= 0) return;
  const uint32_t gen = g_adx_gen.load();
  std::thread([juego, afs, entrada, volumen, gen] {
    AfsPs2 a;
    int hz = 0;
    std::vector<int16_t> pcm = a.Abrir(juego, afs) ? Adx(a.Entrada(size_t(entrada)), hz) : std::vector<int16_t>();
    REXLOG_INFO("dbz3 pasarela: ADX {} {}: {:.1f} s", afs, entrada, hz ? double(pcm.size()) / hz : 0.0);
    if (pcm.empty() || gen != g_adx_gen.load()) return;
    for (int16_t& x : pcm) x = int16_t(std::clamp(int(float(x) * volumen), -32768, 32767));
#if REX_PLATFORM_WIN32
    WAVEFORMATEX f{};
    f.wFormatTag = WAVE_FORMAT_PCM;
    f.nChannels = 1;
    f.nSamplesPerSec = DWORD(hz);
    f.wBitsPerSample = 16;
    f.nBlockAlign = 2;
    f.nAvgBytesPerSec = DWORD(hz) * 2;
    HWAVEOUT wo = nullptr;
    if (waveOutOpen(&wo, WAVE_MAPPER, &f, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) return;
    WAVEHDR h{};
    h.lpData = reinterpret_cast<LPSTR>(pcm.data());
    h.dwBufferLength = DWORD(pcm.size() * 2);
    if (waveOutPrepareHeader(wo, &h, sizeof(h)) == MMSYSERR_NOERROR) {
      if (waveOutWrite(wo, &h, sizeof(h)) == MMSYSERR_NOERROR)
        while (!(h.dwFlags & WHDR_DONE) && gen == g_adx_gen.load()) Sleep(20);
      waveOutReset(wo);
      waveOutUnprepareHeader(wo, &h, sizeof(h));
    }
    waveOutClose(wo);
#endif
  }).detach();
}
void CallarAdxPs2() { ++g_adx_gen; }

// ---- Infinite World (ficha 02 C): textos de las misiones ----
// DATA_USA.AFS de IW (USA): desde la entrada 132, un #AMB por mision [#AMT descripcion, #AMT titulo] en el orden del
// juego (132 "Training No.1"... 142 "Defeat Raditz!"... 174 "Endure Gohan's attacks!"; medido 2026-10-15 con
// E:/DBZ3HD_trabajo/campanas/iw_titulos.py). Vale una carpeta extraida (USR/DATA_USA.AFS) o la ISO.
std::filesystem::path BuscarIW() {
  std::vector<std::filesystem::path> dirs;
  if (const char* e = std::getenv("DBZ3_PS2_GAMES")) dirs.emplace_back(e);
  else dirs.push_back(dbz3::settings::UserSettingsPath().parent_path() / "ps2_games");
  std::error_code ec;
  for (const auto& d : dirs) {
    std::vector<std::filesystem::path> hay;
    for (const auto& e : std::filesystem::directory_iterator(d, ec)) {
      const std::string n = Lower(e.path().filename().string());
      if (n.find("infinite world") == std::string::npos) continue;
      if (e.is_directory(ec) || (n.size() > 4 && n.compare(n.size() - 4, 4, ".iso") == 0) || EsRuta(n)) {
        if (const auto p = Destino(e.path()); !p.empty()) hay.push_back(p);
      }
    }
    std::sort(hay.begin(), hay.end());
    if (!hay.empty()) return hay.front();
  }
  return {};
}

bool LeerMisionesIW(const std::filesystem::path& iw, const std::vector<int>& numeros, DatosIW& out, std::string& error) {
  out = DatosIW{};
  out.origen = iw.filename().string();
  // lector de DATA_USA.AFS: fichero de la carpeta o de la ISO
  Iso iso;
  std::ifstream f;
  std::string en_iso;
  std::error_code ec;
  if (std::filesystem::is_directory(iw, ec)) {
    for (const auto& e : std::filesystem::recursive_directory_iterator(iw, ec)) {
      if (Lower(e.path().filename().string()) == "data_usa.afs") {
        f.open(e.path(), std::ios::binary);
        break;
      }
    }
  } else if (iso.Open(iw)) {
    en_iso = iso.Find("data_usa.afs");
  }
  auto leer = [&](uint64_t off, uint32_t n) -> std::vector<uint8_t> {
    if (!en_iso.empty()) return iso.ReadFile(en_iso, off, n);
    std::vector<uint8_t> b(n);
    f.clear();
    f.seekg(std::streamoff(off));
    f.read(reinterpret_cast<char*>(b.data()), std::streamsize(n));
    b.resize(size_t(std::max<std::streamsize>(0, f.gcount())));
    return b;
  };
  if (en_iso.empty() && !f) {
    error = "sin DATA_USA.AFS en " + iw.string();
    return false;
  }
  const std::vector<uint8_t> h = leer(0, 8);
  if (h.size() < 8 || std::memcmp(h.data(), "AFS", 3) != 0) {
    error = "DATA_USA.AFS no valido";
    return false;
  }
  const uint32_t n = std::min<uint32_t>(U32(h, 4), 65536);
  const std::vector<uint8_t> tab = leer(8, 8 * n);
  for (const int m : numeros) {
    MisionIW mi;
    mi.numero = m;
    const uint32_t k = 132u + uint32_t(m);
    if (k < n && 8 * k + 8 <= tab.size()) {
      const std::vector<uint8_t> amb = leer(U32(tab, 8 * k), U32(tab, 8 * k + 4));
      if (amb.size() >= 0x20 && std::memcmp(amb.data(), "#AMB", 4) == 0 && U32(amb, 0x10) == 2) {
        const uint32_t t = U32(amb, 0x14);
        auto hijo = [&](uint32_t c) {
          const uint32_t o = U32(amb, t + 16 * c), sz = U32(amb, t + 16 * c + 4);
          return o + sz <= amb.size() ? std::vector<uint8_t>(amb.begin() + o, amb.begin() + o + sz) : std::vector<uint8_t>();
        };
        const std::vector<uint8_t> desc = hijo(0), tit = hijo(1);
        if (AmtN(desc) >= 1) mi.texto = Textura(desc, 0);
        if (AmtN(tit) >= 1) mi.titulo = Textura(tit, 0);
      }
    }
    out.misiones.push_back(std::move(mi));
  }
  return true;
}

// ---- Lectura generica de los juegos de PS2 del jugador (B1, B2, IW): ISO o carpeta extraida ----
struct AfsPs2::Impl {
  Iso iso;
  std::ifstream f;
  std::string en_iso;
  std::vector<std::pair<uint32_t, uint32_t>> tab;
  std::vector<uint8_t> Leer(uint64_t off, uint32_t n) {
    if (!en_iso.empty()) return iso.ReadFile(en_iso, off, n);
    std::vector<uint8_t> b(n);
    f.clear();
    f.seekg(std::streamoff(off));
    f.read(reinterpret_cast<char*>(b.data()), std::streamsize(n));
    b.resize(size_t(std::max<std::streamsize>(0, f.gcount())));
    return b;
  }
};

bool AfsPs2::Abrir(const std::filesystem::path& juego, const std::string& afs) {
  auto d = std::make_shared<Impl>();
  std::error_code ec;
  const std::string nombre = Lower(afs);
  if (std::filesystem::is_directory(juego, ec)) {
    for (const auto& e : std::filesystem::recursive_directory_iterator(juego, ec)) {
      if (Lower(e.path().filename().string()) == nombre) {
        d->f.open(e.path(), std::ios::binary);
        break;
      }
    }
    if (!d->f) return false;
  } else if (!d->iso.Open(juego) || (d->en_iso = d->iso.Find(nombre)).empty()) {
    return false;
  }
  const std::vector<uint8_t> h = d->Leer(0, 8);
  if (h.size() < 8 || std::memcmp(h.data(), "AFS", 3) != 0) return false;
  const uint32_t n = std::min<uint32_t>(U32(h, 4), 65536);
  const std::vector<uint8_t> t = d->Leer(8, 8 * n);
  for (uint32_t k = 0; 8 * k + 8 <= t.size(); ++k) d->tab.push_back({U32(t, 8 * k), U32(t, 8 * k + 4)});
  if (d->tab.empty()) return false;
  d_ = std::move(d);
  return true;
}
size_t AfsPs2::Cuantas() const { return d_ ? d_->tab.size() : 0; }
std::vector<uint8_t> AfsPs2::Entrada(size_t k, uint32_t n) {
  if (!d_ || k >= d_->tab.size() || !d_->tab[k].second) return {};
  return d_->Leer(d_->tab[k].first, std::min(n, d_->tab[k].second));
}

std::vector<uint8_t> HijoAmb(const std::vector<uint8_t>& amb, uint32_t c) {
  if (amb.size() < 0x20 || std::memcmp(amb.data(), "#AMB", 4) != 0 || c >= U32(amb, 0x10)) return {};
  const uint32_t t = U32(amb, 0x14);
  if (size_t(t) + 16 * (c + 1) > amb.size()) return {};
  const uint32_t o = U32(amb, t + 16 * c), sz = U32(amb, t + 16 * c + 4);
  return size_t(o) + sz <= amb.size() ? std::vector<uint8_t>(amb.begin() + o, amb.begin() + o + sz) : std::vector<uint8_t>();
}
int CuantasAmt(const std::vector<uint8_t>& amt) { return AmtN(amt); }
Imagen TexturaAmt(const std::vector<uint8_t>& amt, int j) { return AmtN(amt) > 0 ? Textura(amt, j) : Imagen{}; }

std::filesystem::path BuscarJuego(const std::string& clave) {
  std::filesystem::path d = dbz3::settings::UserSettingsPath().parent_path() / "ps2_games";
  if (const char* e = std::getenv("DBZ3_PS2_GAMES")) d = e;
  std::vector<std::filesystem::path> hay;
  std::error_code ec;
  const std::string c = Lower(clave);
  for (const auto& e : std::filesystem::directory_iterator(d, ec)) {
    const std::string n = Lower(e.path().filename().string());
    if (n.find(c) == std::string::npos) continue;
    if (e.is_directory(ec) || (n.size() > 4 && n.compare(n.size() - 4, 4, ".iso") == 0) || EsRuta(n)) {
      if (const auto p = Destino(e.path()); !p.empty()) hay.push_back(p);
    }
  }
  std::sort(hay.begin(), hay.end());
  return hay.empty() ? std::filesystem::path() : hay.front();
}

std::filesystem::path IsoPs2(int juego) {
  return juego == 0 ? BuscarIsoB1() : juego == 1 ? BuscarJuego("budokai 2") : BuscarIW();
}

// Serie del SYSTEM.CNF (BOOT2 = cdrom0:\SLES_518.39;1 -> "sles_518.39"), vacia si no es una ISO de PS2 legible.
static std::string SeriePs2(const std::filesystem::path& p) {
  Iso iso;
  if (!iso.Open(p)) return {};
  const std::string cnf = iso.Find("system.cnf");
  if (cnf.empty()) return "?";
  const std::vector<uint8_t> b = iso.ReadFile(cnf, 0, 512);
  std::string s = Lower(std::string(b.begin(), b.end()));
  const size_t i = s.find("cdrom0:");
  if (i == std::string::npos) return "?";
  size_t j = i + 7;
  while (j < s.size() && (s[j] == '\\' || s[j] == '/')) ++j;
  size_t k = j;
  while (k < s.size() && s[k] != ';' && s[k] != '\r' && s[k] != '\n') ++k;
  return s.substr(j, k - j);
}

std::string RegistrarIsoPs2(int juego, const std::filesystem::path& iso) {
  // series conocidas (las de las copias medidas; otra region no se rechaza, solo se avisa en el registro)
  static const char* const kSerie[3][2] = {{"sles_512.33", ""}, {"sles_518.39", ""}, {"slus_218.42", ""}};
  static const char* const kNombre[3] = {"Budokai 1", "Budokai 2", "Infinite World"};
  const std::string s = SeriePs2(iso);
  if (s.empty())
    return T::T("Ese archivo no es una ISO de PlayStation 2 legible.", "That file is not a readable PlayStation 2 ISO.");
  for (int g = 0; g < 3; ++g) {
    if (g != juego && s == kSerie[g][0]) {
      return std::string(T::T("Esa ISO es de ", "That ISO is ")) + kNombre[g] + T::T(", no de ", ", not ") + kNombre[juego] +
             ".";
    }
  }
  if (s != kSerie[juego][0]) REXLOG_INFO("dbz3 ps2: {} con serie {} (otra region?): se acepta", kNombre[juego], s);
  std::filesystem::path d = dbz3::settings::UserSettingsPath().parent_path() / "ps2_games";
  if (const char* e = std::getenv("DBZ3_PS2_GAMES")) d = e;
  std::error_code ec;
  std::filesystem::create_directories(d, ec);
  static const char* const kRuta[3] = {"Dragon Ball Z Budokai 1.ruta", "Dragon Ball Z Budokai 2.ruta",
                                       "Dragon Ball Z Infinite World.ruta"};
  std::ofstream f(d / kRuta[juego], std::ios::binary | std::ios::trunc);
  f << dbz3::launcher::PathUtf8(std::filesystem::absolute(iso, ec));
  f.close();
  if (f.fail()) return T::T("No se pudo guardar la eleccion (carpeta ps2_games).", "Could not save the choice (ps2_games folder).");
  REXLOG_INFO("dbz3 ps2: {} -> {} (serie {})", kNombre[juego], iso.string(), s);
  return {};
}

namespace {
std::atomic<bool> g_eligiendo{false};
std::mutex g_eleccion_mu;
std::string g_eleccion;
}  // namespace

void ElegirIsoPs2EnSegundoPlano(int juego) {
  if (g_eligiendo.exchange(true)) return;
  std::thread([juego] {
    const std::string r = ElegirIsoPs2(juego);
    {
      std::lock_guard<std::mutex> lk(g_eleccion_mu);
      g_eleccion = r == "-" ? std::string()
                   : r.empty() ? std::string(T::T("¡Listo! Ya puedes jugarlo.", "Done! You can play it now."))
                               : r;
    }
    g_eligiendo = false;
  }).detach();
}

std::string AvisoEleccionPs2() {
  if (g_eligiendo) {
    return T::T("Elige la ISO en la ventana que se ha abierto (si no la ves, mira la barra de tareas).",
                "Pick the ISO in the window that just opened (if you can't see it, check the taskbar).");
  }
  std::lock_guard<std::mutex> lk(g_eleccion_mu);
  return g_eleccion;
}

std::string ElegirIsoPs2(int juego) {
  static const char* const kDesc[3] = {"Dragon Ball Z Budokai (PS2) .iso", "Dragon Ball Z Budokai 2 (PS2) .iso",
                                       "Dragon Ball Z Infinite World (PS2) .iso"};
  std::string out;
  if (!dbz3::launcher::PickFileDialog(out, kDesc[juego], "*.iso")) return "-";
  return RegistrarIsoPs2(juego, std::filesystem::u8path(out));
}

}  // namespace dbz3::pasarela
