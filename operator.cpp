// SPDX-License-Identifier: GPL-2.0
#include "operator.hpp"
#include "operator_factory.hpp"
#include "color.hpp"
#include "command.hpp"
#include "document.hpp"
#include "edge.hpp"
#include "globals.hpp"
#include "mainwindow.hpp"
#include "scene.hpp"
#include "svg_cache.hpp"
#include "topological_order.hpp"

#include <QAction>
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QStyle>

#include <cassert>

Operator::Operator(MainWindow &w_)
	: QGraphicsPixmapItem()
	, w(w_)
	, topo_id(0)
	, topo_text(nullptr)
	, button_offset(0)
	, button_left_boundary(0)
	, button_right_boundary(0)
	, button_height(0)
{
	// Prioritize mouseclicks over those of edges
	setZValue(1.0);
}

Operator::~Operator()
{
}

Operator::ButtonBackground::ButtonBackground(int height, Operator *o)
	: QGraphicsRectItem(0.0, -height, o->boundingRect().width(), height, o)
{
}

void Operator::ButtonBackground::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	Operator *o = dynamic_cast<Operator *>(parentItem());
	o->clicked(event);
}

static QPixmap name_to_pixmap(const char *name, int size)
{
	return QIcon(name).pixmap(QSize(size, size));
}

Operator::ButtonBase::ButtonBase(const char *tooltip, Operator *parent)
	: QGraphicsPixmapItem(parent)
	, op(*parent)
	, tooltip(tooltip)
{
	setAcceptHoverEvents(true);
}

void Operator::ButtonBase::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
	op.w.show_tooltip(tooltip);
}

void Operator::ButtonBase::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
	op.w.hide_tooltip();
}

Operator::Button::Button(const char *icon_name, const char *tooltip,
			 const std::function<void()> &fun_, Side side, Operator *parent)
	: ButtonBase(tooltip, parent)
	, fun(fun_)
{
	setPixmap(name_to_pixmap(icon_name, default_button_height));
	parent->add_button(this, side);
}

void Operator::Button::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		fun();
		// Select operator
		Operator *o = dynamic_cast<Operator *>(parentItem());
		o->clicked(event);
	} else {
		QGraphicsPixmapItem::mousePressEvent(event);
	}
}

Operator::MenuButton::MenuButton(Side side, const char *tooltip, Operator *parent)
	: ButtonBase(tooltip, parent)
{
	QPixmap p(default_button_height, default_button_height);
	p.fill(Qt::black);
	setPixmap(p);
	entries.reserve(10);
	parent->add_button(this, side);
}

void Operator::MenuButton::add_entry(const QPixmap &pixmap, const QString &text, const std::function<void()> &fun)
{
	// Default to first entry
	size_t num = entries.size();
	if (num == 0)
		setPixmap(pixmap);
	entries.push_back({pixmap, fun});
	menu.addAction(pixmap, text, [this,num](){fire_entry(num);});
}

void Operator::MenuButton::add_entry(const char *pixmap, const QString &text, const std::function<void()> &fun)
{
	add_entry(name_to_pixmap(pixmap, default_button_height), text, fun);
}

void Operator::MenuButton::fire_entry(size_t nr)
{
	if (nr >= entries.size())
		return;
	setPixmap(entries[nr].pixmap);
	entries[nr].fun();
}

void Operator::MenuButton::set_pixmap(size_t nr)
{
	if (nr >= entries.size())
		return;
	setPixmap(entries[nr].pixmap);
}

void Operator::MenuButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		menu.exec(event->screenPos());

		// Select operator
		Operator *o = dynamic_cast<Operator *>(parentItem());
		o->clicked(event);
	} else {
		QGraphicsPixmapItem::mousePressEvent(event);
	}

}

Operator::MenuButton *Operator::make_color_menu(Operator *op, const std::function<void(ColorType)> &fun, Side side)
{
	auto menu = new MenuButton(side, "Set color mode", op);
	menu->add_entry(get_color_pixmap(ColorType::RW, default_button_height, true), "Red/Black/White", [fun]() { fun(ColorType::RW); });
	menu->add_entry(get_color_pixmap(ColorType::HSV, default_button_height, true), "Hue/Lightness, saturated", [fun]() { fun(ColorType::HSV); });
	menu->add_entry(get_color_pixmap(ColorType::HSV_WHITE, default_button_height, true), "Hue/Lightness, white", [fun]() { fun(ColorType::HSV_WHITE); });
	return menu;
}

