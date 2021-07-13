/*
 * Compilation: make
 * Usage: ./heat --help
 *
 * This is an initial version and was not throughly reviewed. Informal tests
 * with valgrind show no leaks or errors, but they only cover a narrow set of
 * inputs. See the TODOs and FIXMEs for possible causes of problems.
 */
/* for logging.h */
#define _POSIX_C_SOURCE 200112L
#include "logging.h"
#include "dry.h"
#include "args.h"
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Magic number for the input file */
#define IN_MNUMBER "P2"

/* Magic number for the output files */
#define OUT_MNUMBER "P6"

/* Copy the surface b into surface a, both w x h */
static inline void
copy(double *a, double const *b, DRY(size_t, w, h))
{
  memcpy(a, b, w * h * sizeof(*a));
}

/*
 * Return the max temperature in the w x h surface. We need to do this for
 * every timestep to draw the temperatures correctly. Calculating a theoretical
 * maximum value for the timestep (O(1) instead of O(n)) is not acceptable as
 * the mapping ends up being wrong if it differs from the actual max value.
 *
 * An alternative is to keep track of the max values while calculating the heat
 * on the plate, but that would require synchronization between the threads and
 * add considerable complexity to the code. A quick perf analysis revealed that
 * this function is not a bottleneck for images up to 1000x1000, so leave it
 * like this for now.
 */
static double
max(double const *surface, DRY(size_t, w, h))
{
  double ans = surface[0];
  for (size_t i = 1; i < w * h; i++)
    if (surface[i] > ans)
      ans = surface[i];
  return ans;
}

/*
 * Read a .pgm-like file with the initial state of the plate into a surface
 * (array of doubles), dynamically allocated to the size of the image and
 * returned on success. On error, nothing is allocated and NULL is returned.
 * The error is registered to stderr. The size of the image is read into w,h.
 * On error, this size may or may not be read into the vars.
 *
 * We use a .pgm so the user can create the initial plate state with image
 * editing software. The values of the PGM can actually be doubles, and the
 * maxvalue is not important. The image dimensions should be in (0,
 * sqrt(UINT64_MAX)). There is no control to check if that condition is met.
 *
 * So one can, for instance, draw in GIMP and then user perl to replace the
 * color values written in GIMP with a dict mapping those values to real
 * temperature values (doubles). The magic number is expected to be
 * IN_MNUMBER.
 *
 * No comments allowed in the .pgm - GIMP places one in the header, remove it
 * manually before using this code.
 */
static double *
init(char const *filename, DRY(size_t *, w, h))
{
  double *ans = NULL;
  FILE *f = fopen(filename, "r");
  if (!f) {
    LOG_ERROR("Could not open %s: %s\n", filename, strerror(errno));
    goto init_return;
  }
  char mnumber[3] = { '\0' };
  size_t ignore;
  int rc = fscanf(f, " %2s %zu %zu %zu", mnumber, w, h, &ignore);
  if (rc != 4) {
    LOG_ERROR("%s: Could not read header: %s\n", filename, ferror(f) ?
        strerror(errno) : "EOF");
    goto init_fopen;
  }
  if (strcmp(mnumber, IN_MNUMBER)) {
    LOG_ERROR("%s: Wrong magic number: %s. Expected "IN_MNUMBER"\n", filename,
        mnumber);
    goto init_fopen;
  }
  if (!*w || !*h) {
    LOG_ERROR("Image dimensions should be > 0 (got %zu, %zu)\n", *w, *h);
    goto init_fopen;
  }
  size_t area = *w * *h;
  if (area / *w != *h || (area * sizeof(*ans)) / area != sizeof(*ans)) {
    LOG_ERROR("Image dimensions (%zu, %zu) too large (overflows)\n", *w, *h);
    goto init_fopen;
  }
  ans = malloc(*w * *h * sizeof(*ans));
  if (!ans) {
    LOG_ERROR("%d: %s\n", __LINE__, strerror(errno));
    goto init_fopen;
  }
  for (size_t i = 0; i < *w * *h; i++)
    if (fscanf(f, "%lf ", ans + i) != 1) {
      LOG_ERROR("%s: Reading point %zu: %s\n", filename, i, ferror(f) ?
          strerror(errno) : "EOF");
      goto init_malloc;
    }
  goto init_fopen;
init_malloc:
  free(ans);
  ans = NULL;
init_fopen:
  fclose(f);
init_return:
  return ans;
}

