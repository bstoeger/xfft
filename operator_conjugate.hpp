// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_CONJUGATE_HPP
#define OPERATOR_CONJUGATE_HPP

#include "operator.hpp"

class OperatorConjugate : public OperatorNoState<OperatorId::Conjugate, 1, 1>
{
	bool input_connection_changed() override;
	void execute() override;
public:
	inline static constexpr const char *icon = ":/icons/conjugate.svg";
	inline static constexpr const char *tooltip = "Add Conjugate";

	using OperatorNoState::OperatorNoState;
	void init() override;
private:
	friend class Operator;
	template<size_t N> void calculate();
};

#endif
