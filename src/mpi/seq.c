#include "args.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "shared.c"

#define TAG 1

// copy b into a
// TODO memcpy
static void
copy(double *a, double *b, uint32_t n)
{
  memcpy(a, b, n * n * sizeof(*a));
}

// Initialize the surface with initial and boundary conditions
static void
init(double *surface, uint32_t n)
{
  for (uint32_t i = 0; i < n; i++)
    for (uint32_t j = 0; j < n; j++)
      // Boundary condition: Zero at the edges
      if (i == n - 1 || i == 0 || j == 0 || j == n - 1)
        surface[i * n + j] = 10.0;
      // Initial condition: 10
      else
        surface[i * n + j] = 0.0;
}

static void
write(FILE *f, double *surface, uint32_t n)
{
  if (fwrite(surface, sizeof(*surface), n * n, f) != n * n)
    exit(errno);
}

int
main(int argc, char **argv)
{
  struct argp_arguments args;
  memset(&args, 0, sizeof(args));
  args.time = false;
  args.output = false;
  args.diffusivity = 0.1;
  args.n = 100;
  args.spacestep = 1 / (double)args.n;
  args.timestep = (args.spacestep * args.spacestep) / (4 * args.diffusivity);
  args.iters = 3000;
  struct argp argp = {
    ARGP_OPT, argp_parse_options, ARGP_DOCA, ARGP_DOC, 0, 0, 0
  };
  if (argp_parse(&argp, argc, argv, ARGP_FLAGS, ARGP_INDEX, &args) ==
      ARGP_KEY_ERROR) {
    fprintf(stderr, "%s, error while parsing parameters\n", argv[0]);
    return EXIT_FAILURE;
  }
  double alpha = args.diffusivity * (args.timestep / (args.spacestep * args.spacestep));

  FILE *f = fopen("heat.bin", "w");
  if (!f)
    exit(errno);
	double *surface = malloc((size_t)(args.n * args.n) * sizeof(*surface));
  if (!surface)
    exit(errno);
  double *old_surface = malloc((size_t)(args.n * args.n) * sizeof(*old_surface));
  if (!old_surface)
    exit(errno);
  init(surface, args.n);
  copy(old_surface, surface, args.n);
	for (uint32_t iters = 0; iters < args.iters; iters++) {
      for (uint32_t i = 1; i < args.n - 1; i++) {
        for (uint32_t j = 1; j < args.n - 1; j++) {
          uint32_t center = i * args.n + j;
          uint32_t W = center - 1,
                   E = center + 1,
                   N = center - args.n,
                   S = center + args.n;
          surface[center] = old_surface[center] + alpha * (old_surface[E] +
              old_surface[W] - 4 * old_surface[center] + old_surface[S] +
              old_surface[N]);
        }
      }
    copy(old_surface, surface, args.n);
    write(f, surface, args.n);
  }
  free(surface);
  free(old_surface);
  if (fclose(f))
    exit(errno);
  return EXIT_SUCCESS;
}
