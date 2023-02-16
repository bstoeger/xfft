// SPDX-License-Identifier: GPL-2.0

template <typename T1, typename T2, typename T3, typename FUNC>
void transform_data(size_t n, const T1 *in1, const T2 *in2, T3 *out, FUNC fn)
{
	for (size_t i = 0; i < n * n; ++i)
		*out++ = fn(*in1++, *in2++);
}

template <typename T1, typename T2, typename T3, typename FUNC>
void transform_data(size_t n, FFTBuf &in1, FFTBuf &in2, FFTBuf &out, FUNC fn)
{
	transform_data<T1, T2, T3>(n, in1.get_data<T1>(), in2.get_data<T2>(), out.get_data<T3>(), fn);
}
