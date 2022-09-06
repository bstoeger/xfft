// SPDX-License-Identifier: GPL-2.0
// This class keeps track of the operators in the scene
// Operators are sorted by right, left, top and bottom boundaries to
// make a rather quick collision check possible.
// A quad- or R-tree might be more efficient.
#ifndef OPERATOR_LIST_HPP
#define OPERATOR_LIST_HPP

#include "view_connection.hpp"

#include <vector>
#include <list>

class Scene;

struct CornerDistance {
	ConnectorDesc		conn;
	QPointF			pos;
	double			d;
	CornerDistance(Operator *op, int corner_id, QPointF pos, double d);
};

class OperatorList {
public:
	// Entry in lists for sorting by boundary
	struct Entry {
		Operator *op;
		double	boundary;
		bool operator<(const Entry & e);
		Entry(Operator *op, double boundary);
	};
private:
	// Since operators will be added only rather sparingly, we keep them in
	// pre-sorted vectors that can be searched quickly.
	std::vector<Entry> right_list;
	std::vector<Entry> left_list;
	std::vector<Entry> top_list;
	std::vector<Entry> bottom_list;

	// List of all view connections
	std::list<ViewConnection> view_connections;
public:
	using view_iterator = std::list<ViewConnection>::iterator;
	using view_list = std::list<view_iterator>;
private:
	static void add(Operator *, double boundary, std::vector<Entry> &list);
	static void remove(Operator *, std::vector<Entry> &list);

	bool check_hit(Operator *, bool left_right, double boundary, double a, double b, QPointF &hit_at) const;
	Operator *find_first_hit(const std::vector<Entry> &list, bool go_up, bool left_right,
				 double from, double to, double a, double b, QPointF &hit_at,
				 const Operator *ignore) const;
	void make_view_connections(Operator *op_from, const ConnectorPos &pos_from,
				   Operator *op_to, const ConnectorPos &pos_to,
				   Scene &scene, bool check_existing);
	void add_intra_op_view_connection(Operator *op, int corner_from, int corner_to, Scene &scene);
	void add_intra_op_view_connection(Operator *op, int corner_from, const ConnectorPos &pos_to, Scene &scene);
	void add_intra_op_view_connection(Operator *op, const ConnectorPos &from, const ConnectorPos &pos_to, Scene &scene);
	void add_view_connection(const ConnectorDesc &from, const ConnectorDesc &to, Scene &scene);
	void add_view_connection(Operator *op_from, const ConnectorPos &pos_from,
				 Operator *op_to, const ConnectorPos &pos_to,
				 Scene &scene);
public:
	OperatorList();
	void add(Operator *, Scene &scene);
	void remove(Operator *, Scene &scene);
	void remove_view(const view_iterator &it);
	size_t num_operators() const;

	// Find first operator in path
	// Returns nullptr if non in path
	// The last argument is ignored (for when we start at this
	// operator and move away from there)
	Operator *find_first_in_path(const QPointF &from, const QPointF &to, QPointF &hit_at, const Operator *ignore) const;

	// Return operator with safety rectangle under give position.
	// Return nullptr if none
	Operator *get_operator_by_safety_rect(const QPointF &pos) const;

	// Returns whether safety rectangle of an oparator is in the given rectangle
	bool operator_in_rect(const QRectF &rect) const;

	// Get all corners of operators visible from this point
	std::vector<CornerDistance>  get_visible_corners(const QPointF &pos) const;

	// Clear all objects
	void clear();
};

#endif
