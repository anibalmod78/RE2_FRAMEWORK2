#pragma once
#include "common.hpp"

// Vigila (sin modificar nada) como el PROPIO juego usa las funciones de carga
// y registro de jugadores. Salida: dmc5_ss_trace.log
namespace ss::trace {

bool install();            // idempotente; reintentar hasta que devuelva true
void set_enabled(bool on);
void summary();            // cuantas veces se disparo cada hook

// Aviso en el PRE de cada CharacterFactory.create(id, costume, parent, mode, finishEvent).
// Permite grabar la llamada real del juego para reproducirla despues.
using CreateListener = void (*)(int32_t id, void* costume, void* parent, int32_t mode, void* finish_event);
void set_create_listener(CreateListener fn);

} // namespace ss::trace
