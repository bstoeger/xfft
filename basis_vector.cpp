// SPDX-License-Identifier: GPL-2.0
#include "basis_vector.hpp"

#include <QPainter>
#include <cmath>

BasisVector::BasisVector(const QPointF &origin_, QGraphicsItem *parent)
	: QGraphicsItem(parent)
	, origin(origin_)
{
}

void BasisVector::set(const QPointF &v_)
{
	v = v_;

	double angle = v.x() == 0 && v.y() == 0 ? 0.0 : atan2(v.y(), v.x());

	double x = -arrow_head_width;
	double y = arrow_head_width / 2.0;
	arrow_head1 = QPointF(x * cos(angle) - y * sin(angle), x * sin(angle) + y * cos(angle)) + v;

	x = -arrow_head_width;
	y = -arrow_head_width / 2.0;
	arrow_head2 = QPointF(x * cos(angle) - y * sin(angle), x * sin(angle) + y * cos(angle)) + v;
}

void BasisVector::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
	painter->setPen(Qt::red);
	painter->drawLine(QLineF(origin, origin + v));

	painter->drawLine(QLineF(origin + v, origin + arrow_head1));
	painter->drawLine(QLineF(origin + v, origin + arrow_head2));
}

static void extend_rect(QPointF &top_left, QPointF &bottom_right, const QPointF &p)
{
	if (p.x() < top_left.x())
		top_left.setX(p.x());
	if (p.x() > bottom_right.x())
		bottom_right.setX(p.x());
	if (p.y() < top_left.y())
		top_left.setY(p.y());
	if (p.y() > bottom_right.y())
		bottom_right.setY(p.y());
}

QRectF BasisVector::boundingRect() const
{
	QPointF top_left = origin;
	QPointF bottom_right = origin;
	extend_rect(top_left, bottom_right, origin + v);
	extend_rect(top_left, bottom_right, origin + arrow_head1);
	extend_rect(top_left, bottom_right, origin + arrow_head2);
	return QRectF(top_left, bottom_right);
}
