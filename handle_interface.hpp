// SPDX-License-Identifier: GPL-2.0
// "Interface" class for objects that support dragging
#ifndef HANDLE_INTERFACE_HPP
#define HANDLE_INTERFACE_HPP

#include <QPointF>
#include <QKeyEvent>

// Called for operators in drag mode
class HandleInterface {
public:
	virtual void drag(const QPointF &, Qt::KeyboardModifiers) = 0;
	virtual void leave_drag_mode(bool commit) = 0;
};

#endif
