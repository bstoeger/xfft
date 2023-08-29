// SPDX-License-Identifier: GPL-2.0
#include "topological_order.hpp"
#include "edge.hpp"
#include "operator.hpp"

#include <cassert>

void TopologicalOrder::add_operator(Operator *o)
{
	size_t id = ops.size();
	ops.push_back(o);
	o->set_topo_id(id);
}

// Get all elements in range from which the end of the range can be reached
// For such an element the entry in the return vector is set to 1 (otherwise to zero)
// The number of elements is returned in the last argument
std::vector<int> TopologicalOrder::get_end_reachable_from(size_t begin, size_t end, size_t &num)
{
	size_t size = end - begin;
	std::vector<int> items(size, 0);
	std::vector<size_t> stack;
	stack.reserve(size);
	stack.push_back(end - 1);
	items[end - 1 - begin] = 1;
	num = 1;

	while (!stack.empty()) {
		Operator *op = ops[stack.back()];
		stack.pop_back();

		size_t num_input = op->num_input();
		for (size_t i = 0; i < num_input; ++i) {
			const Connector &conn = op->get_input_connector(i);
			const Connector *parent = conn.get_parent();
			if (!parent)
				continue;

			size_t id = parent->op()->get_topo_id();
			// Skip over elements outside of range
			assert(id < end);
			if (id < begin)
				continue;
			// Skip already visited items
			if (items[id - begin] != 0)
				continue;

			// Mark used item and push on stack
			items[id - begin] = 1;
			stack.push_back(id);
			++num;
		}
	}
	return items;
}

// Get all elements reachable from the begin of the range
// For such an element the entry in the return vector is set to 1 (otherwise to zero)
// The number of elements is returned in the last argument
std::vector<int> TopologicalOrder::get_reachable_from_begin(size_t begin, size_t end, size_t &num)
{
	size_t size = end - begin;
	std::vector<int> items(size, 0);
	std::vector<size_t> stack;
	stack.reserve(size);
	stack.push_back(begin);
	items[0] = 1;
	num = 1;

	while (!stack.empty()) {
		Operator *op = ops[stack.back()];
		stack.pop_back();

		size_t num_output = op->num_output();
		for (size_t i = 0; i < num_output; ++i) {
			const Connector &conn = op->get_output_connector(i);
			const std::vector<Connector *> children = conn.get_children();
			for (const Connector *child: children) {
				size_t id = child->op()->get_topo_id();
				// Skip over elements outside of range
				assert(id > begin);
				if (id >= end)
					continue;
				// Skip already visited items
				if (items[id - begin] != 0)
					continue;

				// Mark used item and push on stack
				items[id - begin] = 1;
				stack.push_back(id);
				++num;
			}
		}
	}
	return items;
}

// Adding an edge while keeping topological order is surprisingly simple
// [see D. J. Pearce and P. H. J. Kelly, ACM J. Exper. Algorithmics, 11, 1-24 (2006)]:
// Only the "affected range" (elements between both connected elements)
// has to be considered. In this range find all children of the new lower element
// and all parents of the new higher element. And rearrange these elements by
// making the parents higher as the children, while preserving the topological
// order of the parents and children.
void TopologicalOrder::add_edge(Edge *e)
{
	Operator *from = e->get_operator_from();
	Operator *to = e->get_operator_to();
	size_t id_from = from->get_topo_id();
	size_t id_to = to->get_topo_id();

	// If the elements are already in topological order, quit right away.
	assert(id_from != id_to);
	if (id_from < id_to) {
		return;
	}

	size_t affected_range_begin = id_to;
	size_t affected_range_end = id_from + 1;
	size_t affected_range_size = affected_range_end - affected_range_begin;

	// Collect parent and child elements
	size_t num_parents;
	size_t num_children;
	std::vector<int> parents = get_end_reachable_from(affected_range_begin, affected_range_end, num_parents);
	std::vector<int> children = get_reachable_from_begin(affected_range_begin, affected_range_end, num_children);

	size_t num_reorder = num_parents + num_children;
	assert(num_reorder <= affected_range_size);
	std::vector<size_t> reorder_ids;
	std::vector<Operator *> reorder_vals;
	reorder_ids.reserve(num_reorder);
	reorder_vals.reserve(num_reorder);

	for (size_t i = 0; i < affected_range_size; ++i) {
		assert(!parents[i] || !children[i]);
		if (parents[i] || children[i])
			reorder_ids.push_back(affected_range_begin + i);
	}
	for (size_t i = 0; i < affected_range_size; ++i) {
		if (parents[i])
			reorder_vals.push_back(ops[affected_range_begin + i]);
	}
	for (size_t i = 0; i < affected_range_size; ++i) {
		if (children[i])
			reorder_vals.push_back(ops[affected_range_begin + i]);
	}
	assert(reorder_ids.size() == num_reorder && reorder_vals.size() == num_reorder);
	for (size_t i = 0; i < num_reorder; ++i) {
		size_t id = reorder_ids[i];
		Operator *op = reorder_vals[i];
		ops[id] = op;
		op->set_topo_id(id);
	}
}

