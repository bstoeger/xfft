// SPDX-License-Identifier: GPL-2.0
// This is the virtual base class of all operators.
// An operator object has two modes:
//	add mode: item is in the process of being added.
//	placed mode: item is placed in the canvas.
// The object starts in add mode.
//
// Initialization is performed with two virtual methods:
//	- The constructor. It should only initialize all subobjects.
//	- The init() method. This is called after the operator was added
//	  to the scene, i.e. there is full access to the graphics system.
//	  In this method buttons are added, etc.
//
// Subclasses must provide three static constexprs:
//	static constexpr OperatorId id; -> a unique id used for loading and saving.
//	static constexpr const char *icon; -> the pixmap name of the icon.
//	static constexpr const char *tooltip; -> the tooltip.
#ifndef OPERATOR_HPP
#define OPERATOR_HPP

#include "operator_id.hpp"
#include "selectable.hpp"
#include "connector.hpp"
#include "connector_pos.hpp"
#include "operator_list.hpp"
#include "fft_buf.hpp"
#include "handle_interface.hpp"

#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QSvgRenderer>
#include <QGraphicsSvgItem>
#include <QKeyEvent>
#include <QJsonObject>
#include <QJsonArray>
#include <QMenu>

#include <vector>
#include <array>
#include <functional>

class Document;
class MainWindow;
class Scene;
class TopologicalOrder;
enum class ColorType;

