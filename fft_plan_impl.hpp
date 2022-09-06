// SPDX-License-Identifier: GPL-2.0
#include "fft_complete.hpp"
#include "fft_buf.hpp"

#include <cassert>
#include <fftw3.h>

template <size_t N>
inline void FFTPlan::execute()
{
	assert(in.get_size() == N);
	if (!plan) {
		out.clear();
		return;
	}

	fftw_execute(static_cast<fftw_plan>(plan));

	// Renormalize and calculate maximum, respectively complete for real data
	Extremes minmax;
	double factor = 1.0 / static_cast<double>(N);

	if (in_is_complex) {
		if (norm) {
			std::complex<double> *from = mid.get();
			double *data = out.get_real_data();
			for (size_t i = 0; i < N * N; ++i) {
				*data = std::norm(*from++);
				// Note that *data is multiplied by factor
				minmax.reg(*data++, factor);
			}
		} else {
			std::complex<double> *data = out.get_complex_data();
			for (size_t i = 0; i < N * N; ++i) {
				// Note that *data is multiplied by factor
				minmax.reg(*data++, factor);
			}
		}
	} else {
		if (norm) {
			fft_complete<N>(mid.get(), out.get_real_data(),
				        [&minmax, factor](std::complex<double> d)
					{ double d2 = std::norm(d);
					return minmax.reg(d2, factor); });
		} else if (forward) {
			fft_complete<N>(mid.get(), out.get_complex_data(),
					[&minmax, factor](std::complex<double> d)
					{ return minmax.reg(d, factor); });
		} else {
			fft_complete<N>(mid.get(), out.get_complex_data(),
					[&minmax, factor](std::complex<double> d)
					{ return std::conj(minmax.reg(d, factor)); });
		}
	}
	out.set_extremes(minmax);
}
