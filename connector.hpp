// SPDX-License-Identifier: GPL-2.0
#ifndef CONNECTOR_HPP
#define CONNECTOR_HPP

#include <QGraphicsRectItem>
#include <QEnterEvent>
#include <QJsonArray>

#include "operator_list.hpp"

class Edge;
class FFTBuf;

class Connector : public QGraphicsRectItem {
public:
	static constexpr double width = 5.0;
	static constexpr double height = 10.0;

protected:
	void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
	void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
private:
	size_t id;
	bool output;
	bool highlighted;
	bool selected;

	OperatorList::view_list	view_list;

	// For input nodes we only allow one connection
	Edge 			*parent;

	// For output nodes we allow multiple connections
	std::vector<Edge *>	children;

	// Position where we paint edges to (in scene coordinates)
	QPointF			safety_pos;
public:
	enum { Type = UserType + 2 };
	int type() const override { return Type; }

	Connector(size_t id, bool output, Operator *parent);

	ConnectorDesc connector_desc();
	size_t get_id() const;

	void set_highlighted(bool highlighted_);
	void set_selected(bool selected_);
	bool is_output() const;
	bool has_input_connection() const;
	void output_buffer_changed();

	// Distance to y position
	double y_dist(double y) const;

	QPointF line_from() const;

	Operator *op();
	const Operator *op() const;

	void set_safety_pos(const QPointF &p);
	QPointF get_safety_pos() const;

	// Input connector is either not connected or connected to an empty buffer
	bool is_empty_buffer();

	// Get buffer of input connector
	FFTBuf &get_buffer();

	OperatorList::view_list &get_view_list();
	const OperatorList::view_list &get_view_list() const;

	// Connect
	void set_input_connection(Edge *);
	void remove_input_connection(Edge *);
	void add_output_connection(Edge *);
	void remove_output_connection(Edge *);

	// Delete all edges
	void remove_edges();

	// Recalculate the position of all edges
	void recalculate_edges();

	const Connector *get_parent() const;			// nullptr if no parent
	Edge *get_parent_edge();				// nullptr if no parent
	std::vector<Connector *> get_children() const;
	const std::vector<Edge *> &get_children_edges() const;

	// Write all output edges to JSON. Used for saving.
	void out_edges_to_json(QJsonArray &out) const;
};

#endif
