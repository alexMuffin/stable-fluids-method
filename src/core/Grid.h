#pragma once

#include <algorithm>
#include <vector>
#include <cstddef>

// -----------------------------------------------------------------------------
// Grid / field storage.
//
// Convention (documented once, used everywhere via IX):
//   - Storage is ROW-MAJOR. The fast axis is i (x / column), the slow axis is j
//     (y / row):  index = i + (N + 2) * j.
//   - Fields are stored on a COLLOCATED grid (all quantities at cell centres)
//     with a 1-cell ghost border. Interior cells are i, j in [1, N]; the border
//     lives at i or j in {0, N + 1} and holds boundary values.
//   - j = 0 is the bottom wall, j = N + 1 is the top (open/lid) side,
//     i = 0 is the left wall, i = N + 1 is the right wall.
//   - Cell (i, j) centre in world coordinates is ((i - 0.5)/N, (j - 0.5)/N),
//     so the unit square maps to interior cells 1..N.
// -----------------------------------------------------------------------------
namespace sf {

// A scalar field over the padded (N+2) x (N+2) grid.
class Field {
public:
    Field() = default;
    explicit Field(int N) { resize(N); }

    void resize(int N) {
        N_ = N;
        stride_ = N + 2;
        data_.assign(static_cast<std::size_t>(stride_) * stride_, 0.0);
    }

    void clear() { std::fill(data_.begin(), data_.end(), 0.0); }

    int N() const { return N_; }
    int stride() const { return stride_; }

    // Row-major index helper. Use this EVERYWHERE instead of hand-rolling it.
    int idx(int i, int j) const { return i + stride_ * j; }

    double&       operator()(int i, int j)       { return data_[idx(i, j)]; }
    double        operator()(int i, int j) const { return data_[idx(i, j)]; }

    double*       raw()       { return data_.data(); }
    const double* raw() const { return data_.data(); }

    void swap(Field& other) {
        data_.swap(other.data_);
        std::swap(N_, other.N_);
        std::swap(stride_, other.stride_);
    }

private:
    int N_ = 0;
    int stride_ = 0;
    std::vector<double> data_;
};

} // namespace sf
