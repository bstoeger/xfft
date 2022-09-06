// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_MULT_HPP
#define OPERATOR_MULT_HPP

#include "operator.hpp"

class OperatorMult : public OperatorNoState<OperatorId::Mult>
{
	size_t num_input() const override;
	size_t num_output() const override;
	bool input_connection_changed() override;
	void execute() override;
public:
	inline static constexpr const char *icon = ":/icons/mult.svg";
	inline static constexpr const char *tooltip = "Add Multiplication";

	using OperatorNoState::OperatorNoState;
	void init() override;
private:
	friend class Operator;
	template<size_t N> void calculate();
};

#endif