void Operator::MenuButton::set_pixmap(ColorType c)
{
	switch (c) {
	case ColorType::RW:		return set_pixmap(0);
	case ColorType::HSV:		return set_pixmap(1);
	case ColorType::HSV_WHITE:	return set_pixmap(2);
	default:			return;
	}
}

static QPixmap make_brush_icon(int size, bool antialias)
{
	int icon_size = qApp->style()->pixelMetric(QStyle::PM_SmallIconSize);
	QImage img(icon_size, icon_size, QImage::Format_Grayscale8);
	img.fill(0);
	QPainter painter;
	painter.begin(&img);
	painter.setPen(QPen(Qt::white, size, Qt::SolidLine, Qt::RoundCap));
	painter.setRenderHint(QPainter::Antialiasing, antialias);
	painter.drawPoint(icon_size / 2, icon_size / 2);
	painter.end();
	return QPixmap::fromImage(img);
}

Operator::MenuButton *Operator::make_brush_menu(Operator *op, const std::function<void(int,bool)> &fun, Side side)
{
	auto menu = new MenuButton(side, "Set brush", op);
	static constexpr int min_size = 1, max_size = 13;
	for (int i = min_size; i <= max_size; ++i)
		menu->add_entry(make_brush_icon(i, false), QString("%1 Pixel").arg(i), [fun,i]() { fun(i, false); });
	for (int i = min_size; i <= max_size; ++i)
		menu->add_entry(make_brush_icon(i, true), QString("%1 Pixel, antialiased").arg(i), [fun,i]() { fun(i, true); });
	return menu;
}

void Operator::MenuButton::set_pixmap(int brush, bool antialias)
{
	if (brush < 1 || brush > 13)
		return;
	set_pixmap(brush - 1 + (antialias ? 13 : 0));
}

Operator::TextButton::TextButton(const char *text, const char *tooltip, const std::function<void()> &fun_, Side side, Operator *parent)
	: QGraphicsTextItem(text, parent)
	, op(*parent)
	, tooltip(tooltip)
	, fun(fun_)
{
	QFont font("Times", 10);
	setFont(font);
	parent->add_button(this, text, side);
	setAcceptHoverEvents(true);
}

void Operator::TextButton::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
	op.w.show_tooltip(tooltip);
}

void Operator::TextButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
	op.w.hide_tooltip();
}

void Operator::TextButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		fun();
		// Select operator
		Operator *o = dynamic_cast<Operator *>(parentItem());
		o->clicked(event);
	} else {
		QGraphicsTextItem::mousePressEvent(event);
	}

}

static QBrush scroller_handle_standard_brush(Qt::gray);
static QBrush scroller_handle_highlighted_brush(Qt::red);

Operator::Scroller::Scroller(double min_, double max_, bool logarithmic_, const std::function<void(double)> &fun_, Operator *parent)
	: QGraphicsRectItem(parent)
	, op(parent)
	, min(min_)
	, max(max_)
	, val(min_)
	, logarithmic(logarithmic_)
	, fun(fun_)
{
	parent->add_scroller(this);
	setPen(Qt::NoPen);
	setBrush(Qt::white);

	// Add ruler
	// TODO: The coordinates apparently are relative to the Operator field,
	// not to this scroller? Why?
	{
		double ruler_height = height * ruler_fraction;
		double ruler_top = (height - ruler_height) / 2.0;
		QRectF ruler_rect = rect();
		ruler_rect.setY(ruler_rect.y() + ruler_top);
		ruler_rect.setHeight(ruler_height);
		QGraphicsRectItem *ruler = new QGraphicsRectItem(ruler_rect, this);
		ruler->setPen(Qt::NoPen);
		ruler->setBrush(Qt::black);
	}

	// Add scroll handle
	{
		QRectF handle_rect = rect();
		handle_rect.setWidth(handle_width);
		handle = new Handle(handle_rect, this);
		handle->setPen(QPen(Qt::black, 1));
		handle->setBrush(scroller_handle_standard_brush);
		handle->setAcceptHoverEvents(true);
		handle->setAcceptTouchEvents(true);
	}
}

