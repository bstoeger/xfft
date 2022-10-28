// SPDX-License-Identifier: GPL-2.0
#include "mainwindow.hpp"
#include "about.hpp"
#include "document.hpp"
#include "operator.hpp"
#include "globals.hpp"
#include "examples.hpp"

#include <QActionGroup>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>

#include <cassert>

std::list<MainWindow *> MainWindow::windows;

MainWindow::MainWindow(const Document *previous_document)
{
	document = std::make_unique<Document>(previous_document, *this);
	set_title();

	setAttribute(Qt::WA_DeleteOnClose);

	QMenu *file_menu = menuBar()->addMenu("File");
	add_file_menu_item("document-new", "New", &MainWindow::new_window, file_menu);
	add_file_menu_item("document-open", "Open", &MainWindow::open, file_menu);
	recent_file_menu = file_menu->addMenu("Open recent");
	add_file_menu_item("document-save", "Save", &MainWindow::save_action, file_menu);
	add_file_menu_item("document-save-as", "Save as", &MainWindow::save_as, file_menu);
	add_file_menu_item("window-close", "Close", &MainWindow::close_action, file_menu);
	populate_recent_file_menu();

	QMenu *edit_menu = menuBar()->addMenu("Edit");
	QAction *undo_action = document->undo_action(this);
	QAction *redo_action = document->redo_action(this);
	undo_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
	redo_action->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z));
	undo_action->setIcon(QIcon::fromTheme("edit-undo"));
	redo_action->setIcon(QIcon::fromTheme("edit-redo"));
	edit_menu->addAction(undo_action);
	edit_menu->addAction(redo_action);

	delete_action = new QAction(QIcon::fromTheme("edit-delete"), QStringLiteral("Delete"), this);
	delete_action->setShortcut(Qt::Key_Delete);
	delete_action->setEnabled(false);
	connect(delete_action, &QAction::triggered, [this]() { scene->delete_selection(); });
	edit_menu->addAction(delete_action);

	QMenu *add_menu = menuBar()->addMenu("Add");
	QToolBar *toolbar = addToolBar("Toolbar");

	for (auto const &desc: operator_factory.get_descs()) {
		OperatorId id = desc.id;
		if (desc.init_states.empty()) {
			add_icon(desc.icon, desc.tooltip,
				 [this, id]() { add_op(id); },
				 add_menu, toolbar);
		} else {
			add_operator_menu(desc, add_menu, toolbar);
		}
		if (desc.add_separator)
			toolbar->addSeparator();
	}

	toolbar->addSeparator();
	add_icon(":/icons/magnifier.svg", "Magnify",
		 [this]() { add_magnifier(); },
		 add_menu, toolbar);

	size_menu = menuBar()->addMenu("FFT Size");
	QActionGroup *size_group = new QActionGroup(this);
	for (size_t size: Document::supported_fft_sizes)
		add_size_menu_item(size, size_menu, size_group, document->fft_size);

	QMenu *examples_menu = menuBar()->addMenu("Examples");
	examples_menu->setToolTipsVisible(true);
	for (auto [id, name, description]: examples.get_descs())
		add_examples_menu_item(examples_menu, id, name, description);

	{
		QMenu *help_menu = menuBar()->addMenu("Help");
		QAction *act = new QAction("About xfft", this);
		help_menu->addAction(act);
		connect(act, &QAction::triggered, [this] { show_about_dialog(this); });

		act = new QAction("About license", this);
		help_menu->addAction(act);
		connect(act, &QAction::triggered, [this] { show_gpl_dialog(this); });

		act = new QAction("About Qt", this);
		help_menu->addAction(act);
		connect(act, &QAction::triggered, [this] { QMessageBox::aboutQt(this); });
	}

	scene = new Scene(*this, this);
	scene->setSceneRect(QRectF(0, 0, 5000, 5000));
	connect(scene, &Scene::selection_changed, this, &MainWindow::selection_changed);

	view = new QGraphicsView(scene);
	view->setMouseTracking(true);
	setCentralWidget(view);

	status_bar = new QStatusBar;
	setStatusBar(status_bar);

	// At last, when no exception can occur anymore, add a pointer to this window
	// to the window list
	windows.push_back(this);
	my_it = std::prev(windows.end());
}

