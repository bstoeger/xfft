// SPDX-License-Identifier: GPL-2.0
// This file declares functions that perform a binary operation
// on one or more data blocks and put the result in a third data block.
// It is assumed that the data is aligned according to AlignedBuf.
// A convenience wrapper operates directly on FFTBuf objects.

#ifndef TRANSFORM_DATA_HPP
#define TRANSFORM_DATA_HPP

#include <functional>

template <typename T1, typename T2, typename T3, typename FUNC>
void transform_data(size_t n, FFTBuf &in1, FFTBuf &in2, FFTBuf &out, FUNC fn);

template <typename T1, typename T2, typename T3, typename FUNC>
void transform_data(size_t n, const T1 *in1, const T2 *in2, T3 *out, FUNC fn);

#include "transform_data_impl.hpp"

#endif
