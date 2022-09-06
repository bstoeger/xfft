// SPDX-License-Identifier: GPL-2.0
#include "selection.hpp"
#include "selectable.hpp"
#include "operator.hpp"
#include "edge.hpp"
#include "command.hpp"
#include "document.hpp"

#include <algorithm>

Selection::Selection()
{
}

void Selection::remove_from_selection(const Selectable *s)
{
	auto it = std::find(selection.begin(), selection.end(), s);
	if (it == selection.end())
		return;
	*it = selection.back();
	selection.pop_back();
}

void Selection::deselect_all()
{
	for (Selectable *s: selection)
		s->do_deselect();
	selection.clear();
}

void Selection::deselect(Selectable *s)
{
	auto it = std::find(selection.begin(), selection.end(), s);
	if (it == selection.end())
		return;
	(*it)->do_deselect();
	*it = selection.back();
	selection.pop_back();
}

void Selection::select(Selectable *sel)
{
	bool found = false;
	for (Selectable *s: selection) {
		if (s != sel)
			s->do_deselect();
		else
			found = true;
	}
	selection.clear();
	selection.push_back(sel);
	if (!found)
		sel->do_select(this);
}

bool Selection::is_selected(Selectable *s)
{
	return std::find(selection.begin(), selection.end(), s) != selection.end();
}

void Selection::select_add(Selectable *sel)
{
	if (is_selected(sel))
		return;
	selection.push_back(sel);
	sel->do_select(this);
}

void Selection::remove_all(Document &d, Scene &s)
{
	std::vector<Operator *> operators_to_remove;
	std::vector<Edge *> edges_to_remove;
	for (Selectable *selectable: selection) {
		if (Operator *o = dynamic_cast<Operator *>(selectable))
			operators_to_remove.push_back(o);
		else if (Edge *e = dynamic_cast<Edge *>(selectable))
			edges_to_remove.push_back(e);
	}
	d.place_command<CommandRemoveObjects>(d, s, std::move(operators_to_remove), std::move(edges_to_remove));
	assert(selection.empty()); // Since we removed all items, there must not be any selection left
}

void Selection::clear()
{
	selection.clear();
}

bool Selection::is_empty() const
{
	return selection.empty();
}
