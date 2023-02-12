// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_MERGE_HPP
#define OPERATOR_MERGE_HPP

#include "operator.hpp"

class OperatorMerge : public OperatorNoState<OperatorId::Merge, 2, 1>
{
	bool input_connection_changed() override;
	void execute() override;
public:
	inline static constexpr const char *icon = ":/icons/merge.svg";
	inline static constexpr const char *tooltip = "Add Merge";

	using OperatorNoState::OperatorNoState;
	void init() override;
private:
	friend class Operator;
	template<size_t N> void calculate();
};

#endif
