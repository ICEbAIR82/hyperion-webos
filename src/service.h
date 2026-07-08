#pragma once

#include "settings.h"
#include "unicapture.h"
#include <glib.h>
#include <pthread.h>

#define SERVICE_NAME "org.webosbrew.piccapplus.service"

typedef struct {
    bool running;
    bool connected;
    bool power_paused;

    capture_backend_t ui_backend;
    capture_backend_t video_backend;

    bool ui_backend_initialized;
    bool video_backend_initialized;

    unicapture_state_t unicapture;

    settings_t* settings;

    uint8_t* led_rgb_buffer;

    pthread_t connection_thread;
    bool connection_loop_running;

    GMainLoop* loop;
} service_t;

int service_init(service_t* service, settings_t* settings);
int service_register(service_t* service, GMainLoop* loop);

void service_init_backends(service_t* service);
void service_destroy_backends(service_t* service);

int service_start(service_t* service);
int service_stop(service_t* service);

int service_destroy(service_t* service);

// Shared building blocks behind both the Luna API (getSettings/setSettings/status)
// and the HTTP REST API, so the two never drift apart.
// Caller owns the returned jvalue_ref and must j_release() it.
jvalue_ref service_build_status_json(service_t* service);
jvalue_ref service_build_settings_json(service_t* service);

// Snapshot of the current per-LED colors (post-smoothing, the same values
// just sent to WLED), as {"returnValue":true,"leds":[[r,g,b],...]}. Empty
// array if the service isn't running or isn't in direct-LED-output mode.
jvalue_ref service_build_live_leds_json(service_t* service);

// Applies a (partial) settings JSON, persists it, and cleanly restarts the
// service if it was already running. Returns 0 on success.
int service_apply_settings_json(service_t* service, jvalue_ref parsed);
