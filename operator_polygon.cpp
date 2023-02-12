// SPDX-License-Identifier: GPL-2.0
#include "operator_polygon.hpp"
#include "document.hpp"
#include "mainwindow.hpp"
#include "scramble.hpp"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>

#include <cassert>

QJsonObject OperatorPolygonState::to_json() const
{
	QJsonObject res;
	res["mode"] = static_cast<int>(mode);
	res["draw_mode"] = static_cast<int>(draw_mode);
	res["width"] = width;
	res["height"] = height;
	res["offset_x"] = offset.x();
	res["offset_y"] = offset.y();
	res["rotation"] = rotation;
	return res;
}

void OperatorPolygonState::from_json(const QJsonObject &desc)
{
	mode = static_cast<size_t>(desc["mode"].toInt());
	draw_mode = static_cast<OperatorPolygonDrawMode>(desc["draw_mode"].toInt());
	width = desc["width"].toInt();
	height = desc["height"].toInt();
	offset = QPoint(desc["offset_x"].toInt(), desc["offset_y"].toInt());
	rotation = desc["rotation"].toDouble();
}

void OperatorPolygon::state_reset()
{
	make_polygon();
	paint_polygon();
	place_arrows();
	update_buffer();
}

OperatorPolygon::Arrow::Arrow(SVGCache::Id svg_id,
			      double angle,
			      double offset_x,
			      double offset_y,
			      const QPointF &pos_,
			      Type type_,
			      OperatorPolygon *parent)
	: QGraphicsSvgItem(parent)
	, op(*parent)
	, pos(pos_)
	, type(type_)
	, base_angle(angle)
	, svg(svg_cache.get(svg_id))
	, svg_highlighted(svg_cache.get_highlighted(svg_id))
{
	static constexpr double size = 16.0;

	setSharedRenderer(&svg);
	QSizeF rect_size = boundingRect().size();
	double scale = size / std::max(rect_size.width(), rect_size.height());

	QSizeF new_size = rect_size * scale;
	setScale(scale);
	translation_base = QTransform{1, 0, 0, 1,
				      -new_size.width() * offset_x,
				      -new_size.height() * offset_y};
	offset = { -rect_size.width() / 2.0,
		   -rect_size.height() / 2.0 };

	setAcceptHoverEvents(true);
	setAcceptTouchEvents(true);
}

void OperatorPolygon::Arrow::set_transformation(const QTransform &transform, double angle)
{
	QTransform trans = translation_base;
	QTransform rot;
	QPointF trans_vector2 = transform.map(pos);
	QTransform trans2{1, 0, 0, 1, trans_vector2.x(), trans_vector2.y()};
	rot.rotate(angle + base_angle);
	setTransform(trans * rot * trans2);
}

void OperatorPolygon::Arrow::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
	QString tooltip = type == Type::width || type == Type::height ?
				QStringLiteral("Drag to resize, Ctrl to retain shape") :
			  type == Type::rotation ?
				QStringLiteral("Drag to rotate") :
				QStringLiteral("Drag to move, Shift to fix horizontally, Ctrl to fix vertically");
	op.w.show_tooltip(tooltip);
	setSharedRenderer(&svg_highlighted);
}

void OperatorPolygon::Arrow::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
	op.w.hide_tooltip();
	setSharedRenderer(&svg);
}

void OperatorPolygon::Arrow::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (!(event->buttons() & Qt::LeftButton))
		return;
	static_cast<OperatorPolygon *>(parentItem())->clicked_arrow(event, type);
}

QPointF OperatorPolygon::scene_to_local(const QPointF &p) const
{
	size_t n = get_fft_size();
	return p - QPointF(n / 2, n / 2) - state.offset;
}

double OperatorPolygon::scene_to_angle(const QPointF &p) const
{
	QPointF p2 = scene_to_local(p);
	return atan2(p2.y(), p2.x());
}

double OperatorPolygon::scene_to_horizontal_dist(const QPointF &p) const
{
	QPointF p2 = scene_to_local(p);
	return p2.x() * cos(state.rotation) + p2.y() * sin(state.rotation);
}

double OperatorPolygon::scene_to_vertical_dist(const QPointF &p) const
{
	QPointF p2 = scene_to_local(p);
	return -p2.x() * sin(state.rotation) + p2.y() * cos(state.rotation);
}

QPoint OperatorPolygon::scene_to_pos(const QPointF &p) const
{
	size_t n = get_fft_size();
	QPointF p2 = p - QPointF(n / 2, n / 2);
	return p2.toPoint();
}

