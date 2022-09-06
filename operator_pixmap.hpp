// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_PIXMAP_HPP
#define OPERATOR_PIXMAP_HPP

#include "operator.hpp"

#include <QImage>
#include <QString>
#include <QPoint>
#include <QPainter>

class OperatorPixmapState final : public Operator::StateTemplate<OperatorPixmapState> {
	QJsonObject to_json() const override;
	void from_json(const QJsonObject &) override;
	size_t n; // Set once at initialization
public:
	void init(size_t n);
	QImage image;
	QString directory;
	int brush_size = 1;
	bool antialiasing = false;
};

class OperatorPixmap : public OperatorTemplate<OperatorId::Pixmap, OperatorPixmapState>
{
	size_t num_input() const override;
	size_t num_output() const override;
	bool input_connection_changed() override;
	void execute() override;

	void load_file();
	void clear();
	void invert();
	void update_buffers();

	void init() override;
	void placed() override;
	void state_reset() override;

	bool handle_click(QGraphicsSceneMouseEvent *event) override;
	void drag_handle(const QPointF &p, Qt::KeyboardModifiers) override;
	void restore_handles() override; // Tells us that we exited from drag mode

	QPainter painter;
	QPen pen;
	QPoint pos;
	bool dont_accumulate_undo;

	// Switch between pens
	MenuButton *brush_menu;
	void switch_brush(int size, bool antialiasing);
public:
	inline static constexpr const char *icon = ":/icons/pixmap.svg";
	inline static constexpr const char *tooltip = "Add Pixmap";

	OperatorPixmap(MainWindow &w);
private:
	friend class Operator;
	template<size_t N> void calculate();
};

#endif