class Operator : public QGraphicsPixmapItem
	       , public Selectable
	       , public HandleInterface
{
public:
	// Class that describes the state of an operator.
	// Many operators will not have a state.
	// Other operators subclass this class.
	class State {
	public:
		virtual ~State();
		virtual std::unique_ptr<State> clone() const = 0;
		virtual QJsonObject to_json() const = 0;
		virtual void from_json(const QJsonObject &) = 0;
	};

	// Template using the CRTP pattern to define the State::clone() function:
	// Pass the subclass as template parameter.
	template <typename Subclass>
	class StateTemplate : public State {
	private:
		std::unique_ptr<State> clone() const;
	};

	// Some operators can start with different states.
	// Each initial state has an icon.
	class InitState {
	public:
		const char *icon;
		const char *name;
		std::unique_ptr<State> state;
	};
	static std::vector<InitState> get_init_states();

	virtual const State &get_state() const = 0;
	State &get_state();
	void place_set_state_command(const QString &text, std::unique_ptr<State> state, bool merge);
	virtual void set_state(const State &) = 0;
	virtual void swap_state(State &) = 0;
	virtual void state_from_json(const QJsonObject &) = 0;
	virtual void state_reset() = 0; // called if state was reset

	void move_event(QPointF mouse_pos); // called by the scene when moving the operator
	void leave_move_mode(bool commit);
	void move_to(QPointF pos);
protected:
	MainWindow &w;
private:
	// Save state while dragging
	std::unique_ptr<State> saved_state;
public:
	// These functions are used when dragging:
	// - save_state() is called on beginning of draging and saves the current state.
	// - restore_state() is called if the user presses ESC or the right mouse button. It restores the saved state.
	// - commit_state() is called if the drag mode is exited. It discards the saved state.
	// restore_state() destroys any saved state.
	// restore_state() and commit_state() have no effect if there is no saved state.
	// Therefore, it is safe to call commit_state() followed by restore_state(). The second call will be a no-op.
	void save_state();
	void restore_state();
	void commit_state();
protected:
	std::vector<Connector *> input_connectors;
	std::vector<Connector *> output_connectors;
	std::vector<FFTBuf> output_buffers;

	// A background rectangle that will only be added if there are buttons.
	// Clicks are routed through to the actual operator.
	class ButtonBackground : public QGraphicsRectItem {
		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
	public:
		ButtonBackground(int height, Operator *o);
	};

	// Where to place buttons
	enum class Side {
		left, right
	};

	// Base class for pixmap-derived buttons. Shows a tooltip on hovering
	class ButtonBase : protected QGraphicsPixmapItem {
		Operator &op;
		QString tooltip;
		void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
		void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
	public:
		ButtonBase(const char *tooltip, Operator *parent);
	};

	// Primitve button class.
	// A pixmap that responds to clicks.
	static constexpr int default_button_height = 16;
	class Button : private ButtonBase {
		std::function<void()> fun;
		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
	public:
		Button(const char *icon_name, const char *tooltip, const std::function<void()> &fun, Side side, Operator *parent);
	};

	// A pixmap that opens a menue. Ownership of the menu is taken.
	// The caller is responsible for equipping the menu with actions that do something.
	class MenuButton : private ButtonBase {
		QMenu menu;
		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
		struct Entry {
			QPixmap pixmap;
			std::function<void()> fun;
		};
		std::vector<Entry> entries;
		void fire_entry(size_t i);
	public:
		MenuButton(Side side, const char *tooltip, Operator *parent);
		void add_entry(const char *pixmap, const QString &text, const std::function<void()> &fun);
		void add_entry(const QPixmap &pixmap, const QString &text, const std::function<void()> &fun);
		void set_pixmap(size_t nr);
		void set_pixmap(ColorType);			// For menu generated by make_color_menu
		void set_pixmap(int brush, bool antialias);	// For menu generated by make_brush_menu
	};

	// Construct a colorwheel changer menu
	static MenuButton *make_color_menu(Operator *op, const std::function<void(ColorType)> &fun, Side side);

	// Construct a brush menu
	static MenuButton *make_brush_menu(Operator *op, const std::function<void(int,bool)> &fun, Side side);

	// As Button, but with text instead of pixmap
	// A pixmap that responds to clicks.
	class TextButton : public QGraphicsTextItem {
		Operator &op;
		QString tooltip;
		std::function<void()> fun;
		void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
		void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
	public:
		TextButton(const char *txt, const char *tooltip, const std::function<void()> &fun, Side side, Operator *parent);
	};

	// Primitive scroller class.
	// Implemented as one rectangle for the background and an additional rectangle for the actual scroller.
	class Scroller : public QGraphicsRectItem {
	protected:
		friend class Operator;
		// The handle
		class Handle : public QGraphicsRectItem
			     , public HandleInterface {
		private:
			void leave_drag_mode(bool commit) override;
			void drag(const QPointF &, Qt::KeyboardModifiers) override;
			Scroller *get_scroller();

			double old_pos;			// Save old position while scrolling
			double click_pos;		// Save position of click
		protected:
			void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
			void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
			void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
			using QGraphicsRectItem::QGraphicsRectItem;
		} *handle;
	private:
		Operator *op;
		double min, max, val;
		bool logarithmic;
		std::function<void(double)> fun;

		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
		static constexpr double ruler_fraction = 0.2;
		static constexpr double handle_width = 8.0;
	protected:
		friend Handle;
		void set_pos(double);
	public:
		static constexpr size_t height = 16;
		Scroller(double min, double max, bool logarithmic, const std::function<void(double)> &fun, Operator *parent);
		void reset(double min, double max, bool logarithmic, double v);
		void set_val(double v);
	};

	// Make the scroller handle friend, so that it can push through mouse press events.
	friend class Scroller::Handle;

	// A simple handle class: an SVG item that exists in a highlighted and non-highlighted
	// state. The state switches on mouse-enter and exit.
	// Note: users must call set_pos(), not setPos().
	class Handle : public QGraphicsSvgItem {
		void hoverEnterEvent(QGraphicsSceneHoverEvent *);
		void hoverLeaveEvent(QGraphicsSceneHoverEvent *);
		Operator &op;
		QPointF offset;
		QString tooltip; // I found Qt's tooltips too intrusive, so make our own.
	public:
		QSvgRenderer &svg;
		QSvgRenderer &svg_highlighted;

		void set_pos(const QPointF &pos);

		Handle(const char *tooltip, Operator *parent);
	};
	friend Handle;

	// Helper functions that change an output buffer type.
	// Returns true if buffer actually was changed.
	// Making a forwarded buffer always returns true,
	// because we don't know if the forwarded buffer changed.
	bool make_output_empty(size_t bufid);
	bool make_output_complex(size_t bufid);
	bool make_output_real(size_t bufid);
	bool make_output_forwarded(size_t bufid, FFTBuf &copy);

	// Call this function if the output buffers were changed outside
	// of a input_connection_changed() chain. Will not, only change
	// downstream buffers.
	void output_buffer_changed();

	// Used for saving.
	virtual OperatorId get_id() const = 0;

	// Update all child objects in the topological order
	void execute_topo();

	size_t get_fft_size() const;

	// Dispatch the calculate<size_t fft_size>() function template of an operator with
	// the correct size as a template argument.
	// This may allow the compiler to generate perfect loops. In the end, it is probably
	// not worth it, because the pre- and postlogue to align memory takes only a fraction
	// of time compared to the main loop. But it also shouldn't hurt.
	template <typename operator_t>
	void dispatch_calculate(operator_t &op);
private:
	// Border around operator.
	// Is drawn thicker if operator is selected.
	QGraphicsRectItem *border;
	static constexpr int border_unselected_thickness = 1;
	static constexpr int border_selected_thickness = 2;
	void set_border(int thickness);

	std::vector<ConnectorPos> connector_pos;
	std::array<OperatorList::view_list,4> corners_view_list;
	// Rectangle + "safety distance".
	// Is only initialized after final placement of this object.
	QRectF safety_rect;

	// Place in topological order list.
	size_t topo_id;
	QGraphicsSimpleTextItem *topo_text;	// For debugging purposes

	void add_connectors(std::vector<Connector *> &array, size_t num, bool output);
	void reset_connector_positions();
	void reset_connector_positions(const std::vector<Connector *> &array, bool output);
	Connector *nearest_connector(std::vector<Connector *> connectors, double y) const;

	// For placing additional buttons.
	int button_offset;
	int button_left_boundary;
	int button_right_boundary;
	int button_height;

	// Get x position of button to be added.
	// Modifies button_left_boundary or button_right_boundary.
	double get_new_button_x(int size, Side side);

	// These two functions are called during dragging of handles
	// drag() transforms the position in operator-local coordinate and calls the virtual function drag_handle();
	// leave_drag_mode() commits or restores state and calls the virtual function restore_handles();
	void leave_drag_mode(bool commit) final;
	void drag(const QPointF &, Qt::KeyboardModifiers) final;

	// Apply function to all view lists
	template <typename Function> // callable of taking a OperatorList::view_list parameter
	void for_all_view_lists(Function f);

	// Edges that have to make a detour owing to this operator
	std::vector<Edge *> get_obstructed_edges();

	Scene &get_scene();
	const Scene &get_scene() const;
	Document &get_document();
	const Document &get_document() const;

	void enter_move_mode(QPointF mouse_pos);
	bool move_started;
	QPointF move_start_pos;
	QPointF move_mouse_start_pos;

	// For moving an operator: remove and readd to view list
	void remove_from_view_list();
	void readd_to_view_list();
protected:
	// This virtual function is called if dragging is stopped
	virtual void restore_handles();
	// This virtual function is called if a handle is dragged
	virtual void drag_handle(const QPointF &, Qt::KeyboardModifiers);

	void enter_drag_mode();

	void add_button(QGraphicsPixmapItem *, Side side);
	void add_button(QGraphicsTextItem *, const char *, Side side);
	void add_scroller(QGraphicsRectItem *);
	void add_button_new_line();
	QGraphicsTextItem *add_text_line();

	// Handle mouse press. Return true if the mouse press was handled, i.e.
	// we should not go into move-mode.
	virtual bool handle_click(QGraphicsSceneMouseEvent *);

	// Handle mouse press.
	void mousePressEvent(QGraphicsSceneMouseEvent *) override final;

	// Size of simple transformation operators
	static constexpr size_t simple_size = 64;

	// Simple transformation operators, which just display a pixmap,
	// call this function in their init() with the pixmap name.
	void init_simple(const char *icon_name);
public:
	enum { Type = UserType + 1 };
	int type() const override { return Type; }

	// Defines the closest distance that connector paths should come
	static constexpr double safety_distance = 10.0;

	Operator(MainWindow &m);
	virtual ~Operator();
	virtual void init();

	// Virtual function that is called when the operator was placed on the scene
	virtual void placed();

	void prepare_init();
	void finish_init();
	void enter_placed_mode();

	// Locate corners of safety rectangle visible from a point
	// (ignore itermediate obstacles)
	// Returns a bit field with the following bit ids:
	// 2   1
	//  +-+
	//  | |
	//  +-+
	// 3   0
	// Returns 0 if pos is inside the safety rectangle
	int visible_corners(const QPointF &pos) const;
	QPointF corner_coord(int corner) const;
	QRectF get_safety_rect() const;
	void update_safety_rect();

	// Safety rect with double safety distance in local coordinates.
	// Used while placing the operator to show the area which
	// has to be free of other operators.
	QRectF get_double_safety_rect() const;

	// Go to the right or left of security rectangle (+1.0)
	QPointF go_out_of_safety_rect(const QPointF &pos) const;

	virtual size_t num_input() const = 0;
	virtual size_t num_output() const = 0;
	const double *get_output(size_t id) const;

	// Called if an input connection changed
	virtual bool input_connection_changed() = 0;

	// Execute this operator
	virtual void execute() = 0;

	Connector *nearest_connector(const QPointF &pos) const;
	const std::vector<ConnectorPos> &get_connector_pos() const;
	Connector &get_input_connector(size_t id);
	Connector &get_output_connector(size_t id);
	const Connector &get_input_connector(size_t id) const;
	const Connector &get_output_connector(size_t id) const;
	FFTBuf &get_output_buffer(size_t id);
	const FFTBuf &get_output_buffer(size_t id) const;

	// Get all edges - input as well as output
	std::vector<Edge *> get_edges();

	// Register a free view to another connector
	void add_view_connection(ConnectorType type, const OperatorList::view_iterator &it);
	void remove_view_connection(ConnectorType type, const ViewConnection *conn);

	const OperatorList::view_list &get_view_list(ConnectorType type) const;
	OperatorList::view_list &get_view_list(ConnectorType type);

	size_t get_topo_id() const;
	void set_topo_id(size_t id);

	void select() override;
	void deselect() override;

	// Add to scene and topological order.
	void add_to_scene();

	// Remove operator from scene, but don't delete operator.
	// Two versions: for already placed and not-yet placed operators.
	void remove_placed_from_scene();
	void remove_unplaced_from_scene();

	// Recalculate the position of the edges when moving the operator.
	void recalculate_edges();

	// Remove this operator: remove all edges and recalculate view positions.
	void remove() override;

	// Remove all edges of operator. Used when clearing the scene.
	void remove_edges();

	// Append edges of output iterators to JSON array. Used for saving.
	void out_edges_to_json(QJsonArray &) const;

	// Used for saving.
	QJsonObject to_json() const;

	// Construct an operator from a JSON description.
	static Operator *from_json(MainWindow &w, const QJsonObject &desc);

	// Call this function to select operator
	void clicked(QGraphicsSceneMouseEvent *event);
};

