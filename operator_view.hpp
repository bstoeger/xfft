// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_VIEW_HPP
#define OPERATOR_VIEW_HPP

#include "operator.hpp"
#include "aligned_buf.hpp"
#include "color.hpp"

#include <QImage>

class OperatorViewState final : public Operator::StateTemplate<OperatorViewState> {
	QJsonObject to_json() const override;
	void from_json(const QJsonObject &) override;
public:
	OperatorViewState();
	ColorMode mode = ColorMode::LINEAR;
	double scale = 1.0;
	ColorType color_type = ColorType::RW;
	QString directory;
};

class OperatorView : public OperatorTemplate<OperatorId::View, OperatorViewState>
{
	AlignedBuf<uint32_t> imagebuf;

	QString get_scale_text() const;

	size_t num_input() const override;
	size_t num_output() const override;
	bool input_connection_changed() override;
	void show_empty();
	void execute() override;
	void init() override;
	void state_reset() override;
	void restore_handles() override;

	void set_scale(double scale);

	// Save as png file
	void save_file();

	// Display zoom level
	QGraphicsTextItem *text;

	// Switch between color modes
	MenuButton *color_menu;
	MenuButton *mode_menu;
	void switch_color(ColorType type);
	void switch_mode(ColorMode mode);

	Scroller *scroller;

	bool dont_accumulate_undo;

	template<size_t N, typename T> void calculate_doit();
public:
	inline static constexpr const char *icon = ":/icons/view.svg";
	inline static constexpr const char *tooltip = "Add View";

	using OperatorTemplate::OperatorTemplate;
private:
	friend class Operator;
	template<size_t N> void calculate();
};

#endif
