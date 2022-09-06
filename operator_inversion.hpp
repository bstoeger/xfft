// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_INVERSION_HPP
#define OPERATOR_INVERSION_HPP

#include "operator.hpp"

enum class OperatorInversionType {
	inversion,
	rot_4_plus,
	rot_4_minus,
	m_x,
	m_y,
	m_xy,
	m_minus_xy
};

class OperatorInversionState final : public Operator::StateTemplate<OperatorInversionState> {
	QJsonObject to_json() const override;
	void from_json(const QJsonObject &) override;
public:
	OperatorInversionType type = OperatorInversionType::inversion;
};

class OperatorInversion : public OperatorTemplate<OperatorId::Inversion, OperatorInversionState>
{
	static InitState make_init_state(OperatorInversionType type);
	void state_reset() override;

	size_t num_input() const override;
	size_t num_output() const override;
	bool input_connection_changed() override;
	void execute() override;

	MenuButton *menu;
	void set_type(OperatorInversionType type);
	static const char *get_pixmap_name(OperatorInversionType type);
	static QPixmap get_pixmap(OperatorInversionType type, int size);
	static const char *get_tooltip(OperatorInversionType type);
public:
	inline static constexpr const char *icon = ":/icons/inversion.svg";
	inline static constexpr const char *tooltip = "Add Operation";
	static std::vector<InitState> get_init_states();

	using OperatorTemplate::OperatorTemplate;
	void init() override;
private:
	friend class Operator;
	template<size_t N> void calculate();
	template <typename T, size_t N> void transform(FFTBuf &in_buf, FFTBuf &out_buf);
};

#endif
