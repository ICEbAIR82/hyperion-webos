#include "blackbar_detector.h"
#include <stddef.h>

// A pixel counts as "black" if every channel is below this.
#define BLACK_PIXEL_THRESHOLD 20
// A row/column counts as part of a bar if at least this fraction of its
// pixels are black - tolerates subtitles, film grain, slight compression
// noise, without missing genuine letterbox/pillarbox bars.
#define BLACK_LINE_FRACTION 0.98f
// Never crop away more than this fraction of a dimension - protects against
// misreading a genuinely dark scene (or a fully black frame) as a full-frame
// letterbox, which would otherwise crop away most or all of the picture.
#define MAX_BAR_FRACTION 0.25f
// A newly detected border size must repeat for this many consecutive frames
// before it's actually applied to the LED mapping, so a momentary dark
// scene or scene-cut doesn't visibly jitter the LED edges.
#define STABILITY_FRAMES 30

static int _stable_top = 0, _stable_bottom = 0, _stable_left = 0, _stable_right = 0;
static int _candidate_top = 0, _candidate_bottom = 0, _candidate_left = 0, _candidate_right = 0;
static int _candidate_frames = 0;

void blackbar_detector_reset(void)
{
    _stable_top = _stable_bottom = _stable_left = _stable_right = 0;
    _candidate_top = _candidate_bottom = _candidate_left = _candidate_right = 0;
    _candidate_frames = 0;
}

static int row_is_black(const uint8_t* rgb, int width, int y)
{
    const uint8_t* row = rgb + (size_t)y * (size_t)width * 3;
    int black_count = 0;
    for (int x = 0; x < width; x++) {
        int r = row[x * 3 + 0], g = row[x * 3 + 1], b = row[x * 3 + 2];
        if (r < BLACK_PIXEL_THRESHOLD && g < BLACK_PIXEL_THRESHOLD && b < BLACK_PIXEL_THRESHOLD)
            black_count++;
    }
    return black_count >= (int)((float)width * BLACK_LINE_FRACTION);
}

static int col_is_black(const uint8_t* rgb, int width, int height, int x)
{
    int black_count = 0;
    for (int y = 0; y < height; y++) {
        const uint8_t* px = rgb + ((size_t)y * (size_t)width + (size_t)x) * 3;
        if (px[0] < BLACK_PIXEL_THRESHOLD && px[1] < BLACK_PIXEL_THRESHOLD && px[2] < BLACK_PIXEL_THRESHOLD)
            black_count++;
    }
    return black_count >= (int)((float)height * BLACK_LINE_FRACTION);
}

static int count_top(const uint8_t* rgb, int width, int height)
{
    int max_bar = (int)((float)height * MAX_BAR_FRACTION);
    int n = 0;
    while (n < max_bar && row_is_black(rgb, width, n))
        n++;
    return n;
}

static int count_bottom(const uint8_t* rgb, int width, int height)
{
    int max_bar = (int)((float)height * MAX_BAR_FRACTION);
    int n = 0;
    while (n < max_bar && row_is_black(rgb, width, height - 1 - n))
        n++;
    return n;
}

static int count_left(const uint8_t* rgb, int width, int height)
{
    int max_bar = (int)((float)width * MAX_BAR_FRACTION);
    int n = 0;
    while (n < max_bar && col_is_black(rgb, width, height, n))
        n++;
    return n;
}

static int count_right(const uint8_t* rgb, int width, int height)
{
    int max_bar = (int)((float)width * MAX_BAR_FRACTION);
    int n = 0;
    while (n < max_bar && col_is_black(rgb, width, height, width - 1 - n))
        n++;
    return n;
}

active_area_t blackbar_detector_update(const uint8_t* rgb, int width, int height)
{
    if (width > 0 && height > 0) {
        int top = count_top(rgb, width, height);
        int bottom = count_bottom(rgb, width, height);
        int left = count_left(rgb, width, height);
        int right = count_right(rgb, width, height);

        // A frame that's almost entirely black (fade-to-black, black scene)
        // isn't a reliable letterbox reading either way - ignore it rather
        // than let it drag the stable border toward a near-total crop.
        int horizontal_valid = (top + bottom) < (int)((float)height * 0.9f);
        int vertical_valid = (left + right) < (int)((float)width * 0.9f);

        if (!horizontal_valid) {
            top = _candidate_top;
            bottom = _candidate_bottom;
        }
        if (!vertical_valid) {
            left = _candidate_left;
            right = _candidate_right;
        }

        if (top == _candidate_top && bottom == _candidate_bottom
            && left == _candidate_left && right == _candidate_right) {
            if (_candidate_frames < STABILITY_FRAMES)
                _candidate_frames++;
        } else {
            _candidate_top = top;
            _candidate_bottom = bottom;
            _candidate_left = left;
            _candidate_right = right;
            _candidate_frames = 1;
        }

        if (_candidate_frames >= STABILITY_FRAMES) {
            _stable_top = _candidate_top;
            _stable_bottom = _candidate_bottom;
            _stable_left = _candidate_left;
            _stable_right = _candidate_right;
        }
    }

    active_area_t area;
    area.vmin = height > 0 ? (float)_stable_top / (float)height : 0.0f;
    area.vmax = height > 0 ? 1.0f - (float)_stable_bottom / (float)height : 1.0f;
    area.hmin = width > 0 ? (float)_stable_left / (float)width : 0.0f;
    area.hmax = width > 0 ? 1.0f - (float)_stable_right / (float)width : 1.0f;
    return area;
}
