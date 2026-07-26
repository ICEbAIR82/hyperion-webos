#pragma once

#include <stdint.h>

// Active picture area within a captured frame, as fractions of the full
// frame (0..1) - the sub-rectangle left over once any detected black
// letterbox (top/bottom) or pillarbox (left/right) bars are excluded.
typedef struct {
    float hmin, hmax, vmin, vmax;
} active_area_t;

// Clears all detection state. Call once when capture (re)starts, so a
// stale border detected from whatever was playing before a stop doesn't
// carry over into a fresh session.
void blackbar_detector_reset(void);

// Analyzes one RGB24 frame (tightly packed, width*height*3 bytes) for black
// bars along each edge and returns the current active picture area.
// Safe to call every frame: internally debounced so the returned area only
// changes after several consecutive frames agree on a new border, avoiding
// a flickering LED mapping during dark scenes, fades, or scene cuts.
active_area_t blackbar_detector_update(const uint8_t* rgb, int width, int height);
