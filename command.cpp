// SPDX-License-Identifier: GPL-2.0
#include "document.hpp"
#include "command.hpp"
#include "connector.hpp"
#include "edge.hpp"
#include "operator.hpp"
#include "scene.hpp"

#include <QMessageBox>

CommandPlace::CommandPlace(Document &document_, Scene &scene_, std::unique_ptr<Operator> op_,
			   std::vector<std::pair<Connector *, Connector *>> edges_to_add_,
			   std::vector<std::pair<Connector *, Connector *>> edges_to_remove_)
	: document(document_)
	, scene(scene_)
	, op_to_add(std::move(op_))
	, edges_to_add(std::move(edges_to_add_))
	, edges_to_remove(std::move(edges_to_remove_))
{
	setText("Add operator");
}

static void place_edge(const std::pair<Connector *, Connector *> &edge, Document &d, Scene &scene)
{
	auto [conn_from, conn_to] = edge;
	if (!conn_from || !conn_to)
		return;
	auto e = std::make_unique<Edge>(conn_from, conn_to, d);
	scene.addItem(&*e);
	e->recalculate();
	e->add_connection();
	e.release();
}

static void place_edges(const std::vector<std::pair<Connector *, Connector *>> &edges, Document &d, Scene &scene)
{
	for (auto const &e: edges)
		place_edge(e, d, scene);
}

static void remove_edge(const std::pair<Connector *, Connector *> &edge)
{
	auto [conn_from, conn_to] = edge;
	if (!conn_from || !conn_to)
		return;
	Edge *e = conn_to->get_parent_edge();
	if (!e) {
		QMessageBox::warning(nullptr, "Error", "Trying to remove inexistent edge");
		return;
	}
	e->remove();
}

static void remove_edges(const std::vector<std::pair<Connector *, Connector *>> &edges)
{
	// We use the fact that input connectors can have only one edge.
	// This will have to be made smarter if we support input connectors with multiple edges.
	for (auto const &e: edges)
		remove_edge(e);
}

static Operator *add_operator(std::unique_ptr<Operator> &op)
{
	op->add_to_scene();
	op->enter_placed_mode();
	return op.release();
}

static std::unique_ptr<Operator> remove_operator(Operator *&op)
{
	op->remove_placed_from_scene();
	return std::unique_ptr<Operator>(std::exchange(op, nullptr));
}

void CommandPlace::redo()
{
	op_to_remove = add_operator(op_to_add);

	// Note: it is crucial to remove edges before adding edges, because adding edges may induce edge removal.
	remove_edges(edges_to_remove);
	place_edges(edges_to_add, document, scene);
	std::swap(edges_to_add, edges_to_remove);
}

void CommandPlace::undo()
{
	// Note: it is crucial to remove edges before adding edges, because adding edges may induce edge removal.
	remove_edges(edges_to_remove);
	place_edges(edges_to_add, document, scene);
	std::swap(edges_to_add, edges_to_remove);
	op_to_add = remove_operator(op_to_remove);
}

CommandPlaceEdge::CommandPlaceEdge(Document &document_, Scene &scene_,
				   std::pair<Connector *, Connector *> edge_to_add_,
				   std::pair<Connector *, Connector *> edge_to_remove_)
	: document(document_)
	, scene(scene_)
	, edge_to_add(edge_to_add_)
	, edge_to_remove(edge_to_remove_)
{
	setText("Add edge");
}

void CommandPlaceEdge::redo()
{
	// Note: it is crucial to remove edges before adding edges, because adding edges may induce edge removal.
	remove_edge(edge_to_remove);
	place_edge(edge_to_add, document, scene);
	std::swap(edge_to_add, edge_to_remove);
}

void CommandPlaceEdge::undo()
{
	redo(); // Undo and redo do the same thing
}

CommandRemoveObjects::CommandRemoveObjects(Document &document_, Scene &scene_,
					   std::vector<Operator *> ops_to_remove_,
					   std::vector<Edge *> edges_to_remove)
	: document(document_)
	, scene(scene_)
	, ops_to_remove(ops_to_remove_)
{
	if (ops_to_remove.size() > 1)
		setText(QStringLiteral("Delete %1 operators").arg(ops_to_remove.size()));
	else if (!ops_to_remove.empty())
		setText("Delete operator");
	else if (edges_to_remove.size() > 1)
		setText(QStringLiteral("Delete %1 edges").arg(edges_to_remove.size()));
	else
		setText("Delete edge");

	// Collect all edges to the operators to be removed, because these must likewise be removed
	for (Operator *o: ops_to_remove) {
		for (Edge *e: o->get_edges()) {
			if (std::find(edges_to_remove.begin(), edges_to_remove.end(), e) == edges_to_remove.end())
				edges_to_remove.push_back(e);
		}
	}
	edges_to_add_or_remove.reserve(edges_to_remove.size());
	for (Edge *e: edges_to_remove)
		edges_to_add_or_remove.emplace_back(e->get_connector_from(), e->get_connector_to());
}

void CommandRemoveObjects::redo()
{
	remove_edges(edges_to_add_or_remove);
	ops_to_add.reserve(ops_to_remove.size());
	for (auto &op: ops_to_remove)
		ops_to_add.push_back(remove_operator(op));
	ops_to_remove.clear();
}

void CommandRemoveObjects::undo()
{
	for (auto &op: ops_to_add)
		ops_to_remove.push_back(add_operator(op));
	ops_to_add.clear();
	place_edges(edges_to_add_or_remove, document, scene);
}

CommandSetState::CommandSetState(Document &document_, Scene &scene_,
				 const QString &text, Operator *op_,
				 std::unique_ptr<Operator::State> state_,
				 bool merge_)
	: document(document_)
	, scene(scene_)
	, op(op_)
	, merge(merge_)
	, state(std::move(state_))
{
	setText(text);
}

void CommandSetState::redo()
{
	op->swap_state(*state);
	op->state_reset();
}

void CommandSetState::undo()
{
	redo(); // Undo and redo do the same thing
}

int CommandSetState::id() const
{
	return 4711;
}

bool CommandSetState::mergeWith(const QUndoCommand *cmd_)
{
	// To merge state setting commands, we essentially don't have to do anything.
	// The old command has been executed and thus the operator is in the new state.
	// The state to be redone remains unchanged. Thus, only check whether the merge
	// should be done.
	const CommandSetState *cmd = dynamic_cast<const CommandSetState *>(cmd_);
	return cmd && cmd->op == op && cmd->merge;
}

CommandMove::CommandMove(Document &document_, Scene &scene_,
			 Operator *op_, QPointF old_pos_, QPointF new_pos_)
	: document(document_)
	, scene(scene_)
	, op(op_)
	, old_pos(old_pos_)
	, new_pos(new_pos_)
{
	setText("Move operator");
}

void CommandMove::redo()
{
	op->move_to(new_pos);
	std::swap(new_pos, old_pos);
}

void CommandMove::undo()
{
	redo(); // Undo and redo do the same thing
}