void Operator::Scroller::reset(double min_, double max_, bool logarithmic_, double v)
{
	min = min_;
	max = max_;
	logarithmic = logarithmic_;
	set_val(v);
}

void Operator::Scroller::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->buttons() & Qt::LeftButton) {
		double pos = mapFromScene(event->scenePos()).x()  - rect().x();
		double old_pos = handle->rect().x() - rect().x();
		pos = pos > old_pos ?
			old_pos + handle_width :
			old_pos - handle_width;
		set_pos(pos);

		// Select operator
		Operator *o = dynamic_cast<Operator *>(parentItem());
		o->clicked(event);
	} else {
		QGraphicsRectItem::mousePressEvent(event);
	}
}

void Operator::Scroller::set_pos(double pos)
{
	double max_pos = rect().width() - handle_width;
	if (pos < 0.0)
		pos = 0.0;
	if (pos > max_pos)
		pos = max_pos;

	QRectF r = handle->rect();
	r.moveLeft(pos + rect().x());
	handle->setRect(r);

	double rel_pos = pos / max_pos;
	val = logarithmic ?
		min * exp(log(max / min) * rel_pos) :
		min + (max - min) * rel_pos;

	fun(val);
}

void Operator::Scroller::set_val(double val)
{
	val = std::clamp(val, min, max);

	double rel_pos = logarithmic ?
		log(val / min) / log(max / min) :
		(val - min) / (max - min);

	double max_pos = rect().width() - handle_width;
	double pos = rel_pos * max_pos;

	QRectF r = handle->rect();
	r.moveLeft(pos + rect().x());
	handle->setRect(r);
}

void Operator::Scroller::Handle::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
	setBrush(scroller_handle_highlighted_brush);
}

void Operator::Scroller::Handle::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
	setBrush(scroller_handle_standard_brush);
}

void Operator::Scroller::Handle::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (!(event->buttons() & Qt::LeftButton)) {
		QGraphicsRectItem::mousePressEvent(event);
		return;
	}

	click_pos = event->scenePos().x();
	old_pos = rect().x();

	Operator *o = dynamic_cast<Operator *>(parentItem()->parentItem());
	o->get_scene().enter_drag_mode(this);

	// Select operator
	o->clicked(event);
}

void Operator::Scroller::Handle::leave_drag_mode(bool commit)
{
	if (!commit)
		get_scroller()->set_pos(old_pos);
	get_scroller()->op->restore_handles();
}

void Operator::Scroller::Handle::drag(const QPointF &pos, Qt::KeyboardModifiers)
{
	Scroller *s = get_scroller();
	double new_pos = pos.x() - s->rect().x() - click_pos + old_pos;
	s->set_pos(new_pos);
}

Operator::Scroller *Operator::Scroller::Handle::get_scroller()
{
	return static_cast<Scroller *>(parentItem());
}

Operator::Handle::Handle(const char *tooltip_, Operator *parent)
	: QGraphicsSvgItem(parent)
	, op(*parent)
	, tooltip(tooltip_)
	, svg(svg_cache.get(SVGCache::Id::move))
	, svg_highlighted(svg_cache.get_highlighted(SVGCache::Id::move))
{

	static constexpr double size = 16.0;

	setSharedRenderer(&svg);
	QSizeF rect_size = boundingRect().size();
	double scale = size / std::max(rect_size.width(), rect_size.height());

	QSizeF act_size = rect_size * scale;
	offset = QPointF{ act_size.width() / 2.0, act_size.height() / 2.0 };
	setScale(scale);

	setAcceptHoverEvents(true);
	setAcceptTouchEvents(true);
}

void Operator::Handle::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
	op.w.show_tooltip(tooltip);
	setSharedRenderer(&svg_highlighted);
}

void Operator::Handle::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
	op.w.hide_tooltip();
	setSharedRenderer(&svg);
}

void Operator::Handle::set_pos(const QPointF &pos)
{
	setPos(pos - offset);
}

void Operator::init()
{
}

void Operator::placed()
{
}

void Operator::clicked(QGraphicsSceneMouseEvent *event)
{
	get_scene().selectable_clicked(this, event);
}

bool Operator::handle_click(QGraphicsSceneMouseEvent *)
{
	return false;
}

