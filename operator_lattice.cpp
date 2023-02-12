// SPDX-License-Identifier: GPL-2.0
#include "operator_lattice.hpp"
#include "document.hpp"
#include "basis_vector.hpp"

#include <QGraphicsSceneMouseEvent>

#include <cassert>
#include <numeric>

QJsonObject OperatorLatticeState::to_json() const
{
	QJsonObject res;
	res["d"] = static_cast<int>(d);
	res["x1"] = p1.x();
	res["y1"] = p1.y();
	res["x2"] = p2.x();
	res["y2"] = p2.y();
	return res;
}

void OperatorLatticeState::from_json(const QJsonObject &desc)
{
	d = desc["d"].toInt();
	p1 = QPoint(desc["x1"].toInt(), desc["y1"].toInt());
	p2 = QPoint(desc["x2"].toInt(), desc["y2"].toInt());
}

void OperatorLattice::state_reset()
{
	paint_lattice();
	place_handles();
	update_buffer();
}

void OperatorLattice::init()
{
	size_t n = get_fft_size();
	image = QImage(n, n, QImage::Format_Grayscale8);
	image.fill(0);
	setPixmap(QPixmap::fromImage(image));
	dont_accumulate_undo = true;
	handles_visible = true;

	handle1 = new Handle(false, "Drag to move, Ctrl to fix horizontaly, Shift to fix vertically", this);
	handle2 = new Handle(true, "Drag to move, Ctrl to fix horizontaly, Shift to fix vertically", this);
	QPointF centre(n/2, n/2);
	basis1 = new BasisVector(centre, this);
	basis2 = new BasisVector(centre, this);
	basis1->setZValue(2.0);
	basis2->setZValue(2.0);

	new Button(":/icons/reset.svg", "Reset lattice", [this](){clear();}, Side::left, this);
	new TextButton(("0D"), "Enter 0D (Dirac peak) mode", [this](){ set_d(0); }, Side::left, this);
	new TextButton(("1D"), "Enter 1D mode", [this](){ set_d(1); }, Side::left, this);
	new TextButton(("2D"), "Enter 2D mode", [this](){ set_d(2); }, Side::left, this);

	place_handles();
}

void OperatorLattice::placed()
{
	make_output_real(0);
	output_buffers[0].set_extremes(Extremes(1.0));
	paint_lattice();
	update_buffer();
}

OperatorLattice::Handle::Handle(bool second_axis_, const char *tooltip, Operator *parent)
	: Operator::Handle(tooltip, parent)
	, second_axis(second_axis_)
{
}

void OperatorLattice::Handle::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (!(event->buttons() & Qt::LeftButton))
		return;
	dynamic_cast<OperatorLattice *>(parentItem())->clicked_handle(event, second_axis);
}

void OperatorLattice::paint_basis()
{
	size_t fft_size = get_fft_size();
	QPointF centre(fft_size / 2, fft_size / 2);
	if (state.d == 0)
		return;
	basis1->set(state.p1);
	if (state.d == 2)
		basis2->set(state.p2);
}

void OperatorLattice::place_handles()
{
	if (state.d == 0) {
		handle1->setVisible(false);
		handle2->setVisible(false);
		basis1->setVisible(false);
		basis2->setVisible(false);
		return;
	}

	size_t fft_size = get_fft_size();
	QPointF centre(fft_size / 2, fft_size / 2);
	handle1->set_pos(state.p1 + centre);
	handle1->setVisible(handles_visible);
	basis1->setVisible(true);

	if (state.d == 1) {
		handle2->setVisible(false);
		basis2->setVisible(false);
	} else {
		handle2->set_pos(state.p2 + centre);
		handle2->setVisible(handles_visible);
		basis2->setVisible(true);
	}
}

void OperatorLattice::hide_handles()
{
	handles_visible = false;
	handle1->setVisible(false);
	handle2->setVisible(false);
}

void OperatorLattice::show_handles()
{
	handles_visible = true;
	place_handles();
}

void OperatorLattice::clicked_handle(QGraphicsSceneMouseEvent *event, bool second_axis_)
{
	second_axis = second_axis_;
	hide_handles();

	clicked_pos = mapFromScene(event->scenePos());
	clicked_old_pos = second_axis ? state.p2 : state.p1;

	enter_drag_mode();
}

