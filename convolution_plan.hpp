// SPDX-License-Identifier: GPL-2.0
// Calculate the convolution of two buffers by multiplication of two
// Fourier transforms, followed by an inverse Fourier transform.
// The input buffers can be either real or complex. If at least one
// of the input buffers is complex, so must be the output buffer.
#ifndef CONVOLUTION_PLAN_HPP
#define CONVOLUTION_PLAN_HPP

#include "aligned_buf.hpp"

#include <complex>

class FFTBuf;

class ConvolutionPlan {
	FFTBuf &in1;
	FFTBuf &in2;
	AlignedBuf<std::complex<double>> mid1;
	AlignedBuf<std::complex<double>> mid2;
	AlignedBuf<std::complex<double>> temp;	// Intermediate buffer for real to complex transforms
	FFTBuf &out;
	void *plan1;				// nullptr if input is empty (i.e. generate empty output)
	void *plan2;				// nullptr if input is empty (i.e. generate empty output)
	void *plan3;				// nullptr if input is empty (i.e. generate empty output)
	bool in1_is_complex;
	bool in2_is_complex;
public:
	ConvolutionPlan(FFTBuf &in1, FFTBuf &in2, FFTBuf &out);
	~ConvolutionPlan();

	// Realized as template so that optimizer can make individual functions
	// for each size. This is most likely not worth it.
	// Include "convolution_plan_impl.hpp" to access this template.
	template<size_t N>
	void execute();
};

#endif