void Operator::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		clicked(event);
		if (!handle_click(event))
			enter_move_mode(event->scenePos());
	} else {
		if (!handle_click(event))
			QGraphicsPixmapItem::mousePressEvent(event);
	}
}

void Operator::prepare_init()
{
}

void Operator::finish_init()
{
	int total_button_height = button_height + button_offset;
	if (total_button_height > 0) {
		ButtonBackground *button_background = new ButtonBackground(total_button_height, this);
		button_background->setPen(Qt::NoPen);
		button_background->setBrush(Qt::white);
		button_background->setZValue(-2.0);
	}

	border = new QGraphicsRectItem(this);
	deselect();		// Paint normal border

	// Add connectors
	connector_pos.reserve(num_input() + num_output() + 4);
	add_connectors(input_connectors, num_input(), false);
	add_connectors(output_connectors, num_output(), true);

	// Initialize output buffers to empty buffer
	output_buffers.resize(num_output());
}

void Operator::add_to_scene()
{
	// Add to topological order
	get_document().topo.add_operator(this);

	// Add to scene
	get_scene().addItem(this);
}

void Operator::add_connectors(std::vector<Connector *> &array, size_t num, bool output)
{
	array.reserve(num);

	QRectF bounding_rect = boundingRect();

	double step_y =  bounding_rect.height() / static_cast<double>(num + 1);
	double x = output ? bounding_rect.width() : -Connector::width - 1;
	double y = step_y - Connector::height / 2.0;
	for (size_t i = 0; i < num; ++i) {
		Connector *conn = new Connector(i, output, this);
		array.push_back(conn);
		conn->setPos(x, y);
		conn->setVisible(true);

		y += step_y;
	}
}

void Operator::reset_connector_positions()
{
	connector_pos.clear();
	connector_pos.reserve(input_connectors.size() + output_connectors.size() + 4);
	reset_connector_positions(input_connectors, false);
	reset_connector_positions(output_connectors, true);
}

void Operator::reset_connector_positions(const std::vector<Connector *> &array, bool output)
{
	QRectF scene_bounding_rect = sceneBoundingRect();

	double safety_x = output ? scene_bounding_rect.right() + safety_distance
				 : scene_bounding_rect.left() - safety_distance;

	QPointF corner_top_pos(safety_x, safety_rect.top());
	connector_pos.emplace_back(ConnectorType::corner(output ? 1 : 2), corner_top_pos);

	int i = 0;
	for (Connector *conn: array) {
		QPointF relativePos = conn->pos();
		double y = relativePos.y();
		QPointF pos(safety_x, scene_bounding_rect.top() + y + Connector::height / 2.0);
		conn->set_safety_pos(pos);

		ConnectorType type = output ?
			ConnectorType::output_connector(i) :
			ConnectorType::input_connector(i);
		connector_pos.emplace_back(type, pos);
		++i;
	}

	QPointF corner_bottom_pos(safety_x, scene_bounding_rect.bottom() + safety_distance);
	connector_pos.emplace_back(ConnectorType::corner(output ? 0 : 3), corner_bottom_pos);
}

double Operator::get_new_button_x(int size, Side side)
{
	if (side == Side::left) {
		double x = button_left_boundary;
		button_left_boundary += size;
		return x;
	} else {
		button_right_boundary += size;
		return boundingRect().width() - button_right_boundary;
	}
}

void Operator::add_button(QGraphicsPixmapItem *item, Side side)
{
	QSize size = item->pixmap().size();
	if (size.height() > button_height)
		button_height = size.height();
	double x = get_new_button_x(size.width(), side);
	item->setPos(x, -button_offset - size.height());
}

void Operator::add_button(QGraphicsTextItem *item, const char *text, Side side)
{
	QFontMetrics fm(item->font());
	double height = fm.height();
	double text_width = fm.horizontalAdvance(text);
	double total_width = std::max(16.0, text_width);

	if (height > button_height)
		button_height = height;
	double x = get_new_button_x(total_width, side);
	item->setPos(x, -button_offset - height - fm.descent());
}

void Operator::add_scroller(QGraphicsRectItem *item)
{
	double operator_width = boundingRect().width();
	double width = operator_width - button_left_boundary - button_right_boundary;
	double height = Scroller::height;
	if (height > button_height)
		button_height = height;
	item->setRect(button_left_boundary, -button_offset - height, width, height);
}