/* Used for the heatmap (color gradient representing the temps) */
struct RGB {
  uint8_t r, g, b;
  double val;
};

/*
 * Flush n w x h surfaces to iter files named itX.pgm, for all X in [0+offset,
 * n+offset). Returns 0 on success, 1 on error, reporting the error to stderr.
 */
static int
flush(double *surfaces, DRY(uint64_t, n, offset), DRY(size_t, w, h))
{
  /* Colder */
  struct RGB blue = { 0, 0, 255, 0.0};
  struct RGB cyan = { 0, 255, 255, 0.25};
  struct RGB green = { 0, 255, 0, 0.5};
  struct RGB yellow = { 255, 255, 0, 0.75};
  struct RGB red = { 255, 0, 0, 1.0};
  /* Hotter */
  struct RGB heatmap[5] = { blue, cyan, green, yellow, red };
  char filename[256];
  for (uint64_t i = 0; i < n; i++) {
    int rc = snprintf(filename, 256, "it%"PRIu64".ppm", i + offset);
    if (rc < 0 || rc >= 256) {
      LOG_ERROR("Generating filename for iter %"PRIu64"\n", i + offset);
      return 1;
    }
    FILE *f = fopen(filename, "w");
    if (!f) {
      LOG_ERROR("Opening %s: %s\n", filename, strerror(errno));
      return 1;
    }
    // FIXME check rc
    fprintf(f, OUT_MNUMBER" %zu %zu 255 ", w, h);
    double mval = max(surfaces + i * w * h, w, h);
    for (size_t j = 0; j < w * h; j++) {
      double p = surfaces[(size_t)i * w * h + j] / mval;
      // TODO we can use our struct for this
      uint8_t rgb[3] = { red.r, red.g, red.b };
      for (uint8_t k = 0; k < 5; k++)
        if (p < heatmap[k].val) {
          struct RGB *prev = heatmap + (k - 1 < 0 ? 0 : k - 1);
          double vdiff = prev->val - heatmap[k].val;
          double diff = vdiff ? (p - heatmap[k].val) / vdiff : 0.0;
          rgb[0] = (uint8_t)((prev->r - heatmap[k].r) * diff + heatmap[k].r);
          rgb[1] = (uint8_t)((prev->g - heatmap[k].g) * diff + heatmap[k].g);
          rgb[2] = (uint8_t)((prev->b - heatmap[k].b) * diff + heatmap[k].b);
          break;
        }
      if (fwrite(rgb, 1, 3, f) != 3) {
        LOG_ERROR("%s: Could not write pixel %zu: %s\n", filename, j,
            strerror(errno));
        if (fclose(f))
          LOG_ERROR("%s: Could not close file: %s\n", filename,
              strerror(errno));
        return 1;
      }
    }
    if (fclose(f)) {
      LOG_ERROR("%s: Could not close file: %s\n", filename, strerror(errno));
      return 1;
    }
  }
  return 0;
}

