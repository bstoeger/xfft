// SPDX-License-Identifier: GPL-2.0
#include "operator_wave.hpp"
#include "document.hpp"
#include "basis_vector.hpp"
#include "color.hpp"

#include <QGraphicsSceneMouseEvent>
#include <QMenu>

#include <cassert>

QJsonObject OperatorWaveState::to_json() const
{
	QJsonObject res;
	res["mode"] = static_cast<int>(mode);
	res["hx"] = h.x();
	res["hy"] = h.y();
	res["amplitude_mag"] = amplitude_mag;
	res["amplitude_phase"] = amplitude_phase;
	return res;
}

void OperatorWaveState::from_json(const QJsonObject &desc)
{
	mode = static_cast<OperatorWaveMode>(desc["mode"].toInt());
	h = QPoint(desc["hx"].toInt(), desc["hy"].toInt());
	amplitude_mag = desc["amplitude_mag"].toDouble();
	amplitude_phase = desc["amplitude_phase"].toDouble();
}

void OperatorWave::state_reset()
{
	set_scrollers();
	place_handle();
	mode_menu->set_pixmap((int)state.mode);
	paint_wave();
	update_buffer();
}

void OperatorWave::init()
{
	size_t n = get_fft_size();
	imagebuf = AlignedBuf<uint32_t>(n * n);
	dont_accumulate_undo = true;

	// TODO: See comment in OperatorView::init().
	// TODO: It would be nicer to call paint_wave(), but that is only possible after placed().
	{
		QPixmap empty_pixmap(n, n);
		empty_pixmap.fill(Qt::black);
		setPixmap(empty_pixmap);
	}

	handle = new Handle("Drag to move, Ctrl to snap to horizontal, vertical or diagonal", this);
	QPointF centre(n/2, n/2);
	basis = new BasisVector(centre, this);
	basis->setZValue(2.0);

	scroller_phase = new Scroller(0.0, 1.0, false, [this](double v){ set_amplitude_phase(v); }, this);
	add_button_new_line();
	scroller_mag = new Scroller(0.0, 1.0, false, [this](double v){ set_amplitude_mag(v); }, this);
	add_button_new_line();
	new Button(":/icons/reset.svg", "Reset plane wave", [this](){clear();}, Side::left, this);
	mode_menu = new MenuButton(Side::left, "Set mode", this);
	mode_menu->add_entry(":/icons/mag_phase.svg", "Magnitude/Phase", [this](){ switch_mode(OperatorWaveMode::mag_phase); });
	mode_menu->add_entry(":/icons/long_trans.svg", "Longitudinal/Transversal", [this](){ switch_mode(OperatorWaveMode::long_trans); });

	place_handle();
	show_handle();
}

void OperatorWave::placed()
{
	make_output_complex(0);
	set_scrollers();

	paint_wave();
	place_handle();
	show_handle();
	update_buffer();
}

OperatorWave::Handle::Handle(const char *tooltip, Operator *parent)
	: Operator::Handle(tooltip, parent)
{
}

void OperatorWave::Handle::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (!(event->buttons() & Qt::LeftButton))
		return;
	dynamic_cast<OperatorWave *>(parentItem())->clicked_handle(event);
}

void OperatorWave::paint_basis()
{
	basis->set(state.h);
}

void OperatorWave::place_handle()
{
	size_t fft_size = get_fft_size();
	QPointF centre(fft_size / 2, fft_size / 2);
	handle->set_pos(state.h + centre);
}

void OperatorWave::show_handle()
{
	handle->setVisible(true);
}

void OperatorWave::hide_handle()
{
	handle->setVisible(false);
}

void OperatorWave::clicked_handle(QGraphicsSceneMouseEvent *event)
{
	hide_handle();

	clicked_pos = mapFromScene(event->scenePos());
	clicked_old_pos = state.h;

	enter_drag_mode();
}

static inline int sq(int x)
{
	return x * x;
}