MainWindow::~MainWindow()
{
	windows.erase(my_it);
}

void MainWindow::add_file_menu_item(const char *icon, const char *text, void (MainWindow::*fun)(), QMenu *menu)
{
	QAction *act = new QAction(QIcon::fromTheme(icon), text, this);
	connect(act, &QAction::triggered, this, fun);
	menu->addAction(act);
}

void MainWindow::add_icon(const char *img, const char *tooltip, const std::function<void(void)> &fun,
			  QMenu *menu, QToolBar *toolbar)
{
	QIcon icon(img);
	QAction *act = new QAction(icon, tooltip, this);
	act->setStatusTip(tooltip);
	connect(act, &QAction::triggered, this, fun);
	menu->addAction(act);
	toolbar->addAction(act);
}

void MainWindow::add_operator_menu(const OperatorFactory::Desc &desc, QMenu *menu, QToolBar *toolbar)
{
	QToolButton *button = new OperatorMenu(this, desc, menu);
	toolbar->addWidget(button);
}

MainWindow::OperatorMenu::OperatorMenu(MainWindow *parent_, const OperatorFactory::Desc &desc_, QMenu *menu)
	: QToolButton(parent_)
	, parent(parent_)
	, desc(desc_)
	, current_state(0)
{
	assert(desc.init_states.size() > 0);

	QIcon icon(desc.icon);
	connect(this, &QToolButton::clicked, this, &OperatorMenu::add_current_op);
	QMenu *submenu = new QMenu(desc.tooltip, this);
	size_t state_nr = 0;
	for (const Operator::InitState &s: desc.init_states) {
		QIcon icon(s.icon);
		QAction *act = new QAction(icon, s.name, this);
		connect(act, &QAction::triggered, [this, state_nr] { add_op(state_nr); });
		act->setStatusTip(s.name);
		submenu->addAction(act);
		++state_nr;
	}
	menu->addMenu(submenu);
	setMenu(submenu);
	setIcon(QIcon(desc.init_states[current_state].icon));
	setPopupMode(QToolButton::MenuButtonPopup);
}

void MainWindow::OperatorMenu::add_op(size_t state_nr)
{
	current_state = state_nr;
	setIcon(QIcon(desc.init_states[state_nr].icon));
	add_current_op();
}

void MainWindow::OperatorMenu::add_current_op()
{
	parent->add_op(desc.id, *desc.init_states[current_state].state);
}

void MainWindow::add_size_menu_item(size_t size, QMenu *menu, QActionGroup *group, size_t default_size)
{
	QString num = QString::number(size);
	QString tooltip = QStringLiteral("%1×%2").arg(num, num);
	QAction *act = new QAction(tooltip, this);
	act->setCheckable(true);
	if (size == default_size)
		act->setChecked(true);
	group->addAction(act);
	menu->addAction(act);
	connect(act, &QAction::triggered, this,
		[this,size] { set_fft_size(size); });
}

void MainWindow::add_examples_menu_item(QMenu *menu, const char *id, const char *name, const char *description)
{
	QAction *act = new QAction(name, this);
	act->setToolTip(description);
	menu->addAction(act);
	connect(act, &QAction::triggered, this,
		[this, id] { load_example(id); });
}

void MainWindow::populate_recent_file_menu()
{
	recent_file_menu->clear();

	QStringList files = Globals::get_recent_files();
	for (int i = 0; i < files.size(); ++i) {
		QAction *act = new QAction(files[i], this);
		recent_file_menu->addAction(act);
		connect(act, &QAction::triggered, this,
			[this,i] { open_recent(i); });
	}
}

