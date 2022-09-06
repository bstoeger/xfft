// SPDX-License-Identifier: GPL-2.0
// This class template implements an (over) aligned buffer.
// At the moment we align at 64 bytes, hoping that this is sufficient for
// getting optimal fftw plans and SIMD optimizations. Increasing this value
// is trivial.

#ifndef ALIGNED_BUF_HPP
#define ALIGNED_BUF_HPP

#include <cstddef>	// For size_t

template <typename T>
class AlignedBuf {
	T *buf;
public:
	constexpr static size_t align = 64;

	AlignedBuf();				// Undefined buffer
	AlignedBuf(size_t n);			// Buffer containing n elements
	AlignedBuf(AlignedBuf &&);		// Move buffer
	AlignedBuf &operator=(AlignedBuf &&);	// Move buffer
	~AlignedBuf();

	bool operator!() const;
	operator bool() const;

	T *get();
	const T *get() const;
};

#include "aligned_buf_impl.hpp"

#endif
