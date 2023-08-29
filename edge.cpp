// SPDX-License-Identifier: GPL-2.0
#include "edge.hpp"
#include "operator.hpp"
#include "connector.hpp"
#include "scene.hpp"
#include "document.hpp"
#include "topological_order.hpp"
#include "globals.hpp"

#include <QCursor>
#include <QGraphicsSceneMouseEvent>

#include <cassert>
#include <boost/heap/binomial_heap.hpp>

enum class EdgeMode {
	unplaced, placed, selected, replace
};

static const QColor unplaced_color = Qt::gray;
static const QColor real_color = Qt::blue;
static const QColor complex_color = Qt::red;
static constexpr int pen_width_standard = 3;
static constexpr int pen_width_replace = 3;

static QPen get_pen(EdgeMode mode, bool comp)
{
	QColor color = comp ? complex_color : real_color;
	switch (mode) {
		default:
		case EdgeMode::unplaced:
			return QPen(unplaced_color, pen_width_standard);
		case EdgeMode::placed:
			return QPen(color, pen_width_standard);
		case EdgeMode::selected:
			return QPen(color, pen_width_standard, Qt::DotLine);
		case EdgeMode::replace:
			return QPen(color, pen_width_replace, Qt::DotLine);
	};
}

Edge::Edge(Connector *connector_from_, Document &document_)
	: document(document_)
	, connector_from(connector_from_)
	, connector_to(nullptr)
	, comp(false)
	, can_be_placed(false)
	, replace_edge(nullptr)
{
	assert(connector_from);
	first_point = connector_from->line_from();
	second_point = connector_from->op()->go_out_of_safety_rect(first_point);
	connector_from->set_selected(true);

	// We start in unplaced mode -> make line gray and transparent
	setPen(get_pen(EdgeMode::unplaced, comp));
	setOpacity(0.5);

	// If this is an input connector and it already has an edge,
	// place this edge in add mode
	set_replace_edge(connector_from);
}

Edge::Edge(Connector *connector_from_, Connector *connector_to_, Document &document_)
	: document(document_)
	, connector_from(connector_from_)
	, connector_to(connector_to_)
	, comp(connector_from->is_complex_buffer())
	, can_be_placed(false)
	, replace_edge(nullptr)
{
	assert(connector_from);
	assert(connector_to);
	check_connector_to(connector_to);

	setPen(get_pen(EdgeMode::placed, comp));
}

Edge::~Edge()
{
}

void Edge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	painter->setRenderHint(QPainter::Antialiasing);
	QGraphicsPathItem::paint(painter, option, widget);
}

void Edge::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (!(event->buttons() & Qt::LeftButton))
		return;
	static_cast<Scene *>(scene())->selectable_clicked(this, event);
}

// What follows is a simple implementation of the A* algorithm.
struct TreeEntry {
	const TreeEntry	*parent;		// nullptr: at root
	ViewConnection *view_connection;	// By which connection we got there
	ConnectorDesc	conn;			// If not a corner, this is the destination element
	QPointF		pos;
	double		dist;			// Distance travelled from source
	double		estimate;		// Estimated total distance
	bool		closed;			// True: we found optimal path to this entry

	TreeEntry(const TreeEntry *parent, ViewConnection *view_connection,
		  const ConnectorDesc &conn_, const QPointF &pos, double dist, double estimate);
	bool operator<(const TreeEntry &e2) const;
};

struct IndirectTreeEntry {
	TreeEntry *entry;
public:
	IndirectTreeEntry(TreeEntry *);
	bool operator<(const IndirectTreeEntry &e2) const;
	bool operator==(const IndirectTreeEntry &e2) const;
	TreeEntry &operator*();
	TreeEntry *operator->();
	const TreeEntry &operator*() const;
	const TreeEntry *operator->() const;
};

class PathFinder
{
	ConnectorDesc target;
	QPointF target_pos;
	QPointF from_pos;

	std::vector<TreeEntry> entries;
	using heap_t = boost::heap::binomial_heap<IndirectTreeEntry>;
	heap_t open_list;
	TreeEntry	*final_entry;

	void iterate();
	double heuristics(const QPointF &) const;
	void expand(const TreeEntry *parent, const ConnectorDesc &from, const OperatorList::view_list &view_list);
public:
	PathFinder(Connector *target, const QPointF &target_pos, const OperatorList &operator_list);
	void calculate(const std::vector<CornerDistance> &corner_distances, const QPointF &from_pos);
	void calculate(const ConnectorDesc &connector_from, const QPointF &from_pos_);
	void to_lines(std::vector<QPointF> &lines) const;
	void register_view_connections(Edge *e) const;
	void add_entry(const TreeEntry *parent, ViewConnection *view_connection,
		       const ConnectorDesc &conn, const QPointF &pos, double dist, double estimate);
};

