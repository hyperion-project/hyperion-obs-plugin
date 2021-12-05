#pragma once

#include <callback/signal.h>

void hyperion_enable(const char *location, const int port);
void hyperion_disable();
void hyperion_release();
signal_handler_t* hyperion_get_signal_handler();