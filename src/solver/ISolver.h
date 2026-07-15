#pragma once

#include "core/Config.h"
#include "core/Grid.h"

// -----------------------------------------------------------------------------
// Abstract solver interface.
//
// The renderer and app talk to the physics ONLY through this interface, so the
// Phase 1 Stam solver and the Phase 2 Lattice-Boltzmann solver are swappable.
// A solver exposes read-only access to the velocity field on the collocated
// grid; the renderer never touches physics internals.
// -----------------------------------------------------------------------------
namespace sf {

class ISolver {
public:
    virtual ~ISolver() = default;

    // Advance the simulation by one time step of length cfg().dt.
    virtual void step() = 0;

    // Reset all fields to the initial (quiescent) state.
    virtual void reset() = 0;

    // Read-only velocity components, sampled at cell centres (collocated grid).
    virtual const Field& u() const = 0;
    virtual const Field& v() const = 0;

    // Active configuration. Mutating this (e.g. changing Re) takes effect on the
    // next step; call reset() if a clean restart is wanted.
    virtual const SimConfig& cfg() const = 0;
    virtual SimConfig&       cfg()       = 0;

    // Human-readable name for on-screen readout.
    virtual const char* name() const = 0;
};

} // namespace sf