void TopologicalOrder::remove_operator(Operator *o)
{
	assert(o);
	size_t id = o->get_topo_id();
	assert(ops[id] == o);
	ops.erase(ops.begin() + id);

	for (size_t num_elements = ops.size(); id < num_elements; ++id)
		ops[id]->set_topo_id(id);
}

EdgeCycle TopologicalOrder::find_connection(const Operator *from, const Operator *to) const
{
	EdgeCycle res;
	size_t id_from = from->get_topo_id();
	size_t id_to = to->get_topo_id();

	if (id_from > id_to)
		return res;

	size_t affected_range_size = id_to - id_from + 1;
	std::vector<size_t> path_lengths(affected_range_size, std::string::npos);
	std::vector<size_t> path_parent(affected_range_size, std::string::npos);
	std::vector<Edge *> path_edges(affected_range_size, nullptr);
	path_lengths[0] = 0;
	for (size_t act_id = id_from; act_id < id_to; ++act_id) {
		// If we cant reach this node -> no reason to bother with it!
		size_t shortest = path_lengths[act_id - id_from];
		if (shortest == std::string::npos)
			continue;
		size_t next_length = shortest + 1;

		// Iterate over all outgoing edges
		Operator *op = ops[act_id];
		size_t num_output = op->num_output();
		for (size_t i = 0; i < num_output; ++i) {
			const Connector &conn = op->get_output_connector(i);
			const std::vector<Edge *> &children = conn.get_children_edges();
			for (Edge *child: children) {
				size_t id = child->get_connector_to()->op()->get_topo_id();
				assert(id > act_id);
				if (id > id_to) {
					continue;
				}
				if (path_lengths[id - id_from] <= next_length) {
					continue;
				}
				path_lengths[id - id_from] = next_length;
				path_parent[id - id_from] = act_id;
				path_edges[id - id_from] = child;
			}
		}
	}

	size_t length = path_lengths[affected_range_size - 1];
	if (length == std::string::npos)
		return res;

	res.reserve(length);
	for (size_t act_id = id_to; act_id != id_from; act_id = path_parent[act_id - id_from])
		res.push_back(path_edges[act_id - id_from]);
	assert(res.size() == length);

	return res;
}

static void mark_children(const Operator *op, std::vector<int> &update, size_t id_from, size_t id_to, size_t act_id)
{
	size_t num_output = op->num_output();
	for (size_t i = 0; i < num_output; ++i) {
		const Connector &conn = op->get_output_connector(i);
		bool is_complex = conn.is_complex_buffer();
		const std::vector<Edge *> &children = conn.get_children_edges();
		for (Edge *child: children) {
			child->set_complex(is_complex);
			size_t id = child->get_connector_to()->op()->get_topo_id();
			assert(id > act_id && id < id_to);
			update[id - id_from] = 1;
		}
	}
}

void TopologicalOrder::update_buffers(Operator *op, bool update_first) const
{
	size_t id_from = op->get_topo_id();
	size_t id_to = ops.size();
	size_t range_size = id_to - id_from;

	std::vector<int> update(range_size, 0);
	update[0] = 1;

	for (size_t act_id = id_from; act_id < id_to; ++act_id) {
		if (!update[act_id - id_from])
			continue;
		Operator *op = ops[act_id];
		if (update_first || act_id !=id_from)
			if (!op->input_connection_changed())
				continue;

		mark_children(op, update, id_from, id_to, act_id);
	}
}

void TopologicalOrder::execute(Operator *op, bool update_first) const
{
	size_t id_from = op->get_topo_id();
	size_t id_to = ops.size();
	size_t range_size = id_to - id_from;

	std::vector<int> update(range_size, 0);
	update[0] = 1;

	for (size_t act_id = id_from; act_id < id_to; ++act_id) {
		if (!update[act_id - id_from])
			continue;
		Operator *op = ops[act_id];
		if (update_first || act_id !=id_from)
			op->execute();

		mark_children(op, update, id_from, id_to, act_id);
	}
}

void TopologicalOrder::for_all_children(void (*func)(Operator *))
{
	size_t size = ops.size();
	std::vector<int> update(size, 0);

	for (size_t act_id = 0; act_id < size; ++act_id) {
		Operator *op = ops[act_id];
		if (update[act_id])
			func(op);

		mark_children(op, update, 0, size, act_id);
	}
}

void TopologicalOrder::update_all_buffers()
{
	for_all_children([](Operator *op) { op->input_connection_changed(); });
}

void TopologicalOrder::execute_all()
{
	for_all_children([](Operator *op) { op->execute(); });
}

void TopologicalOrder::clear()
{
	ops.clear();
}

const std::vector<Operator *> &TopologicalOrder::get_operators() const
{
	return ops;
}

Operator *TopologicalOrder::get_by_id(size_t id)
{
	return id > ops.size() ? nullptr : ops[id];
}