void Operator::add_button_new_line()
{
	button_offset += button_height;
	button_height = 0;
	button_left_boundary = 0;
	button_right_boundary = 0;
}

QGraphicsTextItem *Operator::add_text_line()
{
	add_button_new_line();

	QFont font("Times", 10);
	QFontMetrics fm(font);
	int height = fm.height();

	QGraphicsTextItem *res = new QGraphicsTextItem(this);
	button_height = height;
	res->setFont(font);
	res->setPos(0, -button_offset - height - fm.descent());

	add_button_new_line();

	return res;
}

const std::vector<ConnectorPos> &Operator::get_connector_pos() const
{
	return connector_pos;
}

QRectF Operator::get_double_safety_rect() const
{
	return boundingRect().adjusted(
		-2*safety_distance, -2*safety_distance-button_offset-button_height,
		 2*safety_distance,  2*safety_distance);
}

void Operator::update_safety_rect()
{
	safety_rect = sceneBoundingRect().adjusted(
			-safety_distance, -safety_distance-button_offset-button_height,
			 safety_distance,  safety_distance);
}

void Operator::enter_placed_mode()
{
	reset_connector_positions();

	get_document().operator_list.add(this, get_scene());
}

Connector *Operator::nearest_connector(const QPointF &pos) const
{
	QRectF rect = sceneBoundingRect();

	double y = pos.y() - rect.top();
	if (input_connectors.empty())
		return nearest_connector(output_connectors, y);
	if (output_connectors.empty())
		return nearest_connector(input_connectors, y);

	if (pos.x() - rect.left() < rect.right() - pos.x()) {
		// We're closer to the left side (input connectors)
		return nearest_connector(input_connectors, y);
	} else {
		// We're closer to the right side (output connectors)
		return nearest_connector(output_connectors, y);
	}
}

Connector *Operator::nearest_connector(std::vector<Connector *> connectors, double y) const
{
	assert(!connectors.empty());
	Connector *res = connectors[0];
	double dist = res->y_dist(y);
	for (size_t i = 1; i < connectors.size(); ++i) {
		double dist2 = connectors[i]->y_dist(y);
		if (dist2 < dist) {
			dist = dist2;
			res = connectors[i];
		}
	}
	return res;
}

Connector &Operator::get_input_connector(size_t id)
{
	assert(id < input_connectors.size());
	return *input_connectors[id];
}

const Connector &Operator::get_input_connector(size_t id) const
{
	assert(id < input_connectors.size());
	return *input_connectors[id];
}

Connector &Operator::get_output_connector(size_t id)
{
	assert(id < output_connectors.size());
	return *output_connectors[id];
}

const Connector &Operator::get_output_connector(size_t id) const
{
	assert(id < output_connectors.size());
	return *output_connectors[id];
}

FFTBuf &Operator::get_output_buffer(size_t id)
{
	assert(id < output_connectors.size());
	return output_buffers[id];
}

void Operator::init_simple(const char *icon_name)
{
	setPixmap(name_to_pixmap(icon_name, simple_size));
}

static bool obstructed_corner(const QPointF &pos, const QPointF &corner_pos, bool left, bool up)
{
	bool line_left = (pos.x() - corner_pos.x()) < 0.0;
	bool line_up = (pos.y() - corner_pos.y()) < 0.0;
	return line_left == left && line_up == up;
}

// Locate corners of safety rectangle visible from the given point (ignore itermediate obstacles)
// Returns a bit field with the following bit ids:
// 2   1
//  +-+
//  | |
//  +-+
// 3   0
int Operator::visible_corners(const QPointF &pos) const
{
	int res = 0;
	if (!obstructed_corner(pos, corner_coord(0), true, true))
		res |= (1 << 0);
	if (!obstructed_corner(pos, corner_coord(1), true, false))
		res |= (1 << 1);
	if (!obstructed_corner(pos, corner_coord(2), false, false))
		res |= (1 << 2);
	if (!obstructed_corner(pos, corner_coord(3), false, true))
		res |= (1 << 3);
	return res;
}

QPointF Operator::corner_coord(int corner) const
{
	switch (corner) {
	case 0: return safety_rect.bottomRight();
	case 1: return safety_rect.topRight();
	case 2: return safety_rect.topLeft();
	case 3: return safety_rect.bottomLeft();
	}
	assert(false);
}