template <typename Subclass>
std::unique_ptr<Operator::State> Operator::StateTemplate<Subclass>::clone() const
{
	return std::unique_ptr<Operator::State>(new Subclass(*(Subclass *)this));
}

// Template that defines some virtual functions, such as state-handling
// and the number of connectors.
template <OperatorId Id, typename StateType, size_t NumInput, size_t NumOutput>
class OperatorTemplate : public Operator {
	OperatorId get_id() const override final;
protected:
	std::unique_ptr<StateType> clone_state() const;
	const State &get_state() const override final;
	void set_state(const State &) override final;
	void swap_state(State &) override final;
	void state_from_json(const QJsonObject &) override final;
	size_t num_input() const override final;
	size_t num_output() const override final;
	StateType state;
	using Operator::Operator;

	// If there are no input connections, disable execution of input_connection_changed() and execute()
	bool input_connection_changed() override {
		if constexpr (NumInput == 0)
			assert(false);
		return false;
	}
	void execute() override {
		if constexpr (NumInput == 0)
			assert(false);
	}
public:
	inline static constexpr OperatorId id = Id;
};

template <OperatorId Id, typename StateType, size_t NumInput, size_t NumOutput>
std::unique_ptr<StateType> OperatorTemplate<Id, StateType, NumInput, NumOutput>::clone_state() const
{
	return std::unique_ptr<StateType>(new StateType(state));
}

