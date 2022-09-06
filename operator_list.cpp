// SPDX-License-Identifier: GPL-2.0
#include "operator_list.hpp"
#include "operator.hpp"
#include "edge.hpp"

#include <algorithm>
#include <cassert>

bool operator<(double boundary, const OperatorList::Entry &e)
{
	return boundary < e.boundary;
}

bool operator<(const OperatorList::Entry &e, double boundary)
{
	return e.boundary < boundary;
}

OperatorList::Entry::Entry(Operator *op_, double boundary_)
	: op(op_)
	, boundary(boundary_)
{
}

bool OperatorList::Entry::operator<(const Entry & e)
{
	return boundary < e.boundary;
}

void OperatorList::add(Operator *op, double boundary, std::vector<Entry> &list)
{
	auto it = upper_bound(list.begin(), list.end(), boundary);
	list.insert(it, Entry(op, boundary));
}

void OperatorList::remove(Operator *op, std::vector<Entry> &list)
{
	auto it = std::find_if(list.begin(), list.end(), [op](const Entry &e){ return e.op == op; });
	assert(it != list.end());
	list.erase(it);
}

OperatorList::OperatorList()
{
	left_list.reserve(24);
	right_list.reserve(24);
	top_list.reserve(24);
	bottom_list.reserve(24);
}

void OperatorList::add_view_connection(Operator *op_from, const ConnectorPos &pos_from,
				       Operator *op_to, const ConnectorPos &pos_to,
				       Scene &scene)
{
	ConnectorDesc from(op_from, pos_from.type);
	ConnectorDesc to(op_to, pos_to.type);
	view_connections.emplace_back(from, pos_from.pos, to, pos_to.pos, scene);
	view_iterator it = std::prev(view_connections.end());
	op_from->add_view_connection(pos_from.type, it);
	op_to->add_view_connection(pos_to.type, it);
}

// If the last parameter is true, check before whether this connection already exists.
// This is used when deleting an operator and the connection might already exist.
void OperatorList::make_view_connections(Operator *op_from, const ConnectorPos &pos_from,
					 Operator *op_to, const ConnectorPos &pos_to,
					 Scene &scene, bool check_existing)
{
	double delta_x = pos_to.pos.x() - pos_from.pos.x();
	double delta_y = pos_to.pos.y() - pos_from.pos.y();
	if (pos_from.type.is_input_connector()) {
		if (delta_x > 0.0)
			return;
	} else if (pos_from.type.is_output_connector()) {
		if (delta_x < 0.0)
			return;
	} else {
		switch (pos_from.type.corner_id()) {
		case 0:
			if (delta_x < 0.0 && delta_y < 0.0) return;
			break;
		case 1:
			if (delta_x < 0.0 && delta_y > 0.0) return;
			break;
		case 2:
			if (delta_x > 0.0 && delta_y > 0.0) return;
			break;
		case 3:
			if (delta_x > 0.0 && delta_y < 0.0) return;
			break;
		}
	}
	if (pos_to.type.is_input_connector()) {
		if (delta_x < 0.0)
			return;
	} else if (pos_to.type.is_output_connector()) {
		if (delta_x > 0.0)
			return;
	} else {
		switch (pos_to.type.corner_id()) {
		case 0:
			if (delta_x > 0.0 && delta_y > 0.0) return;
			break;
		case 1:
			if (delta_x > 0.0 && delta_y < 0.0) return;
			break;
		case 2:
			if (delta_x < 0.0 && delta_y < 0.0) return;
			break;
		case 3:
			if (delta_x < 0.0 && delta_y > 0.0) return;
			break;
		}
	}

	if (check_existing) {
		const view_list &view_list = op_from->get_view_list(pos_from.type);
		ConnectorDesc desc_from(op_from, pos_from.type);
		ConnectorDesc desc_to(op_to, pos_to.type);
		if (std::find_if(view_list.begin(), view_list.end(),
				[ &desc_from, &desc_to](const view_iterator &it)
				{ return it->get_other(desc_from) == desc_to; }) != view_list.end()) {
			return;
		}
	}

	QPointF dummy;
	Operator *hit = find_first_in_path(pos_from.pos, pos_to.pos, dummy, op_to);
	if (hit == nullptr)
		add_view_connection(op_from, pos_from, op_to, pos_to, scene);
}

void OperatorList::add_intra_op_view_connection(Operator *op, int corner_from, int corner_to, Scene &scene)
{
	ConnectorPos pos_from(ConnectorType::corner(corner_from), op->corner_coord(corner_from));
	ConnectorPos pos_to(ConnectorType::corner(corner_to), op->corner_coord(corner_to));
	add_view_connection(op, pos_from, op, pos_to, scene);
}

void OperatorList::add_intra_op_view_connection(Operator *op, int corner_from, const ConnectorPos &pos_to, Scene &scene)
{
	ConnectorPos pos_from(ConnectorType::corner(corner_from), op->corner_coord(corner_from));
	add_view_connection(op, pos_from, op, pos_to, scene);
}

