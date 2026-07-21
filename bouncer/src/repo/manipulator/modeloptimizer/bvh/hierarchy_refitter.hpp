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

#ifndef BVH_HIERARCHY_REFITTER_HPP
#define BVH_HIERARCHY_REFITTER_HPP

#include "bvh.hpp"
#include "bottom_up_algorithm.hpp"
#include "platform.hpp"

namespace bvh {

template <typename Bvh>
class HierarchyRefitter : public BottomUpAlgorithm<Bvh> {
protected:
    using BottomUpAlgorithm<Bvh>::bvh;
    using BottomUpAlgorithm<Bvh>::traverse_in_parallel;
    using BottomUpAlgorithm<Bvh>::parents;

    template <typename UpdateLeaf>
    void refit_in_parallel(const UpdateLeaf& update_leaf) {
        bvh::assert_in_parallel();

        // Refit every node of the tree in parallel
        traverse_in_parallel(
            [&] (size_t i) { update_leaf(bvh.nodes[i]); },
            [&] (size_t i) {
                auto& node = bvh.nodes[i];
                auto first_child = node.first_child_or_primitive;
                node.bounding_box_proxy() = bvh.nodes[first_child + 0]
                    .bounding_box_proxy()
                    .to_bounding_box()
                    .extend(bvh.nodes[first_child + 1].bounding_box_proxy().to_bounding_box());
            });
    }

public:
    HierarchyRefitter(Bvh& bvh)
        : BottomUpAlgorithm<Bvh>(bvh)
    {}

    template <typename UpdateLeaf>
    void refit(const UpdateLeaf& update_leaf) {
        #pragma omp parallel
        {
            refit_in_parallel(update_leaf);
        }
    }
};

} // namespace bvh

#endif