TreeEntry::TreeEntry(const TreeEntry *parent_, ViewConnection *view_connection_,
		     const ConnectorDesc &conn_, const QPointF &pos_, double dist_, double estimate_)
	: parent(parent_)
	, view_connection(view_connection_)
	, conn(conn_)
	, pos(pos_)
	, dist(dist_)
	, estimate(estimate_)
	, closed(false)
{
}

bool TreeEntry::operator<(const TreeEntry &e2) const
{
	// Because priority lists give the top element,
	// entries with small length are considered as larger
	return estimate > e2.estimate;
}

IndirectTreeEntry::IndirectTreeEntry(TreeEntry *entry_)
	: entry(entry_)
{
}

bool IndirectTreeEntry::operator<(const IndirectTreeEntry &e2) const
{
	return *entry < *(e2.entry);
}

bool IndirectTreeEntry::operator==(const IndirectTreeEntry &e2) const
{
	return entry == e2.entry;
}

TreeEntry &IndirectTreeEntry::operator*()
{
	return *entry;
}

TreeEntry *IndirectTreeEntry::operator->()
{
	return entry;
}

const TreeEntry &IndirectTreeEntry::operator*() const
{
	return *entry;
}

const TreeEntry *IndirectTreeEntry::operator->() const
{
	return entry;
}

PathFinder::PathFinder(Connector *target_, const QPointF &target_pos_, const OperatorList &operator_list)
	: target(target_->connector_desc())
	, target_pos(target_pos_)
	, final_entry(nullptr)
{
	size_t num_operators = operator_list.num_operators();
	entries.reserve(num_operators * 4 + 2);
	//open_list.reserve(num_operators * 4 + 2);
}

void PathFinder::add_entry(const TreeEntry *parent, ViewConnection *view_connection,
			   const ConnectorDesc &conn, const QPointF &pos, double dist, double estimate)
{
	// We use pointers to elements in a vector. Therefore the vector must never grow.
	assert(entries.capacity() >= entries.size() + 1);

	TreeEntry &e = entries.emplace_back(parent, view_connection, conn, pos, dist, estimate);
	open_list.push(IndirectTreeEntry(&e));
}

void PathFinder::calculate(const std::vector<CornerDistance> &corner_distances, const QPointF &from_pos_)
{
	from_pos = from_pos_;
	for (const CornerDistance &corner: corner_distances)
		add_entry(nullptr, nullptr, corner.conn, corner.pos, corner.d, corner.d + heuristics(corner.pos));
	iterate();
}

void PathFinder::calculate(const ConnectorDesc &connector_from, const QPointF &from_pos_)
{
	from_pos = from_pos_;
	expand(nullptr, connector_from, connector_from.op->get_view_list(connector_from.type));
	iterate();
}

double PathFinder::heuristics(const QPointF &pos) const
{
	QPointF diff = pos - target_pos;
	return sqrt(diff.x()*diff.x() + diff.y()*diff.y());
}

void PathFinder::iterate()
{
	while (!open_list.empty()) {
		IndirectTreeEntry e = open_list.top();
		if (!e->conn.type.is_corner()) {
			// Yay, found the shortest path
			final_entry = &*e;
			return;
		}
		open_list.pop();
		e->closed = true;
		expand(&*e, e->conn, e->conn.op->get_view_list(e->conn.type));
	}
	// Ugh, we did not find a path, leave final_entry as nullptr.
}

void PathFinder::expand(const TreeEntry *parent, const ConnectorDesc &from, const OperatorList::view_list &view_list)
{
	for (const OperatorList::view_iterator &it: view_list) {
		QPointF pos;
		ConnectorDesc child = it->get_other(from, pos);

		// Use connections to connectors only if they are the target
		if (!child.type.is_corner() && child != target)
			continue;

		double new_dist = parent ? parent->dist : 0.0;
		new_dist += it->get_dist();

		// Try to find this node
		auto it2 = std::find_if(entries.begin(), entries.end(), [&child](const TreeEntry &e)
									{return e.conn == child;});
		if (it2 == entries.end()) {
			add_entry(parent, &*it, child, pos, new_dist, new_dist+heuristics(pos));
			continue;
		}

		// Found node. If it is closed, we made a cycle and give up with this path
		if (it2->closed)
			continue;

		// We have to search this entry in the open list
		IndirectTreeEntry e(&*it2);
		auto it3 = std::find(open_list.begin(), open_list.end(), e);
		assert(it3 != open_list.end());
		if ((*it3)->dist <= new_dist)
			continue;

		// We found a shorter way to this entry -> update accordingly
		auto handle = heap_t::s_handle_from_iterator(it3);
		double diff = (*handle)->dist - new_dist;
		assert(diff > 0.0);
		(*handle)->parent = parent;
		(*handle)->view_connection = &*it;
		(*handle)->dist = new_dist;
		(*handle)->estimate -= diff;
		open_list.decrease(handle);
	}
}