void OperatorPolygon::clicked_arrow(QGraphicsSceneMouseEvent *event, Arrow::Type type)
{
	move_type = type;

	QPointF pos = mapFromScene(event->scenePos());
	switch (move_type) {
	case Arrow::Type::width:
		original_value = state.width;
		original_value_alt = state.height;
		clicked_value = scene_to_horizontal_dist(pos);
		break;
	case Arrow::Type::height:
		original_value = state.height;
		original_value_alt = state.width;
		clicked_value = scene_to_vertical_dist(pos);
		break;
	case Arrow::Type::rotation:
		original_value = state.rotation;
		clicked_value = scene_to_angle(pos);
		break;
	case Arrow::Type::move:
		original_offset = state.offset;
		clicked_offset = scene_to_pos(pos);
		break;
	}

	hide_arrows();
	enter_drag_mode();
}

void OperatorPolygon::init()
{
	size_t n = get_fft_size();
	image = QImage(n, n, QImage::Format_Grayscale8);
	image.fill(0);

	state.width = n / 4;
	state.height = n / 4;
	dont_accumulate_undo = true;

	arrows[0] = new Arrow(SVGCache::Id::arrow_updown, 0.0, 0.5, 1.0, { 0, -1 }, Arrow::Type::height, this);
	arrows[1] = new Arrow(SVGCache::Id::arrow_updown, 0.0, 0.5, 0.0, { 0, 1 }, Arrow::Type::height, this);
	arrows[2] = new Arrow(SVGCache::Id::arrow_updown, 90.0, 0.5, 0.0, { -1, 0 }, Arrow::Type::width, this);
	arrows[3] = new Arrow(SVGCache::Id::arrow_updown, 90.0, 0.5, 1.0, { 1, 0 }, Arrow::Type::width, this);
	arrows[4] = new Arrow(SVGCache::Id::arrow_leftdown, 0.0, 0.5, 0.5, { 1, -1 }, Arrow::Type::rotation, this);
	arrows[5] = new Arrow(SVGCache::Id::arrow_leftdown, 90.0, 0.5, 0.5, { 1, 1 }, Arrow::Type::rotation, this);
	arrows[6] = new Arrow(SVGCache::Id::arrow_leftdown, 270.0, 0.5, 0.5, { -1, -1 }, Arrow::Type::rotation, this);
	arrows[7] = new Arrow(SVGCache::Id::arrow_leftdown, 180.0, 0.5, 0.5, { -1, 1 }, Arrow::Type::rotation, this);
	arrows[8] = new Arrow(SVGCache::Id::move, 0.0, 0.5, 0.5, { 0, 0 }, Arrow::Type::move, this);

	new Button(":/icons/reset.svg", "Reset shape", [this](){ clear(); }, Side::left, this);
	new Button(":/icons/poly_3.svg", "Change to triangle", [this](){ set_mode(3); }, Side::left, this);
	new Button(":/icons/poly_4.svg", "Change to rectangle", [this](){ set_mode(4); }, Side::left, this);
	new Button(":/icons/poly_5.svg", "Change to pentagon", [this](){ set_mode(5); }, Side::left, this);
	new Button(":/icons/poly_6.svg", "Change to hexagon", [this](){ set_mode(6); }, Side::left, this);
	new Button(":/icons/circle.svg", "Change to ellipse", [this](){ set_mode(0); }, Side::left, this);
	new Button(":/icons/inversion.svg", "Invert (rotate by 180°)", [this](){ invert(); }, Side::left, this);

	draw_mode_menu = new MenuButton(Side::left, "Set rendering type", this);
	draw_mode_menu->add_entry(":/icons/poly_6.svg", "Filled", [this](){ set_draw_mode(OperatorPolygonDrawMode::fill); });
	draw_mode_menu->add_entry(":/icons/poly_dots.svg", "Dots", [this](){ set_draw_mode(OperatorPolygonDrawMode::dots); });
	draw_mode_menu->add_entry(":/icons/poly_lines.svg", "Lines", [this](){ set_draw_mode(OperatorPolygonDrawMode::line); });

	make_polygon();
	paint_polygon();
	place_arrows();
	show_arrows();
}

void OperatorPolygon::placed()
{
	make_output_real(0);
	output_buffers[0].set_extremes(Extremes(1.0));

	make_polygon();
	paint_polygon();
	update_buffer();
}

void OperatorPolygon::make_polygon()
{
	if (state.mode == 0) {
		// Circle -> no polygon needed
		poly.clear();
		return;
	}

	poly.resize(state.mode);
	double step = 2 * M_PI / state.mode;
	double base = state.mode % 2 == 0 ? M_PI / state.mode : -M_PI / 2;
	double scale = state.mode == 4 ? sqrt(2.0) : 1.0;
	for (size_t i = 0; i < state.mode; ++i) {
		double angle = i * step + base;
		poly[i] = QPointF(cos(angle) * scale, sin(angle) * scale);
	}
}

