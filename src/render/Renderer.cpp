#include "render/Renderer.h"
#include "core/MathHelpers.h"

#include <cmath>
#include <cstdio>

namespace sf {

namespace {

// The simulation viewport is a square inset in the window; the rest holds HUD.
constexpr int kMargin = 16;

// Blue -> cyan -> green -> yellow -> red heatmap for a normalized value [0, 1].
Color heatColor(float t) {
    t = clampv(t, 0.0f, 1.0f);
    // Piecewise linear through 5 control colors.
    const float r = clampv(1.5f * t - 0.5f, 0.0f, 1.0f);
    const float g = clampv(1.0f - std::fabs(2.0f * t - 1.0f) * 1.2f, 0.0f, 1.0f);
    const float b = clampv(1.0f - 1.5f * t, 0.0f, 1.0f);
    return Color{ static_cast<unsigned char>(r * 255),
                 static_cast<unsigned char>(g * 255),
                 static_cast<unsigned char>(b * 255), 255 };
}

} // namespace

Renderer::Renderer(int screenW, int screenH)
    : screenW_(screenW), screenH_(screenH) {}

Renderer::~Renderer() {
    if (heatTex_.id != 0) UnloadTexture(heatTex_);
}

// The square drawing area for the cavity, in screen pixels.
static int viewSize(int screenW, int screenH) {
    return (screenW < screenH ? screenW : screenH) - 2 * kMargin;
}

Vector2 Renderer::worldToScreen(float wx, float wy) const {
    const int size = viewSize(screenW_, screenH_);
    // y is up in the world, down on screen.
    return Vector2{ kMargin + wx * size,
                    kMargin + (1.0f - wy) * size };
}

void Renderer::sampleVelocity(const ISolver& solver, float wx, float wy,
                              float& outU, float& outV) const {
    const Field& u = solver.u();
    const Field& v = solver.v();
    const int N = solver.cfg().N;

    // World [0,1] -> grid coords where cell centre (i,j) is at ((i-0.5)/N,...).
    float gx = wx * N + 0.5f;
    float gy = wy * N + 0.5f;
    gx = clampv(gx, 1.0f, static_cast<float>(N));
    gy = clampv(gy, 1.0f, static_cast<float>(N));

    const int i0 = static_cast<int>(gx), i1 = i0 + 1;
    const int j0 = static_cast<int>(gy), j1 = j0 + 1;
    const float sx = gx - i0, sy = gy - j0;

    auto bilerp = [&](const Field& f) {
        return (1 - sx) * ((1 - sy) * f(i0, j0) + sy * f(i0, j1)) +
               sx       * ((1 - sy) * f(i1, j0) + sy * f(i1, j1));
    };
    outU = static_cast<float>(bilerp(u));
    outV = static_cast<float>(bilerp(v));
}

// ---------------------------------------------------------------------------
// Tracer particles.
// ---------------------------------------------------------------------------
void Renderer::reseedParticles(const ISolver& solver) {
    (void)solver;
    particles_.clear();
    const int side = 48;   // side x side grid of tracers
    particles_.reserve(side * side);
    for (int j = 0; j < side; ++j) {
        for (int i = 0; i < side; ++i) {
            particles_.push_back(Particle{
                (i + 0.5f) / side,
                (j + 0.5f) / side });
        }
    }
}

void Renderer::update(const ISolver& solver, float dt) {
    if (particles_.empty()) reseedParticles(solver);

    for (auto& p : particles_) {
        float su, sv;
        sampleVelocity(solver, p.x, p.y, su, sv);
        p.x += su * dt;
        p.y += sv * dt;
        // Respawn particles that drift out of the cavity.
        if (p.x < 0.0f || p.x > 1.0f || p.y < 0.0f || p.y > 1.0f) {
            // Reseed near the lid where flow is injected, spread across x.
            p.x = static_cast<float>((&p - particles_.data()) % 97) / 97.0f;
            p.y = 0.98f;
        }
    }
}

// ---------------------------------------------------------------------------
// Draw: heatmap.
// ---------------------------------------------------------------------------
void Renderer::drawHeatmap(const ISolver& solver) const {
    const int N = solver.cfg().N;
    const Field& u = solver.u();
    const Field& v = solver.v();

    // (Re)create the streaming texture if the grid size changed.
    if (heatN_ != N || heatTex_.id == 0) {
        if (heatTex_.id != 0) UnloadTexture(const_cast<Texture2D&>(heatTex_));
        Image img = GenImageColor(N, N, BLACK);
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        const_cast<Renderer*>(this)->heatTex_ = LoadTextureFromImage(img);
        SetTextureFilter(const_cast<Texture2D&>(heatTex_), TEXTURE_FILTER_BILINEAR);
        UnloadImage(img);
        const_cast<Renderer*>(this)->heatN_ = N;
        heatPixels_.assign(static_cast<size_t>(N) * N * 4, 0);
    }

    // Normalize against lid speed so the color scale is stable.
    const float vmax = static_cast<float>(solver.cfg().lidSpeed);

    for (int j = 1; j <= N; ++j) {
        for (int i = 1; i <= N; ++i) {
            const float su = static_cast<float>(u(i, j));
            const float sv = static_cast<float>(v(i, j));
            const float mag = std::sqrt(su * su + sv * sv);
            const Color c = heatColor(mag / (vmax > 0 ? vmax : 1.0f));
            // Texture row 0 is the top of the image; world y is up, so flip j.
            const int tx = i - 1;
            const int ty = N - j;
            const size_t idx = (static_cast<size_t>(ty) * N + tx) * 4;
            heatPixels_[idx + 0] = c.r;
            heatPixels_[idx + 1] = c.g;
            heatPixels_[idx + 2] = c.b;
            heatPixels_[idx + 3] = 255;
        }
    }
    UpdateTexture(heatTex_, heatPixels_.data());

    const int size = viewSize(screenW_, screenH_);
    const Rectangle src{ 0, 0, static_cast<float>(N), static_cast<float>(N) };
    const Rectangle dst{ static_cast<float>(kMargin), static_cast<float>(kMargin),
                         static_cast<float>(size), static_cast<float>(size) };
    DrawTexturePro(heatTex_, src, dst, Vector2{ 0, 0 }, 0.0f, WHITE);
}

// ---------------------------------------------------------------------------
// Draw: tracer particles.
// ---------------------------------------------------------------------------
void Renderer::drawParticles() const {
    DrawRectangle(kMargin, kMargin, viewSize(screenW_, screenH_),
                  viewSize(screenW_, screenH_), Color{ 8, 10, 20, 255 });
    for (const auto& p : particles_) {
        const Vector2 s = worldToScreen(p.x, p.y);
        DrawPixelV(s, Color{ 120, 200, 255, 200 });
    }
}

// ---------------------------------------------------------------------------
// Draw: streamlines traced from a coarse seed grid.
// ---------------------------------------------------------------------------
void Renderer::drawStreamlines(const ISolver& solver) const {
    DrawRectangle(kMargin, kMargin, viewSize(screenW_, screenH_),
                  viewSize(screenW_, screenH_), Color{ 8, 10, 20, 255 });

    const int seeds = 26;
    const int steps = 40;
    const float h = 0.004f;   // integration step in world units

    for (int sj = 0; sj < seeds; ++sj) {
        for (int si = 0; si < seeds; ++si) {
            float x = (si + 0.5f) / seeds;
            float y = (sj + 0.5f) / seeds;
            Vector2 prev = worldToScreen(x, y);
            for (int k = 0; k < steps; ++k) {
                float su, sv;
                sampleVelocity(solver, x, y, su, sv);
                const float mag = std::sqrt(su * su + sv * sv);
                if (mag < 1e-5f) break;
                x += su / mag * h;
                y += sv / mag * h;
                if (x < 0 || x > 1 || y < 0 || y > 1) break;
                const Vector2 cur = worldToScreen(x, y);
                const float t = mag / static_cast<float>(solver.cfg().lidSpeed);
                DrawLineV(prev, cur, heatColor(t));
                prev = cur;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// HUD.
// ---------------------------------------------------------------------------
void Renderer::drawHud(const ISolver& solver, RenderMode mode, bool paused,
                       double simTime, float fps) const {
    const SimConfig& cfg = solver.cfg();
    const int size = viewSize(screenW_, screenH_);
    const int x = kMargin + size + 20;
    int y = kMargin;
    const int lh = 22;

    const char* modeName =
        mode == RenderMode::Heatmap     ? "Heatmap" :
        mode == RenderMode::Particles   ? "Particles" : "Streamlines";

    char buf[128];
    DrawText("2D CFD - Lid-driven cavity", x, y, 20, RAYWHITE); y += lh + 6;

    std::snprintf(buf, sizeof(buf), "Solver : %s", solver.name());
    DrawText(buf, x, y, 18, LIGHTGRAY); y += lh;
    std::snprintf(buf, sizeof(buf), "FPS    : %.0f", static_cast<double>(fps));
    DrawText(buf, x, y, 18, LIGHTGRAY); y += lh;
    std::snprintf(buf, sizeof(buf), "Sim t  : %.2f", simTime);
    DrawText(buf, x, y, 18, LIGHTGRAY); y += lh;
    std::snprintf(buf, sizeof(buf), "Re     : %.0f", cfg.reynolds);
    DrawText(buf, x, y, 18, LIGHTGRAY); y += lh;
    std::snprintf(buf, sizeof(buf), "nu     : %.5f", cfg.viscosity());
    DrawText(buf, x, y, 18, LIGHTGRAY); y += lh;
    std::snprintf(buf, sizeof(buf), "Grid   : %d x %d", cfg.N, cfg.N);
    DrawText(buf, x, y, 18, LIGHTGRAY); y += lh;
    std::snprintf(buf, sizeof(buf), "View   : %s", modeName);
    DrawText(buf, x, y, 18, LIGHTGRAY); y += lh;
    std::snprintf(buf, sizeof(buf), "State  : %s", paused ? "PAUSED" : "running");
    DrawText(buf, x, y, 18, paused ? YELLOW : GREEN); y += lh + 12;

    DrawText("Controls", x, y, 18, RAYWHITE); y += lh;
    const char* help[] = {
        "SPACE  pause / resume",
        "R      reset field",
        "1/2/3  heatmap/particles/streamlines",
        "UP/DN  Re x2 / /2 (resets)",
        "ESC    quit",
    };
    for (const char* line : help) {
        DrawText(line, x, y, 16, GRAY);
        y += lh - 2;
    }
}

void Renderer::draw(const ISolver& solver, RenderMode mode, bool paused,
                    double simTime, float fps) const {
    switch (mode) {
        case RenderMode::Heatmap:     drawHeatmap(solver);     break;
        case RenderMode::Particles:   drawParticles();         break;
        case RenderMode::Streamlines: drawStreamlines(solver); break;
    }
    // Frame the cavity.
    DrawRectangleLines(kMargin, kMargin, viewSize(screenW_, screenH_),
                       viewSize(screenW_, screenH_), DARKGRAY);
    drawHud(solver, mode, paused, simTime, fps);
}

} // namespace sf
