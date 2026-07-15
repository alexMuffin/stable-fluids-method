# Project brief: Real-time 2D CFD — open cavity with shear flow

## Goal
Build a Windows-only, real-time 2D CFD simulation of a square cavity with three solid
walls and one open side. Wind flows across the open side, parallel to it, at 1 m/s,
dragging fluid inside the cavity and driving a recirculating vortex. Long-term goal:
grow this into a full physics engine with a graphical interface showing live flow.

This setup is equivalent to the classic **lid-driven cavity** benchmark (Ghia,
Ghia & Shin, 1982), which gives us free validation data.

## Physical setup
- Domain: unit square cavity (1 x 1 in simulation units).
- Three walls (left, right, bottom): **no-slip** boundary, u = v = 0.
- Open side (top): tangential velocity fixed at u = 1, v = 0 (the "moving lid").
- Target Reynolds number: start at **Re = 100**, later Re = 400 and 1000.
  - Note: physically, air at 1 m/s over a 1 m cavity gives Re ~ 66,000 (turbulent).
    We deliberately use artificially high viscosity to stay laminar and match the
    benchmark. nu = U * L / Re = 1 / Re in simulation units.

## Tech stack
- **Language:** C++ (C++17 or later).
- **Build:** CMake, targeting Visual Studio / MSVC on Windows.
- **Rendering:** raylib (simple, fast to get pixels on screen). May migrate to
  SDL2 + OpenGL or DirectX 11 later — keep the renderer isolated so this is easy.
- **No heavy dependencies.** Solver code must be self-contained.

## Numerical approach (two phases)
### Phase 1 — Jos Stam "Stable Fluids" solver
- Semi-Lagrangian advection, implicit diffusion (Gauss-Seidel), pressure
  projection (Gauss-Seidel Poisson solve), velocity fields stored on a
  collocated grid with a 1-cell boundary border.
- Grid: 128 x 128 to start; make size a config parameter.
- Unconditionally stable; good for getting a working, visually convincing
  vortex fast. Not physically accurate (too diffusive) — that is expected.

### Phase 2 — Lattice Boltzmann (D2Q9, BGK collision)
- More physically accurate, no Poisson solve, naturally parallel (good path
  to multithreading and GPU compute shaders later).
- Bounce-back boundaries on the three walls; velocity boundary (Zou-He) on
  the open/lid side.
- Validate against Ghia et al. centerline velocity profiles at Re = 100.

## Architecture
Keep solver and renderer strictly separated:

```
/src
  /core      -> Grid/field storage, simulation config, math helpers
  /solver    -> physics only: step(dt), advect, diffuse, project, boundaries
               (later: lbm/ subfolder with the LBM implementation behind the
                same solver interface)
  /render    -> velocity-magnitude heatmap, tracer particles, streamlines;
                knows nothing about the physics internals, only reads fields
  /app       -> main loop: input -> solver.step(dt) -> renderer.draw(grid)
```

- Define an abstract `ISolver` interface (step, read velocity field, set
  boundary config) so Stam and LBM implementations are swappable.
- All simulation parameters (grid size, Re/viscosity, lid speed, dt) in one
  config struct, adjustable at runtime where feasible.

## Visualization requirements
- Real-time heatmap of velocity magnitude (blue = slow, red = fast).
- Tracer particles advected through the field to show the vortex.
- Toggle between heatmap / particles / streamlines.
- On-screen readout: FPS, sim time, Re, grid size.
- Keyboard controls: pause, reset, change Re, switch solver (later).

## Roadmap
1. CMake + raylib skeleton: window opens, draws a colored 128x128 grid at 60 FPS.
2. Implement Stam solver core (advect, diffuse, project) with the cavity
   boundary conditions above.
3. Hook solver to renderer; verify the primary vortex forms and settles.
4. Add particles/streamlines and runtime controls.
5. Implement D2Q9 LBM behind the same ISolver interface.
6. Validation: extract u(y) along the vertical centerline and v(x) along the
   horizontal centerline at steady state, compare to Ghia et al. Re = 100 data
   (plot or print RMS error).
7. Stretch: internal obstacles, adjustable inflow, multithreading (OpenMP or
   std::thread), then GPU compute shaders, then 3D.

## Conventions
- Column-major or row-major — pick one, document it in core/Grid.h, use an
  IX(i, j) index helper everywhere.
- Units: simulation units (L = 1, U = 1); Re is the only free physical parameter.
- Keep every step compilable and runnable; commit per roadmap step.

## Start here
Begin with roadmap step 1: create the CMake project, fetch raylib (e.g. via
CMake FetchContent), and render a placeholder grid. Then proceed to step 2.
