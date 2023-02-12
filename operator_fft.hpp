// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_FFT_HPP
#define OPERATOR_FFT_HPP

#include "operator.hpp"
#include "fft_plan.hpp"

#include <memory>

enum class OperatorFFTType {
	FWD,
	INV,
	NORM,
};

class OperatorFFTState final : public Operator::StateTemplate<OperatorFFTState> {
	QJsonObject to_json() const override;
	void from_json(const QJsonObject &) override;
public:
	OperatorFFTType type = OperatorFFTType::FWD;
};

class OperatorFFT : public OperatorTemplate<OperatorId::FFT, OperatorFFTState, 1, 1>
{
	static InitState make_init_state(OperatorFFTType type);
	bool input_connection_changed() override;
	void set_type(OperatorFFTType type_);
	void execute() override;
	void init() override;
	void state_reset() override;
	static QPixmap get_pixmap(OperatorFFTType type, int size);
	bool update_plan();

	MenuButton *menu;
	std::unique_ptr<FFTPlan> plan;

	friend class Operator;
	template<size_t N> void calculate();
public:
	using OperatorTemplate::OperatorTemplate;
	inline static constexpr const char *icon = ":/icons/fft.svg";
	inline static constexpr const char *tooltip = "Add FT";
	static std::vector<Operator::InitState> get_init_states();
};

#endif
