// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_SPLIT_HPP
#define OPERATOR_SPLIT_HPP

#include "operator.hpp"

class OperatorSplit : public OperatorNoState<OperatorId::Split, 1, 2>
{
	bool input_connection_changed() override;
	void execute() override;
public:
	inline static constexpr const char *icon = ":/icons/split.svg";
	inline static constexpr const char *tooltip = "Add Split";

	using OperatorNoState::OperatorNoState;
	void init() override;
};

#endif
