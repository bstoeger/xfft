// SPDX-License-Identifier: GPL-2.0
#include "operator_gauss.hpp"
#include "document.hpp"
#include "scramble.hpp"
#include "color.hpp"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>

#include <cassert>

void OperatorGauss::init()
{
		// If meta is pressed, only modify angle.
		// If shift is pressed, only modify shape.
		// If control is pressed, scale.
	handle1 = new Handle(Handle::Type::first_axis, "Drag to modify, Shift retains angle, Ctrl retains shape, Meta rotates", this);
	handle2 = new Handle(Handle::Type::second_axis, "Drag to modify, Shift retains angle, Ctrl retains shape, Meta rotates", this);
	handle3 = new Handle(Handle::Type::move, "Drag to move center", this);

	size_t n = get_fft_size();
	image = QImage(n, n, QImage::Format_Grayscale8);
	center = QPointF(n / 2.0, n / 2.0);
	scale = n * 2.0 * sqrt(s_factor);
	dont_accumulate_undo = true;

	new Button(":/icons/reset.svg", "Reset shape", [this](){ clear(); }, Side::left, this);

	image.fill(0);
	setPixmap(QPixmap::fromImage(image));
	place_handles();
	show_handles();
}

QJsonObject OperatorGaussState::to_json() const
{
	QJsonObject res;
	res["e1"] = e1;
	res["e2"] = e2;
	res["angle"] = angle;
	res["offset_x"] = offset.x();
	res["offset_y"] = offset.y();
	return res;
}

void OperatorGaussState::from_json(const QJsonObject &desc)
{
	e1 = desc["e1"].toDouble();
	e2 = desc["e2"].toDouble();
	angle = desc["angle"].toDouble();
	offset = QPointF(desc["offset_x"].toDouble(), desc["offset_y"].toDouble());
}

void OperatorGauss::state_reset()
{
	place_handles();
	calculate_gauss();

	// Execute children
	execute_topo();
}

void OperatorGauss::placed()
{
	make_output_real(0);
	output_buffers[0].set_extremes(Extremes(1.0));

	calculate_gauss();
}

size_t OperatorGauss::num_input() const
{
	return 0;
}

size_t OperatorGauss::num_output() const
{
	return 1;
}

OperatorGauss::Handle::Handle(Type type_, const char *tooltip, Operator *parent)
	: Operator::Handle(tooltip, parent)
	, type(type_)
{
}

void OperatorGauss::Handle::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (!(event->buttons() & Qt::LeftButton))
		return;
	dynamic_cast<OperatorGauss *>(parentItem())->clicked_handle(event, type);
}

void OperatorGauss::show_handles()
{
	handle1->setVisible(true);
	handle2->setVisible(true);
	handle3->setVisible(true);
}

void OperatorGauss::hide_handles()
{
	handle1->setVisible(false);
	handle2->setVisible(false);
	handle3->setVisible(false);
}

void OperatorGauss::place_handles()
{
	handle1->set_pos(QPointF(cos(state.angle) * state.e1 * scale, sin(state.angle) * state.e1 * scale) + state.offset + center);
	handle2->set_pos(QPointF(-sin(state.angle) * state.e2 * scale, cos(state.angle) * state.e2 * scale) + state.offset + center);
	handle3->set_pos(state.offset + center);
}

void OperatorGauss::clicked_handle(QGraphicsSceneMouseEvent *event, Handle::Type type)
{
	move_type = type;
	hide_handles();

	clicked_pos = mapFromScene(event->scenePos());
	clicked_offset = state.offset;
	e1_old = state.e1;
	e2_old = state.e2;
	angle_old = state.angle;

	enter_drag_mode();
}

void OperatorGauss::drag_handle(const QPointF &p, Qt::KeyboardModifiers modifiers)
{
	auto new_state = clone_state();
	if (move_type == Handle::Type::move) {
		new_state->offset = p - clicked_pos + clicked_offset;
	} else {
		// If meta1 is pressed, only modify angle.
		// If shift is pressed, only modify shape.
		// If control is pressed, scale.
		QPointF rel = p - center - new_state->offset;
		if (!(modifiers & Qt::MetaModifier)) {
			double e = sqrt(rel.x()*rel.x() + rel.y()*rel.y()) / scale;
			if (move_type == Handle::Type::second_axis) {
				new_state->e1 = (modifiers & Qt::ControlModifier && e2_old > 0.00001) ?
					e1_old * e / e2_old : e1_old;
				new_state->e2 = e;
			} else {
				new_state->e2 = (modifiers & Qt::ControlModifier && e1_old > 0.00001) ?
					e2_old * e / e1_old : e2_old;
				new_state->e1 = e;
			}
		}

		if (!(modifiers & Qt::ShiftModifier)) {
			new_state->angle = atan2(rel.y(), rel.x());
			if (move_type == Handle::Type::second_axis)
				new_state->angle -= M_PI / 2.0;
		}
	}

	place_set_state_command("Modify Gaussian", std::move(new_state), !dont_accumulate_undo);
	dont_accumulate_undo = false;
}