void PathFinder::to_lines(std::vector<QPointF> &lines) const
{
	for (const TreeEntry *act = final_entry; act; act = act->parent)
		lines.push_back(act->pos);
	lines.push_back(from_pos);
}

void PathFinder::register_view_connections(Edge *e) const
{
	for (const TreeEntry *act = final_entry; act; act = act->parent) {
		if (act->view_connection) {
			act->view_connection->add_edge(e);
			e->register_view_connection(act->view_connection);
		}
	}
}

void Edge::set_replace_edge(Connector *to)
{
	assert(to);
	if (to->is_output())
		return;
	if (replace_edge)
		unwarn_replace_edge();
	if (to->has_input_connection()) {
		replace_edge = to->get_parent_edge();
		assert(replace_edge);

		replace_edge->setPen(get_pen(EdgeMode::replace, comp));
	}
}

void Edge::check_connector_to(Connector *to)
{
	// By default, not OK.
	can_be_placed = false;

	if (!to ||
	    connector_from->is_output() == to->is_output() ||
	    connector_from->op() == to->op()) {
		if (connector_from->is_output())
			unwarn_replace_edge();
		return;
	}

	if (connector_from->is_output() == to->is_output())
		return;

	// No connection connecting an operator to itself
	// (This case is not found in the loop test below, because
	// it is a zero-length loop)
	if (connector_from->op() == to->op())
		return;

	// Check for loop (i.e. can we reach the source from the target)
	Operator *to_op = to->op();
	Operator *from_op = connector_from->op();
	if (to->is_output())
		std::swap(to_op, from_op);
	cycle = document.topo.find_connection(to_op, from_op);
	if (!cycle.empty()) {
		cycle.warn();
		return;
	}

	can_be_placed = true;

	// If there is already a connection, highlight this connection
	set_replace_edge(to);
}

void Edge::unwarn_cycle()
{
	cycle.unwarn();
	cycle.clear();
}

void Edge::unwarn_replace_edge()
{
	if (replace_edge) {
		replace_edge->setPen(get_pen(EdgeMode::placed, comp));
		replace_edge = nullptr;
	}
}

void Edge::unwarn()
{
	unwarn_cycle();
	unwarn_replace_edge();
}

void Edge::calculate_add_edge(Scene *scene, const QPointF &pos_)
{
	QPointF pos = pos_;
	Connector *conn = scene->connector_at(pos);

	// We don't connect to the same connector
	if (conn == connector_from)
		conn = nullptr;

	if (connector_to && conn != connector_to)
		connector_to->set_selected(false);
	if (conn && conn != connector_to)
		conn->set_selected(true);

	if (connector_to != conn) {
		unwarn_cycle();

		// Connector changed -> check if everything is OK
		check_connector_to(conn);

		// If something changed, move the forbidden sign
		if (!can_be_placed && conn)
			scene->set_cursor(Qt::ForbiddenCursor);
		else
			scene->set_cursor(Qt::ClosedHandCursor);

		connector_to = conn;
	}
	calculate(pos);
}

void Edge::calculate(const QPointF &pos_)
{
	QPointF pos = pos_;

	// We have two different modes:
	// If the mouse is in free space, we have to add
	// visibility lines from this position to the tree.
	// If the mouse is over a connector, we can use the
	// existing visibility tree.
	QPointF target_pos = connector_from->get_safety_pos();
	path_finder = std::make_unique<PathFinder>(connector_from, target_pos, document.operator_list);

	std::vector<QPointF> lines;
	lines.reserve(10);		// We need at least 3
	lines.push_back(first_point);

	//QPainterPath path(first_point);
	if (!connector_to) {
		// First check if we are inside a safety rect and move out if this is the case
		Operator *op = document.operator_list.get_operator_by_safety_rect(pos);
		if (op)
			pos = op->go_out_of_safety_rect(pos);

		// Check the trivial case whether we can directly reach the target
		double x_diff = pos.x() - second_point.x();
		bool left_right_connector = connector_from->is_output();
		bool left_right_target = x_diff > 0.0;
		// Note: we can check for 0.0 because x is set to exactly 0.0
		// by go_out_of_safety rect.
		bool direction_ok = x_diff == 0.0 || (left_right_connector == left_right_target);
		QPointF dummy;
		if (direction_ok &&
		    document.operator_list.find_first_in_path(second_point, pos, dummy, nullptr) == nullptr) {
			lines.push_back(second_point);
			lines.push_back(pos);
		} else {
			std::vector<CornerDistance> corner_distances;
			corner_distances = document.operator_list.get_visible_corners(pos);
			path_finder->calculate(corner_distances, pos);
			path_finder->to_lines(lines);
		}
	} else {
		QPointF pos_to = connector_to->get_safety_pos();
		path_finder->calculate(connector_to->connector_desc(), pos_to);
		path_finder->to_lines(lines);
		lines.push_back(connector_to->line_from());
	}
	render_lines(lines);
}

