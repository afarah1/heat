/* for strtod */
#define _POSIX_C_SOURCE 200112L
#include <argp.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define ARGP_FLAGS 0
#define ARGP_INDEX 0
#define ARGP_MAX_ARGS 0
static char const ARGP_DOC[] = "Calculates the heat equation on a 2D surface, "
  "outputting the result to stdout";
static char const ARGP_DOCA[] = " ";
static struct argp_option const ARGP_OPT[] = {
  {"time", 't', NULL, OPTION_ARG_OPTIONAL, "Output a elapsed time to stdout.", 0},
  {"output", 'o', NULL, OPTION_ARG_OPTIONAL, "Output a .pgm to stdout.", 0},
  {"resolution", 'n', "UNITS", 0, "The surface is the unit square, to be "
    "represented by a nxn matrix. Default: 100.", 0},
  {"iterations", 'i', "ITERS", 0, "Number of iterations. Default is 3000.", 0},
  {"spacestep", 'p', "METERS", 0, "Spacestep in meters. Default 1/(n+2).", 0},
  {"diffusivity", 'd', "J/ M3 K", 0, "Diffusivity in J/M3 K. Default is 0.1.", 0},
  {"timestep", 's', "SECONDS", 0, "Timestep in seconds. Default spacestep2 / "
    "(4 diffusivity).", 0},
  { 0 }
};

struct argp_arguments {
#if ARGP_MAX_ARGS == 0
  char **input;
#else
  char *input[ARGP_MAX_ARGS];
#endif
  int n, iters;
  double timestep, spacestep, diffusivity;
  bool output, time;
};

#define ASSERTSTRTO(nptr, endptr)\
  do {\
    if (errno || (endptr) == (nptr)) {\
      fprintf(stderr, "Invalid argument: %s. Error: %s.\n", (nptr), errno ?\
          strerror(errno) : "No digits were found");\
      exit(EXIT_FAILURE);\
    }\
  } while(0)

// FIXME assert positives
static error_t
argp_parse_options(int key, char *arg, struct argp_state *state)
{
  /* FIXME assumes some values > 0 and sizeof(size_t) <= sizeof(long) */
  struct argp_arguments *arguments = (struct argp_arguments *)(state->input);
  char *endptr = NULL;
  errno = 0;
  switch(key) {
    case 'o':
      arguments->output = true;
      break;
    case 't':
      arguments->time = true;
      break;
    case 's':
      arguments->timestep = strtod(arg, &endptr);
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
    case 'n':
      arguments->n = (int)strtol(arg, &endptr, 10);
      ASSERTSTRTO(arg, endptr);
      break;
    case 'i':
      arguments->iters = (int)strtol(arg, &endptr, 10);
      ASSERTSTRTO(arg, endptr);
      break;
    case ARGP_KEY_ARG:
      argp_usage(state);
      break;
    case ARGP_KEY_END:
      return 0;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
