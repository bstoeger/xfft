// SPDX-License-Identifier: GPL-2.0
#include "operator_adder.hpp"
#include "operator.hpp"
#include "command.hpp"
#include "document.hpp"
#include "edge.hpp"
#include "mainwindow.hpp"
#include "scene.hpp"

#include <cassert>

OperatorAdder::OperatorAdder(MainWindow &w_, std::unique_ptr<Operator> op_)
	: w(w_)
	, op(std::move(op_))
	, prohibited(false)
	, connector_under_mouse(nullptr)
	, input_edges(op->num_input())
	, output_edges(op->num_output())
{
	op->prepare_init();
	op->init();
	op->finish_init();
	op->add_to_scene();
	op->placed(); // Call virtual function so that derived objects can do some initialization with connectors
	op->setOpacity(0.25);

	safety_rect = std::make_unique<QGraphicsRectItem>(op->get_double_safety_rect(), op.get());
	safety_rect->setBrush(Qt::red);
	safety_rect->setPen(Qt::NoPen);
	safety_rect->setZValue(-1.0);
	safety_rect->setOpacity(0.5);
}

OperatorAdder::~OperatorAdder()
{
	unwarn_cycles();
	if (op)
		op->remove_unplaced_from_scene();
}

OperatorAdder::EdgeList::EdgeList(size_t num)
	: edges(num)
	, count(0)
{
}

OperatorAdder::EdgeList::~EdgeList()
{
	clear();
}

void OperatorAdder::EdgeList::clear()
{
	for (std::unique_ptr<Edge> &e: edges) {
		if (e)
			e->remove_temporary();
		e.reset();
	}
}

// Special case for input edges:
// If user tries to connect twice to the same input connector, move the old edge,
// because an input connector can only be connected to one edge.
// Returns true if edge was moved
bool OperatorAdder::EdgeList::add_input(Connector *conn)
{
	auto it = std::find_if(edges.begin(), edges.end(),
			       [conn](const std::unique_ptr<Edge> &e) {
				return e && e->get_connector_from() == conn;
			       });
	// Did try to connect to same connector
	if (it == edges.end())
		return false;

	// Replace edge with itself -> do nothing
	if (&*it == &edges[count])
		return true;

	edges[count] = std::move(*it);
	return true;
}

void OperatorAdder::EdgeList::add(MainWindow &w, Connector *conn, bool output)
{
	if (edges.empty())
		return;

	if (!output || !add_input(conn)) {
		// If we replace an old edge, unwarn it first
		if (edges[count])
			edges[count]->remove_temporary();

		edges[count] = std::make_unique<Edge>(conn, w.get_document());
		w.get_scene().addItem(edges[count].get());
	}

	++count;
	if (count >= edges.size())
		count = 0;
}

void OperatorAdder::EdgeList::move_to(const QPointF &pos_, const QRectF &bounding_rect, bool output)
{
	if (edges.empty())
		return;

	QPointF pos = pos_;
	if (output)
		pos.rx() += bounding_rect.width();

	double step_y =  bounding_rect.height() / static_cast<double>(edges.size() + 1);
	pos.ry() += step_y;

	for (std::unique_ptr<Edge> &e: edges) {
		if (e)
			e->calculate(pos);
		pos.ry() += step_y;
	}
}

std::vector<std::unique_ptr<Edge>> &OperatorAdder::EdgeList::get_edges()
{
	return edges;
}

void OperatorAdder::EdgeList::to_connectors(std::vector<std::pair<Connector *, Connector *>> &res, Operator &op, bool output) const
{
	int connector_id = 0;
	for (auto &e: edges) {
		if (e) {
			if (output)
				res.emplace_back(&op.get_output_connector(connector_id), e->get_connector_from());
			else
				res.emplace_back(e->get_connector_from(), &op.get_input_connector(connector_id));
			++connector_id;
		}
	}
}

std::vector<std::pair<Connector *, Connector *>> OperatorAdder::EdgeList::replace_edges_to_connectors()
{
	std::vector<std::pair<Connector *, Connector *>> res;
	for (auto &e: edges) {
		if (!e)
			continue;
		if (Edge *e2 = e->get_and_clear_replace_edge())
			res.emplace_back(e2->get_connector_from(), e2->get_connector_to());
	}
	return res;
}

