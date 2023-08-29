// SPDX-License-Identifier: GPL-2.0
// Defines an edge in the connectivity graph
// This class is responsible for drawing nice paths without hitting
// other operators

#ifndef EDGE_HPP
#define EDGE_HPP

#include "selectable.hpp"
#include "edge_cycle.hpp"

#include <memory>

#include <QGraphicsPathItem>
#include <QPainterPath>

class Connector;
class Scene;
class Document;
class Operator;
class ViewConnection;
class PathFinder;

class Edge : public QGraphicsPathItem
	   , public Selectable {
	Document &document;
	Connector *connector_from;
	Connector *connector_to;
	QPointF	first_point;
	QPointF	second_point;

	QPainterPath path;

	// We keep a pointer to the A* path finder to reuse when we place
	// the edge on the scene
	std::unique_ptr<PathFinder> path_finder;

	// Is the data transported over this edge complex?
	bool comp;

	// True, if this edge can be placed
	// (no cycles, input to output and at most one incoming edge).
	bool can_be_placed;

	// If the user tries to make a cycle, remember the relevant edges,
	// to paint them in warning colour.
	EdgeCycle cycle;

	// If the user tries to replace an input connection,
	// paint the edge to be replaced with a different pen.
	Edge *replace_edge;

	// Check if connector is input connector and already has a edge.
	// If yes, mark the edge as will be replaced.
	void set_replace_edge(Connector *to);

	// View connections that are used
	std::vector<ViewConnection *> view_connections;

	void check_connector_to(Connector *to);
	void render_lines(const std::vector<QPointF> &lines);

	void unwarn_cycle();
	void unwarn_replace_edge();

	// For debugging purposes: helper lines
	std::unique_ptr<QGraphicsPathItem> debug_lines;
protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
	// Override paint method to enable anti-aliasing
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
public:
	Edge(Connector *connector_from, Document &document);
	Edge(Connector *connector_from_, Connector *connector_to_, Document &document_);
	~Edge();

	// These methods are called while adding a new edge or a new operator.
	// If an edge is added, the to connector will be set according to the cursor position.
	// If an operator was added, the to connector is provided as argument.
	void calculate_add_edge(Scene *scene, const QPointF &pos);
	void calculate(const QPointF &pos);

	// Call before destroying this edge to remove warn cycle and replace edge
	void unwarn();

	// Enter selected or unselected mode
	void select() override;
	void deselect() override;

	// Prepares for adding this edge. Returns true if it can be added.
	bool attempt_add();
	void add_connection();

	Operator *get_operator_from();
	Operator *get_operator_to();
	Connector *get_connector_from();
	Connector *get_connector_to();

	const Connector *get_connector_to() const;

	Edge *get_and_clear_replace_edge();

	void remove() override;
	void remove_temporary();
	void register_view_connection(ViewConnection *);
	void unregister_view_connections();

	void set_complex(bool comp);

	// Recalculate path, if old path was obstructed (or freed)
	void recalculate();

	// Recalculate path when moving operator
	void recalculate_move(bool from_input);

	// Turn into JSON object. Used for saving.
	QJsonObject to_json() const;
};

#endif