QRectF Operator::get_safety_rect() const
{
	return safety_rect;
}

QPointF Operator::go_out_of_safety_rect(const QPointF &pos) const
{
	// Check if we are closer to the right or left
	double delta_left = pos.x() - safety_rect.left();
	double delta_right = safety_rect.right() - pos.x();
	return {delta_left < delta_right ?
			safety_rect.left() - 1.0 :
			safety_rect.right() + 1.0,
		pos.y()};
}

OperatorList::view_list &Operator::get_view_list(ConnectorType type)
{
	if (type.is_input_connector()) {
		int id = type.input_connector_id();
		return input_connectors[id]->get_view_list();
	} else if (type.is_output_connector()) {
		int id = type.output_connector_id();
		return output_connectors[id]->get_view_list();
	} else {
		assert(type.is_corner());
		int id = type.corner_id();
		return corners_view_list[id];
	}
}

void Operator::add_view_connection(const ConnectorType type, const OperatorList::view_iterator &it)
{
	get_view_list(type).push_back(it);
}

void Operator::remove_view_connection(const ConnectorType type, const ViewConnection *conn)
{
	OperatorList::view_list &list = get_view_list(type);
	auto it = std::find_if(list.begin(), list.end(), [conn](const OperatorList::view_iterator &item)
							 { return &*item == conn; });
	assert(it != list.end());
	list.erase(it);
}

size_t Operator::get_topo_id() const
{
	return topo_id;
}

void Operator::set_topo_id(size_t id)
{
	topo_id = id;

	// For debugging: print id
	if (Globals::debug_mode) {
		QString text;
		text.setNum(id);
		if (topo_text) {
			topo_text->setText(text);
		} else {
			topo_text = new QGraphicsSimpleTextItem(text, this);
			topo_text->setVisible(true);
		}
	}
}

bool Operator::make_output_empty(size_t bufid)
{
	FFTBuf &buf = output_buffers[bufid];
	if (!buf.is_forwarded() && buf.is_empty())
		return false;
	buf = FFTBuf();
	return true;
}

bool Operator::make_output_complex(size_t bufid)
{
	FFTBuf &buf = output_buffers[bufid];
	if (!buf.is_forwarded() && buf.is_complex())
		return false;
	size_t n = get_document().fft_size;
	buf = FFTBuf(true, n);
	return true;
}

bool Operator::make_output_real(size_t bufid)
{
	FFTBuf &buf = output_buffers[bufid];
	if (!buf.is_forwarded() && buf.is_real())
		return false;
	size_t n = get_document().fft_size;
	buf = FFTBuf(false, n);
	return true;
}

bool Operator::make_output_forwarded(size_t bufid, FFTBuf &copy)
{
	output_buffers[bufid] = FFTBuf(copy);
	return true;
}

void Operator::output_buffer_changed()
{
	get_document().topo.update_buffers(this, false);
}

Document &Operator::get_document()
{
	return w.get_document();
}

const Document &Operator::get_document() const
{
	return w.get_document();
}

Scene &Operator::get_scene()
{
	return w.get_scene();
}

const Scene &Operator::get_scene() const
{
	return w.get_scene();
}

void Operator::set_border(int thickness)
{
	int offset1 = 1 + (thickness-1) / 2;
	int offset2 = thickness / 2;
	border->setRect(boundingRect().adjusted(-offset1, -offset1-button_offset-button_height,
						 offset2, offset2));
	border->setPen(QPen(Qt::black, thickness));
}

void Operator::select()
{
	set_border(border_selected_thickness);
}

void Operator::deselect()
{
	set_border(border_unselected_thickness);
}

static void remove_views(OperatorList &operator_list, const OperatorList::view_list &views)
{
	// Note that we copy the list because the view delete themselves from
	// the list on erasure
	OperatorList::view_list views_copy = views;
	for (auto &it: views_copy)
		operator_list.remove_view(it);
}

void Operator::remove_edges()
{
	// In a first step, remove all edges to and from this operator
	for (Connector *conn: input_connectors)
		conn->remove_edges();
	for (Connector *conn: output_connectors)
		conn->remove_edges();
}