// Custom Bresenham line-drawing algorithm, because QPainter
// does strange things that do not end up in a centrosymmetric
// picture, even if the points were arranged in a centrosymmatric
// manner.
static void draw_line(unsigned char *data, int n, const QPoint &p1, const QPoint &p2)
{
	int dx =  std::abs(p2.x() - p1.x());
	int dy = -std::abs(p2.y() - p1.y());
	int step_x = p1.x() < p2.x() ? 1 : -1;
	int step_y = p1.y() < p2.y() ? n : -n;
	int e = dx + dy;
	unsigned char *act = data + p1.x() + p1.y() * n;
	unsigned char *end = data + p2.x() + p2.y() * n;

	for (;;) {
		*act = 255;
		if (act == end)
			return;
		int e2 = e * 2;
		if (e2 > dy) {
			e += dy;
			act += step_x;
		}
		if (e2 < dx) {
			e += dx;
			act += step_y;
		}
	}
}

// Custom Bresenham line-drawing algorithm for filling polygons,
// because QPainter does strange things that do not end up in a
// centrosymmetric picture, even if the points were arranged in a
// centrosymmatric manner.
static void draw_line_scanline(std::vector<int> &scanline, QPoint p1, QPoint p2)
{
	int dx =  std::abs(p2.x() - p1.x());
	int dy = -std::abs(p2.y() - p1.y());
	int step_x = p1.x() < p2.x() ? 1 : -1;
	int step_y = p1.y() < p2.y() ? 2 : -2;

	if (step_y == 0)
		return;
	bool right = step_y > 0; // We draw cw (because y is in the wrong direction!),
				 // therefore up means right side.

	// The algorithm below only paints the first point in each row.
	// Therefore, invert for up-right and down left.
	bool invert = (right && step_x > 0) || (!right && step_x < 0);
	if (invert) {
		std::swap(p1, p2);
		step_x = -step_x;
		step_y = -step_y;
	}

	int e = dx + dy;
	int *act = &scanline[2 * p1.y() + !!right];
	int *end = &scanline[2 * p2.y() + !!right];
	int act_x = p1.x();

	*act = act_x;
	for (;;) {
		if (act == end)
			return;
		int e2 = e * 2;
		if (e2 > dy) {
			e += dy;
			act_x += step_x;
		}
		if (e2 < dx) {
			e += dx;
			act += step_y;
			*act = act_x;
		}
	}
}

void draw_poly_line(unsigned char *data, size_t n, const QPolygon &poly)
{
	for (int i = 1; i < poly.size(); ++i)
		draw_line(data, (int)n, poly[i - 1], poly[i]);
	draw_line(data, (int)n, poly.back(), poly[0]);
}

void draw_poly_filled(unsigned char *data, size_t n, const QPolygon &poly)
{
	auto [min_it, max_it] = std::minmax_element(poly.begin(), poly.end(), [] (const QPoint &p1, const QPoint &p2)
									      { return p1.y() < p2.y(); });
	int min_y = min_it->y();
	int max_y = max_it->y();
	if (min_y >= max_y)
		return;

	std::vector<int> scanlines(2*(max_y - min_y + 1), 0); // pairs of from_x, to_x

	for (int i = 1; i < poly.size(); ++i) {
		QPoint p1 = poly[i - 1];
		QPoint p2 = poly[i];
		p1.ry() -= min_y;
		p2.ry() -= min_y;
		draw_line_scanline(scanlines, p1, p2);
	}
	QPoint p1 = poly.back();
	QPoint p2 = poly[0];
	p1.ry() -= min_y;
	p2.ry() -= min_y;
	draw_line_scanline(scanlines, p1, p2);

	int *act_line = &scanlines[0];
	unsigned char *line = &data[n * min_y];
	for (int y = min_y; y <= max_y; ++y) {
		for (int x = act_line[0]; x <= act_line[1]; ++x)
			line[x] = 255;
		act_line += 2;
		line += n;
	}
}

void OperatorPolygon::paint_polygon()
{
	size_t n = get_fft_size();

	unsigned char *data = image.bits();
	std::fill(data, data + n*n, 0);

	QPoint center = state.offset + QPoint(n / 2, n / 2);

	trans = QTransform();
	trans.scale(state.width / 2, state.height / 2);
	QTransform rotate;
	rotate.rotateRadians(state.rotation);
	trans *= rotate;
	QTransform translate(1, 0, 0, 1, center.x(), center.y());
	trans *= translate;

	if (state.mode == 0) {
		QPainter painter(&image);
		painter.setRenderHint(QPainter::Antialiasing, false);
		if (state.draw_mode == OperatorPolygonDrawMode::fill)
			painter.setBrush(QBrush(Qt::white, Qt::SolidPattern));
		else
			painter.setPen(Qt::white);
		painter.setTransform(trans);
		painter.drawEllipse(QPointF(0.0,0.0), 1.5, 1.5);
	} else {
		// Transform and clip to drawing area
		QPolygon boundary(QRect(0, 0, n - 1, n - 1));
		QPolygon poly_trans = trans.map(poly).toPolygon().intersected(boundary);

		// QPolygon::intersected() closes the polygon, so let's remove the last point.
		if (poly_trans.size() < 1) {
			setPixmap(QPixmap::fromImage(image));
			return;
		}
		poly_trans.pop_back();

		switch (state.draw_mode) {
		case OperatorPolygonDrawMode::dots:
			for (const QPoint &p: poly_trans)
				data[p.x() + p.y() * n] = 255;
			break;
		case OperatorPolygonDrawMode::line:
			draw_poly_line(data, n, poly_trans);
			break;
		case OperatorPolygonDrawMode::fill:
			draw_poly_filled(data, n, poly_trans);
			break;
		}
	}
	setPixmap(QPixmap::fromImage(image));
}

