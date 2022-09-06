// SPDX-License-Identifier: GPL-2.0
// Wrapper around FFTW plan, that allocates and deallocates memory.
//
// The input buffer can either be real, complex, or empty.
// The plan can either be forward (F) or backward (F^-1).
// Either the complex Fourier transform or its real norm is calculated.
// The output buffer must accordingly be either complex or real.
//
// This gives quite a lot of combinations which makes the code quite intricate.
// It might be better to split this into different classes.

#ifndef FFT_PLAN_HPP
#define FFT_PLAN_HPP

#include "aligned_buf.hpp"

#include <complex>

class FFTBuf;

class FFTPlan {
	FFTBuf &in;
	AlignedBuf<std::complex<double>> mid;		// Intermediate buffer for real transforms.
	FFTBuf &out;
	void *plan;					// nullptr if input is empty (i.e. generate empty output).

	// The following are marked const, because the kind of plan can not change.
	// To change, destroy and recreate.
	const bool forward;				// Forward or backward transform.
	const bool norm;				// Calculate norm of complex.
	const bool in_is_complex;
public:
	FFTPlan(FFTBuf &in, FFTBuf &out, bool forward, bool norm);
	~FFTPlan();

	// Fill in buffer and call execute.
	// Realized as template so that optimizer can make individual functions
	// for each size. This is most likely not worth it.
	// Include "fft_plan_impl.hpp" to access this template.
	template<size_t N>
	void execute();
};

#endif
