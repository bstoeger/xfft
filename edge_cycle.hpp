// SPDX-License-Identifier: GPL-2.0
// Ihis class is used to keep track of cycles.
// The cycles will be painted in red to warn the user.

#ifndef EDGE_CYCLE_HPP
#define EDGE_CYCLE_HPP

#include <vector>

class Edge;

class EdgeCycle : public std::vector<Edge *> {
public:
	void warn();
	void unwarn();
};

#endif
