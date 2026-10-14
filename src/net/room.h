// dbz3 - Codigo de sala del online: la IP publica y el puerto del anfitrion en 10 letras (XXXXX-XXXXX),
// apertura automatica del puerto en el router (UPnP de Windows, sin librerias de terceros) y la
// consulta STUN que da la IP publica cuando el router no hace UPnP.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dbz3::net {

// ip en orden de red (la de sockaddr_in), puerto en orden de host.
std::string EncodeRoom(uint32_t ip, uint16_t port);
// Acepta el codigo con o sin guion, en minusculas y con O/I/L en vez de 0/1. false si no es un codigo.
bool DecodeRoom(const std::string& code, uint32_t* ip, uint16_t* port);

// Abre `port` UDP en el router hacia este PC (bloquea unos segundos: llamar en un hilo aparte).
// Devuelve la IP publica que da el router, o "" si no hay UPnP.
std::string UpnpOpen(uint16_t port, const std::string& local_ip);
void UpnpClose(uint16_t port);
// Al cerrar el juego (aunque sea sin pasar por Close) se quita el puerto del router.
void UpnpCloseAtExit(uint16_t port);
// Quita ya el puerto abierto (si hay). La salida del juego es std::_Exit, que no pasa por atexit:
// la llama main.cpp antes (sin esto el puerto se quedaba abierto en el router, 2026-10-14).
void UpnpCloseMapped();

// STUN (RFC 5389): peticion de 20 bytes y lectura de la respuesta (XOR-MAPPED-ADDRESS).
std::vector<uint8_t> StunRequest();
bool IsStun(const uint8_t* p, int n);
bool StunMapped(const uint8_t* p, int n, uint32_t* ip, uint16_t* port);

bool RoomSelfTest();

}  // namespace dbz3::net
