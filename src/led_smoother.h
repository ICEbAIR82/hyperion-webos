#pragma once

#include <stdint.h>

// Allocates internal state for led_count LEDs. Call again (or led_smoother_close
// first) if led_count changes at runtime.
int led_smoother_init(int led_count);

// Exponentially smooths led_rgb in place towards its input values, using the
// actual elapsed wall-clock time since the last call as the basis for the
// decay (so capture-rate jitter doesn't change the effective smoothing speed).
// Per channel, rise_time_ms is used while a value is increasing (getting
// brighter) and fall_time_ms while it is decreasing (getting darker) - human
// eyes tend to want scene cuts to black to happen faster than fade-ins.
// A time <= 0 disables smoothing for that direction (passthrough).
void led_smoother_apply(uint8_t* led_rgb, int led_count, int rise_time_ms, int fall_time_ms);

void led_smoother_close(void);
