// SPDX-License-Identifier: GPL-2.0
#include "aligned_buf.hpp"

template <typename T>
static T my_conj(T);

template <>
std::complex<double>
inline my_conj<std::complex<double>> (std::complex<double> d)
{
	return std::conj(d);
}

template <>
double
inline my_conj<double> (double d)
{
	return d;
}

template <typename T1, typename T2, typename FUNC>
static inline void fft_complete(size_t N, T1 * __restrict__ in, T2 * __restrict__ data, FUNC fn)
{
	in = assume_aligned(in);
	data = assume_aligned(data);

	T2 *out = data;
	T2 *out2 = data + N;
	*out++ = fn(std::conj(*in++));
	for (size_t x = 0; x < N / 2 - 1; ++x) {
		T2 d = fn(*in++);
		*out++ = my_conj(d);
		*--out2 = d;
	}
	*out++ = fn(std::conj(*in++));
	out += N / 2 - 1;
	out2 = data + N*N;

	for (size_t y = 0; y < N - 1; ++y) {
		*out++ = fn(std::conj(*in++));
		for (size_t x = 0; x < N / 2 - 1; ++x) {
			T2 d = fn(*in++);
			*out++ = my_conj(d);
			*--out2 = d;
		}
		*out++ = fn(std::conj(*in++));
		out += N / 2 - 1;
		out2 -= N / 2 + 1;
	}
}
