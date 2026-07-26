#pragma once

#include "service.h"

#ifdef __cplusplus
extern "C" {
#endif

void rest_api_init(service_t* service);

// Returned strings are malloc'd JSON; caller must free().
char* rest_api_get_config(void);
char* rest_api_get_status(void);

// Malloc'd JSON: {"returnValue":true,"leds":[[r,g,b],...]}. Caller must free().
char* rest_api_get_live_leds(void);

// body is the raw JSON request body of a PUT /api/config request. Writes a
// malloc'd JSON response into *out_response (caller must free()) and returns
// an HTTP status code (200 on success, 400 on a parse/apply failure).
int rest_api_put_config(const char* body, char** out_response);

// body is {"wbred":int,"wbgreen":int,"wbblue":int} (any subset). Updates the
// running service's white balance gains in-memory only - no restart, no
// disk write - so the web UI's sliders can apply changes live while being
// dragged. Missing keys keep their current value. Malloc'd JSON
// {"returnValue":true/false}; caller must free().
char* rest_api_put_whitebalance_live(const char* body);

// Returned strings are malloc'd JSON ({"returnValue": true/false}); caller must free().
char* rest_api_start(void);
char* rest_api_stop(void);

// body is {"color":"#rrggbb"} (any CSS color string). Brings the TV app to
// the foreground and, once there, it shows that color fullscreen (via its
// existing status poll picking up calibrationActive/calibrationColor) so you
// can calibrate white/color balance against the real capture pipeline
// instead of just watching the web UI's own live-color view. Malloc'd JSON
// {"returnValue":true}; caller must free().
char* rest_api_calibrate_start(const char* body);
char* rest_api_calibrate_stop(void);

// Starts an async local-subnet scan for WLED devices (no-op if already running).
void rest_api_wled_discover_start(void);
// Malloc'd JSON: {"scanning": bool, "devices": [{"ip","name","ledCount"}, ...]}. Caller must free().
char* rest_api_wled_discover_results(void);
// Probes a single address (defaults to the currently configured wled_address if
// ip is NULL/empty) for its name and LED count. Malloc'd JSON:
// {"reachable": bool, "name", "ledCount", "configuredLedCount"}. Caller must free().
char* rest_api_wled_info(const char* ip);

// since/boot come straight from the "since"/"boot" query parameters (may be
// NULL/empty - see log_server_build_json_since() for exactly how those are
// interpreted). Malloc'd JSON: {"bootId","latestSeq","lines":[...]}. Caller
// must free().
char* rest_api_get_logs(const char* since, const char* boot);

#ifdef __cplusplus
}
#endif
