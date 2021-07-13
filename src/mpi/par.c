#include "args.h"
#include <mpi.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "shared.c"

#define BOUNDARY 10.0
#define INITIAL 0.0

// ad-hoc copy for the rank surface which does not copy ghost rows
static inline void
copy(double *a, double *b, size_t w, size_t h)
{
  memcpy(a + w, b + w, w * (h - 2) * sizeof(*a));
}

// ad-hoc initializer for rank surface (w, h include ghost rows)
static void
init(double *surface, size_t w, size_t h)
{
  // skip ghost rows
  for (size_t i = 1; i < h - 1; i++) {
    surface[i * w] = surface[(i + 1) * w - 1] = BOUNDARY;
    for (size_t j = 1; j < w - 1; j++)
      surface[i * w + j] = INITIAL;
  }
}

static void
write(FILE *f, double *surface, size_t n)
{
  size_t total = n * n;
  if (fwrite(surface, sizeof(*surface), total, f) != total)
    MPI_Abort(MPI_COMM_WORLD, errno);;
}

#define WORLD MPI_COMM_WORLD
#define TAG 0

#define RECUR(w_)\
  do{\
    for (int j = 1; j < (w_) - 1; j++) {\
      int center = i * args.n + j;\
      int W = center - 1,\
          E = center + 1,\
          N = center - args.n,\
          S = center + args.n;\
      surface[center] = old_surface[center] + alpha * (old_surface[E] +\
          old_surface[W] - 4 * old_surface[center] + old_surface[S] +\
          old_surface[N]);\
    }\
  }while(0)

/* Northen send index */
#define NS 3
/* Northen recv index */
#define NR 1
/* Northen recv2 index */
#define NR2 2
/* Northen send2 index */
#define NS2 5
/* Sourther send index */
#define SS 4
/* Sourther recv index */
#define SR 0