void OperatorLattice::drag_handle(const QPointF &p, Qt::KeyboardModifiers modifiers)
{
	QPoint pos = second_axis ? state.p2 : state.p1;

	pos = (p - clicked_pos + clicked_old_pos).toPoint();

	// If control is pressed, snap to either horizontal, vertical or diagonal
	if (modifiers & Qt::ControlModifier)
		pos.setX(clicked_old_pos.x());
	else if (modifiers & Qt::ShiftModifier)
		pos.setY(clicked_old_pos.y());

	auto new_state = clone_state();
	if (second_axis)
		new_state->p2 = pos;
	else
		new_state->p1 = pos;
	place_set_state_command("Modify lattice", std::move(new_state), !dont_accumulate_undo);
	dont_accumulate_undo = false;
}

void OperatorLattice::restore_handles()
{
	show_handles();
	dont_accumulate_undo = true;
}

// TODO: Templatize size for consistency
void OperatorLattice::paint_0d()
{
	size_t n = get_fft_size();
	unsigned char *data = image.bits();
	double *out = output_buffers[0].get_real_data();

	data[n/2 + n*n/2] = 255;
	out[0] = 1.0;
}

void OperatorLattice::paint_row_quadrant(QPoint p)
{
	int n = get_fft_size();
	unsigned char *data = image.bits();
	double *out = output_buffers[0].get_real_data();

	int pos_x = n / 2 + p.x();
	int pos_y = n / 2 + p.y();

	data += pos_y * n + pos_x;
	out += pos_y * n + pos_x;

	// Scramble output quadrant
	if (p.x() < 0)
		out += n/2;
	else
		out -= n/2;
	if (p.y() < 0)
		out += n/2 * n;
	else
		out -= n/2 * n;

	if (p.x() < 0 && p.y() < 0) {
		// Upper left quadrant
		while (pos_x > 0 && pos_y > 0) {
			*data = 255;
			*out = 1.0;
			pos_x += p.x();
			pos_y += p.y();
			data += p.x() + n*p.y();
			out += p.x() + n*p.y();
		}
	} else if (p.x() >= 0 && p.y() < 0) {
		// Upper right quadrant
		while (pos_x < n && pos_y > 0) {
			*data = 255;
			*out = 1.0;
			pos_x += p.x();
			pos_y += p.y();
			data += p.x() + n*p.y();
			out += p.x() + n*p.y();
		}
	} else if (p.x() < 0 && p.y() >= 0) {
		// Lower left quadrant
		while (pos_x > 0 && pos_y < n) {
			*data = 255;
			*out = 1.0;
			pos_x += p.x();
			pos_y += p.y();
			data += p.x() + n*p.y();
			out += p.x() + n*p.y();
		}
	} else {
		assert(p.x() >= 0 && p.y() >= 0);
		// Lower right quadrant
		while (pos_x < n && pos_y < n) {
			*data = 255;
			*out = 1.0;
			pos_x += p.x();
			pos_y += p.y();
			data += p.x() + n*p.y();
			out += p.x() + n*p.y();
		}
	}
}

void OperatorLattice::paint_1d(QPoint p)
{
	// Start with a point at the centre
	paint_0d();

	// No lattice vector -> nothing to do
	if (p.x() == 0 && p.y() == 0)
		return;

	paint_row_quadrant(p);
	paint_row_quadrant(-p);
}

static inline void mod_positive(int &v, int mod)
{
	v %= mod;
	if (v < 0)
		v += mod;
}

static inline void mod_negative(int &v, int mod)
{
	v %= mod;
	if (v >= 0)
		v -= mod;
}

