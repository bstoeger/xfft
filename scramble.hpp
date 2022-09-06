// SPDX-License-Identifier: GPL-2.0
// This header declares a scramble function that rearranges the quadrants
// of two-dimensional buffers, while applying a transformation.
// +-+-+    +-+-+
// |A|B|    |D|C|
// +-+-+ => +-+-+
// |C|D|    |B|A|
// +-+-+    +-+-+
//
// To allow for real and complex data, the function is realized as a template.

#ifndef SCRAMBLE_HPP
#define SCRAMBLE_HPP

#include <functional>

// Copy between iterators
template <size_t N, typename T1, typename T2, typename FUNC>
inline void scramble(const T1 *in, T2 * out, FUNC fn);

#include "scramble_impl.hpp"

#endif