int
main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  struct argp_arguments args;
  memset(&args, 0, sizeof(args));
  args.time = false;
  args.output = false;
  args.diffusivity = 0.1;
  args.n = 100;
  args.iters = 3000;
  args.spacestep = -1.0;
  args.timestep = -1.0;
  struct argp argp = {
    ARGP_OPT, argp_parse_options, ARGP_DOCA, ARGP_DOC, 0, 0, 0
  };
  if (argp_parse(&argp, argc, argv, ARGP_FLAGS, ARGP_INDEX, &args) ==
      ARGP_KEY_ERROR) {
    fprintf(stderr, "%s, error while parsing parameters\n", argv[0]);
    return EXIT_FAILURE;
  }
  if (args.spacestep < 0)
    args.spacestep = 1 / (double)args.n;
  if (args.timestep < 0)
    args.timestep = (args.spacestep * args.spacestep) / (4 * args.diffusivity);
  double alpha = args.diffusivity * (args.timestep / (args.spacestep * args.spacestep));
  FILE *f = fopen("heat.bin", "w");
  if (!f)
    MPI_Abort(WORLD, errno);;
  int rank, world_size;
  MPI_Comm_rank(WORLD, &rank);
  MPI_Comm_size(WORLD, &world_size);
  int rpr = (args.n - 2) / world_size;
  int remaining = (args.n - 2) - (rpr * world_size);
  double *esurface = NULL, *eold_surface = NULL;
  if (remaining && !rank) {
    esurface = malloc((size_t)((remaining + 2) * args.n) * sizeof(*esurface));
    if (!esurface)
      MPI_Abort(WORLD, errno);;
    eold_surface = malloc((size_t)((remaining + 2) * args.n) * sizeof(*eold_surface));
    if (!eold_surface)
      MPI_Abort(WORLD, errno);;
    init(esurface, (size_t)args.n, (size_t)(remaining + 2));
    copy(eold_surface, esurface, (size_t)args.n, (size_t)(remaining + 2));
  }
  double *surface = malloc((size_t)((rpr + 2) * args.n) * sizeof(*surface));
  if (!surface)
    MPI_Abort(WORLD, errno);;
  double *old_surface = malloc((size_t)((rpr + 2) * args.n) * sizeof(*old_surface));
  if (!old_surface)
    MPI_Abort(WORLD, errno);;
  double *wsurface = NULL;
  if (rank == 0) {
    wsurface = malloc((size_t)(args.n * args.n) * sizeof(*wsurface));
    for (int j = 0; j < args.n; j++)
      wsurface[j] = wsurface[(args.n - 1) * args.n + j] = BOUNDARY;
    if (!wsurface)
      MPI_Abort(WORLD, errno);;
  }
  init(surface, (size_t)args.n, (size_t)(rpr + 2));
  copy(old_surface, surface, (size_t)args.n, (size_t)(rpr + 2));
  /* Ghost boundaries */
  if (!rank) {
    for (int j = 0; j < args.n; j++)
      surface[j] = old_surface[j] = BOUNDARY;
    if (remaining)
      for (int j = 0; j < args.n; j++)
        esurface[(remaining + 1) * args.n + j] = eold_surface[(remaining + 1) * args.n + j] = BOUNDARY;
  } else if (rank == world_size - 1 && !remaining) {
    for (int j = 0; j < args.n; j++)
      surface[(rpr + 1) * args.n + j] = old_surface[(rpr + 1) * args.n + j] = BOUNDARY;
  }
  for (int iters = 0; iters < args.iters; iters++) {
    // TODO improve this array thing
    MPI_Request requests[6];
    if (rank) {
      /* Get our upper ghost row from the northen rank */
      MPI_Irecv(old_surface + 1, args.n - 2, MPI_DOUBLE, rank - 1, TAG, WORLD, requests + NR);
      /* Send our upper dep row to the northen rank */
      MPI_Isend(old_surface + args.n + 1, args.n - 2, MPI_DOUBLE, rank - 1, TAG, WORLD, requests + NS);
    }
    if (rank != world_size - 1 || remaining) {
      int south = rank + 1 == world_size ? 0 : rank + 1;
      /* Get our bottom ghost row from the southern rank */
      MPI_Irecv(old_surface + (rpr + 1) * args.n + 1, args.n - 2, MPI_DOUBLE, south, TAG, WORLD, requests + SR);
      /* Send our bottom dep row to the southern rank */
      MPI_Isend(old_surface + rpr * args.n + 1, args.n - 2, MPI_DOUBLE, south, TAG, WORLD, requests + SS);
    }
    /* Calculate heat within rank submatrix except on dep rows */
    /* Skip ghost rows */
    for (int i = 2; i < rpr; i++)
      RECUR(args.n);
    /* Master has to work extra if there are remaining rows */
    if (!rank && remaining) {
      double *kludge = surface, *okludge = old_surface;
      surface = esurface; old_surface = eold_surface;
      /* Skip ghost and dep rows */
      for (int i = 2; i < remaining; i++)
        RECUR(args.n);
      /* Get our upper ghost row from the northen rank */
      MPI_Irecv(old_surface + 1, args.n - 2, MPI_DOUBLE, world_size - 1, TAG, WORLD, requests + NR2);
      /* Send our upper dep row to the northen rank */
      MPI_Isend(old_surface + args.n + 1, args.n - 2, MPI_DOUBLE, world_size - 1, TAG, WORLD, requests + NS2);
      /* Calculate heat on dep rows once we recv them */
      int i = remaining;
      RECUR(args.n);
      MPI_Wait(requests + NR2, MPI_STATUS_IGNORE);
      i = 1;
      RECUR(args.n);
      copy(eold_surface, esurface, (size_t)args.n, (size_t)(remaining + 2));
      surface = kludge; old_surface = okludge;
    }
    /* Calculate heat on dep rows once we recv them */
    if (rank && (rank != world_size - 1 || remaining)) {
      int ready;
      int done = 0;
      while (done < 2) {
        MPI_Waitany(2, requests, &ready, MPI_STATUS_IGNORE);
        int i = ready == NR ? 1 : rpr;
        RECUR(args.n);
        done++;
      }
      /* Notice the first and last rank do not have to wait for N/S row */
    } else if (!rank) {
      int i = 1;
      RECUR(args.n);
      MPI_Wait(requests + SR, MPI_STATUS_IGNORE);
      i = rpr;
      RECUR(args.n);
    } else {
      int i = rpr;
      RECUR(args.n);
      MPI_Wait(requests + NR, MPI_STATUS_IGNORE);
      i = 1;
      RECUR(args.n);
    }
    copy(old_surface, surface, (size_t)args.n, (size_t)(rpr + 2));
    // TODO put all this in a buffer and send to master at the end
    /* Send info to master */
    if (rank) {
      MPI_Request sss;
      /* Don't send ghost rows */
      MPI_Isend(surface + args.n, rpr * args.n, MPI_DOUBLE, 0, TAG, WORLD, &sss);
    } else {
      MPI_Request sss[world_size - 1];
      memcpy(wsurface + args.n, surface + args.n, (size_t)(rpr * args.n) * sizeof(*wsurface));
      if (remaining)
        memcpy(wsurface + (world_size * rpr + 1) * args.n, esurface + args.n, (size_t)(remaining * args.n) * sizeof(*wsurface));
      for (int r = 1; r < world_size; r++)
        MPI_Irecv(wsurface + (r * rpr + 1) * args.n, rpr * args.n, MPI_DOUBLE, r, TAG, WORLD, sss + r - 1);
      MPI_Waitall(world_size - 1, sss, MPI_STATUS_IGNORE);
      write(f, wsurface, (size_t)args.n);
    }
  }
  // todo remaining rows
  free(surface);
  free(old_surface);
  if (fclose(f))
    MPI_Abort(WORLD, errno);;
  MPI_Finalize();
  return EXIT_SUCCESS;
}
