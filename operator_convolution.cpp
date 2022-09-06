// SPDX-License-Identifier: GPL-2.0
#include "operator_convolution.hpp"
#include "convolution_plan_impl.hpp"
#include "document.hpp"

void OperatorConvolution::init()
{
	init_simple(icon);
}

size_t OperatorConvolution::num_input() const
{
	return 2;
}

size_t OperatorConvolution::num_output() const
{
	return 1;
}

bool OperatorConvolution::input_connection_changed()
{
	if (input_connectors[0]->is_empty_buffer() || input_connectors[1]->is_empty_buffer()) {
		plan.reset();
		return make_output_empty(0);
	}

	FFTBuf &new_buf1 = input_connectors[0]->get_buffer();
	FFTBuf &new_buf2 = input_connectors[1]->get_buffer();

	bool updated_output = new_buf1.is_complex() || new_buf2.is_complex() ?
		make_output_complex(0) : make_output_real(0);

	plan = std::make_unique<ConvolutionPlan>(new_buf1, new_buf2, output_buffers[0]);

	return updated_output;
}

template<size_t N>
void OperatorConvolution::calculate()
{
	plan->execute<N>();
}

void OperatorConvolution::execute()
{
	if (!plan)
		return;
	dispatch_calculate(*this);
}
