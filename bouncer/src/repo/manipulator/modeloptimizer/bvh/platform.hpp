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

#ifndef BVH_PLATFORM_HPP
#define BVH_PLATFORM_HPP

#include <cstddef>
#include <cassert>

#ifdef _OPENMP
#include <omp.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
#define bvh_restrict      __restrict
#define bvh_always_inline __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define bvh_restrict      __restrict
#define bvh_always_inline __forceinline
#else
#define bvh_restrict
#define bvh_always_inline inline
#endif

#if defined(__GNUC__) || defined(__clang__)
#define bvh_likely(x)   __builtin_expect(x, true)
#define bvh_unlikely(x) __builtin_expect(x, false)
#else
#define bvh_likely(x)   x
#define bvh_unlikely(x) x
#endif

namespace bvh {

#ifdef _OPENMP
inline size_t get_thread_count() { return omp_get_num_threads(); }
inline size_t get_thread_id()    { return omp_get_thread_num(); }
inline void assert_not_in_parallel() { assert(omp_get_level() == 0); }
inline void assert_in_parallel() { assert(omp_get_level() > 0); }
#else
inline constexpr size_t get_thread_count() { return 1; }
inline constexpr size_t get_thread_id()    { return 0; }
inline void assert_not_in_parallel() {}
inline void assert_in_parallel() {}
#endif

} // namespace bvh

#endif