template <OperatorId Id, typename StateType, size_t NumInput, size_t NumOutput>
OperatorId OperatorTemplate<Id, StateType, NumInput, NumOutput>::get_id() const
{
	return Id;
}

template <OperatorId Id, typename StateType, size_t NumInput, size_t NumOutput>
const Operator::State &OperatorTemplate<Id, StateType, NumInput, NumOutput>::get_state() const
{
	return state;
}

template <OperatorId Id, typename StateType, size_t NumInput, size_t NumOutput>
void OperatorTemplate<Id, StateType, NumInput, NumOutput>::state_from_json(const QJsonObject &json)
{
	static_cast<Operator::State &>(state).from_json(json);
}

template <OperatorId Id, typename StateType, size_t NumInput, size_t NumOutput>
void OperatorTemplate<Id, StateType, NumInput, NumOutput>::set_state(const Operator::State &state_)
{
	state = dynamic_cast<const StateType &>(state_);
}

template <OperatorId Id, typename StateType, size_t NumInput, size_t NumOutput>
void OperatorTemplate<Id, StateType, NumInput, NumOutput>::swap_state(Operator::State &state_)
{
	std::swap(state, dynamic_cast<StateType &>(state_));
}

template <OperatorId Id, typename StateType, size_t NumInput, size_t NumOutput>
size_t OperatorTemplate<Id, StateType, NumInput, NumOutput>::num_input() const
{
	return NumInput;
}

template <OperatorId Id, typename StateType, size_t NumInput, size_t NumOutput>
size_t OperatorTemplate<Id, StateType, NumInput, NumOutput>::num_output() const
{
	return NumOutput;
}

// Dummy class for those operators that don't have a state
class OperatorStateNone : public Operator::StateTemplate<OperatorStateNone> {
	virtual QJsonObject to_json() const override;
	virtual void from_json(const QJsonObject &) override;
};

// Operator that has no state
template <OperatorId Id, size_t NumInput, size_t NumOutput>
class OperatorNoState : public OperatorTemplate<Id, OperatorStateNone, NumInput, NumOutput> {
	void state_reset() override final;
protected:
	using OperatorTemplate<Id, OperatorStateNone, NumInput, NumOutput>::OperatorTemplate;
};

template <OperatorId Id, size_t NumInput, size_t NumOutput>
void OperatorNoState<Id, NumInput, NumOutput>::state_reset()
{
}

#include "operator_impl.hpp"

#endif
