// SPDX-License-Identifier: GPL-2.0
// This header file declares a function that completes FFT buffers
// that resulted from real->complex transforms.
// The function is implemented as an inline template so that it can
// be inlined for speed. To each copied value a function is applied.
#ifndef FFT_COMPLETE_HPP
#define FFT_COMPLETE_HPP

#include <functional>

/*
TODO: Forward declaration produces errors
template <typename T1, typename T2, typename FUNC, size_t N>
static inline void fft_complete(T1 *in, T2 *data, FUNC fn);
*/

#include "fft_complete_impl.hpp"

#endif
