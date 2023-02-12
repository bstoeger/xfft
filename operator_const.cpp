// SPDX-License-Identifier: GPL-2.0
#include "operator_const.hpp"
#include "document.hpp"
#include "aligned_buf.hpp"      // For assume_aligned

#include <QGraphicsSceneMouseEvent>
#include <QMenu>

#include <cassert>

OperatorConst::OperatorConst(MainWindow &w)
	: OperatorTemplate(w)
	, imagebuf(size * size)
	, current_color_type((ColorType)-1)
{
}

void OperatorConst::init()
{
	// Paint handle above buttons
	handle = new Handle("Click and drag to change value", this);
	handle->setZValue(2.0);
	dont_accumulate_undo = true;

	// Make image buffer black
	uint32_t *out = assume_aligned(imagebuf.get());
	for (size_t i = 0; i < size * size; ++i)
		*out++ = qRgb(0.0, 0.0, 0.0);

	// Paint image before adding ruler, so that correct size is set.
	paint_image();

	scroller_scale = new Scroller(0.01, 100.0, true, [this](double v) { set_scale(v); }, this);
	add_button_new_line();

	new Button(":/icons/reset.svg", "Reset to 0", [this](){clear();}, Side::left, this);
	new TextButton(("1"), "Set to 1", [this]() { set({ 1.0, 0.0 }); }, Side::left, this);
	new TextButton(("-1"), "Set to -1", [this]() { set({ -1.0, 0.0 }); }, Side::left, this);
	new TextButton(("i"), "Set to i", [this]() { set({ 0.0, 1.0 }); }, Side::left, this);
	new TextButton(("-i"), "Set to -i", [this]() { set({ 0.0, -1.0 }); }, Side::left, this);
	mode_menu = make_color_menu(this, [this](ColorType t) { switch_mode(t); }, Side::left);

	text = add_text_line();
	text->setPlainText("1");
}

QJsonObject OperatorConstState::to_json() const
{
	QJsonObject res;
	res["color_type"] = static_cast<int>(color_type);
	res["v_real"] = v.real();
	res["v_imag"] = v.imag();
	return res;
}

void OperatorConstState::from_json(const QJsonObject &desc)
{
	color_type = static_cast<ColorType>(desc["color_type"].toInt());
	v = std::complex<double>(desc["v_real"].toDouble(), desc["v_imag"].toDouble());
}

void OperatorConst::state_reset()
{
	if (current_color_type != state.color_type)
		paint_image();
	calculate();
	place_handle();
	set_scroller();
}

void OperatorConst::paint_image()
{
	current_color_type = state.color_type; // save old color type to avoid useless repainting of the image
	make_color_wheel(imagebuf, size, scale, state.color_type);

	uint32_t *out = imagebuf.get();
	QImage image(reinterpret_cast<unsigned char *>(out),
		     size, size, QImage::Format_RGB32);
	setPixmap(QPixmap::fromImage(image));
}

void OperatorConst::placed()
{
	make_output_complex(0);
	calculate();
	place_handle();
	set_scroller();
}

static double round_to_3(double v)
{
	return std::round(v * 1000.0) / 1000.0;
}

void OperatorConst::calculate()
{
	size_t fft_size = get_fft_size();
	size_t n = fft_size * fft_size;
	std::complex<double> *buf = assume_aligned(output_buffers[0].get_complex_data());
	std::complex<double> v = state.v * state.scale;
	for (size_t i = 0; i < n; ++i)
		*buf++ = v;
	output_buffers[0].set_extremes(state.scale);

	// Format string
	double real_disp = round_to_3(v.real());
	double imag_disp = round_to_3(v.imag());
	bool has_real = std::abs(real_disp) > 0.0005;
	bool has_imag = std::abs(imag_disp) > 0.0005;
	QString str;
	if (has_real && has_imag) {
		if (std::abs(imag_disp - 1.0) < 0.0005) {
			str = QStringLiteral("%1+i").arg(real_disp);
		} else if (std::abs(imag_disp + 1.0) < 0.0005) {
			str = QStringLiteral("%1-i").arg(real_disp);
		} else if (v.imag() >= 0.0) {
			str = QStringLiteral("%1+%2i").arg(real_disp)
						      .arg(imag_disp);
		} else {
			str = QStringLiteral("%1%2i").arg(real_disp)
						     .arg(imag_disp);
		}
	} else if (has_real) {
		str = QStringLiteral("%1").arg(real_disp);
	} else if (has_imag) {
		if (std::abs(imag_disp - 1.0) < 0.0005)
			str = QStringLiteral("i");
		else if (std::abs(imag_disp + 1.0) < 0.0005)
			str = QStringLiteral("-i");
		else
			str = QStringLiteral("%1i").arg(imag_disp);
	} else {
		str = "0";
	}

	text->setPlainText(str);

	// Execute children
	execute_topo();
}

void OperatorConst::place_handle()
{
	double pos_x = state.v.real() * size / scale / 2.0 + size / 2.0;
	double pos_y = -state.v.imag() * size / scale / 2.0 + size / 2.0;
	handle->set_pos({ pos_x, pos_y });
}

bool OperatorConst::handle_click(QGraphicsSceneMouseEvent *event)
{
	if (event->button() != Qt::LeftButton)
		return false;
	enter_drag_mode();
	drag_handle(mapFromScene(event->scenePos()), event->modifiers());
	return true;
}

void OperatorConst::drag_handle(const QPointF &p, Qt::KeyboardModifiers)
{
	auto new_state = clone_state();
	double x_rel = (p.x() - size / 2.0) / size * 2.0 * scale;
	double y_rel = -(p.y() - size / 2.0) / size * 2.0 * scale;
	std::complex<double> v(x_rel, y_rel);
	if (std::norm(v) > 1.0)
		v = std::polar(1.0, std::arg(v));
	set(v);
	dont_accumulate_undo = false;
}

void OperatorConst::set(std::complex<double> v)
{
	auto new_state = clone_state();
	new_state->v = v;
	place_set_state_command("Set constant", std::move(new_state), !dont_accumulate_undo);
}

void OperatorConst::restore_handles()
{
	dont_accumulate_undo = true;
}

void OperatorConst::switch_mode(ColorType type)
{
	auto new_state = clone_state();
	new_state->color_type = type;
	place_set_state_command("Change constant mode", std::move(new_state), false);
}

void OperatorConst::set_scale(double scale)
{
	auto new_state = clone_state();
	new_state->scale = scale;
	place_set_state_command("Set constant magnitude", std::move(new_state), !dont_accumulate_undo);
	dont_accumulate_undo = false;
}

void OperatorConst::set_scroller()
{
	scroller_scale->set_val(state.scale);
}

void OperatorConst::clear()
{
	auto new_state = std::make_unique<OperatorConstState>();
	place_set_state_command("Reset constant", std::move(new_state), false);
}
