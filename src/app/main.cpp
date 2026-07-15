// -----------------------------------------------------------------------------
// App entry point: input -> solver.step() -> renderer.draw().
//
// Real-time 2D CFD of a lid-driven cavity (Phase 1: Jos Stam "Stable Fluids").
// -----------------------------------------------------------------------------
#include "core/Config.h"
#include "solver/StamSolver.h"
#include "render/Renderer.h"

#include "raylib.h"

#include <memory>

using namespace sf;

int main() {
    SimConfig cfg;
    cfg.N        = 128;
    cfg.reynolds = 100.0;
    cfg.dt       = 0.016;

    const int screenW = 1100;
    const int screenH = 720;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(screenW, screenH, "Stable Fluids - Lid-driven cavity (Re=100)");
    SetTargetFPS(60);

    std::unique_ptr<ISolver> solver = std::make_unique<StamSolver>(cfg);
    Renderer renderer(screenW, screenH);
    renderer.reseedParticles(*solver);

    RenderMode mode   = RenderMode::Heatmap;
    bool       paused = false;
    double     simTime = 0.0;

    while (!WindowShouldClose()) {
        // --- Input -----------------------------------------------------------
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;

        if (IsKeyPressed(KEY_R)) {
            solver->reset();
            renderer.reseedParticles(*solver);
            simTime = 0.0;
        }

        if (IsKeyPressed(KEY_ONE))   mode = RenderMode::Heatmap;
        if (IsKeyPressed(KEY_TWO))   mode = RenderMode::Particles;
        if (IsKeyPressed(KEY_THREE)) mode = RenderMode::Streamlines;

        // Change Reynolds number; this restarts the field for a clean result.
        if (IsKeyPressed(KEY_UP)) {
            solver->cfg().reynolds *= 2.0;
            solver->reset();
            renderer.reseedParticles(*solver);
            simTime = 0.0;
        }
        if (IsKeyPressed(KEY_DOWN)) {
            solver->cfg().reynolds = (solver->cfg().reynolds > 2.0)
                                     ? solver->cfg().reynolds / 2.0 : 1.0;
            solver->reset();
            renderer.reseedParticles(*solver);
            simTime = 0.0;
        }

        // --- Update ----------------------------------------------------------
        if (!paused) {
            solver->step();
            simTime += solver->cfg().dt;
            renderer.update(*solver, static_cast<float>(solver->cfg().dt));
        }

        // --- Draw ------------------------------------------------------------
        BeginDrawing();
        ClearBackground(Color{ 18, 18, 24, 255 });
        renderer.draw(*solver, mode, paused, simTime,
                      static_cast<float>(GetFPS()));
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
