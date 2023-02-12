// SPDX-License-Identifier: GPL-2.0
#include "operator_pixmap.hpp"
#include "document.hpp"
#include "globals.hpp"
#include "scramble.hpp"

#include <QFileDialog>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QByteArray>
#include <QIcon>
#include <QGraphicsSceneMouseEvent>

#include <cassert>

void OperatorPixmapState::init(size_t n_)
{
	n = n_;
	image = QImage(n, n, QImage::Format_Grayscale8);
	image.fill(0);
	directory = Globals::get_last_image_directory();
}

OperatorPixmap::OperatorPixmap(MainWindow &w)
	: OperatorTemplate(w)
{
	state.init(get_fft_size());
}

void OperatorPixmap::init()
{
	setPixmap(QPixmap::fromImage(state.image));

	new Button(":/icons/open.svg", "Load pixmap", [this](){ load_file(); }, Side::left, this);
	new Button(":/icons/reset.svg", "Clear", [this](){ clear(); }, Side::left, this);
	new Button(":/icons/inversion.svg", "Invert (rotate by 180°", [this](){ invert(); }, Side::left, this);
	brush_menu = make_brush_menu(this, [this](int size, bool antialias){ switch_brush(size, antialias); }, Side::left);
	dont_accumulate_undo = true;
}

QJsonObject OperatorPixmapState::to_json() const
{
	QJsonObject res;
	QByteArray data(reinterpret_cast<const char *>(image.constBits()), n*n);
	res["data"] = QString(data.toBase64());
	res["brush_size"] = brush_size;
	res["antialiasing"] = antialiasing;
	return res;
}

void OperatorPixmapState::from_json(const QJsonObject &desc)
{
	QString encoded = desc["data"].toString();
	if (encoded.isEmpty()) {
		image.fill(0);
	} else {
		QByteArray array = QByteArray::fromBase64(encoded.toLatin1());
		const unsigned char *data = reinterpret_cast<const unsigned char *>(array.data());
		unsigned char *bits = image.bits();
		for (size_t i = 0; i < n*n; ++i) {
			*bits++ = *data++;
		}
	}
	brush_size = desc["brush_size"].toInt();
	antialiasing = desc["antialiasing"].toBool();
}

void OperatorPixmap::state_reset()
{
	setPixmap(QPixmap::fromImage(state.image));
	brush_menu->set_pixmap(state.brush_size, state.antialiasing);
	update_buffers();
}

void OperatorPixmap::load_file()
{
	QString filename = QFileDialog::getOpenFileName(nullptr, "Open Image", state.directory, "Images (*.png *.xpm *.jpg)");
	if (filename.isEmpty()) {
		return;
	}

	auto new_state = clone_state();
	if (!new_state->image.load(filename)) {
		QMessageBox::warning(nullptr, "Error", "Couldn't load image");
		return;
	}
	size_t n = get_fft_size();
	new_state->image = new_state->image.scaled(n, n, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	new_state->image = new_state->image.convertToFormat(QImage::Format_Grayscale8, Qt::AvoidDither);
	//setPixmap(QPixmap::fromImage(new_state->image));
	Globals::set_last_image(filename);
	new_state->directory = Globals::get_last_image_directory();

	place_set_state_command("Load pixmap", std::move(new_state), false);
}

void OperatorPixmap::invert()
{
	auto new_state = clone_state();
	new_state->image = state.image.mirrored(true, true);
	place_set_state_command("Invert pixmap", std::move(new_state), false);
}

void OperatorPixmap::clear()
{
	auto new_state = clone_state();
	new_state->image.fill(0);
	place_set_state_command("Clear pixmap", std::move(new_state), false);
}

void OperatorPixmap::placed()
{
	make_output_real(0);
	update_buffers();
}

void OperatorPixmap::switch_brush(int size, bool antialiasing_)
{
	state.brush_size = size;
	state.antialiasing = antialiasing_;
}

bool OperatorPixmap::handle_click(QGraphicsSceneMouseEvent *event)
{
	if (event->button() != Qt::LeftButton && event->button() == Qt::RightButton)
		return false;

	enter_drag_mode();
	QColor c = event->button() == Qt::RightButton ? Qt::black : Qt::white;
	pen = QPen(c, state.brush_size, Qt::SolidLine, Qt::RoundCap);
	QPointF pos_f = mapFromScene(event->scenePos());
	pos = pos_f.toPoint();
	drag_handle(pos_f, event->modifiers());
	return true;
}

void OperatorPixmap::drag_handle(const QPointF &p, Qt::KeyboardModifiers)
{
	auto new_state = clone_state();
	QPoint next_pos = p.toPoint();
	painter.begin(&new_state->image);
	painter.setRenderHint(QPainter::Antialiasing, new_state->antialiasing);
	painter.setPen(pen);
	// Qt has a strange bug(?) whereby drawLine doesn't degenerate to a single point for some brushes.
	if (pos == next_pos)
		painter.drawPoint(pos);
	else
		painter.drawLine(pos, next_pos);
	painter.end();
	pos = next_pos;
	place_set_state_command("Draw on pixmap", std::move(new_state), !dont_accumulate_undo);

	dont_accumulate_undo = false;
}

void OperatorPixmap::restore_handles()
{
	dont_accumulate_undo = true;
}

template<size_t N>
void OperatorPixmap::calculate()
{
	const unsigned char *in = state.image.constBits();
	double *out = output_buffers[0].get_real_data();
	scramble<N, unsigned char, double>
		(in, out, [](unsigned char c) { return static_cast<double>(c) / 255.0; });
}

void OperatorPixmap::update_buffers()
{
	dispatch_calculate(*this);
	output_buffers[0].set_extremes(Extremes(1.0));

	setPixmap(QPixmap::fromImage(state.image));

	// Execute children
	execute_topo();
}
