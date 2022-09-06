// SPDX-License-Identifier: GPL-2.0
#include "scene.hpp"
#include "command.hpp"
#include "document.hpp"
#include "edge.hpp"
#include "magnifier.hpp"
#include "mainwindow.hpp"
#include "operator_adder.hpp"
#include "operator.hpp"

#include <QGraphicsSceneHoverEvent>
#include <QScrollBar>

#include <cassert>

Scene::Scene(MainWindow &w_, QObject *parent)
	: QGraphicsScene(parent)
	, w(w_)
	, mode(Mode::normal)
	, edge(nullptr)
	, magnifier(nullptr)
	, handle_drag(nullptr)
	, operator_move(nullptr)
{
}

Scene::~Scene()
{
	clear();
}

void Scene::clear()
{
	enter_normal_mode();
	selection.clear();
	emit selection_changed(selection.is_empty());
}

void Scene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (operator_adder) {
		QPointF pos = event->scenePos();
		operator_adder->move_to(pos, connector_at(pos));
	} else if (edge) {
		edge->calculate_add_edge(this, event->scenePos());
		// Fall through to standard mouseEvent, so that we highlight the
		// connector under the mouse
		QGraphicsScene::mouseMoveEvent(event);
	} else if (handle_drag) {
		handle_drag->drag(event->scenePos(), event->modifiers());
	} else if (operator_move) {
		operator_move->move_event(event->scenePos());
	} else if (magnifier) {
		magnifier->go(this, event->scenePos());
	} else {
		QGraphicsScene::mouseMoveEvent(event);
	}
}

void Scene::connector_clicked(Connector *conn)
{
	if (mode != Mode::normal)
		return;

	assert(conn);
	assert(!edge);

	mode = Mode::connect;
	edge = new Edge(conn, w.get_document());
	set_cursor(Qt::ClosedHandCursor);
	addItem(edge);
}

void Scene::enter_drag_mode(HandleInterface *handle)
{
	if (mode != Mode::normal)
		return;

	assert(handle);
	assert(!handle_drag);

	mode = Mode::drag;
	handle_drag = handle;
	set_cursor(Qt::ClosedHandCursor);
}

void Scene::enter_move_mode(Operator *op)
{
	if (mode != Mode::normal)
		return;

	assert(op);
	assert(!operator_move);

	mode = Mode::move;
	operator_move = op;
	set_cursor(Qt::ClosedHandCursor);
}

void Scene::selectable_clicked(Selectable *s, QGraphicsSceneMouseEvent *event)
{
	if (mode != Mode::normal && mode != Mode::drag)
		return;

	if (event->modifiers() & Qt::ShiftModifier)
		selection.select_add(s);
	else
		selection.select(s);
	emit selection_changed(selection.is_empty());
}

void Scene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (mode == Mode::add_object && event->button() == Qt::LeftButton) {
		assert(operator_adder);
		if (operator_adder->clicked())
			enter_normal_mode();
	} else if (mode == Mode::add_object && event->button() == Qt::RightButton) {
		assert(operator_adder);
		operator_adder->clear_edges();
	} else if (mode == Mode::magnify) {
		assert(magnifier);
		enter_normal_mode();
	} else if (itemAt(event->scenePos(), QTransform()) == nullptr) {
		if (!(event->modifiers() & Qt::ShiftModifier)) {
			selection.deselect_all();
			emit selection_changed(selection.is_empty());
		}
	} else {
		QGraphicsScene::mousePressEvent(event);
	}
}

void Scene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (edge) {
		place_edge();
	} else if (handle_drag) {
		assert(handle_drag);
		handle_drag->leave_drag_mode(true);
		handle_drag = nullptr;
		enter_normal_mode();
	} else if (operator_move) {
		assert(operator_move);
		operator_move->leave_move_mode(true);
		operator_move = nullptr;
		enter_normal_mode();
	}

	QGraphicsScene::mouseReleaseEvent(event);
}

