#pragma once

#include "led_layout.h"
#include <pbnjson.h>
#include <stdbool.h>

#define SETTINGS_PERSISTENCE_PATH "/media/developer/apps/usr/palm/services/org.webosbrew.piccapplus.service/config.json"

// Settings stored in config.json file

typedef struct _settings_t {
    char* video_backend;
    char* ui_backend;

    char* address;
    int port;
    int priority;
    bool unix_socket;

    int fps;
    bool send_nv12;
    int width;
    int height;
    bool vsync;
    int quirks;

    bool no_video;
    bool no_gui;

    bool autostart;

    bool dump_frames;

    bool no_hdr;
    bool no_powerstate;

    // Direct-to-WLED output (DDP), bypassing Hyperion/HyperHDR entirely
    bool direct_led_output;
    char* wled_address;
    int wled_port;
    int smoothing_rise_time_ms;
    int smoothing_fall_time_ms;

    // Detects black letterbox (top/bottom) or pillarbox (left/right) bars
    // in the captured frame and maps LEDs onto the actual picture area
    // instead of the raw capture frame, like HyperHDR's black-bar detector.
    bool letterbox_detection;

    // Dims all LEDs uniformly (on top of their own per-region colors) based
    // on the captured frame's overall average brightness, for a more
    // atmospheric effect during predominantly dark scenes. Off by default -
    // this is a subjective mood effect, unlike letterbox_detection above
    // which is closer to a straightforward correctness fix.
    bool scene_dimming;

    led_region_t* led_layout;
    int led_count;

    // Parameters behind the web UI's LED layout generator (webui/index.html
    // buildClassicLayout). The backend never reads these itself - led_layout/
    // led_count above (the actual per-LED regions) is the only thing capture
    // uses - they're persisted purely so the web UI can redisplay what you
    // last entered instead of always showing its built-in example values.
    int led_layout_top;
    int led_layout_bottom;
    int led_layout_left;
    int led_layout_right;
    int led_layout_gap_length;
    int led_layout_gap_position;
    int led_layout_input_position;
    int led_layout_group_h;
    int led_layout_group_v;
    bool led_layout_reverse;

    // Per-channel white/color balance gain (percent, 100 = unchanged),
    // applied as a straight multiply to every LED's RGB value right before
    // it's sent out. Corrects for the strip's own color response (e.g. a
    // captured neutral white looking slightly blue/green on the physical
    // LEDs) - not a real RGBW white-channel split, just a 3-channel
    // correction, which is all an RGB-only strip needs or can use.
    int wb_red;
    int wb_green;
    int wb_blue;
} settings_t;

void settings_init(settings_t*);

int settings_load_json(settings_t* settings, jvalue_ref source);
int settings_save_json(settings_t* settings, jvalue_ref target);

int settings_load_file(settings_t* settings, char* source);
int settings_save_file(settings_t* settings, char* target);
