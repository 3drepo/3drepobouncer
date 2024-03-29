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

#ifndef BVH_BVH_HPP
#define BVH_BVH_HPP

#include <climits>
#include <memory>
#include <cassert>

#include "bounding_box.hpp"
#include "utilities.hpp"

namespace bvh {

/// This structure represents a BVH with a list of nodes and primitives indices.
/// The memory layout is such that the children of a node are always grouped together.
/// This means that each node only needs one index to point to its children, as the other
/// child can be obtained by adding one to the index of the first child. The root of the
/// hierarchy is located at index 0 in the array of nodes.
template <typename Scalar>
struct Bvh {
    using IndexType  = typename SizedIntegerType<sizeof(Scalar) * CHAR_BIT>::Unsigned;
    using ScalarType = Scalar;

    // The size of this structure should be 32 bytes in
    // single precision and 64 bytes in double precision.
    struct Node {
        Scalar bounds[6];
        IndexType primitive_count;
        IndexType first_child_or_primitive;

        bool is_leaf() const { return primitive_count != 0; }

        /// Accessor to simplify the manipulation of the bounding box of a node.
        /// This type is convertible to a `BoundingBox`.
        struct BoundingBoxProxy {
            Node& node;

            BoundingBoxProxy(Node& node)
                : node(node)
            {}

            BoundingBoxProxy& operator = (const BoundingBox<Scalar>& bbox) {
                node.bounds[0] = bbox.min[0];
                node.bounds[1] = bbox.max[0];
                node.bounds[2] = bbox.min[1];
                node.bounds[3] = bbox.max[1];
                node.bounds[4] = bbox.min[2];
                node.bounds[5] = bbox.max[2];
                return *this;
            }

            operator BoundingBox<Scalar> () const {
                return BoundingBox<Scalar>(
                    Vector3<Scalar>(node.bounds[0], node.bounds[2], node.bounds[4]),
                    Vector3<Scalar>(node.bounds[1], node.bounds[3], node.bounds[5]));
            }

            BoundingBox<Scalar> to_bounding_box() const {
                return BoundingBox<Scalar>(
                    Vector3<Scalar>(node.bounds[0], node.bounds[2], node.bounds[4]),
                    Vector3<Scalar>(node.bounds[1], node.bounds[3], node.bounds[5]));
            }

            Scalar half_area() const { return to_bounding_box().half_area(); }

            BoundingBoxProxy& extend(const BoundingBox<Scalar>& bbox) {
                return *this = to_bounding_box().extend(bbox);
            }

            BoundingBoxProxy& extend(const Vector3<Scalar>& vector) {
                return *this = to_bounding_box().extend(vector);
            }
        };

        BoundingBoxProxy bounding_box_proxy() {
            return BoundingBoxProxy(*this);
        }

        const BoundingBoxProxy bounding_box_proxy() const {
            return BoundingBoxProxy(*const_cast<Node*>(this));
        }
    };

    /// Given a node index, returns the index of its sibling.
    static size_t sibling(size_t index) {
        assert(index != 0);
        return index % 2 == 1 ? index + 1 : index - 1;
    }

    /// Returns true if the given node is the left sibling of another.
    static bool is_left_sibling(size_t index) {
        assert(index != 0);
        return index % 2 == 1;
    }

    std::unique_ptr<Node[]>   nodes;
    std::unique_ptr<size_t[]> primitive_indices;

    size_t node_count = 0;
};

} // namespace bvh

#endif
