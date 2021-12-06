#ifndef HYPERIONOBS_H
#define HYPERIONOBS_H

#include <callback/signal.h>

#include <QString>

void hyperion_start_streaming(QString& address, int port);
void hyperion_stop_streaming();
void hyperion_release();
signal_handler_t* hyperion_get_signal_handler();

#endif // HYPERIONOBS_H