void OperatorList::add_intra_op_view_connection(Operator *op, const ConnectorPos &pos_from, const ConnectorPos &pos_to, Scene &scene)
{
	add_view_connection(op, pos_from, op, pos_to, scene);
}

void OperatorList::add(Operator *op, Scene &scene)
{
	QRectF rect = op->get_safety_rect();

	// If the new operator is placed on existing view connections
	// these have to be removed. Edges using this connections
	// are collected, because their path has to be recalculated later on
	std::vector<Edge *> edges_to_recalculate;
	for (auto it = view_connections.begin(); it != view_connections.end(); ) {
		auto next_it = std::next(it);
		if (it->cuts_rect(rect)) {
			// We copy the vector, because removing edges will modify the original vector
			std::vector<Edge *> edges = it->get_edges();
			for (Edge *e: edges)
				e->unregister_view_connections();
			assert(!it->used_by_edge());
			edges_to_recalculate.insert(edges_to_recalculate.end(), edges.begin(), edges.end());
			view_connections.erase(it);
		}
		it = next_it;
	}

	// Before inserting, generate all view connections.
	// We could speed this up by remembering which parts are shadowed -
	// but is it worth the trouble?
	const std::vector<ConnectorPos> &connectors = op->get_connector_pos();
	for (Entry &entry: left_list) {
		for (const ConnectorPos &from: connectors) {
			for (const ConnectorPos &to: entry.op->get_connector_pos())
				make_view_connections(op, from, entry.op, to, scene, false);
		}
	}

	// Add inter-operator connectivities, so that we can go around corners
	for (size_t i = 0; i < 4; ++i)
		add_intra_op_view_connection(op, i, (i + 1) % 4, scene);

	const ConnectorPos *prev = nullptr;
	for (const ConnectorPos &conn: connectors) {
		if (conn.type.is_corner())
			continue;
		if (conn.type.is_input_connector()) {
			add_intra_op_view_connection(op, 2, conn, scene);
			add_intra_op_view_connection(op, 3, conn, scene);
		} else {
			add_intra_op_view_connection(op, 0, conn, scene);
			add_intra_op_view_connection(op, 1, conn, scene);
		}
		if (prev && prev->type.is_input_connector() == conn.type.is_input_connector())
			add_intra_op_view_connection(op, *prev, conn, scene);
		prev = &conn;
	}

	add(op, rect.left(), left_list);
	add(op, rect.right(), right_list);
	add(op, rect.top(), top_list);
	add(op, rect.bottom(), bottom_list);

	// Recalculate edges
	for (Edge *e: edges_to_recalculate)
		e->recalculate();
}

void OperatorList::remove(Operator *op, Scene &scene)
{
	remove(op, left_list);
	remove(op, right_list);
	remove(op, top_list);
	remove(op, bottom_list);

	// Reconstruct the views that were blocked by this operator.
	QRectF removed_rect = op->get_safety_rect();

	// We loop from left to right. Thus we only have to connect output to input connectors.
	for (auto it1 = left_list.begin(); it1 != left_list.end(); ++it1) {
		// We can stop if the left side of the operator is to the right of the old operator.
		if (it1->boundary > removed_rect.right())
			break;
		Operator *op1 = it1->op;
		QRectF safety_rect1 = op1->get_safety_rect();
		const std::vector<ConnectorPos> &connectors = op1->get_connector_pos();

		for (auto it2 = std::next(it1); it2 != left_list.end(); ++it2) {
			// No point in checking if right side of operator is to the left of old operator
			Operator *op2 = it2->op;
			QRectF safety_rect2 = op2->get_safety_rect();
			if (safety_rect2.right() < removed_rect.left())
				continue;

			// No point in checking if both are above or below old operator
			if (safety_rect1.bottom() < removed_rect.top() && safety_rect2.bottom() < removed_rect.top())
				continue;
			if (safety_rect1.top() > removed_rect.bottom() && safety_rect2.top() > removed_rect.bottom())
				continue;

			for (const ConnectorPos &from: connectors) {
				for (const ConnectorPos &to: op2->get_connector_pos()) {
					make_view_connections(op1, from, op2, to, scene, true);
				}
			}
		}
	}
}

void OperatorList::remove_view(const view_iterator &it)
{
	view_connections.erase(it);
}

bool OperatorList::check_hit(Operator *op, bool left_right, double boundary, double a, double b, QPointF &hit_at) const
{
	double y_pos = a*boundary + b;
	QRectF rect = op->get_safety_rect();
	if (left_right) {
		if (y_pos >= rect.top() && y_pos <= rect.bottom()) {
			hit_at = QPointF(boundary, y_pos);
			return true;
		}
	} else {
		if (y_pos >= rect.left() && y_pos <= rect.right()) {
			hit_at = QPointF(y_pos, boundary);
			return true;
		}
	}
	return false;
}

