This repository contains code to calculate heat diffusion on a two-dimensional
surface, with both OpenMP and MPI implementations for parallel processing.

The OpenMP version takes as input a `.pgm` file describing the initial state of
the surface and outputs either the time it took to compute the diffusion or the
state of the plate at every iteration (see Usage, below). An example is
provided below with the initial state of a plate and the state after 100 and
500 iterations.

![A png montage demonstrating the output](https://github.com/afarah1/heat/raw/master/example_output.png "Example output")

The MPI version uses a fixed input and has a fixed binary output, and provides
an interface for reading the results and an SDL implementation to display it as
a sequence of images.

This was done for a uni assignment and while I was exploring some concepts
such as macros, interfaces, [goto
chains](https://wiki.sei.cmu.edu/confluence/display/c/MEM12-C.+Consider+using+a+goto+chain+when+leaving+a+function+on+error+when+using+and+releasing+resources),
and SDL, hence the different behavior for each implementation and the messy
code. I was told it's useful for performance analysis so I'm publishing it (under
public domain). Tests are lacking, but I think it's reasonably well documented.

# Compilation

Just run `make`.

Besides OpenMP and OpenMPI, the only dependencies are SDL and pkg-config for
the MPI version (see the Makefile for details).

# Usage

## OpenMP

```
Usage: heat [OPTION...] FILENAME
Calculates heat dissipation on a 2D surface described in a .pgm passed as arg
(see heat.c for details).

  -b, --buffer=BYTES         Output buffer size (only applicable if called with
                             -o). Default 512MB.
  -d, --diffusivity=J/ M3 K  Diffusivity. Default is 0.1.
  -i, --iterations=ITERS     Number of iterations. Default is 1000.
  -o, --output               Output .ppms.
  -p, --spacestep=METERS     Spacestep. Default 1/w.
  -s, --timestep=SECONDS     Timestep. Default spacestep2 / (4 diffusivity).
  -?, --help                 Give this help list
      --usage                Give a short usage message

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.
```

## MPI

```
Usage: heat [OPTION...]
Calculates the heat equation on a 2D surface, outputting the result to stdout

  -d, --diffusivity=J/ M3 K  Diffusivity in J/M3 K. Default is 0.1.
  -i, --iterations=ITERS     Number of iterations. Default is 3000.
  -n, --resolution=UNITS     The surface is the unit square, to be represented
                             by a nxn matrix. Default: 100.
  -o, --output               Output a .pgm to stdout.
  -p, --spacestep=METERS     Spacestep in meters. Default 1/(n+2).
  -s, --timestep=SECONDS     Timestep in seconds. Default spacestep2 / (4
                             diffusivity).
  -t, --time                 Output a elapsed time to stdout.
  -?, --help                 Give this help list
      --usage                Give a short usage message

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.
```

## SDL implementation

```
Usage: display [OPTION...] SIZE ITERS
Displays the heat dissipation

  -s, --simple               Use a 2-color heatmap instead of the standard
                             5-color one.
  -?, --help                 Give this help list
      --usage                Give a short usage message
```

# Physics
## Heat diffusion

The heat equation is a [partial differential equation](https://en.wikipedia.org/wiki/Partial_differential_equation) given by

<img src="https://render.githubusercontent.com/render/math?math=\rho c \frac{\partial \theta}{\partial t} = k \nabla^2 \theta">

Where <img src="https://render.githubusercontent.com/render/math?math=\rho$ is the [density](https://en.wikipedia.org/wiki/Density) of the material, $c"> is the [heat capacity](https://en.wikipedia.org/wiki/Heat_capacity)
and <img src="https://render.githubusercontent.com/render/math?math=k$ is the [thermal conductivity](https://en.wikipedia.org/wiki/Thermal_conductivity), and $\theta"> is a function that
gives the heat at a particular point in space and time. We want to
find a <img src="https://render.githubusercontent.com/render/math?math=\theta"> that solves this equation.

The LHS describes the heat accumulated as time passes, and the RHS
describes how much heat is flowing through the material. This process
is called diffusion and the equation is the same for other physical
phenomena (such as momentum diffusion).

By dividing both sides by <img src="https://render.githubusercontent.com/render/math?math=\rho c"> we have

<img src="https://render.githubusercontent.com/render/math?math=\frac{\partial \theta}{\partial t} = \alpha \nabla^2 \theta">

Where <img src="https://render.githubusercontent.com/render/math?math=\alpha"> is the thermal diffusivity of the surface (<img src="https://render.githubusercontent.com/render/math?math=\frac{J}{m^3 K}">).

# Math
## Discretization

In order to solve this computationally we need to find an
approximation for <img src="https://render.githubusercontent.com/render/math?math=\theta"> using a numerical method. We'll use
[finite differences](https://en.wikipedia.org/wiki/Finite_difference).

To use the method we need to define discrete space and time
dimensions. We define our space as

<img src="https://render.githubusercontent.com/render/math?math=(x_i, y_i) = (i \times \Delta s, j \times \Delta s), i, j = 0, 1, 2, ..., n - 1">

I.e. it's a two dimensional mesh where <img src="https://render.githubusercontent.com/render/math?math=\Delta s = \frac{1}{n - 1}"> is
the [norm of the partition](https://en.wikipedia.org/wiki/Partition_of_an_interval) (which we'll call the "space step"). The
discrete time is also a mesh

<img src="https://render.githubusercontent.com/render/math?math=t_k = k \times \Delta t, k = 0, 1, 2, ...">

Where <img src="https://render.githubusercontent.com/render/math?math=\Delta t"> is the user defined time step, also the norm.

## Finite differences and recurrence

Since we are in two dimensions, the [gradient](https://en.wikipedia.org/wiki/Partition_of_an_interval) squared is given by the
second partials, so we can rewrite the equation to

<img src="https://render.githubusercontent.com/render/math?math=\frac{\partial \theta}{\partial t} = \alpha (\frac{\partial^2 \theta}{\partial x^2} + \frac{\partial^2 \theta}{\partial y^2})">

Using a forward difference for the time dimension and a central
difference for the space ([FTCS scheme](https://en.wikipedia.org/wiki/FTCS_scheme)) we have

<img src="https://render.githubusercontent.com/render/math?math=\frac{\partial \theta}{\partial t} \approx \frac{\theta^{k + 1}_{i,j} - \theta^{k}_{i,j}}{\Delta t}">

<img src="https://render.githubusercontent.com/render/math?math=\frac{\partial^2 \theta}{\partial x^2} \approx \frac{\theta^{k}_{i+1,j} - 2\theta^k_{i,j} + \theta^k_{i-1,j}}{(\Delta s)^2}">

<img src="https://render.githubusercontent.com/render/math?math=\frac{\partial^2 \theta}{\partial y^2} \approx \frac{\theta^{k}_{i,j+1} - 2\theta^k_{i,j} + \theta^k_{i,j-1}}{(\Delta s)^2}">

So the original equations becomes:

<img src="https://render.githubusercontent.com/render/math?math=\frac{\theta^{k + 1}_{i,j} - \theta^{k}_{i,j}}{\Delta t} = \alpha (\frac{\theta^{k}_{i+1,j} - 2\theta^k_{i,j} + \theta^k_{i-1,j}}{(\Delta s)^2}  + \frac{\theta^{k}_{i,j+1} - 2\theta^k_{i,j} + \theta^k_{i,j-1}}{(\Delta s)^2})">

Isolating <img src="https://render.githubusercontent.com/render/math?math=\theta^{k+1}_{i,j}"> to get the recurrence equation we have

<img src="https://render.githubusercontent.com/render/math?math=\theta^{k+1}_{i,j} = \theta^k_{i,j} + \frac{\alpha \Delta t}{(\Delta s)^2} (\theta^k_{i+1, j} + \theta^k_{i-1,j} - 4\theta^k_{i,j} + \theta^k_{i,j+1} + \theta^k_{i,j-1})">

## Stability and convergence

This [explicit method](https://en.wikipedia.org/wiki/Explicit_and_implicit_methods) is [numerically stable](https://en.wikipedia.org/wiki/Numerical_stability#Stability_in_numerical_differential_equations) and convergent when <img src="https://render.githubusercontent.com/render/math?math=\Delta t \le \frac{(\Delta s)^2}{2\alpha}">. The numerical error is proportional to the time
step and the square of the space step. A more precise
method is the Crank-Nicolson one.

For k = 0 we establish an initial condition. For i = 0 and i = n - 1 we establish boundary conditions <img src="https://render.githubusercontent.com/render/math?math=$\theta^k"> must remain constant at
the boundaries, see Dirichlet Conditions, Steady State Solutions.