std::vector<Edge *> Operator::get_edges()
{
	std::vector<Edge *> res;
	for (Connector *conn: input_connectors) {
		Edge *e = conn->get_parent_edge();
		if (e)
			res.push_back(conn->get_parent_edge());
	}
	for (Connector *conn: output_connectors) {
		auto &edges = conn->get_children_edges();
		res.insert(res.end(), edges.begin(), edges.end());
	}
	return res;
}

// Apply a function to all views of this operator:
//	1) Input connectors
//	2) Output connectors
//	3) Corners
template <typename Function>
void Operator::for_all_view_lists(Function f)
{
	for (Connector *conn: input_connectors)
		f(conn->get_view_list());
	for (Connector *conn: output_connectors)
		f(conn->get_view_list());
	for (const OperatorList::view_list &views: corners_view_list)
		f(views);
}

void Operator::remove_placed_from_scene()
{
	// Deselect
	remove_from_selection();

	// Remove own edges
	remove_edges();


	std::vector<Edge *> clear_edges = get_obstructed_edges();

	// Remove all view connections from the edges
	for (Edge *e: clear_edges)
		e->unregister_view_connections();
	for (Edge *e: get_edges())
		e->unregister_view_connections();

	// Next, remove all views
	Document &d = get_document();
	OperatorList &operator_list = d.operator_list;
	for_all_view_lists([&operator_list](const OperatorList::view_list &v)
			   { remove_views(operator_list, v); } );

	// Next, delete from operator list. This also removes all views
	operator_list.remove(this, get_scene());

	// Next, remove from topological order
	d.topo.remove_operator(this);

	// Recalculate obstructed edges
	for (Edge *e: clear_edges)
		e->recalculate();

	get_scene().removeItem(this);
}

void Operator::remove_unplaced_from_scene()
{
	get_document().topo.remove_operator(this);
	get_scene().removeItem(this);
}

// TODO: remove
void Operator::remove()
{
	remove_placed_from_scene();
	delete this;
}

Operator::State::~State()
{
}

std::vector<Operator::InitState> Operator::get_init_states()
{
	return {};
}

// TODO: An ugly hack to avoid having two identical versions of the state getter.
Operator::State &Operator::get_state()
{
	return const_cast<State &>(const_cast<const Operator &>(*this).get_state());
}

void Operator::place_set_state_command(const QString &text, std::unique_ptr<State> state, bool merge)
{
	Document &d = get_document();
	d.place_command<CommandSetState>(d, get_scene(), text, this, std::move(state), merge);
}

void Operator::save_state()
{
	saved_state = get_state().clone();
}

QJsonObject OperatorStateNone::to_json() const
{
	return QJsonObject{};
}

void OperatorStateNone::from_json(const QJsonObject &)
{
}

void Operator::restore_state()
{
	if (!saved_state)
		return;
	set_state(*saved_state);
	saved_state.reset();
	state_reset(); // recalculate operator
}

void Operator::commit_state()
{
	saved_state.reset();
}

// Append edges of output iterators to JSon array. Used for saving.
void Operator::out_edges_to_json(QJsonArray &out) const
{
	for (Connector *conn: output_connectors)
		conn->out_edges_to_json(out);
}

QJsonObject Operator::to_json() const
{
	QJsonObject res;

	QPointF pos = scenePos();

	res["type"] = QString::fromStdString(operator_factory.id_to_string(get_id()));
	res["x"] = pos.x();
	res["y"] = pos.y();
	res["state"] = get_state().to_json();

	return res;
}

Operator *Operator::from_json(MainWindow &w, const QJsonObject &desc)
{
	// For historical reasons, we still support numeric type-ids as well as strings.
	QJsonValue id_v = desc["type"];
	OperatorId id = id_v.isString() ? operator_factory.string_to_id(id_v.toString().toStdString())
					: static_cast<OperatorId>(id_v.toInt());

	std::unique_ptr<Operator> op = operator_factory.make(id, w);
	if (!op)
		return nullptr;

	// TODO: This is horrible. We simulate adding of the operator
	// as if the user would have done it. Detangle this.
	QPointF pos(desc["x"].toDouble(), desc["y"].toDouble());
	op->setPos(pos);

	op->prepare_init();
	op->init();
	op->finish_init();
	op->add_to_scene();
	op->placed();
	op->update_safety_rect();
	op->enter_placed_mode();
	op->state_from_json(desc["state"].toObject());
	op->state_reset();

	return op.release();
}

