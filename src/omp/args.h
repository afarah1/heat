#pragma once
/* for strtoull */
#define _POSIX_C_SOURCE 200112L
#include <argp.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define ARGP_FLAGS 0
#define ARGP_INDEX 0
#define ARGP_N_ARGS 1
static char const ARGP_DOC[] = "Calculates heat dissipation on a 2D surface "
  "described in a .pgm passed as arg (see heat.c for details).";
static char const ARGP_DOCA[] = "FILENAME";
static struct argp_option const ARGP_OPT[] = {
  {"output", 'o', NULL, OPTION_ARG_OPTIONAL, "Output .ppms.", 0},
  {"buffer", 'b', "BYTES", 0, "Output buffer size (only applicable if called with -o). Default 512MB.", 0},
  {"iterations", 'i', "ITERS", 0, "Number of iterations. Default is 1000.", 0},
  {"spacestep", 'p', "METERS", 0, "Spacestep. Default 1/w.", 0},
  {"diffusivity", 'd', "J/ M3 K", 0, "Diffusivity. Default is 0.1.", 0},
  {"timestep", 's', "SECONDS", 0, "Timestep. Default spacestep2 / (4 diffusivity).", 0},
  { 0 }
};

struct argp_arguments {
#if ARGP_N_ARGS == 0
  char **input;
#else
  char *input[ARGP_N_ARGS];
#endif
  uint64_t iters;
  size_t bsize;
  double timestep, spacestep, diffusivity;
  bool output;
};

#define ASSERTSTRTO(nptr, endptr)\
  do {\
    if (errno || (endptr) == (nptr)) {\
      fprintf(stderr, "Invalid argument: %s. Error: %s.\n", (nptr), errno ?\
          strerror(errno) : "No digits were found");\
      exit(EXIT_FAILURE);\
    }\
  } while(0)

static error_t
argp_parse_options(int key, char *arg, struct argp_state *state)
{
  struct argp_arguments *arguments = (struct argp_arguments *)(state->input);
  char *endptr = NULL;
  errno = 0;
  switch(key) {
    case 'o':
      arguments->output = true;
      break;
    case 's':
      arguments->timestep = strtod(arg, &endptr);
      ASSERTSTRTO(arg, endptr);
      break;
    case 'b':
      if (SIZE_MAX < UINT64_MAX)
        LOG_INFO("SIZE_MAX < UINT64_MAX, buffer size might be casted down\n");
      arguments->bsize = (size_t)strtoull(arg, &endptr, 10);
      ASSERTSTRTO(arg, endptr);
      break;
    case 'p':
      arguments->spacestep = strtod(arg, &endptr);
      ASSERTSTRTO(arg, endptr);
      break;
    case 'd':
      arguments->diffusivity = strtod(arg, &endptr);
      ASSERTSTRTO(arg, endptr);
      break;
    case 'i':
      arguments->iters = (uint64_t)strtoull(arg, &endptr, 10);
      ASSERTSTRTO(arg, endptr);
      break;
    case ARGP_KEY_ARG:
      if (state->arg_num >= ARGP_N_ARGS)
        argp_usage(state);
      arguments->input[state->arg_num] = arg;
      break;
    case ARGP_KEY_END:
      if (state->arg_num < ARGP_N_ARGS)
        argp_usage(state);
      return 0;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
