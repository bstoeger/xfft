// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_LATTICE_HPP
#define OPERATOR_LATTICE_HPP

#include "operator.hpp"

#include <QImage>

class BasisVector;

class OperatorLatticeState final : public Operator::StateTemplate<OperatorLatticeState> {
	QJsonObject to_json() const override;
	void from_json(const QJsonObject &) override;
public:
	size_t d = 2;		// Dimensions: 0, 1 or 2
	QPoint p1 { 10, 0 };	// Defined if d is 1 or 2
	QPoint p2 { 0, 10 };	// Defined if d is 2
};

class OperatorLattice : public OperatorTemplate<OperatorId::Lattice, OperatorLatticeState, 0, 1>
{
	QImage image;

	class Handle : public Operator::Handle {
		void mousePressEvent(QGraphicsSceneMouseEvent *);
	public:
		bool second_axis;
		Handle(bool second_axis, const char *tooltip, Operator *parent);
	};

	Handle *handle1;
	Handle *handle2;
	bool second_axis;
	QPointF clicked_pos;
	QPointF clicked_old_pos;
	bool dont_accumulate_undo;
	bool handles_visible;

	BasisVector *basis1;
	BasisVector *basis2;

	// Update FFT-buffer and children
	void update_buffer();
	void placed() override;
	void state_reset() override;

	void set_d(size_t d);
	void clear();

	void paint_lattice();
	void paint_basis();
	void paint_0d();
	void paint_row_quadrant(QPoint p);
	void paint_1d(QPoint p);
	void paint2d(int step_x, int step_y, int spacing_x);
	void paint_2d(QPoint p1, QPoint p2);

	void place_handles();
	void hide_handles();
	void show_handles();
	void restore_handles() override;
	void drag_handle(const QPointF &, Qt::KeyboardModifiers) override;
public:
	inline static constexpr const char *icon = ":/icons/lattice.svg";
	inline static constexpr const char *tooltip = "Add Lattice";

	using OperatorTemplate::OperatorTemplate;

	void clicked_handle(QGraphicsSceneMouseEvent *, bool second_axis);
	void init() override;
};

#endif