void Operator::leave_drag_mode(bool commit)
{
	if (commit)
		commit_state();
	else
		restore_state();

	restore_handles();
}

void Operator::drag(const QPointF &pos, Qt::KeyboardModifiers mod)
{
	drag_handle(mapFromScene(pos), mod);
}

void Operator::restore_handles()
{
}

void Operator::enter_drag_mode()
{
	save_state();
	get_scene().enter_drag_mode(this);
}

void Operator::drag_handle(const QPointF &, Qt::KeyboardModifiers)
{
}

// Collect all edges that use a view connection concerning this operator.
// Prepare the recalculation of these edges by removing them from all view connections.
// Thus, view connections concerning these edges can be removed in the next step.
std::vector<Edge *> Operator::get_obstructed_edges()
{
	std::vector<Edge *> res;
	res.reserve(20);		// Heuristics
	for_all_view_lists([&res](const OperatorList::view_list &v) {
		for (const OperatorList::view_iterator &it: v)
			it->collect_edges(res);
	});
	return res;
}

void Operator::enter_move_mode(QPointF mouse_pos)
{
	move_start_pos = pos();
	move_mouse_start_pos = mouse_pos;
	get_scene().enter_move_mode(this);

	// Only do the heavy work of unregistering the operator
	// from the view list when the mouse was actually moved.
	move_started = false;
}

void Operator::remove_from_view_list()
{
	for (Edge *e: get_edges())
		e->unregister_view_connections();

	// Remove from operator list
	w.get_document().operator_list.remove(this, get_scene());

	std::vector<Edge *> clear_edges = get_obstructed_edges();

	// Remove all view connections from the edges
	for (Edge *e: clear_edges)
		e->unregister_view_connections();
	for (Edge *e: get_edges())
		e->unregister_view_connections();

	// Then, remove all views
	Document &d = get_document();
	OperatorList &operator_list = d.operator_list;
	for_all_view_lists([&operator_list](const OperatorList::view_list &v)
			   { remove_views(operator_list, v); } );

	// Recalculate obstructed edges
	for (Edge *e: clear_edges)
		e->recalculate();
}

void Operator::readd_to_view_list()
{
	update_safety_rect();
	reset_connector_positions();
	w.get_document().operator_list.add(this, get_scene());
	recalculate_edges();
}

void Operator::move_event(QPointF mouse_pos)
{
	// If this is the first move, remove the operator from the view list
	if (!move_started) {
		move_started = true;
		remove_from_view_list();
	}

	QPointF move_to = mouse_pos - move_mouse_start_pos + move_start_pos;

	// Don't move where there is a different operator
	QRectF bounding_rect = boundingRect();
	QRectF safety_rect = bounding_rect;
	safety_rect.moveTo(move_to);
	safety_rect.adjust(-safety_distance, -safety_distance-button_offset-button_height,
			    safety_distance,  safety_distance);
	if (w.get_document().operator_list.operator_in_rect(safety_rect))
		return;

	setPos(move_to);
	update_safety_rect();
	reset_connector_positions();

	// Recalculate edges
	for (Connector *conn: input_connectors) {
		Edge *e = conn->get_parent_edge();
		if (e)
			e->recalculate_move(false);
	}
	for (Connector *conn: output_connectors) {
		auto &edges = conn->get_children_edges();
		for (Edge *e: edges)
			e->recalculate_move(true);
	}
}

void Operator::recalculate_edges()
{
	// In a first step, remove all edges to and from this operator
	for (Connector *conn: input_connectors)
		conn->recalculate_edges();
	for (Connector *conn: output_connectors)
		conn->recalculate_edges();
}

void Operator::leave_move_mode(bool commit)
{
	// There wasn't even a move -> do nothing.
	if (!move_started)
		return;

	if (!commit)
		setPos(move_start_pos);

	readd_to_view_list();

	if (commit) {
		Document &d = get_document();
		d.place_command<CommandMove>(d, get_scene(), this, move_start_pos, pos());
	}
}

void Operator::move_to(QPointF pos)
{
	remove_from_view_list();
	setPos(pos);
	readd_to_view_list();
}

void Operator::execute_topo()
{
	// Only execute children. This is called by an operator that has updated its data.
	get_document().topo.execute(this, false);
}

size_t Operator::get_fft_size() const
{
	return get_document().fft_size;
}
