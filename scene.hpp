// SPDX-License-Identifier: GPL-2.0
#ifndef SCENE_HPP
#define SCENE_HPP

#include "handle_interface.hpp"
#include "operator_adder.hpp"
#include "selection.hpp"

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsView>

#include <memory>

class Connector;
class Edge;
class Magnifier;
class MainWindow;
class Operator;
class Selectable;

class Scene : public QGraphicsScene
{
	Q_OBJECT
private:
	MainWindow &w;
	enum class Mode {
		normal,
		add_object,
		connect,
		drag,
		move,
		magnify
	} mode;
	Selection selection;

	// Defined if we are in add mode
	std::unique_ptr<OperatorAdder> operator_adder;

	// Defined if we are in connect mode
	Edge *edge;

	// Defined if we are in magnify mode
	Magnifier *magnifier;

	// Defined if we are in drag mode
	HandleInterface *handle_drag;

	// Defined if we are in move mode
	Operator *operator_move;

	// Mode changes
	void exit_mode();

	void place_edge();
	void delete_temporary_edge();
	void delete_magnifier();

	void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
	void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
signals:
	void selection_changed(bool empty_selection);
public:
	Scene(MainWindow &w, QObject *parent);
	~Scene();

	// When the canvas is cleared, clear all pointers to objects.
	void clear();

	QGraphicsView *get_view();
	const QGraphicsView *get_view() const;
	QPoint get_scroll_position() const;
	void set_scroll_position(const QPoint &) const;
	void set_cursor(Qt::CursorShape shape);

	// Mode changes
	void enter_add_object_mode(std::unique_ptr<Operator> &&);
	void enter_magnifier_mode();
	void enter_normal_mode();
	void enter_drag_mode(HandleInterface *);
	void enter_move_mode(Operator *);

	// Events
	void connector_clicked(Connector *);
	void selectable_clicked(Selectable *, QGraphicsSceneMouseEvent *);
	void delete_selection();

	// Get connector at location or, if location is on item, the closest connector of this item.
	// If location is neither above connector nor item, return nullptr
	Connector *connector_at(const QPointF &pos);
};

#endif
