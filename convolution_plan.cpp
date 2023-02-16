// SPDX-License-Identifier: GPL-2.0
#include "convolution_plan.hpp"
#include "fft_buf.hpp"
#include "fft_complete.hpp"

#include <fftw3.h>
#include <cassert>

ConvolutionPlan::ConvolutionPlan(FFTBuf &in1_, FFTBuf &in2_, FFTBuf &out_)
	: in1(in1_)
	, in2(in2_)
	, out(out_)
	, in1_is_complex(in1.is_complex())
	, in2_is_complex(in2.is_complex())
{

	size_t n = in1.get_size();
	assert(n == in2.get_size());
	assert(n == out.get_size());

	if (in1.is_empty() || in2.is_empty()) {
		plan1 = nullptr;
		plan2 = nullptr;
		plan3 = nullptr;
		return;
	}

	assert(out.is_complex() == (in1_is_complex || in2_is_complex));
	if (in1_is_complex || in2_is_complex) {
		mid1 = AlignedBuf<std::complex<double>>(n * n);
		mid2 = AlignedBuf<std::complex<double>>(n * n);
	} else {
		mid1 = AlignedBuf<std::complex<double>>(n * (n / 2 + 1));
		mid2 = AlignedBuf<std::complex<double>>(n * (n / 2 + 1));
	}
	if (in1_is_complex != in2_is_complex) {
		temp = AlignedBuf<std::complex<double>>(n * (n / 2 + 1));
	}

	if (in1_is_complex) {
		auto save = in1.save();
		plan1 = fftw_plan_dft_2d(n, n, reinterpret_cast<fftw_complex*>(in1.get_complex_data()),
					reinterpret_cast<fftw_complex*>(mid1.get()), 1, FFTW_MEASURE);
		in1.restore(save);
	}
	if (in2_is_complex) {
		auto save = in2.save();
		plan2 = fftw_plan_dft_2d(n, n, reinterpret_cast<fftw_complex*>(in2.get_complex_data()),
					reinterpret_cast<fftw_complex*>(mid2.get()), 1, FFTW_MEASURE);
		in2.restore(save);
	}
	if (!in1_is_complex && !in2_is_complex) {
		auto save1 = in1.save();
		plan1 = fftw_plan_dft_r2c_2d(n, n, in1.get_real_data(),
					reinterpret_cast<fftw_complex*>(mid1.get()), FFTW_MEASURE);
		in1.restore(save1);

		auto save2 = in2.save();
		plan2 = fftw_plan_dft_r2c_2d(n, n, in2.get_real_data(),
					reinterpret_cast<fftw_complex*>(mid2.get()), FFTW_MEASURE);
		in2.restore(save2);
	}
	if (!in1_is_complex && in2_is_complex) {
		auto save1 = in1.save();
		plan1 = fftw_plan_dft_r2c_2d(n, n, in1.get_real_data(),
					reinterpret_cast<fftw_complex*>(temp.get()), FFTW_MEASURE);
		in1.restore(save1);
	}
	if (in1_is_complex && !in2_is_complex) {
		auto save2 = in2.save();
		plan2 = fftw_plan_dft_r2c_2d(n, n, in2.get_real_data(),
					reinterpret_cast<fftw_complex*>(temp.get()), FFTW_MEASURE);
		in2.restore(save2);
	}
	if (in1_is_complex || in2_is_complex) {
		plan3 = fftw_plan_dft_2d(n, n, reinterpret_cast<fftw_complex*>(mid1.get()),
					reinterpret_cast<fftw_complex*>(out.get_complex_data()), -1, FFTW_MEASURE);
	} else {
		plan3 = fftw_plan_dft_c2r_2d(n, n, reinterpret_cast<fftw_complex*>(mid1.get()),
					    out.get_real_data(), FFTW_MEASURE);
	}
}

ConvolutionPlan::~ConvolutionPlan()
{
	fftw_destroy_plan(static_cast<fftw_plan>(plan1));
	fftw_destroy_plan(static_cast<fftw_plan>(plan2));
	fftw_destroy_plan(static_cast<fftw_plan>(plan3));
}

void ConvolutionPlan::execute()
{

	if (!plan1) {
		out.clear();
		return;
	}

	// Execute forward FFTs
	fftw_execute(static_cast<fftw_plan>(plan1));
	fftw_execute(static_cast<fftw_plan>(plan2));

	// Optionally complete real data
	size_t N = in1.get_size();
	if (in1_is_complex != in2_is_complex) {
		std::complex<double> *mid = in1_is_complex ? mid2.get() : mid1.get();
		fft_complete(N, temp.get(), mid, [](std::complex<double> d) { return d; });
	}

	// Multiply frequencies
	std::complex<double> * __restrict__ freq1 = assume_aligned(mid1.get());
	std::complex<double> * __restrict__ freq2 = assume_aligned(mid2.get());
	if (in1_is_complex || in2_is_complex) {
		for (size_t i = 0; i < N * N; ++i)
			*freq1++ *= *freq2++;
	} else {
		for (size_t i = 0; i < N * (N / 2 + 1); ++i)
			*freq1++ *= *freq2++;
	}

	// Execute reverse transform, scale and collect min, max
	fftw_execute(static_cast<fftw_plan>(plan3));

	Extremes minmax;
	double factor = 1.0 / static_cast<double>(N);
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
