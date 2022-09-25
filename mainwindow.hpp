// SPDX-License-Identifier: GPL-2.0
#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "mode.hpp"
#include "scene.hpp"
#include "document.hpp"
#include "operator_factory.hpp"

#include <QMainWindow>
#include <QToolButton>

#include <memory>

class QActionGroup;
class QFileInfo;
class QStatusBar;

class MainWindow : public QMainWindow
{
	// Maintain a list of of windows and an iterator to the own entry
	// so that we can remove it from the list.
	static std::list<MainWindow *> windows;
	std::list<MainWindow *>::iterator my_it;

	std::unique_ptr<Document> document;
	Scene *scene;
	QGraphicsView *view;

	// Check if this is the file opened in this window.
	bool is_file(const QFileInfo &f) const;

	// A pointer to the recent file menu, so we can repopulate it on change.
	QMenu *recent_file_menu;

	// A pointer to the fft-size file menu, so we can update it on change.
	QMenu *size_menu;

	// Delete action. Enabled if one or more items are selected.
	QAction *delete_action;

	QStatusBar *status_bar;

	// Populate recent file menu with strings from the settings.
	void populate_recent_file_menu();

	// Menu actions
	void new_window();
	void open();
	void open_recent(int nr);
	bool close(); // returns true if file was actually closed
	void close_action();
	bool save(); // returns true if file was actually saved
	void save_action();
	void save_as();
	void add_op(OperatorId);
	void add_op(OperatorId, const Operator::State &init_state);
	void add_magnifier();
	void set_fft_size(size_t size);
	void update_size_menu(size_t size);
	void load_example(const char *id);

	// Events
	void closeEvent(QCloseEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;

	void add_icon(const char *icon, const char *tooltip, const std::function<void(void)> &fun,
		      QMenu *menu, QToolBar *toolbar);
	void add_operator_menu(const OperatorFactory::Desc &desc, QMenu *menu, QToolBar *toolbar);
	void add_file_menu_item(const char *icon, const char *text, void (MainWindow::*fun)(), QMenu *menu);
	void add_size_menu_item(size_t size, QMenu *menu, QActionGroup *group, size_t default_size);
	void add_examples_menu_item(QMenu *menu, const char *id, const char *name, const char *description);

	class OperatorMenu : public QToolButton {
		MainWindow *parent;
		const OperatorFactory::Desc &desc;
		size_t current_state;
		void add_op(size_t state_nr);
		void add_current_op();
	public:
		OperatorMenu(MainWindow *parent, const OperatorFactory::Desc &desc, QMenu *menu);
	};
public:
	MainWindow(const Document *previous_document);
	~MainWindow();

	Document &get_document();
	const Document &get_document() const;
	Scene &get_scene();
	const Scene &get_scene() const;

	// TODO: remove last parameter
	void open(const QString &filename);

	// Update title to reflect filename.
	void set_title();

	// Find window which has this file opened.
	// Returns nullptr if no window found.
	static MainWindow *find_window(const QFileInfo &f);

	// Update the recent file menu of all windows
	static void update_recent_files();

	void show_tooltip(const QString &s);
	void hide_tooltip();

	void selection_changed(bool is_empty);
};

#endif
