// SPDX-License-Identifier: GPL-2.0
#include "document.hpp"
#include "globals.hpp"
#include "scene.hpp"
#include "operator.hpp"
#include "mainwindow.hpp"
#include "edge.hpp"

#include <QAction>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUndoStack>
#include <QUndoCommand>

Document::Document(const Document *previous_document, MainWindow &w)
	: undo_stack(new QUndoStack)
	, fft_size(256)
{
	static int number = 0;
	name = "New document " + QString::number(++number);

	QObject::connect(undo_stack.get(), &QUndoStack::cleanChanged, [&w]() { w.set_title(); });

	if (previous_document)
		fft_size = previous_document->fft_size;
}

Document::~Document()
{
}

QString Document::get_directory() const
{
	if (filename.isEmpty()) {
		return Globals::get_file_directory();
	} else {
		QFileInfo info(filename);
		return info.path();
	}
}

bool Document::save(MainWindow *w, Scene *scene)
{
	if (filename.isEmpty())
		return save_as(w, scene);
	else
		return save(filename, w, scene);
}

bool Document::save_as(MainWindow *w, Scene *scene)
{

	QString fn = QFileDialog::getSaveFileName(nullptr, "Save File", get_directory(),
						  "XFFT Files (*.xfft)");
	return save(fn, w, scene);
}

bool Document::save(const QString &fn, MainWindow *w, Scene *scene)
{
	QFile out(fn);
	if (!out.open(QIODevice::WriteOnly)) {
		QMessageBox::warning(nullptr, "Error", "Couldn't open file for output.");
		return false;
	}

	QJsonObject json;

	// Fill global data
	json["fft_size"] = static_cast<int>(fft_size);
	QPoint scroll_pos = scene->get_scroll_position();
	json["scroll_x"] = scroll_pos.x();
	json["scroll_y"] = scroll_pos.y();
	json["size_x"] = w->size().width();
	json["size_y"] = w->size().height();

	// Write operators
	{
		QJsonArray ops;
		for (const Operator *op: topo.get_operators())
			ops.push_back(op->to_json());
		json["operators"] = ops;
	}

	// Write edges
	{
		QJsonArray edges;
		for (const Operator *op: topo.get_operators())
			op->out_edges_to_json(edges);
		json["edges"] = edges;
	}

	QJsonDocument json_doc(json);
	if (!out.write(json_doc.toJson())) {
		QMessageBox::warning(nullptr, "Error", "Couldn't write to file.");
		out.close();
		QFile::remove(fn);
		return false;
	}
	set_filename(fn);
	undo_stack->setClean();
	return true;
}

void Document::load(MainWindow *w, Scene *scene)
{
	QString fn = QFileDialog::getOpenFileName(nullptr, "Open File", get_directory(),
						  "XFFT Files (*.xfft)");
	if (fn.isEmpty())
		return;
	load(w, scene, fn);
}

void Document::load_example(MainWindow *w, Scene *scene, const char *id)
{
	QString fn = QStringLiteral(":/examples/%1.xfft").arg(id);
	QFile in(fn);
	if (!in.open(QIODevice::ReadOnly)) {
		QMessageBox::warning(nullptr, "Error", "Can't access example file (shouldn't happen!).");
		return;
	}

	load(w, scene, in, QString());
}

void Document::load(MainWindow *w, Scene *scene, const QString &fn)
{
	QFile in(fn);
	if (!in.open(QIODevice::ReadOnly)) {
		QMessageBox::warning(nullptr, "Error", "Couldn't open file.");
		return;
	}

	{
		QFileInfo info(in);
		if (MainWindow *w = MainWindow::find_window(info)) {
			w->raise();
			return;
		}
	}

	load(w, scene, in, fn);
}

