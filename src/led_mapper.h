#pragma once

#include "blackbar_detector.h"
#include "led_layout.h"
#include <stdint.h>

// Computes one averaged RGB triplet per entry in layout[] from a tightly
// packed RGB24 frame (width * height * 3 bytes, no row padding).
// out_led_rgb must have room for led_count * 3 bytes.
void led_mapper_compute(const uint8_t* rgb, int width, int height,
    const led_region_t* layout, int led_count, uint8_t* out_led_rgb);

// Remaps each region in layout[] (fractions of the full frame) onto area
// (itself expressed as fractions of the full frame), writing the result
// into out (must have room for led_count entries). Used when black-bar
// detection has found the active picture is smaller than the full frame,
// so LEDs hug the actual picture edge instead of the raw capture edge.
void led_mapper_remap_to_active_area(const led_region_t* layout, int led_count,
    active_area_t area, led_region_t* out);
