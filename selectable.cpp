// SPDX-License-Identifier: GPL-2.0
#include "selectable.hpp"
#include "selection.hpp"

Selectable::Selectable()
	: selection(nullptr)
{
}

Selectable::~Selectable()
{
	if (selection)
		selection->remove_from_selection(this);
}

void Selectable::remove_from_selection()
{
	if (selection)
		selection->deselect(this);
}

void Selectable::do_select(Selection *s)
{
	selection = s;
	select();
}

void Selectable::do_deselect()
{
	selection = nullptr;
	deselect();
}
