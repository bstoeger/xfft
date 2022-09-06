// SPDX-License-Identifier: GPL-2.0
#include "globals.hpp"
#include "mainwindow.hpp"

#include <QApplication>
#include <QDate>
#include <QSettings>

#include <iostream>

int main(int argc, char *argv[])
{
	Q_INIT_RESOURCE(xfft);
	QApplication app(argc, argv);
	QCoreApplication::setApplicationName("FFT");
	QCoreApplication::setOrganizationName("TU Wien");
	QCoreApplication::setOrganizationDomain("crystallography.at");
	QCoreApplication::setApplicationVersion(QT_VERSION_STR);

	// Parse options until we reach the first "--", which means that only filenames follow.
	QStringList args = QCoreApplication::arguments();
	args.removeFirst();
	std::vector<QString> filenames;
	bool no_options = false;
	for (const QString &arg: qAsConst(args)) {
		if (arg.isEmpty())
			continue;
		if (!no_options && arg[0] == '-') {
			if (arg == "-debug")
				Globals::debug_mode = true;
			else if (arg == "--")
				no_options = true;
			else
				std::cerr << "Unknown option: " << arg.toStdString();
		} else {
			filenames.push_back(arg);
		}
	}

	if (filenames.empty()) {
		// Open a window with default settings
		MainWindow *w = new MainWindow(nullptr);
		w->show();
	} else {
		for (const QString &filename: qAsConst(filenames)) {
			MainWindow *w = new MainWindow(nullptr);
			w->open(filename);
			w->show();
		}
	}

	return app.exec();
}
