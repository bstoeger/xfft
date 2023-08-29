// SPDX-License-Identifier: GPL-2.0
#include "connector.hpp"
#include "operator.hpp"
#include "scene.hpp"
#include "edge.hpp"

#include <QBrush>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>

#include <cassert>

static QBrush standard_brush(Qt::black);
static QBrush highlighted_brush(Qt::red);

Connector::Connector(size_t id_, bool output_, Operator *op)
	: QGraphicsRectItem(0.0, 0.0, width, height, op)
	, id(id_)
	, output(output_)
	, highlighted(false)
	, selected(false)
	, parent(nullptr)
{
	children.reserve(5);		// Heuristics
	setBrush(standard_brush);
	setAcceptHoverEvents(true);
	setAcceptTouchEvents(true);

	// Prioritize mouseclicks over those of edges
	setZValue(1.0);

	setCursor(Qt::CrossCursor);
}

ConnectorDesc Connector::connector_desc()
{
	return { op(), is_output()
			? ConnectorType::output_connector(id)
			: ConnectorType::input_connector(id) };
}

size_t Connector::get_id() const
{
	return id;
}

void Connector::set_highlighted(bool highlighted_)
{
	if (highlighted == highlighted_)
		return;
	highlighted = highlighted_;

	setBrush(highlighted || selected ? highlighted_brush : standard_brush);
}

void Connector::set_selected(bool selected_)
{
	if (selected == selected_)
		return;
	selected = selected_;

	setBrush(highlighted || selected ? highlighted_brush : standard_brush);
}

bool Connector::is_output() const
{
	return output;
}

Operator *Connector::op()
{
	return static_cast<Operator *>(parentItem());
}

bool Connector::is_empty_buffer() const
{
	if (!output && !parent)
		return true;
	return get_buffer().is_empty();
}

bool Connector::is_complex_buffer() const
{
	if (!output && !parent)
		return false;
	return get_buffer().is_complex();
}

const FFTBuf &Connector::get_buffer() const
{
	if (output) {
		return op()->get_output_buffer(id);
	} else {
		assert(parent);
		Connector *from = parent->get_connector_from();
		assert(from);
		return from->op()->get_output_buffer(from->id);
	}
}

// Waiting for C++23 deducing this
FFTBuf &Connector::get_buffer()
{
	if (output) {
		return op()->get_output_buffer(id);
	} else {
		assert(parent);
		Connector *from = parent->get_connector_from();
		assert(from);
		return from->op()->get_output_buffer(from->id);
	}
}

const Operator *Connector::op() const
{
	return static_cast<Operator *>(parentItem());
}

OperatorList::view_list &Connector::get_view_list()
{
	return view_list;
}

const OperatorList::view_list &Connector::get_view_list() const
{
	return view_list;
}

double Connector::y_dist(double y) const
{
	return fabs(pos().y() + height / 2.0 - y);
}

QPointF Connector::line_from() const
{
	return scenePos() + QPointF(width / 2.0, height / 2.0);
}

void Connector::set_safety_pos(const QPointF &p)
{
	safety_pos = p;
}

QPointF Connector::get_safety_pos() const
{
	return safety_pos;
}

void Connector::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
	set_highlighted(true);
}

void Connector::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
	set_highlighted(false);
}

void Connector::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (!(event->buttons() & Qt::LeftButton))
		return;
	static_cast<Scene *>(scene())->connector_clicked(this);
}

bool Connector::has_input_connection() const
{
	assert(!output);
	return !!parent;
}

void Connector::set_input_connection(Edge *c)
{
	assert(!output);
	assert(!parent);
	parent = c;
}

void Connector::remove_input_connection(Edge *c)
{
	assert(!output);
	assert(parent == c);
	parent = nullptr;
}

void Connector::add_output_connection(Edge *c)
{
	assert(output);
	children.push_back(c);
}

void Connector::remove_output_connection(Edge *c)
{
	assert(output);
	auto it = std::find(children.begin(), children.end(), c);
	assert(it != children.end());
	*it = children.back();
	children.pop_back();
}

const Connector *Connector::get_parent() const
{
	assert(!output);
	if (!parent)
		return nullptr;
	return parent->get_connector_from();
}

Edge *Connector::get_parent_edge()
{
	assert(!output);
	return parent;
}

std::vector<Connector *> Connector::get_children() const
{
	assert(output);
	size_t size = children.size();
	std::vector<Connector *> res(size);
	for (size_t i = 0; i < size; ++i)
		res[i] = children[i]->get_connector_to();
	return res;
}

const std::vector<Edge *> &Connector::get_children_edges() const
{
	assert(output);
	return children;
}

void Connector::remove_edges()
{
	if (parent) {
		parent->remove();
		assert(!parent);
	}

	// Note that we copy the children list because the edges remove
	// themselves from the vector during deletion!
	std::vector<Edge *> children_copy = children;
	for (Edge *e: children_copy)
		e->remove();
}

void Connector::recalculate_edges()
{
	if (parent)
		parent->recalculate();

	for (Edge *e: children)
		e->recalculate();
}

void Connector::out_edges_to_json(QJsonArray &out) const
{
	for (const Edge *e: children)
		out.push_back(e->to_json());
}
