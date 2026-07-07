#pragma once

#include "service.h"

#ifdef __cplusplus
extern "C" {
#endif

void rest_api_init(service_t* service);

// Returned strings are malloc'd JSON; caller must free().
char* rest_api_get_config(void);
char* rest_api_get_status(void);

// body is the raw JSON request body of a PUT /api/config request. Writes a
// malloc'd JSON response into *out_response (caller must free()) and returns
// an HTTP status code (200 on success, 400 on a parse/apply failure).
int rest_api_put_config(const char* body, char** out_response);

// Returned strings are malloc'd JSON ({"returnValue": true/false}); caller must free().
char* rest_api_start(void);
char* rest_api_stop(void);

// Starts an async local-subnet scan for WLED devices (no-op if already running).
void rest_api_wled_discover_start(void);
// Malloc'd JSON: {"scanning": bool, "devices": [{"ip","name","ledCount"}, ...]}. Caller must free().
char* rest_api_wled_discover_results(void);
// Probes a single address (defaults to the currently configured wled_address if
// ip is NULL/empty) for its name and LED count. Malloc'd JSON:
// {"reachable": bool, "name", "ledCount", "configuredLedCount"}. Caller must free().
char* rest_api_wled_info(const char* ip);

#ifdef __cplusplus
}
#endif
