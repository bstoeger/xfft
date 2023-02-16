// SPDX-License-Identifier: GPL-2.0
#include "fft_plan.hpp"
#include "fft_buf.hpp"
#include "fft_complete.hpp"

#include <fftw3.h>
#include <cassert>

FFTPlan::FFTPlan(FFTBuf &in_, FFTBuf &out_, bool forward_, bool norm_)
	: in(in_)
	, out(out_)
	, forward(forward_)
	, norm(norm_)
	, in_is_complex(in.is_complex())
{
	assert((norm && out.is_real()) ||
	       (!norm && out.is_complex()));

	size_t n = in.get_size();
	assert(n == out.get_size());

	if (norm) {
		mid = AlignedBuf<std::complex<double>>(in_is_complex ? n * n : n * (n / 2 + 1));
	}

	if (in.is_empty()) {
		plan = nullptr;
	} else if (in_is_complex) {
		auto save = in.save();
		auto out_buf = reinterpret_cast<fftw_complex*>(
			norm ? mid.get() : out.get_complex_data()
		);

		plan = fftw_plan_dft_2d(n, n, reinterpret_cast<fftw_complex*>(in.get_complex_data()),
					out_buf, forward ? 1 : -1, FFTW_MEASURE);
		in.restore(save);
	} else {
		mid = AlignedBuf<std::complex<double>>(n * (n / 2 + 1));
		auto save = in.save();
		plan = fftw_plan_dft_r2c_2d(n, n, in.get_real_data(),
					    reinterpret_cast<fftw_complex*>(mid.get()),
					    FFTW_MEASURE);
		in.restore(save);
	}
}

FFTPlan::~FFTPlan()
{
	fftw_destroy_plan(static_cast<fftw_plan>(plan));
}

void FFTPlan::execute()
{
	if (!plan) {
		out.clear();
		return;
	}

	fftw_execute(static_cast<fftw_plan>(plan));

	// Renormalize and calculate maximum, respectively complete for real data
	Extremes minmax;
	size_t N = in.get_size();
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
			fft_complete(N, mid.get(), out.get_real_data(),
				        [&minmax, factor](std::complex<double> d)
					{ double d2 = std::norm(d);
					return minmax.reg(d2, factor); });
		} else if (forward) {
			fft_complete(N, mid.get(), out.get_complex_data(),
					[&minmax, factor](std::complex<double> d)
					{ return minmax.reg(d, factor); });
		} else {
			fft_complete(N, mid.get(), out.get_complex_data(),
					[&minmax, factor](std::complex<double> d)
					{ return std::conj(minmax.reg(d, factor)); });
		}
	}
	out.set_extremes(minmax);
}