Document &MainWindow::get_document()
{
	return *document;
}

const Document &MainWindow::get_document() const
{
	return *document;
}

Scene &MainWindow::get_scene()
{
	return *scene;
}

const Scene &MainWindow::get_scene() const
{
	return *scene;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (!close())
		event->ignore();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
	case Qt::Key_Escape:
		scene->enter_normal_mode();
		break;
	case Qt::Key_Delete:
		scene->delete_selection();
		break;
	}
}

void MainWindow::selection_changed(bool is_empty)
{
	delete_action->setEnabled(!is_empty);
}

void MainWindow::new_window()
{
	MainWindow *w = new MainWindow(&*document);
	w->show();
}

void MainWindow::open()
{
	document->load(this, scene);
	update_size_menu(document->fft_size);
}

void MainWindow::open(const QString &filename)
{
	document->load(this, scene, filename);
	update_size_menu(document->fft_size);
}

void MainWindow::open_recent(int i)
{
	QStringList files = Globals::get_recent_files();
	if (i < 0 || i >= files.size())
		return;
	open(files[i]);
	update_size_menu(document->fft_size);
}

void MainWindow::load_example(const char *id)
{
	document->load_example(this, scene, id);
	update_size_menu(document->fft_size);
}

bool MainWindow::close()
{
	if (document->changed()) {
		QMessageBox msg(QMessageBox::Warning, "Save changes?",
				"You have unsaved changes, which will be lost. Save them?",
				QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
				this);

		switch (msg.exec()) {
		case QMessageBox::Save:
			if (!save())
				return false;
			break;
		case QMessageBox::Discard:
			break;
		default:
			return false;
		}
	}

	document->clear();
	scene->clear();
	QMainWindow::close();
	return true;
}

void MainWindow::close_action()
{
	close();
}

bool MainWindow::save()
{
	if (!document->save(this, scene))
		return false;
	set_title();
	return true;
}

void MainWindow::save_action()
{
	save();
}

void MainWindow::save_as()
{
	document->save_as(this, scene);
	set_title();
}

void MainWindow::add_op(OperatorId id)
{
	scene->enter_add_object_mode(operator_factory.make(id, *this));
}

void MainWindow::add_op(OperatorId id, const Operator::State &init_state)
{
	scene->enter_add_object_mode(operator_factory.make(id, init_state, *this));
}

void MainWindow::add_magnifier()
{
	scene->enter_magnifier_mode();
}

void MainWindow::set_fft_size(size_t size)
{
	document->change_fft_size(size, scene);
}

void MainWindow::set_title()
{
	QString title = document->name;
	if (document->changed())
		title += QStringLiteral(" *");
	title += QStringLiteral(" - XFFT");
	setWindowTitle(title);
}

bool MainWindow::is_file(const QFileInfo &f) const
{
	const QString &fn = document->filename;
	if (fn.isEmpty())
		return false;
	QFileInfo f2(fn);
	return f.exists() && f2.exists() && f == f2;
}

MainWindow *MainWindow::find_window(const QFileInfo &f)
{
	auto it = std::find_if(windows.begin(), windows.end(),
			      [&f](const MainWindow *w)
			      { return w->is_file(f); });
	return it == windows.end() ? nullptr : *it;
}

void MainWindow::update_recent_files()
{
	for (MainWindow *w: windows)
		w->populate_recent_file_menu();
}

void MainWindow::update_size_menu(size_t size)
{
	for (size_t i = 0; i < std::size(Document::supported_fft_sizes); ++i) {
		if (Document::supported_fft_sizes[i] == size) {
			size_menu->actions()[i]->setChecked(true);
			break;
		}
	}
}

void MainWindow::show_tooltip(const QString &s)
{
	status_bar->showMessage(s);
}

void MainWindow::hide_tooltip()
{
	status_bar->clearMessage();
}
