// SPDX-License-Identifier: GPL-2.0
// Undoable commands, based on Qt's QUndoCommand

#ifndef COMMAND_HPP
#define COMMAND_HPP

#include "operator.hpp" // for Operator::State

#include <vector>
#include <memory>

#include <QUndoCommand>

class Document;
class Scene;
class Connector;

// Place an operator
class CommandPlace : public QUndoCommand {
	Document &document;
	Scene &scene;

	// For redo():
	std::unique_ptr<Operator> op_to_add;

	// For undo():
	Operator *op_to_remove;

	// For undo() and redo():
	std::vector<std::pair<Connector *, Connector *>> edges_to_add;
	std::vector<std::pair<Connector *, Connector *>> edges_to_remove;

	void undo() override final;
	void redo() override final;
public:
	CommandPlace(Document &document, Scene &scene,
		     std::unique_ptr<Operator> op,
		     std::vector<std::pair<Connector *, Connector *>> edges_to_add,
		     std::vector<std::pair<Connector *, Connector *>> edges_to_remove);
};

// Place an edge
class CommandPlaceEdge : public QUndoCommand {
	Document &document;
	Scene &scene;

	// For undo() and redo():
	std::pair<Connector *, Connector *> edge_to_add;
	std::pair<Connector *, Connector *> edge_to_remove;

	void undo() override final;
	void redo() override final;
public:
	// If there is no edge to remove, the connector pointers are set to null
	CommandPlaceEdge(Document &document, Scene &scene,
			 std::pair<Connector *, Connector *> edge_to_add,
			 std::pair<Connector *, Connector *> edge_to_remove);
};

// Remove objects
class CommandRemoveObjects : public QUndoCommand {
	Document &document;
	Scene &scene;

	// For redo():
	std::vector<Operator *> ops_to_remove;

	// For undo():
	std::vector<std::unique_ptr<Operator>> ops_to_add;

	// For undo() and redo():
	std::vector<std::pair<Connector *, Connector *>> edges_to_add_or_remove;

	void undo() override final;
	void redo() override final;
public:
	CommandRemoveObjects(Document &document, Scene &scene,
			     std::vector<Operator *> ops_to_remove,
			     std::vector<Edge *> edges_to_remove);
};

class CommandSetState : public QUndoCommand {
	Document &document;
	Scene &scene;

	Operator *op;
	bool merge;

	// For undo() and redo():
	std::unique_ptr<Operator::State> state;

	int id() const override final; // We have to implement this so that Qt tries to merge commands. Sigh.
	void undo() override final;
	void redo() override final;
	bool mergeWith(const QUndoCommand *) override final;
public:
	CommandSetState(Document &document, Scene &scene,
			const QString &text, Operator *op,
			std::unique_ptr<Operator::State> state,
			bool merge);
};

class CommandMove : public QUndoCommand {
	Document &document;
	Scene &scene;

	Operator *op;

	// For undo() and redo():
	QPointF old_pos, new_pos;

	void undo() override final;
	void redo() override final;
public:
	CommandMove(Document &document, Scene &scene, Operator *op,
		    QPointF old_pos, QPointF new_pos);
};

#endif
