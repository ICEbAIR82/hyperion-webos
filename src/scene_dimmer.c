#include "scene_dimmer.h"
#include <math.h>
#include <stdbool.h>
#include <time.h>

// Average frame luminance (0-255) at/above which the scene counts as
// "bright enough" for full LED brightness. Below this, gain scales down
// linearly toward 0 for a fully black frame.
#define DARK_THRESHOLD 90.0f

// Fixed smoothing time constants (not user-configurable, kept simple).
// Asymmetric like the LED color smoother: brightening back up after a dark
// scene ends can fade in gently, but a cut to a dark scene should dim down
// a bit more promptly.
#define GAIN_RISE_TIME_MS 800
#define GAIN_FALL_TIME_MS 400

static float _smoothed_gain = 1.0f;
static struct timespec _last_ts;
static bool _has_last = false;

void scene_dimmer_reset(void)
{
    _smoothed_gain = 1.0f;
    _has_last = false;
}

static double alpha_for(double dt, int time_ms)
{
    double tau = time_ms / 1000.0;
    double alpha = 1.0 - exp(-dt / tau);
    if (alpha > 1.0)
        alpha = 1.0;
    if (alpha < 0.0)
        alpha = 0.0;
    return alpha;
}

float scene_dimmer_compute_gain(const uint8_t* rgb, int width, int height)
{
    if (width <= 0 || height <= 0)
        return _smoothed_gain;

    size_t pixel_count = (size_t)width * (size_t)height;
    uint64_t sum = 0;
    for (size_t i = 0; i < pixel_count; i++) {
        sum += rgb[i * 3 + 0] + rgb[i * 3 + 1] + rgb[i * 3 + 2];
    }
    float avg_luminance = (float)sum / (float)(pixel_count * 3);

    float target_gain = avg_luminance / DARK_THRESHOLD;
    if (target_gain > 1.0f)
        target_gain = 1.0f;
    if (target_gain < 0.0f)
        target_gain = 0.0f;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (!_has_last) {
        _smoothed_gain = target_gain;
        _last_ts = now;
        _has_last = true;
        return _smoothed_gain;
    }

    double dt = (double)(now.tv_sec - _last_ts.tv_sec) + (double)(now.tv_nsec - _last_ts.tv_nsec) / 1e9;
    _last_ts = now;

    int time_ms = target_gain >= _smoothed_gain ? GAIN_RISE_TIME_MS : GAIN_FALL_TIME_MS;
    double alpha = alpha_for(dt, time_ms);
    _smoothed_gain = (float)(alpha * target_gain + (1.0 - alpha) * _smoothed_gain);

    return _smoothed_gain;
}
