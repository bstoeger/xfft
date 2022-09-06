// SPDX-License-Identifier: GPL-2.0
#include "aligned_buf.hpp"	// For assume_aligned

template <size_t N, typename T1, typename T2, typename T3, typename FUNC>
void transform_data(const T1 *in1, const T2 *in2, T3 *out, FUNC fn)
{
	constexpr size_t n = N * N;
	for (size_t i = 0; i < n; ++i)
		*out++ = fn(*in1++, *in2++);
}

template <size_t N, typename T1, typename T2, typename T3, typename FUNC>
void transform_data(FFTBuf &in1, FFTBuf &in2, FFTBuf &out, FUNC fn)
{
	transform_data<N, T1, T2, T3>(in1.get_data<T1>(), in2.get_data<T2>(), out.get_data<T3>(), fn);
}
