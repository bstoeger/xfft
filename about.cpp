// SPDX-License-Identifier: GPL-2.0
#include "about.hpp"
#include "version.hpp"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTextBrowser>

static QString about_text = QStringLiteral(R"(
<i>xfft</i> v%1<br/>
<br/>
An interactive tool to demonstrate the properties of the Fourier transform as used in crystallography.<br/>
<br/>
Source code:
<ul>
	<li><a href="https://github.com/bstoeger/xfft/">Github</a></li>
</ul>
Based on:
<ul>
	<li><a href="https://www.qt.io/">The Qt toolkit</a></li>
	<li><a href="https://www.fftw.org/">The FFTW library</a></li>
	<li><a href="https://www.boost.org/">The boost C++ libraries</a></li>
</ul>
)");

class AboutDialog : public QDialog {
public:
	AboutDialog(bool gpl, QWidget *parent);
};

static QString gpl_text()
{
	QFile file(":/LICENSE");
	file.open(QIODevice::ReadOnly);
	return QString(file.readAll());
}

AboutDialog::AboutDialog(bool gpl, QWidget *parent) : QDialog(parent)
{
	setWindowTitle(gpl ? "License" : "About xfft");
	QVBoxLayout *layout = new QVBoxLayout(this);

	QTextBrowser *text = new QTextBrowser(this);
	layout->addWidget(text);
	text->setMinimumSize(600, gpl ? 600 : 400);
	text->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
	if (gpl)
		text->setPlainText(gpl_text());
	else
		text->setHtml(about_text.arg(version));
	text->setOpenExternalLinks(true);

	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	layout->addWidget(buttons);

	QPushButton *ok = buttons->addButton(QDialogButtonBox::Ok);
	connect(ok, &QPushButton::clicked, this, [this]() { accept(); });
}

void show_about_dialog(QWidget *parent)
{
	AboutDialog dialog(false, parent);
	dialog.exec();
}

void show_gpl_dialog(QWidget *parent)
{
	AboutDialog dialog(true, parent);
	dialog.exec();
}