int
main(int argc, char **argv)
{
  int ans = EXIT_FAILURE;
  struct argp_arguments args;
  memset(&args, 0, sizeof(args));
  args.output = false;
  args.diffusivity = 0.1;
  args.iters = 1000;
  args.spacestep = -1.0;
  args.timestep = -1.0;
  args.bsize = 536870912; /* 512MB */
  struct argp argp = {
    ARGP_OPT, argp_parse_options, ARGP_DOCA, ARGP_DOC, 0, 0, 0
  };
  if (argp_parse(&argp, argc, argv, ARGP_FLAGS, ARGP_INDEX, &args) ==
      ARGP_KEY_ERROR) {
    LOG_CRITICAL("While parsing parameters. Try --help.\n");
    goto main_return;
  }
  size_t w = 0, h = 0;
  /* Checks for w * h * sizeof(*surface) <= SIZE_MAX && w > 0 && h > 0 */
  double *surface = init(args.input[0], &w, &h);
  if (!surface) {
    LOG_CRITICAL("%d: %s\n", __LINE__, strerror(errno));
    goto main_return;
  }
  size_t surface_size = w * h * sizeof(*surface);
  double *osurface = malloc(surface_size);
  if (!osurface) {
    LOG_CRITICAL("%d: %s\n", __LINE__, strerror(errno));
    goto main_surface;
  }
  copy(osurface, surface, w, h);
  // FIXME how is this defined for w != h? Seems to work like this for w > h...
  if (args.spacestep < 0)
    args.spacestep = 1 / (double)w;
  if (args.timestep < 0)
    args.timestep = (args.spacestep * args.spacestep) / (4 * args.diffusivity);
  double alpha = args.diffusivity * (args.timestep / (args.spacestep *
        args.spacestep));
  /*
   * Create a buffer where we will store the per-iter surfaces so we only
   * print the pgms once the buffer is filled (hopefully after the main loop
   * exits)
   */
  // TODO test this buffer thing more extensively, also do a perf analysis on
  // it to see if it really avoids our app being IO-bound
  double *wsurfaces = NULL;
  uint64_t wsurfaces_n = 0;
  if (args.output) {
    wsurfaces_n = args.bsize / surface_size;
    if (!wsurfaces_n) {
      LOG_CRITICAL("Buffer size is too small to fit a single surface.\n");
      goto main_osurface;
    } else {
      // TODO check for overflow?
      wsurfaces = malloc((size_t)(wsurfaces_n) * surface_size);
      if (!wsurfaces) {
        LOG_ERROR("%d: %s\n", __LINE__, strerror(errno));
        goto main_osurface;
      }
    }
    if (wsurfaces_n < args.iters)
      LOG_WARNING("Buffer size is too small to fit all iterations.\n");
  }
  uint64_t wsurfaces_i = 0;
  uint64_t flushes = 0;
  for (uint64_t iters = 0; iters < args.iters; iters++) {
    if (args.output) {
      // TODO check for overflow?
      copy(wsurfaces + (size_t)wsurfaces_i * w * h, surface, w, h);
      if (wsurfaces_i >= wsurfaces_n - 1) {
        LOG_WARNING("Buffer had to be flushed to disk.\n");
        if (flush(wsurfaces, wsurfaces_n, flushes * wsurfaces_n, w, h))
          goto main_wsurface;
        wsurfaces_i = 0;
        flushes++;
      } else {
        wsurfaces_i++;
      }
    }
#pragma omp parallel for collapse(2)
    for (size_t i = 1; i < h - 1; i++) {
      for (size_t j = 1; j < w - 1; j++) {
        size_t center = i * w + j;
        size_t W = center - 1,
               E = center + 1,
               N = center - w,
               S = center + w;
        surface[center] = osurface[center] + alpha * (osurface[E] + osurface[W]
            - 4 * osurface[center] + osurface[S] + osurface[N]);
      }
    }
    copy(osurface, surface, w, h);
  }
  if (wsurfaces_i)
    if (flush(wsurfaces, wsurfaces_i, flushes * wsurfaces_n, w, h))
      goto main_wsurface;
  ans = EXIT_SUCCESS;
main_wsurface:
  if (wsurfaces)
    free(wsurfaces);
main_osurface:
  free(osurface);
main_surface:
  free(surface);
main_return:
  return ans;
}
