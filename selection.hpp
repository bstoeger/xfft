// SPDX-License-Identifier: GPL-2.0
// Class that keeps track of all selected items.

#ifndef SELECTION_HPP
#define SELECTION_HPP

#include <vector>

class Selectable;
class Document;
class Scene;

class Selection {
	std::vector<Selectable *> selection;
protected:
	friend class Selectable;

	// Called by selectables in destructor -> remove from list
	void remove_from_selection(const Selectable *);
public:
	Selection();

	bool is_selected(Selectable *);			// Returns true if object is among selected.
	void deselect_all();				// Deselect all objects.
	void deselect(Selectable *);			// Deselect single objects.
	void select(Selectable *);			// Select single item and deselect all others.
	void select_add(Selectable *);			// Select single item and leave others selected.
	void remove_all(Document &d, Scene &s);		// Removes all selected objects via undo command.
	void clear();					// Clears selection list without calling anything.
	bool is_empty() const;
};

#endif
