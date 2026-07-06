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

    led_region_t* led_layout;
    int led_count;
} settings_t;

void settings_init(settings_t*);

int settings_load_json(settings_t* settings, jvalue_ref source);
int settings_save_json(settings_t* settings, jvalue_ref target);

int settings_load_file(settings_t* settings, char* source);
int settings_save_file(settings_t* settings, char* target);
