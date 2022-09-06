// SPDX-License-Identifier: GPL-2.0
// The operators are nodes in a directed acyclic multigraph.
// This class keeps them sorted in topological order

#ifndef TOPOLOGICAL_ORDER_HPP
#define TOPOLOGICAL_ORDER_HPP

#include "edge_cycle.hpp"

#include <cstddef>		// For size_t

class Operator;
class Edge;

class TopologicalOrder {
	std::vector<Operator *> ops;

	std::vector<int> get_end_reachable_from(size_t begin, size_t end, size_t &num);
	std::vector<int> get_reachable_from_begin(size_t begin, size_t end, size_t &num);

	void for_all_children(void (*func)(Operator *));
public:
	// Add/remove edges and operators. There is no need for
	// a remove_edge() call, because topological order stays
	// unchanged.
	void add_operator(Operator *);
	void add_edge(Edge *);
	void remove_operator(Operator *op);

	// To prevent formation of cycles we check whether one
	// element is reachable from the other.
	// Since it is a cheap operation in a topologically ordered graph,
	// find the shortest connection.
	EdgeCycle find_connection(const Operator *from, const Operator *to) const;

	// After adding an edge, update buffers of this operator and children
	// If the second argument is false, the passed-in operator will not be updated
	void update_buffers(Operator *op, bool update_first) const;

	// Execute operator and children
	// If the second argument is false, the input operator will not be executed
	void execute(Operator *op, bool update_first) const;

	// Delete all entries
	void clear();

	// Get list to operators in topological order.
	// Used for saving.
	const std::vector<Operator *> &get_operators() const;

	// Used for loading. Returns nullptr if id is invalid
	Operator *get_by_id(size_t id);

	// After loading: update all buffers and execute all children
	void update_all_buffers();
	void execute_all();
};

#endif
