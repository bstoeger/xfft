// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_CONVOLUTION_HPP
#define OPERATOR_CONVOLUTION_HPP

#include "operator.hpp"
#include "convolution_plan.hpp"

#include <memory>

class OperatorConvolution : public OperatorNoState<OperatorId::Convolution, 2, 1>
{
	bool input_connection_changed() override;
	void execute() override;
	std::unique_ptr<ConvolutionPlan> plan;
public:
	inline static constexpr const char *icon = ":/icons/convolution.svg";
	inline static constexpr const char *tooltip = "Add Convolution";

	using OperatorNoState::OperatorNoState;
	void init() override;
private:
	friend class Operator;
	template<size_t N> void calculate();
};

#endif
