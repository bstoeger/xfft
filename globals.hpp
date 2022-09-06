// SPDX-License-Identifier: GPL-2.0
#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <cstddef>
#include <QString>

class Globals {
	static QString last_image;
	static QString last_save_image;
protected:
	friend int main(int argc, char **argv);
public:
	static bool debug_mode;
	static QString get_file_directory();
	static void set_last_file(const QString &);

	static QString get_last_image_directory();
	static void set_last_image(const QString &);

	static QString get_last_save_image_directory();
	static void set_last_save_image(const QString &);

	static QStringList get_recent_files();
};

#endif
