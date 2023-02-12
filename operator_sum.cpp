// SPDX-License-Identifier: GPL-2.0
#include "operator_sum.hpp"
#include "document.hpp"
#include "transform_data.hpp"

void OperatorSum::init()
{
	init_simple(icon);
}

bool OperatorSum::input_connection_changed()
{
	bool is_empty0 = input_connectors[0]->is_empty_buffer();
	bool is_empty1 = input_connectors[1]->is_empty_buffer();

	// Empty if both input buffers are empty.
	if (is_empty0 && is_empty1)
		return make_output_empty(0);

	// Copy of other buffer if one is empty.
	if (is_empty0)
		return make_output_forwarded(0, input_connectors[1]->get_buffer());
	if (is_empty1)
		return make_output_forwarded(0, input_connectors[0]->get_buffer());

	// Real if both input buffers are real.
	if (!input_connectors[0]->get_buffer().is_complex() &&
	   !input_connectors[1]->get_buffer().is_complex())
		return make_output_real(0);

	// Complex, because at least one is complex.
	return make_output_complex(0);
}
template <typename T1, typename T2, typename T3>
static T3 sum(T1 d1, T2 d2)
{
	return d1 + d2;
}

template<size_t N>
void OperatorSum::calculate()
{
	FFTBuf &buf1 = input_connectors[0]->get_buffer();
	FFTBuf &buf2 = input_connectors[1]->get_buffer();
	FFTBuf &buf_out = output_buffers[0];

	if (!buf1.is_complex() && !buf2.is_complex()) {
		// Add reals
		transform_data<N, double, double, double>
			(buf1, buf2, buf_out,
			sum<double, double, double>);
	} else if (buf1.is_complex() && !buf2.is_complex()) {
		// Add complex to real
		transform_data<N, std::complex<double>, double, std::complex<double>>
			(buf1, buf2, buf_out,
			sum<std::complex<double>, double, std::complex<double>>);
	} else if (!buf1.is_complex() && buf2.is_complex()) {
		// Add real to complex
		transform_data<N, double, std::complex<double>, std::complex<double>>
			(buf1, buf2, buf_out,
			sum<double, std::complex<double>, std::complex<double>>);
	} else {
		// Add complex values
		transform_data<N, std::complex<double>, std::complex<double>, std::complex<double>>
			(buf1, buf2, buf_out,
			sum<std::complex<double>, std::complex<double>, std::complex<double>>);
	}

	output_buffers[0].set_extremes(buf1.get_extremes() + buf2.get_extremes());
}

void OperatorSum::execute()
{
	if (input_connectors[0]->is_empty_buffer() || input_connectors[1]->is_empty_buffer())
		return; // Empty or copy -> nothing to do

	dispatch_calculate(*this);
}
