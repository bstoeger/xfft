// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_SUM_HPP
#define OPERATOR_SUM_HPP

#include "operator.hpp"

class OperatorSum : public OperatorNoState<OperatorId::Sum>
{
	size_t num_input() const override;
	size_t num_output() const override;
	bool input_connection_changed() override;
	void execute() override;
	void init() override;
public:
	inline static constexpr const char *icon = ":/icons/sum.svg";
	inline static constexpr const char *tooltip = "Add Sum";

	using OperatorNoState::OperatorNoState;
private:
	friend class Operator;
	template<size_t N> void calculate();
};

#endif
