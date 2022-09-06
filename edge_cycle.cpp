// SPDX-License-Identifier: GPL-2.0
#include "edge_cycle.hpp"
#include "edge.hpp"

#include <QPen>

static const QPen warn_pen = QPen(Qt::red, 3);
static const QPen placed_pen = QPen(Qt::black, 3);

void EdgeCycle::warn()
{
	// Highlight cycle
	for (Edge *edge: *this) {
		edge->setPen(warn_pen);
	}
}

void EdgeCycle::unwarn()
{
	for (Edge *edge: *this) {
		edge->setPen(placed_pen);
	}
}
