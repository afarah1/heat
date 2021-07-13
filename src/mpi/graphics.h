#pragma once
#include <stdint.h>

/* Initializes the graphics environment. On failure, don't init anything. */
void
graphics_init(uint32_t n);

/* Draws the surface using a 5-color heatmap. Returns 0 on success, 1 on failure. */
int
graphics_draw5(double **surface, uint32_t n);

/* Draws the surface using a 2-color heatmap. Returns 0 on success, 1 on failure. */
int
graphics_draw2(double **surface, uint32_t n);


/* Ends the graphics environment. Fail-silent. */
void
graphics_end();
