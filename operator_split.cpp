// SPDX-License-Identifier: GPL-2.0
#include "operator_split.hpp"
#include "document.hpp"

void OperatorSplit::init()
{
	init_simple(icon);
}

// We have to consider three cases:
// 1) Input buffer is empty
//	-> Both output buffers are empty
// 2) Input buffer is real
//	-> Amplitudes are copy of input
//	-> Phases are empty
// 3) Input buffer is complex
//	-> Both output buffers are real
bool OperatorSplit::input_connection_changed()
{
	if (input_connectors[0]->is_empty_buffer()) {
		// Empty
		return make_output_empty(0) |
		       make_output_empty(1);
	}

	FFTBuf &buf = input_connectors[0]->get_buffer();
	if (buf.is_complex()) {
		// Complex
		return make_output_real(0) |
		       make_output_real(1);
	} else {
		// Real
		return make_output_forwarded(0, input_connectors[0]->get_buffer()) |
		       make_output_empty(1);
	}
}

void OperatorSplit::execute()
{
	if (input_connectors[0]->is_empty_buffer())
		return; // Empty -> nothing to do

	FFTBuf &buf = input_connectors[0]->get_buffer();
	if (!buf.is_complex()) {
		// If input is real, output is simply a copy -> nothing to do
		return;
	}

	size_t fft_size = get_fft_size();
	size_t n = fft_size * fft_size;
	std::complex<double> *in = buf.get_complex_data();
	double *out_amplitudes = output_buffers[0].get_real_data();
	double *out_phases = output_buffers[1].get_real_data();

	for (size_t i = 0; i < n; ++i) {
		std::complex<double> c = *in++;
		*out_amplitudes++ = std::abs(c);
		*out_phases++ = std::arg(c) / M_PI;
	}

	output_buffers[0].set_extremes(buf.get_extremes());
	output_buffers[1].set_extremes(Extremes(1.0));
}
