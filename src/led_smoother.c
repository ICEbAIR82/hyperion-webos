#include "led_smoother.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

static float* _prev = NULL;
static int _count = 0;
static struct timespec _last_ts;
static bool _has_last = false;

int led_smoother_init(int led_count)
{
    led_smoother_close();

    _prev = calloc((size_t)led_count * 3, sizeof(float));
    if (!_prev)
        return -1;

    _count = led_count;
    _has_last = false;
    return 0;
}

static double _alpha_for(double dt, int time_ms)
{
    if (time_ms <= 0)
        return 1.0;

    double tau = time_ms / 1000.0;
    double alpha = 1.0 - exp(-dt / tau);
    if (alpha > 1.0)
        alpha = 1.0;
    if (alpha < 0.0)
        alpha = 0.0;
    return alpha;
}

void led_smoother_apply(uint8_t* led_rgb, int led_count, int rise_time_ms, int fall_time_ms)
{
    if (_prev == NULL || led_count != _count)
        return;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if ((rise_time_ms <= 0 && fall_time_ms <= 0) || !_has_last) {
        for (int i = 0; i < led_count * 3; i++)
            _prev[i] = (float)led_rgb[i];
        _last_ts = now;
        _has_last = true;
        return;
    }

    double dt = (double)(now.tv_sec - _last_ts.tv_sec) + (double)(now.tv_nsec - _last_ts.tv_nsec) / 1e9;
    _last_ts = now;

    double rise_alpha = _alpha_for(dt, rise_time_ms);
    double fall_alpha = _alpha_for(dt, fall_time_ms);

    for (int i = 0; i < led_count * 3; i++) {
        double target = led_rgb[i];
        double alpha = target >= _prev[i] ? rise_alpha : fall_alpha;
        _prev[i] = (float)(alpha * target + (1.0 - alpha) * _prev[i]);
        led_rgb[i] = (uint8_t)(_prev[i] + 0.5f);
    }
}

void led_smoother_hold(void)
{
    if (_prev == NULL)
        return;

    // Only the clock moves - _prev is deliberately left alone. The buffer the
    // caller is holding has white balance and scene dimming applied on top of
    // these smoothed values, so feeding it back in here would drag _prev
    // towards the already-corrected colors and double-apply those gains once
    // real frames resume.
    clock_gettime(CLOCK_MONOTONIC, &_last_ts);
}

void led_smoother_close(void)
{
    free(_prev);
    _prev = NULL;
    _count = 0;
    _has_last = false;
}
