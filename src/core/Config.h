#pragma once

// -----------------------------------------------------------------------------
// Simulation configuration.
//
// Units are simulation units: the cavity is the unit square (L = 1) and the lid
// moves at U = 1. The only free physical parameter is the Reynolds number, from
// which the kinematic viscosity follows: nu = U * L / Re = 1 / Re.
// -----------------------------------------------------------------------------
namespace sf {

struct SimConfig {
    // Grid: N x N interior cells (a 1-cell border is added internally).
    int   N        = 128;

    // Physics.
    double reynolds = 100.0;   // Re; nu is derived from this.
    double lidSpeed = 1.0;     // U, tangential velocity on the open/lid side.
    double dt       = 0.016;   // simulation time step.

    // Solver iteration counts (Gauss-Seidel sweeps).
    int    diffuseIters = 20;
    int    projectIters = 40;

    // Derived: kinematic viscosity.
    double viscosity() const { return lidSpeed * 1.0 / reynolds; }
};

} // namespace sf
