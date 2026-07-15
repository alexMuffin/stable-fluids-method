#pragma once

// -----------------------------------------------------------------------------
// Small numeric helpers shared by solver and renderer.
// -----------------------------------------------------------------------------
namespace sf {

template <typename T>
inline T clampv(T x, T lo, T hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

template <typename T>
inline T lerp(T a, T b, T t) {
    return a + (b - a) * t;
}

} // namespace sf
