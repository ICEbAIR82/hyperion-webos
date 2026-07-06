#include "led_mapper.h"
#include "led_layout.h"
#include <stddef.h>

static int _clamp(int v, int lo, int hi)
{
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}

void led_mapper_compute(const uint8_t* rgb, int width, int height,
    const led_region_t* layout, int led_count, uint8_t* out_led_rgb)
{
    for (int i = 0; i < led_count; i++) {
        const led_region_t* region = &layout[i];

        int x0 = _clamp((int)(region->hmin * width), 0, width);
        int x1 = _clamp((int)(region->hmax * width + 0.5f), 0, width);
        int y0 = _clamp((int)(region->vmin * height), 0, height);
        int y1 = _clamp((int)(region->vmax * height + 0.5f), 0, height);

        if (x1 <= x0)
            x1 = _clamp(x0 + 1, 0, width);
        if (y1 <= y0)
            y1 = _clamp(y0 + 1, 0, height);

        uint32_t sum_r = 0, sum_g = 0, sum_b = 0;
        uint32_t count = 0;

        for (int y = y0; y < y1; y++) {
            const uint8_t* row = rgb + (size_t)y * width * 3;
            for (int x = x0; x < x1; x++) {
                sum_r += row[x * 3 + 0];
                sum_g += row[x * 3 + 1];
                sum_b += row[x * 3 + 2];
                count++;
            }
        }

        if (count == 0) {
            out_led_rgb[i * 3 + 0] = 0;
            out_led_rgb[i * 3 + 1] = 0;
            out_led_rgb[i * 3 + 2] = 0;
        } else {
            out_led_rgb[i * 3 + 0] = (uint8_t)(sum_r / count);
            out_led_rgb[i * 3 + 1] = (uint8_t)(sum_g / count);
            out_led_rgb[i * 3 + 2] = (uint8_t)(sum_b / count);
        }
    }
}
