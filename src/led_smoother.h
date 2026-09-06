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

// Keeps the elapsed-time basis above in sync while the caller is holding its
// last output instead of feeding new frames (see service_feed_frame). Without
// this, the whole hold window would land in the next call's elapsed time,
// driving the decay to ~1.0 and snapping the LEDs straight to the new color
// instead of fading like they do everywhere else.
void led_smoother_hold(void);

void led_smoother_close(void);
