// SPDX-License-Identifier: GPL-2.0
#include "fft_complete.hpp"
#include "fft_plan.hpp"

#include <fftw3.h>
#include <cassert>

template <size_t N>
inline void ConvolutionPlan::execute()
{
	assert(in1.get_size() == N);

	if (!plan1) {
		out.clear();
		return;
	}

	// Execute forward FFTs
	fftw_execute(static_cast<fftw_plan>(plan1));
	fftw_execute(static_cast<fftw_plan>(plan2));

	// Optionally complete real data
	if (in1_is_complex != in2_is_complex) {
		std::complex<double> *mid = in1_is_complex ? mid2.get() : mid1.get();
		fft_complete<N>(temp.get(), mid, [](std::complex<double> d) { return d; });
	}

	// Multiply frequencies
	std::complex<double> * __restrict__ freq1 = assume_aligned(mid1.get());
	std::complex<double> * __restrict__ freq2 = assume_aligned(mid2.get());
	if (in1_is_complex || in2_is_complex) {
		for (size_t i = 0; i < N * N; ++i) {
			*freq1++ *= *freq2++;
		}
	} else {
		for (size_t i = 0; i < N * (N / 2 + 1); ++i) {
			*freq1++ *= *freq2++;
		}
	}

	// Execute reverse transform, scale and collect min, max
	fftw_execute(static_cast<fftw_plan>(plan3));

	Extremes minmax;
	constexpr double factor = 1.0 / static_cast<double>(N);
	if (in1_is_complex || in2_is_complex) {
		std::complex<double> *data = out.get_complex_data();
		for (size_t i = 0; i < N * N; ++i) {
			// Note that *data is multiplied by factor
			minmax.reg(*data++, factor);
		}
	} else {
		double *data = out.get_real_data();
		for (size_t i = 0; i < N * N; ++i) {
			// Note that *data is multiplied by factor
			minmax.reg(*data++, factor);
		}
	}
	out.set_extremes(minmax);
}