void Scene::place_edge()
{
	if (mode != Mode::connect)
		return;
	assert(edge);
	// Save the edge to replace now, because edge->attempt_add() clears that edge.
	Edge *replace_edge = edge->get_and_clear_replace_edge();
	if (edge->attempt_add()) {
		// Remove old connection, if any
		auto replace_edge_connectors = replace_edge ?
			std::make_pair(replace_edge->get_connector_from(), replace_edge->get_connector_to()) :
			std::make_pair(nullptr, nullptr);
		auto connectors = std::make_pair(edge->get_connector_from(), edge->get_connector_to());
		Document &d = w.get_document();
		d.place_command<CommandPlaceEdge>(d, *this, connectors, replace_edge_connectors);
	}
	enter_normal_mode(); // This will delete the temporary edge
}

void Scene::delete_temporary_edge()
{
	// Connector path may not be set after it was placed on the scene
	if (!edge)
		return;
	edge->remove_temporary();
	delete edge;
	edge = nullptr;
}

Connector *Scene::connector_at(const QPointF &pos)
{
	for (QGraphicsItem *item: items(pos, Qt::IntersectsItemBoundingRect)) {
		switch (item->type()) {
		case Connector::Type:
			return qgraphicsitem_cast<Connector *>(item);
		case Operator::Type:
			// Ignore the operator that we are adding right now!
			const Operator *op = qgraphicsitem_cast<Operator *>(item);
			if (operator_adder && operator_adder->is_operator(op))
				continue;
			return op->nearest_connector(pos);
		}
	}
	return nullptr;
}

void Scene::enter_normal_mode()
{
	exit_mode();
	mode = Mode::normal;
	set_cursor(Qt::ArrowCursor);
}

void Scene::enter_add_object_mode(std::unique_ptr<Operator> &&op)
{
	exit_mode();
	mode = Mode::add_object;

	assert(!operator_adder);
	//addItem(&*op);
	operator_adder = std::make_unique<OperatorAdder>(w, std::move(op));
}

void Scene::enter_magnifier_mode()
{
	exit_mode();
	mode = Mode::magnify;

	assert(!magnifier);
	magnifier = new Magnifier(get_view()->parentWidget());
}

void Scene::delete_magnifier()
{
	if (!magnifier)
		return;
	delete magnifier;
	magnifier = nullptr;
}

void Scene::exit_mode()
{
	switch (mode) {
	case Mode::normal:
		break;
	case Mode::add_object:
		operator_adder.reset();
		break;
	case Mode::connect:
		delete_temporary_edge();
		break;
	case Mode::drag:
		// handle_drag was set to nullptr if the value was commited
		if (handle_drag) {
			handle_drag->leave_drag_mode(false);
			handle_drag = nullptr;
		}
		break;
	case Mode::move:
		// operator_move was set to nullptr if the value was commited
		if (operator_move) {
			operator_move->leave_move_mode(false);
			operator_move = nullptr;
		}
		break;
	case Mode::magnify:
		delete_magnifier();
		break;
	}
}

void Scene::delete_selection()
{
	enter_normal_mode();

	selection.remove_all(w.get_document(), *this);
	emit selection_changed(selection.is_empty());
}

QGraphicsView *Scene::get_view()
{
	return views().front();
}

const QGraphicsView *Scene::get_view() const
{
	return views().front();
}

void Scene::set_cursor(Qt::CursorShape shape)
{
	get_view()->viewport()->setCursor(shape);
}

QPoint Scene::get_scroll_position() const
{
	const QGraphicsView *view = get_view();
	return {
		view->horizontalScrollBar()->value(),
		view->verticalScrollBar()->value()
	};
}

void Scene::set_scroll_position(const QPoint &p) const
{
	const QGraphicsView *view = get_view();
	view->horizontalScrollBar()->setValue(p.x());
	view->verticalScrollBar()->setValue(p.y());
}
