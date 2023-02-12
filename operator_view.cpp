// SPDX-License-Identifier: GPL-2.0
#include "operator_view.hpp"
#include "document.hpp"
#include "scramble.hpp"
#include "globals.hpp"

#include <QFileDialog>
#include <QGraphicsScene>
#include <QMessageBox>

#include <array>

// Minimum and maximum scales of the color modes
// and whether the scroller is logrithmic.
struct ScaleDesc {
	double min, max;
	bool log_scroller;
};
static constexpr ScaleDesc linear_scale = { 1.0, 1e8, true };
static constexpr ScaleDesc root_scale = { 1.0, 100.0, true };
static constexpr ScaleDesc log_scale = { 2.0, 100.0, true };

OperatorViewState::OperatorViewState()
	: directory(Globals::get_last_save_image_directory())
{
}

QJsonObject OperatorViewState::to_json() const
{
	QJsonObject res;
	res["scale"] = scale;
	res["color_type"] = static_cast<int>(color_type);
	res["mode"] = static_cast<int>(mode);
	return res;
}

void OperatorViewState::from_json(const QJsonObject &desc)
{
	scale = desc["scale"].toDouble();
	color_type = static_cast<ColorType>(desc["color_type"].toInt());
	if (desc.contains("mode"))
		mode = static_cast<ColorMode>(desc["mode"].toInt());
}


static ScaleDesc get_scale_desc(ColorMode mode)
{
	switch (mode) {
		case ColorMode::LINEAR:
		default:
			return linear_scale;
		case ColorMode::ROOT:
			return root_scale;
		case ColorMode::LOG:
			return log_scale;
	}
}

void OperatorView::state_reset()
{
	ScaleDesc desc = get_scale_desc(state.mode);
	text->setPlainText(get_scale_text());
	scroller->reset(desc.min, desc.max, desc.log_scroller, state.scale);
	color_menu->set_pixmap(static_cast<int>(state.color_type));
	mode_menu->set_pixmap(static_cast<int>(state.mode));
	execute();
}

void OperatorView::init()
{
	size_t n = get_fft_size();
	imagebuf = AlignedBuf<uint32_t>(n * n);

	dont_accumulate_undo = true;

	// Show this first to set correct size
	// TODO: Should this be codified (i.e. buttons are added in a function
	// after init() and init() must initialize the bitmap)?
	show_empty();

	new Button(":/icons/save.svg", "Save pixmap as PNG", [this](){ save_file(); }, Side::left, this);
	color_menu = make_color_menu(this, [this](ColorType t){ switch_color(t); }, Side::right);
	mode_menu = new MenuButton(Side::right, "Set rendering type", this);
	mode_menu->add_entry(":/icons/color_mode_linear.svg", "Linear", [this](){ switch_mode(ColorMode::LINEAR); });
	mode_menu->add_entry(":/icons/color_mode_root.svg", "Root", [this](){ switch_mode(ColorMode::ROOT); });
	mode_menu->add_entry(":/icons/color_mode_log.svg", "Logarithm", [this](){ switch_mode(ColorMode::LOG); });
	scroller = new Scroller(linear_scale.min, linear_scale.max, linear_scale.log_scroller, [this](double v){ set_scale(v); }, this);

	text = add_text_line();
	text->setPlainText("1:1");
}

bool OperatorView::input_connection_changed()
{
	return false;
}

void OperatorView::show_empty()
{
	size_t n = get_fft_size();
	QPixmap empty_pixmap(n, n);
	empty_pixmap.fill(Qt::black);
	setPixmap(empty_pixmap);
}

template<size_t N, typename T>
void OperatorView::calculate_doit()
{
	FFTBuf &buf = input_connectors[0]->get_buffer();
	uint32_t *out = imagebuf.get();
	double max = sqrt(buf.get_max_norm());
	auto [factor1, factor2] = get_color_factors(state.mode, max, state.scale);
	const T *in = buf.get_data<T>();
	uint32_t (*fun)(T, double, double) = get_color_lookup_function<T>(state.color_type, state.mode);

	scramble<N, T, uint32_t>
		(in, out, [factor1, factor2 ,fun](T c)
		{ return (*fun)(c, factor1, factor2); });
}

template<size_t N>
void OperatorView::calculate()
{
	if (input_connectors[0]->is_empty_buffer()) {
		show_empty();
		return;
	}

	FFTBuf &buf = input_connectors[0]->get_buffer();

	if (buf.is_complex())
		calculate_doit<N, std::complex<double>>();
	else
		calculate_doit<N, double>();

	QImage image(reinterpret_cast<unsigned char *>(imagebuf.get()),
		     N, N, QImage::Format_RGB32);
	setPixmap(QPixmap::fromImage(image));
}

void OperatorView::execute()
{
	dispatch_calculate(*this);
}

static double round_to_digits(double v, int digits)
{
	double factor = pow(10.0, static_cast<double>(digits));
	return std::round(v * factor) / factor;
}

QString OperatorView::get_scale_text() const
{
	switch (state.mode) {
	case ColorMode::LINEAR:
	default:
		return QStringLiteral("1:%1").arg(round_to_digits(state.scale, 3));
	case ColorMode::ROOT:
		return QStringLiteral("Exponent: %1").arg(round_to_digits(state.scale, 2));
	case ColorMode::LOG:
		return QStringLiteral("Base: %1").arg(round_to_digits(state.scale, 2));
	}
}

void OperatorView::restore_handles()
{
	dont_accumulate_undo = true;
}

void OperatorView::set_scale(double scale)
{
	auto new_state = clone_state();

	ScaleDesc desc = get_scale_desc(state.mode);
	new_state->scale = std::clamp(scale, desc.min, desc.max);
	place_set_state_command("Set view scale", std::move(new_state), !dont_accumulate_undo);

	dont_accumulate_undo = false;
}

void OperatorView::switch_color(ColorType type)
{
	if (state.color_type == type)
		return;

	auto new_state = clone_state();
	new_state->color_type = type;

	place_set_state_command("Set view color", std::move(new_state), false);
}

void OperatorView::switch_mode(ColorMode mode)
{
	if (state.mode == mode)
		return;

	auto new_state = clone_state();
	new_state->mode = mode;
	switch (mode) {
	case ColorMode::LINEAR:
	default:
		new_state->scale = 1.0;
		break;
	case ColorMode::ROOT:
		new_state->scale = 2.0;
		break;
	case ColorMode::LOG:
		new_state->scale = 10.0;
		break;
	}

	place_set_state_command("Set view mode", std::move(new_state), false);
}

void OperatorView::save_file()
{
	QString filename = QFileDialog::getSaveFileName(nullptr, "Save Image", state.directory, "PNG Image (*.png)");
	if (filename.isEmpty())
		return;

	if (!pixmap().save(filename, "PNG"))
		QMessageBox::warning(nullptr, "Error", "Couldn't save image");

	Globals::set_last_save_image(filename);
}
