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

#ifndef BVH_SPHERE_HPP
#define BVH_SPHERE_HPP

#include <optional>

#include "vector.hpp"
#include "bounding_box.hpp"
#include "ray.hpp"

namespace bvh {

/// Sphere primitive defined by a center and a radius.
template <typename Scalar>
struct Sphere  {
    struct Intersection {
        Scalar  t;

        Scalar distance() const { return t; }
    };

    using ScalarType       = Scalar;
    using IntersectionType = Intersection;

    Vector3<Scalar> origin;
    Scalar radius;

    Sphere() = default;
    Sphere(const Vector3<Scalar>& origin, Scalar radius)
        : origin(origin), radius(radius)
    {}

    Vector3<Scalar> center() const {
        return origin;
    }

    BoundingBox<Scalar> bounding_box() const {
        return BoundingBox<Scalar>(origin - Vector3<Scalar>(radius), origin + Vector3<Scalar>(radius));
    }

    template <bool AssumeNormalized = false>
    std::optional<Intersection> intersect(const Ray<Scalar>& ray) const {
        auto oc = ray.origin - origin;
        auto a = AssumeNormalized ? Scalar(1) : dot(ray.direction, ray.direction);
        auto b = 2 * dot(ray.direction, oc);
        auto c = dot(oc, oc) - radius * radius;

        auto delta = b * b - 4 * a * c;
        if (delta >= 0) {
            auto inv = -Scalar(0.5) / a;
            auto sqrt_delta = std::sqrt(delta);
            auto t0 = (b + sqrt_delta) * inv;
            if (t0 >= ray.tmin && t0 <= ray.tmax)
                return std::make_optional(Intersection { t0 });
            auto t1 = (b - sqrt_delta) * inv;
            if (t1 >= ray.tmin && t1 <= ray.tmax)
                return std::make_optional(Intersection { t1 });
        }

        return std::nullopt;
    }
};

} // namespace bvh

#endif
