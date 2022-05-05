/*
Copyright (C) 2020 Arsène Pérard-Gayot

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef BVH_VECTOR_HPP
#define BVH_VECTOR_HPP

#include <cstddef>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <type_traits>
#include <vector>

#include "platform.hpp"

namespace bvh {

template <typename, size_t> struct Vector;

/// Helper class to set the elements of a vector.
template <size_t I, typename Scalar, size_t N>
struct VectorSetter {
    template <typename... Args>
    bvh_always_inline static void set(Vector<Scalar, N>& v, Scalar s, Args... args) {
        v[I] = s;
        VectorSetter<I + 1, Scalar, N>::set(v, args...);
    }
};

template <typename Scalar, size_t N>
struct VectorSetter<N, Scalar, N> {
    bvh_always_inline static void set(Vector<Scalar, N>&) {}
};

/// An N-dimensional vector class.
template <typename Scalar, size_t N>
struct Vector {
    Scalar values[N];

    Vector() = default;
    bvh_always_inline explicit Vector(Scalar s) { std::fill(values, values + N, s); }

    template <size_t M, std::enable_if_t<(M > N), int> = 0>
    bvh_always_inline explicit Vector(const Vector<Scalar, M>& other) {
        std::copy(other.values, other.values + N, values);
    }

    template <typename... Args>
    bvh_always_inline Vector(Scalar first, Scalar second, Args... args) {
        set(first, second, args...);
    }

    template <typename F>
    bvh_always_inline Vector(F f) {
        for (size_t i = 0; i < N; ++i)
            values[i] = f(i);
    }

    template <typename F>
    bvh_always_inline Vector(std::vector<F> v){
        for (size_t i = 0; i < N; ++i)
            values[i] = v[i];
    }

    template <typename... Args>
    bvh_always_inline void set(Args... args) {
        VectorSetter<0, Scalar, N>::set(*this, Scalar(args)...);
    }

    bvh_always_inline Vector operator - () const {
        return Vector([this] (size_t i) { return -values[i]; });
    }

    bvh_always_inline Vector inverse() const {
        return Vector([this] (size_t i) { return Scalar(1) / values[i]; });
    }

    bvh_always_inline Vector safe_inverse() const {
        static constexpr auto threshold = std::numeric_limits<Scalar>::epsilon();
        return Vector([&] (size_t i) {
            return Scalar(1) / (std::fabs(values[i]) < threshold ? std::copysign(threshold, values[i]) : values[i]);
        });
    }

    bvh_always_inline Vector& operator += (const Vector& other) {
        return *this = *this + other;
    }

    bvh_always_inline Vector& operator -= (const Vector& other) {
        return *this = *this - other;
    }

    bvh_always_inline Vector& operator *= (const Vector& other) {
        return *this = *this * other;
    }

    bvh_always_inline Scalar& operator [] (size_t i) { return values[i]; }
    bvh_always_inline Scalar  operator [] (size_t i) const { return values[i]; }
};

template <typename Scalar, size_t N>
bvh_always_inline
Vector<Scalar, N> operator + (const Vector<Scalar, N>& a, const Vector<Scalar, N>& b) {
    Vector<Scalar, N> v;
    for (size_t i = 0; i < N; i++)
    {
        v[i] = a[i] + b[i];
    }
    return v;
}

template <typename Scalar, size_t N>
bvh_always_inline
Vector<Scalar, N> operator - (const Vector<Scalar, N>& a, const Vector<Scalar, N>& b) {
    Vector<Scalar, N> v;
    for (size_t i = 0; i < N; i++)
    {
        v[i] = a[i] - b[i];
    }
    return v;
}

template <typename Scalar, size_t N>
bvh_always_inline
Vector<Scalar, N> operator * (const Vector<Scalar, N>& a, const Vector<Scalar, N>& b) {
    Vector<Scalar, N> v;
    for (size_t i = 0; i < N; i++)
    {
        v[i] = a[i] * b[i];
    }
    return v;
}

template <typename Scalar, size_t N>
bvh_always_inline
Vector<Scalar, N> min(const Vector<Scalar, N>& a, const Vector<Scalar, N>& b) {
    Vector<Scalar, N> v;
    for (size_t i = 0; i < N; i++)
    {
        v[i] = std::min(a[i], b[i]);
    }
    return v;
}

template <typename Scalar, size_t N>
bvh_always_inline
Vector<Scalar, N> max(const Vector<Scalar, N>& a, const Vector<Scalar, N>& b) {
    Vector<Scalar, N> v;
    for (size_t i = 0; i < N; i++)
    {
        v[i] = std::max(a[i], b[i]);
    }
    return v;
}

template <typename Scalar, size_t N>
bvh_always_inline
Vector<Scalar, N> operator * (const Vector<Scalar, N>& a, Scalar s) {
    Vector<Scalar, N> v;
    for (size_t i = 0; i < N; i++)
    {
        v[i] = a[i] * s;
    }
    return v;
}

template <typename Scalar, size_t N>
bvh_always_inline
Vector<Scalar, N> operator * (Scalar s, const Vector<Scalar, N>& b) {
    return b * s;
}

template <typename Scalar, size_t N>
bvh_always_inline
Scalar dot(const Vector<Scalar, N>& a, const Vector<Scalar, N>& b) {
    Scalar sum = a[0] * b[0];
    for (size_t i = 1; i < N; ++i)
        sum += a[i] * b[i];
    return sum;
}

template <typename Scalar, size_t N>
bvh_always_inline
Scalar length(const Vector<Scalar, N>& v) {
    return std::sqrt(dot(v, v));
}

template <typename Scalar, size_t N>
bvh_always_inline
Vector<Scalar, N> normalize(const Vector<Scalar, N>& v) {
    auto inv = Scalar(1) / length(v);
    return v * inv;
}

template <typename Scalar>
using Vector3 = Vector<Scalar, 3>;

template <typename Scalar>
bvh_always_inline
Vector3<Scalar> cross(const Vector3<Scalar>& a, const Vector3<Scalar>& b) {
    return Vector3<Scalar>([=] (size_t i) {
        size_t j = (i + 1) % 3;
        size_t k = (i + 2) % 3;
        return a[j] * b[k] - a[k] * b[j];
    });
}

} // namespace bvh

#endif
