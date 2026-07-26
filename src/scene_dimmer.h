#pragma once

#include <stdint.h>

// Resets internal smoothing state. Call once when capture (re)starts, so a
// stale gain from a previous session doesn't linger.
void scene_dimmer_reset(void);

// Computes a global brightness gain (0.0-1.0) from the overall average
// luminance of the captured frame: full gain for anything reasonably lit,
// scaling down toward 0 for genuinely dark scenes. The returned value is
// smoothed over actual elapsed wall-clock time (like led_smoother), so
// scene cuts and transitions don't visibly pump the LEDs' brightness up
// and down. Call once per captured frame.
float scene_dimmer_compute_gain(const uint8_t* rgb, int width, int height);
