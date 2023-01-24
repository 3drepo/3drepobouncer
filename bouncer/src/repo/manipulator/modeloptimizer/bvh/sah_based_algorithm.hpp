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

#ifndef BVH_SAH_BASED_ALGORITHM_HPP
#define BVH_SAH_BASED_ALGORITHM_HPP

#include "bvh.hpp"

namespace bvh {

template <typename Bvh>
class SahBasedAlgorithm {
    using Scalar = typename Bvh::ScalarType;

public:
    /// Cost of intersecting a ray with a node of the data structure.
    /// This cost is relative to the cost of intersecting a primitive,
    /// which is assumed to be equal to 1.
    Scalar traversal_cost = 1;

protected:
    ~SahBasedAlgorithm() {}

    Scalar compute_cost(const Bvh& bvh) const {
        // Compute the SAH cost for the entire BVH
        Scalar cost(0);
        #pragma omp parallel for reduction(+: cost)
        for (size_t i = 0; i < bvh.node_count; ++i) {
            auto half_area = bvh.nodes[i].bounding_box_proxy().half_area();
            if (bvh.nodes[i].is_leaf())
                cost += half_area * static_cast<Scalar>(bvh.nodes[i].primitive_count);
            else
                cost += half_area * traversal_cost;
        }
        return cost / bvh.nodes[0].bounding_box_proxy().half_area();
    }
};

} // namespace bvh

#endif
