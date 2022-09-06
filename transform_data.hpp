// SPDX-License-Identifier: GPL-2.0
// This file declares functions that perform a binary or unary operation
// on one or more data blocks and put the result in a third data block.
// It is assumed that the data is aligned according to AlignedBuf.
// A convenience wrapper operates directly on FFTBuf objects.

#ifndef TRANSFORM_DATA_HPP
#define TRANSFORM_DATA_HPP

#include <functional>

template <size_t N, typename T1, typename T2, typename T3, typename FUNC>
void transform_data(const T1 *, const T2 *, T3 *, FUNC fn);

template <size_t N, typename T1, typename T2, typename T3, typename FUNC>
void transform_data(FFTBuf &, FFTBuf &, FFTBuf &, FUNC fn);

#include "transform_data_impl.hpp"

#endif
