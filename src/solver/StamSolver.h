#pragma once

#include "solver/ISolver.h"
#include "core/Grid.h"
#include "core/Config.h"

// -----------------------------------------------------------------------------
// Jos Stam "Stable Fluids" solver (semi-Lagrangian advection, implicit
// diffusion via Gauss-Seidel, pressure projection via a Gauss-Seidel Poisson
// solve). Unconditionally stable. Configured here for the lid-driven cavity:
//   - left/right/bottom walls: no-slip  (u = v = 0)
//   - top (open/lid) side:     u = lidSpeed, v = 0
//
// Reference: Stam, "Stable Fluids", SIGGRAPH 1999.
// -----------------------------------------------------------------------------
namespace sf {

class StamSolver : public ISolver {
public:
    explicit StamSolver(const SimConfig& cfg);

    void step() override;
    void reset() override;

    const Field& u() const override { return u_; }
    const Field& v() const override { return v_; }

    const SimConfig& cfg() const override { return cfg_; }
    SimConfig&       cfg()       override { return cfg_; }

    const char* name() const override { return "Stam (Stable Fluids)"; }

private:
    // Boundary condition type for set_bnd, matching Stam's convention:
    //   b = 0 -> scalar (pressure/divergence, Neumann zero-gradient)
    //   b = 1 -> x-velocity component
    //   b = 2 -> y-velocity component
    void setBnd(int b, Field& f) const;

    void diffuse(int b, Field& x, const Field& x0, double diff) const;
    void advect(int b, Field& d, const Field& d0,
                const Field& u, const Field& v) const;
    void project(Field& u, Field& v, Field& p, Field& div) const;

    void velStep();

    SimConfig cfg_;

    // Current and scratch/previous velocity fields.
    Field u_, v_;
    Field u0_, v0_;

    // Scratch for the pressure projection.
    Field p_, div_;
};

} // namespace sf
