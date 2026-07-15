#include "solver/StamSolver.h"
#include "core/MathHelpers.h"

#include <cmath>

namespace sf {

StamSolver::StamSolver(const SimConfig& cfg) : cfg_(cfg) {
    reset();
}

void StamSolver::reset() {
    const int N = cfg_.N;
    u_.resize(N);
    v_.resize(N);
    u0_.resize(N);
    v0_.resize(N);
    p_.resize(N);
    div_.resize(N);
    // Quiescent start; the lid drives the flow through the boundary condition.
    setBnd(1, u_);
    setBnd(2, v_);
}

// -----------------------------------------------------------------------------
// Boundary conditions for the lid-driven cavity.
//
// The domain boundary sits half a cell outside the last interior cell, so a
// wall value W is enforced by mirroring the interior into the ghost cell:
//   ghost = 2*W - interior   (=> face average equals W).
// For no-slip walls W = 0, giving ghost = -interior. For the moving lid the
// tangential (u) component uses W = lidSpeed; the normal (v) component uses
// W = 0. Scalars (b == 0) use zero-gradient (ghost = interior).
// -----------------------------------------------------------------------------
void StamSolver::setBnd(int b, Field& f) const {
    const int N = cfg_.N;

    // Left (i = 0) and right (i = N+1) walls: no-slip for both velocity comps.
    for (int j = 1; j <= N; ++j) {
        f(0,     j) = (b == 1 || b == 2) ? -f(1, j) : f(1, j);
        f(N + 1, j) = (b == 1 || b == 2) ? -f(N, j) : f(N, j);
    }

    // Bottom wall (j = 0): no-slip.
    for (int i = 1; i <= N; ++i) {
        f(i, 0) = (b == 1 || b == 2) ? -f(i, 1) : f(i, 1);
    }

    // Top / open lid (j = N+1): u = lidSpeed, v = 0.
    for (int i = 1; i <= N; ++i) {
        if (b == 1) {
            f(i, N + 1) = 2.0 * cfg_.lidSpeed - f(i, N);   // tangential = U
        } else if (b == 2) {
            f(i, N + 1) = -f(i, N);                        // normal = 0
        } else {
            f(i, N + 1) = f(i, N);                         // scalar Neumann
        }
    }

    // Corners: average of the two adjacent edge ghosts.
    f(0,     0)     = 0.5 * (f(1, 0)     + f(0, 1));
    f(0,     N + 1) = 0.5 * (f(1, N + 1) + f(0, N));
    f(N + 1, 0)     = 0.5 * (f(N, 0)     + f(N + 1, 1));
    f(N + 1, N + 1) = 0.5 * (f(N, N + 1) + f(N + 1, N));
}

// Implicit (backward Euler) diffusion solved with Gauss-Seidel relaxation.
void StamSolver::diffuse(int b, Field& x, const Field& x0, double diff) const {
    const int N = cfg_.N;
    const double a = cfg_.dt * diff * N * N;   // dt * nu / h^2, with h = 1/N
    const double denom = 1.0 + 4.0 * a;

    for (int k = 0; k < cfg_.diffuseIters; ++k) {
        for (int j = 1; j <= N; ++j) {
            for (int i = 1; i <= N; ++i) {
                x(i, j) = (x0(i, j) +
                           a * (x(i - 1, j) + x(i + 1, j) +
                                x(i, j - 1) + x(i, j + 1))) / denom;
            }
        }
        setBnd(b, x);
    }
}

// Semi-Lagrangian advection: trace each cell centre back through the velocity
// field and bilinearly sample the previous field.
void StamSolver::advect(int b, Field& d, const Field& d0,
                        const Field& u, const Field& v) const {
    const int N = cfg_.N;
    const double dt0 = cfg_.dt * N;   // dt scaled to grid cells (h = 1/N)

    for (int j = 1; j <= N; ++j) {
        for (int i = 1; i <= N; ++i) {
            double x = i - dt0 * u(i, j);
            double y = j - dt0 * v(i, j);

            x = clampv(x, 0.5, N + 0.5);
            y = clampv(y, 0.5, N + 0.5);

            const int i0 = static_cast<int>(x);
            const int i1 = i0 + 1;
            const int j0 = static_cast<int>(y);
            const int j1 = j0 + 1;

            const double s1 = x - i0, s0 = 1.0 - s1;
            const double t1 = y - j0, t0 = 1.0 - t1;

            d(i, j) = s0 * (t0 * d0(i0, j0) + t1 * d0(i0, j1)) +
                      s1 * (t0 * d0(i1, j0) + t1 * d0(i1, j1));
        }
    }
    setBnd(b, d);
}

// Pressure projection: make the velocity field divergence-free (Chorin).
void StamSolver::project(Field& u, Field& v, Field& p, Field& div) const {
    const int N = cfg_.N;
    const double h = 1.0 / N;

    for (int j = 1; j <= N; ++j) {
        for (int i = 1; i <= N; ++i) {
            div(i, j) = -0.5 * h * (u(i + 1, j) - u(i - 1, j) +
                                    v(i, j + 1) - v(i, j - 1));
            p(i, j) = 0.0;
        }
    }
    setBnd(0, div);
    setBnd(0, p);

    for (int k = 0; k < cfg_.projectIters; ++k) {
        for (int j = 1; j <= N; ++j) {
            for (int i = 1; i <= N; ++i) {
                p(i, j) = (div(i, j) +
                           p(i - 1, j) + p(i + 1, j) +
                           p(i, j - 1) + p(i, j + 1)) * 0.25;
            }
        }
        setBnd(0, p);
    }

    for (int j = 1; j <= N; ++j) {
        for (int i = 1; i <= N; ++i) {
            u(i, j) -= 0.5 * (p(i + 1, j) - p(i - 1, j)) / h;
            v(i, j) -= 0.5 * (p(i, j + 1) - p(i, j - 1)) / h;
        }
    }
    setBnd(1, u);
    setBnd(2, v);
}

void StamSolver::velStep() {
    const double visc = cfg_.viscosity();

    // Diffuse into the scratch fields (u0/v0 become the diffused velocity).
    u0_.swap(u_);
    v0_.swap(v_);
    diffuse(1, u_, u0_, visc);
    diffuse(2, v_, v0_, visc);
    project(u_, v_, p_, div_);

    // Advect using the projected field; result back into u_/v_.
    u0_.swap(u_);
    v0_.swap(v_);
    advect(1, u_, u0_, u0_, v0_);
    advect(2, v_, v0_, u0_, v0_);
    project(u_, v_, p_, div_);
}

void StamSolver::step() {
    velStep();
}

} // namespace sf
