// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_MODULATE_HPP
#define OPERATOR_MODULATE_HPP

#include "operator.hpp"

class OperatorModulate : public OperatorNoState<OperatorId::Modulate>
{
	size_t num_input() const override;
	size_t num_output() const override;
	bool input_connection_changed() override;
	void execute() override;
public:
	inline static constexpr const char *icon = ":/icons/modulate.svg";
	inline static constexpr const char *tooltip = "Add Modulate";

	using OperatorNoState::OperatorNoState;
	void init() override;
private:
	friend class Operator;
	template<size_t N> void calculate();
};

#endif
