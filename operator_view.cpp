// SPDX-License-Identifier: GPL-2.0
#include "operator_view.hpp"
#include "document.hpp"
#include "scramble.hpp"
#include "globals.hpp"

#include <QFileDialog>
#include <QGraphicsScene>
#include <QMessageBox>

#include <array>

OperatorViewState::OperatorViewState()
	: directory(Globals::get_last_save_image_directory())
{
}

QJsonObject OperatorViewState::to_json() const
{
	QJsonObject res;
	res["scale"] = scale;
	res["color_type"] = static_cast<int>(color_type);
	return res;
}

void OperatorViewState::from_json(const QJsonObject &desc)
{
	scale = desc["scale"].toDouble();
	color_type = static_cast<ColorType>(desc["color_type"].toInt());
}

void OperatorView::state_reset()
{
	update_scale_text();
	scroller->set_val(state.scale);
	mode_menu->set_pixmap((int)state.color_type);
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
	mode_menu = make_color_menu(this, [this](ColorType t){ switch_mode(t); }, Side::right);
	scroller = new Scroller(1.0, 10e7, true, [this](double v) { set_scale(v); }, this);

	text = add_text_line();
	text->setPlainText("1:1");
}

size_t OperatorView::num_input() const
{
	return 1;
}

size_t OperatorView::num_output() const
{
	return 0;
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
	double factor = state.scale / sqrt(buf.get_max_norm());
	const T *in = buf.get_data<T>();
	uint32_t (*fun)(T, double) = get_color_lookup_function<T>(state.color_type);

	scramble<N, T, uint32_t>
		(in, out, [factor,fun](T c)
		{ return (*fun)(c, factor); });
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

static double round_to_3(double v)
{
	return std::round(v * 1000.0) / 1000.0;
}

void OperatorView::update_scale_text()
{
	double rounded_scale = round_to_3(state.scale);
	QString str = QString("1:%1").arg(rounded_scale);

	text->setPlainText(str);
}

void OperatorView::restore_handles()
{
	dont_accumulate_undo = true;
}

void OperatorView::set_scale(double scale_)
{
	auto new_state = clone_state();

	// Only scales >= 1.0 are supported
	new_state->scale = std::max(1.0, scale_);
	place_set_state_command("Set view scale", std::move(new_state), !dont_accumulate_undo);

	dont_accumulate_undo = false;
}

void OperatorView::switch_mode(ColorType type)
{
	if (state.color_type == type)
		return;

	auto new_state = clone_state();
	new_state->color_type = type;

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