void Document::load(MainWindow *w, Scene *scene, QFile &in, const QString &fn)
{
	// If there are changes, load into a new window, otherwise continue
	// loading into the same window
	if (changed()) {
		auto new_window = std::make_unique<MainWindow>(this);
		Document *d = &new_window->get_document();

		// If loading succeeded, let the window live, otherwise close it
		// (implicitly, by letting new_window go out of scope).
		if (d->load_doit(new_window.get(), &new_window->get_scene(), in, fn)) {
			new_window->show();
			new_window.release();
		}
	} else {
		clear();
		scene->clear();

		load_doit(w, scene, in, fn);
	}
}

// Return true on success
bool Document::load_doit(MainWindow *w, Scene *scene, QFile &in, const QString &fn)
{
	QByteArray data = in.readAll();
	QJsonDocument json_doc = QJsonDocument::fromJson(data);
	QJsonObject json = json_doc.object();

	size_t fft_size = static_cast<size_t>(json["fft_size"].toInt());
	if (std::find(std::begin(supported_fft_sizes), std::end(supported_fft_sizes), fft_size)
			== std::end(supported_fft_sizes)) {
		QMessageBox::warning(nullptr, "Error", "No or invalid FFT size");
		return false;
	}

	QSize size(json["size_x"].toInt(), json["size_y"].toInt());
	QPoint scroll_pos(json["scroll_x"].toInt(), json["scroll_y"].toInt());

	w->resize(size);
	scene->set_scroll_position(scroll_pos);
	change_fft_size(fft_size, scene);

	// Load operators
	{
		QJsonArray ops = json["operators"].toArray();
		for (const QJsonValue &op_desc: ops) {
			Operator *op = Operator::from_json(*w, op_desc.toObject());
			if (!op) {
				QMessageBox::warning(nullptr, "Error", "Invalid operator");
				return false;
			}
		}
	}

	// Load edges
	{
		QJsonArray edges = json["edges"].toArray();
		for (const QJsonValue &v: edges) {
			QJsonObject desc = v.toObject();
			Operator *op_from = topo.get_by_id(desc["op_from"].toInt());
			Operator *op_to = topo.get_by_id(desc["op_to"].toInt());
			if (!op_from || !op_to) {
				QMessageBox::warning(nullptr, "Error", "Invalid edge");
				return false;
			}
			Connector &conn_from = op_from->get_output_connector(desc["conn_from"].toInt());
			Connector &conn_to = op_to->get_input_connector(desc["conn_to"].toInt());

			auto e = std::make_unique<Edge>(&conn_from, &conn_to, *this);
			scene->addItem(&*e);
			e->recalculate();
			e->add_connection();
			e.release();
		}
	}

	// Update buffers and execute
	topo.update_all_buffers();
	topo.execute_all();

	if (!fn.isEmpty())
		set_filename(fn);

	w->set_title();

	undo_stack->setClean();

	return true;
}

void Document::set_filename(const QString &fn)
{
	Globals::set_last_file(fn);
	filename = fn;
	QFileInfo info(fn);
	name = info.completeBaseName();
}

void Document::clear()
{
	undo_stack->clear();
	operator_list.clear();
	topo.clear();
}

bool Document::change_fft_size(size_t size, Scene *scene)
{
	if (operator_list.num_operators() > 0) {
		QMessageBox box(QMessageBox::Question, "Clear?",
				"Size change will clear the canvas.\n"
				"Are you sure that you want to continue?",
				QMessageBox::Yes|QMessageBox::No);
		if (box.exec() != QMessageBox::Yes)
			return false;
	}

	clear();
	scene->clear();

	fft_size = size;
	return true;
}

void Document::place_command_internal(QUndoCommand *cmd)
{
	undo_stack->push(cmd);
}

QAction *Document::undo_action(QObject *parent) const
{
	return undo_stack->createUndoAction(parent);
}

QAction *Document::redo_action(QObject *parent) const
{
	return undo_stack->createRedoAction(parent);
}

bool Document::changed() const
{
	return !undo_stack->isClean();
}
