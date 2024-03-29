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

#ifndef BVH_PREFIX_SUM_HPP
#define BVH_PREFIX_SUM_HPP

#include <functional>
#include <memory>
#include <numeric>

#include "utilities.hpp"

namespace bvh {

/// Parallel prefix sum. The parallel algorithm used in this implementation
/// needs twice the work as the naive serial version, and is thus enabled
/// only if the number of cores if greater or equal than 3.
template <typename T>
class PrefixSum {
public:
    /// Performs the prefix sum. Must be called from a parallel region.
    template <typename F = std::plus<T>>
    void sum_in_parallel(const T* input, T* output, size_t count, F f = F()) {
        bvh::assert_in_parallel();

        size_t thread_count = bvh::get_thread_count();
        size_t thread_id    = bvh::get_thread_id();

        // This algorithm is not effective when there are fewer than 2 threads.
        if (thread_count <= 2) {
            #pragma omp single
            { std::partial_sum(input, input + count, output, f); }
            return;
        }

        // Allocate temporary storage
        #pragma omp single
        {
            if (per_thread_data_size < thread_count + 1) {
                per_thread_sums = std::make_unique<T[]>(thread_count + 1);
                per_thread_data_size = thread_count + 1;
                per_thread_sums[0] = 0;
            }
        }

        T sum = T(0);

        // Compute partial sums
        #pragma omp for nowait schedule(static)
        for (size_t i = 0; i < count; ++i) {
            sum = f(sum, input[i]);
            output[i] = sum;
        }
        per_thread_sums[thread_id + 1] = sum;

        #pragma omp barrier

        // Fix the sums
        auto offset = std::accumulate(per_thread_sums.get(), per_thread_sums.get() + thread_id + 1, T(0), f);

        #pragma omp for schedule(static)
        for (size_t i = 0; i < count; ++i)
            output[i] = f(output[i], offset);
    }

private:
    std::unique_ptr<T[]> per_thread_sums;
    size_t per_thread_data_size = 0;
};

} // namespace bvh

#endif
