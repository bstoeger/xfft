// SPDX-License-Identifier: GPL-2.0
#include "operator_merge.hpp"
#include "document.hpp"

void OperatorMerge::init()
{
	init_simple(icon);
}

size_t OperatorMerge::num_input() const
{
	return 2;
}

size_t OperatorMerge::num_output() const
{
	return 1;
}

// We have to consider four cases:
// 1) Input amplitudes is empty
//	-> Output buffers are empty
// 2) Input amplitudes is real and input phases is empty
//	-> Output is a copy of amplitudes
// 3) Input amplitudes is complex and input phases is empty
//	-> Output is real
// 4) Input amplitude is non-empty and input phases is non-empty
//	-> Output is complex
//	-> Both output buffers are real
bool OperatorMerge::input_connection_changed()
{
	if (input_connectors[0]->is_empty_buffer()) {
		// Amplitudes empty
		return make_output_empty(0);
	}

	FFTBuf &in_buf1 = input_connectors[0]->get_buffer();
	if (input_connectors[1]->is_empty_buffer()) {
		if (in_buf1.is_complex()) {
			// Amplitudes complex and phases empty
			return make_output_real(0);
		} else {
			// Amplitudes real and phases empty
			return make_output_forwarded(0, input_connectors[0]->get_buffer());
		}
	}

	// Both are non-empty
	return make_output_complex(0);
}

template<size_t N>
void OperatorMerge::calculate()
{
	constexpr size_t n = N*N;
	FFTBuf &amplitude_buf = input_connectors[0]->get_buffer();
	if (input_connectors[1]->is_empty_buffer()) {
		if (!amplitude_buf.is_complex())
			return; // Simply copy -> nothing to do

		// Extract amplitudes
		std::complex<double> *in = amplitude_buf.get_complex_data();
		double *out = output_buffers[0].get_real_data();
		for (size_t i = 0; i < n; ++i) {
			std::complex<double> c = *in++;
			*out++ = std::abs(c);
		}
		double max_norm = amplitude_buf.get_max_norm();
		output_buffers[0].set_extremes(Extremes(max_norm));
		return;
	}

	// Both buffers are non-empty - we have to consider four cases
	// (each buffer can be real or complex)
	FFTBuf &phase_buf = input_connectors[1]->get_buffer();
	std::complex<double> *out = output_buffers[0].get_complex_data();
	output_buffers[0].set_extremes(amplitude_buf.get_extremes());

	if (amplitude_buf.is_complex()) {
		std::complex<double> *amplitude_in = amplitude_buf.get_complex_data();
		if (phase_buf.is_complex()) {
			// Complex amplitudes, complex phases
			std::complex<double> *phase_in = phase_buf.get_complex_data();
			for (size_t i = 0; i < n; ++i) {
				double amp = std::abs(*amplitude_in++);
				double phase = std::arg(*phase_in++);
				std::complex<double> d = std::polar(amp, phase);
				*out++ = d;
			}
		} else {
			// Complex amplitudes, real phases
			double *phase_in = phase_buf.get_real_data();
			for (size_t i = 0; i < n; ++i) {
				double amp = std::abs(*amplitude_in++);
				double phase = *phase_in++ * M_PI;
				std::complex<double> d = std::polar(amp, phase);
				*out++ = d;
			}
		}
	} else {
		double *amplitude_in = amplitude_buf.get_real_data();
		if (phase_buf.is_complex()) {
			// Real amplitudes, complex phases
			std::complex<double> *phase_in = phase_buf.get_complex_data();
			for (size_t i = 0; i < n; ++i) {
				double amp = *amplitude_in++;
				double phase = std::arg(*phase_in++);
				std::complex<double> d = std::polar(amp, phase);
				*out++ = d;
			}
		} else {
			// Real amplitudes, real phases
			double *phase_in = phase_buf.get_real_data();
			for (size_t i = 0; i < n; ++i) {
				double amp = *amplitude_in++;
				double phase = *phase_in++ * M_PI;
				std::complex<double> d = std::polar(amp, phase);
				*out++ = d;
			}
		}
	}
}

void OperatorMerge::execute()
{
	if (input_connectors[0]->is_empty_buffer())
		return; // Empty -> nothing to do

	dispatch_calculate(*this);
}