bool OperatorAdder::warn_cycles()
{
	// All cycles are recalculated, therefore at first clear the old cycle list.
	unwarn_cycles();

	bool found_cycle = false;
	for (auto &edge_in: input_edges.get_edges()) {
		if (!edge_in)
			continue;
		Operator *op_from = edge_in->get_operator_from();
		for (auto &edge_out: output_edges.get_edges()) {
			if (!edge_out)
				continue;
			Operator *op_to = edge_out->get_operator_from();

			EdgeCycle cycle;
			if (op_from == op_to) {
				// Special case: direct connection
				cycle.push_back(&*edge_in);
				cycle.push_back(&*edge_out);
			} else {
				cycle = w.get_document().topo.find_connection(op_to, op_from);
				if (cycle.empty())
					continue;
				cycle.push_back(&*edge_in);
				cycle.push_back(&*edge_out);
			}

			if (!cycle.empty()) {
				found_cycle = true;
				cycle.warn();
				cycles.push_back(std::move(cycle));
			}
		}
	}

	return found_cycle;
}

void OperatorAdder::unwarn_cycles()
{
	for (EdgeCycle &cycle: cycles)
		cycle.unwarn();
	cycles.clear();
}

void OperatorAdder::add_connector_edge(Connector *conn)
{
	assert(conn);
	unwarn_cycles();
	if (conn->is_output())
		input_edges.add(w, conn, false);
	else
		output_edges.add(w, conn, true);
	warn_cycles();
}

void OperatorAdder::move_to(const QPointF &pos, Connector *connector_under_mouse_)
{
	if (!op->isVisible())
		op->setVisible(true);

	op->setPos(pos);
	op->update_safety_rect();

	Connector *old_connector_under_mouse = std::exchange(connector_under_mouse, connector_under_mouse_);
	if (old_connector_under_mouse != connector_under_mouse) {
		if (old_connector_under_mouse)
			old_connector_under_mouse->set_highlighted(false);
		if (connector_under_mouse) {
			connector_under_mouse->set_highlighted(true);
			w.get_scene().set_cursor(Qt::CrossCursor);
		} else {
			w.get_scene().set_cursor(Qt::ArrowCursor);
		}
	}
	if (!connector_under_mouse) {
		bool new_prohibited = w.get_document().operator_list.operator_in_rect(op->get_safety_rect());
		bool old_prohibited = std::exchange(prohibited, new_prohibited);
		if (old_prohibited != prohibited)
			w.get_scene().set_cursor(prohibited ? Qt::ForbiddenCursor : Qt::ArrowCursor);
	}

	QRectF bounding_rect = op->boundingRect();
	input_edges.move_to(pos, bounding_rect, false);
	output_edges.move_to(pos, bounding_rect, true);
}

bool OperatorAdder::clicked()
{
	if (connector_under_mouse) {
		add_connector_edge(connector_under_mouse);
	} else if (op->isVisible() && !prohibited && cycles.empty()) {
		op->setOpacity(1.0);

		// The undo command will add the operator to the scene ant topo. Therefore remove it for now.
		w.get_document().topo.remove_operator(op.get());
		w.get_scene().removeItem(op.get());

		std::vector<std::pair<Connector *, Connector *>> edges;
		edges.reserve(op->num_input() + op->num_output());
		input_edges.to_connectors(edges, *op, false);
		output_edges.to_connectors(edges, *op, true);
		auto edges_to_replace = output_edges.replace_edges_to_connectors();
		input_edges.clear();
		output_edges.clear();
		Document &d = w.get_document();
		d.place_command<CommandPlace>(d, w.get_scene(), std::move(op), edges, edges_to_replace);
		return true;
	}
	return false;
}

void OperatorAdder::clear_edges()
{
	unwarn_cycles();
	input_edges.clear();
	output_edges.clear();
}

bool OperatorAdder::is_operator(const Operator *op_) const
{
	return &*op == op_;
}