void Edge::recalculate_move(bool from_input)
{
	unregister_view_connections();

	Connector *conn1 = connector_from;
	Connector *conn2 = connector_to;
	if (from_input)
		std::swap(conn1, conn2);
	QPointF pos = conn2->get_safety_pos();

	QPointF target_pos = conn1->get_safety_pos();
	QPointF first_point = conn1->line_from();
	QPointF second_point = conn1->op()->go_out_of_safety_rect(first_point);
	path_finder = std::make_unique<PathFinder>(conn1, target_pos, document.operator_list);

	std::vector<QPointF> lines;
	lines.reserve(10);		// We need at least 3
	lines.push_back(first_point);

	// First check if we are inside a safety rect and move out if this is the case
	Operator *op = document.operator_list.get_operator_by_safety_rect(pos);
	if (op)
		pos = op->go_out_of_safety_rect(pos);

	// Check the trivial case whether we can directly reach the target
	double x_diff = pos.x() - second_point.x();
	bool left_right_connector = conn1->is_output();
	bool left_right_target = x_diff > 0.0;
	// Note: we can check for 0.0 because x is set to exactly 0.0
	// by go_out_of_safety rect.
	bool direction_ok = x_diff == 0.0 || (left_right_connector == left_right_target);
	QPointF dummy;
	if (direction_ok &&
	    document.operator_list.find_first_in_path(second_point, pos, dummy, nullptr) == nullptr) {
		lines.push_back(second_point);
		lines.push_back(pos);
	} else {
		std::vector<CornerDistance> corner_distances;
		corner_distances = document.operator_list.get_visible_corners(pos);
		path_finder->calculate(corner_distances, pos);
		path_finder->to_lines(lines);
	}
	lines.push_back(conn2->line_from());
	render_lines(lines);

	path_finder->register_view_connections(this);
}

void Edge::recalculate()
{
	unregister_view_connections();

	std::vector<QPointF> lines;
	lines.reserve(10);		// We need at least 3

	QPointF first_point = connector_from->line_from();
	lines.push_back(first_point);

	QPointF pos_to = connector_to->get_safety_pos();

	QPointF target_pos = connector_from->get_safety_pos();
	path_finder = std::make_unique<PathFinder>(connector_from, target_pos, document.operator_list);
	path_finder->calculate(connector_to->connector_desc(), pos_to);
	path_finder->to_lines(lines);
	lines.push_back(connector_to->line_from());

	render_lines(lines);

	path_finder->register_view_connections(this);
}

void Edge::set_complex(bool comp_)
{
	comp = comp_;
	setPen(get_pen(EdgeMode::placed, comp));
}

