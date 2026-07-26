#pragma once

#include "log.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_SERVER_DEFAULT_PORT 8091

// Starts an in-memory ring buffer of recent log lines plus a raw TCP
// listener on tcp_port (meant for `telnet <tv-ip> <port>` / a PuTTY raw
// session): every connecting client first gets the current backlog, then a
// live tail of everything logged afterwards. The same ring buffer backs
// log_server_build_json_since() below, used by GET /api/logs for the web
// UI. Together these let a sporadic multi-second dropout be captured after
// the fact - either by leaving a terminal open for hours, or by leaving the
// web UI's Log tab open - instead of needing to be caught live.
// Safe to call once; a second call is a no-op (logs a warning).
void log_server_init(uint16_t tcp_port);
void log_server_stop(void);

// Called from log.c for every log message that already passes the active
// log level filter (i.e. the same messages that already go to stderr) -
// appends it to the ring buffer and broadcasts it to connected TCP clients.
// No-op if log_server_init() hasn't run yet.
void log_server_push(LogLevel level, double elapsed_seconds, const char* module, const char* message);

// Builds {"bootId","latestSeq","lines":[{"seq","level","t","text"}, ...]}
// for buffered lines with seq > since_seq. client_boot_id is the "bootId"
// the caller last saw; if it's empty/NULL or doesn't match this process's
// current boot id, since_seq is ignored and the whole current buffer is
// returned instead (tagged with the new boot id) - this is how a polling
// client (the web UI) notices "the service restarted since my last poll"
// instead of just silently missing the gap. Malloc'd JSON; caller must
// free().
char* log_server_build_json_since(uint64_t since_seq, const char* client_boot_id);

#ifdef __cplusplus
}
#endif
