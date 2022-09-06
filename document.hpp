// SPDX-License-Identifier: GPL-2.0
#ifndef DOCUMENT_HPP
#define DOCUMENT_HPP

#include "topological_order.hpp"
#include "operator_list.hpp"

#include <cstddef>
#include <memory>

#include <QString>

class MainWindow;
class QAction;
class QUndoStack;
class QUndoCommand;

class Document {
	bool save(const QString &fn, MainWindow *w, Scene *scene); // Returns true if file actually saved
	void set_filename(const QString &fn);

	// Get directory either from current file or from globals
	QString get_directory() const;

	std::unique_ptr<QUndoStack> undo_stack;
	void place_command_internal(QUndoCommand *cmd); // Takes ownership of cmd
signals:
	bool changedChanged(); // Changed status of the document changed.
public:
	TopologicalOrder topo;
	OperatorList operator_list;

	static const constexpr size_t supported_fft_sizes[] = {
		128, 256, 512, 1024
	};

	// Copy defaults from previous document of not nullptr
	Document(const Document *previous_document, MainWindow &w);
	~Document();

	QString filename;	// If empty: unnamed document
	QString name;
	size_t fft_size;

	bool save(MainWindow *w, Scene *scene);
	bool save_as(MainWindow *w, Scene *scene);
	void load(MainWindow *w, Scene *scene);
	void load(MainWindow *w, Scene *scene, const QString &filename);

	// Delete all objects
	void clear();

	// Change fft size, return true on success
	bool change_fft_size(size_t size, Scene *scene);

	QAction *undo_action(QObject *parent) const;
	QAction *redo_action(QObject *parent) const;
	bool changed() const;

	template<typename Command, typename... Args>
	void place_command(Args&&... args);
};

#include "document_impl.hpp"

#endif