void OperatorGauss::restore_handles()
{
	show_handles();
	dont_accumulate_undo = true;
}

// Fill data in a scrambled manner
// x and y are doubles that vary from -1 to 1 and are passed to a two-dimensional function.
template <size_t N, typename T, typename FUNC>
static inline void fill_data_quadrant(T *out, double x_start, double y_start, double step, FUNC fn)
{
	double y = y_start;
	for (size_t j = 0; j < N / 2; ++j) {
		double x = x_start;
		for (size_t i = 0; i < N / 2; ++i) {
			*out++ = fn(x, y);
			x += step;
		}
		y += step;
		out += N / 2;
	}
}

template <size_t N, typename T, typename FUNC>
static inline void fill_data(T *data, const QPointF &offset, FUNC fn)
{
	double step = 2.0 / N;
	double off_x = step * offset.x();
	double off_y = step * offset.y();

	fill_data_quadrant<N, T>(data,                    0.0 - off_x,  0.0 - off_y, step, fn);
	fill_data_quadrant<N, T>(data + N / 2,           -1.0 - off_x,  0.0 - off_y, step, fn);
	fill_data_quadrant<N, T>(data + N * N / 2,        0.0 - off_x, -1.0 - off_y, step, fn);
	fill_data_quadrant<N, T>(data + (N + 1) * N / 2, -1.0 - off_x, -1.0 - off_y, step, fn);
}

std::array<double, 3> OperatorGauss::calculate_tensor() const
{
	// Input: an angle of the first eigenvector and two eigenvalues e1 and e2.
	// Calculate the coordinates (x,y) of the first (unit) eigenvector.
	// The second (unit) eigenvector has coordinates (-y,x)
	double x = cos(state.angle);
	double y = sin(state.angle);

	// The variance matrix now has the form
	// / e1*x^2+e2*x^2 (e1-e2)*x*y   \.
	// \ (e1-e2)*x*y   e2*x^2+e1*x^2 /
	// From there we can calculate the sigmas s1 and s2
	// as  s1 = sqrt(e1*x^2+e2*y^2)
	// and s2 = sqrt(e2*x^2+e1*y^2)
	// The correlation coefficient finally is
	// r = (e1-e2)*x*y / (s1*s2)
	double e1_ = state.e1*state.e1;
	double e2_ = state.e2*state.e2;
	double s1 = sqrt(e1_*x*x + e2_*y*y);
	double s2 = sqrt(e2_*x*x + e1_*y*y);
	if (s1 < 1E-5 || s2 < 1E-5) {
		return { 0.0, 0.0, 0.0 };
	}
	double r = (e1_-e2_) * x * y / (s1*s2);

	double prefactor = -1.0 / (2.0 * (1.0 - r*r));
	double fxx = prefactor / (s1 * s1);
	double fyy = prefactor / (s2 * s2);
	double fxy = -2.0 * prefactor * r / (s1 * s2);
	return { fxx, fyy, fxy };
}

template<size_t N>
void OperatorGauss::calculate()
{
	// First step: calculate pixmap
	// auto [ fxx, fyy, fxy ] = calculate_tensor();
	// TODO: clang implements the standard to the point and therefore
	// we can't capture structured bindings. Let's hope for C++20!
	auto axes = calculate_tensor();

	double *data = output_buffers[0].get_real_data();
	fill_data<N, double>(data, state.offset,
			    [fxx = axes[0], fyy = axes[1], fxy = axes[2]](double x, double y)
			    { return exp(x*x*fxx + y*y*fyy + x*y*fxy); });

	// Second step: update pixmap
	const double *in = output_buffers[0].get_real_data();
	unsigned char *out = image.bits();
	scramble<N, double, unsigned char>
		(in, out, &::real_to_grayscale_unchecked);

}

void OperatorGauss::calculate_gauss()
{
	dispatch_calculate(*this);

	// Paint ellipse
	QPainter painter(&image);
	QTransform trans;
	trans.rotateRadians(state.angle);
	QTransform translate(1, 0, 0, 1, center.x() + state.offset.x(), center.y() + state.offset.y());
	trans *= translate;

	painter.setTransform(trans);
	painter.setPen(Qt::red);
	painter.drawEllipse(QPointF(0.0,0.0), state.e1 * scale, state.e2 * scale);

	setPixmap(QPixmap::fromImage(image));
}

void OperatorGauss::clear()
{
	auto new_state = std::make_unique<OperatorGaussState>();
	place_set_state_command("Reset Gaussian", std::move(new_state), false);
}

bool OperatorGauss::input_connection_changed()
{
	// There are no input connections
	assert(false);
}

void OperatorGauss::execute()
{
	// There are no input connections
	assert(false);
}
