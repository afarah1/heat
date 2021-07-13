/* for strtod */
#define _POSIX_C_SOURCE 200112L
#include <argp.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define ARGP_FLAGS 0
#define ARGP_INDEX 0
#define ARGP_ARGS 2
static char const ARGP_DOC[] = "Displays the heat dissipation";
static char const ARGP_DOCA[] = "SIZE ITERS";
static struct argp_option const ARGP_OPT[] = {
  {"simple", 's', NULL, OPTION_ARG_OPTIONAL, "Use a 2-color heatmap instead of "
    "the standard 5-color one.", 0},
  { 0 }
};

struct argp_arguments {
#if ARGP_ARGS > 0
  char *input[ARGP_ARGS];
#endif
  uint32_t n, iters, input_size;
  bool simple;
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
  /* FIXME assumes some values > 0 and sizeof(size_t) <= sizeof(long) */
  struct argp_arguments *arguments = (struct argp_arguments *)(state->input);
  char *endptr = NULL;
  errno = 0;
  switch(key) {
    case 's':
      arguments->simple = true;
      break;
    case ARGP_KEY_ARG:
			switch(arguments->input_size) {
        case 0:
					arguments->n = (uint32_t)strtoul(arg, &endptr, 10);
          ASSERTSTRTO(arg, endptr);
          break;
        case 1:
					arguments->iters = (uint32_t)strtoul(arg, &endptr, 10);
          ASSERTSTRTO(arg, endptr);
          break;
				case ARGP_ARGS:
          argp_usage(state);
      }
			arguments->input_size++;
			break;
		case ARGP_KEY_END:
			if (state->arg_num < ARGP_ARGS)
				argp_usage(state);
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}
