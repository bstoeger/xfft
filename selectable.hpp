// SPDX-License-Identifier: GPL-2.0
// Classes that derive from the Selectable class can be selected by the
// user. They must implement an interface:
//	select(): called if the user selects the object.
//	deselect(): called if the user delesct the object.
//	remove(): called if the user presses "del" while object is selected.
// The destructor takes care of removing the object from the selection list.

#ifndef SELECTABLE_HPP
#define SELECTABLE_HPP

class Selection;

class Selectable {
	Selection *selection;		// Points to selection or nullptr if unselected.

	// These two method are called by the Selection class.
	// They do the bookkeeping and then call the virtual methods below.
	friend class Selection;
	void do_select(Selection *s);
	void do_deselect();
public:
	Selectable();
	virtual ~Selectable();
protected:
	virtual void select() = 0;
	virtual void deselect() = 0;
	virtual void remove() = 0;	// TODO: remove
	void remove_from_selection();
};

#endif