template<size_t N>
void OperatorPolygon::calculate()
{
	// Copy into output buffer
	const unsigned char *in = image.constBits();
	double *out = output_buffers[0].get_real_data();
	scramble<N, unsigned char, double>
		(in, out, [](unsigned char c)
		{ return static_cast<double>(c) / 255.0; });
}

void OperatorPolygon::update_buffer()
{
	dispatch_calculate(*this);

	// Execute children
	execute_topo();
}

void OperatorPolygon::show_arrows()
{
	for (Arrow *a: arrows)
		a->setVisible(true);
}

void OperatorPolygon::hide_arrows()
{
	for (Arrow *a: arrows)
		a->setVisible(false);
}

void OperatorPolygon::place_arrows()
{
	for (Arrow *a: arrows)
		a->set_transformation(trans, state.rotation / M_PI * 180.0);
}

// Signum function where we don't care for the result of input is zero
static int sgn(int i)
{
	return i > 0 ? 1 : -1;
}

void OperatorPolygon::drag_handle(const QPointF &p, Qt::KeyboardModifiers modifiers)
{
	auto new_state = clone_state();
	switch (move_type) {
	case Arrow::Type::width:
		new_state->width = std::abs((scene_to_horizontal_dist(p) - clicked_value) * 2 * sgn(clicked_value)
			+ original_value);
		if (modifiers & Qt::ControlModifier) {
			if (std::abs(original_value) > 0.1)
				new_state->height = original_value_alt * new_state->width / original_value;
		} else {
			new_state->height = original_value_alt;
		}
		break;
	case Arrow::Type::height:
		new_state->height = std::abs((scene_to_vertical_dist(p) - clicked_value) * 2 * sgn(clicked_value)
			+ original_value);
		if (modifiers & Qt::ControlModifier) {
			if (std::abs(original_value) > 0.1)
				new_state->width = original_value_alt * new_state->height / original_value;
		} else {
			new_state->width = original_value_alt;
		}
		break;
	case Arrow::Type::rotation:
		new_state->rotation = scene_to_angle(p) - clicked_value + original_value;
		break;
	case Arrow::Type::move:
		new_state->offset = scene_to_pos(p) - clicked_offset + original_offset;

		if (modifiers & Qt::ControlModifier)
			new_state->offset.setY(original_offset.y());
		else if (modifiers & Qt::ShiftModifier)
			new_state->offset.setX(original_offset.x());
		break;
	}

	QString text = move_type == Arrow::Type::width || move_type == Arrow::Type::height ? "Scale polygon"
		     : move_type == Arrow::Type::rotation ? "Rotate polygon"
		     : "Move polygon";
	place_set_state_command(text, std::move(new_state), !dont_accumulate_undo);
	dont_accumulate_undo = false;
}

void OperatorPolygon::restore_handles()
{
	show_arrows();
	dont_accumulate_undo = true;
}

void OperatorPolygon::clear()
{
	auto new_state = clone_state();
	size_t n = get_fft_size();
	new_state->width = n / 4;
	new_state->height = n / 4;
	new_state->rotation = 0.0;
	new_state->offset = QPoint();

	place_set_state_command("Reset polygon", std::move(new_state), false);
}

void OperatorPolygon::invert()
{
	auto new_state = clone_state();
	new_state->offset = -state.offset;
	new_state->rotation = state.rotation + M_PI;

	place_set_state_command("Invert polygon", std::move(new_state), false);
}

void OperatorPolygon::set_mode(size_t mode)
{
	if (state.mode == mode)
		return;

	auto new_state = clone_state();
	new_state->mode = mode;
	place_set_state_command("Set polygon number", std::move(new_state), false);
}

void OperatorPolygon::set_draw_mode(OperatorPolygonDrawMode draw_mode)
{
	if (state.draw_mode == draw_mode)
		return;

	auto new_state = clone_state();
	new_state->draw_mode = draw_mode;
	place_set_state_command("Set polygon drawing mode", std::move(new_state), false);
}
