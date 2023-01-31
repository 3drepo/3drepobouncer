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

#ifndef BVH_BOUNDING_BOX_HPP
#define BVH_BOUNDING_BOX_HPP

#include "vector.hpp"

namespace bvh {

/// A bounding box, represented with two extreme points.
template <typename Scalar>
struct BoundingBox {
    Vector3<Scalar> min, max;

    BoundingBox() = default;
    bvh_always_inline BoundingBox(const Vector3<Scalar>& v) : min(v), max(v) {}
    bvh_always_inline BoundingBox(const Vector3<Scalar>& min, const Vector3<Scalar>& max) : min(min), max(max) {}

    bvh_always_inline BoundingBox& shrink(const BoundingBox& bbox) {
        min = bvh::max(min, bbox.min);
        max = bvh::min(max, bbox.max);
        return *this;
    }

    bvh_always_inline BoundingBox& extend(const BoundingBox& bbox) {
        min = bvh::min(min, bbox.min);
        max = bvh::max(max, bbox.max);
        return *this;
    }

    bvh_always_inline BoundingBox& extend(const Vector3<Scalar>& v) {
        min = bvh::min(min, v);
        max = bvh::max(max, v);
        return *this;
    }

    bvh_always_inline Vector3<Scalar> diagonal() const {
        return max - min;
    }

    bvh_always_inline Vector3<Scalar> center() const {
        return (max + min) * Scalar(0.5);
    }

    bvh_always_inline Scalar half_area() const {
        auto d = diagonal();
        return (d[0] + d[1]) * d[2] + d[0] * d[1];
    }

    bvh_always_inline Scalar volume() const {
        auto d = diagonal();
        return d[0] * d[1] * d[2];
    }

    bvh_always_inline unsigned largest_axis() const {
        auto d = diagonal();
        unsigned axis = 0;
        if (d[0] < d[1]) axis = 1;
        if (d[axis] < d[2]) axis = 2;
        return axis;
    }

    bvh_always_inline Scalar largest_extent() const {
        return diagonal()[largest_axis()];
    }

    bvh_always_inline bool is_contained_in(const BoundingBox& other) const {
        return
            max[0] <= other.max[0] && min[0] >= other.min[0] &&
            max[1] <= other.max[1] && min[1] >= other.min[1] &&
            max[2] <= other.max[2] && min[2] >= other.min[2];
    }

    bvh_always_inline static BoundingBox full() {
        return BoundingBox(
            Vector3<Scalar>(-std::numeric_limits<Scalar>::max()),
            Vector3<Scalar>(std::numeric_limits<Scalar>::max()));
    }

    bvh_always_inline static BoundingBox empty() {
        return BoundingBox(
            Vector3<Scalar>(std::numeric_limits<Scalar>::max()),
            Vector3<Scalar>(-std::numeric_limits<Scalar>::max()));
    }
};

} // namespace bvh

#endif
