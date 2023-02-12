// SPDX-License-Identifier: GPL-2.0
#include "operator_conjugate.hpp"
#include "document.hpp"

void OperatorConjugate::init()
{
	init_simple(icon);
}

bool OperatorConjugate::input_connection_changed()
{
	// Empty if the input buffer is empty.
	if (input_connectors[0]->is_empty_buffer())
		return make_output_empty(0);

	FFTBuf &buf = input_connectors[0]->get_buffer();

	// For real data, the complex conjugate is the identity function
	if (buf.is_complex())
		return make_output_complex(0);
	else
		return make_output_forwarded(0, input_connectors[0]->get_buffer());
}

template<size_t N>
void OperatorConjugate::calculate()
{
	FFTBuf &in_buf = input_connectors[0]->get_buffer();
	FFTBuf &out_buf = output_buffers[0];

	// Only called for complex buffers
	auto *in = in_buf.get_data<std::complex<double>>();
	auto *out = out_buf.get_data<std::complex<double>>();
	for (size_t i = 0; i < N*N; ++i)
		*out++ = std::conj(*in++);

	out_buf.set_extremes(in_buf.get_extremes());
}

void OperatorConjugate::execute()
{
	if (input_connectors[0]->is_empty_buffer() ||
	    !input_connectors[0]->get_buffer().is_complex())
		return; // Empty or real -> nothing to do

	dispatch_calculate(*this);
}
