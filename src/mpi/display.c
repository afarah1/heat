/* ./this N ITERS */
#define _POSIX_C_SOURCE 200112L
#include "args_display.h"
#include "logging.h"
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "graphics.h"
#include <stddef.h>

int
main(int argc, char **argv)
{
  struct argp_arguments args;
  memset(&args, 0, sizeof(args));
  args.simple = false;
  struct argp argp = {
    ARGP_OPT, argp_parse_options, ARGP_DOCA, ARGP_DOC, 0, 0, 0
  };
  if (argp_parse(&argp, argc, argv, ARGP_FLAGS, ARGP_INDEX, &args) ==
      ARGP_KEY_ERROR) {
    fprintf(stderr, "%s, error while parsing parameters\n", argv[0]);
    return EXIT_FAILURE;
  }
  double **surface = malloc((size_t)args.n * sizeof(*surface));
  if (!surface)
    exit(errno);
  for (uint32_t i = 0; i < args.n; i++) {
    surface[i] = malloc((size_t)args.n * sizeof(*(surface[i])));
    if (!surface[i])
      exit(errno);
  }
  graphics_init(args.n);
  FILE *f = fopen("heat.bin", "r");
  if (!f) {
    LOG_CRITICAL("Could not open heat.bin: %s\n", strerror(errno));
    exit(errno);
  }
  uint32_t iter = 0;
  while (iter < args.iters) {
    for (uint32_t i = 0; i < args.n; i++)
      if (fread(surface[i], sizeof(surface[0][0]), args.n, f) != args.n) {
        LOG_CRITICAL("Could not read line %"PRIu32" iter %"PRIu32": %s\n", i, iter, strerror(errno));
        exit(errno);
      }
    if (args.simple)
			graphics_draw2(surface, args.n);
		else
			graphics_draw5(surface, args.n);
    iter++;
  }
  fclose(f);
  graphics_end();
  free(surface);
  return 0;
}
