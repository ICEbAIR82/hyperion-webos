#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Starts an asynchronous local-subnet scan for WLED devices (probes each
// candidate host's /json/info over HTTP). Idempotent: a no-op if a scan is
// already running. Non-blocking - returns immediately.
void wled_discovery_start(void);

// Malloc'd JSON string: {"scanning": bool, "devices": [{"ip","name","ledCount"}, ...]}
// Caller must free().
char* wled_discovery_get_results(void);

// Probes a single WLED device (any reachable address, not just ones found by
// a scan) for its name and configured LED count. Malloc'd JSON string:
// {"reachable": bool, "name": "...", "ledCount": N}. Caller must free().
char* wled_discovery_probe_one(const char* ip);

#ifdef __cplusplus
}
#endif
