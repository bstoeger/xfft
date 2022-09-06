// SPDX-License-Identifier: GPL-2.0
#include "globals.hpp"
#include "mainwindow.hpp"

#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

#include <algorithm>
#include <functional>

bool Globals::debug_mode = false;

QString Globals::get_file_directory()
{
	QStringList last_files = get_recent_files();

	if (last_files.isEmpty())
		return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
	QFileInfo info(last_files[0]);
	return info.path();
}

static bool same_file(const QFileInfo &info, const QString &fn)
{
	QFileInfo info2(fn);
	return info.exists() && info2.exists() && info == info2;
}

void Globals::set_last_file(const QString &fn)
{
	QStringList last_files = get_recent_files();
	QSettings settings;
	QFileInfo info(fn);

	auto it = std::find_if(last_files.begin(), last_files.end(),
			       [&info](const QString &fn)
			       { return same_file(info, fn); });

	if (it != last_files.end()) {
		std::rotate(last_files.begin(), it, std::next(it));
	} else {
		last_files.push_front(fn);
		while (last_files.size() > 10)
			last_files.pop_back();
	}

	settings.setValue("last_files", last_files);

	// Make the recent file visible in the recent file menus
	MainWindow::update_recent_files();
}

QStringList Globals::get_recent_files()
{
	QSettings settings;
	return settings.value("last_files").toStringList();
}

static QString path_of_file(const QString &f)
{
	QFileInfo info(f);
	return info.path();
}

QString Globals::get_last_image_directory()
{
	QSettings settings;
	QString last_image = settings.value("last_image").toString();
	return last_image.isEmpty() ?
		QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) :
		path_of_file(last_image);
}

void Globals::set_last_image(const QString &s)
{
	QSettings settings;
	settings.setValue("last_image", s);
}

QString Globals::get_last_save_image_directory()
{
	QSettings settings;
	QString last_save_image = settings.value("last_save_image").toString();
	return last_save_image.isEmpty() ?
		QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) :
		path_of_file(last_save_image);
}

void Globals::set_last_save_image(const QString &s)
{
	QSettings settings;
	settings.setValue("last_save_image", s);
}