Operator *OperatorList::find_first_hit(const std::vector<Entry> &list, bool go_up, bool left_right,
				       double from, double to, double a, double b, QPointF &hit_at,
				       const Operator *ignore) const
{
	if (list.empty())
		return nullptr;
	if (go_up) {
		assert(from < to);
		auto it = upper_bound(list.begin(), list.end(), from);
		while (it != list.end() && it->boundary < to) {
			if (it->op != ignore && check_hit(it->op, left_right, it->boundary, a, b, hit_at))
				return it->op;
			++it;
		}
	} else {
		assert(to < from);
		auto it = lower_bound(list.begin(), list.end(), from);
		while (it != list.begin()) {
			--it;
			if (it->boundary < to)
				break;
			if (it->op != ignore && check_hit(it->op, left_right, it->boundary, a, b, hit_at))
				return it->op;
		}
	}
	return nullptr;
}

Operator *OperatorList::find_first_in_path(const QPointF &from, const QPointF &to, QPointF &hit_at, const Operator *ignore) const
{
	double delta_x = to.x() - from.x();
	double delta_y = to.y() - from.y();

	// We search for the first horizontal wall we hit and
	// the first vertical wall we hit. Then check which one
	// of the two we hit first.
	QPointF hit_at_horizontal;
	Operator *hit_horizontal = nullptr;

	// We don't have to check horizontal wall for vertical lines
	if (fabs(delta_x) > 0.01) {
		// Calculate line equation of the form y = ax+b
		double a = delta_y / delta_x;
		double b = from.y() - a * from.x();
		hit_horizontal = delta_x > 0
			? find_first_hit(left_list, true, true, from.x(), to.x(), a, b, hit_at_horizontal, ignore)
			: find_first_hit(right_list, false, true, from.x(), to.x(), a, b, hit_at_horizontal, ignore);
	}

	QPointF hit_at_vertical;
	Operator *hit_vertical = nullptr;

	// We don't have to check vertical wall for horizontal lines
	if (fabs(delta_y) > 0.01) {
		// Calculate line equation of the form x = ay+b
		double a = delta_x / delta_y;
		double b = from.x() - a * from.y();
		hit_vertical = delta_y > 0
			? find_first_hit(top_list, true, false, from.y(), to.y(), a, b, hit_at_vertical, ignore)
			: find_first_hit(bottom_list, false, false, from.y(), to.y(), a, b, hit_at_vertical, ignore);
	}

	if (!hit_horizontal && !hit_vertical)
		return nullptr;

	if (!hit_horizontal || (hit_vertical && (delta_x > 0.0) == (hit_at_vertical.x() < hit_at_horizontal.x()))) {
		hit_at = hit_at_vertical;
		return hit_vertical;
	} else {
		hit_at = hit_at_horizontal;
		return hit_horizontal;
	}
}

// TODO: For the following two accessors, check in which list we have the least to search
Operator *OperatorList::get_operator_by_safety_rect(const QPointF &pos) const
{
	auto it = upper_bound(left_list.begin(), left_list.end(), pos.x());
	while (it != left_list.begin()) {
		--it;
		QRectF rect = it->op->get_safety_rect();
		if (rect.contains(pos))
			return it->op;
	}
	return nullptr;
}

bool OperatorList::operator_in_rect(const QRectF &rect) const
{
	auto it = upper_bound(left_list.begin(), left_list.end(), rect.right());
	while (it != left_list.begin()) {
		--it;
		QRectF safety_rect = it->op->get_safety_rect();
		if (safety_rect.intersects(rect))
			return true;
	}
	return false;
}

static double euclidean_dist(const QPointF &p1, const QPointF &p2)
{
	QPointF diff = p2 - p1;
	return sqrt(diff.x()*diff.x() + diff.y()*diff.y());
}

CornerDistance::CornerDistance(Operator *op, int corner_id, QPointF pos_, double d_)
	: conn(op, ConnectorType::corner(corner_id))
	, pos(pos_)
	, d(d_)
{
}

std::vector<CornerDistance> OperatorList::get_visible_corners(const QPointF &pos) const
{
	size_t num_operators = left_list.size();
	std::vector<CornerDistance> res;
	res.reserve(3 * num_operators);		// One can see at the most three corners of a rectangle.

	for (const Entry &e: left_list) {
		Operator *op = e.op;
		int visible_corners = op->visible_corners(pos);
		for (int i = 0; i < 4; ++i) {
			if ((visible_corners & (1 << i)) == 0)
				continue;
			QPointF corner_pos = op->corner_coord(i);
			QPointF dummy;
			Operator *hit = find_first_in_path(pos, corner_pos, dummy, op);
			if (hit)
				continue;

			double dist = euclidean_dist(pos, corner_pos);
			res.emplace_back(op, i, corner_pos, dist);
		}
	}
	return res;
}

size_t OperatorList::num_operators() const
{
	return left_list.size();
}

void OperatorList::clear()
{
	// Copy list, because operator removes itself from the list
	for (Entry &e: left_list)
		e.op->remove_edges();
	view_connections.clear();
	for (Entry &e: left_list)
		delete e.op;
	right_list.clear();
	left_list.clear();
	top_list.clear();
	bottom_list.clear();
}
