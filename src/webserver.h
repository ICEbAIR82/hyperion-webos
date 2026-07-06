#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WEBSERVER_DEFAULT_PORT 8090

int webserver_start(uint16_t port);
void webserver_stop(void);

#ifdef __cplusplus
}
#endif
