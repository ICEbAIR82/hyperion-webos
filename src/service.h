#pragma once

#include "led_layout.h"
#include "settings.h"
#include "unicapture.h"
#include <glib.h>
#include <luna-service2/lunaservice.h>
#include <pthread.h>

#define SERVICE_NAME "org.webosbrew.piccapplus.service"
#define APP_ID "org.webosbrew.piccapplus"

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
    // Scratch copy of settings->led_layout, remapped onto the currently
    // detected active picture area when letterbox_detection is on. Same
    // lifetime as led_rgb_buffer (allocated/freed alongside it).
    led_region_t* remapped_led_layout;

    pthread_t connection_thread;
    bool connection_loop_running;

    GMainLoop* loop;

    // Whichever bus registration succeeded in service_register (public
    // preferred, legacy private-bus as fallback) - kept around so
    // service_start_calibration can ask applicationManager to foreground the
    // TV app from a REST request, without re-registering anything.
    LSHandle* handle;

    // White-balance calibration mode: ephemeral, in-memory only (not a
    // setting, doesn't survive a restart). While active, the TV app shows a
    // fullscreen solid color instead of its normal UI - it picks this up via
    // its existing status poll, see service_build_status_json. The color is
    // just relayed as-is; this code never interprets it.
    bool calibration_active;
    char calibration_color[16];
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

// Enters/exits white-balance calibration mode: starting it also asks
// applicationManager to bring the TV app to the foreground, so the user
// actually sees the fullscreen color it's about to display. color is an
// arbitrary CSS color string (e.g. "#ffffff"), copied as-is - not
// interpreted or validated here.
void service_start_calibration(service_t* service, const char* color);
void service_stop_calibration(service_t* service);

// Updates white/color balance gains, LED gamma and LED brightness in-memory only,
// without a restart - see the definition in service.c for why this needs to be
// separate from service_apply_settings_json. Returns 0 on success.
int service_apply_whitebalance_live(service_t* service, int wb_red, int wb_green, int wb_blue, double led_gamma, int led_brightness);
