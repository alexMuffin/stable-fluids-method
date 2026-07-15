#pragma once

#include "solver/ISolver.h"

#include "raylib.h"

#include <vector>

// -----------------------------------------------------------------------------
// Renderer: reads velocity fields from an ISolver and draws them. It knows
// nothing about the physics internals — only how to turn fields into pixels.
//
// Modes:
//   Heatmap    -> velocity-magnitude heatmap (blue = slow, red = fast)
//   Particles  -> tracer particles advected through the field
//   Streamlines-> short streamlines traced from a seed grid
// -----------------------------------------------------------------------------
namespace sf {

enum class RenderMode { Heatmap, Particles, Streamlines };

class Renderer {
public:
    Renderer(int screenW, int screenH);
    ~Renderer();

    // Advance visualization-only state (tracer particle positions).
    void update(const ISolver& solver, float dt);

    // Draw the current frame for the given solver and mode.
    void draw(const ISolver& solver, RenderMode mode, bool paused,
              double simTime, float fps) const;

    void reseedParticles(const ISolver& solver);

private:
    struct Particle { float x, y; };   // world coords in [0, 1]

    void drawHeatmap(const ISolver& solver) const;
    void drawParticles() const;
    void drawStreamlines(const ISolver& solver) const;
    void drawHud(const ISolver& solver, RenderMode mode, bool paused,
                 double simTime, float fps) const;

    // Bilinear velocity sample at world position (wx, wy) in [0, 1].
    void sampleVelocity(const ISolver& solver, float wx, float wy,
                        float& outU, float& outV) const;

    // Map world coords in [0, 1] (y up) to screen pixels (y down).
    Vector2 worldToScreen(float wx, float wy) const;

    int screenW_, screenH_;

    // Heatmap is drawn via a streamed texture (one texel per interior cell).
    Texture2D heatTex_{};
    int       heatN_ = 0;
    mutable std::vector<unsigned char> heatPixels_;   // RGBA, updated per frame

    std::vector<Particle> particles_;
};

} // namespace sf