void OperatorWave::drag_handle(const QPointF &p, Qt::KeyboardModifiers modifiers)
{
	auto new_state = clone_state();
	QPoint &pos = new_state->h;

	pos = (p - clicked_pos + clicked_old_pos).toPoint();

	// If control is pressed, snap to either horizontal, vertical or diagonal
	if (modifiers & Qt::ControlModifier) {
		// Calculate square distance to lines
		int dist_hor = sq(pos.y());
		int dist_ver = sq(pos.x());
		int dist_diag1 = sq(pos.x() - pos.y());
		int dist_diag2 = sq(pos.x() + pos.y());

		// Snap to closest line
		if (dist_hor <= dist_ver && dist_hor <= dist_diag1 && dist_hor <= dist_diag2) {
			pos.setY(0);
		} else if (dist_ver <= dist_diag1 && dist_ver <= dist_diag2) {
			pos.setX(0);
		} else if (dist_diag1 <= dist_diag2) {
			int x = (pos.x() + pos.y()) / 2;
			pos.setX(x);
			pos.setY(x);
		} else {
			int x = (pos.x() - pos.y()) / 2;
			pos.setX(x);
			pos.setY(-x);
		}
	}

	place_set_state_command("Modify wave", std::move(new_state), !dont_accumulate_undo);
	dont_accumulate_undo = false;
}

void OperatorWave::restore_handles()
{
	show_handle();
	dont_accumulate_undo = true;
}

size_t OperatorWave::num_input() const
{
	return 0;
}

size_t OperatorWave::num_output() const
{
	return 1;
}

template <size_t N>
void OperatorWave::paint_quadrant_mag_phase(uint32_t *out, std::complex<double> *data, int start_x, int start_y,
					    double max_mag, double max_phase, double max)
{
	auto [factor1, factor2] = get_color_factors(ColorMode::LINEAR, max, 1.0);
	auto color_fn = get_color_lookup_function<std::complex<double>>(ColorType::RW, ColorMode::LINEAR);
	double v_x = state.h.x();
	double v_y = state.h.y();
	// We multiply in M_PI / 180.0 into the product so that we can directly get the sine.
	double act_prod = (v_x * start_x + v_y * start_y) * M_PI / 180.0;
	double step_x = v_x * M_PI / 180.0;
	// When stepping in y direction remove the whole x-increase.
	double step_y = (v_y - N * v_x) * M_PI / 180.0;

	for (size_t i = 0; i < N; ++i) {
		for (size_t j = 0; j < N; ++j) {
			//double prod = v_x * act_x + v_y * act_y;
			act_prod += step_x;
			double v = cos(act_prod);
			double phase = v * max_phase;
			std::complex<double> c = v * max_mag;
			c *= std::polar(1.0, phase);
			*data++ = c;
			*out++ = (*color_fn)(c, factor1, factor2);
		}
		act_prod += step_y;
		out += N;
		data += N;
	}
}

template <size_t N>
void OperatorWave::paint_quadrant_long_trans(uint32_t *out, std::complex<double> *data, int start_x, int start_y,
					     double max_re, double max_im, double max)
{
	auto [factor1, factor2] = get_color_factors(ColorMode::LINEAR, max, 1.0);
	auto color_fn = get_color_lookup_function<std::complex<double>>(ColorType::RW, ColorMode::LINEAR);
	double v_x = state.h.x();
	double v_y = state.h.y();
	// We multiply M_PI / 180.0 into the product so that we can directly get the sinus.
	double act_prod = (v_x * start_x + v_y * start_y) * M_PI / 180.0;
	double step_x = v_x * M_PI / 180.0;
	// When stepping in y direction remove the whole x-increase.
	double step_y = (v_y - N * v_x) * M_PI / 180.0;

	for (size_t i = 0; i < N; ++i) {
		for (size_t j = 0; j < N; ++j) {
			act_prod += step_x;
			double v = cos(act_prod);
			std::complex<double> c(v * max_re, v * max_im);
			*data++ = c;
			*out++ = (*color_fn)(c, factor1, factor2);
		}
		act_prod += step_y;
		out += N;
		data += N;
	}
}

