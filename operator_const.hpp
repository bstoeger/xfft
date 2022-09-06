// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_CONST_HPP
#define OPERATOR_CONST_HPP

#include "operator.hpp"
#include "color.hpp"

#include <QImage>

enum class ColorType;

class OperatorConstState final : public Operator::StateTemplate<OperatorConstState> {
	QJsonObject to_json() const override;
	void from_json(const QJsonObject &) override;
public:
	ColorType color_type = ColorType::RW;
	std::complex<double> v = {1.0, 0.0};
	double scale = 1.0;
};

class OperatorConst : public OperatorTemplate<OperatorId::Const, OperatorConstState>
{
	static constexpr size_t size = 128;
	static constexpr double scale = 1.05;

	AlignedBuf<uint32_t> imagebuf;

	Handle *handle;
	QGraphicsTextItem *text;
	ColorType current_color_type;
	bool dont_accumulate_undo;

	size_t num_input() const override;
	size_t num_output() const override;
	bool input_connection_changed() override;
	void execute() override;
	void placed() override;
	void state_reset() override;

	void clear();

	void paint_image();

	void place_handle();
	void set(std::complex<double>);

	bool handle_click(QGraphicsSceneMouseEvent *event) override;
	void drag_handle(const QPointF &p, Qt::KeyboardModifiers) override;
	void restore_handles() override;

	// Switch between color modes
	MenuButton *mode_menu;
	void switch_mode(ColorType type);

	// Overall scale
	Scroller *scroller_scale;
	void set_scale(double scale);
	void set_scroller();

	// Paint
	void calculate();
public:
	inline static constexpr const char *icon = ":/icons/const.svg";
	inline static constexpr const char *tooltip = "Add Constant";

	OperatorConst(MainWindow &w);
	void init() override;
};

#endif
