// SPDX-License-Identifier: GPL-2.0
#include "aligned_buf.hpp"	// For assume_aligned

template <size_t N, typename T1, typename T2, typename FUNC>
inline void scramble_internal(const T1 *__restrict__ in, T2 *__restrict__ out, FUNC fn)
{
	in = assume_aligned(in);
	out = assume_aligned(out);

	for (size_t i = 0; i < N; ++i) {
		for (size_t j = 0; j < N; j++)
			*out++ = fn(*in++);
		in += N;
		out += N;
	}
}

template <size_t N, typename T1, typename T2, typename FUNC>
inline void scramble(const T1 *__restrict__ in,
		     T2 *__restrict__ out,
		     FUNC fn)
{
	scramble_internal<N/2, T1, T2>(in, out + N/2 + (N * N/2), fn);
	scramble_internal<N/2, T1, T2>(in + N/2, out + (N * N/2), fn);
	scramble_internal<N/2, T1, T2>(in + (N/2*2 * N/2), out + N/2, fn);
	scramble_internal<N/2, T1, T2>(in + N/2 + (N * N/2), out, fn);
}
