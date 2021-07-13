/* for logging.h */
#define _POSIX_C_SOURCE 200112L
#include "graphics.h"
#include "logging.h"
#include <assert.h>
#include <SDL.h>
#include <stdint.h>
#include "shared.c"

static SDL_Window *WINDOW = NULL;
static SDL_Renderer *RENDERER = NULL;

void
graphics_init(uint32_t n)
{
  assert(!WINDOW && !RENDERER);
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    LOG_CRITICAL("On init: %s\n", SDL_GetError());
    goto init;
  }
  SDL_CreateWindowAndRenderer(n, n, 0, &WINDOW, &RENDERER);
  if (!WINDOW || !RENDERER) {
    LOG_CRITICAL("On window creation: %s\n", SDL_GetError());
    goto init;
  }
  goto success;
init:
  SDL_Quit();
success:
  return;
}

struct RGB {
  uint8_t r, g, b;
  double val;
};

int
graphics_draw2(double **surface, uint32_t n)
{
  if (SDL_SetRenderDrawColor(RENDERER, 0, 0, 0, SDL_ALPHA_OPAQUE)) {
    LOG_ERROR("While setting the renderer color: %s\n", SDL_GetError());
    return 1;
  }
  if (SDL_RenderClear(RENDERER)) {
    LOG_ERROR("While clearing the renderer: %s\n", SDL_GetError());
    return 1;
  }
  double mval = 10.0;//max(surface, n);
  //mval = mval <= 0 ? 1 : mval;
  for (uint32_t i = 0; i < n; i++) {
    for (uint32_t j = 0; j < n; j++) {
      double p = surface[i][j] / mval;
      uint8_t red = (uint8_t)(255 * p);
      uint8_t blue = (uint8_t)(255 * (1 - p));
      if (SDL_SetRenderDrawColor(RENDERER, red, 0, blue, SDL_ALPHA_OPAQUE)) {
        //if (SDL_SetRenderDrawColor(RENDERER, p, p, p, SDL_ALPHA_OPAQUE)) {
        LOG_ERROR("While drawing color with: %s\n", SDL_GetError());
        return 1;
      }
      if (SDL_RenderDrawPoint(RENDERER, j, i) < 0) {
        LOG_ERROR("While drawing point %"PRIu32" %"PRIu32": %s\n", i, j,
            SDL_GetError());
        return 1;
      }
    }
  }
  SDL_RenderPresent(RENDERER);
  return 0;
}


int
graphics_draw5(double **surface, uint32_t n)
{
  if (SDL_SetRenderDrawColor(RENDERER, 0, 0, 0, SDL_ALPHA_OPAQUE)) {
    LOG_ERROR("While setting the renderer color: %s\n", SDL_GetError());
    return 1;
  }
  if (SDL_RenderClear(RENDERER)) {
    LOG_ERROR("While clearing the renderer: %s\n", SDL_GetError());
    return 1;
  }
  double mval = max(surface, n);
  mval = mval <= 0 ? 1 : mval;
  // Cooler
  struct RGB blue = { 0, 0, 255, 0.0};
  struct RGB cyan = { 0, 255, 255, 0.25};
  struct RGB green = { 0, 255, 0, 0.5};
  struct RGB yellow = { 255, 255, 0, 0.75};
  struct RGB red = { 255, 0, 0, 1.0};
  // Hotter
  struct RGB heatmap[5] = { blue, cyan, green, yellow, red };
  for (uint32_t i = 0; i < n; i++) {
    for (uint32_t j = 0; j < n; j++) {
      // Scale the point to a position in the gradiend
      double p = surface[i][j] / mval;
      uint8_t r = red.r, g = red.g, b = red.b;
      for (uint8_t k = 0; k < 5; k++)
        if (p < heatmap[k].val) {
          struct RGB *prev = heatmap + (k - 1 < 0 ? 0 : k - 1);
          double vdiff = prev->val - heatmap[k].val;
          double diff = vdiff ? (p - heatmap[k].val) / vdiff : 0.0;
          r = (uint8_t)((prev->r - heatmap[k].r) * diff + heatmap[k].r);
          g = (uint8_t)((prev->g - heatmap[k].g) * diff + heatmap[k].g);
          b = (uint8_t)((prev->b - heatmap[k].b) * diff + heatmap[k].b);
          break;
        }
      if (SDL_SetRenderDrawColor(RENDERER, r, g, b, SDL_ALPHA_OPAQUE)) {
        //if (SDL_SetRenderDrawColor(RENDERER, p, p, p, SDL_ALPHA_OPAQUE)) {
        LOG_ERROR("While drawing color with: %s\n", SDL_GetError());
        return 1;
      }
      if (SDL_RenderDrawPoint(RENDERER, j, i) < 0) {
        LOG_ERROR("While drawing point %"PRIu32" %"PRIu32": %s\n", i, j,
            SDL_GetError());
        return 1;
      }
    }
  }
  SDL_RenderPresent(RENDERER);
  return 0;
}

void
graphics_end()
{
  if (RENDERER)
    SDL_DestroyRenderer(RENDERER);
  if (WINDOW)
    SDL_DestroyWindow(WINDOW);
  SDL_Quit();
}
