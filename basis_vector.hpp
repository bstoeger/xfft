// SPDX-License-Identifier: GPL-2.0
// This is a very simple QGraphicsItem, which draws the representation
// of a basis vector.

#ifndef BASIS_VECTOR_HPP
#define BASIS_VECTOR_HPP

#include <QGraphicsItem>

class BasisVector : public QGraphicsItem {
	QPointF origin;
	QPointF v;
	QPointF arrow_head1;
	QPointF arrow_head2;

	void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
	QRectF boundingRect() const;
	static constexpr double arrow_head_width = 10.0;
public:
	BasisVector(const QPointF &origin, QGraphicsItem *parent);
	void set(const QPointF &v);
};

#endif
