// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_POW_HPP
#define OPERATOR_POW_HPP

#include "operator.hpp"

class OperatorPowState final : public Operator::StateTemplate<OperatorPowState> {
	QJsonObject to_json() const override;
	void from_json(const QJsonObject &) override;
public:
	// Supported exponents:
	// -3 -> 1/3 (cube root)
	// -2 -> 1/2 (square root)
	//  2 -> 2 (square)
	//  3 -> 3 (cube)
	// -1 -> -1 (inverse)
	int exponent = -2;
};

class OperatorPow : public OperatorTemplate<OperatorId::Pow, OperatorPowState>
{
	void state_reset() override;

	size_t num_input() const override;
	size_t num_output() const override;
	bool input_connection_changed() override;
	void execute() override;

	MenuButton *menu;
	void set_exponent(int exponent);
	static QPixmap get_pixmap(int exponent, int size);
	static const char *get_tooltip(int exponent);
	static InitState make_init_state(int exponent);
public:
	inline static constexpr const char *icon = ":/icons/pow.svg";
	inline static constexpr const char *tooltip = "Add power function";
	static std::vector<InitState> get_init_states();

	using OperatorTemplate::OperatorTemplate;
	void init() override;
private:
	friend class Operator;
	template<size_t N> void calculate();
};

#endif
