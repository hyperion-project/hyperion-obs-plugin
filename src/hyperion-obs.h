#ifndef HYPERIONOBS_H
#define HYPERIONOBS_H

#include <callback/signal.h>
#include <QString>

#define QSTRING_CSTR(str) str.toLocal8Bit().constData()

const int DEFAULT_SIZEDECIMATION = 8;
const int DEFAULT_PRIORITY = 150;

void hyperion_start_streaming(QString& address, int port, int priority = DEFAULT_PRIORITY, int sizeDecimation = DEFAULT_SIZEDECIMATION);
void hyperion_stop_streaming();
void hyperion_release();
signal_handler_t* hyperion_get_signal_handler();

#endif // HYPERIONOBS_H
