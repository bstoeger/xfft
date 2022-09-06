// SPDX-License-Identifier: GPL-2.0
// Class that encapsulates state and code needed while adding an operator,
// notably a list of edges to be added and a background rectangular item
// that visualizes the boundary which has to be kept clear.

#ifndef OPERATOR_ADDER_HPP
#define OPERATOR_ADDER_HPP

#include "edge_cycle.hpp"

#include <memory>
#include <list>

#include <QPointF>
#include <QRectF>
#include <QGraphicsRectItem>

class Operator;
class Connector;
class MainWindow;

class OperatorAdder {
	// Reference to document
	MainWindow &w;

	std::unique_ptr<Operator> op;
	bool prohibited;
	Connector *connector_under_mouse;

	// Visualize the space needed for the operator with a rectangle.
	// The rectangle is the size of the operator plus twice the "safety" distance
	// to all sides.
	std::unique_ptr<QGraphicsRectItem> safety_rect;

	// Keep track of input and output edges that are to be added
	class EdgeList {
		std::vector<std::unique_ptr<Edge>> edges;
		size_t count;
		bool add_input(Connector *conn);
	public:
		EdgeList(size_t num);
		~EdgeList();
		void clear();
		void add(MainWindow &w, Connector *conn, bool output);
		void move_to(const QPointF &pos, const QRectF &bounding_rect, bool output);
		bool warn_cycles();
		std::vector<std::unique_ptr<Edge>> &get_edges();
		void to_connectors(std::vector<std::pair<Connector *, Connector *>> &res, Operator &op, bool output) const; // Adds to array
		std::vector<std::pair<Connector *, Connector *>> replace_edges_to_connectors(); // Clears the edges to replace
	};
	EdgeList input_edges;
	EdgeList output_edges;

	// Add edge to a connector is added
	void add_connector_edge(Connector *conn);

	// Test for cycles (there may be more than one)
	std::list<EdgeCycle> cycles;
	bool warn_cycles();
	void unwarn_cycles();
public:
	OperatorAdder(MainWindow &w, std::unique_ptr<Operator> op);
	~OperatorAdder();

	// Return true if operator was placed
	bool clicked();

	// Remove edges to be added
	void clear_edges();

	bool is_operator(const Operator *) const;

	void move_to(const QPointF &pos, Connector *connector_under_mouse);
};

#endif
