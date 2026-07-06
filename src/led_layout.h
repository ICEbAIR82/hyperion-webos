#pragma once

// LED capture region: a fractional rectangle of the captured frame ([0,1] x [0,1])
// to average for one LED's color.

typedef struct _led_region {
    float hmin;
    float hmax;
    float vmin;
    float vmax;
} led_region_t;

// Fallback layout used when no "ledLayout" is present in the config file, in
// physical LED order along the strip. Taken from the Schlafzimmer NANOCELL
// HyperHDR strip layout (reference_config.json).
#define LED_LAYOUT_DEFAULT_COUNT 178

extern const led_region_t led_layout_default[LED_LAYOUT_DEFAULT_COUNT];