template <size_t N>
void OperatorWave::calculate()
{
	uint32_t *out = imagebuf.get();
	std::complex<double> *data = output_buffers[0].get_complex_data();

	if (state.mode == OperatorWaveMode::mag_phase) {
		double max_mag = state.amplitude_mag * max_amplitude;
		double max_phase = state.amplitude_phase * M_PI / 2.0;
		double max = max_mag;

		paint_quadrant_mag_phase<N/2>(out, data + N/2 + N*N/2, -int(N)/2, -int(N)/2, max_mag, max_phase, max);	// Top left
		paint_quadrant_mag_phase<N/2>(out + N/2, data  + N*N/2, 0, -int(N)/2, max_mag, max_phase, max);		// Top right
		paint_quadrant_mag_phase<N/2>(out + N*N/2, data  + N/2, -int(N)/2, 0, max_mag, max_phase, max);		// Bottom left
		paint_quadrant_mag_phase<N/2>(out + N/2 + N*N/2, data, 0, 0, max_mag, max_phase, max);			// Bottom right
		output_buffers[0].set_extremes(Extremes(sq(max_mag)));
	} else {
		// Longitudinal and transversal maximum vectors vectors
		double v_x = state.h.x();
		double v_y = state.h.y();
		double len = sqrt(v_x*v_x + v_y*v_y);
		double long_re = v_x / len * state.amplitude_mag * max_amplitude;
		double long_im = v_y / len * state.amplitude_mag * max_amplitude;
		double trans_re = v_y / len * state.amplitude_phase * max_amplitude;
		double trans_im = v_x / len * state.amplitude_phase * max_amplitude;
		double max_re = long_re + trans_re;
		double max_im = long_im + trans_im;
		double max_norm = sq(max_re) + sq(max_im);
		double max = sqrt(max_norm);

		paint_quadrant_long_trans<N/2>(out, data + N/2 + N*N/2, -int(N)/2, -int(N)/2, max_re, max_im, max);	// Top left
		paint_quadrant_long_trans<N/2>(out + N/2, data  + N*N/2, 0, -int(N)/2, max_re, max_im, max);		// Top right
		paint_quadrant_long_trans<N/2>(out + N*N/2, data  + N/2, -int(N)/2, 0, max_re, max_im, max);		// Bottom left
		paint_quadrant_long_trans<N/2>(out + N/2 + N*N/2, data, 0, 0, max_re, max_im, max);			// Bottom right

		output_buffers[0].set_extremes(Extremes(max_norm));
	}

	QImage image(reinterpret_cast<unsigned char *>(imagebuf.get()),
		     N, N, QImage::Format_RGB32);
	setPixmap(QPixmap::fromImage(image));
}

void OperatorWave::paint_wave()
{
	dispatch_calculate(*this);

	paint_basis();
}

void OperatorWave::update_buffer()
{
	// Execute children
	execute_topo();
}

void OperatorWave::switch_mode(OperatorWaveMode mode)
{
	if (state.mode == mode)
		return;

	auto new_state = clone_state();
	new_state->mode = mode;
	place_set_state_command("Set wave mode", std::move(new_state), false);
}

void OperatorWave::set_amplitude_mag(double v)
{
	auto new_state = clone_state();
	new_state->amplitude_mag = v;
	place_set_state_command("Set wave magnitude", std::move(new_state), !dont_accumulate_undo);
	dont_accumulate_undo = false;
}

void OperatorWave::set_amplitude_phase(double v)
{
	auto new_state = clone_state();
	new_state->amplitude_phase = v;
	place_set_state_command("Set wave phase", std::move(new_state), !dont_accumulate_undo);
	dont_accumulate_undo = false;
}

void OperatorWave::set_scrollers()
{
	scroller_mag->set_val(state.amplitude_mag);
	scroller_phase->set_val(state.amplitude_phase);
}

void OperatorWave::clear()
{
	auto new_state = std::make_unique<OperatorWaveState>();
	place_set_state_command("Reset wave", std::move(new_state), !dont_accumulate_undo);
}

bool OperatorWave::input_connection_changed()
{
	// There are no input connections
	assert(false);
}

void OperatorWave::execute()
{
	// There are no input connections
	assert(false);
}