void OperatorLattice::paint_2d(QPoint p1, QPoint p2)
{
	if (p1.x() == 0 && p1.y() == 0)
		return paint_1d(p2);
	if (p2.x() == 0 && p2.y() == 0)
		return paint_1d(p1);

	if (p1.x() * p2.y() == p2.x() * p1.y()) {
		// If both basis vector are parallel, make a 1D lattice
		// with gcd of both vectors.
		if (p1.x() != 0) {
			assert(p2.x() != 0);
			int gcd = std::gcd(p1.x(), p2.x());
			int factor = p1.x() / gcd;
			QPoint p(gcd, p1.y() / factor);
			return paint_1d(p);
		} else {
			assert(p1.y() != 0);
			assert(p2.y() != 0);
			int gcd = std::gcd(p1.y(), p2.y());
			int factor = p1.y() / gcd;
			QPoint p(p1.x() / factor, gcd);
			return paint_1d(p);
		}
	}

	// We search the primitive lattice vector in x-direction.
	// In principle, we express the (1,0) in the coordinate system of the lattice,
	// turn it into the smallest integral vector and transform into the original
	// coordinate system. Since the original and the resulting y-coordinates
	// are zero, we can omit some calculations.
	int d = p1.x() * p2.y() - p1.y() * p2.x();	// Determinant of the matrix describing the lattice basis
	assert(d != 0);
	int gcd = std::gcd(p2.y(), -p1.y());
	gcd = std::gcd(gcd, d);				// Make transformed vector integral
	int x_int = p2.y() / gcd;
	int y_int = -p1.y() / gcd;
	if (d < 0) {
		x_int = -x_int;
		y_int = -y_int;
	}

	// Turn it back into the original coordinate system.
	// We're only interesting in the x coordinate (y is 0), which will be the spacing
	// of the lattice in x-direction.
	int spacing_x = x_int * p1.x() + y_int * p2.x();

	// Find a primitive lattice vector with the lowest possible positive non-zero y coordinate
	// by application of Euclid's algorithm.
	if (p1.y() < 0)
		p1 = -p1;
	if (p2.y() < 0)
		p2 = -p2;
	while (p2.y() != 0) {
		int quot = p1.y() / p2.y();
		p1 -= quot * p2;
		std::swap(p1, p2);
	}
	assert(p1.y() != 0);
	int step_y = p1.y();

	// Make x coordinate as short as possible
	int step_x = p1.x();
	mod_positive(step_x, spacing_x);

	paint2d(step_x, step_y, spacing_x);
}

void OperatorLattice::paint2d(int step_x, int step_y, int spacing_x)
{
	int n = get_fft_size();

	// Paint bottom right quadrant
	unsigned char *data = image.bits();
	double *out = output_buffers[0].get_real_data();

	unsigned char *act = &data[n/2 + n*n/2];
	double *act_out = &out[0];
	int first_x = 0;
	for (int y = 0; y < n/2; y += step_y) {
		for (int x = first_x; x < n/2; x += spacing_x) {
			act[x] = 255;
			act_out[x] = 1.0;
		}
		first_x += step_x;
		mod_positive(first_x, spacing_x);
		act += n * step_y;
		act_out += n * step_y;
	}

	// Paint bottom left quadrant
	act = &data[n/2 + n*n/2];
	act_out = &out[n];
	first_x = -spacing_x;
	for (int y = 0; y < n/2; y += step_y) {
		for (int x = first_x; x >= -n/2; x -= spacing_x) {
			act[x] = 255;
			act_out[x] = 1.0;
		}
		first_x += step_x;
		mod_negative(first_x, spacing_x);
		act += n * step_y;
		act_out += n * step_y;
	}

	// Paint top right quadrant
	act = &data[n/2 + n*(n/2-step_y)];
	act_out = &out[n*(n-step_y)];
	first_x = -step_x;
	first_x %= spacing_x;
	if (first_x < 0)
		first_x += spacing_x;
	for (int y = step_y; y < n/2; y += step_y) {
		for (int x = first_x; x < n/2; x += spacing_x) {
			act[x] = 255;
			act_out[x] = 1.0;
		}
		first_x -= step_x;
		mod_positive(first_x, spacing_x);
		act -= n * step_y;
		act_out -= n * step_y;
	}

	// Paint top left quadrant
	act = &data[n/2 + n*(n/2-step_y)];
	act_out = &out[n + n*(n-step_y)];
	first_x = -step_x-spacing_x;
	mod_negative(first_x, spacing_x);
	for (int y = step_y; y < n/2; y += step_y) {
		for (int x = first_x; x >= -n/2; x -= spacing_x) {
			act[x] = 255;
			act_out[x] = 1.0;
		}
		first_x -= step_x;
		mod_negative(first_x, spacing_x);
		act -= n * step_y;
		act_out -= n * step_y;
	}
}

void OperatorLattice::paint_lattice()
{
	image.fill(0);
	output_buffers[0].clear_data();

	switch (state.d) {
	case 0:
	default:
		paint_0d();
		break;
	case 1:
		paint_1d(state.p1);
		break;
	case 2:
		paint_2d(state.p1, state.p2);
		break;
	}

	setPixmap(QPixmap::fromImage(image));
	paint_basis();
}

void OperatorLattice::update_buffer()
{
	// Execute children
	execute_topo();
}

void OperatorLattice::clear()
{
	auto new_state = std::make_unique<OperatorLatticeState>();
	place_set_state_command("Reset lattice", std::move(new_state), false);
}

void OperatorLattice::set_d(size_t d)
{
	if (state.d == d)
		return;

	if (d > 2)
		d = 2;

	auto new_state = clone_state();
	new_state->d = d;
	place_set_state_command("Set lattice dimensionality", std::move(new_state), false);
}
