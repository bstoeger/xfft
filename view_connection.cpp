// SPDX-License-Identifier: GPL-2.0
#include "view_connection.hpp"
#include "operator.hpp"
#include "scene.hpp"
#include "globals.hpp"

#include <cassert>

#include <QPen>

static const QPen corner_pen = QPen(Qt::red);
static const QPen connector_pen = QPen(Qt::green);
static const QPen same_pen = QPen(Qt::blue);

// TODO: duplicate code
static double euclidean_dist(const QPointF &p1, const QPointF &p2)
{
	QPointF diff = p2 - p1;
	return sqrt(diff.x()*diff.x() + diff.y()*diff.y());
}

ViewConnection::Vertex::Vertex(const ConnectorDesc &desc_, const QPointF &pos_)
	: desc(desc_)
	, pos(pos_)
{
}

ViewConnection::ViewConnection(const ConnectorDesc &from, const QPointF &pos_from,
			       const ConnectorDesc &to, const QPointF &pos_to, Scene &scene)
	: left(from, pos_from)
	, right(to, pos_to)
{
	if (right.pos.x() < left.pos.x() || (right.pos.x() == left.pos.x() && right.pos.y() < left.pos.x()))
		std::swap(left, right);

	dist = euclidean_dist(pos_from, pos_to);

	// For debugging purposes draw lines
	if (Globals::debug_mode) {
		line = std::make_unique<QGraphicsLineItem>(left.pos.x(), left.pos.y(), right.pos.x(), right.pos.y(), nullptr);
		const QPen &pen = from.op == to.op ? same_pen : (
			from.type.is_corner() && to.type.is_corner() ? corner_pen : connector_pen
		);
		line->setPen(pen);
		line->setEnabled(false);	// Don't get any mousePressEvents, etc
		line->setZValue(-1.0);		// Draw behind everything
		scene.addItem(&*line);
	}
}

ViewConnection::~ViewConnection()
{
	left.desc.op->remove_view_connection(left.desc.type, this);
	right.desc.op->remove_view_connection(right.desc.type, this);
}

bool ViewConnection::cuts_rect(const QRectF &rect) const
{
	double rleft = rect.left();
	double rright = rect.right();
	double rtop = rect.top();
	double rbottom = rect.bottom();
	if (right.pos.x() < rleft)
		return false;
	if (left.pos.x() > rright)
		return false;
	if (left.pos.y() < rtop && right.pos.y() < rtop)
		return false;
	if (left.pos.y() > rbottom && right.pos.y() > rbottom)
		return false;
	if (rect.contains(left.pos) || rect.contains(right.pos))
		return true;
	double delta_x = right.pos.x() - left.pos.x();
	double delta_y = right.pos.y() - left.pos.y();
	assert(delta_x >= 0.0);
	if (delta_x > 0.01) {
		double a = delta_y / delta_x;
		double b = left.pos.y() - a * left.pos.x();
		double y_pos = a*rect.left() + b;
		if (y_pos > rect.top() && y_pos < rect.bottom())
			return true;
		y_pos = a*rect.right() + b;
		if (y_pos > rect.top() && y_pos < rect.bottom())
			return true;
	}
	if (fabs(delta_y) > 0.01) {
		double a = delta_x / delta_y;
		double b = left.pos.x() - a * left.pos.y();
		double x_pos = a*rect.top() + b;
		if (x_pos > rect.left() && x_pos < rect.right())
			return true;
		x_pos = a*rect.bottom() + b;
		if (x_pos > rect.left() && x_pos < rect.right())
			return true;
	}
	return false;
}

double ViewConnection::get_dist() const
{
	return dist;
}

ConnectorDesc ViewConnection::get_other(const ConnectorDesc &desc, QPointF &pos)
{
	if (left.desc == desc) {
		pos = right.pos;
		return right.desc;
	} else {
		pos = left.pos;
		return left.desc;
	}
}

ConnectorDesc ViewConnection::get_other(const ConnectorDesc &desc)
{
	return left.desc == desc ? right.desc : left.desc;
}

bool ViewConnection::used_by_edge() const
{
	return !edges.empty();
}

const std::vector<Edge *> &ViewConnection::get_edges() const
{
	return edges;
}

void ViewConnection::collect_edges(std::vector<Edge *> &v) const
{
	std::copy_if(edges.begin(), edges.end(), std::back_inserter(v),
		     [&v](Edge *e) { return std::find(v.begin(), v.end(), e) == v.end(); });
}

void ViewConnection::add_edge(Edge *e)
{
	//assert(std::find(edges.begin(), edges.end(), e) == edges.end());
	edges.push_back(e);
}

void ViewConnection::remove_edge(Edge *e)
{
	auto it = std::find(edges.begin(), edges.end(), e);
	assert(it != edges.end());
	*it = edges.back();
	edges.pop_back();
}
