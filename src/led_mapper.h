#pragma once

#include "led_layout.h"
#include <stdint.h>

// Computes one averaged RGB triplet per entry in layout[] from a tightly
// packed RGB24 frame (width * height * 3 bytes, no row padding).
// out_led_rgb must have room for led_count * 3 bytes.
void led_mapper_compute(const uint8_t* rgb, int width, int height,
    const led_region_t* layout, int led_count, uint8_t* out_led_rgb);
