// SPDX-License-Identifier: GPL-2.0
// This class describes which corners or connector of an operator
// can be seen from the corner or connector of a different operator

#ifndef VIEW_CONNECTION_HPP
#define VIEW_CONNECTION_HPP

#include "connector_pos.hpp"

// For debugging purposes
#include <QGraphicsLineItem>
#include <QRect>

#include <memory>

class Scene;
class Edge;

class ViewConnection {
private:
	struct Vertex {
		ConnectorDesc desc;
		QPointF pos;
		Vertex(const ConnectorDesc &desc, const QPointF &pos);
	};
	// We always go from left (low x) to right (hiqh x)
	// If both operators have the same x-coordinate,
	// we go from top (low y) to bottom (high y)
	Vertex left;
	Vertex right;
	double dist;

	// Remember all edges that use this connection
	std::vector<Edge *> edges;

	// For debugging purposes
	std::unique_ptr<QGraphicsLineItem> line;
public:
	~ViewConnection();
	ViewConnection(const ConnectorDesc &from, const QPointF &pos_from, const ConnectorDesc &to, const QPointF &pos_to, Scene &scene);
	ConnectorDesc get_other(const ConnectorDesc &, QPointF &pos);
	ConnectorDesc get_other(const ConnectorDesc &);

	bool cuts_rect(const QRectF &rect) const;
	double get_dist() const;

	// Get all edges
	bool used_by_edge() const;
	const std::vector<Edge *> &get_edges() const;

	// Collect edges into array without generating duplicates
	void collect_edges(std::vector<Edge *> &) const;

	void add_edge(Edge *);
	void remove_edge(Edge *);
};

#endif