void Edge::render_lines(const std::vector<QPointF> &lines)
{
	size_t num_points = lines.size();
	assert(num_points >= 2);

	std::vector<double> len(num_points - 1);
	for (size_t i = 0; i < num_points - 1; ++i) {
		QLineF l(lines[i], lines[i + 1]);
		len[i] = l.length();
	}

	std::vector<QPointF> ctrl1(num_points);
	std::vector<QPointF> ctrl2(num_points);
	ctrl2[0] = lines[0];
	ctrl1.back() = lines.back();

	for (size_t i = 1; i < num_points - 1; ++i) {
		QLineF l1(lines[i], lines[i - 1]);
		QLineF l2(lines[i], lines[i + 1]);
		QLineF u1 = l1.unitVector();
		QLineF u2 = l2.unitVector();
		static constexpr double max_f = 20.0;
		double diff_x = (u1.x2() - u2.x2());
		double diff_y = (u1.y2() - u2.y2());
		double scale1 = std::min(len[i - 1] / 2.0, max_f);
		double scale2 = std::min(len[i] / 2.0, max_f);
		QPointF c1(l1.x1()+diff_x*scale1, l1.y1()+diff_y*scale1);
		QPointF c2(l1.x1()-diff_x*scale2, l1.y1()-diff_y*scale2);
		ctrl1[i] = c1;
		ctrl2[i] = c2;
	}

	for (size_t i = 1; i < num_points - 1; ++i) {
		QLineF l1(ctrl1[i - 1], ctrl2[i - 1]);
		QLineF l2(ctrl1[i], ctrl2[i]);
		QPointF intersect;
		if (l1.intersects(l2, &intersect) == QLineF::BoundedIntersection) {
			ctrl2[i-1] = intersect;
			ctrl1[i] = intersect;
		}
	}

	QPainterPath debug_path;
	if (Globals::debug_mode)
		debug_path.moveTo(lines[0]);

	path = QPainterPath();
	path.moveTo(lines[0]);
	for (size_t i = 1; i < num_points; ++i) {
		path.cubicTo(ctrl2[i - 1], ctrl1[i], lines[i]);
		if (Globals::debug_mode) {
			debug_path.lineTo(ctrl2[i - 1]);
			debug_path.lineTo(ctrl1[i]);
		}
	}

	if (Globals::debug_mode) {
		debug_lines = std::make_unique<QGraphicsPathItem>(debug_path);
		scene()->addItem(&*debug_lines);
		debug_lines->setEnabled(false);		// Don't get any mousePressEvents, etc.
		debug_lines->setPen(QPen(Qt::red, 3));	// Use a thick, visible pen
		debug_lines->setZValue(-1.0);		// Draw behind everything
	}

	setPath(path);
}

Edge *Edge::get_and_clear_replace_edge()
{
	return std::exchange(replace_edge, nullptr);
}

bool Edge::attempt_add()
{
	connector_from->set_selected(false);
	if (connector_to)
		connector_to->set_selected(false);
	if (!can_be_placed) {
		unwarn();
		return false;
	}

	// We always connect from output to input
	if (!connector_from->is_output())
		std::swap(connector_from, connector_to);

	unwarn();
	return true;
}

void Edge::add_connection()
{
	assert(connector_to);

	unregister_view_connections();

	// Register edges
	assert(path_finder);
	path_finder->register_view_connections(this);
	path_finder.reset();				// Delete unused path finder

	// Change topological order before actually adding the connection
	document.topo.add_edge(this);

	connector_to->set_input_connection(this);
	connector_from->add_output_connection(this);
	document.topo.update_buffers(connector_to->op(), true);
	document.topo.execute(connector_to->op(), true);

	// Give it placed color
	setPen(get_pen(EdgeMode::placed, comp));
	setOpacity(1.0);
}

void Edge::select()
{
	setPen(get_pen(EdgeMode::selected, comp));
}

void Edge::deselect()
{
	setPen(get_pen(EdgeMode::placed, comp));
}

Operator *Edge::get_operator_from()
{
	return connector_from->op();
}

Operator *Edge::get_operator_to()
{
	return connector_to->op();
}

Connector *Edge::get_connector_from()
{
	return connector_from;
}

Connector *Edge::get_connector_to()
{
	return connector_to;
}

const Connector *Edge::get_connector_to() const
{
	return connector_to;
}

void Edge::remove_temporary()
{
	unwarn();
	connector_from->set_selected(false);
	if (connector_to)
		connector_to->set_selected(false);
}

void Edge::remove()
{
	// Topological order does not change and therefore
	// we don't have to do any work besides removing this edge
	// from the respective connectors
	assert(connector_from);
	assert(connector_to);

	connector_from->remove_output_connection(this);
	connector_to->remove_input_connection(this);

	unregister_view_connections();
	document.topo.update_buffers(connector_to->op(), true);
	document.topo.execute(connector_to->op(), true);

	delete this;
}

void Edge::register_view_connection(ViewConnection *v)
{
	view_connections.push_back(v);
}

void Edge::unregister_view_connections()
{
	for (ViewConnection *v: view_connections)
		v->remove_edge(this);
	view_connections.clear();
}

QJsonObject Edge::to_json() const
{
	QJsonObject res;
	res["op_from"] = static_cast<int>(connector_from->op()->get_topo_id());
	res["op_to"] = static_cast<int>(connector_to->op()->get_topo_id());
	res["conn_from"] = static_cast<int>(connector_from->get_id());
	res["conn_to"] = static_cast<int>(connector_to->get_id());
	return res;
}
